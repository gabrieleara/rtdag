#ifndef RTDAG_LOGGING_H
#define RTDAG_LOGGING_H

#include <cstdio>

enum class logging_level {
    NONE = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4,
};

// For backwards compatibility
using enum logging_level;

// Expects variable CONFIG_LOG_LEVEL to be set
#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL logging_level::DEBUG
#endif

constexpr logging_level THE_LOG_LEVEL{CONFIG_LOG_LEVEL};

#define LOG(level, format, ...)                                                \
    do {                                                                       \
        if constexpr (level <= THE_LOG_LEVEL) {                                \
            std::printf("[%7s] %s:%d: %s(): " format "\n", #level, __FILE__,   \
                        __LINE__, __func__, ##__VA_ARGS__);                    \
        }                                                                      \
    } while (0)

#define LOG_DEBUG(format, ...) LOG(DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG(INFO, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) LOG(WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG(ERROR, format, ##__VA_ARGS__)

// These stay in the program regardless of DEBUG or LOG_LEVEL configurations
#define check(cond, format, ...)                                               \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::printf("ERROR: " format "\n", ##__VA_ARGS__);                 \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

#endif // RTDAG_LOGGING_H
