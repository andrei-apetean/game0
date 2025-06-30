#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./src/core/os.h"

#define COMPILER "clang"
#define BIN_DIR "./bin"
#define OBJ_DIR "./bin_int"
#define ACTIVE_TARGET "program"

#define MAX_COMPILE_FLAGS 1024
#define MAX_FOLDER_DEPTH 256

static char g_vulkan_include[OS_MAX_PATH];
static char g_vulkan_link[OS_MAX_PATH];

typedef struct {
    char      path[OS_MAX_PATH];
    dir_iter* iter;
} dir_info;

typedef enum {
    LIB_VULKAN = 0,
    LIB_MATH,
    LIB_WAYLAND,
    LIBS_COUNT,
} l_libs;

typedef enum {
    LINK_VULKAN = 0,
    LINKS_COUNT,
} l_links;

typedef enum {
    INCLUDE_VULKAN = 0,
    INCLUDE_PROGRAM,
    INCLUDE_EXTERNAL,
    INCLUDES_COUNT,
} i_includes;

typedef enum {
    DEFINE_DEBUG = 0,
    DEFINES_COUNT,
} d_defines;

typedef enum {
    FLAG_02 = 0,
    FLAG_GSYMBOLS,
    FLAGS_COUNT,
} f_flags;

typedef enum {
    BUILD_MODE_RELEASE = 0,
    BUILD_MODE_DEBUG,
} build_mode;

typedef struct {
    char* name;
    char* src_dir;
    int   flags[FLAGS_COUNT];
    int   libs[LIBS_COUNT];
    int   includes[INCLUDES_COUNT];
    int   defines[DEFINES_COUNT];
    int   links[LINKS_COUNT];
} target;

static char* g_libs[LIBS_COUNT] = {
    "-lvulkan",  
    "-lm",
    "-lwayland-client",
};

static char* g_links[LINKS_COUNT] = {
  "",  // vulkan, reserved
 };

static char* g_includes[INCLUDES_COUNT] = {
    "",  // vulkan, reserved
    "-I./src",
    "-I./external",
};

static char* g_defines[DEFINES_COUNT] = {
    "-D_DEBUG",
};

static char* g_flags[FLAGS_COUNT] = {
    "-02",
    "-g",
};

static target g_targets[] = {
    {.name = "program",
     .src_dir = "./src",

     .includes[INCLUDE_VULKAN] = 1,
     .includes[INCLUDE_PROGRAM] = 1,
     .includes[INCLUDE_EXTERNAL] = 1,

     .links[LINK_VULKAN] = 1,

     .libs[LIB_VULKAN] = 1,
     .libs[LIB_MATH] = 1,
#ifdef OS_LINUX
     .libs[LIB_WAYLAND] = 1
#endif  // OS_LINUX
        // flags and defines will be assigned based on user input
    },
};

static int g_projects_count = sizeof(g_targets) / sizeof(g_targets[0]);

int needs_recompile(const char* src_file, const char* obj_file) {
    if (!file_exists(obj_file)) return 1;
    return file_modtime(src_file) > file_modtime(obj_file);
}

void print_usage(const char* prog);

int bootstrap(char** argv);

target* find_target(const char* name) { return g_targets; }

void build_target(target* project, build_mode mode);

void build_all(void) {}

int g_release_mode = 0;

int main(int argc, char** argv) {
    {
        char* vulkan_sdk = os_getenv("VULKAN_SDK");
        if (!vulkan_sdk) {
            printf("Cannot build project. $VULKAN_SDK is not set on the system\n");
            return -1;
        }
        snprintf(g_vulkan_include, OS_MAX_PATH, "-I%s" OS_PATH_SEP "%s", vulkan_sdk,
                 "include");
        snprintf(g_vulkan_link, OS_MAX_PATH, "-L%s" OS_PATH_SEP "%s", vulkan_sdk,
                 "lib");
        g_includes[INCLUDE_VULKAN] = g_vulkan_include;
        g_links[LINK_VULKAN] = g_vulkan_link;
    }

    // if (bootstrap(argv)) {
    //     return 0;
    // }
    if (argc < 2) {
        target* active = find_target(ACTIVE_TARGET);
        build_target(active, BUILD_MODE_DEBUG);
        return 0;
    }
    const char* command = argv[1];
    if (strcmp(command, "release") == 0) {
        if (argc > 2) {
            target* active = find_target(argv[2]);
            build_target(active, BUILD_MODE_RELEASE);
        } else {
            build_all();
        }
        return 0;
        // } else if (strcmp(command, "release") == 0) {
        //     const char* target = argc > 2 ? argv[2] : "all";
        //     return build_target(target, "release");
        // } else if (strcmp(command, "clean") == 0) {
        //     const char* target = argc > 2 ? argv[2] : "all";
        //     return clean_target(target);
    }

    if (!dir_exists(BIN_DIR)) {
        dir_create(BIN_DIR);
    }
    if (!dir_exists(OBJ_DIR)) {
        dir_create(OBJ_DIR);
    }

    return 0;
}

