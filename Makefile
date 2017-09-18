all:y86emul y86dis

y86emul:
	gcc -Wall -lm y86emul.c -o y86emul
y86dis:
	gcc -Wall -lm y86dis.c -o y86dis
clean:			
	rm y86emul y86dis

