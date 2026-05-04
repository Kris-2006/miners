#include "../../include/common.h"
#include "../../include/server.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void *get_in_addr(struct sockaddr_storage *s) {
  if (s->ss_family == AF_INET)
    return &(((struct sockaddr_in *)s)->sin_addr);

  return &(((struct sockaddr_in6 *)s)->sin6_addr);
}

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void handle_message(server_ctx_t *ctx, client_t *c, msg_t *msg) {
  if (!ctx || !c || !msg) {
    LOG_ERR("hlogin-NULL");
    return;
  }

  if (msg->TYPE == MSG_LOGIN) {
    client_auth(ctx, c, msg->sender);
    msg_t s = {.TYPE = MSG_STATUS};
    strcpy(s.payload, "login_ok");
    s.len = strlen("login_ok");
    strcpy(s.sender, "SERVER");
    char buf[BUFSIZ];
    size_t size = msg_pack(&s, (uint8_t *)buf, BUFSIZ);
    buf[size] = 0;
    send(c->fd, buf, size, 0);
    LOG_INFO("Successful login: %s", msg->sender);

    msg_t bcast = {.TYPE = MSG_STATUS};
    strcpy(bcast.sender, "SERVER");
    sprintf(bcast.payload, "%s joined the miners.\n", c->username);
    bcast.len = strlen(bcast.payload);
    broadcast(ctx, &bcast, c->fd);
    LOG_INFO("Broadcasted\n");

  } else if (msg->TYPE == MSG_TEXT) {

    if (!c->auth)
      return;
    broadcast(ctx, msg, c->fd);
    LOG_INFO("Broadcasted message from %s:%s", msg->sender, msg->payload);
  }
}
volatile sig_atomic_t g_running = 1;

void h_sigint(int sig) {
  (void)sig;
  g_running = 0;
}

int main() {
  int fd, status;
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = h_sigint;
  sigaction(SIGINT, &sa, NULL);

  // filling in ip..
  if ((status = getaddrinfo(NULL, DEF_PORT, &hints, &res)) != 0) {
    LOG_ERR("%s", gai_strerror(status));
    return -1;
  }

  // socket creation
  if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
    LOG_ERR("socket failed");
    return -1;
  }

  // port reuse.
  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {

    LOG_ERR("addr use");
    return -1;
  }

  // bind

  if ((status = bind(fd, res->ai_addr, res->ai_addrlen)) == -1) {
    LOG_ERR("bind");
    freeaddrinfo(res);
    close(fd);
    return -1;
  }

  // listen

  if ((status = listen(fd, BACKLOG)) == -1) {
    LOG_ERR("listen");
    freeaddrinfo(res);
    close(fd);
    return -1;
  }

  server_ctx_t *server = server_ctx_init();
  if (server == NULL) {
    LOG_ERR("server allocation failed.");
    freeaddrinfo(res);
    close(fd);
    return -1;
  }

  server->listen_fd = fd;

  int epfd = epoll_create1(0);
  if (epfd == -1) {
    LOG_ERR("epoll instance error");
    server_ctx_free(server);
    freeaddrinfo(res);
    close(fd);
    return -1;
  }
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = server->listen_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
  server->epoll_fd = epfd;

  struct epoll_event events[MAX_EVENTS];

  while (g_running) {
    struct sockaddr_storage conn;
    char ip[INET6_ADDRSTRLEN];
    socklen_t len = sizeof conn;
    // int connfd = accept(fd, (struct sockaddr *)&conn, &len);
    //
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      LOG_ERR("epoll_wait");
      break;
    }
    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == server->listen_fd) {
        // handle new connections..
        int connfd = accept(server->listen_fd, (struct sockaddr *)&conn, &len);
        inet_ntop(conn.ss_family, get_in_addr(&conn), ip, INET6_ADDRSTRLEN);
        LOG("Connection", " From [+]%s[+]", ip);
        set_nonblocking(connfd);
        client_add(server, connfd);

        ev.events = EPOLLIN;
        ev.data.fd = connfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

      } else if (events[i].events & EPOLLIN) {

        client_t *c = clientfd_find(server, events[i].data.fd);
        // char buf[BUFSIZ];
        int size = recv(events[i].data.fd, c->recv_buf + c->recv_len,
                        2 * MAX_BUFF - c->recv_len, 0);
        if (size <= 0) {
          ev.events = 0;
          ev.data.fd = events[i].data.fd;
          LOG_ERR("recv");
          epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
          close(events[i].data.fd);
          client_remove(server, events[i].data.fd);
          continue;
        }

        c->recv_len += size;
        while (c->recv_len >= 35) {

          uint16_t net_len;
          memcpy(&net_len, c->recv_buf + 33, 2);
          uint16_t payload_len = ntohs(net_len);

          int total_len = 35 + payload_len;

          int remaining = c->recv_len - total_len;
          if (remaining < 0)
            break;

          msg_t msg;
          int e = msg_unpack((uint8_t *)c->recv_buf, total_len, &msg);
          if (e == -1) {
            LOG_ERR("msg_unpack error\n");
            break;
          }

          handle_message(server, c, &msg);
          memmove(c->recv_buf, c->recv_buf + total_len, remaining);
          c->recv_len = remaining;
        }
        // msg_t msg;
        // int e = msg_unpack((uint8_t *)c->recv_buf, c->recv_len, &msg);
        // launch message handler...
        // if (e == 0)
        //   handle_message(server, c, &msg);
        //   send(events[i].data.fd, buf, size, 0);
      }
    }
  }

  server_ctx_free(server);
  freeaddrinfo(res);
  close(fd);
}
