#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define OS_WIN32
#define OS_PATH_SEP "\\"
#define OS_EXE_EXT ".exe"
#elif defined(__linux__)
#define OS_LINUX
#define OS_PATH_SEP "/"
#define OS_EXE_EXT ""
#else
#error "unsupported platform"
#endif

#define OS_MAX_PATH 512

int      file_exists(const char* path);
int      file_delete(const char* path);
int      file_copy(const char* from, const char* to);
int      file_rename(const char* from, const char* to);
uint64_t file_size(const char* path);
uint64_t file_modtime(const char* path);

int path_join(char* dst, size_t dst_size, const char* base, const char* comp);
int path_isdir(const char* path);
int path_isfile(const char* path);
const char* path_dirname(char* path);
const char* path_basename(const char* path);
const char* path_extension(const char* path);

int dir_exists(const char* path);
int dir_create(const char* path);
int dir_create_recurse(const char* path);
int dir_delete(const char* path);

typedef struct dir_iter dir_iter;

dir_iter*   dir_open(const char* path);
const char* dir_read(dir_iter* iter);
void        dir_close(dir_iter* iter);

uint64_t os_time_msec(void);
uint64_t os_time_usec(void);
void     os_sleep(uint64_t ms);
char*    os_getenv(const char* name);
int      os_setenv(const char* name, const char* value);
int      os_getcwd(char* buffer, size_t size);
int      os_chdir(const char* path);
int      os_pid(void);
int      os_exec(char* const* args);
