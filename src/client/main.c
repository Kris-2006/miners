#include "../../include/common.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  if (argc < 2) {
    LOG_INFO("Usage: %s addr [port]", argv[0]);
    return -1;
  }

  const char *port = (argc == 3) ? argv[2] : DEF_PORT;

  int fd, status;
  struct addrinfo server_addr, *hints;
  memset(&server_addr, 0, sizeof server_addr);
  server_addr.ai_family = AF_INET;
  server_addr.ai_socktype = SOCK_STREAM;
  server_addr.ai_flags = AI_PASSIVE;

  status = getaddrinfo(argv[1], port, &server_addr, &hints);
  if (status != 0) {
    LOG_ERR("%s", gai_strerror(status));
    return -1;
  }

  fd = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
  if (fd == -1) {
    LOG_ERR("Socket failed");
    return -1;
  }

  status = connect(fd, (struct sockaddr *)hints->ai_addr, hints->ai_addrlen);

  if (status == -1) {
    LOG_ERR("connect");
    close(fd);
    return -1;
  }

  const char *msg = "Hello , World!!";
  send(fd, msg, strlen(msg), 0);

  char buf[MAX_BUFF];
  recv(fd, buf, MAX_BUFF, 0);
  buf[MAX_BUFF - 1] = 0;

  LOG("echo", " %s", buf);

  freeaddrinfo(hints);
  close(fd);

  return 0;
}
