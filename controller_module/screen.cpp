// Library used: https://github.com/tvlad1234/pico-displayDrivs
// Fonts: https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts
#include "pico/stdlib.h"

extern "C" {
#include "ili9341.h"
#include "gfx.h"
#include "FreeSans24pt7b.h"
#include "rizzi.h"
#include "dice.h"
#include "rizzi_color.h"
}

volatile int c = 50;

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

// Draws a bitmap (1 color) image. Data is from exporting image as .xbm in GIMP, then renaming to .h
// From: https://github.com/adafruit/Adafruit-GFX-Library/blob/7ab6d09178a75a65b72d26dcbba3ec2f11882366/Adafruit_GFX.cpp#L967
void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t b = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        b >>= 1;
      else
        b = *(const unsigned char *)(&bitmap[j * byteWidth + i / 8]);

      if (b & 0x01)
        GFX_drawPixel(x + i, y, color);
    }
  }
}

// Draws a color image. Use convert.py tool to get data
void draw_image(int16_t x, int16_t y, const uint16_t data[], int16_t w, int16_t h) {
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      GFX_drawPixel(x + i, y, data[j*w + i]);
    }
  }
}

// Draws a color image, with 1-bit transparency. Use convert.py to get data.
void draw_image_transparent(int16_t x, int16_t y, const uint16_t data[], const uint8_t alpha_mask[], int16_t w, int16_t h) {
    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            if ((alpha_mask[(j*w + i) / 8] >> (7 - (j*w + i) % 8)) & 1) {
                GFX_drawPixel(x + i, y, data[j*w + i]);
        
            }
        }
    }
}

void screen_run()
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

        draw_image(10, 100, rizzi_color_data, RIZZI_COLOR_WIDTH, RIZZI_COLOR_HEIGHT);

        draw_image_transparent(10, 50, dice_data, dice_alpha_mask, DICE_WIDTH, DICE_HEIGHT);

        GFX_flush();
    }
}