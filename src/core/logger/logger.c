#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <core/logger/logger.h>

static const char *const log_level_strings[6] = {
    "[FATAL]: ",
    "[ERROR]: ",
    "[WARN ]: ",
    "[INFO ]: ",
    "[DEBUG]: ",
    "[TRACE]: "
}; 

void hkk_log_output(LogLevel_t level, const char* message, ...) {
    if(message == NULL) return;
    if((int16)level < 0 || (int16)level >= MAX_LOG_LEVEL) level = LOG_LEVEL_INFO;

    bool8 err = level < LOG_LEVEL_WARN;
    const char* current_log_level = log_level_strings[level];

    char log_buffer[HLOG_MSG_MAX_LEN];
    memset(log_buffer, 0, sizeof(log_buffer));

    va_list argp;
    va_start(argp, message);
    int32 written = vsnprintf(log_buffer, HLOG_MSG_MAX_LEN, message, argp);
    va_end(argp);
    
    if(written < 0) return;
    if(written >= HLOG_MSG_MAX_LEN) {}  // Do nothing, for now

    char msg[HLOG_MSG_MAX_LEN];
    snprintf(msg, sizeof(msg), "%s%s\n", current_log_level, log_buffer);
    
    HPRINT(msg);                        // SEGGER print shorthand macro
    if(err) printf("%s", msg);
    else printf("%s", msg);
    
    return;
}