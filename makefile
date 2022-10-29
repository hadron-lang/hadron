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
	@gdb -ex=r\ index.idk -ex=set\ confirm\ off -ex=q --silent ./debug.bin
clean:
	@clear
	@rm debug.bin
