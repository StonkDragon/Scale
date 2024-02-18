#ifndef VERSION
#define VERSION 24.0
#endif

#ifdef _WIN32
#define SCL_ROOT_DIR "C:\\Scale"
#else
#define SCL_ROOT_DIR "/opt"
#endif

#if defined(__APPLE__)
#define LIB_SCALE_FILENAME "libScaleRuntime.dylib"
#elif defined(__linux__)
#define LIB_SCALE_FILENAME "libScaleRuntime.so"
#elif defined(_WIN32)
#define LIB_SCALE_FILENAME "ScaleRuntime.dll"
#endif
