#include <string.h>
#include <stdio.h>
//#include <malloc.h>
#include "y86emul.h"
#include <math.h>
#include <stdlib.h>

int reg[8];
//Registers to be used by the program

int programCounter;

//Program counter

unsigned char * memspace;

//Blocks of memory 

 int memsize;

 // Stores the length of memory


int OF, ZF, SF;

//Three Flags

ProgramStatus status = AOK;

//Current status of program execution.
 

int main (int argc, char ** argv)
{
//	Checks to see if number of arguements is correct
	
	if (argc <= 1)
	{
		printf("ERROR: Not enough input arguements.\n");
		return 0;
	}
	
	
	char helptest[] = "-h\0";
	char * input = (char *)malloc(sizeof(argv[1]));
	strcpy(input, argv[1]);
	
	if (strcmp(input, helptest) == 0)
	{
		printf("This emulator can be used to run programs written in Y86 instructions.\n");
		printf("Usage: \n");
		printf("./y86emul <y86 file name>\n");
		free(input);
		return 0;
	}
	
	
	if (strlen(input) < 5)
	{
		printf("ERROR: Invalid input file: %s\n", input);	
		free(input);
		return 0;
	}
	
	int i = 0;
	int j = 0;

	
	for (; input[i] != '\0'; i++)
	{
		if (input[i] == '.')
		{
			break;
		}
	}
	
	char fextention[] = ".y86";
	char * temp = &input[i];
	
	if (strcmp(temp, fextention) != 0)
	{
		printf("ERROR: Invalid file extension: %s\n", temp);
		
		return 0;
	}


	char mode[] = "r\0";
	FILE * inputfile = fopen(input, mode);
	if (inputfile == NULL)
	{
		printf("ERROR: File not found: %s\n", input);
		
		return 0;
	}

	char c;
	char * prog = (char *) calloc(1, sizeof(char));
	prog[0] = '\0';
	
	do
	{
		c = fgetc(inputfile);
		
		if (feof(inputfile))
		{
			break;
		}
		
		prog = append(prog, c);
		
	} while(1);

	fclose(inputfile);
	
	

	char * duplicate = copy(prog);
	char * token = strtok(duplicate,"\n\t\r");
	char * memory;
	int count = 0;
	
	do
	{
		if (strcmp(token, ".size") == 0)
		{
			if (count == 0)
			{	
				count++;
			}
			else
			{
				printf("ERROR:\n\t More than one .size directive has been detected. \n");
				printf("\t Please make sure that the file has exactly one .size directive \n");
				return 0;
			}
			memory = strtok(NULL, "\n\t\r");	
		}
		token = strtok(NULL, "\n\t\r");		
		
		if (token == NULL)
		{
			break;				
		}
	} while (1);			

	if (count == 0)				
	{
		printf("ERROR:\n\t No .size directive was detected in the .y86 file. \n\t Please make sure that the file has exactly one .size directive\n");
		return 0;	
	}
	
	
	int size = hextodec(memory);
	
	memsize = size;
	memspace = (unsigned char *) malloc((size) * sizeof(unsigned char));
	
	programCounter = -1;	
	
	for (i = 0; i <size; i++)
	{
		memspace[i] = 0;
	}
	
	// Begin looking for the other directives
	free(duplicate);
	
	duplicate = copy(prog);
	token = strtok(duplicate, "\n\t\r");
	
	char * arg;		//	Arguement of the directive
	char * address;		//	Location in memory where the arg is to be stored
	
	int ai = 0;		//	AddressIndex dec representation of hex address in memory
	
	do
	{
		if (strcmp(token, ".size") == 0)
		{
				
			token = strtok(NULL, "\n\t\r");
		}
		else if (strcmp(token, ".text") == 0)
		{
			
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			ai = hextodec(address);
			
			if (programCounter == -1)
			{
				programCounter = ai;
			}
			else if (programCounter != -1)
			{
				printf("ERROR: More than one .text directive detected\n");
				return 0;
			}
			
			j = 0;
			
			while (j < strlen(arg))
			{
				memspace[ai] = (unsigned char) gettwobytes(arg, j);
				ai++;
				j += 2;
			}
			
			free(address);
			free(arg);	
		}
		else if (strcmp(token, ".byte") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			memspace[hextodec(address)] = (unsigned char) hextodec(arg);
			
			free(address);
			free(arg);
		}
		else if (strcmp(token, ".long") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			union converter Converter;
			Converter.integer = atoi(arg);
			
			for(i = 0; i < 4; i++)
			{
				memspace[i+hextodec(address)] = Converter.byte[i];
			}
			
			free(address);
			free(arg);
		}
		else if (strcmp(token, ".string") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			int len = strlen(arg);
			
			i = hextodec(address);
			
			for(j = 1; j < len - 1; j++)
			{
				memspace[i] = (unsigned char)arg[j];
				i++;
			}
			
			free(arg);
			free(address);
		}
		
		else if (token[0] == '.')
		{
			status = INS;
		}
		token = strtok(NULL, "\n\t");
		if (token == NULL)
		{
			break;
		}
	} while (1);
	//printmemory(memsize);
	free(duplicate);
	
	if (status == INS)
	{
		printf("ERROR: Invalid directive encountered: %s\n", token);
		return 0;
	}
	
	
	executeprog();
	

	printstatus();

	free(memspace);
	free(input);	
	free(prog);	
	return 0;	
}

