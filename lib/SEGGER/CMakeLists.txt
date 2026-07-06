# --- SEGGER RTT --------------------------------------------------------------
add_library(segger_rtt 
    STATIC
        SEGGER_RTT.c
        SEGGER_RTT_printf.c
        SEGGER_RTT_ASM_ARMv7M.S
)

# --- SEGGER SYSTEMVIEW CORE -------------------------------------------------------
add_library(segger_sysview 
    STATIC
        SEGGER_SYSVIEW.c
)

# --- SEGGER RTOS HOOK --------------------------------------------------------
add_library(segger_sysview_freertos 
    STATIC
        FreeRTOSV11/SEGGER_SYSVIEW_FreeRTOS.c
        FreeRTOSV11/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c
)

target_link_libraries(segger_sysview_freertos
    PRIVATE
        FreeRTOS-Kernel
)