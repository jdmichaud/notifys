#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "defines.h"
#include "notify.h"
#include "server.h"

#define BUFFER_SIZE 4096
#define SOME_FILENAME "/tmp/test/toto"

#define STDIN_INDEX  0
#define NOTIFY_INDEX 1
#define SOCKET_INDEX 2

typedef struct fd_list {
  uint32_t fd;
  struct fd_list *next;
} fd_list_t;

void usage(char *prgname) {
  fprintf(stderr, "usage: %s <address> <port> <path> [path ...]\n", prgname);
}

void add_fd(fd_list_t **fd_list, int32_t fd) {
  fd_list_t *node = malloc(sizeof (node));
  if (node == NULL) {
    perror("malloc");
  }
  node->fd = fd;
  node->next = NULL;

  if (*fd_list != NULL) {
    node->next = *fd_list;
  }
  *fd_list = node;
}

void remove_fd(fd_list_t **fd_list, fd_list_t *node) {
  if (*fd_list == node) {
    *fd_list = (*fd_list)->next;
  } else {
    for (fd_list_t *ptr = *fd_list; ptr->next != NULL; ptr = ptr->next) {
      if (ptr->next == node) {
        ptr->next = ptr->next->next;
      }
    }
  }
  free(node);
}

void install_fds(struct pollfd *fds, int32_t wfd, int32_t sfd) {
  // Poll standard input
  fds[STDIN_INDEX].fd = STDIN_FILENO;
  fds[STDIN_INDEX].events = POLLIN;
  fds[STDIN_INDEX].revents = 0;
  // Poll watches
  fds[NOTIFY_INDEX].fd = wfd;
  fds[NOTIFY_INDEX].events = POLLIN;
  fds[NOTIFY_INDEX].revents = 0;
  // Poll socket
  fds[SOCKET_INDEX].fd = sfd;
  fds[SOCKET_INDEX].events = POLLIN;
  fds[SOCKET_INDEX].revents = 0;
}

int8_t watch(int32_t wfd, watched_path_t *wp, int32_t sfd) {
  struct pollfd fds[SOCKET_INDEX + 1];
  event_t events[MAX_SIMULTANEOUS_EVENTS];
  fd_list_t *client_fds = NULL;
  install_fds(fds, wfd, sfd);
  while (1) {
    memset(events, 0, MAX_SIMULTANEOUS_EVENTS * sizeof (event_t));
    if (poll(fds, SOCKET_INDEX + 1, -1) <= 0) {
      if (errno == EINTR) {
        continue; // A signal unblocked poll, just ignore it
      } else {
        return -1; // A real error.
      }
    } else {
      // Check stdin is ready to read
      if (fds[STDIN_INDEX].revents & POLLIN) {
        // Stop on key pressed
        break;
      }
      if (fds[NOTIFY_INDEX].revents & POLLIN) {
        int32_t nb_events = 0;
        if ((nb_events = parse_events(wfd, wp, events)) < 0) return ERROR;
        if (nb_events) {
          // We don't manage multiple simultaneous events for now
          for (fd_list_t *head = client_fds; head != NULL; head = client_fds) {
            answer(head->fd, events[0].path, events[0].event);
            remove_fd(&client_fds, head);
          }
          for (uint32_t i = nb_events; i != 0; --i) {
            free(events[i].path);
            free(events[i].event);
          }
        }
      }
      if (fds[SOCKET_INDEX].revents & POLLIN) {
        int32_t cfd = serve(sfd);
        add_fd(&client_fds, cfd);
        char buffer[BUFFER_SIZE];
        // Ignore the request content
        if (read_request(cfd, buffer, BUFFER_SIZE)) return ERROR;
      }
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    usage(argv[0]);
    return ERROR;
  }
  // Network setup
  struct sockaddr_in addr;
  if (create_addr(argv, &addr)) {
    return ERROR;
  }
  int32_t sfd;
  if ((sfd = prepare_socket(addr)) < 0) {
    return ERROR;
  }
  // Allocate a watch descriptors array with an additional NULL entry
  // at the end so the size doesn't have to be passed around
  watched_path_t *wp = calloc(argc - 2, sizeof (watched_path_t));
  if (wp == NULL) {
    perror("calloc");
    return ERROR;
  }
  memset(wp, 0, (argc - 2) * sizeof (watched_path_t));
  // Initialize the watch descriptors
  int32_t wfd; // watch file descriptor
  if ((wfd = init(argc - 3, &argv[3], wp)) < 0) {
    return ERROR;
  }
  // poll the various file descriptors and call the appropriate actions
  watch(wfd, wp, sfd);
  // Cleanup
  free(wp);
  close(wfd);
  close(sfd);
  return 0;
}
