/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"
#include "stdlib.h"

// We're going to use PIO to print "Hello, world!" on the same GPIO which we
// normally attach UART0 to.
#define PIO_TX_PIN 0

// Check the pin is compatible with the platform
#if PIO_TX_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

int main() {
    const uint SERIAL_BAUD = 115200;
        
    // UART setup
    uart_init(uart0, SERIAL_BAUD);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
    char buffer[2] = {'2', '\0'};

    // // RX Code - Not used for master
    // while(true){
    //     uart_read_blocking(uart0, (uint8_t*) buffer, sizeof(int));
    //     if(*buffer != 0)
    //         break;
    // }
    

    // TX Code - Used for all 
    while(true) {
        // uart_puts(uart0, "Working");
        uart_puts(uart0, buffer);
        sleep_ms(100); // Increased delay to match receiver
    }
}
