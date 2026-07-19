#pragma once

#ifdef HPLATFORM_RISCV
    // Logging
    #define HLOG_LEVEL                          LOG_TRACE                       // TRACE | DEBUG | INFO | WARN | ERROR

    // RTOS
    #define HTASK_PRIORITY_LOW                  1                               // For RTOS tasks
    #define HTASK_PRIORITY_MEDIUM               2                               // For RTOS tasks
    #define HTASK_PRIORITY_HIGH                 3                               // For RTOS tasks

    // WiFi
    #ifndef HENABLE_WIFI
        #define HENABLE_WIFI                    0                               // Enable or disable WiFi module
    #endif
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
    #ifndef HSEGGER_SYSVIEW
        #define HSEGGER_SYSVIEW                 0                               // Use SEGGER SysView for debugging
    #endif
    #if HSEGGER_SYSVIEW
        #define HPRINT(val)                     SEGGER_SYSVIEW_PrintfHost(val)  // Shorthand macro
    #else
        #define HPRINT(val)                     do {} while (0)
    #endif
#endif // HPLATFORM_RISCV