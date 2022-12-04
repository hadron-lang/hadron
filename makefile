SRC=$(shell find src/ -name '*.c') main.c
CARGS=-Wall -Wextra -Wpedantic -g -lm
COMPILE=gcc -o out/debug.bin $(SRC) $(CARGS)

gcc:
	$(COMPILE)
run:
	$(gcc)
	./out/debug.bin temp/index.idk
debug:
	$(COMPILE)
	gdb -ex=r\ temp/index.idk -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
