#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <poll.h>

#include "defines.h"
#include "notify.h"

int32_t init(int argc, char **argv, watched_path_t *wp) {
  int32_t wfd = inotify_init1(IN_NONBLOCK); // watch file decriptor
  if (wfd < 0) {
    perror("inotify_init1");
    return -1;
  }

  for (uint8_t i = 0; i < argc; ++i) {
    wp[i].watch_descriptor = inotify_add_watch(wfd, argv[i], IN_CREATE | IN_DELETE);
    if (wp[i].watch_descriptor < 0) {
      perror("inotify_add_watch");
      return -1;
    }
    wp[i].path = argv[i];
    fprintf(stdout, "watching %s\n", argv[i]);
  }

  return wfd;
}

int8_t handle_inotify_event(struct inotify_event *event, watched_path_t *wp,
  event_t *converted_event)
{
  char *event_name = "unknown event";
  char *path = "unknown path";
  if (event->mask & IN_CREATE) {
    event_name = "IN_CREATE";
  } else if (event->mask & IN_DELETE) {
    event_name = "IN_DELETE";
  }
  // We suppose the watched_path array has a last entry with path == NULL
  for (uint8_t i = 0; wp[i].path; ++i) {
    if (event->wd == wp[i].watch_descriptor) {
      path = wp[i].path;
      break;
    }
  }
  converted_event->path =
    (char *) calloc(strlen(path) + strlen(event->name) + 1, sizeof (char));
  sprintf(converted_event->path, "%s%s%s",
    path,
    path[strlen(path) - 1] == '/' ? "" : "/",
    event->name);
  converted_event->event = strdup(event_name);

  return 0;
}

int32_t parse_events(uint32_t wfd, watched_path_t *wp, event_t *events) {
  // Notify file descriptors cannot be partially read, otherwise an
  // 'Illegal argument' exception during the call to read. So we need to
  // have a big enough buffer. We consider that we can't have more than
  // MAX_SIMULTANEOUS_EVENTS events at once here,
  char buf[sizeof (struct inotify_event) * MAX_SIMULTANEOUS_EVENTS]
  // This ensure alignment is the same as the inotify_event
  // (see inotify manual page)
       __attribute__ ((aligned(__alignof__(struct inotify_event))));
  int32_t nb_events = 0;

  while (1) {
    // None blocking read
    ssize_t len = read(wfd, buf, sizeof (buf));
    if (len <= 0) {
      // EAGAIN means there was nothing to read. If read exits without
      // having read anything but EAGAIN is not set, there was an error.
      if (errno != EAGAIN) {
        perror("read");
        return -1;
      }
      // Nothing to read for now, get out of handle_events
      break;
    } else if ((size_t) len > sizeof (struct inotify_event)) {
      char *ptr = buf;
      while ((ptr - buf) < (ssize_t) (len - sizeof (struct inotify_event))) {
        handle_inotify_event((struct inotify_event*) ptr, wp, &events[nb_events++]);
        ptr += sizeof (struct inotify_event);
      }
    }
  }
  return nb_events;
}
