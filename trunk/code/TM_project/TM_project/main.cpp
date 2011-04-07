#include <iostream>
#include <string>

//Declare Functions
void funcALU (int ALUOp, int ALU_A, int ALU_B);
void Fetch (int PC);
void Decode ( );
void Execute ( );
void MemAccess (int ..);
void WriteBack ();
void ControlUnit (int …);
void FowardUnit (...);
void HazardDetectionUnit (...);
void InstructionsFromFile(string filename);
//I think he's expecting us to only pass the DATA around as paramaters,
//not the whole buffer, etc.

//Variables
int memory[512]; //For Memory Array
int regArray [10]; //For Register File
int inst[64]; //for instruction memory

//Register Defines (optional. Chad added this)
#define $t0 regArray[0]
#define $t1 regArray[1]
#define $t2 regArray[2]
#define $t3 regArray[3]
#define $t4 regArray[4]
#define $t5 regArray[5]
#define $t6 regArray[6]
#define $t7 regArray[7]
#define $t8 regArray[8]
#define $t9 regArray[9]

//Buffers
struct IFID //For the IF/ID registers
	{
	int PCinc;


	};
struct IDEX //For the ID/EX registers
	{


	};
struct EXMEM //For the EX/MEM registers
	{


	};
struct MEMWB //For the MEM/WB registers
	{


	};


// Control Signals
int MemWrite, MemRead, RegDst, RegWrite, MemtoReg, ALUSrc, ALUOp;

//Others

int main ()
{
// Clear Memory and Register contents
for(int i = 0;i < 512; ++i)
	memory[i] = 0;
for(int i = 0;i < 10; ++i)
	regArray[i] = 0;

//Initialize Memory contents according to the project handout.
memory[0] = 1;
memory[2] = -1;

//Read instructions from the input file and store them into memory.

//Print initial memory and register contents

// Set PC and execute program by fetching instruction from the memory Unit until the program ends. Looping.

return 0;
}

void funcALU (int ALUOp, int ALU_A, int ALU_B) {

}

void Fetch ( int PC) {

}

void Decode ( ) {

}

void Execute ( ){

	//example...
	switch(opcode) {
		case 0x0001: //add
			ALUOut= A + B;
			break;
		case 0x0003: //and
			ALUOut= A & B;
			break;
		case 0x0005: //mult
			ALUOut= high(A,B);
			break;
		case 0x000a://slt
			if (A < B)
			ALUOut= 1;
			else
			ALUOut= 0;
			break;
		case 0x000a://sltu
			if ((unsigned word)A < (unsigned word)B)
			ALUOut= 1;
			else
			ALUOut= 0;
			break;
		case 0x000b://addi
			ALUOut= A + sign_extend5to16((IR & 0x001F));
			break;
		case 0x000c://subi
			ALUOut= A -sign_extend5to16((IR & 0x001F));
			break;
		case 0x0010://sw
			ALUOut= A + sign_extend5to16((IR & 0x001F));
			break;
		default:
			break;
	}//end switch
}//Execute end

void MemAccess ( int ..) {

}

void WriteBack ( ) {

}

void ControUnit ( int …) {

}

void FowardUnit (…){

};

void HazardDetectionUnit (…) {

};