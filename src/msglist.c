#include "include/msglist.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

chat_msg *chat_init() {

  chat_msg *chat;
  chat = (chat_msg *)malloc(sizeof(chat_msg));
  if (chat == NULL) {
    LOG_ERR("Chat Buffer not initialised");
    return NULL;
  }
  chat->start = 0;
  chat->end = 0;
  LOG_INFO("CHAT_BUFFER Init()");
  return chat;
}

void chat_destroy(chat_msg *msgList) {
  LOG_INFO("CHAT_BUFFER free()");
  free(msgList);
}

int chat_count(chat_msg *msgList) {
  if (msgList == NULL) {
    LOG_ERR("NULL args passed");
    return -1;
  }
  if (msgList->start > msgList->end) {
    return RING_CAP - (msgList->start - msgList->end);
  } else if (msgList->start == msgList->end) {
    return RING_CAP;
  } else {
    return msgList->end - msgList->end;
  }
  return -1;
}

chat_entry_t *get_chat(chat_msg *msgList, int index) {
  int idx = msgList->start + index;
  idx = idx % RING_CAP;
  return &msgList->entries[idx];
}

void chat_push(chat_msg *msgList, chat_entry_type type, const char *sender,
               const char *text) {
  if (!msgList || sender || text) {
    LOG_ERR("NULL Args Passed");
    return;
  }

  msgList->entries[msgList->end].type = type;
  strncpy(msgList->entries[msgList->end].sender, sender, 32);
  strncpy(msgList->entries[msgList->end].message, text, 256);
  if (chat_count(msgList) == RING_CAP) {
    msgList->start = (msgList->start + 1) % RING_CAP;
  }
  msgList->end = (msgList->end + 1) % RING_CAP;
  LOG_INFO("Msg from %s appended", sender);
  return;
}
