// Library used: https://github.com/tvlad1234/pico-displayDrivs
// Fonts: https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts
#include <string.h>

#include "ili9341.h"
#include "gfx.h"

#include "FreeMono9pt7b.h"
#include "FreeMono12pt7b.h"
#include "FreeMonoBold12pt7b.h"
#include "FreeMono18pt7b.h"
#include "FreeMonoBold18pt7b.h"
#include "FreeMono24pt7b.h"
#include "FreeMonoBold24pt7b.h"
#include "rizzi.h"
#include "dice.h"
#include "rizzi_color.h"

#include "ui.h"

#define LCD_DC 20
#define LCD_CS 17
#define LCD_RST 21
#define LCD_SCK 18
#define LCD_TX 19

#define rgb(r, g, b) (uint16_t) ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
#define COLOR_HIGHLIGHT rgb(255, 106, 233)
#define COLOR_BG rgb(16, 15, 15)
#define COLOR_GREEN rgb(0, 255, 17)
#define COLOR_UI_2 rgb(52, 51, 49)
#define COLOR_TX rgb(255, 255, 255)
#define COLOR_TX_2 rgb(135, 133, 128)

#define BUTTON_HEIGHT 30
#define CHARS_PER_ROW 15
#define ROWS_PER_SCREEN 8

#define VOLUME_STEP 5

#define MAX_OCTAVE 5

#define EMPTY_MSG "(empty)"


void (*active_screen)(int);

int cursor = 0;

int volume = 75;
SampleList samples;
int octave = 0;

void (*volume_changed_callback)(int) = NULL;
SampleList (*get_sample_list_callback)() = NULL;
void (*sample_changed_callback)(int) = NULL;
void (*octave_changed_callback)(int) = NULL;



void main_screen(int);
void sample_screen(int);
void volume_screen(int);
void transpose_screen(int);

// Draws a bitmap (1 color) image. Data is from exporting image as .xbm in GIMP, then renaming to .h
// From: https://github.com/adafruit/Adafruit-GFX-Library/blob/7ab6d09178a75a65b72d26dcbba3ec2f11882366/Adafruit_GFX.cpp#L967
void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
{

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t b = 0;

    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
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
void draw_image(int16_t x, int16_t y, const uint16_t data[], int16_t w, int16_t h)
{
    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            GFX_drawPixel(x + i, y, data[j * w + i]);
        }
    }
}

// Draws a color image, with 1-bit transparency. Use convert.py to get data.
void draw_image_transparent(int16_t x, int16_t y, const uint16_t data[], const uint8_t alpha_mask[], int16_t w, int16_t h)
{
    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            if ((alpha_mask[(j * w + i) / 8] >> (7 - (j * w + i) % 8)) & 1)
            {
                GFX_drawPixel(x + i, y, data[j * w + i]);
            }
        }
    }
}

void draw_vbar(int x, int y, int w, int h, int percentage, uint16_t color1, uint16_t color2) {
    GFX_fillRect(x, y, w, h, color1);
    int l = h * (percentage / 100.0);
    if (percentage != 0)
        GFX_fillRect(x, y + h - l, w, l, color2);
}

void draw_menu(int x, int y, int input, char* labels[], void (*callback)(int), int size, int highlight)
{
    switch (input) {
        case INPUT_LEFT:
            cursor--;
            break;
        case INPUT_RIGHT:
            cursor++;
            break;
    }
    if (cursor < 0) {
        cursor += size;
    }
    cursor %= size;

    char* empty[] = {EMPTY_MSG};
    if (size == 0) {
        size = 1;
        labels = empty;
    }

    int pages;
    int current_page;
    if (size <= ROWS_PER_SCREEN) {
        pages = 1;
        current_page = 0;
    }
    else {
        pages = (size - 1) / (ROWS_PER_SCREEN - 1) + 1;
        current_page = cursor / (ROWS_PER_SCREEN - 1);
    }

    int start = current_page * (ROWS_PER_SCREEN - 1);
    int stop = pages == 1 ? size : MIN(size, (current_page + 1) * (ROWS_PER_SCREEN - 1));

    int cursor_y = y;
    for (int i = start; i < stop; i++) {
        GFX_setCursor(x, cursor_y);
        cursor_y += BUTTON_HEIGHT;

        if (i == highlight) {
            GFX_setTextColor(COLOR_HIGHLIGHT);
        }
        else {
            GFX_setTextColor(COLOR_TX);
        }

        if (cursor == i) {
            if (input == INPUT_SELECT) {
                if (callback != NULL) {
                    callback(cursor);
                    return;
                }
            }
            GFX_setFont(&FreeMonoBold12pt7b);
            if (strlen(labels[i]) <= CHARS_PER_ROW) {
                GFX_printf(">%s", labels[i]);    
            }
            else {
                GFX_printf(">%.*s...", CHARS_PER_ROW - 3, labels[i]);
            }
        }
        else {
            GFX_setFont(&FreeMono12pt7b);
            if (strlen(labels[i]) <= CHARS_PER_ROW) {
                GFX_printf("%s", labels[i]);    
            }
            else {
                GFX_printf("%.*s...", CHARS_PER_ROW - 3, labels[i]);
            }
        }
    }
    if (pages != 1) {
        GFX_setCursor(x, y + BUTTON_HEIGHT * (ROWS_PER_SCREEN - 1));
        GFX_setFont(&FreeMono12pt7b);
        GFX_setTextColor(COLOR_TX_2);
        GFX_printf("(%d/%d)", current_page + 1, pages);
    }
}

