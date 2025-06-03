#include "private/system.h"
#include <dlfcn.h>

void* dl_open(const char* path) { return dlopen(path, RTLD_LAZY | RTLD_LOCAL); }

void* dl_sym(void* module, const char* symbol) { return dlsym(module, symbol); }

void dl_close(void* module) { dlclose(module); }
