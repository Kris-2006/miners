#ifndef MINERS_SERVER_H
#define MINERS_SERVER_H

#define MAX_EVENTS 64
#include "../../include/common.h"
#include "../../include/protocol.h"

typedef struct {
  int id;
  int fd;
  char username[32];
  int auth;
} client_t;

int client_add(int fd);
void client_remove(int fd);

client_t *client_find(int fd);

void broadcast(const msg_t *msg, int exclude_fd);

#endif