target* find_project(const char* name) {
    for (int i = 0; i < g_projects_count; i++) {
        if (strcmp(name, g_targets[i].name) == 0) {
            return &g_targets[i];
        }
    }
    return NULL;
}

void print_usage(const char* prog) {
    printf("Usage: %s [command] [options] [target]\n\n", prog);

    printf("Commands:\n");
    printf("  build [target]     Build project(s) in debug mode (default)\n");
    printf("  release [target]   Build project(s) in release mode\n");
    printf("  clean [target]     Clean build artifacts\n");
    printf("  rebuild [target]   Clean and build project(s)\n");
    printf("  run [target]       Build and run the specified project\n");
    printf("  test [target]      Build and run tests\n");
    printf("  list               List all available projects\n");
    printf("  help               Show this help message\n\n");

    printf("Options:\n");
    printf("  -v, --verbose      Enable verbose output\n");
    printf("  -j, --jobs N       Use N parallel jobs\n");
    printf("  -f, --force        Force rebuild even if up-to-date\n");
    printf("  -q, --quiet        Suppress non-error output\n");
    printf("  --dry-run          Show what would be built without building\n\n");

    printf("Targets:\n");
    printf("  all                Build all projects (default)\n");
    printf("  <project-name>     Build specific project\n");
    printf("  <path/to/project>  Build project at specific path\n\n");

    printf("Examples:\n");
    printf("  %s                 Build all projects in debug mode\n", prog);
    printf("  %s build           Same as above\n", prog);
    printf("  %s release         Build all projects in release mode\n", prog);
    printf("  %s build engine    Build only the 'engine' project\n", prog);
    printf("  %s clean           Clean all build artifacts\n", prog);
    printf("  %s rebuild game    Clean and rebuild the 'game' project\n", prog);
    printf("  %s run game -v     Build and run 'game' with verbose output\n", prog);
    printf("  %s list            Show all available projects\n", prog);
}

int bootstrap(char** argv) {
    const char* build_exe = argv[0];
    const char* build_src = __FILE__;

    uint64_t exe_mod = file_modtime(build_exe);
    uint64_t src_mod = file_modtime(build_src);

    printf("Generating build config...\n");
    printf("Done\n");
    if (src_mod > exe_mod) {
        printf("\n=== Bootstrap Check ===\n");
        printf("Executable: %s\n", build_exe);
        printf("Source:     %s\n", build_src);
        printf("Rebuilding...\n");
        char tmp_exe[OS_MAX_PATH];
        snprintf(tmp_exe, sizeof(tmp_exe), "%s.tmp.%d", build_exe, os_pid());
        const char* rebuild_cmd[] = {
            "clang", "-o", tmp_exe, build_src, "-D_CONFIG", NULL,
        };

        printf("Running: ");
        for (int i = 0; rebuild_cmd[i]; i++) {
            printf("%s ", rebuild_cmd[i]);
        }
        printf("\n\n");
        int status = os_exec((char* const*)rebuild_cmd);
        if (status == 0) {
            if (file_rename(tmp_exe, build_exe)) {
                file_delete(tmp_exe);
                printf("Rebuild succeeded\n\n");
                os_exec(argv);

            } else {
                printf("\n[ err ] Rename failed!\n\n");
            }
        } else {
            printf("\n[ err ] Rebuild failed: %d!\n\n", status);
            file_delete(tmp_exe);
        }

        return 1;
    }

    printf("%s is up to date\n", build_exe);
    return 0;
}

