#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defines.h"
#include "server.h"

// Days of the week 3-letters abbreviations
static const char dow[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
// Months of the year 3-letters abbreviations
static const char moy[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int8_t create_addr(char **argv, struct sockaddr_in *addr) {
    // If localhost is provided, convert it to 127.0.0.1,
  // otherwise just use the address provided
  if (!strcmp(argv[1], "localhost")) {
    argv[1] = "127.0.0.1";
  }
  // Retrieve address
  struct in_addr inp;
  if (!inet_aton(argv[1], &inp)) {
    fprintf(stderr, "Invalid address: %s\n", argv[1]);
    return ERROR;
  }
  // Retrieve port number
  char *endptr;
  uint32_t portno = strtol(argv[2], &endptr, 10);
  if (argv[2] == endptr || portno > MAX_PORT_NO) {
    fprintf(stderr, "Invalid port: %s\n", argv[2]);
    return ERROR;
  }
  // Assign a name to the socket
  memset((void *) addr, 0, sizeof (addr));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inp.s_addr;
  addr->sin_port = htons(portno);

  return 0;
}

int32_t prepare_socket(struct sockaddr_in addr) {
  // Open a socket
  int32_t sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    perror("socket");
    return -1;
  }
  // Set the address/port associated to that socket reusable
  uint8_t t = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof (uint8_t));

  if (bind(sfd, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) != 0) {
    perror("bind");
    return -1;
  }
  // Listen only to one client
  if (listen(sfd, 1)) {
    perror("listen");
    return -1;
  }

  return sfd;
}

int32_t serve(uint8_t sfd) {
  // Accept the new connection
  int32_t cfd = accept(sfd, NULL, 0);
  return cfd;
}

int8_t read_request(uint32_t cfd, char *buffer, ssize_t buffer_size) {
  ssize_t tlen = 0; // total read length
  uint8_t request_incomplete = 1;
  while (request_incomplete && tlen < buffer_size) {
    ssize_t len = read(cfd, buffer + tlen, buffer_size - tlen);
    tlen += len;
    request_incomplete = buffer[tlen - 1] != '\n' && buffer[tlen - 2] != '\n';
  }
  if (request_incomplete) {
    fprintf(stderr, "request size overflow\n");
    return ERROR;
  }
  return 0;
}

int8_t answer(uint32_t cfd, char *path, char *event) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  // Create the body
  char body[MAX_BODY_SIZE];
  snprintf(body, MAX_BODY_SIZE, "{\"event\": \"%s\",\"file\":\"%s\"}", event, path);
  // Create the response containing the body
  char response[MAX_RESPONSE_SIZE];
  snprintf(response, MAX_RESPONSE_SIZE, "HTTP/1.0 200 OK\n");
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "Server: notifys %i.%i.%i\n",
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "Date: %s, %i %s %i %i:%i:%i GMT\n",
    dow[tm.tm_wday], tm.tm_mday, moy[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "Content-type: application/json\n");
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "Content-Length: %lu\n", strlen(body));
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "Last-Modified: %s, %i %s %i %i:%i:%i GMT\n",
    dow[tm.tm_wday], tm.tm_mday, moy[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "\n");
  snprintf(response + strlen(response),
    MAX_RESPONSE_SIZE - strlen(response),
    "%s", body);
  size_t tlen = 0;
  while (tlen < strnlen(response, MAX_RESPONSE_SIZE)) {
    ssize_t len = write(cfd, response, strlen(response));
    if (len < 0) {
      perror("write");
      close(cfd);
      return ERROR;
    }
    tlen += len;
  }
  // The client will close cfd
  return 0;
}
