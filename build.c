
// build.c - Self-contained single-file build system in C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PATH 512
#define BIN_DIR "./bin"
#define SHADERS_DIR "assets/shaders"
#define BIN_SHADERS_DIR "bin/assets/shaders"
#define VULKAN_SDK_ENV "VULKAN_SDK"

bool file_exists(const char* path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISREG(s.st_mode);
}

bool dir_exists(const char* path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

int dir_create(const char* path) {
    return mkdir(path, 0755);
}

int proc_exec(const char* args[]) {
  if (args == NULL || args[0] == NULL) {
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

// Prebuild: compile shaders and generate xdg-shell
int game_prebuild() {
    if (!dir_exists(BIN_SHADERS_DIR)) {
        dir_create("./bin/assets");
        dir_create(BIN_SHADERS_DIR);
    }

    const char* vk_sdk = getenv(VULKAN_SDK_ENV);
    if (!vk_sdk) {
        printf("[err] VULKAN_SDK not set.\n");
        return -1;
    }

    char glslc[MAX_PATH];
    snprintf(glslc, MAX_PATH, "%s/bin/glslc", vk_sdk);

    DIR* d = opendir(SHADERS_DIR);
    if (!d) {
        printf("[err] Cannot open %s\n", SHADERS_DIR);
        return -1;
    }
    struct dirent* e;
    while ((e = readdir(d))) {
        if (strstr(e->d_name, ".vert") || strstr(e->d_name, ".frag")) {
            char in[MAX_PATH], out[PATH_MAX];
            snprintf(in, MAX_PATH, "%s/%s", SHADERS_DIR, e->d_name);
            snprintf(out, MAX_PATH, "%s/%s.spv", BIN_SHADERS_DIR, e->d_name);

            printf("[inf] Compiling %s -> %s\n", in, out);
            const char* cmd[] = { glslc, in, "-o", out, NULL };
            if (proc_exec(cmd) != 0) {
                printf("[err] Failed compiling %s\n", in);
                closedir(d);
                return -1;
            }
        }
    }
    closedir(d);
    return 0;
}

// Actual build command
int game_build() {
    const char* vk_sdk = getenv(VULKAN_SDK_ENV);
    if (!vk_sdk) {
        printf("[err] VULKAN_SDK not set.\n");
        return -1;
    }

    char include[MAX_PATH], lib[MAX_PATH];
    snprintf(include, MAX_PATH, "-I%s/Include", vk_sdk);
    snprintf(lib, MAX_PATH, "-L%s/Lib", vk_sdk);

    const char* cmd[] = {
        "gcc", "-g", "-o", "./bin/game0", "./src/main.c", "./external/xdg-shell.c",
        "-D_DEBUG",
        "-I./include", "-I./src", "-I./external", include, lib,
        "-lvulkan", "-lm",
        "-lwayland-client",
         NULL
    };
    return proc_exec(cmd);
}

int build_all() {
    if (!dir_exists(BIN_DIR)) {
        printf("[inf] Creating bin dir...\n");
        dir_create(BIN_DIR);
    }
    if (game_prebuild() != 0) return -1;
    return game_build();
}

void print_usage(const char* argv0) {
    printf("Usage: %s [options]\n", argv0);
    printf("\t(default)       Build everything\n");
    printf("\t-h, --help      Show help\n");
}

int main(int argc, char** argv) {
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    return build_all();
}
