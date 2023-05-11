#ifndef CHIPS_CHIP8_H
#define CHIPS_CHIP8_H

#include "SDL.h"
#include "SDL_surface.h"

#include <stdint.h>

#define CHIP8_FUNC(FUNC) int FUNC (struct chip8 *chip, short int opcode)

#define CHIP8_STATE_NORMAL	0
#define CHIP8_STATE_DELAY	1
#define CHIP8_STATE_KEYDELAY	2

extern int CHIP8_KEY[16];

struct chip8 {
	char mem[4096];
	char reg[16];
	short int i;
	char dt;
	char st;

	char state;
	char delay_reg;
	uint64_t last_delay;

	uint8_t kbstate[16];

	short int pc;

	short int stack[12];
	int stack_lvl;

	uint64_t pixbuf[32]; // 0th byte: pix0, and so on

	char debug;

	SDL_Surface *surf;
};

int chip8_init(struct chip8 *chip);
int chip8_load_rom (struct chip8 *chip, const char *fname);
int chip8_tick(struct chip8 *chip);

#endif // Header guard
