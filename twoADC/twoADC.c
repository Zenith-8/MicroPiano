#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// two ADC‘s CS
#define CS_ADC1 17
#define CS_ADC2 20

#define NUM_KEYS_PER_ADC 8

// MCP3208 read cspin and channel
uint16_t mcp3208_read(uint cs_pin, uint8_t channel) {
    uint8_t tx[3];
    uint8_t rx[3];

    tx[0] = 0x06 | ((channel & 0x04) >> 2);
    tx[1] = (channel & 0x03) << 6;
    tx[2] = 0x00;

    gpio_put(cs_pin, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(cs_pin, 1);

    uint16_t result = ((rx[1] & 0x0F) << 8) | rx[2];
    return result;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("Dual MCP3208 Start\n");

    spi_init(SPI_PORT, 500 * 1000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // initialize two MCP3208's CS
    gpio_init(CS_ADC1);
    gpio_set_dir(CS_ADC1, GPIO_OUT);
    gpio_put(CS_ADC1, 1);

    gpio_init(CS_ADC2);
    gpio_set_dir(CS_ADC2, GPIO_OUT);
    gpio_put(CS_ADC2, 1);

    while (true) {

        // ADC 1 (CH0~7)
        for (int ch = 0; ch < NUM_KEYS_PER_ADC; ch++) {
            uint16_t adc = mcp3208_read(CS_ADC1, ch);
            float voltage = adc * 3.3f / 4095.0f;

            printf("A1_CH%d: %4d (%.2fV)  ", ch, adc, voltage);
        }

        printf("\n");

        // ADC 2 (CH0~7)
        for (int ch = 0; ch < NUM_KEYS_PER_ADC; ch++) {
            uint16_t adc = mcp3208_read(CS_ADC2, ch);
            float voltage = adc * 3.3f / 4095.0f;

            printf("A2_CH%d: %4d (%.2fV)  ", ch, adc, voltage);
        }

        printf("\n----------------------\n");

        sleep_ms(200);
    }
}