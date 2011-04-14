// 16bit RISC Processor - Computer Architecture - Spring 2011
// Authors: Stephen Cernota, Jeff Hsu, Chad S. (lol)
#include <iostream>
#include <fstream>
#include <string>

#define DATA_SIZE 512
#define REG_SIZE 10
#define INST_SIZE 64

using namespace std;

// Declare Functions
void funcALU (int ALUOp, int ALU_A, int ALU_B);
void Fetch ();
void Decode ();
void Execute ();
void MemAccess (); // (int ...)
void WriteBack ();
void ControlUnit (); // (int ...)
void FowardUnit ();
void HazardDetectionUnit ();
// I think he's expecting us to only pass the DATA around as parameters, not the whole buffer, etc.

// Variables
int dataMem[DATA_SIZE]; // Data Memory
int regFile[REG_SIZE]; // Register File
int instMem[INST_SIZE]; // Instruction Memory

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

// Buffers - Pipelined Registers
struct IFID
{
	int PCInc;
	int IF_Flush;
	int Instruction;

} IFID, IFIDtemp;

struct IDEX
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

} IDEX, IDEXtemp;

struct EXMEM
{
	int ALUResult;
	int ForwardBMuxResult;
	int RegDstMuxResult;

	///// Control
	// EX/MEM
	int MemRead;
	int MemWrite;
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	
} EXMEM, EXMEMtemp;

struct MEMWB
{
	int DataMemoryResult;
	int ALUResult;
	int EXMEM_RegisterRd;

	//// Control
	// MEM/WB
	int RegWrite;
	int MemtoReg;
	
} MEMWB, MEMWBtemp;

struct HAZARD
{
	// Inputs
	int IDEX_MemRead;
	int IDEX_RegisterRt;
	int IFID_RegisterRs;
	int IFID_RegisterRt;
	int RegEQ; // For reduced branch delay

	// Outputs
	int PCWrite;
	int IFID_Write;
	int LinetoMux; // What to name this?
	int PCSrc; // Extra input to control left branch mux

} HAZARD, HAZARDtemp;

struct CONTROL
{
	///// Control
	int IF_Flush;
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
	
} CONTROL, CONTROLtemp;

struct FORWARD
{
	// Inputs
	int IDEX_RegisterRs;
	int IDEX_RegisterRt;
	// EX Hazard
	int EXMEM_RegWrite;
	int EXMEM_RegisterRd;
	// MEM Hazard
	int MEMWB_RegWrite;
	int MEMWB_RegisterRd;

	// Outputs
	int ForwardA;
	int ForwardB;

} FORWARD, FORWARDtemp;

// Program Counter
int PC;

