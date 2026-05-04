#include "../../include/common.h"
#include "../../include/protocol.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void send_msg(int server_fd, const char *uname, const char *msg) {

  msg_t Message;
  Message.TYPE = MSG_TEXT;
  strncpy(Message.sender, uname, 32);
  strncpy(Message.payload, msg, 256);
  Message.len = strnlen(Message.payload, 256);

  char send_buf[256];
  int size;

  size = msg_pack(&Message, (uint8_t *)send_buf, BUFSIZ);
  char *ptr = send_buf;
  while (size > 0) {
    int sent = send(server_fd, ptr, size, 0);
    ptr = ptr + (char)sent;
    size -= sent;
  }
}

int user_login(int server_fd, const char *uname) {

  char login_status[256];
  msg_t msg_login;
  msg_login.TYPE = MSG_LOGIN;
  strcpy(msg_login.sender, uname);
  msg_login.len = 0;
  int size;
  size = msg_pack(&msg_login, (uint8_t *)login_status, 256);
  send(server_fd, login_status, size, 0);
  LOG_INFO("Sent Login..");
  size = recv(server_fd, login_status, sizeof(login_status), 0);
  if (size < 0) {
    LOG_ERR("recv-user_login");
    return -1;
  }
  msg_t login_cnf;
  if (msg_unpack((uint8_t *)login_status, sizeof(login_status), &login_cnf) ==
      0) {

    if (strcmp(login_cnf.payload, "login_ok") == 0)
      LOG_INFO("Login sucessful");
    else
      LOG_ERR("Login Unsucessful");
    return -1;
  } else
    LOG_ERR("Error: Invalid Message from server.");
  return -1;
}

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

  status = user_login(fd, "its'me");
  while (1)
    send_msg(fd, "its'me", "Hello World\n Maybe i was wrong !!\n.");

  char buf[MAX_BUFF];
  recv(fd, buf, MAX_BUFF, 0);
  buf[MAX_BUFF - 1] = 0;

  LOG("echo", " %s", buf);

  freeaddrinfo(hints);
  close(fd);

  return 0;
}
