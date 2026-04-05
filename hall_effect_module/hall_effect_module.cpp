#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_SETUP_RETRIES 5    // Maximum number of UART Setup retries for retransmission
#define UART_SLEEP_MS 100       // Time for intermediate retries to take place

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define IS_RECEIVER 1

//      ┌───────────────────────────────────────┐
//      │          Hall Effect Board            │
//      └───────────────────────────────────────┘
//      Last Updated: 3.29
//      Last Update Content: 
//     

void uart_setup(uint8_t* can_id) {
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }
    sleep_ms(50);
    // Read 1 char from UART, store in can_id
    uart_read_blocking(UART_ID, can_id, 1);
    // Loop through UART retransmissions  
    for(int counter = 0; counter < UART_SETUP_RETRIES; counter++) {
        uart_putc(UART_ID, (*can_id) + 1);
        sleep_ms(UART_SLEEP_MS);
    }

    // TODO: Only used for debugging
    if(*can_id != 0) {
        printf("UART Succeeded!: %c\n", *can_id); 
    } else {
        printf("UART Failed!\n");
    }
}

int main()
{
    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Variable Definitions
    uint8_t can_id = 0;

    // UART Setup for obtaining CAN ID
    // uart_setup(&can_id);

    // Send out a string, with CR/LF conversions
    sleep_ms(5000);
    printf("Successfully booted up!\n");

    // TODO: Remove if-statement when making master-specific program
    if(IS_RECEIVER) {
        // Initializes the CAN ID for forwarding
        uart_setup(&can_id);

    } else {
        // This is only temporary to allow flashing of same program to master/slave 
         while(true) {
            uart_putc(UART_ID, 7 + 48);
            // printf("Sending: %c\n", 7 + 48);
            sleep_ms(500);
        }
    }
}
