SRC=$(shell find src/ -name '*.c') main.c
CARGS=-Wall -Wextra -Wpedantic -g -lm
COMPILE=gcc -o out/debug.bin $(SRC) $(CARGS)

build:
	gcc -o hadron $(SRC) $(CARGS)
check:
	echo ok
test:
	gcc $(CARGS) -lm -o out/test.bin tests/main.c
	./out/test.bin
gcc:
	$(COMPILE)
run:
	$(COMPILE)
	./out/debug.bin .data/index.hadron
debug:
	$(COMPILE)
	gdb -ex=r\ .data/index.hadron -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
clean:
	rm ./hadron
