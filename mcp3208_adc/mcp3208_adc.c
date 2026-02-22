#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

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

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    printf("MCP3208 Reader Start\n");

    spi_init(SPI_PORT, 500 * 1000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    while (true) {
        uint16_t adc = mcp3208_read(0);

        float voltage = adc * 3.3f / 4095.0f;

        printf("ADC=%d  V=%.3f\n", adc, voltage);

        sleep_ms(200);
    }
}