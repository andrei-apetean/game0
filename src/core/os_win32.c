
#include "os_defines.h"

#if defined(OS_WIN32)
#include "os.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>


#include <direct.h>
#include <fileapi.h>
#include <process.h>
#include <shlwapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#pragma comment(lib, "shlwapi.lib")

int file_exists(const char* path) {
    DWORD attrs = GetFileAttributes(path);
    return (attrs != INVALID_FILE_ATTRIBUTES &&
            !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

int file_delete(const char* path) { return DeleteFileA(path); }

int file_copy(const char* from, const char* to) {
    return CopyFileA(from, to, FALSE) ? 1 : 0;
}

int file_rename(const char* from, const char* to) {
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) ? 1 : 0;
}

uint64_t file_size(const char* path) {
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) {
        CloseHandle(file);
        return 0;
    }

    CloseHandle(file);
    return (uint64_t)size.QuadPart;
}

uint64_t file_modtime(const char* path) {
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    FILETIME ft;
    if (!GetFileTime(file, NULL, NULL, &ft)) {
        CloseHandle(file);
        return 0;
    }

    CloseHandle(file);

    // Convert FILETIME to Unix timestamp (microseconds since epoch)
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // Windows FILETIME is in 100ns intervals since Jan 1, 1601
    // Unix timestamp is seconds since Jan 1, 1970
    // Difference is 11644473600 seconds
    return (ull.QuadPart / 10000000ULL) - 11644473600ULL;
}

int path_join(char* dst, size_t dst_size, const char* base, const char* comp) {
    if (!dst || !base || !comp || dst_size == 0) {
        return 0;
    }

    size_t base_len = strlen(base);
    size_t comp_len = strlen(comp);
    size_t needed = base_len + 1 + comp_len + 1;  // +1 for separator, +1 for null

    if (needed > dst_size) {
        return 0;
    }

    strcpy_s(dst, dst_size, base);

    // Add separator if base doesn't end with one
    if (base_len > 0 && base[base_len - 1] != '\\' && base[base_len - 1] != '/') {
        strcat_s(dst, dst_size, OS_PATH_SEP);
    }

    strcat_s(dst, dst_size, comp);
    return 1;
}

