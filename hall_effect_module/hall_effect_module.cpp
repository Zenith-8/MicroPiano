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

volatile bool send_enable = false;
volatile uint8_t uart_char = 0;

#define CS_ADC1 20
#define CS_ADC2 21
#define NUM_KEYS_PER_ADC 8

//      ┌───────────────────────────────────────┐
//      │          Hall Effect Board            │
//      └───────────────────────────────────────┘
//      Last Updated: 3.29
//      Last Update Content: 
//     

// void uart_setup(uint8_t* can_id) {
//     while (uart_is_readable(UART_ID)) {
//         uart_getc(UART_ID);
//     }
//     sleep_ms(50);
//     // Read 1 char from UART, store in can_id
//     uart_read_blocking(UART_ID, can_id, 1);
//     // Loop through UART retransmissions  
//     for(int counter = 0; counter < UART_SETUP_RETRIES; counter++) {
//         uart_putc(UART_ID, (*can_id) + 1);
//         sleep_ms(UART_SLEEP_MS);
//     }

//     // TODO: Only used for debugging
//     if(*can_id != 0) {
//         printf("UART Succeeded!: %c\n", *can_id); 
//     } else {
//         printf("UART Failed!\n");
//     }
// }

void uart_check() {
    if (uart_is_readable(UART_ID)) {
        uint8_t c = uart_getc(UART_ID);

        if (c == '\n' || c == '\r') return;

        uart_char = c;
        send_enable = true;

        printf("UART Input: %c (%d)\n", c, c);
    }
}

void usb_check() {
    int c = getchar_timeout_us(0);

    if (c == PICO_ERROR_TIMEOUT) return;

    if (c == '\n' || c == '\r') return;

    uart_char = (uint8_t)c;
    send_enable = true;

    printf("USB Input: %c (%d)\n", uart_char, uart_char);
}

