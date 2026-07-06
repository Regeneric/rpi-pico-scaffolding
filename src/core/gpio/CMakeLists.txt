add_library(libgpio
    STATIC
        gpio.c
)

target_include_directories(libgpio
    PUBLIC
        ${PROJECT_INC_DIR}
)

target_link_libraries(libgpio
    PUBLIC
        pico_stdlib 

        hardware_gpio
)