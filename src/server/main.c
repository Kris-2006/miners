#include "../../include/common.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void *get_in_addr(struct sockaddr_storage *s) {
  if (s->ss_family == AF_INET)
    return &(((struct sockaddr_in *)s)->sin_addr);

  return &(((struct sockaddr_in6 *)s)->sin6_addr);
}

int main() {
  int fd, status;
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

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

  while (1) {
    struct sockaddr_storage conn;
    char ip[INET6_ADDRSTRLEN];
    socklen_t len = sizeof conn;
    int connfd = accept(fd, (struct sockaddr *)&conn, &len);

    inet_ntop(conn.ss_family, get_in_addr(&conn), ip, INET6_ADDRSTRLEN);
    LOG("Connection", " From [+]%s[+]", ip);

    char buf[BUFSIZ];
    int size = recv(connfd, buf, BUFSIZ, 0);
    if (size <= 0) {
      LOG_ERR("recv");
      close(connfd);
      continue;
    }
    buf[size] = 0;

    send(connfd, buf, size, 0);
    close(connfd);
  }

  freeaddrinfo(res);
  close(fd);
}