int main ()
{
	// Clear Register File, Data Memory, and Instrution Memory // (DONE)
	for(int i = 0; i < REG_SIZE; ++i)
		regFile[i] = 0;
	for(int i = 0; i < DATA_SIZE; ++i)
		dataMem[i] = 0;
	for(int i = 0; i < INST_SIZE; ++i)
		instMem[i] = 0;

	// Initialize Data Memory contents according to the project handout
	// Set Register File
	// registers need to be redefined
	$v0 = 0x0040;	//$v0 = 0040 hex; 
	$v1 = 0x1010;	//$v1 = 1010 hex; 
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

		// Add 4 more no op instructions to end of instruction memory, either add them in "instructions.txt" file or add them here
		
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
	cout << "\nDEBUG: PRINTING INSTRUCTION MEM.\n"; //(for our own benefit - print instruction memory)
	for(int i = 0; i < 64; ++i)
		cout << "instMem[" << i << "] = " << instMem[i] << endl;

	// Set all register pipelines and signals up - basically have no ops set up to run in all other stages when first executing
	// Stage structures
	IFID.PCInc = 2;
	IFID.IF_Flush = 0;
	IFID.Instruction = 0;

	IDEX.RegisterOne = 0;
	IDEX.RegisterTwo = 0; 
	IDEX.SignExtendImmediate = 0;

	IDEX.IFID_RegisterRs = 0;
	IDEX.IFID_RegisterRt_toMux = 0;		
	IDEX.IFID_RegisterRt_toForward = 0;	
	IDEX.IFID_RegisterRd = 0;

	IDEX.ALUOp = 0;
	IDEX.ALUSrc = 0;
	IDEX.RegDst = 0;

	IDEX.MemRead = 0;
	IDEX.MemWrite = 0;
	
	IDEX.RegWrite = 0;
	IDEX.MemtoReg = 0;

	EXMEM.ALUResult = 0;
	EXMEM.ForwardBMuxResult = 0;
	EXMEM.RegDstMuxResult = 0;

	EXMEM.MemRead = 0;
	EXMEM.MemWrite = 0;
	EXMEM.RegWrite = 0;
	EXMEM.MemtoReg = 0;

	MEMWB.DataMemoryResult = 0;
	MEMWB.ALUResult = 0;
	MEMWB.EXMEM_RegisterRd = 0;

	MEMWB.RegWrite = 0;
	MEMWB.MemtoReg = 0;

	// Set Hazard, Control and Forward units before execution
	HAZARD.IDEX_MemRead = 0;
	HAZARD.IDEX_RegisterRt = 0;
	HAZARD.IFID_RegisterRs = 0;
	HAZARD.IFID_RegisterRt = 0;
	HAZARD.RegEQ = 0;
	HAZARD.PCWrite = 0;
	HAZARD.IFID_Write = 0;
	HAZARD.LinetoMux = 0; // What to name this?
	HAZARD.PCSrc = 0;

	CONTROL.IF_Flush = 0;
	CONTROL.ALUOp = 0;
	CONTROL.ALUSrc = 0;
	CONTROL.RegDst = 0;
	CONTROL.MemRead = 0;
	CONTROL.MemWrite = 0;
	CONTROL.RegWrite = 0;
	CONTROL.MemtoReg = 0;

	FORWARD.IDEX_RegisterRs = 0;
	FORWARD.IDEX_RegisterRt = 0;
	FORWARD.EXMEM_RegWrite = 0;
	FORWARD.EXMEM_RegisterRd = 0;
	FORWARD.MEMWB_RegWrite = 0;
	FORWARD.MEMWB_RegisterRd = 0;
	FORWARD.ForwardA = 0;
	FORWARD.ForwardB = 0;

	// Set PC and execute program by fetching instruction from the memory Unit until the program ends. Looping.
	PC = 0;
	while( PC < instCount )
	{
		Fetch();
		Decode();
		Execute();
		MemAccess();
		WriteBack();

		// Update all pipeline register values
		IFID = IFIDtemp;
		IDEX = IDEXtemp;
		EXMEM = EXMEMtemp;
		MEMWB = MEMWBtemp;

		// Update units
		CONTROL = CONTROLtemp;
		HAZARD = HAZARDtemp;
		FORWARD = FORWARDtemp;
	}

	return 0;
}

// Reads in two values (function code and ALUOp), outputs one value (that travels to ALU, what is it called?)
void ALUControl (int SignExtendImmediate, int ALUOp) 
{
	if(ALUOp == 0)
	{
		switch(SignExtendImmediate)
		{
			case 0: // add
				break;
			case 1: // sub
				break;
			case 2: // and
				break;
			case 3: // or
				break;
			case 4: // xor
				break;
			case 5: // nor
				break;
			case 6: // slt
				break;
		}
	}
	else
	{
		switch(ALUOp)
		{
			case 1: // add
				break;
			case 2: // sub
				break;
			case 3: // and
				break;
			case 4: // or
				break;
			case 5: // xor
				break;
			case 6: // nor
				break;
			case 7: // slt	
				break;
			case 8: // sll
				break;
			case 9: // srl	
				break;
		}
	}
}

void Fetch () 
{
	// Note: I created a global PC integer named: PC , We may end up needing a PCtemp

	// How to deal with MUX before PC?

	IFIDtemp.Instruction = instMem[PC];
	IFIDtemp.PCInc = PC + 2;

	// How to grab IF_Flush from Control Unit?
	//IFIDtemp.IF_Flush;
}

