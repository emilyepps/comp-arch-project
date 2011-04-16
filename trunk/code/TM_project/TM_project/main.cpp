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
void HazardDetectionUnit (char * op);
void ControlUnit (char * op);
void FowardUnit ();

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

	int ALUOp;
	int ALUSrc;
	int RegDst;
	int MemRead;
	int MemWrite;
	int RegWrite;
	int MemtoReg;

} IDEX, IDEXtemp;

struct EXMEM
{
	int ALUResult;
	int ForwardBMuxResult;
	int RegDstMuxResult;

	int MemRead;
	int MemWrite;
	int RegWrite;
	int MemtoReg;
	
} EXMEM, EXMEMtemp;

struct MEMWB
{
	int DataMemoryResult;
	int ALUResult;
	int EXMEM_RegisterRd;

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
	int LinetoMux;
	int PCSrc; // Extra input to control left branch mux

} HAZARD, HAZARDtemp;

struct CONTROL
{
	int IF_Flush;

	int ALUOp;
	int ALUSrc;
	int RegDst;
	int MemRead;
	int MemWrite;
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
	HAZARD.LinetoMux = 0;
	HAZARD.PCSrc = 0;

	CONTROL.IF_Flush = 0;
	CONTROL.ALUOp = 1;
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

	// MUX before PC
	if(HAZARD.PCSrc == 0)
		IFIDtemp.Instruction = instMem[IFID.PCInc];
	else
		//IFIDtemp.Instruction = instMem[ ? ]; // Value comes from Reduced Branch Shift Adder, where is that value stored?

	IFIDtemp.PCInc += 2;
	IFIDtemp.IF_Flush = CONTROL.IF_Flush;
}

void Decode ( ) 
{
	// Convert integer form of instruction to char array
	char inst[16];
	itoa(IFID.Instruction, inst, 2);

	char rs[3] = {inst[4], inst[5], inst[6]};
	IDEXtemp.IFID_RegisterRs = atoi(rs);
	char rt[3] = {inst[7], inst[8], inst[9]};
	IDEXtemp.IFID_RegisterRt_toMux = atoi(rt);
	IDEXtemp.IFID_RegisterRt_toForward = atoi(rt);
	char rd[3] = {inst[10], inst[11], inst[12]};
	IDEXtemp.IFID_RegisterRd = atoi(rd);

	// Read from RegFile
	IDEXtemp.RegisterOne = regFile[IDEXtemp.IFID_RegisterRs];
	IDEXtemp.RegisterTwo = regFile[IDEXtemp.IFID_RegisterRt_toMux]; 

	// Sign Extend
	//IDEXtemp.SignExtendImmediate;

	// Grab Opcode
	char opcode[4] = {inst[0], inst[1], inst[2], inst[3]};

	// Hazard Detection Unit
	HazardDetectionUnit(opcode);

	// Control Unit
	ControlUnit(opcode);

	// Mux after Control Unit
	if( HAZARDtemp.LinetoMux == 0 )
	{
		IDEXtemp.ALUOp = CONTROLtemp.ALUOp;
		IDEXtemp.ALUSrc = CONTROLtemp.ALUSrc;
		IDEXtemp.RegDst = CONTROLtemp.RegDst;
		IDEXtemp.MemRead = CONTROLtemp.MemRead;
		IDEXtemp.MemWrite = CONTROLtemp.MemWrite;
		IDEXtemp.RegWrite = CONTROLtemp.RegWrite;
		IDEXtemp.MemtoReg = CONTROLtemp.MemtoReg;
	}
	else
	{
		IDEXtemp.ALUOp = 0;
		IDEXtemp.ALUSrc = 0;
		IDEXtemp.RegDst = 0;
		IDEXtemp.MemRead = 0;
		IDEXtemp.MemWrite = 0;
		IDEXtemp.RegWrite = 0;
		IDEXtemp.MemtoReg = 0;
	}

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

void HazardDetectionUnit (char * op) 
{
	int opcode = atoi(op);

	HAZARDtemp.IFID_RegisterRs = IDEXtemp.IFID_RegisterRs;
	HAZARDtemp.IFID_RegisterRt = IDEXtemp.IFID_RegisterRt_toMux;

	if ( IDEX.MemRead && ( ( IDEX.IFID_RegisterRt_toForward == HAZARDtemp.IFID_RegisterRs) || ( IDEX.IFID_RegisterRt_toForward == HAZARDtemp.IFID_RegisterRt) ) )
		// stall pipeline
		// How to stall pipeline? //////////////////////////

	// RegEQ - for reduced branch delay
	if ( IDEXtemp.RegisterOne == IDEXtemp.RegisterTwo )
		HAZARDtemp.RegEQ = 1;
	else
		HAZARDtemp.RegEQ = 0;

	// What to do with all these signals? How to use opcode to set some of these signals? ////////////////////////////////
	HAZARDtemp.IDEX_MemRead;
	HAZARDtemp.IDEX_RegisterRt;
	HAZARDtemp.PCWrite;
	HAZARDtemp.IFID_Write;
	HAZARDtemp.LinetoMux;
	HAZARDtemp.PCSrc;
};

void ControUnit (char * op)
{
	// Input: Instruction opcode
	// Output: Control signals, IF.Flush

	int opcode = atoi(op);

	CONTROLtemp.IF_Flush; // How to set this? ////////////////

	// RegDst
	if(opcode == 0)
		CONTROLtemp.RegDst = 1;
	else
		CONTROLtemp.RegDst = 0;

	// ALUOp
	int ALUOp;
	switch(opcode)
	{
		case 0: ALUOp = 0; break;
		case 1: ALUOp = 8; break;
		case 2: ALUOp = 9; break;
		case 3: ALUOp = 1; break;
		case 4: ALUOp = 2; break;
		case 5: ALUOp = 3; break;
		case 6: ALUOp = 4; break;
		case 7: ALUOp = 5; break;
		case 8: ALUOp = 6; break;
		case 9: ALUOp = 7; break;
		case 10: ALUOp = 1; break;
		case 11: ALUOp = 1; break;
		case 12: ALUOp = 1; break;
		default: ALUOp = 1; break; // What number to make nops and jumps? This affects ALU ////////////////
	}
	CONTROLtemp.ALUOp = ALUOp;

	// ALUSrc
	if( opcode == 0 || opcode == 12 || opcode == 14 )
		CONTROLtemp.ALUSrc = 0;
	else
		CONTROLtemp.ALUSrc = 1;

	// MemRead
	if( opcode == 10 )
		CONTROLtemp.MemRead = 1;
	else
		CONTROLtemp.MemRead = 0;

	// MemWrite
	if( opcode == 11 )
		CONTROLtemp.MemWrite = 1;
	else
		CONTROLtemp.MemWrite = 0;

	// RegWrite
	if( opcode >= 11)
		CONTROLtemp.RegWrite = 0;
	else
		CONTROLtemp.RegWrite = 1;

	// MemtoReg
	if( opcode == 10)
		CONTROLtemp.MemtoReg = 1;
	else
		CONTROLtemp.MemtoReg = 0;
}

void FowardUnit ()
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