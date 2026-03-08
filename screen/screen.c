// Library used: https://github.com/tvlad1234/pico-displayDrivs
// Fonts: https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts
#include "pico/stdlib.h"

#include "ili9341.h"
#include "gfx.h"
#include "FreeSans24pt7b.h"

int c;

void gpio_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (gpio == 15) c++;
        if (gpio == 16) c--;
    }
}

int main()
{   
    gpio_init(15);
    gpio_init(16);
    gpio_set_irq_enabled_with_callback(15, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(16, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    LCD_setPins(20, 17, 21, 18, 19);

    LCD_initDisplay();
    LCD_setRotation(2);
    GFX_createFramebuf();

    GFX_setFont(&FreeSans24pt7b);

    while (true)
    {
        GFX_clearScreen();
        GFX_setCursor(0, 48);
        GFX_printf("Hello GFX!\n%d", c);
        GFX_flush();
        sleep_ms(16);
    }
}