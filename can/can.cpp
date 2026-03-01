#include <stdio.h>
#include "pico/stdlib.h"
#include "mcp2515/mcp2515.h"

//#define SEND 1

int main()
{
    MCP2515 mcp2515(spi0, 17, 19, 16, 18);
    
    stdio_init_all();

    mcp2515.reset();

    mcp2515.setBitrate(CAN_5KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

#ifdef SEND
    // can ID, length of data, data
    // max data length of 8 bytes
    can_frame frame{0x100, 8, "Hello!!"};    

    sleep_ms(10000);
    while (true) {
        
        MCP2515::ERROR error = mcp2515.sendMessage(&frame);
        
        printf("Sent message. Error: %d\n", error);
        sleep_ms(1000);
    }
#else
    sleep_ms(5000);
    printf("Receiver initialized!");

    can_frame frame;
    while (true) {
        MCP2515::ERROR error = mcp2515.readMessage(&frame);
        if (error == MCP2515::ERROR_OK) {
            printf("CAN id: %x", frame.can_id);
            for (int i = 0; i < frame.can_dlc; i++) {
                printf("%2x ", frame.data[i]);
            }
            printf("\n");
        }
        else {
            // if (error != MCP2515::ERROR_NOMSG) {
                printf("Error recieving message: %d\n", error);
            // }
        }
        sleep_ms(1);
    }
#endif
}
