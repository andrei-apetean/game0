#pragma once

#include <stdint.h>

#include "../render_types.h"

int32_t get_render_state_size_vk();
int32_t connect_device_vk(render_state* state, render_device_params* params);
void disconnect_device_vk(render_state* state);
