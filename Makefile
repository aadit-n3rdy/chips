PROJ:=chips

CC:=clang
DEPS:=sdl2
DEFS:=
CFLAGS:=-Wall -Iinclude $(foreach DEP,$(DEPS),$(shell pkg-config $(DEP) --cflags --shared)) $(foreach DEF,$(DEFS), -D$(DEF))
LINK := $(foreach DEP,$(DEPS),$(shell pkg-config $(DEP) --libs --shared)) -lm


SRC:=$(wildcard src/*.c)
OBJ:=$(patsubst src/%.c, obj/%.o, $(SRC))

ROMS_SRC:=$(wildcard rom_src/*.raw)
ROMS:=$(patsubst rom_src/%.raw, roms/%.ch8, $(ROMS_SRC))

OUT:=bin/$(PROJ)

.DEFAULT_GOAL :=  all
.PHONY: clean all test $(PROJ)

all: $(OUT) $(ROMS)

$(PROJ): $(OUT)

clean:
	-- rm $(OBJ)
	-- rm $(OUT)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINK)

roms/%.ch8: rom_src/%.raw
	xxd -r -p $@ $<
