// 16bit RISC Processor - Computer Architecture - Spring 2011
// Authors: Stephen Cernota, Jeff Hsu, Chad S. (lol)
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// Declare Functions
void funcALU (int ALUOp, int ALU_A, int ALU_B);
void Fetch (int PC);
void Decode ();
void Execute ();
void MemAccess (); // (int ...)
void WriteBack ();
void ControlUnit (); // (int ...)
void FowardUnit ();
void HazardDetectionUnit ();
void InstructionsFromFile(string fileName);
// I think he's expecting us to only pass the DATA around as parameters, not the whole buffer, etc.

// Variables
int dataMem[512]; // Data Memory
int regFile[10]; // Register File
int instMem[64]; // Instruction Memory

// Register Defines (Optional -Chad)
#define $t0 regFile[0]
#define $t1 regFile[1]
#define $t2 regFile[2]
#define $t3 regFile[3]
#define $v0 regFile[4]
#define $v1 regFile[5]
#define $v2 regFile[6]
#define $v3 regFile[7]
#define $a0 regFile[8]
#define $a1 regFile[9]

// Buffers
// *Im having a hard time deciding what to name things LOL* - Stephen
struct IFID //For the IF/ID registers
{
	// IF/ID
	int PCInc;
	int IF_Flush; // From now on make "."'s into "_"'s ?
	int Instruction;

	///// Control
	// ID/EX
	int ALUOp;
	int ALUSrc;
	int RegDst;
	// EX/MEM
	int MemRead;
	int MemWrite;
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	////
};

struct IDEX //For the ID/EX registers
{
	int RegisterOne;
	int RegisterTwo; 
	int SignExtendImmediate;

	int IFID_RegisterRs;
	int IFID_RegisterRt_toMux;		// Connects to Mux
	int IFID_RegisterRt_toForward;	// Connects to ForwardingUnit
	int IFID_RegisterRd;

	///// Control
	// ID/EX
	int ALUOp;
	int ALUSrc;
	int RegDst;
	// EX/MEM
	int MemRead;
	int MemWrite;
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	////
};

struct EXMEM //For the EX/MEM registers
{
	int ALUResult;
	int ForwardBMuxResult;
	int RegDstMux;

	///// Control
	// EX/MEM
	int MemRead;
	int MemWrite;
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	////
};

struct MEMWB //For the MEM/WB registers
{
	int DataMemoryResult;
	int ALUResult;
	int EXMEM_RegisterRd;

	//// Control
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	////
};


// Control Signals
int MemWrite, MemRead, RegDst, RegWrite, MemtoReg, ALUSrc, ALUOp;

// Others

int main ()
{
	// Clear Register File, Data Memory, and Instrution Memory // (DONE)
	for(int i = 0; i < 10; ++i)
		regFile[i] = 0;
	for(int i = 0; i < 512; ++i)
		dataMem[i] = 0;
	for(int i = 0; i < 64; ++i)
		instMem[i] = 0;

	// Initialize Data Memory contents according to the project handout.
	//
	// Set Register File // (How do we want to redefine these registers?)  (see below, until we figure something else out... -Chad)
	$v0 = 0x0040;	//$v0 = 0040 hex; // you can redefine $v0-3, $t0, and $a0-1 with
	$v1 = 0x1010;	//$v1 = 1010 hex; // your register numbers such as $1, $2, etc.
	$v2 = 0x000F;	//$v2 = 000F hex;
	$v3 = 0x00F0;	//$v3 = 00F0 hex;
	$t0 = 0x0000;	//$t0 = 0000 hex;
	$a0 = 0x0010;	//$a0 = 0010 hex;
	$a1 = 0x0005;	//$a1 = 0005 hex;

	// Set Data Memory
	dataMem[$a0] = 0x0101;		//Mem[$a0] = 0101 hex
	dataMem[$a0 + 2] = 0x0110;	//Mem[$a0+2] = 0110 hex
	dataMem[$a0 + 4] = 0x0011;	//Mem[$a0+4] = 0011 hex
	dataMem[$a0 + 6] = 0x00F0;	//Mem[$a0+6] = 00F0 hex
	dataMem[$a0 + 8] = 0x00FF;	//Mem[$a0+8] = 00FF hex

	// Read instructions from the input file and store them into memory. // (DONE)
	ifstream f;
	string tempString;
	int instCount = 0;

	cout << "Loading \"instrutions.txt\"..." << endl;
	f.open("instructions.txt");

	if (f.is_open())
	{
		while ( f.good() )
		{
			getline(f, tempString);
			if( tempString.length() > 0 )
			{
				// Convert instruction from string to int
				int tempInt = (int)strtol(tempString.c_str(),NULL,2);	//tempString is read as ASCII for binary bits, converted into a long int, typecasted to int
				// Store instruction in instruction memory
				instMem[instCount] = tempInt;
			}
			
			instCount++;
		}
		
		cout << "Done.\n\n" << endl;
		f.close();
	 }
	else
	{
		cout << "ERROR: cannot open \"instructions.txt\", make sure it is in same folder as main.cpp\n\n" << endl;
		return 0;
	}

	// Print initial memory and register contents // (DONE)
	for(int i = 0; i < 10; ++i)
		cout << "regFile[" << i << "] = " << regFile[i] << endl;
	for(int i = 0; i < 30; ++i)
		cout << "dataMem[" << i << "] = " << dataMem[i] << endl;

	//(for our own benefit - print instruction memory)
	cout << "\nDEBUG: PRINTING INSTRUCTION MEM.\n";
	for(int i = 0; i < 64; ++i)
		cout << "instMem[" << i << "] = " << instMem[i] << endl;

	// Set PC and execute program by fetching instruction from the memory Unit until the program ends. Looping.

	return 0;
}

void funcALU (int ALUOp, int ALU_A, int ALU_B) 
{

}

void Fetch ( int PC) 
{

}

void Decode ( ) 
{

}

void Execute ( )
{
/*	switch(opcode) 
	{
		case 0x0001: // add
			EXMEM_ALUResult = A + B;
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
	}
*/
} 

void MemAccess () // (int ...) 
{

}

void WriteBack () 
{

}

void ControUnit () // (int ...) 
{

}

void FowardUnit ()
{

};

void HazardDetectionUnit () 
{

};