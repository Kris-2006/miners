#include "include/ui.h"
#include "include/common.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

struct ui_ctx {
  WINDOW *chat_win;
  WINDOW *input_win;
  WINDOW *title_win;

  int rows, cols;
  char input_buf[256];
  int input_len;

  char username[32];
};

static bool init_colors();

ui_t *ui_init(const char *username) {

  /* Initialises the ncurses win with predetermined rows x cols
   * doesn't initialise any other method on each scr.
   * uses following border ("-","-","|","|","+","+","+","+")
   * but in method ui_refresh()
   */

  if (!username) {
    LOG_ERR("Invalid Username passed");
    return NULL;
  }

  ui_t *ui;
  ui = (ui_t *)malloc(sizeof(ui_t));
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
  getmaxyx(stdscr, cols, rows);
  ui->rows = rows;
  ui->cols = cols;

  if (!init_colors()) {
    endwin();
    return NULL;
  }

  ui->chat_win = newwin(cols - 10, rows, 5, 0);
  if (!ui->chat_win) {
    free(ui);
    LOG_ERR("CHAT-WINDOW failed");
    return NULL;
  }
  ui->title_win = newwin(5, rows, 0, 0);
  if (!ui->title_win) {
    delwin(ui->chat_win);
    free(ui);
    LOG_ERR("TITLE-WINDOW failed");
    return NULL;
  }
  ui->input_win = newwin(5, rows, cols - 5, 0);
  if (!ui->input_win) {
    delwin(ui->chat_win);
    delwin(ui->title_win);
    free(ui);
    LOG_ERR("INPUT_WIN failed");
    return NULL;
  }
  move(cols - 3, 2);
  return ui;
}

void ui_destroy(ui_t *ui) {
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

static bool init_colors() {

  if (has_colors() == FALSE) {
    LOG_ERR("The terminal has no colors");
    return FALSE;
  }

  start_color();
  // init_pair(0, 0, 0);
  // similar to these here implemented;

  return TRUE;
}
