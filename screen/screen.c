// Library used: https://github.com/tvlad1234/pico-displayDrivs
// Fonts: https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts
#include "pico/stdlib.h"

#include "ili9341.h"
#include "gfx.h"
#include "FreeSans24pt7b.h"
#include "rizzi.h"

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

int c = 50;

void gpio_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (gpio == 15) c++;
        if (gpio == 16) c--;
    }
}

void draw_volume(int x, int y, int w, int h, int vol) {
    GFX_drawRect(x, y, w, h, ILI9341_WHITE);
    GFX_fillRect(x, y, (int) w * (vol / 100.0), h, ILI9341_WHITE);
}

void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t b = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        b >>= 1;
      else
        b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      // Nearly identical to drawBitmap(), only the bit order
      // is reversed here (left-to-right = LSB to MSB):
      if (b & 0x01)
        GFX_drawPixel(x + i, y, color);
    }
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


    int t = 0;
    while (true)
    {
        t++;
        t = t % 30;
        float s = t / 30.0;   

        GFX_clearScreen();
        GFX_setCursor(0, 48);
        GFX_printf("Hello GFX!\n%d", c);
        
        // Colors in RGB 5/6/5
        GFX_drawCircle(120, 290 - (s - s*s) * 120 * 4, 30, ILI9341_GREEN);
        
        draw_volume(10, 250, 220, 40, c);

        drawXBitmap(10, 50, rizzi_bits, 100, 100, ILI9341_WHITE);

        GFX_flush();
    }
}