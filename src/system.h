#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct dir_t dir_t;

typedef struct dirent_t {
    char d_name[255];
    bool is_dir;
} dirent_t;

int32_t process_exec(const char* args[]);

bool      file_exists(const char* path);
bool      dir_exists(const char* path);
bool      dir_create(const char* path);
dir_t*    dir_open(const char* path);
dirent_t* dir_read(dir_t* dir);
void      dir_close(dir_t* dir);
