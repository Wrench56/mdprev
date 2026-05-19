#define CF_DISABLE_FILE_HASH

#include "cforge.h"

#include <stdio.h>

#define APP_NAME "mdprev"
#define BUILD_DIR "build"
#define APP_PATH (BUILD_DIR "/" APP_NAME)

#define CMARK_DIR "cmark-gfm"
#define CMARK_LIB CMARK_DIR "/build/src/libcmark-gfm.a"
#define CMARK_EXTLIB CMARK_DIR "/build/extensions/libcmark-gfm-extensions.a"

#define CC_TAG "[" CF_YELLOW "CC" CF_RESET "] "
#define CM_TAG "[" CF_MAGENTA "CM" CF_RESET "] "
#define LD_TAG "[" CF_CYAN "LD" CF_RESET "] "
#define RN_TAG "[" CF_GREEN "RN" CF_RESET "] "

bool was_rebuilt = false;

#if CF_VERSION_BELOW(1, 0, 2)
#error "CForge too old!"
#endif

CF_CONFIG(release) {
    CF_SET_ENV(mode, "release");

    CF_SET_ENV(cflags, "-O2");
    CF_SET_ENV(lflags, CMARK_EXTLIB " " CMARK_LIB);
    CF_SET_ENV(includes, "-Iincludes/ -I" CMARK_DIR "/src");
}

CF_CONFIG(debug) {
    CF_SET_ENV(mode, "debug");

    CF_SET_ENV(cflags, "-g");
    CF_SET_ENV(lflags, CMARK_EXTLIB " " CMARK_LIB);
    CF_SET_ENV(includes, "-Iincludes/ -I" CMARK_DIR "/src");
}

CF_TARGET(
    release,
    CF_WITH_CONFIG(release),
    CF_DEPENDS(build),
    CF_HELP_STRING("Build in release mode")
) {
    CF_NOP();
}

CF_TARGET(
    debug,
    CF_WITH_CONFIG(debug),
    CF_DEPENDS(build),
    CF_HELP_STRING("Build in debug mode")
) {
    CF_NOP();
}

CF_TARGET(run, CF_DEPENDS(debug), CF_HELP_STRING("Run mdprev")) {
    printf(RN_TAG "Running %s...\n", APP_NAME);
    CF_RUN("./%s/%s", BUILD_DIR, APP_NAME);
}

CF_TARGET(build, CF_DEPENDS(link), CF_HIDDEN) {
    if (!was_rebuilt) {
        return;
    }
    printf(
        "\n=========================\nBuilt using mode: "
        "%s\n=========================\n\n",
        CF_ENV(mode)
    );
}

CF_TARGET(link, CF_DEPENDS(compile), CF_DEPENDS(cmark), CF_HIDDEN) {
    if (CF_FILE_NOT_UTD(APP_PATH) || was_rebuilt) {
        was_rebuilt = true;
        CF_BANNER(LD_TAG "Linking...");
        char* object_files = CF_JOIN_GLOB(CF_GLOB(BUILD_DIR "/*.o"), " ");
        printf(LD_TAG "  %s\n", object_files);
        CF_RUN("cc %s %s -o %s", object_files, CF_ENV(lflags), APP_PATH);
        CF_FILE_MARK_UTD(APP_PATH);
    }
}

static bool compile_pattern(const char* pattern) {
    bool rebuilt = false;

    for CF_GLOBS_EACH(pattern, file) {
        char* output = CF_MAP(
            file,
            CF_MAP_EXT("o"),
            CF_MAP_DIRS(BUILD_DIR "/"),
        );

        if (CF_FILE_NOT_UTD(file) || CF_FILE_NOT_UTD(output)) {
            rebuilt = true;
            CF_BANNER(CC_TAG "Compiling...");
            printf(CC_TAG "  %s\n", file);
            CF_RUNP("cc %s %s -c %s -o %s",
                CF_ENV(cflags),
                CF_ENV(includes),
                file,
                output
            );
            CF_FILE_MARK_UTDP(file);
            CF_FILE_MARK_UTDP(output);
        }
    }
    
    return rebuilt;
}

CF_TARGET(compile, CF_HIDDEN) {
    CF_MKDIR(BUILD_DIR);
    was_rebuilt |= compile_pattern("src/*.c");
    was_rebuilt |= compile_pattern("src/*/*.c");
}

CF_TARGET(cmark, CF_HIDDEN) {
    if CF_FILE_NOT_UTD (CMARK_LIB) {
        CF_RUN(
            "cd %s && mkdir -p build && cd build && cmake "
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -Wno-dev .. && make",
            CMARK_DIR
        );
        CF_RUN("strip --strip-debug %s", CMARK_LIB);
        CF_RUN("ranlib %s", CMARK_LIB);
        CF_FILE_MARK_UTD(CMARK_LIB);
    }
}
