SRC=$(shell find src/ -name '*.c') main.c
CARGS=-Wall -Wextra -Wpedantic -g -lm
COMPILE=gcc -o out/debug.bin $(SRC) $(CARGS)

test:
	gcc -Wall -Wextra -Wpedantic -g -lm -lcriterion -o out/test.bin tests/test.c
	./out/test.bin
gcc:
	$(COMPILE)
run:
	$(COMPILE)
	./out/debug.bin tests/index.hadron
debug:
	$(COMPILE)
	gdb -ex=r\ tests/index.hadron -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
