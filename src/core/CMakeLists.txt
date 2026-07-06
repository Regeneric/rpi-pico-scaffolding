add_subdirectory(logger)
add_subdirectory(gpio)

add_library(libcore INTERFACE)

target_include_directories(libcore INTERFACE
    ${PROJECT_INC_DIR}
)

target_link_libraries(libcore INTERFACE
    liblogger
    libgpio
)