/*
CHIP-8 interpreter
    Copyright (C) 2019 pcm720 <pcm720@gmail.com>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see 
<http://www.gnu.org/licenses/>.
*/

#include "cpu.h"
#include <sys/time.h>

uint16_t indexRegister;
uint16_t programCounter;
uint16_t stack[16];
uint8_t stackPointer;

bool quirkWorkaround;

uint8_t delayTimer;
uint8_t soundTimer;

struct timeval cpuTime;
struct timeval timerTime;
struct timeval cpsTime;
int cps;
int cpsCounter;

void chip8_decode_execute(uint16_t instr);
void draw_sprite(uint8_t screenX, uint8_t screenY, uint8_t bytes);
double timediff_ms(struct timeval *end, struct timeval *start);

void chip8_init(bool quirks) {
    memset(memory, 0x0, sizeof(memory));
    memset(registers, 0x0, sizeof(registers));
    memset(stack, 0x0, sizeof(stack));
    memset(screen, 0x0, sizeof(screen));
    memset(input, 0x0, sizeof(input));
    indexRegister = 0x0;
    programCounter = PROGRAM_ADDRESS;
    stackPointer = 0x0;
    delayTimer = 0x0;
    soundTimer = 0x0;
    drawFlag = false;
    waitForKey = 0x0;
    cycles = 0;
    quirkWorkaround = quirks;
    uint8_t chip8Fontset[80] = { 
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    memcpy(&memory, &chip8Fontset, sizeof(chip8Fontset));
    
    cpuClock = CPU_CLOCK;
    cpuRate = CPU_RATE;
    cpuHalted = false;
    cps = 0;
    cpsCounter = 0;
    gettimeofday(&cpuTime, NULL);
    timerTime = cpuTime;
    cpsTime = cpuTime;
    printf("CHIP-8 CPU initialized.");
    if (quirkWorkaround) printf(" Quirks enabled");
}

int chip8_cycle() {
    //get current time
    struct timeval time_cur;
    gettimeofday(&time_cur, NULL);

    // if difference exceeds cpuRate, execute one cycle
    if (timediff_ms(&time_cur, &cpuTime) >= cpuRate) {
        cpuTime = time_cur;
        // fetch-decode-execute
        if (programCounter > sizeof(memory) - 1) {
            cpuHalted = true;
            printf("\nCPU halted: PC exceeded memory limits\n");
            return 1;
        }
        if (!waitForKey) {
            chip8_decode_execute((memory[programCounter] << 8) | memory[programCounter + 1]);
            cycles++;
        }
        cpsCounter++;
    }

    // if difference exceeds 1000 ms, increment clocks per second counter
    if (timediff_ms(&time_cur, &cpsTime) >= 1000) {
        cps = cpsCounter;
        cpsCounter = 0;
        cpsTime = time_cur;
    }

    // if difference exceeds 16.66 ms, update delay and sound timers
    if (timediff_ms(&time_cur, &timerTime) >= TIMER_RATE) {
        timerTime = time_cur;
        if (delayTimer > 0) delayTimer--;
        if (soundTimer > 0) {
            soundTimer--;
            return 1;
        }
    }
    return 0;
}

void chip8_decode_execute(uint16_t instr) {
    uint16_t temp = 0x0;
    switch(instr & 0xF000) {
        case 0x0000:
            switch (instr & 0x00FF){
                case 0x00: // NOP
                    break;
                case 0xE0: // 00E0 - CLS - clear screen
                    memset(screen, 0x0, sizeof(screen));
                    drawFlag = true;
                    break;
                case 0xEE: // 00EE - RET - return from subroutine
                    programCounter = stack[stackPointer];
                    stackPointer--;
                    break;
                default:
                    printf("Illegal opcode %02X!\n", instr);
                    break;
            }
            break;
        case 0x1000: // 1nnn - JMP addr — jump to nnn 
            programCounter = instr & 0x0FFF;
            programCounter -= 2; // bypass increment and the end of the cycle
            break;
        case 0x2000: // 2nnn - CALL addr - save PC to stack and jump to nnn
            stackPointer++;
            stack[stackPointer] = programCounter;
            programCounter = instr & 0x0FFF;
            programCounter -= 2;
            break;
        case 0x3000: // 3xkk - SE Vx, kk - skip instruction if Vx = kk
            if ((registers[(instr & 0x0F00) >> 8]) == (instr & 0x00FF)) programCounter += 2;
            break;
        case 0x4000: // 4xkk - SNE Vx, kk - skip instruction if Vx != kk
            if ((registers[(instr & 0x0F00) >> 8]) != (instr & 0x00FF)) programCounter += 2;
            break;
        case 0x5000: // 5xy0 - SE Vx, Vy - skip next instruction if Vx = Vy
            if ((registers[(instr & 0x0F00) >> 8]) == (registers[(instr & 0x00F0) >> 4])) programCounter += 2;
            break;
        case 0x6000: // 6xkk - LD Vx, kk - set Vx = kk
            registers[(instr & 0x0F00) >> 8] = instr & 0x00FF;
            break;
        case 0x7000: // 7xkk - ADD Vx, byte - set Vx = Vx + kk
            registers[(instr & 0x0F00) >> 8] += instr & 0x00FF;
            break;
        case 0x8000: // 8??? - register operations
            switch(instr & 0x000F) {
                case 0x0: // 8xy0 - LD Vx, Vy - set Vx = Vy
                    registers[(instr & 0x0F00) >> 8] = registers[(instr & 0x00F0) >> 4];
                    break;
                case 0x1: // 8xy1 - OR Vx, Vy - set Vx = Vx OR Vy
                    registers[(instr & 0x0F00) >> 8] |= registers[(instr & 0x00F0) >> 4];
                    break;
                case 0x2: // 8xy2 - AND Vx, Vy - set Vx = Vx AND Vy
                    registers[(instr & 0x0F00) >> 8] &= registers[(instr & 0x00F0) >> 4];
                    break;
                case 0x3: // 8xy3 - XOR Vx, Vy - set Vx = Vx XOR Vy
                    registers[(instr & 0x0F00) >> 8] ^= registers[(instr & 0x00F0) >> 4];
                    break;
                case 0x4: // 8xy4 - ADD Vx, Vy - set Vx = Vx + Vy, set VF = carry
                    temp = registers[(instr & 0x0F00) >> 8] + registers[(instr & 0x00F0) >> 4];
                    if ((temp & 0xFF00) > 0) registers[0xF] = 0x1;
                    else registers[0xF] = 0x0;
                    registers[(instr & 0x0F00) >> 8] = temp & 0x00FF;
                    break;
                case 0x5: // 8xy5 - SUB Vx, Vy - set Vx = Vx - Vy, set VF = NOT borrow
                    if (registers[(instr & 0x0F00) >> 8] > registers[(instr & 0x00F0) >> 4]) registers[0xF] = 0x1;
                    else registers[0xF] = 0x0;
                    registers[(instr & 0x0F00) >> 8] -= registers[(instr & 0x00F0) >> 4];
                    break;
                case 0x6: // 8xy6 - SHR Vx {, Vy} - set Vx = Vx SHR 1
                    if (quirkWorkaround) {
                        registers[0xF] = registers[(instr & 0x0F00) >> 8] & 0x1;
                        registers[(instr & 0x0F00) >> 8] >>= 1;
                    } else {
                        registers[0xF] = registers[(instr & 0x00F0) >> 4] & 0x1;
                        registers[(instr & 0x00F0) >> 4] >>= 1;
                        registers[(instr & 0x0F00) >> 8] = registers[(instr & 0x00F0) >> 4];
                    }
                    break;
                case 0x7: // 8xy7 - SUBN Vx, Vy - set Vx = Vy - Vx, set VF = NOT borrow
                    if (registers[(instr & 0x00F0) >> 4] > registers[(instr & 0x0F00) >> 8]) registers[0xF] = 0x1;
                    else registers[0xF] = 0x0;
                    registers[(instr & 0x0F00) >> 8] = registers[(instr & 0x00F0) >> 4] - registers[(instr & 0x0F00) >> 8];
                    break;
                case 0xE: // 8xyE - SHL Vx {, Vy} - set Vx = Vx SHL 1
                    if (quirkWorkaround) {
                        registers[0xF] = (registers[(instr & 0x0F00) >> 8] >> 7) & 0x1;
                        registers[(instr & 0x0F00) >> 8] <<= 1;
                    } else {
                        registers[0xF] = (registers[(instr & 0x00F0) >> 4] >> 7) & 0x1;
                        registers[(instr & 0x00F0) >> 4] <<= 1;
                        registers[(instr & 0x0F00) >> 8] = registers[(instr & 0x00F0) >> 4];
                    }
                    break;
                default:
                    printf("Illegal opcode %02X!\n", instr & 0xF00F);
                    break;
            }
            break;
        case 0x9000: // 9xy0 - SNE Vx, Vy - skip next instruction if Vx != Vy.
            if (registers[(instr & 0x0F00) >> 8] != registers[(instr & 0x00F0) >> 4]) programCounter += 2;
            break;
        case 0xA000: // Annn - LD I, addr - set I = nnn.
            indexRegister = instr & 0x0FFF;
            break;
        case 0xB000: // Bnnn - JP V0, addr - jump to location nnn + V0.
            programCounter = (instr & 0x0FFF) + registers[0];
            break;
        case 0xC000: // Cxkk - RND Vx, byte - set Vx = random byte AND kk.
            registers[(instr & 0x0F00) >> 8] = (rand() % 0xFF) & (instr & 0x00FF);
            break;
        case 0xD000: // Dxyn - DRW Vx, Vy, nibble - display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            draw_sprite(registers[(instr & 0x0F00) >> 8], registers[(instr & 0x00F0) >> 4], (instr & 0x000F));
            drawFlag = true;
            break;
        case 0xE000:
            if ((instr & 0x00FF) == 0x009E) { // Ex9E - SKP Vx - skip next instruction if key with the value of Vx is pressed.
                if (input[registers[(instr & 0x0F00) >> 8]] == 0xFF) programCounter += 2;
            } else if (instr & 0x00A1) { // ExA1 - SKNP Vx - skip next instruction if key with the value of Vx is not pressed.
                if (input[registers[(instr & 0x0F00) >> 8]] == 0x0) programCounter += 2;
            } else printf("Illegal opcode %02X!\n", instr);
            break;
        case 0xF000:
            switch(instr & 0x00FF) {
                case 0x07: // Fx07 - LD Vx, DT - set Vx = delay timer value.
                    registers[(instr & 0x0F00) >> 8] = delayTimer;
                    break;
                case 0x0A: // Fx0A - LD Vx, K - wait for a key press, store the value of the key in Vx.
                    waitForKey = true;
                    waitForRegister = (instr & 0x0F00) >> 8;
                    break;
                case 0x15: // Fx15 - LD DT, Vx - set delay timer = Vx.
                    delayTimer = registers[(instr & 0x0F00) >> 8];
                    break;
                case 0x18: // Fx18 - LD ST, Vx - set sound timer = Vx.
                    soundTimer = registers[(instr & 0x0F00) >> 8];
                    break;
                case 0x1E: // Fx1E - ADD I, Vx - set I = I + Vx.
                    indexRegister += registers[(instr & 0x0F00) >> 8];
                    break;
                case 0x29: // Fx29 - LD F, Vx - set I = location of sprite for digit Vx.
                    indexRegister = registers[(instr & 0x0F00) >> 8] * 5; // Each sprites occupies 5 bytes in memory, sprites are loaded at 0x0
                    break;
                case 0x33: // Fx33 - LD B, Vx - store BCD representation of Vx in memory locations I, I+1, and I+2.
                    temp = registers[(instr & 0x0F00) >> 8];
                    memory[indexRegister] = (temp - temp % 100) / 100;
                    memory[indexRegister + 1] = (temp % 100 - temp % 10) / 10;
                    memory[indexRegister + 2] = temp % 10;
                    break;
                case 0x55: // Fx55 - LD [I], Vx - store registers V0 through Vx in memory starting at location I.
                    for (int i = 0; i <= ((instr & 0x0F00) >> 8); i++) {
                        memory[indexRegister + i] = registers[i];
                    }
                    if (!quirkWorkaround) indexRegister += registers[(instr & 0x0F00) >> 8] + 1;
                    break;
                case 0x65: // Fx65 - LD Vx, [I] - read registers V0 through Vx from memory starting at location I.
                    for (int i = 0; i <= ((instr & 0x0F00) >> 8); i++) {
                        registers[i] = memory[indexRegister + i];
                    }
                    if (!quirkWorkaround) indexRegister += registers[(instr & 0x0F00) >> 8] + 1;
                    break;
            }
            break;
        default:
            printf("Illegal opcode %02X!\n", instr & 0xF000);
    }
#ifdef DEBUG
    printf("\n\nInstruction: %04X; Cycle: %i\n", instr, cycles);
    printf("Dump:\nPC: %04X\tSP: %04X\nStack:\n", programCounter, stackPointer);
    for (int i = 0; i < 16; i++) {
        if (i > 0 && ((i % 4) == 0)) printf("\n");
        printf("%02X: %02X\t", i, stack[i]);
    }
    printf("\nRegisters:\n");
    for (int i = 0; i < 16; i++) {
        if (i > 0 && ((i % 4) == 0)) printf("\n");
        printf("V%01X: %02X\t", i, registers[i]);
    }
#endif
    programCounter += 2; //increase PC by 2 (go to the next instruction)
}

void draw_sprite(uint8_t screenX, uint8_t screenY, uint8_t bytes) {
    uint16_t colPosition;
    uint16_t rowPosition;
    uint8_t prevPixel;
    registers[0xF] = 0x0;
    for (uint8_t y = 0; y < bytes; y++) {                                                        // for each sprite row:
        rowPosition = (((screenY + y) % SCREEN_HEIGHT)*64);                                         // calculate the position of [screenY] row in screen array, wrap around if [screenY + Y] > SCREEN_HEIGHT
        for (uint8_t x = 0; x < 8; x++) {                                                           // for each sprite column:
            colPosition = rowPosition + ((screenX + x) % SCREEN_WIDTH);                               // calculate the position of pixel in screen array, wrap around if [screenX + x] > SCREEN_WIDTH
            prevPixel = screen[colPosition];                                                          // save previous value of pixel at [column, row]
            screen[colPosition] ^= (memory[indexRegister + y] >> (7 - x)) & 0x1;                      // XOR this pixel with sprite bit x
            if (prevPixel != screen[colPosition] && screen[colPosition] == 0x0) registers[0xF] = 0x1; // check if pixel value changed and was set to 0 in the process (collision)
        }
    }
}

void generate_state() {
    snprintf(statusString, 50, "Clock: %i Hz | Speed: %03.02f%%", cpuClock, ((double)cps / (double)cpuClock) * 100.0);
}

double timediff_ms(struct timeval* end, struct timeval* start) {
    double diff =  (end->tv_sec - start->tv_sec) * 1000.0 +
                (end->tv_usec - start->tv_usec) / 1000.0;
    return diff;
}