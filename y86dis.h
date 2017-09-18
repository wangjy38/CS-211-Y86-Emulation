#ifndef Y86EMUL_H_
#define Y86EMUL_H_
#include <math.h>
typedef enum {

	AOK, /*Everything is fine; no errors*/
	HLT, /*Halt instruction has occurred; program ends normally*/
	INS,
        ADR
	} ProgramStatus;
union converter {
	char byte[4];
	int integer; 
};
char * append (char * str, char c);
int hextodec(char * num);
char * hextobin(char c) ;
void executeprog();
int bintodec(char * num);
char * copy (char * str);
int gettwobytes(char * str, int position);
void printmemory (int size);
void printstatus ();
void getargs(unsigned char * arg1, unsigned char * arg2);
#endif 
