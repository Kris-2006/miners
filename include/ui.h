#ifndef MINERS_UI_H
#define MINERS_UI_H

#include "include/msglist.h"
#include <ncurses.h>

typedef struct ui_ctx ui_t;

ui_t *ui_init(const char *username);
void ui_destroy(ui_t *ui);

void ui_refresh(ui_t *ui, chat_msg *msgList);

#endif
