#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./src/system.h"
#define PATH_MAX 255

typedef int32_t (*build_step_t)();

typedef enum project_type_t {
    project_type_exec,
    project_type_slib,
    project_type_dlib
} project_type_t;

typedef struct project_t {
    project_type_t project_type;
    build_step_t   pre_build;
    build_step_t   build;
    build_step_t   post_build;
    const char*    name;
} project_t;

void    print_usage(const char* argv_0);
int32_t build_all();
int32_t clean_all();
int32_t build_project(project_t* project);
int32_t clean_project(project_t* project);
int32_t rebuild_project(project_t* project);

int32_t game_prebuild();
int32_t game_build();

static project_t projects[] = {
    {
        .name = "game0",
        .pre_build = game_prebuild,
        .build = game_build,
    },
};

#define PROJECT_COUNT (sizeof(projects) / sizeof(projects[0]))

int main(int argc, char** argv) {
    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("Unknown option: \"%s\"\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }
    if (argc < 2) {
        return build_all();
    }
    if (!dir_exists("./bin")) {
        printf("[inf] Creating bin dir...\n");
        dir_create("./bin");
    }

    return 0;
}

int32_t build_project(project_t* project) {
    if (project->pre_build != NULL) {
        printf("[inf] Running pre build step for [%s].\n", project->name);
        if (project->pre_build() != 0) {
            printf("[err] Failed to execute pre build step for module [%s] !\n",
                   project->name);
            return -1;
        }
        printf("[inf] Pre build step successful.\n");
    }

    if (project->build == NULL) {
        printf(
            "[err] Project [%s]: A project must define at least the build "
            "step!\n",
            project->name);
        return -1;
    }
    printf("[inf] Building [%s]...\n", project->name);
    if (project->build() != 0) {
        printf("[err] Failed to build module [%s]!\n", project->name);
        return -1;
    }
    printf("[inf] Done!\n");

    if (project->post_build != NULL) {
        printf("[inf] Running post build step for [%s].\n", project->name);
        if (project->pre_build() != 0) {
            printf("[err] Failed to execute pre build step for module [%s] !\n",
                   project->name);
            return -1;
        }
    }
    printf("[inf] Done building module [%s].\n", project->name);
    return true;
}

int32_t build_all() {
    int ret = 0;
    if (!dir_exists("./bin")) {
        printf("[inf] Creating bin dir...\n");
        ret = dir_create("./bin");
    }
    if (!dir_exists("./bin")) {
        printf("[err] Creating bin dir failed! Exiting...\n");
        return ret;
    }

    for (size_t i = 0; i < PROJECT_COUNT; i++) {
        ret = build_project(&projects[i]);
        if (ret != 0) {
            return ret;
        }
    }
    return ret;
}

bool recursively_find_file(dir_t* dir, const char* target, char* buff) {
    dirent_t* e = NULL;
    size_t    len = strlen(buff);
    while ((e = dir_read(dir)) != NULL) {
        if (strcmp(".", e->d_name) == 0 || strcmp("..", e->d_name) == 0) {
            continue;
        }
        buff[len] = '/';
        strcpy(&buff[len + 1], e->d_name);

        if (e->is_dir) {
            dir_t* d = dir_open(buff);
            if (d == NULL) {
                printf("[wrn] Failed to open: %s, skipping...\n", buff);
                size_t s = strlen(buff);
                memset(&buff[len], 0, s - len);
                continue;
            }

            if (recursively_find_file(d, target, buff)) {
                free(d);
                return true;
            }

            size_t s = strlen(buff);
            memset(&buff[len], 0, s - len);
            free(d);
        } else if (strcmp(e->d_name, target) == 0 && file_exists(buff)) {
            return true;
        }
    }

    return false;
}

int32_t game_prebuild() {
    {  // shaders
        if (!dir_exists("./bin/assets/shaders")) {
           dir_create("./bin/assets");
           dir_create("./bin/assets/shaders");
      }

        const char* vulkan_sdk = getenv("VULKAN_SDK");
        if (vulkan_sdk == NULL) {
            printf(
                "[err] Failed to find $VULKAN_SDK path!\nMake sure the "
                "Vulkan "
                "SDK "
                "is installed and added to PATH.\nQuitting...\n");
            return -1;
        }
        const char glslc[PATH_MAX] = {0};

        snprintf((char*)glslc, PATH_MAX, "%s/bin/glslc", vulkan_sdk);

        const char shaders_dir[] = "assets/shaders";
        const char bin_shaders_dir[] = "bin/assets/shaders";
        const char spv_ext[] = ".spv";
        const char glsl_ext[] = ".glsl";
        size_t     shaders_dir_len = sizeof(shaders_dir);
        size_t     bin_shaders_dir_len = sizeof(bin_shaders_dir);
        size_t     spv_ext_len = sizeof(spv_ext);
        size_t     glsl_ext_len = sizeof(glsl_ext);

        dir_t* d = dir_open(shaders_dir);
        if (d == NULL) {
            printf("[err] Failed to open shaders directory. Quitting...\n");
            return -1;
        }
        dirent_t* e = NULL;
        while ((e = dir_read(d)) != NULL) {
            // TODO ensure only glsl files get compiled
            if (!e->is_dir) {
                // compose the file name
                char   spv_file[PATH_MAX] = {0};
                char   glsl_file[PATH_MAX] = {0};
                size_t glsl_name_len = strlen(e->d_name);
                size_t spv_name_len = glsl_name_len - glsl_ext_len + 1;

                snprintf(spv_file, PATH_MAX, "./%s/%s%s", bin_shaders_dir,
                         e->d_name, spv_ext);

                snprintf(glsl_file, PATH_MAX, "./%s/%s", shaders_dir,
                         e->d_name);

                printf("[inf] Compiling %s/%s -> %s ...\n", shaders_dir, e->d_name,
                       spv_file);

                const char* cmd[] = {
                    (char*)glslc, glsl_file, "-o", spv_file, NULL,
                };
                if (process_exec(cmd) != 0) {
                    printf("[err] Failed to compile %s! Quitting...\n", e->d_name);
                    return -1;
                }
            }
        }

        free(d);
    }
    {  // xdg shell
        char        xdg_shell_xml[512] = {0};
        const char* protocols_dir = "/usr/share/wayland-protocols";
        const char* xdg_shell = "xdg-shell.xml";
        strcpy(xdg_shell_xml, protocols_dir);
        if (!file_exists("./external/xdg-shell.h") ||
            !file_exists("./external/xdg-shell.c")) {
            dir_t* d = dir_open(protocols_dir);
            if (d == NULL) {
                printf("Failed to open %s. Quitting...", protocols_dir);
                return -1;
            }
            printf("[inf] Searching for %s in %s\n", xdg_shell_xml,
                   protocols_dir);
            bool xdg_found = recursively_find_file(d, xdg_shell, xdg_shell_xml);
            free(d);
            if (xdg_found == false) {
                printf("[err] Could not find %s in %s\n", xdg_shell,
                       protocols_dir);
                return -1;
            }
            printf("[inf] Found xdg_shell spec: %s\n", xdg_shell_xml);

            const char* cmd1[] = {"wayland-scanner", "--version", NULL};
            int32_t     ret = process_exec(cmd1);
            if (ret != 0) {
                printf(
                    "[err] Could not find wayland-scanner in PATH! "
                    "Quitting...\n");
                return -1;
            }

            printf("[inf] Running wayland-scanner...\n");
            const char* cmd2[] = {"wayland-scanner", "client-header",
                                  xdg_shell_xml, "./external/xdg-shell.h",
                                  NULL};
            const char* cmd3[] = {"wayland-scanner", "private-code",
                                  xdg_shell_xml, "./external/xdg-shell.c",
                                  NULL};

            ret = process_exec(cmd2);
            if (ret != 0) {
                printf(
                    "[err] Failed to execute wayland-scanner! Quitting...\n");
                return -1;
            }

            ret = process_exec(cmd3);
            if (ret != 0) {
                printf(
                    "[err] Failed to execute wayland-scanner! Quitting...\n");
                return -1;
            }
        }
        const char* vk_sdk_path = getenv("VULKAN_SDK");
        if (vk_sdk_path == NULL) {
            printf(
                "[err] Failed to find $VULKAN_SDK path!\nMake sure the Vulkan "
                "SDK "
                "is installed and added to PATH.\nQuitting...\n");
            return -1;
        }
        const char vk_sdk_include[PATH_MAX] = {0};
        const char vk_sdk_lib[PATH_MAX] = {0};
        snprintf((char*)vk_sdk_include, PATH_MAX, "-I%s/Include/", vk_sdk_path);
        snprintf((char*)vk_sdk_lib, PATH_MAX, "-L%s/Lib/", vk_sdk_path);
    }
    return 0;
}

int32_t game_build() {
    // todo make platform independent
    const char* vulkan_sdk = getenv("VULKAN_SDK");
    if (vulkan_sdk == NULL) {
        printf(
            "[err] Failed to find $VULKAN_SDK path!\nMake sure the Vulkan SDK "
            "is installed and added to PATH.\nQuitting...\n");
        return -1;
    }
    const char vk_sdk_include[PATH_MAX] = {0};
    const char vk_sdk_lib[PATH_MAX] = {0};
    snprintf((char*)vk_sdk_include, PATH_MAX, "-I%s/Include/", vulkan_sdk);
    snprintf((char*)vk_sdk_lib, PATH_MAX, "-L%s/Lib/", vulkan_sdk);

    const char* compile[] = {
        "gcc",      "./src/main.c",     "-o",           "./bin/game0",
        "-g",       "-I./external",     vk_sdk_include, vk_sdk_lib,
        "-lvulkan", "-lwayland-client", NULL,
    };

    return process_exec(compile);
}

void print_usage(const char* argv_0) {
    printf("Usage %s [args]\n", argv_0);
    printf("[Commands]\n");
    printf(
        "\t[name]         \tBuild module with [name]. If no [name] is "
        "specified it builds all modules.\n");
    printf(
        "\tclean [name],  \tClean module with [name]. If no [name] is "
        "specified it cleans all modules.\n");
    printf("\tlist           \tList all modules.\n");
    printf("\n");
    printf("[Options]\n");
    printf("\t-h, --help     \tPrint help.\n");
    printf("\n");
}

#include "./src/system.c"
