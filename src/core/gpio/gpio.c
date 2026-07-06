#include <core/gpio/gpio.h>

#include <pico/stdlib.h>
#include <hardware/gpio.h>

bool8 gpio_wait_for_level(uint8 pin, uint8 level, uint32 usTimeout) {
    uint32 startTime = time_us_32();
    while(gpio_get(pin) == level) {
        if((time_us_32() - startTime) > usTimeout) return false;
    } return true;
}

bool8 gpio_wait_for_level_count(uint8 pin, uint8 level, uint32 usTimeout, uint32 *count) {
    uint32 startTime = time_us_32();
    while(gpio_get(pin) == level) {
        if((time_us_32() - startTime) > usTimeout) return false;
    } uint32 endTime = (time_us_32() - startTime);
    
    *count = endTime;
    return true;
}