#pragma once
#include <defines.h>

#ifdef HPLATFORM_ARM
    #define HLOG_LEVEL                          LOG_DEBUG                       // TRACE | DEBUG | INFO | WARN | ERROR

    #define HTASK_PRIORITY_LOW                  1                               // For RTOS tasks
    #define HTASK_PRIORITY_MEDIUM               2                               // For RTOS tasks
    #define HTASK_PRIORITY_HIGH                 3                               // For RTOS tasks

    #define HENABLE_WIFI                        true                            // Enable or disable WiFi module
    #if HENABLE_WIFI
        #ifndef WIFI_SSID
            #define HWIFI_SSID                  "pico"                          // WiFi network name
        #else
            #define HWIFI_SSID                  WIFI_SSID                       // If passed from CMake
        #endif
        #ifndef WIFI_PASSWORD 
            #define HWIFI_PASS                  "0123456789"                    // WiFi password
        #else
            #define HWIFI_PASS                  WIFI_PASSWORD                   // If passed from CMake
        #endif
    #endif

    // Debugging
    #define HSEGGER_SYSVIEW                     true                            // Use SEGGER SysView for debugging
    #if HSEGGER_SYSVIEW
        #define HPRINT(val)                     SEGGER_SYSVIEW_PrintfHost(val)  // Shorthand macro
    #else
        #define HPRINT(val)
    #endif
#endif // HPLATFORM_ARM