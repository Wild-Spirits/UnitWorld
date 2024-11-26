#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define VEGA_PLATFORM_WINDOWS         1
    #define VEGA_PLATFORM_WINDOWS_DESKTOP 1
    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif
#elif defined(__linux__) || defined(__gnu_linux__)
    // Linux OS
    #define VEGA_PLATFORM_LINUX 1
    #if defined(__ANDROID__)
        #define VEGA_PLATFORM_ANDROID 1
    #else
        #define VEGA_PLATFORM_LINUX_DESKTOP 1
    #endif
#elif defined(__unix__)
    // Catch anything not caught by the above.
    #define VEGA_PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
    // Posix
    #define VEGA_PLATFORM_POSIX 1
#elif __APPLE__
    // Apple platforms
    #define VEGA_PLATFORM_APPLE 1
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        // iOS Simulator
        #define VEGA_PLATFORM_IOS           1
        #define VEGA_PLATFORM_IOS_SIMULATOR 1
    #elif TARGET_OS_IPHONE
        #define VEGA_PLATFORM_IOS 1
    // iOS device
    #elif TARGET_OS_MAC
    // Other kinds of Mac OS
    #else
        #error "Unknown Apple platform"
    #endif
#else
    #error "Unknown platform!"
#endif

#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP) || defined(VEGA_PLATFORM_LINUX_DESKTOP)
    #define VEGA_PLATFORM_DESKTOP 1
#endif
