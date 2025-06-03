#pragma once

void* dl_open(const char* path);
void* dl_sym(void* module, const char* symbol);
void dl_close(void* module);
