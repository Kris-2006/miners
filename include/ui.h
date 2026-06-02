#ifndef MINERS_UI_H
#define MINERS_UI_H

#include <ncurses.h>

#include "msglist.h"

typedef struct ui_ctx ui_t;

ui_t* ui_init(const char* username);
void ui_destroy(ui_t* ui);

void ui_refresh(ui_t* ui, chat_msg* msgList);
const char* ui_handle_key(ui_t* ui);

void ui_resize(ui_t* ui);

#endif
