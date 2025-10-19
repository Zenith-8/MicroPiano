/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pwm.pio.h"

#ifndef TONE_PIN
#define TONE_PIN 1
#endif

static inline uint32_t tone_half_period_from_hz(float frequency_hz) {
    if (frequency_hz < 1.0f) frequency_hz = 1.0f;
    uint32_t sys_hz = clock_get_hz(clk_sys);
    double half_period = (double)sys_hz / (2.0 * (double)frequency_hz);
    if (half_period < 1.0) half_period = 1.0;
    if (half_period > 4294967295.0) half_period = 4294967295.0; // clamp to uint32_t max
    return (uint32_t)(half_period);
}

static inline void tone_set_frequency(PIO pio, uint sm, float frequency_hz) {
    uint32_t half_period_cycles = tone_half_period_from_hz(frequency_hz);
    pio_sm_put_blocking(pio, sm, half_period_cycles);
}

int main() {
    stdio_init_all();


    // Configure PIO state machine to output tone on TONE_PIN
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &pwm_program);
    printf("Loaded program at %d\n", offset);

    pwm_program_init(pio, sm, offset, TONE_PIN);

    // Set initial frequency (change this variable to pick a different note)
    float frequency_hz = 440.0f; // A4

    // Provide initial half-period before enabling the state machine
    tone_set_frequency(pio, sm, frequency_hz);
    pio_sm_set_enabled(pio, sm, true);

    printf("Playing %.2f Hz on GPIO %d\n", (double)frequency_hz, TONE_PIN);

    while (true) {
        // Idle. To change note at runtime, call tone_set_frequency(pio, sm, new_hz).
        sleep_ms(1000);
    }
}
