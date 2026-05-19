#define CF_DISABLE_FILE_HASH

#include "cforge.h"

#include <stdio.h>

#define APP_NAME "mdprev"
#define BUILD_DIR "build"
#define APP_PATH (BUILD_DIR "/" APP_NAME)
#define AUTOGEN_DIR "includes/autogen"

#define STYLESHEET_HEADER AUTOGEN_DIR "/stylesheet.h"

#define CMARK_DIR "cmark-gfm"
#define CMARK_LIB CMARK_DIR "/build/src/libcmark-gfm.a"
#define CMARK_EXTLIB CMARK_DIR "/build/extensions/libcmark-gfm-extensions.a"

#define MATHJAXMIN "MathJax/tex-svg.js"
#define MATHJAXMIN_HEADER AUTOGEN_DIR "/mathjax.h"

#define PRISM_JS "src/scripts/prism.js"
#define PRISM_JS_HEADER AUTOGEN_DIR "/prism_js.h"
#define PRISM_CSS "src/stylesheets/prism.css"
#define PRISM_CSS_HEADER AUTOGEN_DIR "/prism_css.h"

#define CC_TAG "[" CF_YELLOW "CC" CF_RESET "] "
#define CM_TAG "[" CF_MAGENTA "CM" CF_RESET "] "
#define MJ_TAG "[" CF_MAGENTA "MJ" CF_RESET "] "
#define PM_TAG "[" CF_MAGENTA "PM" CF_RESET "] "
#define GN_TAG "[" CF_BLUE "GN" CF_RESET "] "
#define LD_TAG "[" CF_CYAN "LD" CF_RESET "] "
#define RN_TAG "[" CF_GREEN "RN" CF_RESET "] "

bool was_rebuilt = false;
bool recompile_templater = false;

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
        bool force = false;
        char* output = CF_MAP(
            file,
            CF_MAP_EXT("o"),
            CF_MAP_DIRS(BUILD_DIR "/"),
        );

        if (strcmp("src/gen/templater.c", file) == 0 && recompile_templater) {
            force = true;
        }

        if (CF_FILE_NOT_UTD(file) || CF_FILE_NOT_UTD(output) || force) {
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

CF_TARGET(
    compile,
    CF_DEPENDS(gencss),
    CF_DEPENDS(mathjax),
    CF_DEPENDS(prismjs),
    CF_HIDDEN
) {
    CF_MKDIR(BUILD_DIR);
    was_rebuilt |= compile_pattern("src/*.c");
    was_rebuilt |= compile_pattern("src/*/*.c");
}

CF_TARGET(cmark, CF_HIDDEN) {
    if CF_FILE_NOT_UTD(CMARK_LIB) {
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

CF_TARGET(gencss, CF_HIDDEN) {
    CF_MKDIR(AUTOGEN_DIR);
    const char* stylesheet_src = CF_ENV(MDPREV_STYLESHEET);
    if (stylesheet_src == NULL) {
        stylesheet_src = "src/stylesheets/github-style-light.css";
    }

    if (CF_FILE_NOT_UTD(STYLESHEET_HEADER) || CF_FILE_NOT_UTD(stylesheet_src)) {
        CF_BANNER("%s", GN_TAG "Generating stylesheet.h");
        
        CF_RUN(
            "./tools/header_gen.sh STYLESHEET %s "
            STYLESHEET_HEADER,
            stylesheet_src
        );
        CF_FILE_MARK_UTD(STYLESHEET_HEADER);
        CF_FILE_MARK_UTD((char*) stylesheet_src);
        recompile_templater = true;
    }
}

CF_TARGET(mathjax, CF_HIDDEN) {
    if (CF_FILE_NOT_UTD(MATHJAXMIN_HEADER) || CF_FILE_NOT_UTD(MATHJAXMIN)) {
        CF_BANNER("%s", MJ_TAG "Generating mathjax.h");

        CF_RUN(
            "./tools/header_gen.sh MATHJAX %s "
            MATHJAXMIN_HEADER,
            MATHJAXMIN
        );
        CF_FILE_MARK_UTD(MATHJAXMIN_HEADER);
        CF_FILE_MARK_UTD(MATHJAXMIN);
        recompile_templater = true;
    }
}

CF_TARGET(prismjs, CF_HIDDEN) {
    if (CF_FILE_NOT_UTD(PRISM_JS) || CF_FILE_NOT_UTD(PRISM_JS_HEADER)) {
        CF_BANNER("%s", PM_TAG "Generating prism_js.h");

        CF_RUN(
            "./tools/header_gen.sh PRISM_JS %s "
            PRISM_JS_HEADER,
            PRISM_JS
        );
        CF_FILE_MARK_UTD(PRISM_JS_HEADER);
        CF_FILE_MARK_UTD(PRISM_JS);
        recompile_templater = true;
    }

    if (CF_FILE_NOT_UTD(PRISM_CSS) || CF_FILE_NOT_UTD(PRISM_CSS_HEADER)) {
        CF_BANNER("%s", PM_TAG "Generating prism_css.h");

        CF_RUN(
            "./tools/header_gen.sh PRISM_CSS %s "
            PRISM_CSS_HEADER,
            PRISM_CSS
        );
        CF_FILE_MARK_UTD(PRISM_CSS_HEADER);
        CF_FILE_MARK_UTD(PRISM_CSS);
        recompile_templater = true;
    }
}
