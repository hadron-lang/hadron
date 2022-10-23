gdb:
	@clear
	@gcc -o debug.bin -ggdb main.c
gcc:
	@clear
	@gcc -o debug.bin main.c
run:
	@clear
	@gcc -o debug.bin main.c
	@./debug.bin index.idk
debug:
	@clear
	@gcc -o debug.bin -ggdb main.c
	@gdb -ex=run\ index.idk -ex=quit --silent ./debug.bin
clean:
	@clear
	@rm debug.bin