void executeprog()
{
	unsigned char arg1;
	unsigned char arg2;
	
	int offset;			// Used for any integer operations

	int num1, num2;		// Used in addl, subl, mull, 

	union converter Converter;// Used to convert between unsigned chars and ints

	status = AOK;

	int badscan;

	char inputchar; 	// For read/write b
	int inputword;		// For read/write w

	// Initialize all registers to 0
	reg[7] = reg[6] = reg[5] = reg[4] = reg[3] = reg[2] = reg[1] = reg[0] = 0;

	// Intialize all flags to 0
	OF = ZF = SF = 0;


        //printf("%d",programCounter);
	do
	{       
                
                //printf("%c", memspace[programCounter]);
                //printf("%c", ' ');
		switch (memspace[programCounter])
		{
			
                        // 00 NOP
			case 0x00:
				
				programCounter++;	
				


			// 10 HALT
			case 0x10:
				
				status = HLT;	
				
			
                        break;

			// 20 RRMOVL srcR desR
			case 0x20:
				
				getargs(&arg1, &arg2);
				
				reg[arg2] = reg[arg1];					
				
				programCounter += 2;

			break;

			// 30 IRMOVL notR desR offset
			case 0x30:
				
				getargs(&arg1, &arg2);
				
				if (arg1 < 0x08)
				{
					status = ADR;
					printf("ERROR: IRMOVL instruction has two addresses. Memory Location: %x\n", programCounter);
					break;
				}
				
				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			
				
				offset = Converter.integer;					
				
				reg[arg2] = offset;						
				
				programCounter += 6;

			break;

			// 40 RMMOVL
			case 0x40:
				
				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			
				
				offset = Converter.integer;					

				Converter.integer = reg[arg1];

				if ((offset + reg[arg2] + 3) > memsize)
				{
					status = ADR;
					printf("ERROR: RMMOVL instruction address offset larger than memory space. Memory Location: %x\n", programCounter);
				}

				memspace[offset + reg[arg2] + 0] = Converter.byte[0];	
				memspace[offset + reg[arg2] + 1] = Converter.byte[1];	
				memspace[offset + reg[arg2] + 2] = Converter.byte[2];	
				memspace[offset + reg[arg2] + 3] = Converter.byte[3];	

				programCounter += 6;

			break;

			// 50 MRMOVL 
			case 0x50:

				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			
				
				offset = Converter.integer;					

				if ((offset + reg[arg2] + 3) >=memsize)
				{
					status = ADR;
					printf("ERROR: MRMOVL instruction address offset larger than memory space. Memory Location: %x\n", programCounter);
				}

				Converter.byte[0] = memspace[offset + reg[arg2] + 0];	
				Converter.byte[1] = memspace[offset + reg[arg2] + 1];	
				Converter.byte[2] = memspace[offset + reg[arg2] + 2];	
				Converter.byte[3] = memspace[offset + reg[arg2] + 3];	

				reg[arg1] = Converter.integer;

				programCounter += 6;

			break;

			// ADDL 
			case 0x60:
				
				
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num1 + num2;

				if (offset == 0)
				{
					ZF = 1;
				}else {
                                ZF = 0;
                                }

				if (offset < 0)
				{
					SF = 1;
				}else 
                                {
                                SF = 0;
                                 }

				if ((offset > 0 && num1 < 0 && num2 < 0) || (offset < 0 && num1 > 0 && num2 > 0))
				{
					OF = 1;
				} else 
                                {
                                 OF = 0; 
                                 }

				reg[arg2] = offset;

				programCounter += 2;

			break;

			// 61 SUBL 
			case 0x61:
				
				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num2 - num1;

				if (offset == 0)
				{
					ZF = 1;
				}

				if (offset < 0)
				{
					SF = 1;
				}

				if ((offset > 0 && num1 > 0 && num2 < 0) || (offset < 0 && num1 < 0 && num2 > 0))
				{
					OF = 1;
				}
	
				reg[arg2] = offset;

				programCounter += 2;

			break;

			// 62 ANDL 
			case 0x62:
				
				SF = 0;
				ZF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num1 & num2;

				reg[arg2] = offset;

				if (offset == 0)
				{
					ZF = 1;
				}

				if (offset < 0)
				{
					SF = 1;
				}

				programCounter += 2;

			break;

			// 63 XORL 
			case 0x63:
				
				ZF = 0;
				SF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num1 ^ num2;

				reg[arg2] = offset;

				if (offset == 0)
				{
					ZF = 1;
				}

				if (offset < 0)
				{
					SF = 1;
				}

				programCounter += 2;

			break;

			// 64 MULL srcR desR
			case 0x64:

				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num1 * num2;

				if (offset == 0)
				{
					ZF = 1;
				} 

				if (offset < 0)
				{
					SF = 1;
				}

				if ((offset < 0 && num1 < 0 && num2 < 0) || 
					(offset < 0 && num1 > 0 && num2 > 0) || 
					(offset > 0 && num1 < 0 && num2 > 0) || 
					(offset > 0 && num1 > 0 && num2 < 0))
				{
					OF = 1;
				}

				reg[arg2] = offset;

				programCounter += 2;

			break;

			// 65 CMPL
			case 0x65:

				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				offset = num2 - num1;
                                //printf ("\n %d %d \n",num1, num2);
				if (offset == 0)
				{
					ZF = 1;
				}

				if (offset < 0)
				{
					SF = 1;
				}

				if ((offset > 0 && num1 > 0 && num2 < 0) || (offset < 0 && num1 < 0 && num2 > 0))
				{
					OF = 1;
				}

				programCounter += 2;
			break;
			
			// 70 JMP 32bit destination
			case 0x70:
				// Unconditional Jump
				Converter.byte[0] = memspace[programCounter + 1];			
				Converter.byte[1] = memspace[programCounter + 2];			
				Converter.byte[2] = memspace[programCounter + 3];			
				Converter.byte[3] = memspace[programCounter + 4];			
				
				offset = Converter.integer;					
			
				programCounter = offset;

			break;

			// 71 JLE 32bit destination
			case 0x71:
				// Jump if less than or equal to
				Converter.byte[0] = memspace[programCounter + 1];			
				Converter.byte[1] = memspace[programCounter + 2];			
				Converter.byte[2] = memspace[programCounter + 3];			
				Converter.byte[3] = memspace[programCounter + 4];			
				
				offset = Converter.integer;					

				if (ZF == 1 || (SF ^ OF))
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}

			break;

			// 72 JL  32bit destination
			case 0x72:
				// Jump if strictly less than
				Converter.byte[0] = memspace[programCounter + 1];			// Getting the offset bytes in
				Converter.byte[1] = memspace[programCounter + 2];			// little endian order
				Converter.byte[2] = memspace[programCounter + 3];			//
				Converter.byte[3] = memspace[programCounter + 4];			//
				
				offset = Converter.integer;					// This is the offset amount

				if (ZF == 0 && (SF ^ OF))
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}

			break;

			// 73 JE  32bit destination
			case 0x73:
				// Jump if equal
				Converter.byte[0] = memspace[programCounter + 1];			// Getting the offset bytes in
				Converter.byte[1] = memspace[programCounter + 2];			// little endian order
				Converter.byte[2] = memspace[programCounter + 3];			//
				Converter.byte[3] = memspace[programCounter + 4];			//
				
				offset = Converter.integer;					// This is the offset amount

				if (ZF == 1)
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}

			break;

			// 74 JNE 32bit destination
			case 0x74:
				// Jump if not equal
				Converter.byte[0] = memspace[programCounter + 1];			// Getting the offset bytes in
				Converter.byte[1] = memspace[programCounter + 2];			// little endian order
				Converter.byte[2] = memspace[programCounter + 3];			//
				Converter.byte[3] = memspace[programCounter + 4];			//
				
				offset = Converter.integer;					// This is the offset amouny

				if (ZF == 0)
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}
				
			break;

			// 75 JGE 32bit destination
			case 0x75:
			// Jump if greater than or equal to
				Converter.byte[0] = memspace[programCounter + 1];			// Getting the offset bytes in
				Converter.byte[1] = memspace[programCounter + 2];			// little endian order
				Converter.byte[2] = memspace[programCounter + 3];			//
				Converter.byte[3] = memspace[programCounter + 4];			//
				
				offset = Converter.integer;					// This is the offset amount

				if (!(ZF == 0 && (SF ^ OF)))
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}

			break;

			// 76 JG  32bit destination
			case 0x76:
			// Jump if strictly greater than
				Converter.byte[0] = memspace[programCounter + 1];			// Getting the offset bytes in
				Converter.byte[1] = memspace[programCounter + 2];			// little endian order
				Converter.byte[2] = memspace[programCounter + 3];			//
				Converter.byte[3] = memspace[programCounter + 4];			//
				
				offset = Converter.integer;					// This is the offset amount

				if (!(ZF == 1 || (SF ^ OF)))
				{
					programCounter = offset;
				}
				else
				{
					programCounter += 5;
				}

			break;

			
			case 0x80:


				Converter.byte[0] = memspace[programCounter + 1];			
				Converter.byte[1] = memspace[programCounter + 2];			
				Converter.byte[2] = memspace[programCounter + 3];			
				Converter.byte[3] = memspace[programCounter + 4];			
				
				offset = Converter.integer;					
				
				reg[4] -= 4;							
				
				Converter.integer = programCounter + 5;

				memspace[reg[4] + 0] = Converter.byte[0];	
				memspace[reg[4] + 1] = Converter.byte[1];	
				memspace[reg[4] + 2] = Converter.byte[2];	
				memspace[reg[4] + 3] = Converter.byte[3];	

				programCounter = offset;

			break;

			
			case 0x90:
			
				Converter.byte[0] = memspace[reg[4] + 0];	
				Converter.byte[1] = memspace[reg[4] + 1];	
				Converter.byte[2] = memspace[reg[4] + 2];	
				Converter.byte[3] = memspace[reg[4] + 3]; 

				programCounter = Converter.integer;

				reg[4] += 4;

			break;

			
			case 0xA0:

				getargs(&arg1, &arg2);

				reg[4] -= 4;

				Converter.integer = reg[arg1];
				
				memspace[reg[4] + 0] = Converter.byte[0];	
				memspace[reg[4] + 1] = Converter.byte[1];	
				memspace[reg[4] + 2] = Converter.byte[2];	
				memspace[reg[4] + 3] = Converter.byte[3];	

				programCounter += 2;

			break;

			
			case 0xB0:

				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[reg[4] + 0];			// Getting the offset bytes in
				Converter.byte[1] = memspace[reg[4] + 1];			// little endian order
				Converter.byte[2] = memspace[reg[4] + 2];			//
				Converter.byte[3] = memspace[reg[4] + 3];			//

				offset = Converter.integer;

				reg[arg1] = offset;
				reg[4] += 4;
				programCounter += 2;

			break;

			
			case 0xC0:

				ZF = 0;
				
				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			

				offset = Converter.integer;

				if (1 > scanf("%c", &inputchar))
				{
					ZF = 1;
				}
				
				memspace[reg[arg1] + offset] = inputchar;

				programCounter += 6;

			break;

			
			case 0xC1:

				ZF = 0;
				
				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			

				
				offset = Converter.integer;
				badscan = scanf("%d", &inputword);
				if (badscan < 1)
				{
					ZF = 1;
				}
				
				Converter.integer = inputword;
				memspace[reg[arg1]+ offset + 0] = Converter.byte[0];	
				memspace[reg[arg1]+ offset + 1] = Converter.byte[1];	
				memspace[reg[arg1]+ offset + 2] = Converter.byte[2];	
				memspace[reg[arg1]+ offset + 3] = Converter.byte[3];	

				programCounter += 6;

			break;

			// D0 WRTIEB
			case 0xD0:

				getargs(&arg1, &arg2);
				
				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			

				offset = Converter.integer;
		
				printf("%c", (char)memspace[reg[arg1] + offset]);
				programCounter += 6;

			break;

			
			case 0xD1:

				getargs(&arg1, &arg2);
				
				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			

				offset = Converter.integer;

				Converter.byte[0] = memspace[offset + reg[arg1] + 0];			
				Converter.byte[1] = memspace[offset + reg[arg1] + 1];			
				Converter.byte[2] = memspace[offset + reg[arg1] + 2];			
				Converter.byte[3] = memspace[offset + reg[arg1] + 3];			

				num1 = Converter.integer;
				printf("%d", num1);
				programCounter += 6;

			break;

			
			case 0xE0:

				getargs(&arg1, &arg2);

				Converter.byte[0] = memspace[programCounter + 2];			
				Converter.byte[1] = memspace[programCounter + 3];			
				Converter.byte[2] = memspace[programCounter + 4];			
				Converter.byte[3] = memspace[programCounter + 5];			

				offset = Converter.integer;
				
				Converter.integer = reg[arg2];
				inputchar = Converter.byte[3];
				
				
				if ((inputchar >> 7 & 1) == 0)
				{
					Converter.byte[0] = inputchar;
					Converter.byte[1] = 0x00;
					Converter.byte[2] = 0x00;
					Converter.byte[3] = 0x00;
				}
				else
				{
					Converter.byte[0] = inputchar;
					Converter.byte[1] = 0xff;
					Converter.byte[2] = 0xff;
					Converter.byte[3] = 0xff;
				}
					
				Converter.byte[0] = memspace[reg[arg2]+ offset + 0];	
				Converter.byte[1] = memspace[reg[arg2]+ offset + 1];	
				Converter.byte[2] = memspace[reg[arg2]+ offset + 2];	
				Converter.byte[3] = memspace[reg[arg2]+ offset + 3];	

				reg[arg1] = Converter.integer;
				programCounter += 6;

			break;
			
					
			default:
				status = INS;
			break;
		}
		offset = 0;
		arg1 = arg2 = 0;
	}while (status == AOK);
}



