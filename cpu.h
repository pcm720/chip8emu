/*
Header file for CHIP-8 interpreter
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

#ifndef CPU_H
#define CPU_H

#include "common.h"
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#define PROGRAM_ADDRESS 0x200
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define CPU_CLOCK 500 // Hz
#define CPU_RATE (double)((1.0 / CPU_CLOCK) * 1000.0) // ms

#define TIMER_CLOCK 60 // Hz
#define TIMER_RATE (double)((1.0 / TIMER_CLOCK) * 1000.0) // ms

uint8_t memory[4096];
uint8_t registers[16];

uint8_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
bool drawFlag;
bool waitForKey;
bool cpuHalted;
uint8_t waitForRegister; // register to write the key value to
uint8_t input[16];

int cycles;
double cpuRate;
int cpuClock;

char statusString[50];

void chip8_init(bool quirks);
int chip8_cycle();
void generate_state();

//#define DEBUG
#endif