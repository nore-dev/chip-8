#include <stdio.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include "chip8.h"

#define PIXEL_SIZE 10

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    unsigned pixels[HEIGHT * WIDTH * 4];

    struct Chip8 *ch8 = create_chip8_emulator();

    if (argc < 2)
    {
        printf("Example Usage: chip8_emu test.ch8");
        exit(EXIT_FAILURE);
    }

    load_game(ch8, argv[1]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec wav_spec;
    Uint32 wav_length;
    Uint8 *wav_buffer;

    SDL_LoadWAV("beep.wav", &wav_spec, &wav_buffer, &wav_length);
    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);

    window = SDL_CreateWindow(
        "Chip8 Emulator",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == NULL)
    {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    while (1)
    {
        SDL_Event ev;
        emulate(ch8);

        if (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                break;

            if (ev.type == SDL_KEYDOWN)
            {
                handle_key(ch8, ev.key.keysym.sym, 1);

                if (ev.key.keysym.sym == SDLK_ESCAPE)
                    break;
            }

            if (ev.type == SDL_KEYUP)
                handle_key(ch8, ev.key.keysym.sym, 0);

            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED)
                ch8->need_draw = 1;
        }

        if (ch8->play_beep)
        {
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0);
            ch8->play_beep = 0;
        }

        if (ch8->need_draw)
        {

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, NULL);

            SDL_SetRenderDrawColor(renderer, 30, 255, 10, 255);

            for (int y = 0; y < HEIGHT; y++)
                for (int x = 0; x < WIDTH; x++)
                {
                    int index = y * WIDTH + x;
                    char c = ch8->display_buffer[index] * 255;

                    pixels[index] = pixels[index + 1] = pixels[index + 2] = pixels[index + 3] = c;
                }

            SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
            SDL_RenderCopy(renderer, texture, NULL, NULL);

            ch8->need_draw = 0;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_CloseAudioDevice(device_id);
    SDL_FreeWAV(wav_buffer);
    SDL_DestroyTexture(texture);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}