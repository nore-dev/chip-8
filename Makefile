CC = gcc
TARGET = chip8_emu
LIBS = -lSDL2

ifeq ($(OS), Windows_NT)
	LIBS += -lmingw32 -lSDL2main
endif

all: compile

compile:
	$(CC) main.c chip8.c -I include -L lib $(LIBS) -o $(TARGET) -Wall -Wextra

.PHONY: all compile