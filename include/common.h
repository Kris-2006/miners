#ifndef MINERS_COMMON_H
#define MINERS_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define DEF_PORT "4545"
#define MAX_BUFF 256
#define BACKLOG 20
#define LOG(level, fmt, ...)                                                   \
  fprintf(stderr, "[%s]" fmt "\n", level, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) LOG("ERR", fmt, ##__VA_ARGS__)

#endif
