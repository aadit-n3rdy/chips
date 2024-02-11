#include "chip8.h"

#include "SDL.h"
#include "SDL_surface.h"
#include "SDL_pixels.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int CHIP8_KEY[] = {SDL_SCANCODE_N, 
	SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, 
	SDL_SCANCODE_Y, SDL_SCANCODE_U, SDL_SCANCODE_I, 
	SDL_SCANCODE_H, SDL_SCANCODE_J, SDL_SCANCODE_K,
	SDL_SCANCODE_B, SDL_SCANCODE_M, 
	SDL_SCANCODE_0,
	SDL_SCANCODE_O, 
	SDL_SCANCODE_L,
	SDL_SCANCODE_COMMA};

static int chip8_stack_push(struct chip8 *chip, uint16_t addr) {
	if (chip->stack_lvl < 15) {
		chip->stack[chip->stack_lvl] = addr;
		chip->stack_lvl++;
		printf("Pushed %hx to stack, lvl is %d", addr, chip->stack_lvl);
	} else {
		printf("ERROR: Stack overflow\n");
		exit(-1);
	}
	return 0;
}

static uint16_t chip8_stack_pop(struct chip8 *chip) {
	if (chip->stack_lvl > 0) {
		chip->stack_lvl--;
		printf("Popping%hx from stack, lvl is %d\n", chip->stack[chip->stack_lvl], chip->stack_lvl);
		return chip->stack[chip->stack_lvl];
	} else {
		printf("ERROR: Stack underflow\n");
	}
	return 0;
}

inline static uint16_t chip8_load_instr(struct chip8 *chip) {
	uint16_t opcode = chip->mem[chip->pc];
	opcode = opcode << 8;
	opcode |= (chip->mem[chip->pc+1]) & 0xff;

	return opcode;
}

// Blit to pixel x, y, return 1 if overlap
static uint8_t chip8_pixbuf_blit(struct chip8 *chip, uint8_t x, uint8_t y) {
	uint8_t ret = chip->pixbuf[y][x/8] & (1<<(x%8));
	chip->pixbuf[y][x/8] ^= (1<<(x%8));
	return ret;
}

static void chip8_pixbuf_to_surf(uint8_t pixbuf[32][8], SDL_Surface *surf) {
	SDL_Rect rect;
	rect.w = 10;
	rect.h = 10;
	uint32_t col;
	uint64_t mask;
	for (int i = 0; i < 32; i++) {
		mask = 1;
		for (int j = 0; j < 64; j++, mask = mask << 1) {
			rect.x =  j * 10;
			rect.y = i * 10;
			col = (pixbuf[i][j/8] & (1<<(j%8))) ? 0xffffffff : 0;
			SDL_FillRect(surf, &rect, col);
		}
	}
}

static const uint8_t HEX_SPRITES[16][5] = {
	{0xF0, 0x90, 0x90, 0x90, 0xF0},
	{0x20, 0x60, 0x20, 0x20, 0x70},
	{0xF0, 0x10, 0xF0, 0x80, 0xF0},
	{0xF0, 0x10, 0xF0, 0x10, 0xF0},

	{0x90, 0x90, 0xF0, 0x10, 0x10},
	{0xF0, 0x80, 0xF0, 0x10, 0xF0},
	{0xF0, 0x80, 0xF0, 0x90, 0xF0},
	{0xF0, 0x10, 0x20, 0x40, 0x40},

	{0xF0, 0x90, 0xF0, 0x90, 0xF0},
	{0xF0, 0x90, 0xF0, 0x10, 0xF0},
	{0xF0, 0x90, 0xF0, 0x90, 0x90},
	{0xE0, 0x90, 0xE0, 0x90, 0xE0},

	{0xF0, 0x80, 0x80, 0x80, 0xF0},
	{0xE0, 0x90, 0x90, 0x90, 0xE0},
	{0xF0, 0x80, 0xF0, 0x80, 0xF0},
	{0xF0, 0x80, 0xF0, 0x80, 0x80}
};

int chip8_init (struct chip8 *chip) {
	memcpy(chip->mem, HEX_SPRITES, sizeof(HEX_SPRITES));
	memset(chip->pixbuf, 0, sizeof(chip->pixbuf));
	chip->pc = 0x200;
	for (int i = 0; i < 16; i++) {
		chip->reg[i] = 0;
	}
	chip->dt = 0;
	chip->st = 0;
	chip->stack_lvl = 0;
	chip->debug = 0;
	chip->state = CHIP8_STATE_NORMAL;
	for (int i = 0; i < 16; i++) {
		chip->kbstate[i] = 0;
	}
	return 0;
}

