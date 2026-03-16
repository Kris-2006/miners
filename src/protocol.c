#include "../include/protocol.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

int msg_pack(const msg_t *msg, uint8_t *buf, size_t buf_size) {
  int total_length = 1 + 32 + 2 + sizeof(msg->payload);

  if (total_length > (int)buf_size)
    return -1;

  uint8_t *ptr = buf;
  *ptr++ = (uint8_t)msg->TYPE;

  memset(ptr, 0, 32);
  strncpy((char *)ptr, msg->sender, 31);

  ptr += 32;
  uint16_t net_len = htons(msg->len);

  memcpy(ptr, (char *)&net_len, 2);
  ptr += 2;

  memcpy(ptr, msg->payload, msg->len);
  ptr += msg->len;

  return (int)(ptr - buf);
}

int msg_unpack(const uint8_t *buf, size_t buf_size, msg_t *msg) {

  // header too small
  if (buf_size < 35)
    return -1;

  const uint8_t *ptr = buf;

  msg->TYPE = (msg_type_t)*ptr++;
  strncpy(msg->sender, (char *)ptr, 32);
  msg->sender[31] = '\0';
  ptr += 32;

  uint16_t net_len;
  memcpy(&net_len, ptr, 2);
  msg->len = ntohs(net_len);
  ptr += 2;

  if (msg->len > MAX_BUFF || (ptr + msg->len) > (buf + buf_size))
    return -1;

  memcpy(msg->payload, ptr, msg->len);

  return 0;
}
