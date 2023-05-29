#ifndef  LOG_H_
#define LOG_H_

#include <stdio.h>

#ifdef __cpluplus
extern "C" {
#endif

/*
How to use:
    * set -DLOG_LEVEL=X in the Makefile to select verbosity level.

LOG_LEVEL 0: All logs are off
LOG_LEVEL 1: Error logs or higher
LOG_LEVEL 2: Warning logs or higher
LOG_LEVEL 3: Info logs or higher
LOG_LEVEL 4: Debug logs or higher

Usage Example :
    int main(int argc) {

        LOG(ERROR, "Deu Pau !!!\n");
        LOG(WARNING, "Mega bug !!!\n");
        LOG(INFO, "Annoying useless messages !!!\n");
        LOG(DEBUG, "Hello world %d\n", argc);
        LOG(DEBUG, "Hello world\n");
        return 0;
    }

Output:
    ERROR   log.c:main(68): Deu Pau !!!
    WARNING log.c:main(69): Mega bug !!!
    INFO    log.c:main(70): Annoying useless messages !!!
    DEBUG   log.c:main(71): Hello world 1
    DEBUG   log.c:main(72): Hello world
*/

#define ERROR 1
#define WARNING 2
#define INFO 3
#define DEBUG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG
#endif

#define LOG(level, fmt, ...)                                            \
  do {                                                                  \
    if ((level) <= LOG_LEVEL) {                                         \
      fprintf(stderr, "%7s %s:%s(%d): " fmt, "[" #level "]",            \
              __FILE__, __func__, __LINE__, ## __VA_ARGS__);            \
    }                                                                   \
  } while (0)

#define LOG_DEBUG(fmt, ...) LOG(DEBUG, fmt, ## __VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(INFO, fmt, ## __VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG(WARNING, fmt, ## __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(ERROR, fmt, ## __VA_ARGS__)

// These stay in the program regardless of DEBUG or LOG_LEVEL configurations
#define check(cond, fmt, ...) do {                                      \
    if (!(cond)) {                                                      \
      fprintf(stderr, "Error: " fmt "\n", ## __VA_ARGS__);              \
      exit(1);                                                          \
    }                                                                   \
  } while (0)

#ifdef __cpluplus
}
#endif

#endif // LOG_H_