int chip8_load_rom (struct chip8 *chip, const char *fname) {
	FILE *fp = fopen(fname, "rb");
	if (!fp) {
		printf("ERROR: Could not open ROM\n");
		return -1;
	}
	short int i = 0x200;
	int buf;
	while((buf = fgetc(fp)) != EOF && i < 4096) {
		chip->mem[i] = 0xff & buf;
		i++;
	}
	printf("Mem dump: \n");
	for (i = 0x200; i < 4096; i++) {
		printf("%02hhx ", chip->mem[i]);
		if (i % 32 == 31) {
			printf("\n");
		}
	}
	printf("Read %d bytes from %s \n", i - 0x200, fname);
	return 0;
}

CHIP8_FUNC(chip8_clear_scrn) {
	SDL_FillRect(chip->surf, NULL, SDL_MapRGB(chip->surf->format, 0, 0, 0));
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 8; j++) {
			chip->pixbuf[i][j] = 0;
		}
	}
	chip->pc+=2;
	return 0;
}

CHIP8_FUNC(chip8_jmp) {
	uint16_t addr = opcode & 0xfff;
	chip->pc = addr;
	return 0;
}

CHIP8_FUNC(chip8_call) {
	chip->pc += 2;
	chip8_stack_push(chip, chip->pc);
	chip->pc = opcode & 0xfff;
	return 0;
}

CHIP8_FUNC(chip8_ret) {
	chip->pc = chip8_stack_pop(chip);
	return 0;
}

CHIP8_FUNC(chip8_skip_eq) {
	uint8_t reg = chip->reg[(opcode & 0x0f00) >> 8];
	if ((opcode & 0xff) == reg) {
		chip->pc += 4;
	} else {
		chip->pc += 2;
	}
	return 0;
}

CHIP8_FUNC(chip8_skip_neq) {
	uint8_t reg = chip->reg[(opcode & 0x0f00) >> 8];
	if ((opcode & 0xff) == reg) {
		chip->pc += 2;
	} else {
		chip->pc += 4;
	}
	return 0;
}

CHIP8_FUNC(chip8_skip_eq_reg) {
	uint8_t v1 = chip->reg[(opcode & 0x0f00) >> 8];
	uint8_t v2 = chip->reg[(opcode & 0x00f0) >> 4];
	if (v1 == v2) {
		chip->pc += 4;
	} else {
		chip->pc += 2;
	}
	return 0;
}

CHIP8_FUNC(chip8_load) {
	uint8_t i = (opcode & 0x0f00) >> 8;
	chip->reg[i] = opcode & 0xff;
	printf("Reg: %hhx Val: %hhx\n", i, chip->reg[i]);
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_add) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	chip->reg[x] += opcode & 0xff;
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_load_reg) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	chip->reg[x1] = chip->reg[x2];
	chip->pc += 2;

	return 0;

}

CHIP8_FUNC(chip8_or) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	chip->reg[x1] |= chip->reg[x2];
	chip->pc += 2 > 0;

	return 0;

}

CHIP8_FUNC(chip8_and) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	chip->reg[x1] &= chip->reg[x2];
	chip->pc += 2;

	return 0;

}

CHIP8_FUNC(chip8_xor) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;

	chip->reg[x1] ^= chip->reg[x2];
	chip->pc += 2;

	return 0;

}

CHIP8_FUNC(chip8_add_reg) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	uint16_t sum = (uint16_t)chip->reg[x1] + (uint16_t)chip->reg[x2];
	chip->reg[0xf] = (sum > 255);
	chip->reg[x1] = sum & 0xff;
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_sub_reg) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	chip->reg[0xf] = chip->reg[x1] > chip->reg[x2];
	chip->reg[x1] -= chip->reg[x2];
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_shr) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	chip->reg[0xf] = chip->reg[x1] & 1;
	chip->reg[x1] /= 2;
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_subn_reg) {
	uint8_t x1 = (opcode & 0x0f00) >> 8;
	uint8_t x2 = (opcode & 0x00f0) >> 4;
	chip->reg[0xf] = chip->reg[x2] > chip->reg[x1];
	chip->reg[x1] = chip->reg[x2] - chip->reg[x1];
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_shl) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	chip->reg[0xf] = chip->reg[x] & 0x80;
	chip->reg[x] *= 2;
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_skip_neq_reg) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	uint8_t y = (opcode & 0x00f0) >> 4;
	chip->pc += (x != y) ? 2 : 4;
	return 0;
}

CHIP8_FUNC(chip8_load_index) {
	chip->i = opcode & 0x0fff;
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_jmp_add) {
	chip->pc = chip->reg[0] + (opcode & 0x0fff);
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_rand) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	uint8_t b = (opcode & 0x00ff);
	chip->reg[x] = b & random();
	chip->pc += 2;

	return 0;
}

