#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

#include "mcp2515/mcp2515.h"

// SPI Defines (can)
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_SCK  18
#define PIN_MOSI 19

#define CAN_CS   17

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

// uint16_t mcp3208_read(uint8_t ch) {
//     uint8_t tx[3];
//     uint8_t rx[3];

//     tx[0] = 0x06 | ((ch & 0x04) >> 2);   // start + single
//     tx[1] = (ch & 0x03) << 6;
//     tx[2] = 0x00;

//     gpio_put(PIN_CS, 0);
//     spi_write_read_blocking(SPI_PORT, tx, rx, 3);
//     gpio_put(PIN_CS, 1);

//     uint16_t value = ((rx[1] & 0x0F) << 8) | rx[2];
//     return value;
// }

int main()
{
    stdio_init_all();

    sleep_ms(2000);
    printf("System Booting...\n");

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    //gpio_set_function(CAN_CS,   GPIO_FUNC_SIO);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(CAN_CS);
    gpio_set_dir(CAN_CS, GPIO_OUT);
    gpio_put(CAN_CS, 1);


    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    

    // CAN INIT 
    MCP2515 mcp2515(SPI_PORT, CAN_CS, PIN_MOSI, PIN_MISO, PIN_SCK);

    mcp2515.reset();
    mcp2515.setBitrate(CAN_5KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

    printf("CAN Initialized\n");

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
    }
    //  else {
    //     // This is only temporary to allow flashing of same program to master/slave 
    //      while(true) {
    //         uart_putc(UART_ID, 7 + 48);
    //         // printf("Sending: %c\n", 7 + 48);
    //         sleep_ms(500);
    //     }
    // }
    sleep_ms(1000);

    if(IS_RECEIVER) {
        printf("Receiver Mode\n");

        can_frame frame;

        while (true) {
            MCP2515::ERROR err = mcp2515.readMessage(&frame);

            if (err == MCP2515::ERROR_OK) {
                printf("CAN ID: 0x%x | Data: ", frame.can_id);

                for (int i = 0; i < frame.can_dlc; i++) {
                    printf("%c ", frame.data[i]);
                }
                printf("\n");
            }
            else if (err != MCP2515::ERROR_NOMSG) {
                printf("CAN Read Error: %d\n", err);
            }

            sleep_ms(10);
        }

    } else {
        printf("Sender Mode\n");

        // test 
        can_frame frame{0x100, 8, "Hello!!"};

        while (true) {

            // UART test
            uart_putc(UART_ID, 'A');

            MCP2515::ERROR err = mcp2515.sendMessage(&frame);

            printf("Sent CAN | Error: %d\n", err);

            sleep_ms(1000);
        }
    }

}
