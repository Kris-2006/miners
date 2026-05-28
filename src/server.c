#include "../include/server.h"
#include "common.h"
#include "hashmap.h"
#include "protocol.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int client_add(server_ctx_t *ctx, int fd) {

  if (fd <= 0)
    return -1;

  if (!ctx) {
    LOG_ERR("NULL");
    return -1;
  }

  client_t *client = (client_t *)malloc(sizeof(client_t));
  if (!client) {
    LOG_ERR("NULL");
    return -1;
  }
  client->fd = fd;
  memset(client->username, 0, 32);
  client->auth = 0;
  client->recv_len = 0;

  int status = hm_put(ctx->by_fd, &client->fd, client);
  if (status == -1) {
    LOG_ERR("cannot add client");
    return -1;
  }
  // yet to implement auth.. and byname hm ..
  return 0;
}

void client_remove(server_ctx_t *ctx, unsigned int fd) {

  if (fd <= 0) {
    LOG_ERR("invalid fd");
    return;
  }

  if (!ctx) {
    LOG_ERR("ctx ");
    return;
  }

  client_t *client = hm_get(ctx->by_fd, &fd);
  hm_remove(ctx->by_fd, &fd);
  if (!client)
    return;
  if (client->auth == 1)
    hm_remove(ctx->by_name, client->username);
  free(client);
}

client_t *clientfd_find(server_ctx_t *ctx, int fd) {

  if (!ctx) {
    LOG_ERR("Null");
    return NULL;
  }

  if (fd <= 0) {
    return NULL;
  }

  return hm_get(ctx->by_fd, &fd);
}

client_t *clientname_find(server_ctx_t *ctx, const char *name) {

  if (!ctx) {
    LOG_ERR("Null");
    return NULL;
  }

  if (!name) {
    LOG_ERR("Name is NULL");
    return NULL;
  }

  return (hm_get(ctx->by_name, (void *)name));
}

void broadcast(server_ctx_t *ctx, const msg_t *msg, int exclude_fd) {

  if (!ctx) {
    LOG_ERR("ctx null");
    return;
  }
  if (!msg) {
    LOG_ERR("msg n't");
    return;
  }

  char buf[BUFSIZ];
  size_t size = msg_pack(msg, (uint8_t *)buf, BUFSIZ);
  if (size <= 0)
    return;

  for (int i = 0; i < ctx->by_fd->capacity; i++) {

    hm_entry_t *entry = &ctx->by_fd->entries[i];
    if (entry->key == NULL || ((client_t *)entry->value)->fd == exclude_fd)
      continue;
    send(((client_t *)entry->value)->fd, buf, size, 0);
  }
}

server_ctx_t *server_ctx_init() {

  server_ctx_t *ctx = (server_ctx_t *)malloc(sizeof(server_ctx_t));
  if (!ctx) {
    LOG_ERR("Server Ctx init");
    return NULL;
  }
  ctx->epoll_fd = -1;
  ctx->listen_fd = -1;

  hm_t *by_name = hm_create(MAX_EVENTS, fnv1a_str, cmp_str);
  if (!by_name) {
    LOG_ERR("by_name - malloc");
    free(ctx);
    ctx = NULL;
    return NULL;
  }

  ctx->by_name = by_name;

  hm_t *by_fd = hm_create(MAX_EVENTS, fnv1a_int, cmp_int);
  if (!by_fd) {
    LOG_ERR("by_fd - malloc");
    free(by_name);
    free(ctx);
    ctx = NULL;
    return NULL;
  }
  ctx->by_fd = by_fd;

  return ctx;
}

void server_ctx_free(server_ctx_t *ctx) {
  if (!ctx)
    return;

  hm_destroy(ctx->by_name);
  hm_destroy(ctx->by_fd);
  free(ctx);

  return;
}

void client_auth(server_ctx_t *ctx, client_t *c, const char *uname) {

  if (!ctx || !c || !uname) {
    LOG_ERR("cauth - null value");
    return;
  }

  if (strlen(uname) >= 32) {
    LOG_ERR("cauth - long string");
    return;
  }

  client_t *client = clientname_find(ctx, uname);
  if (client && client != c) {
    msg_t msg;
    msg.TYPE = MSG_DISCONNECT;
    strcpy(msg.sender, "SERVER");
    strcpy(msg.payload, "new session");
    msg.len = strlen("new session");
    char buf[BUFSIZ];
    size_t size = msg_pack(&msg, (uint8_t *)buf, BUFSIZ);
    buf[size] = 0;

    send(client->fd, buf, size, 0);

    epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
    client_remove(ctx, client->fd);
    // close(client->fd);
  }

  strncpy(c->username, uname, 31);
  c->auth = 1;
  hm_put(ctx->by_name, uname, c);

  return;
}

void server_shutdown(server_ctx_t *server) {

  if (!server || !server->by_fd)
    return;

  int cap = server->by_fd->capacity;
  hm_entry_t *entries = server->by_fd->entries;

  msg_t discon_msg;
  discon_msg.TYPE = MSG_DISCONNECT;
  strcpy(discon_msg.sender, "SERVER0xdead");
  discon_msg.len = 0;
  char msg_buff[MAX_BUFF];
  int size = msg_pack(&discon_msg, msg_buff, MAX_BUFF);

  for (int i = 0; i < cap; i++) {
    if (entries[i].state == 1) {
      send(*(int *)entries[i].key, msg_buff, size, 0);
      close(*(int *)entries[i].key);
    }
  }
}
