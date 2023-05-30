#ifndef RTDAG_CONFIG_H
#define RTDAG_CONFIG_H

#define OFF 0
#define ON 1

// Boolean Options (0 means no, 1 means yes)
#define CONFIG_COMPILER_BARRIER ON
#define CONFIG_MEM_ACCESS OFF
#define CONFIG_COUNT_TICK ON
#define CONFIG_OPENCL_USE OFF
#define CONFIG_FRED_USE OFF

// Integer options
#define CONFIG_LOG_LEVEL 0
#define CONFIG_TASK_IMPL 0
#define CONFIG_INPUT_TYPE 0

// Reference values for the integer options
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#define TASK_IMPL_THREAD 0
#define TASK_IMPL_PROCESS 1

#define INPUT_TYPE_YAML 0
#define INPUT_TYPE_HEADER 1

// For backwards compatibility
#define LOG_LEVEL CONFIG_LOG_LEVEL
#define TASK_IMPL CONFIG_TASK_IMPL
#define INPUT_TYPE CONFIG_INPUT_TYPE

#endif // RTDAG_CONFIG_H
