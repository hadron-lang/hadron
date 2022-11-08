SRC=$(shell find src/ -name '*.c') main.c

gdb:
	@clear
	gcc -o out/debug.bin -Wall -g $(SRC)
gcc:
	@clear
	gcc -o out/debug.bin $(SRC)
run:
	@clear
	gcc -o out/debug.bin $(SRC)
	@./out/debug.bin temp/index
debug:
	@clear
	gcc -o out/debug.bin -Wall -g $(SRC)
	@gdb -ex=r\ temp/index -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
