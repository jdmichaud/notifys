#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/types.h>
#include <sys/socket.h>

#define MAX_PORT_NO 65535
// On linux PATH_MAX is 4096. With the HTTP header taking a little more space
// let's allocate to the power of two immediatly above
#define MAX_RESPONSE_SIZE 8192
// Maximum size of the json. 4096 (PATH_MAX) + the 30 char for the
// surrouding json
#define MAX_BODY_SIZE 4096+30

int8_t create_addr(char **argv, struct sockaddr_in *addr);
int32_t prepare_socket(struct sockaddr_in addr);
int32_t serve(uint8_t sfd);
int8_t read_request(uint32_t cfd, char *buffer, ssize_t buffer_size);
int8_t answer(uint32_t cfd, char *path, char *event);

#endif // __SERVER_H__