add_subdirectory(core)
# add_subdirectory(other)
# add_subdirectory(directory)
# add_subdirectory(to)
# add_subdirectory(include)

add_executable(${PROJECT_NAME}
    main.cpp
    # some.cpp
    # other.cpp
    # file.cpp
    # to.cpp
    # include.cpp
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        # libsome
        # libother
        # liblibrary
        # libto
        # libinclude

        segger_sysview
        segger_sysview_freertos
        segger_rtt

        # ---- HKK IoT Drivers
        hkk::bus_i2c_rp2350
        # hkk::bus_spi_rp2350

        # ---- Pico SDK core ----
        pico_stdlib
        pico_stdio
        # pico_stdio_uart
        # pico_stdio_usb
        # pico_stdio_semihosting
        # pico_stdio_rtt

        # pico_multicore
        # pico_sync
        # pico_time
        # pico_util
        # pico_rand
        # pico_unique_id
        # pico_binary_info
        # pico_bootrom
        # pico_flash
        # pico_malloc
        # pico_atomic

        # pico_float
        # pico_double
        # pico_int64_ops
        # pico_divider
        # pico_bit_ops
        # pico_mem_ops

        # pico_async_context
        # pico_async_context_base
        # pico_async_context_poll
        # pico_async_context_threadsafe_background
        # pico_async_context_freertos

        # hardware_adc
        # hardware_base
        # hardware_claim
        # hardware_clocks
        # hardware_dma
        # hardware_exception
        # hardware_flash
        # hardware_gpio
        # hardware_i2c
        # hardware_interp
        # hardware_irq
        # hardware_pio
        # hardware_pll
        # hardware_pwm
        # hardware_resets
        # hardware_rtc
        # hardware_spi
        # hardware_sync
        # hardware_timer
        # hardware_uart
        # hardware_vreg
        # hardware_watchdog
        # hardware_xosc

        # pico_cyw43_arch_none
        # pico_cyw43_arch_lwip_poll
        # pico_cyw43_arch_lwip_threadsafe_background
        # pico_cyw43_arch_lwip_sys_freertos
        # pico_cyw43_driver

        # pico_lwip
        # pico_lwip_arch
        # pico_lwip_freertos
        # pico_lwip_nosys

        # pico_btstack_ble
        # pico_btstack_classic
        # pico_btstack_cyw43
        # pico_btstack_run_loop_async_context
        # pico_btstack_run_loop_poll

        # tinyusb_device
        # tinyusb_host
        # tinyusb_board

        # pico_mbedtls
        # pico_lwip_mbedtls

        # ---- RP2350 specific extras ----
        # hardware_boot_lock
        # hardware_dcp
        # hardware_hazard3
        # hardware_rcp
        # hardware_sync_spin_lock
        # hardware_ticks
        # hardware_xip_cache

        # ---- FreeRTOS ----
        FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})


target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Release>:-Os>

        # Minimal C++ feature set
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)