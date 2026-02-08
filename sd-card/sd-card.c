#include <stdio.h>
#include "pico/stdlib.h"
#include "ff.h"

int read_sd()
{
    FATFS fs;
    FIL file;
    DIR dir;
    FILINFO file_info;

    f_mount(&fs, "0:", 0);

    FRESULT fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        printf("Error opening directory!\n");
        return 0;
    }
    while (true)
    {
        fr = f_readdir(&dir, &file_info);
        if (fr != FR_OK) {
            printf("Error reading directory!\n");
            return 0;
        }
        if (file_info.fname[0] == 0) {
            break;
        }
        printf("File name: %s\n", file_info.fname);
    }

    char line[100];
    FRESULT result = f_open(&file, "dantheman.txt", FA_READ);

    if(result != FR_OK) {
        printf("Failed to read file!\n");
        return 0;
    }

    printf("Contents of file:\n");
    while (f_gets(line, sizeof(line), &file)) {
        printf("%s", line);
    }
    
    printf("\n");
    f_close(&file);

}

int main()
{
    stdio_init_all();
    while (true) {
        read_sd();
        sleep_ms(1000);
    }
}