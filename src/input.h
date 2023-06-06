#pragma once
#ifndef RTDAG_INPUT_H
#define RTDAG_INPUT_H

#include "input_base.h"

#if RTDAG_INPUT_TYPE == INPUT_TYPE_YAML

#include "input_yaml.h"
using input_type = input_yaml;
#define INPUT_TYPE_NAME "yaml"
#define INPUT_TYPE_NAME_CAPS "YAML"

#elif RTDAG_INPUT_TYPE == INPUT_TYPE_HEADER

#include "input_header.h"
using input_type = input_header;
#define INPUT_TYPE_NAME "header"
#define INPUT_TYPE_NAME_CAPS "HEADER"

#endif

// This check right here tells us that the input_type that we aliased can,
// indeed, be used as we want to later:
// - it can be instanciated from a file name (const char*)
// - it derives from input_base class and can be used freely in the rest of
//   the code (using dynamic polymorphism)
static_assert(dag_input_class<input_type>);

#endif // RTDAG_INPUT_H