char * append (char * str, char c)
{
	int len = strlen(str) + 2;
	char * ret = (char *)calloc(len, sizeof(char));
	strcpy(ret, str);
	free(str);
	ret[len-1] = '\0';
	ret[len-2] = c;
	return ret;
}
	



int hextodec(char * num)
{
	// First convert hex string to binary string
	int size = strlen(num);
	char * binstr = (char *) malloc((4*size + 1)*sizeof(char));
	int i;
	for (i = 0; i < 4*size + 1; i++)
	{
		binstr[i] = '\0';
	}
	for (i = 0; i < size; i++)
	{
		strcat(binstr, hextobin(num[i]));
	}
	// Converts from binary to decimal
	int ret = bintodec(binstr);
	free(binstr);
	return ret;
}
	

char * hextobin(char c) 
{
	switch(c)
	{
		case '0':
		return "0000";
		break;
		
		case '1':
		return "0001";
		break;
		
		case '2':
		return "0010";
		break;
		
		case '3':
		return "0011";
		break;
		
		case '4':
		return "0100";
		break;
		
		case '5':
		return "0101";
		break;
		
		case '6':
		return "0110";
		break;
		
		case '7':
		return "0111";
		break;
		
		case '8':
		return "1000";
		break;
		
		case '9':
		return "1001";
		break;
		
		case 'a':
		case 'A':
		return "1010";
		break;
		
		case 'b':
		case 'B':
		return "1011";
		break;
		
		case 'c':
		case 'C':
		return "1100";
		break;
		
		case 'd':
		case 'D':
		return "1101";
		break;
		
		case 'e':
		case 'E':
		return "1110";
		break;
		
		case 'f':
		case 'F':
		return "1111";
		break;
		
		case '\0':
		break;
		
		default:
		printf("Invalid hex character: %c \n", c);
		break;
	}
	return "";
}
	