CHIP8_FUNC(chip8_draw) {
	uint8_t x = chip->reg[(opcode & 0x0f00) >> 8];
	uint8_t y = chip->reg[(opcode & 0x00f0) >> 4];
	uint8_t max = (opcode & 0xf);
	uint64_t cur_row;
	uint8_t arow;
	uint8_t mask;
	printf("Printing sprite at %0#hx to position %hhu, %hhu\n", chip->i, x, y);
	chip->reg[15] = 0;
	for (uint8_t row = 0; row < max; row++) {
		arow = (y+row) % 32;
		cur_row = chip->mem[chip->i + row];
		cur_row &= 0xff;
		mask = 0x80;
		for (uint8_t col = 0; col < 8; col++, mask >>= 1) {
			if (cur_row & mask) {
				chip->reg[15] = chip8_pixbuf_blit(chip, x+col, arow) || chip->reg[15];
			}
		}
	}
	chip8_pixbuf_to_surf(chip->pixbuf, chip->surf);
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_add_index) {
	printf("I:%#hx Vx:%#hhx \n", chip->i, chip->reg[(opcode & 0x0f00) >> 8]);
	chip->i += (uint16_t)(chip->reg[(opcode & 0x0f00) >> 8]);
	chip->pc += 2;
	printf("I:%#hx Vx:%#hhx \n", chip->i, chip->reg[(opcode & 0x0f00) >> 8]);
	return 0;
}

CHIP8_FUNC(chip8_load_sprite_addr) {
	uint8_t x = chip->reg[(opcode & 0x0f00) >> 8];
	chip->i = 0 + (uint16_t)x * 5;
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_bcd) {
	uint8_t val = chip->reg[(opcode & 0x0f00) >> 8];
	chip->mem[chip->i + 2] = val % 10;
	val /= 10;
	chip->mem[chip->i + 1] = val % 10;
	val /= 10;
	chip->mem[chip->i] = val%10;
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_bulk_store) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	for (uint16_t i = 0; i <= x; i++) {
		chip->mem[chip->i + i] = chip->reg[i];
	}
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_bulk_load) {
	uint8_t x = (opcode & 0x0f00) >> 8;
	for (uint16_t i = 0; i <= x; i++) {
		chip->reg[i] = chip->mem[chip->i + i];
	}
	chip->pc += 2;
	return 0;	
}

CHIP8_FUNC(chip8_skip_pressed) {
	uint8_t key = chip->reg[(opcode & 0x0f00) >> 8];
	if (chip->kbstate[key]) {
		printf("%x is pressed, skipping\n", key);
		chip->pc += 4;
	} else {
		printf("%x is not pressed, not skipping\n", key);
		chip->pc += 2;
	}
	return 0;
}

CHIP8_FUNC(chip8_skip_npressed) {
	uint8_t key = chip->reg[(opcode & 0x0f00) >> 8];
	if (chip->kbstate[key]) {
		printf("%x is pressed, not skipping\n", key);
		chip->pc += 2;
	} else {
		printf("%x is not pressed, skipping\n", key);
		chip->pc += 4;
	}
	return 0;
}

CHIP8_FUNC(chip8_wait_key) {
	chip->state = CHIP8_STATE_KEYDELAY;
	chip->delay_reg = (opcode & 0x0f00) >> 8;
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_get_dt) {
	chip->reg[(opcode & 0x0f00) >> 8] = chip->dt;
	printf("Dt value is %hhx, moved to reg %hhx", chip->dt, (unsigned char)((opcode & 0x0f00) >> 8));
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_set_dt) {
	chip->dt = chip->reg[(opcode & 0x0f00) >> 8];
	chip->last_delay = SDL_GetTicks64();
	chip->pc += 2;
	return 0;
}

CHIP8_FUNC(chip8_set_st) {
	chip->st = chip->reg[(opcode & 0x0f00) >> 8];
	chip->pc += 2;
	return 0;
}

