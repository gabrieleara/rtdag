#include "input_wrapper.h"

#define INPUT_TYPE_YAML 0
#define INPUT_TYPE_HEADER 1

#if CONFIG_INPUT_TYPE == INPUT_TYPE_YAML

#include "input_yaml.h"
using input_type = input_yaml;
#define INPUT_TYPE_NAME "yaml"
#define INPUT_TYPE_NAME_CAPS "YAML"

#elif CONFIG_INPUT_TYPE == INPUT_TYPE_HEADER

#include "input_header.h"
using input_type = input_header;
#define INPUT_TYPE_NAME "header"
#define INPUT_TYPE_NAME_CAPS "HEADER"

#else
#error "Invalid input type" CONFIG_INPUT_TYPE
#endif
