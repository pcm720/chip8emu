/*
Header file for SDL2-related functions
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

#ifndef SDL_H
#define SDL_H

#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define COLOR_MAX 255
#define SCALE 10
#define BEEP "res/beep.wav"
#define FONT "res/FreeSans.ttf"
#define FONT_SIZE 25
#define WINDOW_WIDTH 64
#define WINDOW_HEIGHT 35

int sdl_init();
void sdl_quit();
void play_beep();
void render_screen(uint8_t* screen, size_t screen_size, char* statusString);

int get_input(uint8_t* input, uint8_t* extraFlag);
#endif