void set_screen(void (*screen)()) {
    active_screen = screen;
    ui_update(INPUT_NONE);
}

void main_menu_callback(int selection)
{
    switch (selection) {
        case 0:
            set_screen(&volume_screen);
            break;
        case 1:
            if (get_sample_list_callback != NULL) {
                samples = get_sample_list_callback();
            }
            else {
                samples.length = 0;
                samples.selected = -1;
                samples.names = NULL;
            }
            cursor = MAX(0, samples.selected);
            set_screen(&sample_screen);
            break;
        case 2:
            set_screen(&transpose_screen);
            break;
    }
}

void sample_menu_callback(int selection)
{
    if (selection == samples.selected || samples.length == 0) {
        cursor = 1;
        set_screen(&main_screen);
    }
    else {
        if (sample_changed_callback != NULL) {
            sample_changed_callback(selection);
        }
        samples.selected = selection;
        ui_update(INPUT_NONE);
    }
}

void main_screen(int input)
{
    GFX_fillScreen(COLOR_BG);

    GFX_setFont(&FreeMonoBold18pt7b);
    GFX_setTextColor(COLOR_TX);
    GFX_setCursor(14, 40);
    GFX_printf("MicroPiano");
    
    char* labels[] = {
        "Volume",
        "Sample Select",
        "Octave"
    };

    draw_menu(4, 100, input, labels, &main_menu_callback, 3, -1);
}

void sample_screen(int input)
{
    GFX_fillScreen(COLOR_BG);

    GFX_setFont(&FreeMonoBold18pt7b);
    GFX_setTextColor(COLOR_TX);
    GFX_setCursor(44, 40);
    GFX_printf("Samples");

    draw_menu(4, 100, input, samples.names, &sample_menu_callback, samples.length, samples.selected);
}

void volume_screen(int input)
{
    if (input == INPUT_SELECT) {
        set_screen(&main_screen);
        return;
    }

    if (input == INPUT_LEFT) {
        volume -= VOLUME_STEP;
        volume = MAX(volume, 0);
        if (volume_changed_callback != NULL) {
            volume_changed_callback(volume);
        }
    }
    if (input == INPUT_RIGHT) {
        volume += VOLUME_STEP;
        volume = MIN(volume, 100);
        if (volume_changed_callback != NULL) {
            volume_changed_callback(volume);
        }
    }

    GFX_setFont(&FreeMonoBold18pt7b);
    GFX_setTextColor(COLOR_TX);
    GFX_setCursor(54, 40);
    GFX_printf("Volume");

    GFX_fillRect(79, 63, 72, 184, ILI9341_WHITE);
    draw_vbar(79+4, 63+4, 72-8, 184-8, volume, COLOR_UI_2, COLOR_GREEN);

    GFX_setFont(&FreeMonoBold24pt7b);
    GFX_setCursor(72, 300);
    GFX_printf("%d%%", volume);
}

void transpose_screen(int input)
{
    if (input == INPUT_SELECT) {
        set_screen(&main_screen);
        return;
    }

    draw_image(10, 100, rizzi_color_data, RIZZI_COLOR_WIDTH, RIZZI_COLOR_HEIGHT);

    GFX_setFont(&FreeMonoBold18pt7b);
    GFX_setTextColor(COLOR_TX);
    GFX_setCursor(54, 40);
    GFX_printf("Octave");

    if (input == INPUT_LEFT) {
        octave--;
        octave = MAX(octave, -MAX_OCTAVE);
        if (octave_changed_callback != NULL) {
            octave_changed_callback(volume);
        }
    }
    if (input == INPUT_RIGHT) {
        octave++;
        octave = MIN(octave, MAX_OCTAVE);
        if (octave_changed_callback != NULL) {
            octave_changed_callback(octave);
        }
    }
    GFX_setFont(&FreeMonoBold24pt7b);
    GFX_setCursor(72, 250);
    if (octave >= 0) {
        GFX_printf("+%d", octave);
    }
    else {
        GFX_printf("%d", octave);
    }
}

// Call to initialize the UI and LCD
void ui_init()
{
    LCD_setPins(LCD_DC, LCD_CS, LCD_RST, LCD_SCK, LCD_TX);

    active_screen = &main_screen;

    LCD_initDisplay();
    LCD_setRotation(2);
    GFX_createFramebuf();
}

// Called to update the UI every time an input from the user is recieved (INPUT_LEFT, INPUT_RIGHT, INPUT_SELECT, or INPUT_NONE)
void ui_update(int input)
{
    GFX_clearScreen();
    active_screen(input);
    GFX_flush();
}

// Sets a function that the UI will use to get the SampleList
// Includes length of sample list, currently selected sample, and a list of sample names 
void ui_set_get_sample_list_callback(SampleList (*callback)())
{
    get_sample_list_callback = callback;
}

// Set a function that the UI will call any time the user changes the volume
void ui_set_volume_changed_callback(void (*callback)(int))
{
    volume_changed_callback = callback;
}

// Set a function that the UI will call any time the user changes the selected sample
// Callback is given the numerical index of the new sample
void ui_set_sample_changed_callback(void (*callback)(int))
{
    sample_changed_callback = callback;
}

// Set a function that the UI will call any time the user changes the octave
void ui_set_octave_changed_callback(void (*callback)(int))
{
    octave_changed_callback = callback;
}
