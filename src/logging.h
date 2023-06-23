#ifndef RTDAG_LOGGING_H
#define RTDAG_LOGGING_H

#include <cstdio>

#ifndef LOG_LEVEL_NONE
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#endif

#ifndef RTDAG_LOG_LEVEL
#define RTDAG_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define NONE LOG_LEVEL_NONE
#define ERROR LOG_LEVEL_ERROR
#define WARNING LOG_LEVEL_WARNING
#define INFO LOG_LEVEL_INFO
#define DEBUG LOG_LEVEL_DEBUG

#define THE_LOG_LEVEL RTDAG_LOG_LEVEL

#define LOG(level, format, ...)                                                \
    do {                                                                       \
        if constexpr (level <= THE_LOG_LEVEL) {                                \
            std::printf("[%7s] %s:%d: %s(): " format, #level, __FILE__,   \
                        __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__);                    \
        }                                                                      \
    } while (0)

#define LOG_DEBUG(format, ...) LOG(DEBUG, format __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(format, ...) LOG(INFO, format __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARNING(format, ...) LOG(WARNING, format __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(format, ...) LOG(ERROR, format __VA_OPT__(,) __VA_ARGS__)

// These stay in the program regardless of DEBUG or LOG_LEVEL configurations
#define check(cond, format, ...)                                               \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::printf("ERROR: " format "\n" __VA_OPT__(,) __VA_ARGS__);                 \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

#endif // RTDAG_LOGGING_H
