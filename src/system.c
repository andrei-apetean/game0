#include "system.h"

#include <dirent.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct dir_t {
    DIR*     handle;
    dirent_t entry;
} dir_t;

int32_t process_exec(const char* args[]) {
    if (args == NULL) {
        fprintf(stderr, "Error: Arguments are NULL.\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        execvp(args[0], (char* const*)args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            fprintf(stderr, "Process did not exit normally.\n");
            return -1;
        }
    }
}

bool file_exists(const char* path) {
    int result = access(path, F_OK);
    return (result == 0);
}

bool dir_exists(const char* path) {
    int result = access(path, F_OK);
    return (result == 0);
}

bool dir_create(const char* path) {
    int result = mkdir(path, S_IRWXU | S_IRWXG);
    return (result == 0);
}

dir_t* dir_open(const char* path) {
    dir_t* dir = malloc(sizeof(dir_t));
    if (dir == NULL) {
        printf("Failed to open %s!\n", path);
        return NULL;
    }
    dir->handle = opendir(path);
    if (dir->handle == NULL) {
        printf("Failed to open %s!\n", path);
        free(dir);
        return NULL;
    }
    return dir;
}

dirent_t* dir_read(dir_t* dir) {
    struct dirent* ent = readdir(dir->handle);
    if (!ent) {
        return NULL;  // No more files
    }

    snprintf(dir->entry.d_name, sizeof(dir->entry.d_name), "%s", ent->d_name);

    // todo - use stat or lstat for checking entry type
    dir->entry.is_dir = ent->d_type == DT_DIR;
    return &dir->entry;
}

void dir_close(dir_t* dir) {
    closedir(dir->handle);
    free(dir);
}