int bintodec(char * num)
{
	int power = strlen(num) - 1;
	int i, ret = 0;
	for (i = 0; num[i] != '\0'; i++)
	{
		int temp = num[i] - '0';
		ret += temp * (int)pow(2, power);
		power--;
	}
	return ret;
}



char * copy (char * str)
{
	char * ret = (char *) malloc((strlen(str) + 1) * sizeof(char));
	strcpy(ret, str);
	return ret;
}



int gettwobytes(char * str, int position)
{
	char * twobytes = (char *) malloc(3*sizeof(char));
	
	twobytes[2] = '\0';
	twobytes[0] = str[position];
	twobytes[1] = str[position + 1];
	
	int ret = hextodec(twobytes);
	
	free(twobytes);
	
	return ret;
}



void printmemory (int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		unsigned char c = memspace[i];
		if (c!=0) printf("%c", c);
	}
	printf("\n");
}


void printstatus ()
{
	printf("\nExecution: ");
	switch (status)
	{
		case AOK:
			printf("AOK,  everything is fine, no detected errors during execution\n");
		break;

		case INS:
			printf("INS, invalid instruction encountered.\n");
		break;

		case ADR:
			printf("ADR, invalid address has been encountered.\n");
		break;

		case HLT:
			printf("HLT, halt instruction encountered.\n");
		break;
	}
}



void getargs(unsigned char * arg1, unsigned char * arg2)
{
	*arg1 = (memspace[programCounter + 1] & 0xf0) >> 4;
	*arg2 = (memspace[programCounter + 1] & 0x0f);
}



