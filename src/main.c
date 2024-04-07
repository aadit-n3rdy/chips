#include "SDL.h"
#include "SDL_video.h"
#include "SDL_surface.h"
#include "SDL_events.h"

#include <stdio.h>
#include <stdint.h>

#include "chip8.h"

#define WIN_WIDTH      	640
#define WIN_HEIGHT    	320
#define WIN_MULTIPLIER	10

int main(int argc, char **argv) {
	SDL_Window *win;
	SDL_Surface *surf;
	SDL_Event event;
	char quit = 0;
	char debug = 0;

	float freq = 50000;

	struct chip8 chip;

	win = SDL_CreateWindow("CHIPS",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			WIN_WIDTH,
			WIN_HEIGHT,
			0);
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)) {
		printf("ERROR: Could not init SDL2\n");
		exit(-1);
	}

	surf = SDL_GetWindowSurface(win);

	chip.surf = surf;

	chip8_init(&chip);

	chip8_load_rom(&chip, (argc >= 2) ? argv[1] : "roms/emu_logo.ch8");

	if (argc >= 3) {
		freq = atoi(argv[2]);
	}

	printf("Loaded ROM\n");
	char b;

	uint64_t start_timer, end_timer;
	float ms;

	while (!quit) {
		start_timer = SDL_GetPerformanceCounter();
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = 1;
				break;
			} else if (event.type == SDL_KEYDOWN) {
				switch(event.key.keysym.scancode) {
					case SDL_SCANCODE_UP:
						freq *= 3.0f;
						break;
					case SDL_SCANCODE_DOWN:
						freq /= 3.0f;
						break;
					default:
						for (int i = 0; i < 16; i++) {
							if (event.key.keysym.scancode == CHIP8_KEY[i]) {
								chip.kbstate[i] = 1;
								printf("Pressed %x in SDL\n", i);
							}
						}
						break;
				}
			} else if (event.type == SDL_KEYUP) {
				for (int i = 0; i < 16; i++) {
					if (event.key.keysym.scancode == CHIP8_KEY[i]) {
						chip.kbstate[i] = 0;
						printf("Released %x in SDL\n", i);
					}
				}
				break;

			}
		}

		chip8_tick(&chip);

		if (debug) {
			scanf(" %c", &b); switch (b) {
				case 'n':
					break;
				case 'c':
					debug = 0;
					break;
				case 'p':
					printf("I: %#hx\n", chip.i);
					printf("Registers: \n");
					for (int i = 0; i < 16; i++) {
						printf("%#hhx\n", chip.reg[i]);
					}
					break;
				case 'q':
				default:
					quit = 1;
			}
		}
		
		SDL_UpdateWindowSurface(win);

		end_timer = SDL_GetPerformanceCounter();

		ms = (end_timer - start_timer) / (float)SDL_GetPerformanceFrequency();


		SDL_Delay(1000.0f/freq - ms);
	}

	SDL_DestroyWindow(win);

	SDL_Quit();
	return 0;
}
