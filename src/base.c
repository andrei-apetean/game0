#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void debug_log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void debug_abort(void) {
  abort();
}

