#include "common.h"
#include <errno.h>
#include "SDL.h"
#include "cpu.h"

int load_ROM(char* path);

int main(int argc, char* argv[]) {
    uint8_t extraFlag;
    bool quirks = false;
    srand((unsigned) time(NULL));
    if (argc == 1) { printf("chip8emu - a basic CHIP-8 emulator.\nUsage: chip8emu <romfile> <workaround flag>\n\n"
                  " If the program is not working properly, try inputting 1 after ROM file.\n This will enable workarounds for instructions 8XY6, 8XYE, 8X55 and 8X65.\n\n"
                  " Key mapping:\n  1 2 3 C -> 1 2 3 4\n  4 5 6 D -> Q W E R\n  7 8 9 E -> A S D F\n  A 0 B F -> Z X C V\n"
                  " Press '[' key to decrease CPU speed, ']' to increase. Press 'P' to reset the system.\n\n"); return 1; }

    if (sdl_init()) return 1;

    if (argc == 3 && *argv[2] == '1') quirks = true;
    chip8_init(quirks);
    
    if (load_ROM(argv[1])) return 1;

    play_beep();
    printf("\nEntering main loop...\n");
    while(!cpuHalted) {
        if (chip8_cycle()) play_beep();
        
        if (drawFlag) {
            generate_state();
            render_screen((uint8_t*)&screen, sizeof(screen), statusString);
            drawFlag = false;
        }

        if (get_input((uint8_t*)&input, &extraFlag)) break;
        if (waitForKey) {
            for (uint8_t i = 0; i < sizeof(registers); i++) {
                if (input[i] == 0xFF) { registers[waitForRegister] = i; waitForKey = false; }
            }
        }
        switch (extraFlag) {
            case 0xFF:
                printf("Resetting...\n");
                chip8_init(quirks);
                load_ROM(argv[1]);
                render_screen((uint8_t*)&screen, sizeof(screen), statusString);
                extraFlag = 0x0;
                break;
            case 0xF0:
                cpuClock -= 10;
                cpuRate = (1.0 / (double)cpuClock) * 1000.0;
                extraFlag = 0x0;
                break;
            case 0x0F:
                cpuClock += 10;
                cpuRate = (1.0 / (double)cpuClock) * 1000.0;
                extraFlag = 0x0;
                break;
        }
    }

    sdl_quit();    
    return EXIT_SUCCESS;
}

int load_ROM(char* path) {
    FILE* romfile = fopen(path, "rb");
    if (romfile == NULL) { printf("\nFailed to load ROM file: error %i\n",errno); return 1; }

    fseek(romfile, 0, SEEK_END);
    int file_length = ftell(romfile);
    fseek(romfile, 0, SEEK_SET);
    printf("\nLoading ROM file \"%s\" (%i bytes) to memory at offset 0x%03X...\n", path, file_length, PROGRAM_ADDRESS);
    fread(memory + PROGRAM_ADDRESS, file_length, 1, romfile);
    fclose(romfile);

    return 0;
}