#ifndef __NOTIFY_H__
#define __NOTIFY_H__

#include <sys/inotify.h>
#include <poll.h>

#define MAX_SIMULTANEOUS_EVENTS 256

typedef struct {
  int8_t watch_descriptor;
  char *path;
} watched_path_t;

typedef struct {
  char *path;
  char *event;
} event_t;

int32_t init(int argc, char **argv, watched_path_t *wp);
int8_t handle_inotify_event(struct inotify_event *event, watched_path_t *wp,
  event_t *converted_events);
int32_t parse_events(uint32_t wfd, watched_path_t *wp, event_t *events);

#endif // __NOTIFY_H__