int path_isdir(const char* path) {
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

int path_isfile(const char* path) { return file_exists(path); }

void path_fname(char* buffer, const char* path) {
    const char* fname = PathFindFileNameA(path);
    strcpy_s(buffer, OS_MAX_PATH, fname);
    char* dot = strrchr(buffer, '.');
    if (dot) {
        *dot = '\0';
    }
}

const char* path_dirname(char* path) {
    PathRemoveFileSpecA(path);
    return path;
}

const char* path_basename(const char* path) { return PathFindFileNameA(path); }

const char* path_extension(const char* path) {
    const char* ext = PathFindExtensionA(path);
    return ext ? ext : "";
}

int dir_exists(const char* path) { return path_isdir(path); }

int dir_create(const char* path) { return CreateDirectoryA(path, NULL) ? 1 : 0; }

int dir_create_recurse(const char* path) {
    char temp_path[MAX_PATH];
    strcpy_s(temp_path, sizeof(temp_path), path);

    // Remove trailing slashes
    size_t len = strlen(temp_path);
    while (len > 0 && (temp_path[len - 1] == '\\' || temp_path[len - 1] == '/')) {
        temp_path[--len] = '\0';
    }

    // Check if directory already exists
    if (dir_exists(temp_path)) {
        return 1;
    }

    // Find parent directory
    char* last_sep = strrchr(temp_path, '\\');
    if (!last_sep) {
        last_sep = strrchr(temp_path, '/');
    }

    if (last_sep) {
        *last_sep = '\0';
        // Recursively create parent
        if (!dir_create_recurse(temp_path)) {
            return 0;
        }
        *last_sep = '\\';
    }

    return dir_create(temp_path);
}

int dir_delete(const char* path) { return RemoveDirectoryA(path) ? 1 : 0; }

struct dir_iter {
    HANDLE          handle;
    WIN32_FIND_DATA find_data;
    int             first_call;
    char            current_name[MAX_PATH];
};

dir_iter* dir_open(const char* path) {
    dir_iter* iter = (dir_iter*)malloc(sizeof(dir_iter));
    if (!iter) {
        return NULL;
    }

    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    iter->handle = FindFirstFileA(search_path, &iter->find_data);
    if (iter->handle == INVALID_HANDLE_VALUE) {
        free(iter);
        return NULL;
    }

    iter->first_call = 1;
    return iter;
}

const char* dir_read(dir_iter* iter) {
    if (!iter || iter->handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    do {
        if (iter->first_call) {
            iter->first_call = 0;
        } else {
            if (!FindNextFileA(iter->handle, &iter->find_data)) {
                return NULL;
            }
        }

        // Skip "." and ".." entries
        if (strcmp(iter->find_data.cFileName, ".") == 0 ||
            strcmp(iter->find_data.cFileName, "..") == 0) {
            continue;
        }

        strcpy_s(iter->current_name, sizeof(iter->current_name),
                 iter->find_data.cFileName);
        return iter->current_name;

    } while (1);
}

void dir_close(dir_iter* iter) {
    if (iter) {
        if (iter->handle != INVALID_HANDLE_VALUE) {
            FindClose(iter->handle);
        }
        free(iter);
    }
}

uint64_t os_time_msec(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // Convert to milliseconds since Unix epoch
    return (ull.QuadPart / 10000ULL) - 11644473600000ULL;
}

uint64_t os_time_usec(void) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);

    // Convert to microseconds since some reference point
    return (counter.QuadPart * 1000000ULL) / freq.QuadPart;
}

void os_sleep(uint64_t ms) { Sleep((DWORD)ms); }

char* os_getenv(const char* name) {
    static char buffer[32767];  // Max environment variable size on Windows
    DWORD       result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    return (result > 0) ? buffer : NULL;
}

int os_setenv(const char* name, const char* value) {
    return SetEnvironmentVariableA(name, value) ? 1 : 0;
}

int os_getcwd(char* buffer, size_t size) {
    return GetCurrentDirectoryA((DWORD)size, buffer) ? 1 : 0;
}

int os_chdir(const char* path) { return SetCurrentDirectoryA(path) ? 1 : 0; }

int os_pid() { return (int)GetCurrentProcessId(); }

int os_exec(const char** args) {
    if (!args || !args[0]) {
        return -1;
    }

    // Build command line from args array
    const int cmd_max = 32767;
    char      cmdline[32767] = {0};
    size_t    pos = 0;

    for (int i = 0; args[i] && pos < sizeof(cmdline) - 1; i++) {
        if (i > 0) {
            cmdline[pos++] = ' ';
        }

        // Quote argument if it contains spaces
        int has_space = strchr(args[i], ' ') != NULL;
        if (has_space && pos < sizeof(cmdline) - 1) {
            cmdline[pos++] = '"';
        }

        size_t arg_len = strlen(args[i]);
        if (pos + arg_len < sizeof(cmdline)) {
            strcpy_s(cmdline + pos, cmd_max - pos, args[i]);
            pos += arg_len;
        }

        if (has_space && pos < sizeof(cmdline) - 1) {
            cmdline[pos++] = '"';
        }
    }

    STARTUPINFOA        si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(NULL,     // Application name
                                  cmdline,  // Command line
                                  NULL,     // Process security attributes
                                  NULL,     // Thread security attributes
                                  FALSE,    // Inherit handles
                                  0,        // Creation flags
                                  NULL,     // Environment
                                  NULL,     // Current directory
                                  &si,      // Startup info
                                  &pi       // Process info
    );

    if (!success) {
        return -1;
    }

    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
}

#endif  // OS_WIN32
