#include <stdio.h>
#include "pico/stdlib.h"
#include "mcp2515/mcp2515.h"

// Required for ADC/CAN 
#include "hardware/spi.h"

// ADC Definitions
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define NUM_KEYS 8

#define ADC_MODE 1

uint16_t mcp3208_read(uint8_t ch) {
    uint8_t tx[3];
    uint8_t rx[3];

    tx[0] = 0x06 | ((ch & 0x04) >> 2);   // start + single
    tx[1] = (ch & 0x03) << 6;
    tx[2] = 0x00;

    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(PIN_CS, 1);

    uint16_t value = ((rx[1] & 0x0F) << 8) | rx[2];
    return value;
}

int main()
{

    stdio_init_all();

    MCP2515 mcp2515(spi0, 17, 19, 16, 18);
    
    // ADC Initialization

    mcp2515.reset();

    mcp2515.setBitrate(CAN_5KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

#ifdef ADC_MODE
    // can ID, length of data, data
    // max data length of 8 bytes
    can_frame frame{0x100, 8, "Hello!!"};    

    sleep_ms(10000);
    while (true) {
        
        MCP2515::ERROR error = mcp2515.sendMessage(&frame);
        
        printf("Sent message. Error: %d\n", error);
        sleep_ms(1000);
    }
#else
    sleep_ms(5000);
    printf("Receiver initialized!");

    can_frame frame;
    while (true) {
        MCP2515::ERROR error = mcp2515.readMessage(&frame);
        if (error == MCP2515::ERROR_OK) {
            printf("CAN id: 0x%x\n", frame.can_id);
            for (int i = 0; i < frame.can_dlc; i++) {
                printf("%c ", frame.data[i]);
            }
            printf("\n");
        }
        else {
            if (error != MCP2515::ERROR_NOMSG) {
                printf("Error recieving message: %d\n", error);
            }
        }
        sleep_ms(1);
    }
#endif
}
