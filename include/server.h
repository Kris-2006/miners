#ifndef MINERS_SERVER_H
#define MINERS_SERVER_H

#define MAX_EVENTS 64
#include "common.h"
#include "hashmap.h"
#include "protocol.h"

typedef struct {

  int fd;
  char username[32];
  int auth;
  int recv_len;
  char recv_buf[2 * MAX_BUFF];
} client_t;

typedef struct {

  int listen_fd;
  int epoll_fd;
  hm_t *by_name;
  hm_t *by_fd;

} server_ctx_t;

int client_add(server_ctx_t *ctx, int fd);
void client_remove(server_ctx_t *ctx, unsigned int fd);

void client_auth(server_ctx_t *ctx, client_t *c, const char *uname);
client_t *clientfd_find(server_ctx_t *ctx, int fd);
client_t *clientname_find(server_ctx_t *ctx, const char *name);
void broadcast(server_ctx_t *ctx, const msg_t *msg, int exclude_fd);

server_ctx_t *server_ctx_init();
void server_ctx_free(server_ctx_t *ctx);
void server_shutdown(server_ctx_t *server);

#endif
