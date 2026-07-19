#pragma once
#include <defines.h>

#ifndef GPIO_HIGH
    #define GPIO_HIGH   true
#endif
#ifndef GPIO_LOW
    #define GPIO_LOW    false
#endif


#ifdef __cplusplus
extern "C" {
#endif

bool8  gpio_wait_for_level(uint8 pin, uint8 level, uint32 timeout_us);
bool8  gpio_wait_for_level_count(uint8 pin, uint8 level, uint32 timeout_us, uint32 *count);

#ifdef __cplusplus
}
#endif