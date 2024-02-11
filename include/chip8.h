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
	uint8_t mem[4096];
	uint8_t reg[16];
	uint16_t i;
	uint8_t dt;
	uint8_t st;

	uint8_t state;
	uint8_t delay_reg;
	uint64_t last_delay;

	uint8_t kbstate[16];

	uint16_t pc;

	uint16_t stack[12];
	int32_t stack_lvl;

	uint8_t pixbuf[32][8]; // 0th byte: pix0, and so on

	char debug;

	SDL_Surface *surf;
};

int chip8_init(struct chip8 *chip);
int chip8_load_rom (struct chip8 *chip, const char *fname);
int chip8_tick(struct chip8 *chip);

#endif // Header guard
