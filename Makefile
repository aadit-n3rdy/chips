PROJ:=chips

CC:=clang
DEPS:=sdl2
DEFS:=
CFLAGS:=-Wall -Iinclude $(foreach DEP,$(DEPS),$(shell pkg-config $(DEP) --cflags --shared)) $(foreach DEF,$(DEFS), -D$(DEF))
LINK := $(foreach DEP,$(DEPS),$(shell pkg-config $(DEP) --libs --shared)) -lm


SRC:=$(wildcard src/*.c)
OBJ:=$(patsubst src/%.c, obj/%.o, $(SRC))

OUT:=bin/$(PROJ)

.DEFAULT_GOAL :=  all
.PHONY: clean all test $(PROJ)

all: $(OUT) # test

$(PROJ): $(OUT)

clean:
	-- rm $(OBJ)
	-- rm $(OUT)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINK)

trial.ch8: trial.raw
	xxd -r -p trial.raw trial.ch8
