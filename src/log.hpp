#pragma once

// Unified logging that maps to Android logcat on device and to stdio on the
// host. Format strings should NOT include a trailing newline; logcat splits on
// log entries and the host fallback appends one automatically.

#ifndef LOG_TAG
#define LOG_TAG "Nv12Bench"
#endif

#if defined(__ANDROID__)

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#else

#include <cstdio>

#define LOGI(...)                       \
    do {                                \
        std::fprintf(stdout, "[I/" LOG_TAG "] "); \
        std::fprintf(stdout, __VA_ARGS__);        \
        std::fprintf(stdout, "\n");     \
    } while (0)

#define LOGW(...)                       \
    do {                                \
        std::fprintf(stderr, "[W/" LOG_TAG "] "); \
        std::fprintf(stderr, __VA_ARGS__);        \
        std::fprintf(stderr, "\n");     \
    } while (0)

#define LOGE(...)                       \
    do {                                \
        std::fprintf(stderr, "[E/" LOG_TAG "] "); \
        std::fprintf(stderr, __VA_ARGS__);        \
        std::fprintf(stderr, "\n");     \
    } while (0)

#endif
