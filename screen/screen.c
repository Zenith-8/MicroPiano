// An example of how to use the UI. Send z, x, and c in stdio to control, outputs all changed states to stdio
#include <stdio.h>

#include "pico/stdlib.h"
#include "ui.h"

int selected_sample = -1;

char* test_1[] = {
    "123456789012345678901234567890",
    "Piano",
    "metal_pipe",
    "French Horn",
    "Kazoo",
    "So",
    "Many",
    "Instruments intruments instruments",
    "The",
    "Screen",
    "Overflows",
    "So",
    "I",
    "Can",
    "Test",
    "Overflow",
    "Logic"
};

char* test_2[] = {
    "testing edge cases",
    "12345678901234",
    "123456789012345",
    "1234567890123456",
    "12345678901234567",
    "",
    "!$(^#&)@;></{}[]",
    "piano",
    "one more"
};

void volume_changed(int volume)
{
    printf("Volume changed to: %d\n", volume);
}

void sample_changed(int sample)
{
    selected_sample = sample;
    printf("Sample changed to: %s\n", test_1[sample]);
}

void octave_changed(int octave)
{
    printf("Octave changed to: %d\n", octave);
}

SampleList get_sample_list()
{
    SampleList sl = {17, selected_sample, test_1};
    return sl;
}

int main()
{
    ui_init();

    ui_set_get_sample_list_callback(&get_sample_list);
    ui_set_sample_changed_callback(&sample_changed);
    ui_set_volume_changed_callback(&volume_changed);
    ui_set_octave_changed_callback(&octave_changed);

    stdio_init_all();

    ui_update(INPUT_NONE);
    
    while (true)
    {
        char c;
        int input;
        scanf("%c", &c);
        switch (c) {
            case 'z':
                input = INPUT_LEFT;
                break;
            case 'x':
                input = INPUT_RIGHT;
                break;
            case 'c':
                input = INPUT_SELECT;
                break;
            default:
                input = INPUT_NONE;
                break;
        }
        ui_update(input);
    }
}