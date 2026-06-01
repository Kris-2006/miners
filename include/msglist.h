#ifndef MINERS_MSGLIST_H
#define MINERS_MSGLIST_H

#define RING_CAP 256

typedef enum {
  CHAT_TEXT,
  CHAT_STATUS,
  CHAT_ERR,
} chat_entry_type;

typedef struct {
  chat_entry_type type;
  char sender[32];
  char message[256];
} chat_entry_t;

typedef struct {
  chat_entry_t entries[RING_CAP];
  int start;
  int end;
} chat_msg;

chat_msg *chat_init();
void chat_destroy(chat_msg *msgList);

int chat_count(chat_msg *msgList);
chat_entry_t *get_chat(chat_msg *msgList, int index);

void chat_push(chat_msg *msgList, chat_entry_type type, const char *sender,
               const char *text);

#endif
