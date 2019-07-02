/*
SDL2 functions
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

#include "SDL.h"

SDL_Event event;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
Mix_Chunk* soundBeep;
TTF_Font* statusFont;
SDL_Surface* statusSurface;
SDL_Texture* statusTexture;

int video_init();
int sound_init();
int font_init();

void *pixels;
int pitch;

int sdl_init(void) {
    if (video_init()) return EXIT_FAILURE;
    if (sound_init()) return EXIT_FAILURE;
    if (font_init()) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int video_init() {
    texture = NULL;
    soundBeep = NULL;
    
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        { printf("\nSDL failed to initialize! Error: %s\n", SDL_GetError() ); return EXIT_FAILURE; }
    else printf("SDL2, ");

    window = SDL_CreateWindow("chip8emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE, SDL_WINDOW_VULKAN);
    if (window == NULL) { printf("\nSDL_Window failed to initialize! Error: %s\n", SDL_GetError() ); return EXIT_FAILURE; }
    else printf("SDL_Window, ");

    renderer = SDL_CreateRenderer(window, 3, (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (renderer == NULL) { printf("\nSDL_Renderer failed to initialize! Error: %s\n", SDL_GetError() ); return EXIT_FAILURE; }
    else printf("SDL_Renderer initialized.\n");

    SDL_RenderSetScale(renderer,SCALE,SCALE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    return EXIT_SUCCESS;
}

int sound_init() {
    if(Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0)
        { printf("\nSDL_mixer could not initialize! Error: %s\n", Mix_GetError() ); return EXIT_FAILURE; }
    else printf("SDL2_mixer initialized, ");

    soundBeep = Mix_LoadWAV(BEEP);
    if (soundBeep == NULL) { printf("\nFailed to load sound file %s! Error: %s\n", BEEP, Mix_GetError() ); return EXIT_FAILURE; }
    else printf("sound file \"%s\" loaded.\n", BEEP);
    Mix_Volume(-1, MIX_MAX_VOLUME/10);

    return EXIT_SUCCESS;
}

int font_init() {
    if (TTF_Init() < 0) { printf("\nTTF library failed to initialize! Error: %s\n", TTF_GetError() ); return EXIT_FAILURE; }
    else printf("SDL2_ttf initialized, ");

    statusFont = TTF_OpenFont(FONT,FONT_SIZE);
    if (statusFont == NULL) { printf("\nTTF_font failed to load! Error: %s\n", TTF_GetError() ); return EXIT_FAILURE; }
    else printf("TTF font \"%s\" loaded.\n", FONT);
    return EXIT_SUCCESS;
}

void sdl_quit() {
    Mix_FreeChunk(soundBeep);
    soundBeep = NULL;
    TTF_CloseFont(statusFont);
    statusFont = NULL;
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_DestroyTexture(statusTexture);

    TTF_Quit();
    Mix_Quit();
    SDL_Quit();
}

void play_beep() {
    Mix_PlayChannel(-1, soundBeep, 0);
}

void render_screen(uint8_t* screen, size_t screen_size, char* statusString) {
    SDL_Color colorWhite = {255, 255, 255}; 
    SDL_Rect renderQuad = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    uint8_t *pixelPointer;
    
    // update screen texture
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    for (int i = 0; i < screen_size; i++) {
        pixelPointer = ((uint8_t *)pixels) + (i * 4);
        memset(pixelPointer, screen[i]*COLOR_MAX, 4);
    }
    SDL_UnlockTexture(texture);

    
    SDL_RenderClear(renderer);                            // clear window
    SDL_RenderSetScale(renderer,SCALE,SCALE);             // set scaling factor for screen texture
    SDL_RenderCopy(renderer, texture, NULL, &renderQuad); // copy screen texture to renderer

    // render and copy status string to renderer
    statusSurface = TTF_RenderText_Blended_Wrapped(statusFont, statusString, colorWhite, SCREEN_WIDTH*SCALE);
    statusTexture = SDL_CreateTextureFromSurface(renderer, statusSurface);
    renderQuad.h = statusSurface->h;
    renderQuad.w = statusSurface->w;
    renderQuad.y = SCREEN_HEIGHT*SCALE;
    SDL_RenderSetScale(renderer,1,1);
    SDL_RenderCopy(renderer, statusTexture, NULL, &renderQuad);

    SDL_DestroyTexture(statusTexture);
    SDL_FreeSurface(statusSurface);

    // render to window
    SDL_RenderPresent(renderer);
}

int get_input(uint8_t* input, uint8_t* extraFlag) {
    if (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            printf("\nExiting...\n");
            return 1;
        }
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_x: input[0x0] = 0xFF; break;
                case SDLK_1: input[0x1] = 0xFF; break;
                case SDLK_2: input[0x2] = 0xFF; break;
                case SDLK_3: input[0x3] = 0xFF; break;
                case SDLK_q: input[0x4] = 0xFF; break;
                case SDLK_w: input[0x5] = 0xFF; break;
                case SDLK_e: input[0x6] = 0xFF; break;
                case SDLK_a: input[0x7] = 0xFF; break;
                case SDLK_s: input[0x8] = 0xFF; break;
                case SDLK_d: input[0x9] = 0xFF; break;
                case SDLK_z: input[0xA] = 0xFF; break;
                case SDLK_c: input[0xB] = 0xFF; break;
                case SDLK_4: input[0xC] = 0xFF; break;
                case SDLK_r: input[0xD] = 0xFF; break;
                case SDLK_f: input[0xE] = 0xFF; break;
                case SDLK_v: input[0xF] = 0xFF; break;
                case SDLK_p: *extraFlag = 0xFF; break;
                case SDLK_LEFTBRACKET: *extraFlag = 0xF0; break;
                case SDLK_RIGHTBRACKET: *extraFlag = 0x0F; break;
            }
        }
        if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_x: input[0x0] = 0x00; break;
                case SDLK_1: input[0x1] = 0x00; break;
                case SDLK_2: input[0x2] = 0x00; break;
                case SDLK_3: input[0x3] = 0x00; break;
                case SDLK_q: input[0x4] = 0x00; break;
                case SDLK_w: input[0x5] = 0x00; break;
                case SDLK_e: input[0x6] = 0x00; break;
                case SDLK_a: input[0x7] = 0x00; break;
                case SDLK_s: input[0x8] = 0x00; break;
                case SDLK_d: input[0x9] = 0x00; break;
                case SDLK_z: input[0xA] = 0x00; break;
                case SDLK_c: input[0xB] = 0x00; break;
                case SDLK_4: input[0xC] = 0x00; break;
                case SDLK_r: input[0xD] = 0x00; break;
                case SDLK_f: input[0xE] = 0x00; break;
                case SDLK_v: input[0xF] = 0x00; break;
                case SDLK_p:
                case SDLK_LEFTBRACKET:
                case SDLK_RIGHTBRACKET: *extraFlag = 0x00; break;
            }
        }
    }
    return 0;
}