#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./src/core/os.h"

#define COMPILER "clang"
#define BIN_DIR "./bin"
#define BIN_ASSETS "./bin/assets"
#define BIN_SHADERS "./bin/assets/shaders"
#define SHADERS_DIR "./assets/shaders"
#define OBJ_DIR "./bin_int"
#define ACTIVE_TARGET "program"

#define MAX_COMPILE_FLAGS 1024
#define MAX_FOLDER_DEPTH 256

static char g_vulkan_include[OS_MAX_PATH];
static char g_vulkan_link[OS_MAX_PATH];
static char g_vulkan_glslc[OS_MAX_PATH];

typedef int (*build_hook)();

typedef struct {
    dir_iter* iter;
    char      path[OS_MAX_PATH];
} dir_info;

typedef enum {
    LIB_ID_VULKAN = 0,
    LIB_ID_MATH,

#ifdef OS_LINUX
    LIB_ID_WAYLAND,
#endif  // OS_LINUX

    LIB_ID_COUNT,
} lib_id;

typedef enum {
    LINK_ID_VULKAN = 0,
    LINK_ID_COUNT,
} link_id;

typedef enum {
    INCLUDE_ID_VULKAN = 0,
    INCLUDE_ID_PROGRAM,
    INCLUDE_ID_EXTERNAL,
    INCLUDE_ID_COUNT,
} include_id;

typedef enum {
    DEFINE_ID_DEBUG = 0,
    DEFINE_ID_COUNT,
} define_id;

typedef enum {
    FLAG_ID_O2 = 0,
    FLAG_ID_GSYMBOLS,
    FLAG_ID_COUNT,
} flag_id;

typedef enum {
    MODULE_ID_PROGRAM = 0,
#ifdef OS_LINUX
    MODULE_ID_XDG_SHELL,
#endif  // OS_LINUX
    MODULE_ID_COUNT
} module_id;

typedef enum {
    BUILD_MODE_RELEASE = 0,
    BUILD_MODE_DEBUG,
} build_mode;

typedef struct {
    char*      name;
    char*      src_dir;
    int        flags[FLAG_ID_COUNT];
    int        libs[LIB_ID_COUNT];
    int        includes[INCLUDE_ID_COUNT];
    int        defines[DEFINE_ID_COUNT];
    int        links[LINK_ID_COUNT];
    build_hook pre_build;
    build_hook post_build;
} module_info;

typedef enum {
    ARTIFACT_ID_PROGRAM,  // only have the game for now
    ARTIFACT_ID_COUNT,
} artifact_id;

typedef struct {
    char*     name;
    int       module_count;
    module_id modules[MODULE_ID_COUNT];
} artifact_info;

static char* g_libs[LIB_ID_COUNT] = {
    "-lvulkan",
    "-lm",
    "-lwayland-client",
};

static char* g_links[LINK_ID_COUNT] = {
    NULL,  // vulkan, reserved - will populate at runtime
};

static char* g_includes[INCLUDE_ID_COUNT] = {
    NULL,  // vulkan, reserved - will populate at runtime
    "-I./src",
    "-I./external",
};

static char* g_defines[DEFINE_ID_COUNT] = {
    "-D_DEBUG",
};

static char* g_flags[FLAG_ID_COUNT] = {
    "-o2",
    "-g",
};

static int xdg_shell_prebuild();
static int program_prebuild();

static module_info g_module_infos[MODULE_ID_COUNT] = {
    {
        .src_dir = "./src",
        .name = "program",
        .includes[INCLUDE_ID_VULKAN] = 1,
        .includes[INCLUDE_ID_PROGRAM] = 1,
        .includes[INCLUDE_ID_EXTERNAL] = 1,

        .links[LINK_ID_VULKAN] = 1,

        .libs[LIB_ID_VULKAN] = 1,
        .libs[LIB_ID_MATH] = 1,
#ifdef OS_LINUX
        .libs[LIB_ID_WAYLAND] = 1,
#endif  // OS_LINUX
        // flags and defines will be assigned based on user input
        .pre_build = program_prebuild,
    },
#ifdef OS_LINUX
    {
        .name = "xdg-shell",
        .src_dir = "./external/wayland",
        .pre_build = xdg_shell_prebuild,
    },
#endif
};