int chip8_tick(struct chip8 *chip) {
	printf("State: ");
	switch(chip->state) {
		case CHIP8_STATE_KEYDELAY:
			printf("keydelay\n");
			break;
		case CHIP8_STATE_NORMAL:
			printf("normal\n");
			break;
		case CHIP8_STATE_DELAY:
			printf("delay\n");
			break;
	}
	if (chip->state == CHIP8_STATE_KEYDELAY) {
		printf("Waiting for a key\n");
		for (int i = 0; i <= 0xF; i++) {
			if (chip->kbstate[i]) {
				chip->state = CHIP8_STATE_NORMAL;
				chip->reg[(int)chip->delay_reg] = i;
				printf("%x is pressed, ending to reg %#hx\n", i, chip->delay_reg);
				break;
			}
		}
	} else {
		printf("i mean, it gets here\n");
		printf("Delay timer: 0x%hhx\n", chip->dt);
		if (chip->dt) {
			printf("Delay exists\n");
			uint64_t cur_ticks = SDL_GetTicks64();
			if (cur_ticks - chip->last_delay >= 17) {
				printf("Finally\n");
				chip->dt--;
				chip->last_delay = cur_ticks;
				if (chip->dt == 0) {
					printf("Timer done\n");
				}
			}
			printf("Last delay: %lu Now: %lu\n", chip->last_delay, cur_ticks);
		}

		uint16_t opcode;

		printf("PC: %#hx\n", chip->pc);

		opcode = chip8_load_instr(chip);

		printf("INSTR: %#hx\n", opcode);

		switch((opcode & 0xF000) >> 12) {
			case 0:
				switch (opcode) {
					case 0x00E0:
						chip8_clear_scrn(chip, opcode);
						break;
					case 0x00EE:
						chip8_ret(chip, opcode);
						break;
					default:
						printf("ERROR: Invalid opcode %#hx at addr %#hx\n", opcode, chip->pc);
						chip->pc += 2;
				}
				break;
			case 1:
				chip8_jmp(chip, opcode);
				break;
			case 2:
				chip8_call(chip, opcode);
				break;
			case 3:
				chip8_skip_eq(chip, opcode);
				break;
			case 4:
				chip8_skip_neq(chip, opcode);
				break;
			case 5:
				chip8_skip_eq_reg(chip, opcode);
				break;
			case 6:
				chip8_load(chip, opcode);
				break;
			case 7:
				chip8_add(chip, opcode);
				break;
			case 8:
				switch (opcode & 0xf) {
					case 0:
						chip8_load_reg(chip, opcode);
						break;
					case 1:
						chip8_or(chip, opcode);
						break;
					case 2:
						chip8_and(chip, opcode);
						break;
					case 3:
						chip8_xor(chip, opcode);
						break;
					case 4:
						chip8_add_reg(chip, opcode);
						break;
					case 5:
						chip8_sub_reg(chip, opcode);
						break;
					case 6:
						chip8_shr(chip, opcode);
						break;
					case 7:
						chip8_subn_reg(chip, opcode);
						break;
					case 0xe:
						chip8_shl(chip, opcode);
						break;
					default:
						printf("ERROR: Invalid opcode %#hx at addr %#hx\n", opcode, chip->pc);
						chip->pc += 2;
				}
				break;
			case 9:
				chip8_skip_neq_reg(chip, opcode);
				break;
			case 0xA:
				chip8_load_index(chip, opcode);
				break;
			case 0xB:
				chip8_jmp_add(chip, opcode);
				break;
			case 0xC:
				chip8_rand(chip, opcode);
				break;
			case 0xD:
				chip8_draw(chip, opcode);
				break;
			case 0xE:
				switch(opcode & 0xff) {
					case 0x9E:
						chip8_skip_pressed(chip, opcode);
						break;
					case 0xA1:
						chip8_skip_npressed(chip, opcode);
						break;
					default:
						printf("Instruction %#hx unimplemented, incrementing \n", opcode);
						chip->pc += 2;
				}
				break;
			case 0xF:
				switch(opcode & 0xff) {
					case 0x07:
						chip8_get_dt(chip, opcode);
						break;
					case 0x0A:
						chip8_wait_key(chip, opcode);
						break;
					case 0x15:
						chip8_set_dt(chip, opcode);
						break;
					case 0x18:
						printf("Instruction %#hx unimplemented, incrementing \n", opcode);
						chip->pc += 2;
						break;
					case 0x1E:
						chip8_add_index(chip, opcode);
						break;
					case 0x29:
						chip8_load_sprite_addr(chip, opcode);
						break;
					case 0x33:
						chip8_bcd(chip, opcode);
						break;
					case 0x55:
						chip8_bulk_store(chip, opcode);
						break;
					case 0x65:
						chip8_bulk_load(chip, opcode);
						break;
					default:
						printf("Invalid opcode %#hx\n", opcode);
						chip->pc += 2;
				}
				break;
			default:
				printf("Invalid or unimplemented opcode %#hx\n", opcode);
				chip->pc += 2;
		}
	}
	printf("Dumping display\n");
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 64; j++) {
			printf("%hhx", (chip->pixbuf[i][j/8] & (1<<(j&7))) > 0);
		}
		printf("\n");
	}
	return 0;
}
