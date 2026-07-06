add_library(liblogger 
    STATIC
        logger.c
)

target_include_directories(liblogger
    PUBLIC
        ${PROJECT_INC_DIR}
)