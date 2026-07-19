// Newlib
#include <stdio.h>

// hkk
#include <defines.h>

#include <core/logger/logger.h>
#include <core/gpio/gpio.h>

// hkk c-iot-drivers
#include <hkk/utils/utils.hpp>

#include <hkk/bus/i2c/i2c.hpp>
#include <hkk/bus/spi/spi.hpp>
#include <hkk/bus/uart/uart.hpp>
#include <hkk/storage/nvm.hpp>

#include <hkk/drivers/sgp30/sgp30.hpp>
#include <hkk/drivers/dht20/dht20.hpp>
#include <hkk/drivers/bme280/bme280.hpp>
#include <hkk/drivers/pms5003/pms5003.hpp>

// Pico SDK
#include <pico/mutex.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>


int main(void) {
    stdio_init_all();
    sleep_ms(1 * SECOND);

    HTRACE("main.cpp -> main(-):int");
    HINFO("[MAIN   ] System startup");
    
    // Your code here

    HFATAL("[MAIN   ] SYSTEM SHUTDOWN");
    return OK;
}