void Decode ( ) 
{
	// Instrution to use: IFID.Instruction
	//IDEXtemp.IFID_RegisterRs;		// Example: = (IFID.Instruction).substring(12, 16);
	//IDEXtemp.IFID_RegisterRt_toMux;
	//IDEXtemp.IFID_RegisterRt_toForward;
	//IDEXtemp.IFID_RegisterRd;

	// Read from RegFile
	//IDEXtemp.RegisterOne;		// Example: = regFile[x]
	//IDEXtemp.RegisterTwo; 

	// Sign Extend
	//IDEXtemp.SignExtendImmediate;

	// Hazard Detection Unit

	// Control Unit

	///// Control
	// ID/EX
	//IDEXtemp.ALUOp;
	//IDEXtemp.ALUSrc;
	//IDEXtemp.RegDst;
	// EX/MEM
	//IDEXtemp.MemRead;
	//IDEXtemp.MemWrite;
	// MEM/WB
	//IDEXtemp.RegWrite;
	//IDEXtemp.MemtoReg;
	////

	// Other stuff not listed yet
}

void Execute ( )
{
	// Forward Unit

	// Deal with Mux's, ALU, ALUControl
	//EXMEMtemp.ALUResult;
	//EXMEMtemp.ForwardBMuxResult;
	//EXMEMtemp.RegDstMuxResult;

	///// Control
	// EX/MEM
	//EXMEMtemp.MemRead;
	//EXMEMtemp.MemWrite;
	// MEM/WB
	//EXMEMtemp.RegWrite;
	//EXMEMtemp.MemtoReg;
	////

	// Example
	/*	
	switch(opcode) 
	{
		case 0x0001: // add
			EXMEM_ALUResult = A + B;
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
		default:
			break;
	}
	*/
} 

void MemAccess () // (int ...) 
{
	// Deal with DataMem
	//MEMWBtemp.ALUResult;
	//MEMWBtemp.EXMEM_RegisterRd;
	//MEMWBtemp.DataMemoryResult;

	//// Control
	// MEM/WB
	//MEMWBtemp.RegWrite;
	//MEMWBtemp.MemtoReg;
	
}

void WriteBack () 
{
	// One MUX
}

void ControUnit () // (int ...) 
{
	// Input: Instruction opcode
	// Output: Control signals, IF.Flush

	CONTROLtemp.IF_Flush;
	CONTROLtemp.ALUOp;
	CONTROLtemp.ALUSrc;
	CONTROLtemp.RegDst;
	CONTROLtemp.MemRead;
	CONTROLtemp.MemWrite;
	CONTROLtemp.RegWrite;
	CONTROLtemp.MemtoReg;
}

void FowardUnit () // How to use output values?
{
	int ForwardA = 00;
	int ForwardB = 00;

	// EX Hazard
	if ( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult == IDEX.IFID_RegisterRs) )
		ForwardA = 0x02;
	if ( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult == IDEX.IFID_RegisterRt_toForward) )
		ForwardB = 0x02;

	// MEM Hazard
	if ( MEMWB.RegWrite && ( MEMWB.EXMEM_RegisterRd != 0 ) && ( MEMWB.EXMEM_RegisterRd == IDEX.IFID_RegisterRs) )
		ForwardA = 0x02;
	if ( MEMWB.RegWrite && ( MEMWB.EXMEM_RegisterRd != 0 ) && ( MEMWB.EXMEM_RegisterRd == IDEX.IFID_RegisterRt_toForward) )
		ForwardB = 0x02;

	FORWARDtemp.IDEX_RegisterRs;
	FORWARDtemp.IDEX_RegisterRt;
	FORWARDtemp.EXMEM_RegWrite;
	FORWARDtemp.EXMEM_RegisterRd;
	FORWARDtemp.MEMWB_RegWrite;
	FORWARDtemp.MEMWB_RegisterRd;
	FORWARDtemp.ForwardA;
	FORWARDtemp.ForwardB;
};

void HazardDetectionUnit () 
{
	// IFID.RegisterRs and IFID.RegisterRt can be pulled straight from instruction
	//if ( IDEX.MemRead && ( ( IDEX.IFID_RegisterRt_toForward == IFID.RegisterRs ) || ( IDEX.IFID_RegisterRt_toForward == IFID.RegisterRt) ) )
		// stall pipeline

	HAZARDtemp.IDEX_MemRead;
	HAZARDtemp.IDEX_RegisterRt;
	HAZARDtemp.IFID_RegisterRs;
	HAZARDtemp.IFID_RegisterRt;
	HAZARDtemp.RegEQ;
	HAZARDtemp.PCWrite;
	HAZARDtemp.IFID_Write;
	HAZARDtemp.LinetoMux; // What to name this?
	HAZARDtemp.PCSrc;
};