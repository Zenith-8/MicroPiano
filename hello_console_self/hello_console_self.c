#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
// SD/FatFs SPI
#if __has_include("ff.h") && __has_include("sd_card.h") && __has_include("hw_config.h")
#include "ff.h"
#include "f_util.h"
#include "sd_card.h"
#include "hw_config.h" // declares sd_get_by_num/sd_get_num
#endif

// LCD 1602 I2C helpers
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

const int LCD_BACKLIGHT = 0x08;
const int LCD_ENABLE_BIT = 0x04;

static int lcd_addr = 0x27; // typical PCF8574 address

#define LCD_CHARACTER  1
#define LCD_COMMAND    0

/* Quick helper function for single byte transfers */
static inline void i2c_write_byte(uint8_t val) {
    i2c_write_blocking(i2c1, lcd_addr, &val, 1, false);
}

static inline void lcd_toggle_enable(uint8_t val) {
#define LCD_DELAY_US 600
    sleep_us(LCD_DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(LCD_DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(LCD_DELAY_US);
}

static inline void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low  = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

static inline void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

static inline void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static inline void lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

static inline void lcd_string(const char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

static inline void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);
    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}

// ============================================================

volatile bool button_pressed = false;
volatile uint32_t last_press_time = 0;
const uint32_t DEBOUNCE_MS = 200;
#define BUTTON_GPIO 16
static bool button_latched = false;

void button_callback(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Debounce: ignore if too soon after last press
    if (now - last_press_time > DEBOUNCE_MS) {
        button_pressed = true;
        last_press_time = now;
    }
}

#if __has_include("ff.h") && __has_include("sd_card.h") && __has_include("hw_config.h")
static bool sd_self_test(char* line0, size_t line0_len, char* line1, size_t line1_len) {
    FATFS fs;
    FIL fil;
    FRESULT fr;
    UINT io_count;
    const char* test_str = "Pico SD OK!";
    char read_buf[64];

    // Init driver (provided by FatFs SPI lib)
    if (!sd_init_driver()) {
        snprintf(line0, line0_len, "%s", "SD INIT ERR");
        snprintf(line1, line1_len, "%s", "driver fail");
        return false;
    }

    sd_card_t* sd = sd_get_by_num(0);
    if (!sd) {
        snprintf(line0, line0_len, "%s", "SD NO SLOT");
        snprintf(line1, line1_len, "%s", "num=0 missing");
        return false;
    }

    fr = f_mount(&fs, sd->pcName, 1);
    if (fr != FR_OK) {
        snprintf(line0, line0_len, "%s", "SD MOUNT ERR");
        snprintf(line1, line1_len, "F=%d", (int)fr);
        return false;
    }

    fr = f_open(&fil, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        snprintf(line0, line0_len, "%s", "SD OPEN W ERR");
        snprintf(line1, line1_len, "F=%d", (int)fr);
        f_mount(NULL, sd->pcName, 0);
        return false;
    }

    fr = f_write(&fil, test_str, (UINT)strlen(test_str), &io_count);
    f_close(&fil);
    if (fr != FR_OK || io_count != strlen(test_str)) {
        snprintf(line0, line0_len, "%s", "SD WRITE ERR");
        snprintf(line1, line1_len, "F=%d W=%u", (int)fr, (unsigned)io_count);
        f_mount(NULL, sd->pcName, 0);
        return false;
    }

    fr = f_open(&fil, "test.txt", FA_READ);
    if (fr != FR_OK) {
        snprintf(line0, line0_len, "%s", "SD OPEN R ERR");
        snprintf(line1, line1_len, "F=%d", (int)fr);
        f_mount(NULL, sd->pcName, 0);
        return false;
    }

    memset(read_buf, 0, sizeof(read_buf));
    fr = f_read(&fil, read_buf, sizeof(read_buf) - 1, &io_count);
    f_close(&fil);
    f_mount(NULL, sd->pcName, 0);
    if (fr != FR_OK) {
        snprintf(line0, line0_len, "%s", "SD READ ERR");
        snprintf(line1, line1_len, "F=%d", (int)fr);
        return false;
    }

    bool ok = (strncmp(read_buf, test_str, strlen(test_str)) == 0);
    if (ok) {
        snprintf(line0, line0_len, "%s", "SD OK");
        snprintf(line1, line1_len, "%.16s", read_buf);
    } else {
        snprintf(line0, line0_len, "%s", "SD MISMATCH");
        snprintf(line1, line1_len, "got: %.16s", read_buf);
    }
    return ok;
}
#endif

int main() {
    stdio_init_all();

    // 🔹 Use I2C1 on GPIO6 (SDA) and GPIO7 (SCL)
    const uint SDA_PIN = 6;
    const uint SCL_PIN = 7;

    i2c_init(i2c1, 100 * 1000); // 100kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    bi_decl(bi_2pins_with_func(SDA_PIN, SCL_PIN, GPIO_FUNC_I2C));

    lcd_init();

    // Button: use internal pull-up; press to GND triggers falling edge
    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);
    gpio_set_irq_enabled_with_callback(BUTTON_GPIO, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // SD self-test at startup: write, read-back, display result
#if __has_include("ff.h") && __has_include("sd_card.h")
    char l0[17] = {0};
    char l1[17] = {0};
    sd_self_test(l0, sizeof(l0), l1, sizeof(l1));
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_string(l0);
    lcd_set_cursor(1, 0);
    lcd_string(l1);
    sleep_ms(2000);
    lcd_clear();
#else
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_string("SD LIB MISSING");
    lcd_set_cursor(1, 0);
    lcd_string("See CMake libs");
    sleep_ms(1500);
    lcd_clear();
#endif

    // UART initialization remains the same
    const uint SERIAL_BAUD = 115200;
    uart_init(uart0, SERIAL_BAUD);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
    
    const char* send_string = "1";

    while (true) {
        printf("Sending: %s\n", send_string);
        // Transmit over UART
        for (const char *p = send_string; *p; ++p) {
            uart_putc_raw(uart0, *p);
        }

	// Mirror to LCD
	lcd_set_cursor(0, 0);
	lcd_string("Console:");
	lcd_set_cursor(1, 0);
	if (button_latched) {
		lcd_string("Button Pressed   ");
	} else {
		lcd_string(send_string);
	}

	// React to button press (latch state)
	if (button_pressed) {
		button_pressed = false;
		button_latched = true;
    	}

        sleep_ms(50);
    }
}
