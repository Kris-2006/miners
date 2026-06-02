#include "ui.h"

#include <inttypes.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "common.h"
#include "msglist.h"

struct ui_ctx {
    WINDOW* chat_win;
    WINDOW* input_win;
    WINDOW* title_win;

    int rows, cols;
    char input_buf[256];
    int input_len;

    char username[32];
};

enum {
    CP_OTHER = 1,
    CP_SELF,
    CP_STATUS,
    CP_ERR,
};

static bool init_colors();

ui_t* ui_init(const char* username) {
    /* Initialises the ncurses win with predetermined rows x cols
     * doesn't initialise any other method on each scr.
     * uses following border ("-","-","|","|","+","+","+","+")
     * but in method ui_refresh()
     */

    if (!username) {
        LOG_ERR("Invalid Username passed");
        return NULL;
    }

    ui_t* ui;
    ui = (ui_t*)malloc(sizeof(ui_t));
    if (ui == NULL) {
        free(ui);
        LOG_ERR("Malloc-fail");
        return NULL;
    }

    strncpy(ui->username, username, 32);
    ui->input_len = 0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    ui->rows = rows;
    ui->cols = cols;

    if (!init_colors()) {
        endwin();
        return NULL;
    }

    ui->chat_win = newwin(ui->rows - 6, ui->cols, 4, 0);
    if (!ui->chat_win) {
        free(ui);
        LOG_ERR("CHAT-WINDOW failed");
        return NULL;
    }
    ui->title_win = newwin(3, ui->cols, 0, 0);
    if (!ui->title_win) {
        delwin(ui->chat_win);
        free(ui);
        LOG_ERR("TITLE-WINDOW failed");
        return NULL;
    }
    ui->input_win = newwin(3, ui->cols, ui->rows - 3, 0);
    if (!ui->input_win) {
        delwin(ui->chat_win);
        delwin(ui->title_win);
        free(ui);
        LOG_ERR("INPUT_WIN failed");
        return NULL;
    }
    // move(cols - 3, 2);
    return ui;
}

void ui_destroy(ui_t* ui) {
    if (!ui) {
        LOG_ERR("UI[NULL] passed");
        return;
    }
    delwin(ui->chat_win);
    delwin(ui->input_win);
    delwin(ui->title_win);
    endwin();
    free(ui);
}

static void win_box(WINDOW* win) {
    if (!win) return;
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    return;
}

static bool init_colors() {
    if (has_colors() == FALSE) {
        LOG_ERR("The terminal has no colors");
        return FALSE;
    }

    start_color();
    use_default_colors();

    init_pair(CP_OTHER, COLOR_GREEN, -1);
    init_pair(CP_SELF, COLOR_CYAN, -1);
    init_pair(CP_STATUS, COLOR_YELLOW, -1);
    init_pair(CP_ERR, COLOR_RED, -1);

    return TRUE;
}

static void ui_print_msg(ui_t* ui, chat_entry_t* msg) {
    if (!ui) {
        LOG_ERR("Null args.");
        return;
    }
    if (!msg) {
        LOG_ERR("Null args..");
        return;
    }
    switch (msg->type) {
        case CHAT_STATUS:
            wattron(ui->chat_win, A_BOLD | COLOR_PAIR(CP_STATUS));
            wprintw(ui->chat_win, "  :::::: *%s\n", msg->message);
            wattroff(ui->chat_win, A_BOLD | COLOR_PAIR(CP_STATUS));
            break;
        case CHAT_TEXT:
            wattron(ui->chat_win,
                    A_BOLD | COLOR_PAIR(strcmp(ui->username, msg->sender) == 0
                                            ? CP_SELF
                                            : CP_OTHER));
            wprintw(ui->chat_win, "  :::::: %32s", msg->sender);
            wattroff(ui->chat_win,
                     A_BOLD | COLOR_PAIR(strcmp(ui->username, msg->sender) == 0
                                             ? CP_SELF
                                             : CP_OTHER));

            wprintw(ui->chat_win, " %s\n", msg->message);
            break;
        case CHAT_ERR:
            wattron(ui->chat_win, A_BOLD | COLOR_PAIR(CP_ERR));
            wprintw(ui->chat_win, "  :::::: *%s\n", msg->message);
            wattroff(ui->chat_win, A_BOLD | COLOR_PAIR(CP_ERR));
            break;
    }
    return;
}

void ui_refresh(ui_t* ui, chat_msg* msgList) {
    if (!ui) {
        LOG_ERR("UI Null arg passed.");
        return;
    }
    if (!msgList) {
        LOG_ERR("Null Message Object Passed.");
        return;
    }
    werase(ui->title_win);
    wmove(ui->title_win, 1, 2);
    wprintw(ui->title_win, "Chat - %s", ui->username);

    werase(ui->chat_win);
    wmove(ui->chat_win, 1, 1);
    int y = getmaxy(ui->chat_win) - 2;
    int total_msg = chat_count(msgList);
    int start = total_msg > y ? total_msg - y : 0;

    for (int i = y - total_msg; i > 0; i--) wprintw(ui->chat_win, "\n");

    for (int i = start; i < total_msg; i++) {
        ui_print_msg(ui, get_chat(msgList, i));
    }

    werase(ui->input_win);
    wmove(ui->input_win, 1, 2);
    wprintw(ui->input_win, ">> %.*s", ui->input_len, ui->input_buf);
    wattron(ui->input_win, A_BLINK);
    wprintw(ui->input_win, "_");
    wattroff(ui->input_win, A_BLINK);

    win_box(ui->title_win);
    win_box(ui->chat_win);
    win_box(ui->input_win);

    wrefresh(ui->title_win);
    wrefresh(ui->chat_win);
    wrefresh(ui->input_win);

    return;
}

const char* ui_handle_key(ui_t* ui) {
    if (!ui) {
        LOG_ERR("ui_handle_key null args passed.");
        return NULL;
    }
    int ch;
    ch = wgetch(ui->chat_win);

    switch (ch) {
        case KEY_BACKSPACE:
        case 127:
            if (ui->input_len > 0) ui->input_buf[--ui->input_len] = '\0';
            break;
        case KEY_ENTER:
        case '\n':
            if (ui->input_len > 0) {
                ui->input_buf[ui->input_len++] = '\0';
                ui->input_len = 0;
                return ui->input_buf;
            }
            break;
        default:
            if (ch >= 32 && ch <= 126) {
                if (ui->input_len < 255) {
                    ui->input_buf[ui->input_len++] = (char)ch;
                    ui->input_buf[ui->input_len] = '\0';
                }
            }
            break;
    }
    return NULL;
}

// void ui_resize(ui_t *ui){
//
// }