static artifact_info g_artifacts[ARTIFACT_ID_COUNT] = {
    {
        .name = "program",
        .modules =
            {
#ifdef OS_LINUX
                MODULE_ID_XDG_SHELL,
#endif
                MODULE_ID_PROGRAM,
            },
#ifdef OS_LINUX
        .module_count = 2,
#else
        .module_count = 1,
#endif
    },
};
static int64_t g_build_start_time = 0;
static int64_t g_build_end_time = 0;

static void print_usage(const char* prog);
static void print_build_time(void);
static int  bootstrap(int argc, char** argv);
static int  build_artifact(artifact_info* artifact, build_mode mode);
static int  compile_module(module_info* module, build_mode mode, int* obj_count);
static int  compile_directory(module_info* module, const char** cc, int input_idx,
                              int output_idx, int* obj_count);
static int  add_object_files(const char* obj_dir, const char** cmd, int* idx);
static int  link_artifact(artifact_info* artifact);
static int  setup_vulkan_paths(void);
static void ensure_directories_exist(void);

static int find_xdg_shell_xml(const char* protocols_dir, char* output_path);
static artifact_info* find_artifact(const char* name);

int main(int argc, char** argv) {
    if (bootstrap(argc, argv)) {
        return 0;  // Bootstrap handled everything
    }

    g_build_start_time = os_time_usec();

    if (setup_vulkan_paths() != 0) {
        return -1;
    }
    ensure_directories_exist();

    build_mode  mode = BUILD_MODE_DEBUG;
    const char* target_name = ACTIVE_TARGET;

    if (argc >= 2) {
        if (strcmp(argv[1], "release") == 0) {
            mode = BUILD_MODE_RELEASE;
            if (argc > 2) {
                target_name = argv[2];
            }
        } else if (strcmp(argv[1], "help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            target_name = argv[1];
        }
    }

    artifact_info* artifact = find_artifact(target_name);
    if (!artifact) {
        printf("Error: Unknown artifact '%s'\n", target_name);
        return -1;
    }

    int result = build_artifact(artifact, mode);
    if (result == 0) {
        print_build_time();
    } else {
        printf("\ncompilation failed\n\n");
    }

    return result;
}

static int setup_vulkan_paths(void) {
    char* vulkan_sdk = os_getenv("VULKAN_SDK");
    if (!vulkan_sdk) {
        printf("Error: $VULKAN_SDK is not set\n");
        return -1;
    }

    snprintf(g_vulkan_include, OS_MAX_PATH, "-I%s" OS_PATH_SEP "include",
             vulkan_sdk);
    snprintf(g_vulkan_link, OS_MAX_PATH, "-L%s" OS_PATH_SEP "lib", vulkan_sdk);
    snprintf(g_vulkan_glslc, OS_MAX_PATH,
             "%s" OS_PATH_SEP "bin" OS_PATH_SEP "glslc" OS_EXE_EXT, vulkan_sdk);

    g_includes[INCLUDE_ID_VULKAN] = g_vulkan_include;
    g_links[LINK_ID_VULKAN] = g_vulkan_link;

    return 0;
}

static void ensure_directories_exist(void) {
    if (!dir_exists(BIN_DIR)) {
        dir_create(BIN_DIR);
    }
    if (!dir_exists(OBJ_DIR)) {
        dir_create(OBJ_DIR);
    }
    if (!dir_exists(BIN_ASSETS)) {
        dir_create(BIN_ASSETS);
    }
    if (!dir_exists(BIN_SHADERS)) {
        dir_create(BIN_SHADERS);
    }
}

static artifact_info* find_artifact(const char* name) {
    for (int i = 0; i < ARTIFACT_ID_COUNT; i++) {
        if (strcmp(g_artifacts[i].name, name) == 0) {
            return &g_artifacts[i];
        }
    }
    return NULL;
}

void print_usage(const char* prog) {
    printf("Usage: %s [command|target] [target]\n\n", prog);

    printf("Commands:\n");
    printf("  <target>          Build target in debug mode\n");
    printf("  release [target]  Build target in release mode\n");
    printf("  help              Show this help\n\n");

    printf("Available targets:\n");
    for (int i = 0; i < ARTIFACT_ID_COUNT; i++) {
        printf("  %s\n", g_artifacts[i].name);
    }
    printf("\nDefault target: %s\n", ACTIVE_TARGET);
}

int bootstrap(int argc, char** argv) {
    const char* build_exe = argv[0];
    const char* build_src = __FILE__;

    uint64_t exe_time = file_modtime(build_exe);
    uint64_t src_time = file_modtime(build_src);

    if (src_time <= exe_time) {
        return 0;
    }

    printf("\n=== bootstrapping: build script is outdated ===\n");
    char temp_exe[OS_MAX_PATH];
    snprintf(temp_exe, sizeof(temp_exe), "%s.tmp.%d", build_exe, os_pid());

    const char* rebuild_cmd[] = {COMPILER, "-o", temp_exe, build_src, NULL};

    int status = os_exec(rebuild_cmd);
    if (status != 0) {
        printf("error: bootstrap rebuild failed (exit code %d)\n", status);
        file_delete(temp_exe);
        return 1;
    }

    if (!file_rename(temp_exe, build_exe)) {
        printf("error: failed to replace executable\n");
        file_delete(temp_exe);
        return 1;
    }

    printf("bootstrap successful - executing...\n\n");
    const char* cmd[argc + 1];
    for (int i = 0; i < argc; i++) {
        cmd[i] = argv[i];
    }
    cmd[argc] = NULL;
    if (os_exec(cmd) != 0) {
        // Should never reach here if exec succeeds
        printf("error: failed to re-execute after bootstrap\n");
    }
    return 1;
}

int compile_module(module_info* module, build_mode mode, int* obj_count) {
    if (module->pre_build) {
        if (module->pre_build() != 0) return 1;
    }
    module->flags[FLAG_ID_O2] = mode == BUILD_MODE_RELEASE;
    module->flags[FLAG_ID_GSYMBOLS] = mode == BUILD_MODE_DEBUG;
    module->defines[DEFINE_ID_DEBUG] = mode == BUILD_MODE_DEBUG;

    const char* cc[MAX_COMPILE_FLAGS] = {0};

    int idx = 0;
    int input_idx = 2;
    int output_idx = 4;

    cc[idx++] = COMPILER;
    cc[idx++] = "-c";
    idx++;
    cc[idx++] = "-o";
    idx++;
    for (int i = 0; i < INCLUDE_ID_COUNT; i++) {
        if (module->includes[i]) {
            cc[idx++] = g_includes[i];
        }
    }
    for (int i = 0; i < FLAG_ID_COUNT; i++) {
        if (module->flags[i]) {
            cc[idx++] = g_flags[i];
        }
    }
    for (int i = 0; i < DEFINE_ID_COUNT; i++) {
        if (module->defines[i]) {
            cc[idx++] = g_defines[i];
        }
    }

    return compile_directory(module, cc, input_idx, output_idx, obj_count);
}

static int compile_directory(module_info* module, const char** cc, int input_idx,
                             int output_idx, int* obj_count) {
    dir_info stack[MAX_FOLDER_DEPTH] = {0};
    int      top = 0;

    stack[top].iter = dir_open(module->src_dir);
    if (!stack[top].iter) {
        printf("error: cannot open directory '%s'\n", module->src_dir);
        return 1;
    }
    strcpy(stack[top].path, module->src_dir);

    char ifile[OS_MAX_PATH];

    while (top >= 0) {
        const char* entry = dir_read(stack[top].iter);
        if (!entry) {
            dir_close(stack[top].iter);
            top--;
            continue;
        }

        snprintf(ifile, OS_MAX_PATH, "%s/%s", stack[top].path, entry);

        if (path_isdir(ifile)) {
            if (top + 1 < MAX_FOLDER_DEPTH) {
                top++;
                stack[top].iter = dir_open(ifile);
                strcpy(stack[top].path, ifile);
            }
            continue;
        }

        const char* ext = path_extension(ifile);
        if (strcmp(ext, ".c") != 0) {
            continue;
        }
        const char* filename = strrchr(entry, *OS_PATH_SEP);
        filename = filename ? filename + 1 : entry;

        const char* dot = strrchr(filename, '.');
        if (!dot) continue;

        char   basename[128];
        size_t len = dot - filename;
        strncpy(basename, filename, len);
        basename[len] = '\0';

        char ofile[OS_MAX_PATH];
        snprintf(ofile, OS_MAX_PATH, "%s" OS_PATH_SEP "%s" OS_PATH_SEP "%s.o",
                 OBJ_DIR, module->name, basename);
        uint64_t imod = file_modtime(ifile);
        uint64_t omod = file_modtime(ofile);

        if (imod <= omod) { 
            continue;
        }

        cc[input_idx] = ifile;
        cc[output_idx] = ofile;

        printf("  %s.o\n", basename);

        int status = os_exec(cc);
        if (status != 0) {
            printf("Error: Compilation failed for %s (exit code %d)\n", ifile,
                   status);
            return 1;
        }

        (*obj_count)++;
    }

    return 0;
}

int build_artifact(artifact_info* artifact, build_mode mode) {
    printf("=== building artifact '%s' (%s mode) ===\n", artifact->name,
           mode == BUILD_MODE_DEBUG ? "debug" : "release");

    int total_obj_count = 0;
    for (int i = 0; i < artifact->module_count; i++) {
        module_info* module = &g_module_infos[artifact->modules[i]];

        char obj_dir[OS_MAX_PATH];
        snprintf(obj_dir, OS_MAX_PATH, "./%s" OS_PATH_SEP "%s", OBJ_DIR,
                 module->name);
        if (!dir_exists(obj_dir)) {
            dir_create_recurse(obj_dir);
        }
        module->flags[FLAG_ID_O2] = mode == BUILD_MODE_RELEASE;
        module->flags[FLAG_ID_GSYMBOLS] = mode == BUILD_MODE_DEBUG;
        module->defines[DEFINE_ID_DEBUG] = mode == BUILD_MODE_DEBUG;
        printf("compiling module '%s'\n", module->name);
        if (compile_module(module, mode, &total_obj_count) != 0) {
            return 1;
        }
    }

    printf("generating artifact '%s'\n", artifact->name);
    if (link_artifact(artifact) != 0) {
        return 1;
    }
    return 0;
}

int link_artifact(artifact_info* artifact) {
    const char* linker_cmd[MAX_COMPILE_FLAGS] = {0};
    int         idx = 0;
    char        out_path[128];
    snprintf(out_path, 128, "%s" OS_PATH_SEP "%s", BIN_DIR, artifact->name);
    linker_cmd[idx++] = COMPILER;
    linker_cmd[idx++] = "-o";
    linker_cmd[idx++] = out_path;

    int libs_added[LIB_ID_COUNT] = {0};
    int links_added[LINK_ID_COUNT] = {0};

    for (int mod_idx = 0; mod_idx < artifact->module_count; mod_idx++) {
        module_info* module = &g_module_infos[artifact->modules[mod_idx]];

        for (int i = 0; i < LIB_ID_COUNT; i++) {
            if (module->libs[i] && !libs_added[i]) {
                linker_cmd[idx++] = g_libs[i];
                libs_added[i] = 1;
            }
        }

        for (int i = 0; i < LINK_ID_COUNT; i++) {
            if (module->links[i] && !links_added[i] && g_links[i]) {
                linker_cmd[idx++] = g_links[i];
                links_added[i] = 1;
            }
        }
    }

    for (int mod_idx = 0; mod_idx < artifact->module_count; mod_idx++) {
        module_info* module = &g_module_infos[artifact->modules[mod_idx]];
        char         obj_dir[OS_MAX_PATH];
        snprintf(obj_dir, OS_MAX_PATH, "%s/%s", OBJ_DIR, module->name);

        if (add_object_files(obj_dir, linker_cmd, &idx) != 0) {
            return 1;
        }
    }

    int status = os_exec(linker_cmd);
    if (status != 0) {
        printf("Error: Linking failed (exit code %d)\n", status);
        return 1;
    }

    return 0;
}

static int add_object_files(const char* obj_dir, const char** cmd, int* idx) {
    dir_iter* iter = dir_open(obj_dir);
    if (!iter) return 0;

    char        path[OS_MAX_PATH];
    const char* entry;
    while ((entry = dir_read(iter)) != NULL) {
        snprintf(path, OS_MAX_PATH, "%s/%s", obj_dir, entry);

        if (path_isdir(path)) {
            if (add_object_files(path, cmd, idx) != 0) {
                dir_close(iter);
                return 1;
            }
        } else if (strstr(entry, ".o")) {
            // todo: this leaks memory, but it's a build tool
            cmd[(*idx)++] = strdup(path);
        }
    }

    dir_close(iter);
    return 0;
}

void print_build_time() {
    g_build_end_time = os_time_usec();
    float time_s = ((float)(g_build_end_time - g_build_start_time)) / 1000000;
    printf("\ncompilation finished - %.3f s\n", time_s);
}

int program_prebuild() {
    printf("compiling shaders...\n");
    dir_iter* iter = dir_open(SHADERS_DIR);
    if (!iter) {
        printf("failed to open shader directory" SHADERS_DIR);
        return 1;
    }
    int         err = 0;
    const char* entry;
    while ((entry = dir_read(iter))) {
        char input[128] = {0};
        char output[128] = {0};
        snprintf(input, 128, "%s" OS_PATH_SEP "%s", SHADERS_DIR, entry);
        snprintf(output, 128, "%s" OS_PATH_SEP "%s.spv", BIN_SHADERS, entry);
        const char* cmd[] = {
            g_vulkan_glslc, input, "-o", output, NULL,
        };

        uint64_t input_mod = file_modtime(input);
        uint64_t output_mod = file_modtime(output);
        if (input_mod <= output_mod && file_exists(output)) {
            continue;
        }
        printf("  %s\n", entry);
        err = os_exec(cmd);
        if (err) break;
    }
    dir_close(iter);
    if (err) {
        printf("shader compilation failed!\n");
    }
    return err;
}

static int xdg_shell_prebuild(void) {
    const char* protocols_dir = "/usr/share/wayland-protocols";
    const char* xdg_shell_c = "./external/wayland/xdg-shell.c";
    const char* xdg_shell_h = "./external/wayland/xdg-shell.h";

    if (file_exists(xdg_shell_c) && file_exists(xdg_shell_h)) {
        return 0;
    }

    if (!dir_exists("./external/wayland")) {
        dir_create_recurse("./external/wayland");
    }

    char xml_path[OS_MAX_PATH];
    if (find_xdg_shell_xml(protocols_dir, xml_path) != 0) {
        printf("error: could not find xdg-shell.xml in %s\n", protocols_dir);
        return 1;
    }

    printf("found xdg_shell spec: %s\n", strrchr(xml_path, '/') + 1);

    const char* version_cmd[] = {"wayland-scanner", "--version", NULL};
    if (os_exec(version_cmd) != 0) {
        printf("error: wayland-scanner not found in PATH\n");
        return 1;
    }

    printf("running wayland-scanner... %s\n", xml_path);
    const char* header_cmd[] = {"wayland-scanner", "client-header", xml_path,
                                xdg_shell_h, NULL};
    const char* source_cmd[] = {"wayland-scanner", "private-code", xml_path,
                                xdg_shell_c, NULL};

    if (os_exec(header_cmd) != 0) {
        printf("error: failed to generate xdg-shell header\n");
        return 1;
    }

    if (os_exec(source_cmd) != 0) {
        printf("error: failed to generate xdg-shell source\n");
        return 1;
    }

    return 0;
}

static int find_xdg_shell_xml(const char* protocols_dir, char* output_path) {
    dir_info stack[MAX_FOLDER_DEPTH] = {0};
    int      top = 0;

    printf("searching for xdg-shell.xml in %s\n", protocols_dir);

    stack[top].iter = dir_open(protocols_dir);
    if (!stack[top].iter) {
        return 1;
    }
    strcpy(stack[top].path, protocols_dir);

    char path[OS_MAX_PATH];

    while (top >= 0) {
        const char* entry = dir_read(stack[top].iter);
        if (!entry) {
            dir_close(stack[top].iter);
            top--;
            continue;
        }

        snprintf(path, OS_MAX_PATH, "%s/%s", stack[top].path, entry);

        if (path_isdir(path)) {
            if (top + 1 < MAX_FOLDER_DEPTH) {
                top++;
                stack[top].iter = dir_open(path);
                strcpy(stack[top].path, path);
            }
            continue;
        }

        if (strcmp(entry, "xdg-shell.xml") == 0) {
            strcpy(output_path, path);
            // Clean up remaining iterators
            while (top >= 0) {
                dir_close(stack[top].iter);
                top--;
            }
            return 0;
        }
    }

    return 1;  // Not found
}

#include "./src/core/os_linux.c"
