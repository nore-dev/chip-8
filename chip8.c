#include "chip8.h"

struct Chip8 *create_chip8_emulator()
{
    struct Chip8 *ch8 = malloc(sizeof(struct Chip8));

    // Random Seed
    srand(time(NULL));

    ch8->program_counter = PROGRAM_START_POS;
    ch8->need_draw = 1;

    // Ctrl C + Ctrl V
    unsigned char chip8_font[80] =
        {
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

    memcpy(&ch8->memory, &chip8_font, 80);

    return ch8;
}

void load_game(struct Chip8 *ch8, const char *game_name)
{
    FILE *file = fopen(game_name, "rb");

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size > MEMORY_SIZE - PROGRAM_START_POS)
    {
        printf("Too big for memory");
        exit(EXIT_FAILURE);
    }

    char *buffer = (char *)malloc(file_size);

    fread(buffer, 1, file_size, file);

    memcpy(&ch8->memory[PROGRAM_START_POS], buffer, file_size);

    free(buffer);
    fclose(file);
}

void emulate(struct Chip8 *ch8)
{
    // Merge 2 bytes
    unsigned short op_code = ch8->memory[ch8->program_counter] << 8 | ch8->memory[ch8->program_counter + 1];

    unsigned short nnn = op_code & 0xFFF;
    unsigned short nn = op_code & 0xFF;
    unsigned short n = op_code & 0xF;

    unsigned short x = (op_code & 0x0F00) >> 8;
    unsigned short y = (op_code & 0x00F0) >> 4;

    int key_pressed = 0;

    switch (op_code & 0xF000)
    {

    case 0x0000:
        switch (nn)
        {
        case 0xE0: // Clears the screen
            memset(ch8->display_buffer, 0, WIDTH * HEIGHT);
            ch8->need_draw = 1;
            break;

        case 0xEE: // Returns from subroutine
            ch8->program_counter = ch8->stack[ch8->stack_pointer--];
            break;

        default:
            goto UNKNOWN_OP_CODE;
        }
        break;

    case 0x1000:
        ch8->program_counter = nnn;
        return;

    case 0x2000: // Calls subroutine at NNN
        ch8->stack[++ch8->stack_pointer] = ch8->program_counter;
        ch8->program_counter = nnn;
        return;

    case 0x3000: // Skip next instruction if Vx = kk.
        if (ch8->variables[x] == nn)
            ch8->program_counter += 2;
        break;

    case 0x4000: // Skip next instruction if Vx != kk.
        if (ch8->variables[x] != nn)
            ch8->program_counter += 2;
        break;

    case 0x5000: // Skip next instruction if Vx = Vy.
        if (ch8->variables[x] == ch8->variables[y])
            ch8->program_counter += 2;
        break;

    case 0x6000: //Set Vx = kk.
        ch8->variables[x] = nn;
        break;

    case 0x7000: // Set Vx = Vx + kk.
        ch8->variables[x] += nn;
        break;

    case 0x8000:
        switch (n)
        {
        case 0x0: // Set Vx = Vy.
            ch8->variables[x] = ch8->variables[y];
            break;
        case 0x1: // Set Vx OR Vy.
            ch8->variables[x] |= ch8->variables[y];
            break;
        case 0x2: // Set Vx AND Vy.
            ch8->variables[x] &= ch8->variables[y];
            break;
        case 0x3: // Set Vx XOR Vy.
            ch8->variables[x] ^= ch8->variables[y];
            break;
        case 0x4: // Set Vx = Vx + Vy, set VF = carry.
            ch8->variables[0xF] = ch8->variables[y] + ch8->variables[x] > 255;
            ch8->variables[x] += ch8->variables[y];
            break;
        case 0x5: // Set Vx = Vx - Vy, set VF = NOT borrow.
            ch8->variables[0xF] = ch8->variables[x] > ch8->variables[y];
            ch8->variables[x] -= ch8->variables[y];
            break;
        case 0x6:
            ch8->variables[0xF] = ch8->variables[x] & 0x1;
            ch8->variables[x] >>= 1;
            break;
        case 0x7: // Set Vx = Vy - Vx, set VF = NOT borrow.
            ch8->variables[0xF] = ch8->variables[y] > ch8->variables[x];
            ch8->variables[x] = ch8->variables[y] - ch8->variables[x];
            break;
        case 0xE:
            ch8->variables[0xF] = ch8->variables[x] >> 7;
            ch8->variables[x] <<= 1;
            break;
        default:
            goto UNKNOWN_OP_CODE;
        }
        break;

    case 0x9000: // Skip next instruction if Vx != Vy.
        if (ch8->variables[x] != ch8->variables[y])
            ch8->program_counter += 2;
        break;

    case 0xA000: // Set index to NNN
        ch8->index = nnn;
        break;

    case 0xB000: // Jumps to the address NNN plus Varibles[0]
        ch8->program_counter = ch8->variables[0] + nnn;
        return;

    case 0xC000: // Sets VX to the result of a bitwise and operation on a random number
        ch8->variables[x] = (int)(rand() % 0xFF) & nn;
        break;

    case 0xD000: // Draw

        for (int Y = 0; Y < n; Y++)
        {
            unsigned short pixel = ch8->memory[ch8->index + Y];
            for (int X = 0; X < 8; X++)
            {
                unsigned int index = ((ch8->variables[y] + Y) * WIDTH) + ch8->variables[x] + X;

                if ((pixel & (0x80 >> X)) != 0) // >:(
                {
                    ch8->variables[0xF] = ch8->display_buffer[index];
                    ch8->display_buffer[index] ^= 1;
                }
            }
        }

        ch8->need_draw = 1;
        break;

    case 0xE000:
        switch (nn)
        {
        case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed
            if (ch8->keys[ch8->variables[x]] == PRESSED)
                ch8->program_counter += 2;
            break;

        case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed
            if (ch8->keys[ch8->variables[x]] != PRESSED)
                ch8->program_counter += 2;
            break;

        default:
            goto UNKNOWN_OP_CODE;
        }
        break;

    case 0xF000:
        switch (nn)
        {
        case 0x07:
            ch8->variables[x] = ch8->delay_timer;
            break;

        case 0x0A:
            for (int i = 0; i < 16; ++i)
            {
                if (ch8->keys[i] != 0)
                {
                    ch8->variables[x] = i;
                    key_pressed = 1;
                }
            }

            if (!key_pressed)
                return;
            break;

        case 0x15:
            ch8->delay_timer = ch8->variables[x];
            break;

        case 0x18:
            ch8->sound_timer = ch8->variables[x];
            break;

        case 0x1E:
            ch8->variables[0xF] = (ch8->index + ch8->variables[x] > 0xFFF);
            ch8->index += ch8->variables[x];
            break;

        case 0x29:
            ch8->index = ch8->variables[x] * SPRITE_SIZE;
            break;

        case 0x33:
            ch8->memory[ch8->index] = ch8->variables[x] / 100;
            ch8->memory[ch8->index + 1] = (ch8->variables[x] / 10) % 10;
            ch8->memory[ch8->index + 2] = (ch8->variables[x] % 100) % 10;
            break;

        case 0x55:
            for (int i = 0; i <= x; i++)
                ch8->memory[ch8->index + i] = ch8->variables[i];
            ch8->index += x + 1;
            break;

        case 0x65:
            for (int i = 0; i <= x; i++)
                ch8->variables[i] = ch8->memory[ch8->index + i];
            ch8->index += x + 1;
            break;
        }
        break;

    default:
        goto UNKNOWN_OP_CODE;
    }

    if (ch8->delay_timer > 0)
        ch8->delay_timer--;

    if (ch8->sound_timer > 0)
    {
        if (ch8->sound_timer == 1)
            ch8->play_beep = 1;
        ch8->sound_timer--;
    }

    ch8->program_counter += 2;
    return;

UNKNOWN_OP_CODE:
    printf("Unknown Op Code: %#.8x", op_code);
    exit(EXIT_FAILURE);
}

void handle_key(struct Chip8 *ch8, SDL_Keycode key, int is_down)
{
    int address = 0x1F;

    // Hmmmmmm
    if (key == SDLK_1)
        address = 0x1;
    if (key == SDLK_2)
        address = 0x2;
    if (key == SDLK_3)
        address = 0x3;
    if (key == SDLK_4)
        address = 0xC;

    if (key == SDLK_q)
        address = 0x4;
    if (key == SDLK_w)
        address = 0x5;
    if (key == SDLK_e)
        address = 0x6;
    if (key == SDLK_r)
        address = 0xD;

    if (key == SDLK_a)
        address = 0x7;
    if (key == SDLK_s)
        address = 0x8;
    if (key == SDLK_d)
        address = 0x9;
    if (key == SDLK_f)
        address = 0xE;

    if (key == SDLK_z)
        address = 0xA;
    if (key == SDLK_x)
        address = 0x0;
    if (key == SDLK_c)
        address = 0xB;
    if (key == SDLK_v)
        address = 0xF;

    if (address != 0x1F)
        ch8->keys[address] = is_down;
}