SRC=$(shell find src/ -name '*.c') main.c
CARGS=-Wall -Wextra -g -lm
COMPILE=gcc -o out/debug.bin $(SRC) $(CARGS)

gcc:
	@clear
	$(COMPILE)
run:
	@clear
	$(COMPILE)
	@./out/debug.bin temp/index.idk
debug:
	@clear
	$(COMPILE)
	@gdb -ex=r\ temp/index.idk -ex=set\ confirm\ off -ex=q --silent ./out/debug.bin
