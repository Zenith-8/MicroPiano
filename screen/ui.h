#include "pico/stdlib.h"

enum Input {INPUT_NONE, INPUT_LEFT, INPUT_RIGHT, INPUT_SELECT};

struct SampleList {
    int length; // The number of samples in the list
    int selected; // The currently selected sample (-1 for none)
    char** names; // A list of names of the samples
};
typedef struct SampleList SampleList;

void ui_init(void);
void ui_update(int input);
void ui_set_volume_changed_callback(void (*callback)(int));
void ui_set_get_sample_list_callback(SampleList (*callback)());
void ui_set_sample_changed_callback(void (*callback)(int));
void ui_set_octave_changed_callback(void (*callback)(int));