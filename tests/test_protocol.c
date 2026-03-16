#include "../include/protocol.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  msg_t org = {.TYPE = MSG_TEXT,
               .sender = "Alice",
               .payload = "Hello,World!",
               .len = strlen("Hello,World!")};

  uint8_t buf[BUFSIZ];
  int packed = msg_pack(&org, buf, BUFSIZ);
  assert(packed > 0);

  msg_t unpacked;
  assert(msg_unpack(buf, BUFSIZ, &unpacked) == 0);

  assert(unpacked.TYPE == org.TYPE);
  assert(unpacked.len == org.len);
  assert(memcmp(unpacked.sender, org.sender, 32) == 0);
  assert(memcmp(unpacked.payload, org.payload, unpacked.len) == 0);

  LOG_INFO("ALL TESTS ARE COMPLETED.");
  return 0;
}
