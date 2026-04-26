#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "pico/stdlib.h"
#include "screen.h"
#include "audio_i2s.pio.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "ff.h"
#include "mcp2515/mcp2515.h"
#include "wav_sample.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// TODO: Rewrite these to be defined based on existing def statements
#define CAN_MISO 16
#define CAN_MOSI 19
#define CAN_CS 17
#define CAN_SCK 18
#define SD_CS 13

// I2S output for the external DAC.
// GPIO 22: BCLK, GPIO 23: LRCLK, GPIO 24: DIN.
#define I2S_BCLK_PIN 22
#define I2S_LRCLK_PIN 23
#define I2S_DIN_PIN 24

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// Data will be copied from src to dst
const char src[] = "Hello, world! (from DMA)";
char dst[count_of(src)];

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 8
#define UART_RX_PIN 9


void can_init() {
    MCP2515 mcp2515(spi0, CAN_CS, CAN_MOSI, CAN_MISO, CAN_SCK);
    
    // ADC Initialization

    mcp2515.reset();

    mcp2515.setBitrate(CAN_5KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();
}

static PIO g_i2s_pio = pio0;
static int g_i2s_sm = -1;
static int g_i2s_dma_channel = -1;
static uint g_i2s_program_offset = 0;
static bool g_i2s_initialized = false;
static uint32_t g_i2s_sample_rate_hz = 0;

static inline uint16_t read_u16_le(const uint8_t *bytes) {
    return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

static inline uint32_t read_u32_le(const uint8_t *bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8u) |
           (static_cast<uint32_t>(bytes[2]) << 16u) |
           (static_cast<uint32_t>(bytes[3]) << 24u);
}

static inline uint32_t pack_i2s_frame(int16_t left, int16_t right) {
    return (static_cast<uint32_t>(static_cast<uint16_t>(left)) << 16u) |
           static_cast<uint16_t>(right);
}

static bool seek_forward(FIL *file, uint32_t byte_count) {
    const FSIZE_t current = f_tell(file);
    return f_lseek(file, current + byte_count) == FR_OK;
}

static bool load_wav(const std::string &filename, wav_sample_t *sample) {
    FIL file;
    std::memset(sample, 0, sizeof(*sample));

    if (f_open(&file, filename.c_str(), FA_READ) != FR_OK) {
        printf("Failed to open WAV file: %s\n", filename.c_str());
        return false;
    }

    uint8_t riff_header[12];
    UINT bytes_read = 0;
    if (f_read(&file, riff_header, sizeof(riff_header), &bytes_read) != FR_OK || bytes_read != sizeof(riff_header)) {
        printf("Failed to read WAV header: %s\n", filename.c_str());
        f_close(&file);
        return false;
    }

    if (std::memcmp(riff_header, "RIFF", 4) != 0 || std::memcmp(riff_header + 8, "WAVE", 4) != 0) {
        printf("Not a RIFF/WAVE file: %s\n", filename.c_str());
        f_close(&file);
        return false;
    }

    bool found_fmt = false;
    bool found_data = false;

    while (f_tell(&file) + 8 <= f_size(&file)) {
        uint8_t chunk_header[8];
        if (f_read(&file, chunk_header, sizeof(chunk_header), &bytes_read) != FR_OK || bytes_read != sizeof(chunk_header)) {
            break;
        }

        const uint32_t chunk_size = read_u32_le(chunk_header + 4);
        const uint32_t padded_chunk_size = chunk_size + (chunk_size & 1u);

        if (std::memcmp(chunk_header, "fmt ", 4) == 0) {
            if (chunk_size < 16u) {
                printf("Invalid fmt chunk in %s\n", filename.c_str());
                f_close(&file);
                return false;
            }

            uint8_t fmt_chunk[16];
            if (f_read(&file, fmt_chunk, sizeof(fmt_chunk), &bytes_read) != FR_OK || bytes_read != sizeof(fmt_chunk)) {
                printf("Failed to read fmt chunk in %s\n", filename.c_str());
                f_close(&file);
                return false;
            }

            sample->audio_format = read_u16_le(fmt_chunk + 0);
            sample->channel_count = read_u16_le(fmt_chunk + 2);
            sample->sample_rate_hz = read_u32_le(fmt_chunk + 4);
            sample->byte_rate = read_u32_le(fmt_chunk + 8);
            sample->block_align = read_u16_le(fmt_chunk + 12);
            sample->bits_per_sample = read_u16_le(fmt_chunk + 14);

            if (padded_chunk_size > sizeof(fmt_chunk) && !seek_forward(&file, padded_chunk_size - sizeof(fmt_chunk))) {
                printf("Failed to skip fmt extension in %s\n", filename.c_str());
                f_close(&file);
                return false;
            }

            found_fmt = true;
        } else if (std::memcmp(chunk_header, "data", 4) == 0) {
            uint8_t *raw_data = static_cast<uint8_t *>(std::malloc(chunk_size));
            if (!raw_data) {
                printf("Out of memory loading WAV data: %s\n", filename.c_str());
                f_close(&file);
                return false;
            }

            if (f_read(&file, raw_data, chunk_size, &bytes_read) != FR_OK || bytes_read != chunk_size) {
                printf("Failed to read WAV data: %s\n", filename.c_str());
                std::free(raw_data);
                f_close(&file);
                return false;
            }

            if ((chunk_size & 1u) && !seek_forward(&file, 1u)) {
                printf("Failed to skip padded WAV byte: %s\n", filename.c_str());
                std::free(raw_data);
                f_close(&file);
                return false;
            }

            sample->data_size_bytes = chunk_size;
            sample->data = raw_data;
            found_data = true;
        } else {
            if (!seek_forward(&file, padded_chunk_size)) {
                printf("Failed to skip WAV chunk in %s\n", filename.c_str());
                f_close(&file);
                return false;
            }
        }

        if (found_fmt && found_data) {
            break;
        }
    }

    f_close(&file);

    if (!found_fmt || !found_data) {
        printf("Missing fmt/data chunk in %s\n", filename.c_str());
        std::free(const_cast<uint8_t *>(sample->data));
        std::memset(sample, 0, sizeof(*sample));
        return false;
    }

    if (sample->audio_format != 1u) {
        printf("Unsupported WAV format %u in %s\n", sample->audio_format, filename.c_str());
        std::free(const_cast<uint8_t *>(sample->data));
        std::memset(sample, 0, sizeof(*sample));
        return false;
    }

    if ((sample->channel_count != 1u && sample->channel_count != 2u) ||
        (sample->bits_per_sample != 8u && sample->bits_per_sample != 16u) ||
        sample->block_align == 0u || sample->sample_rate_hz == 0u) {
        printf("Unsupported WAV layout in %s\n", filename.c_str());
        std::free(const_cast<uint8_t *>(sample->data));
        std::memset(sample, 0, sizeof(*sample));
        return false;
    }

    return true;
}

static bool build_i2s_frames(const wav_sample_t &sample, uint32_t **frames_out, size_t *frame_count_out) {
    const size_t frame_count = sample.data_size_bytes / sample.block_align;
    if (frame_count == 0u) {
        return false;
    }

    uint32_t *frames = static_cast<uint32_t *>(std::malloc(frame_count * sizeof(uint32_t)));
    if (!frames) {
        return false;
    }

    if (sample.bits_per_sample == 16u) {
        const int16_t *pcm16 = reinterpret_cast<const int16_t *>(sample.data);
        if (sample.channel_count == 1u) {
            for (size_t i = 0; i < frame_count; ++i) {
                frames[i] = pack_i2s_frame(pcm16[i], pcm16[i]);
            }
        } else {
            for (size_t i = 0; i < frame_count; ++i) {
                frames[i] = pack_i2s_frame(pcm16[i * 2u], pcm16[i * 2u + 1u]);
            }
        }
    } else {
        const uint8_t *pcm8 = sample.data;
        if (sample.channel_count == 1u) {
            for (size_t i = 0; i < frame_count; ++i) {
                const int16_t mono = static_cast<int16_t>((static_cast<int>(pcm8[i]) - 128) << 8);
                frames[i] = pack_i2s_frame(mono, mono);
            }
        } else {
            for (size_t i = 0; i < frame_count; ++i) {
                const int16_t left = static_cast<int16_t>((static_cast<int>(pcm8[i * 2u]) - 128) << 8);
                const int16_t right = static_cast<int16_t>((static_cast<int>(pcm8[i * 2u + 1u]) - 128) << 8);
                frames[i] = pack_i2s_frame(left, right);
            }
        }
    }

    *frames_out = frames;
    *frame_count_out = frame_count;
    return true;
}

static bool init_i2s_output(uint32_t sample_rate_hz) {
    if (!g_i2s_initialized) {
        g_i2s_sm = pio_claim_unused_sm(g_i2s_pio, false);
        if (g_i2s_sm < 0) {
            printf("No free PIO state machine for I2S\n");
            return false;
        }

        g_i2s_dma_channel = dma_claim_unused_channel(false);
        if (g_i2s_dma_channel < 0) {
            printf("No free DMA channel for I2S\n");
            pio_sm_unclaim(g_i2s_pio, static_cast<uint>(g_i2s_sm));
            g_i2s_sm = -1;
            return false;
        }

        gpio_set_function(I2S_DIN_PIN, GPIO_FUNC_PIO0);
        gpio_set_function(I2S_BCLK_PIN, GPIO_FUNC_PIO0);
        gpio_set_function(I2S_LRCLK_PIN, GPIO_FUNC_PIO0);

        g_i2s_program_offset = pio_add_program(g_i2s_pio, &audio_i2s_swapped_program);
        audio_i2s_swapped_program_init(g_i2s_pio, static_cast<uint>(g_i2s_sm), g_i2s_program_offset, I2S_DIN_PIN, I2S_BCLK_PIN);

        dma_channel_config dma_config = dma_channel_get_default_config(static_cast<uint>(g_i2s_dma_channel));
        channel_config_set_dreq(&dma_config, pio_get_dreq(g_i2s_pio, static_cast<uint>(g_i2s_sm), true));
        channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
        channel_config_set_read_increment(&dma_config, true);
        channel_config_set_write_increment(&dma_config, false);
        dma_channel_configure(
            static_cast<uint>(g_i2s_dma_channel),
            &dma_config,
            &g_i2s_pio->txf[g_i2s_sm],
            nullptr,
            0,
            false
        );

        g_i2s_initialized = true;
    }

    if (g_i2s_sample_rate_hz != sample_rate_hz) {
        const uint32_t system_clock_hz = clock_get_hz(clk_sys);
        const uint32_t divider = system_clock_hz * 4u / sample_rate_hz;
        pio_sm_set_clkdiv_int_frac(g_i2s_pio, static_cast<uint>(g_i2s_sm), divider >> 8u, divider & 0xffu);
        g_i2s_sample_rate_hz = sample_rate_hz;
    }

    return true;
}

bool play_wav(const std::string &filename) {
    wav_sample_t sample;
    uint32_t *frames = nullptr;
    size_t frame_count = 0u;

    if (!load_wav(filename, &sample)) {
        return false;
    }

    if (!build_i2s_frames(sample, &frames, &frame_count)) {
        printf("Failed to build I2S frames for %s\n", filename.c_str());
        std::free(const_cast<uint8_t *>(sample.data));
        return false;
    }

    if (!init_i2s_output(sample.sample_rate_hz)) {
        std::free(frames);
        std::free(const_cast<uint8_t *>(sample.data));
        return false;
    }

    pio_sm_set_enabled(g_i2s_pio, static_cast<uint>(g_i2s_sm), false);
    pio_sm_clear_fifos(g_i2s_pio, static_cast<uint>(g_i2s_sm));
    pio_sm_restart(g_i2s_pio, static_cast<uint>(g_i2s_sm));
    pio_sm_set_enabled(g_i2s_pio, static_cast<uint>(g_i2s_sm), true);

    dma_channel_transfer_from_buffer_now(static_cast<uint>(g_i2s_dma_channel), frames, frame_count);
    dma_channel_wait_for_finish_blocking(static_cast<uint>(g_i2s_dma_channel));

    static const uint32_t silence_frames[8] = {0};
    dma_channel_transfer_from_buffer_now(static_cast<uint>(g_i2s_dma_channel), silence_frames, count_of(silence_frames));
    dma_channel_wait_for_finish_blocking(static_cast<uint>(g_i2s_dma_channel));

    while (!pio_sm_is_tx_fifo_empty(g_i2s_pio, static_cast<uint>(g_i2s_sm))) {
        tight_loop_contents();
    }

    sleep_ms(1);
    pio_sm_set_enabled(g_i2s_pio, static_cast<uint>(g_i2s_sm), false);

    std::free(frames);
    std::free(const_cast<uint8_t *>(sample.data));
    return true;
}

// Copied from sd card implementatino, modified to support WAV files
int read_sd()
{
    FATFS fs;
    DIR dir;
    FILINFO file_info;

    f_mount(&fs, "0:", 0);

    FRESULT fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        uart_puts(UART_ID, "Error opening directory!\n");
        return 0;
    }   
    while (true)
    {
        fr = f_readdir(&dir, &file_info);
        if (fr != FR_OK) {
            uart_puts(UART_ID, "Error reading directory!\n");
            return 0;
        }
        if (file_info.fname[0] == 0) {
            break;
        }
        uart_puts(UART_ID, "File name: ");
        uart_puts(UART_ID, file_info.fname);
        uart_puts(UART_ID, "\n");
    }

    // char line[100];
    // FRESULT result = f_open(&file, "dantheman.txt", FA_READ);

    // if(result != FR_OK) {
    //     printf("Failed to read file!\n");
    //     return 0;
    // }

    // printf("Contents of file:\n");
    // while (f_gets(line, sizeof(line), &file)) {
    //     printf("%s", line);
    // }

    printf("\n");

    return 1;

}

void uart_core1() {
    while(true){
        // uart_puts(UART_ID, "Hello from core 1!\n");
        play_wav("0:/c4.wav");
        uart_puts(UART_ID, "Playing Tone\n");
        sleep_ms(1000);
    }
}

int main()
{
    stdio_init_all();


    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Get a free channel, panic() if there are none
    int chan = dma_claim_unused_channel(true);
    
    // 8 bit transfers. Both read and write address increment after each
    // transfer (each pointing to a location in src or dst respectively).
    // No DREQ is selected, so the DMA transfers as fast as it can.
    
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    
    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        dst,           // The initial write address
        src,           // The initial read address
        count_of(src), // Number of transfers; in this case each is 1 byte.
        true           // Start immediately.
    );
    
    // We could choose to go and do something else whilst the DMA is doing its
    // thing. In this case the processor has nothing else to do, so we just
    // wait for the DMA to finish.
    dma_channel_wait_for_finish_blocking(chan);
    
    // The DMA has now copied our text from the transmit buffer (src) to the
    // receive buffer (dst), so we can print it out from there.
    puts(dst);

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // CAN Bus Initialisation
    can_init();

    // UART Second Core Startup
    multicore_launch_core1(uart_core1);
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart
    // screen_run();

}
