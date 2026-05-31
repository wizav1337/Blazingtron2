# Makefile for Blazingtron 2 (MinGW + NASM)
# Usage (MSYS2/MinGW): make
# Cross from Linux: x86_64-w64-mingw32-gcc + nasm

CC      := gcc
NASM    := nasm
WINDRES := windres

CFLAGS  := -mwindows -municode -Wall -O2 -s
LIBS    := -luser32 -lgdi32 -lcomctl32 -lkernel32

TARGET  := Blazingtron2.exe
ASMOBJ  := calc.obj
RESOBJ  := resource.o

all: $(TARGET)

$(ASMOBJ): src/calc.asm
	$(NASM) -f win64 $< -o $@

$(RESOBJ): res/resource.rc res/blazingtron.manifest
	$(WINDRES) -i $< -o $@ || echo "Warning: windres failed, building without resources"

$(TARGET): src/main.c $(ASMOBJ) $(RESOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo Built $(TARGET)

clean:
	rm -f $(TARGET) $(ASMOBJ) $(RESOBJ)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
