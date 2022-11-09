SRC=$(shell find src/ -name '*.c') main.c
CARGS=-Wall -Wextra -g

gdb:
	@clear
	gcc -o out/debug.bin $(CARGS) $(SRC)
gcc:
	@clear
	gcc -o out/debug.bin $(SRC)
run:
	@clear
	gcc -o out/debug.bin $(SRC)
	@./out/debug.bin temp/index
debug:
	@clear
	gcc -o out/debug.bin $(CARGS) $(SRC)
	@gdb -ex=r\ temp/index -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
