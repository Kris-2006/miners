#ifndef MINERS_PROTOCOL_H
#define MINERS_PROTOCOL_H

#include "common.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef enum {
  MSG_TEXT = 0x01,
  MSG_LOGIN = 0x02,
  MSG_STATUS = 0x03,
  MSG_LIST = 0x04,
  MSG_DISCONNECT = 0xFF,
} msg_type_t;

typedef struct {
  msg_type_t TYPE;
  char sender[32];
  uint16_t len;
  char payload[MAX_BUFF];
} msg_t;

int msg_pack(const msg_t *msg, uint8_t *buf, size_t buf_size);

int msg_unpack(const uint8_t *buf, size_t buf_size, msg_t *msg);

#endif