int compile_target(target* target, int* obj_count) {
    const char* cc[MAX_COMPILE_FLAGS] = {0};

    int idx = 0;
    int input_idx = 2;
    int output_idx = 4;

    cc[idx++] = COMPILER;
    cc[idx++] = "-c";
    idx++;
    cc[idx++] = "-o";
    idx++;
    for (int i = 0; i < INCLUDES_COUNT; i++) {
        if (target->includes[i]) {
            cc[idx++] = g_includes[i];
        }
    }
    for (int i = 0; i < FLAGS_COUNT; i++) {
        if (target->flags[i]) {
            cc[idx++] = g_flags[i];
        }
    }
    for (int i = 0; i < DEFINES_COUNT; i++) {
        if (target->defines[i]) {
            cc[idx++] = g_defines[i];
        }
    }
    dir_info stack[MAX_FOLDER_DEPTH] = {0};
    int      top = 1;
    sprintf(stack[top].path, "%s", target->src_dir);
    stack[top].iter = dir_open(target->src_dir);
    char path[OS_MAX_PATH] = {0};

    while (top > 0) {
        const char* entry = dir_read(stack[top].iter);
        if (entry == NULL) {
            dir_close(stack[top--].iter);
            continue;
        }
        snprintf(path, OS_MAX_PATH, "%s" OS_PATH_SEP "%s", stack[top].path, entry);
        if (path_isdir(path)) {
            top++;
            stack[top].iter = dir_open(path);
            strcpy(stack[top].path, path);
            continue;
        }
        const char* ext = path_extension(path);
        if (strcmp(ext, ".c") != 0) {
            continue;
        }
        const char* dot = strrchr(entry, '.');
        if (!dot) continue;

        size_t len = dot - entry;
        char   file[128] = {0};
        strcpy(file, entry);
        file[len] = 0;

        char out[OS_MAX_PATH] = {0};

        snprintf(out, OS_MAX_PATH, "%s" OS_PATH_SEP "%s" OS_PATH_SEP "%s.o",
                 OBJ_DIR, target->name, file);
        cc[input_idx] = path;
        cc[output_idx] = out;

        printf("%s\n", out);
        // for (int i = 0; i < idx; i++) {
        //     printf("%s ", cc[i]);
        // }
        (*obj_count)++;
        int status = os_exec((char* const*)cc);
        if (status) {
            printf("compilation failed: %d\n", status);
            return 1;
        }
    }

    return 0;
}

int link_target(target* target, int obj_count) {
    char*  cc[MAX_COMPILE_FLAGS] = {0};
    char*  obj_buff = malloc(OS_MAX_PATH * obj_count);
    char** obj_paths = malloc(sizeof(char*) * obj_count);

    for (int i = 0; i < obj_count; i++) {
        obj_paths[i] = &obj_buff[i * OS_MAX_PATH];
    }

    char out_bin[OS_MAX_PATH] = {0};
    char obj_dir[OS_MAX_PATH] = {0};
    snprintf(out_bin, OS_MAX_PATH, "%s" OS_PATH_SEP "%s", BIN_DIR, target->name);
    snprintf(obj_dir, OS_MAX_PATH, "%s" OS_PATH_SEP "%s", OBJ_DIR, target->name);

    printf("bin_dir %s\n", out_bin);
    printf("obj_dir %s\n", obj_dir);
    int idx = 0;

    cc[idx++] = COMPILER;
    cc[idx++] = "-o";
    cc[idx++] = target->name;
    for (int i = 0; i < LIBS_COUNT; i++) {
        if (target->libs[i]) {
            cc[idx++] = g_libs[i];
        }
    }
    for (int i = 0; i < LINKS_COUNT; i++) {
        if (target->links[i]) {
            cc[idx++] = g_links[i];
        }
    }

    dir_info stack[MAX_FOLDER_DEPTH] = {0};
    int      top = 1;
    sprintf(stack[top].path, "%s", obj_dir);
    stack[top].iter = dir_open(obj_dir);
    char path[OS_MAX_PATH] = {0};
    int  obj_idx = 0;
    while (top > 0) {
        const char* entry = dir_read(stack[top].iter);
        if (entry == NULL) {
            dir_close(stack[top--].iter);
            continue;
        }
        snprintf(path, OS_MAX_PATH, "%s" OS_PATH_SEP "%s", stack[top].path, entry);
        if (path_isdir(path)) {
            top++;
            stack[top].iter = dir_open(path);
            strcpy(stack[top].path, path);
            continue;
        }
        strcpy(obj_paths[obj_idx], path);
        printf("path %s:\n", obj_paths[obj_idx]);
        cc[idx++] = obj_paths[obj_idx];
        obj_idx++;
    }

    for (int i = 0; i < idx; i++) {
        printf("%s ", cc[i]);
    }
    printf("\n");
    int status = os_exec((char* const*)cc);
    if (status) {
        printf("compilation failed: %d\n", status);
        free(obj_paths);
        free(obj_buff);
        return 1;
    }
    free(obj_paths);
    free(obj_buff);
    return 0;
}

void build_target(target* target, build_mode mode) {
    {
        char obj_dir[OS_MAX_PATH];
        snprintf(obj_dir, OS_MAX_PATH, "./%s" OS_PATH_SEP "%s", OBJ_DIR,
                 target->name);
        if (!dir_exists(obj_dir)) {
            dir_create_recurse(obj_dir);
        }
    }
    target->flags[FLAG_02] = mode == BUILD_MODE_RELEASE;
    target->flags[FLAG_GSYMBOLS] = mode == BUILD_MODE_DEBUG;
    target->defines[DEFINE_DEBUG] = mode == BUILD_MODE_DEBUG;

    printf("compiling objects:\n");
    int obj_count = 0;
    if (compile_target(target, &obj_count) != 0) {
        return;
    }
    printf("linking objects:\n");
    if (link_target(target, obj_count) != 0) {
        return;
    }
    printf("compilation finished\n");
}

#include "./src/core/os_linux.c"
