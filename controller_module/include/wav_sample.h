#ifndef WAV_SAMPLE_H
#define WAV_SAMPLE_H

#include <stddef.h>
#include <stdint.h>

typedef struct wav_sample {
    uint16_t audio_format;
    uint16_t channel_count;
    uint32_t sample_rate_hz;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    size_t data_size_bytes;
    const uint8_t *data;
} wav_sample_t;

#endif  // WAV_SAMPLE_H
