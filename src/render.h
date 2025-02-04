#pragma once

#include <stddef.h>

#include "stdbool.h"

typedef struct render_state render_state;

size_t render_get_size();
bool   render_startup(render_state* state);
bool   render_shutdown(render_state* state);

