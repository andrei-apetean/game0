
#include <stdarg.h>
#include <sys/wait.h>

#include "os.h"

#ifdef OS_LINUX

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define ONE_THOUSAND 1000
#define ONE_OVER_ONE_THOUSAND 0.001f;
#define ONE_MILLION 1000000
#define CMD_NOT_FOUND 127
#define CMD_NOT_EXECUTABLE 126

int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int file_delete(const char* path) { return unlink(path) == 0; }

int file_copy(const char* from, const char* to) {
    FILE* src = fopen(from, "rb");
    if (!src) return 0;
    FILE* dst = fopen(to, "wb");
    if (!dst) {
        fclose(src);
        return 0;
    }
    char   buffer[4096];
    size_t bytes;
    int    success = 1;

    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            success = 0;
            break;
        }
    }
    fclose(src);
    fclose(dst);
    return success;
}

int file_rename(const char* from, const char* to) {
    return rename(from, to) == 0;
}

uint64_t file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return (uint64_t)st.st_size;
}

uint64_t file_modtime(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }

    uint64_t seconds = (uint64_t)st.st_mtime;
    uint64_t nanoseconds = (uint64_t)st.st_mtim.tv_nsec;

    return seconds * 1000000000ULL + nanoseconds;
}

int path_join(char* dst, size_t dst_size, const char* base, const char* comp) {
    size_t base_len = strlen(base);
    size_t comp_len = strlen(comp);

    int    needs_sep = base_len > 0 && base[base_len - 1] != '/';
    size_t total_len = base_len + comp_len + (needs_sep ? 1 : 0);

    if (total_len >= dst_size) {
        return -1;
    }

    strcpy(dst, base);
    if (needs_sep) {
        strcat(dst, "/");
    }
    strcat(dst, comp);

    return total_len;
}

int path_isdir(const char* path) {
    if (!path) return 0;

    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    int is_dir = S_ISDIR(st.st_mode);
    return is_dir;
}

int path_isfile(const char* path) {
    if (!path) return 0;

    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

void path_fname(char* buffer, const char* path) {
    const char* filename = path;
    const char* p = path;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            filename = p + 1;
        }
        p++;
    }
    
    const char* dot = NULL;
    p = filename;
    while (*p) {
        if (*p == '.') {
            dot = p;
        }
        p++;
    }
    
    size_t len;
    if (dot && dot > filename) {
        len = dot - filename;
    } else {
        len = strlen(filename);
    }
    
    if (len >= OS_MAX_PATH) {
        len = OS_MAX_PATH - 1;
    }
    
    // Copy and null terminate
    memcpy((void*)buffer, filename, len);
    buffer[len] = '\0';
}
const char* path_dirname(char* path) { return dirname(path); }

const char* path_basename(const char* path) {
    const char* base = strrchr(path, '/');
    return base ? base + 1 : path;
}

const char* path_extension(const char* path) {
     const char* base = path_basename(path);
    const char* ext = strrchr(base, '.');
    return (ext && ext != base) ? ext : NULL;
}

int dir_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int dir_create(const char* path) {
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

int dir_create_recurse(const char* path) {
    char   tmp[OS_MAX_PATH];
    char*  p = NULL;
    size_t len;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (!dir_exists(tmp) && !dir_create(tmp)) {
                return 0;
            }
            *p = '/';
        }
    }
    return dir_create(tmp);
}

int dir_delete(const char* path) { return rmdir(path); }

struct dir_iter {
    DIR*           dir;
    struct dirent* entry;
};

dir_iter* dir_open(const char* path) {
    dir_iter* iter = malloc(sizeof(*iter));
    if (!iter) return NULL;
    iter->dir = opendir(path);
    if (!iter->dir) {
        free(iter);
        return NULL;
    }
    iter->entry = NULL;
    return iter;
}

const char* dir_read(dir_iter* iter) {
    if (!iter || !iter->dir) return NULL;

    do {
        iter->entry = readdir(iter->dir);
        if (!iter->entry) return NULL;
    } while (strcmp(iter->entry->d_name, ".") == 0 ||
             strcmp(iter->entry->d_name, "..") == 0);
    return iter->entry->d_name;
}

void dir_close(dir_iter* iter) {
    if (!iter) return;
    if (iter->dir) closedir(iter->dir);
    free(iter);
}

char* os_getenv(const char* name) { return getenv(name); };

int env_set(const char* name, const char* value) {
    return setenv(name, value, 1) == 0;
}

uint64_t os_time_msec(void) { return os_time_usec() / ONE_THOUSAND; }

uint64_t os_time_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * ONE_MILLION + (uint64_t)tv.tv_usec;
}

void os_sleep(uint64_t ms) { usleep(ms * 1000); }

int os_getcwd(char* buffer, size_t size) { return getcwd(buffer, size) != NULL; }

int os_chdir(const char* path) { return chdir(path) != 0; }

int os_pid(void) {
 return getpid();    
}

int os_exec(const char** args) {
    if (args == NULL || args[0] == NULL) {
        return 1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    } else if (pid == 0) {
        const char* arg0 = args[0];
        execvp(arg0, (char* const*)args);

        if (errno == ENOENT) {
            exit(CMD_NOT_FOUND);  // Command not found
        } else if (errno == EACCES) {
            exit(CMD_NOT_EXECUTABLE);  // Permission denied
        } else {
            exit(125);  // Other exec error
        }
    }
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}
#endif  // OS_LINUX
