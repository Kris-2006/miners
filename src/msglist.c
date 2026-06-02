#include "msglist.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"

chat_msg* chat_init() {
    chat_msg* chat;
    chat = (chat_msg*)malloc(sizeof(chat_msg));
    if (chat == NULL) {
        LOG_ERR("Chat Buffer not initialised");
        return NULL;
    }
    chat->count = 0;
    chat->head = 0;
    LOG_INFO("CHAT_BUFFER Init()");
    return chat;
}

void chat_destroy(chat_msg* msgList) {
    LOG_INFO("CHAT_BUFFER free()");
    free(msgList);
}

int chat_count(chat_msg* msgList) {
    if (msgList == NULL) {
        LOG_ERR("NULL args passed");
        return -1;
    }
    return msgList->count;
}

chat_entry_t* get_chat(chat_msg* msgList, int index) {
    return &msgList
                ->entries[(index - msgList->count + msgList->head + RING_CAP) %
                          RING_CAP];
}

void chat_push(chat_msg* msgList, chat_entry_type type, const char* sender,
               const char* text) {
    if (!msgList || !sender || !text) {
        LOG_ERR("NULL Args Passed");
        return;
    }
    chat_entry_t* e = &msgList->entries[msgList->head];
    e->type = type;
    strncpy(e->sender, sender, 31);
    e->sender[31] = '\0';
    strncpy(e->message, text, 255);
    e->message[255] = '\0';
    msgList->head = (msgList->head + 1) % RING_CAP;
    if (chat_count(msgList) < RING_CAP) msgList->count++;
    // LOG_INFO("Msg from %s appended", sender);
    return;
}