uint16_t mcp3208_read(uint cs_pin, uint8_t channel) {
    uint8_t tx[3];
    uint8_t rx[3];

    tx[0] = 0x06 | ((channel & 0x04) >> 2);
    tx[1] = (channel & 0x03) << 6;
    tx[2] = 0x00;

    gpio_put(cs_pin, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(cs_pin, 1);

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

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

    // CS_ADC1
    gpio_init(CS_ADC1);
    gpio_set_dir(CS_ADC1, GPIO_OUT);
    gpio_put(CS_ADC1, 1);

    // CS_ADC2
    gpio_init(CS_ADC2);
    gpio_set_dir(CS_ADC2, GPIO_OUT);
    gpio_put(CS_ADC2, 1);

    // // Set up our UART
    // uart_init(UART_ID, BAUD_RATE);
    // // Set the TX and RX pins by using the function select on the GPIO
    // // Set datasheet for more information on function select
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    

    // CAN INIT 
    MCP2515 mcp2515(SPI_PORT, CAN_CS, PIN_MOSI, PIN_MISO, PIN_SCK);

    mcp2515.reset();
    mcp2515.setBitrate(CAN_5KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

    printf("CAN Initialized\n");

    // Variable Definitions
    // uint8_t can_id = 0;

    // UART Setup for obtaining CAN ID
    // uart_setup(&can_id);

    // Send out a string, with CR/LF conversions
    sleep_ms(5000);
    printf("Successfully booted up!\n");



    // with ADC
    if (IS_RECEIVER) {
        printf("Pico1: UART TX + CAN RX Mode\n");

        // init UART (send)
        uart_init(UART_ID, BAUD_RATE);
        gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

        while (true) {
            // USB serial input，through UART to Pico2
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT && c != '\n' && c != '\r') {
                uart_putc(UART_ID, (uint8_t)c);
                printf("Pico1: UART sent '%c'\n", (uint8_t)c);
            }

            // listen CAN, wait Pico2 response
            can_frame frame;
            MCP2515::ERROR err = mcp2515.readMessage(&frame);
            if (err == MCP2515::ERROR_OK) {
                uint16_t adc_val = (frame.data[0] << 8) | frame.data[1];
                float voltage = adc_val * 3.3f / 4095.0f;

                if (frame.can_id >= 0x300 && frame.can_id < 0x310) {
                    int ch = frame.can_id - 0x300;
                    printf("Pico1: ADC1_CH%d = %d (%.2fV)\n", ch, adc_val, voltage);
                } else if (frame.can_id >= 0x310 && frame.can_id < 0x320) {
                    int ch = frame.can_id - 0x310;
                    printf("Pico1: ADC2_CH%d = %d (%.2fV)\n", ch, adc_val, voltage);
                }
            }

            sleep_ms(10);
        }
    } else {
        printf("Pico2: UART RX + CAN TX Mode\n");

        uart_init(UART_ID, BAUD_RATE);
        gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

        while (true) {
            if (uart_is_readable(UART_ID)) {
                uint8_t c = uart_getc(UART_ID);
                printf("Pico2: UART received '%c'\n", c);

                // two ADC channels
                for (int ch = 0; ch < NUM_KEYS_PER_ADC; ch++) {
                    uint16_t adc1 = mcp3208_read(CS_ADC1, ch);
                    uint16_t adc2 = mcp3208_read(CS_ADC2, ch);

                    printf("Pico2: ADC1_CH%d=%d | ADC2_CH%d=%d\n", ch, adc1, ch, adc2);

                    // ADC1 data, through CAN to Pico1
                    // CAN ID: 0x300 + channel
                    can_frame frame1;
                    frame1.can_id = 0x300 + ch;
                    frame1.can_dlc = 2;
                    frame1.data[0] = (adc1 >> 8) & 0xFF;  // high
                    frame1.data[1] = adc1 & 0xFF;          // low
                    mcp2515.sendMessage(&frame1);

                    // ADC2 data
                    // CAN ID: 0x310 + channel
                    can_frame frame2;
                    frame2.can_id = 0x310 + ch;
                    frame2.can_dlc = 2;
                    frame2.data[0] = (adc2 >> 8) & 0xFF;
                    frame2.data[1] = adc2 & 0xFF;
                    mcp2515.sendMessage(&frame2);
                }
            }

            sleep_ms(10);
        }
    }


// SEND AND RECEIVE CHAR, WITH uart
    // if (IS_RECEIVER) {
    //     printf("Pico1: UART TX + CAN RX Mode\n");

    //     // init UART (send)
    //     uart_init(UART_ID, BAUD_RATE);
    //     gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //     gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    //     while (true) {
    //         // USB serial input，through UART to Pico2
    //         int c = getchar_timeout_us(0);
    //         if (c != PICO_ERROR_TIMEOUT && c != '\n' && c != '\r') {
    //             uart_putc(UART_ID, (uint8_t)c);
    //             printf("Pico1: UART sent '%c'\n", (uint8_t)c);
    //         }

    //         // listen CAN, wait Pico2 response
    //         can_frame frame;
    //         MCP2515::ERROR err = mcp2515.readMessage(&frame);
    //         if (err == MCP2515::ERROR_OK) {
    //             printf("Pico1: CAN received | ID: 0x%x | Data: %c\n",
    //                 frame.can_id, frame.data[0]);
    //         } else if (err != MCP2515::ERROR_NOMSG) {
    //             printf("Pico1: CAN read error: %d\n", err);
    //         }

    //         sleep_ms(10);
    //     }
        
    // }else {
    //     printf("Pico2: UART RX + CAN TX Mode\n");

    //     // init UART (receive)
    //     uart_init(UART_ID, BAUD_RATE);
    //     gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //     gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    //     while (true) {
    //         // listen UART, wait Pico1 char
    //         if (uart_is_readable(UART_ID)) {
    //             uint8_t c = uart_getc(UART_ID);
    //             printf("Pico2: UART received '%c'\n", c);

    //             // through CAN send back to Pico1
    //             can_frame frame;
    //             frame.can_id = 0x200;
    //             frame.can_dlc = 1;
    //             frame.data[0] = c;
    //             MCP2515::ERROR err = mcp2515.sendMessage(&frame);
    //             printf("Pico2: CAN sent | err: %d\n", err);
    //         }

    //         sleep_ms(10);
    //     }
    // }








// SEND AND RECEIVE CHAR, WITHOUT uart
    // if (IS_RECEIVER) {

    //     // Receiver
    //     printf("Receiver Mode\n");

    //     can_frame frame;

    //     while (true) {

    //         MCP2515::ERROR err = mcp2515.readMessage(&frame);

    //         if (err == MCP2515::ERROR_OK) {
    //             printf("RX | ID: 0x%x | Data: ", frame.can_id);

    //             for (int i = 0; i < frame.can_dlc; i++) {
    //                 printf("%c ", frame.data[i]);
    //             }
    //             printf("\n");
    //         }
    //         else if (err != MCP2515::ERROR_NOMSG) {
    //             printf("CAN Read Error: %d\n", err);
    //         }

    //         sleep_ms(10);
    //     }

    // } else {

    //     // Sender
    //     printf("Sender Mode\n");

    //     can_frame frame;

    //     while (true) {

    //         // uart input
    //         usb_check();

    //         // send can
    //         if (send_enable) {

    //             frame.can_id = 0x100 + uart_char; 
    //             frame.can_dlc = 1;
    //             frame.data[0] = uart_char;

    //             MCP2515::ERROR err = mcp2515.sendMessage(&frame);

    //             printf("Sent CAN | ID: 0x%x | Data: %c | Err: %d\n",
    //                    frame.can_id, uart_char, err);

    //             send_enable = false;
    //         }

    //         sleep_ms(10);
    //     }
    // }



















// CONTINUOUSLY SEND AND RECEIVE 
    // // TODO: Remove if-statement when making master-specific program
    // if(IS_RECEIVER) {
    //     // Initializes the CAN ID for forwarding
    //     uart_setup(&can_id);
    // }
    // //  else {
    // //     // This is only temporary to allow flashing of same program to master/slave 
    // //      while(true) {
    // //         uart_putc(UART_ID, 7 + 48);
    // //         // printf("Sending: %c\n", 7 + 48);
    // //         sleep_ms(500);
    // //     }
    // // }
    // sleep_ms(1000);

    // if(IS_RECEIVER) {
    //     printf("Receiver Mode\n");

    //     can_frame frame;

    //     while (true) {
    //         MCP2515::ERROR err = mcp2515.readMessage(&frame);

    //         if (err == MCP2515::ERROR_OK) {
    //             printf("CAN ID: 0x%x | Data: ", frame.can_id);

    //             for (int i = 0; i < frame.can_dlc; i++) {
    //                 printf("%c ", frame.data[i]);
    //             }
    //             printf("\n");
    //         }
    //         else if (err != MCP2515::ERROR_NOMSG) {
    //             printf("CAN Read Error: %d\n", err);
    //         }

    //         sleep_ms(10);
    //     }

    // } else {
    //     printf("Sender Mode\n");

    //     // test 
    //     can_frame frame{0x100, 8, "Hello!!"};

    //     while (true) {

    //         // UART test
    //         uart_putc(UART_ID, 'A');

    //         MCP2515::ERROR err = mcp2515.sendMessage(&frame);

    //         printf("Sent CAN | Error: %d\n", err);

    //         sleep_ms(1000);
    //     }
    // }

}








