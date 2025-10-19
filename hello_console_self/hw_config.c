// Hardware config for no-OS-FatFS-SD-SPI-RPi-Pico on SPI0, GPIO2..5
#include "hw_config.h"
#include "my_debug.h"

// SPI interfaces
static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = 4, // GP4
        .mosi_gpio = 3, // GP3
        .sck_gpio = 2,  // GP2
        .baud_rate = 12500 * 1000
    }
};

// SD card interfaces
static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",
        .spi = &spis[0],
        .ss_gpio = 5, // GP5
        .use_card_detect = false,
        .card_detect_gpio = -1,
        .card_detected_true = -1
    }
};

size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num < spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}