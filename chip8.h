#ifndef _chip8_h_
#define _chip8_h_

#define WIDTH 64
#define HEIGHT 32
#define PROGRAM_START_POS 512
#define MEMORY_SIZE 4096
#define PRESSED 1
#define SPRITE_SIZE 5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

struct Chip8
{
    unsigned char memory[MEMORY_SIZE];
    unsigned char display_buffer[WIDTH * HEIGHT];

    unsigned short stack[16];
    unsigned char variables[16];
    unsigned char keys[16];

    unsigned char delay_timer;
    unsigned char sound_timer;

    unsigned short stack_pointer;
    unsigned short program_counter;
    unsigned short index;

    int need_draw;
    int play_beep;

} Chip8;

struct Chip8 *create_chip8_emulator();
void emulate(struct Chip8 *ch8);
void handle_key(struct Chip8 *ch8, SDL_Keycode key, int is_down);
void load_game(struct Chip8 *ch8, const char *game_name);

#endif