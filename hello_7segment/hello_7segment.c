/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/critical_section.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// Pins should be connected to high for the segments to light up
// Connections:
//         GPIO17 -> P1 (Digit 0)
//         GPIO18 -> P2 (Digit 1)
//         GPIO19 -> P4 (Colon - skip)
//         GPIO20 -> P6 (Digit 2)
//         GPIO21 -> P8 (Digit 3)

// Ground Pin Connections (segments - LOW to light up)
// GPIO -> Module Pin -> Segment
//         GPIO10 -> P14 -> A (top)
//         GPIO12 -> P16 -> B (top right)
//         GPIO9  -> P13 -> C (bottom right)
//         GPIO15 -> P3  -> D (bottom)
//         GPIO14 -> P5  -> E (bottom left)
//         GPIO7  -> P11 -> F (top left)
//         GPIO11 -> P15 -> G (middle)
//         GPIO13 -> P7  -> DP (decimal point)
//         GPIO8  -> P12 -> Colon

const int select_pins[] = {17, 18, 19, 20, 21};

// Segment control pins in order: A, B, C, D, E, F, G, DP, Colon
const int segment_pins[] = {10, 12, 9, 15, 14, 7, 11, 13, 8};

// Segment patterns:   {A, B, C, D, E, F, G, ., :}
const int LETTER_L[] = {0, 0, 0, 1, 1, 1, 0, 0, 0};
const int LETTER_O[] = {1, 1, 1, 1, 1, 1, 0, 0, 0};
const int LETTER_A[] = {1, 1, 1, 0, 1, 1, 1, 0, 0};
const int LETTER_D[] = {0, 1, 1, 1, 1, 0, 1, 0, 0};
const int LETTER_I[] = {0, 1, 1, 0, 0, 0, 0, 0, 0};
const int LETTER_NONE[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
const int LETTER_ALL[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
const int NUMBER_ZERO[] = {1, 1, 1, 1, 1, 1, 0, 0, 0};
const int NUMBER_ONE[] = {0, 1, 1, 0, 0, 0, 0, 0, 0};
const int NUMBER_TWO[] = {1, 1, 0, 1, 1, 0, 1, 0, 0};
const int NUMBER_THREE[] = {1, 1, 1, 1, 0, 0, 1, 0, 0};
const int NUMBER_FOUR[] = {0, 1, 1, 0, 0, 1, 1, 0, 0};
const int NUMBER_FIVE[] = {1, 0, 1, 1, 0, 1, 1, 0, 0};
const int NUMBER_SIX[] = {1, 0, 1, 1, 1, 1, 1, 0, 0};
const int NUMBER_SEVEN[] = {1, 1, 1, 0, 0, 0, 0, 0, 0};
const int NUMBER_EIGHT[] = {1, 1, 1, 1, 1, 1, 1, 0, 0};
const int NUMBER_NINE[] = {1, 1, 1, 1, 0, 1, 1, 0, 0};

// Array to hold the digit patterns for voltage display
const int* message[] = {NUMBER_ZERO, NUMBER_ZERO, NUMBER_ZERO, NUMBER_ZERO, LETTER_NONE};
// Decimal point flags per digit; only enabled during voltage display
bool decimal_flags[] = {false, false, false, false, false};

// Critical section for thread-safe access to message array
critical_section_t message_lock;

// Lookup table for digit patterns
const int* digit_patterns[] = {
    NUMBER_ZERO, NUMBER_ONE, NUMBER_TWO, NUMBER_THREE, NUMBER_FOUR,
    NUMBER_FIVE, NUMBER_SIX, NUMBER_SEVEN, NUMBER_EIGHT, NUMBER_NINE
};

void clear_all() {
    // Turn off all digit selects
    for(int i = 0; i < 5; i++) {
        gpio_put(select_pins[i], 0);
    }
    // Turn off all segments (set HIGH since LOW lights them up)
    for(int i = 0; i < 9; i++) {
        gpio_put(segment_pins[i], 1);
    }
}

void display_char(int digit_pos, const int* pattern, bool show_decimal) {
    clear_all();
    
    // Enable the digit position
    gpio_put(select_pins[digit_pos], 1);
    
    // Set the segment pattern (LOW to light up)
    for(int i = 0; i < 9; i++) {
        if(i == 7 && show_decimal) {
            // Force decimal point on if requested
            gpio_put(segment_pins[i], 0);
        } else {
            gpio_put(segment_pins[i], !pattern[i]);
        }
    }
}

void core1_entry() {
    // Display format during voltage mode: X.XXX (digit 0 with decimal, then 3 decimal digits)
    // Position mapping: [0]=ones digit with DP, [1]=tenths, [2]=skip(colon), [3]=hundredths, [4]=thousandths
    const int digit_positions[] = {0, 1, 2, 3, 4};

    while(true) {
        // Cycle through each character with persistence of vision
        for(int i = 0; i < 5; i++) {
            critical_section_enter_blocking(&message_lock);
            const int* current_pattern = message[i];
            bool show_decimal = decimal_flags[i];
            critical_section_exit(&message_lock);
            
            display_char(digit_positions[i], current_pattern, show_decimal);
            sleep_ms(4); // Fast multiplexing for steady display
        }
    }
}

void update_voltage_display(float voltage) {
    // Convert voltage to format X.XXX
    // For example: 3.275V -> display "3.275"
    
    // Clamp voltage to 0-9.999 range
    if(voltage < 0.0f) voltage = 0.0f;
    if(voltage > 9.999f) voltage = 9.999f;
    
    // Extract digits
    int ones = (int)voltage;
    int tenths = (int)((voltage - ones) * 10) % 10;
    int hundredths = (int)((voltage - ones) * 100) % 10;
    int thousandths = (int)((voltage - ones) * 1000) % 10;
    
    // Update message array with thread safety (and decimal flags for voltage mode)
    critical_section_enter_blocking(&message_lock);
    message[0] = digit_patterns[ones];
    message[1] = digit_patterns[tenths];
    message[2] = LETTER_NONE; // Skip colon position
    message[3] = digit_patterns[hundredths];
    message[4] = digit_patterns[thousandths];
    decimal_flags[0] = true;
    decimal_flags[1] = false;
    decimal_flags[2] = false;
    decimal_flags[3] = false;
    decimal_flags[4] = false;
    critical_section_exit(&message_lock);
}

int main() {
    stdio_init_all();
    
    printf("ADC Voltage Display on 7-Segment\n");

    // Initialize critical section
    critical_section_init(&message_lock);

    // Init select pins
    for(int i = 0; i < 5; i++) {
        gpio_init(select_pins[i]);
        gpio_set_dir(select_pins[i], GPIO_OUT);
        gpio_put(select_pins[i], 0);
    }

    // Init segment pins
    for(int i = 0; i < 9; i++) {
        gpio_init(segment_pins[i]);
        gpio_set_dir(segment_pins[i], GPIO_OUT);
        gpio_put(segment_pins[i], 1); // HIGH = off
    }

    // Initialize ADC
    adc_init();
    adc_gpio_init(26); // ADC0 on GPIO26
    adc_select_input(0);

    // Power LED setup, GP 28
    const int power_led_pin = 28;
    gpio_init(power_led_pin);
    gpio_set_dir(power_led_pin, GPIO_OUT);

    // Trigger output to external Pico: GP27 (idle HIGH, 50ms LOW pulse)
    const int trigger_pin = 27;
    gpio_init(trigger_pin);
    gpio_set_dir(trigger_pin, GPIO_OUT);
    gpio_put(trigger_pin, 1);

    // UART Setup
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX

    // Launch display logic on core 1
    multicore_launch_core1(core1_entry);
    
    // Show LOAD at startup (no decimal points)
    critical_section_enter_blocking(&message_lock);
    message[0] = LETTER_L;
    message[1] = LETTER_O;
    message[2] = LETTER_NONE;
    message[3] = LETTER_A;
    message[4] = LETTER_D;
    decimal_flags[0] = false;
    decimal_flags[1] = false;
    decimal_flags[2] = false;
    decimal_flags[3] = false;
    decimal_flags[4] = false;
    critical_section_exit(&message_lock);
    
    // Indicate power on
    gpio_put(power_led_pin, 1);
    sleep_ms(2000);
    gpio_put(power_led_pin, 0);

    // Wait for UART data to get ID
    char msg[1];
    uart_read_blocking(uart0, (uint8_t*)&msg, 1);
    while(msg[0] < '0' || msg[0] > '9'){
        uart_read_blocking(uart0, (uint8_t*)&msg, 1);
    }
    
    // Update message to show ID (no decimal points)
    critical_section_enter_blocking(&message_lock);
    message[0] = LETTER_I;
    message[1] = LETTER_D;
    message[2] = LETTER_ALL;
    message[3] = LETTER_NONE;

    // Convert received char to number pattern
    int id_digit = msg[0] - '0';
    message[4] = digit_patterns[id_digit];
    decimal_flags[0] = false;
    decimal_flags[1] = false;
    decimal_flags[2] = false;
    decimal_flags[3] = false;
    decimal_flags[4] = false;
    critical_section_exit(&message_lock);
    
    // Transmit new ID to next device (increment by 1)
    // Only has support for 0-9
    char next_id = (msg[0] - '0' + 1) % 10 + '0';
    for(int iterations = 0; iterations < 50; iterations++){
        uart_putc(uart0, next_id);
        sleep_ms(100);
    }

    // Now switch to voltage display mode
    // Conversion factor for 3.3V reference with 12-bit ADC
    // If you're using a 5V reference, change this to 5.0f
    const float conversion_factor = 5.0f / (1 << 12);
    const float trigger_threshold_v = 1.5f;
    const float reset_threshold_v = 1.4f; // hysteresis to avoid retriggering on noise
    bool above_threshold = false;
    
    // Main loop - read ADC and update display
    while(true) {
        uint16_t result = adc_read();
        float voltage = result * conversion_factor;
        
        // Update the display
        update_voltage_display(voltage);
        
        // Pulse GP27 LOW for 50ms when crossing above threshold, then latch until reset below hysteresis
        if (!above_threshold && voltage > trigger_threshold_v) {
            gpio_put(trigger_pin, 0);
            sleep_ms(50);
            gpio_put(trigger_pin, 1);
            above_threshold = true;
        } else if (above_threshold && voltage < reset_threshold_v) {
            above_threshold = false;
        }
                
        // Update every 100ms for smooth reading
        sleep_ms(100);
    }
}