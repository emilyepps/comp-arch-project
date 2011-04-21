// 16bit RISC Processor - Computer Architecture - Spring 2011
// Authors: Stephen Cernota, Jeff Hsu, Chad Schrepferman
#include <iostream>
#include <fstream>
#include <string>
#include <limits>

#define DATA_SIZE 512
#define REG_SIZE 10
#define INST_SIZE 64

using namespace std;

// Declare Functions
void ALU(int opcode, int SignExtendImmediate, int ALU_A, int ALU_B);
void Fetch ();
void Decode ();
void Execute ();
void MemAccess ();
void WriteBack ();
void HazardDetectionUnit (char * op);
void ControlUnit (char * op);
void ForwardUnit ();
void printAll();

// Variables
int dataMem[DATA_SIZE]; // Data Memory
int regFile[REG_SIZE];  // Register File
int instMem[INST_SIZE]; // Instruction Memory

// Register Defines 
#define $zero regFile[0]
#define $v0 regFile[1]
#define $v1 regFile[2]
#define $v2 regFile[3]
#define $v3 regFile[4]
#define $t0 regFile[5]
#define $a0 regFile[6]
#define $a1 regFile[7]
#define $a2 regFile[8]
#define $a3 regFile[9]

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
	int IFID_RegisterRt_toMux;
	int IFID_RegisterRt_toForward;
	int IFID_RegisterRd;

	int ALUOp;
	int ALUSrc;
	int RegDst;
	int MemRead;
	int MemWrite;
	int RegWrite;
	int MemtoReg;
	int Opcode;

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

struct FloatingBuf	//data that flows backwards, but doesn't pass through a buffer (isn't stored)
{
	int ID_BranchJumpAddress;
	int WB_MuxOutcome;
} FB;

struct HAZARD
{
	int IDEX_MemRead;
	int IDEX_RegisterRt;
	int IFID_RegisterRs;
	int IFID_RegisterRt;
	int RegEQ; // For reduced branch delay

	int PCWrite;
	int IFID_Write;
	int LinetoMux;
	int PCSrc; // Extra input to control left branch mux
	int Branch;
	int Jump;

} HAZARD;

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
	
} CONTROL;

struct FORWARD
{
	int IDEX_RegisterRs;
	int IDEX_RegisterRt;
	int EXMEM_RegWrite;
	int EXMEM_RegisterRd;
	int MEMWB_RegWrite;
	int MEMWB_RegisterRd;

	int ForwardA;
	int ForwardB;

} FORWARD;

// Global Signals
int PC;

int main ()
{
	// Clear Register File, Data Memory, and Instrution Memory
	for(int i = 0; i < REG_SIZE; ++i)
		regFile[i] = 0;
	for(int i = 0; i < DATA_SIZE; ++i)
		dataMem[i] = 0;
	for(int i = 0; i < INST_SIZE; ++i)
		instMem[i] = 0;

	// Set Register File // Set these with instructions!
	$v0 = 0x0040;	//$v0 = 0040 hex;
	$v1 = 0x1010;	//$v1 = 1010 hex; 
	$v2 = 0x000F;	//$v2 = 000F hex;
	$v3 = 0x00F0;	//$v3 = 00F0 hex;
	$t0 = 0x0000;	//$t0 = 0000 hex;
	$a0 = 0x0010;	//$a0 = 0010 hex;
	$a1 = 0x0005;	//$a1 = 0005 hex;
	$a2 = 0x0000;	
	$a3 = 0x0000;

	// Set Data Memory
	dataMem[$a0] = 0x0101;		//Mem[$a0] = 0101 hex
	dataMem[$a0 + 2] = 0x0110;	//Mem[$a0+2] = 0110 hex
	dataMem[$a0 + 4] = 0x0011;	//Mem[$a0+4] = 0011 hex
	dataMem[$a0 + 6] = 0x00F0;	//Mem[$a0+6] = 00F0 hex
	dataMem[$a0 + 8] = 0x00FF;	//Mem[$a0+8] = 00FF hex

	// Read instructions from the input file and store them into memory.
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
				int tempInt = (int)strtol(tempString.c_str(),NULL,2);	//tempString is read as ASCII for binary bits, converted into a long int, typecasted to int
				instMem[instCount] = tempInt;
			}	
			instCount++;
		}

		cout << instCount << endl;
		cout << "Done.\n\n" << endl;
		f.close();
	 }
	else
	{
		cout << "ERROR: cannot open \"instructions.txt\", make sure it is in same folder as main.cpp\n\n" << endl;
		return 0;
	}

	// Print initial memory and register contents
	printAll();

	// Setup signals before execution
	IFID.PCInc = 0;
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
	IDEX.Opcode = 0;

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

	FB.ID_BranchJumpAddress = 0;
	FB.WB_MuxOutcome = 0;

	// Set Hazard, Control and Forward units before execution
	HAZARD.IDEX_MemRead = 0;
	HAZARD.IDEX_RegisterRt = 0;
	HAZARD.IFID_RegisterRs = 0;
	HAZARD.IFID_RegisterRt = 0;
	HAZARD.RegEQ = 0;
	HAZARD.PCWrite = 1;
	HAZARD.IFID_Write = 1;
	HAZARD.LinetoMux = 0;
	HAZARD.PCSrc = 0;
	HAZARD.Branch = 0;
	HAZARD.Jump = 0;

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
	while( PC < (instCount - 1)*2 )
	{
		PC = IFID.PCInc;
		cout << PC << endl;
		cout << "HAZARD.PCWrite: " << HAZARD.PCWrite << endl;
		cout << "HAZARD.IFID_Write: " << HAZARD.IFID_Write << endl;
		Fetch();
		Decode();
		Execute();
		MemAccess();
		cout << "MEMWB.regwrite: " << MEMWB.RegWrite << endl;
		WriteBack();

		IFID = IFIDtemp;		// Update pipeline register values
		IDEX = IDEXtemp;
		EXMEM = EXMEMtemp;
		MEMWB = MEMWBtemp;
		//system("pause");
		cout << endl;
	}

	// Print results after execution
	cout << "//////////////////////////////////////////////////////////////////////" << endl;
	printAll();

	string key;
	cout << "Press a key" << endl;
	cin >> key;

	return 0;
}

int binStringToDec(char* ar,int bits)
{
	int binNum = 0;
	for(int x = bits - 1; x >= 0; x--)
	{ 
		if(ar[x] == '1')
			binNum += (int)pow((double)2,((bits - 1)-x));
	}

	return binNum;
}

string decToBinString(int decNum)
{
	string temp = "0000000000000000";
	int divisor = decNum;
	for (int index = 15; index >= 0; index--)
	{
		temp[index] = divisor % 2;
		divisor = divisor / 2;
	}
	return temp;
}


string cvt_binary(unsigned int input) 
{
	if(input == 0) return "0000000000000000"; // trivial case
	string result;

	for(int i = 16 - 1; i >= 0; --i) {
		
		if(input & (1 << i))
			result += "1";
		else
			result += "0";

	}
	return result;
}

void Fetch () 
{
	cout << "Fetch()\n"; /////////////////////////////////////////////

	// MUX before PC
	if( HAZARD.PCWrite == 1 && HAZARD.IFID_Write == 1)
	{
		if(HAZARD.PCSrc == 0)
		{
			IFIDtemp.Instruction = instMem[IFID.PCInc/2];
		}
		else
		{
			IFIDtemp.Instruction = instMem[FB.ID_BranchJumpAddress/2];
//			IFIDtemp.PCInc = FB.ID_BranchJumpAddress;
		}

	}

	if( HAZARD.IFID_Write == 1 )
		IFIDtemp.PCInc += 2;

	//IFIDtemp.IF_Flush = CONTROL.IF_Flush; // Dont need this, automatically inserted noops after beq's
}

void Decode ( ) 
{
	cout << "Decode()\n"; /////////////////////////////////////////

	// Convert integer form of instruction to char array)
//	char inst[16] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
	string str;
	//str = decToBinString(IFID.Instruction);
	str = cvt_binary(IFID.Instruction);
	cout << "String: " << str << endl;
	cout << "-Instruction: " << str << endl; /////////////////////////////////////////

	// Grab sections of instruction
	char rs[4] = {str[4], str[5], str[6], str[7]};
	cout << "rs: " << rs << endl;
	IDEXtemp.IFID_RegisterRs = binStringToDec(rs,4);
	cout << "-IDEXtemp.IFID_RegisterRs: " << IDEXtemp.IFID_RegisterRs << endl; ////////////////////////////////////


	char rt[4] = {str[8], str[9], str[10], str[11]};
	cout << "rt: " << rt << endl;
	IDEXtemp.IFID_RegisterRt_toMux = binStringToDec(rt,4);
	IDEXtemp.IFID_RegisterRt_toForward = binStringToDec(rt,4);
	cout << "-IDEXtemp.IFID_RegisterRt_tM_tF: " << IDEXtemp.IFID_RegisterRt_toForward << endl; //////////////////////////////////////


	char rd[4] = {str[12], str[13], str[14], str[15]};
	cout << "rd: " << rd << endl;
	IDEXtemp.IFID_RegisterRd = binStringToDec(rd,4);
	cout << "-IDEXtemp.IFID_RegisterRd: " << IDEXtemp.IFID_RegisterRd << endl; //////////////////////////////////////

	// Read from RegFile
	IDEXtemp.RegisterOne = regFile[IDEXtemp.IFID_RegisterRs];
	IDEXtemp.RegisterTwo = regFile[IDEXtemp.IFID_RegisterRt_toMux]; 

	cout << "-RegisterOne: " << IDEXtemp.RegisterOne << endl; ////////////////////////////////////
	cout << "-RegisterTwo: " << IDEXtemp.RegisterTwo << endl; //////////////////////////////////////

	// Sign Extend
	char immediate[4] = {str[12], str[13], str[14], str[15]};
	IDEXtemp.SignExtendImmediate = binStringToDec(immediate,4); 

	// Grab Opcode
	char opcode[4] = {str[0], str[1], str[2], str[3]};

	// Pass opcode
	IDEXtemp.Opcode = binStringToDec(opcode,4);

	cout << "-Opcode: " << IDEXtemp.Opcode << endl; ////////////////////////////////////

	// Hazard Detection Unit
	HazardDetectionUnit(opcode);

	// Control Unit
	ControlUnit(opcode);

	// Mux after Control Unit
	if( HAZARD.LinetoMux == 0 )
	{
		IDEXtemp.ALUOp = CONTROL.ALUOp;
		IDEXtemp.ALUSrc = CONTROL.ALUSrc;
		IDEXtemp.RegDst = CONTROL.RegDst;
		IDEXtemp.MemRead = CONTROL.MemRead;
		IDEXtemp.MemWrite = CONTROL.MemWrite;
		IDEXtemp.RegWrite = CONTROL.RegWrite;
		IDEXtemp.MemtoReg = CONTROL.MemtoReg;
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

	// Two Muxes after Shift Adder
	int muxOneResult;
	if( HAZARD.Branch == 1 )
		muxOneResult = IFID.PCInc + (IDEXtemp.SignExtendImmediate << 1);
	else
		muxOneResult = IFID.PCInc;

	int muxTwoResult;
	if( HAZARD.Jump == 1 )
	{
		char jumpIm[13] = {str[4], str[5], str[6], str[7], str[8], str[9], str[10], str[11], str[12], str[13], str[14], str[15], '0'};
		muxTwoResult = binStringToDec(jumpIm,13);
	}
	else
		muxTwoResult = muxOneResult;

	FB.ID_BranchJumpAddress = muxTwoResult;

}

void Execute()
{
	cout << "Execute()\n"; /////////////////////////////////////////

	// Forward Unit
	ForwardUnit();

	// ForwardA Mux
	int ForwardAResult;
	if( FORWARD.ForwardA == 0 )
		ForwardAResult = IDEX.RegisterOne;
	else if( FORWARD.ForwardA == 1 )
		ForwardAResult = FB.WB_MuxOutcome; // the value coming out of the MUX in the WB stage
	else
		ForwardAResult = EXMEM.ALUResult;
	cout << "FORWARD.forwardA: " << FORWARD.ForwardA << endl;
	cout << "register one: " << IDEX.RegisterOne << endl;
	cout << "WB_muxresult1: " << FB.WB_MuxOutcome << endl;

	// ForwardB Mux
	if( FORWARD.ForwardB == 0 )
		EXMEMtemp.ForwardBMuxResult = IDEX.RegisterTwo;
	else if( FORWARD.ForwardB == 1 )
		EXMEMtemp.ForwardBMuxResult = EXMEM.ALUResult;
	else
		EXMEMtemp.ForwardBMuxResult = FB.WB_MuxOutcome; // the value coming out of the MUX in the WB stage

	// RegDst Mux
	if( IDEX.RegDst == 0 )
		EXMEMtemp.RegDstMuxResult = IDEX.IFID_RegisterRt_toMux;
	else
		EXMEMtemp.RegDstMuxResult = IDEX.IFID_RegisterRd;

	// MUX Before ALU
	int MUXBeforeALUResult;
	if( IDEX.ALUSrc == 0 )
		MUXBeforeALUResult = EXMEMtemp.ForwardBMuxResult;
	else
		MUXBeforeALUResult = IDEX.SignExtendImmediate;

	// ALU Control and ALU
	ALU(IDEX.Opcode, IDEX.SignExtendImmediate, ForwardAResult, MUXBeforeALUResult);

	// Control
	EXMEMtemp.MemRead = IDEX.MemRead;
	EXMEMtemp.MemWrite = IDEX.MemWrite;
	EXMEMtemp.RegWrite = IDEX.RegWrite;
	EXMEMtemp.MemtoReg = IDEX.MemtoReg;
} 

void MemAccess ()
{
	cout << "MemAccess()\n"; /////////////////////////////////////////

	// Deal with DataMem
	if( EXMEM.MemRead == 1 )
		MEMWBtemp.DataMemoryResult = dataMem[EXMEM.ALUResult];

	if( EXMEM.MemWrite == 1 )
		dataMem[EXMEM.ALUResult] = EXMEM.ForwardBMuxResult;

	MEMWBtemp.ALUResult = EXMEM.ALUResult;
	MEMWBtemp.EXMEM_RegisterRd = EXMEM.RegDstMuxResult;

	// Control
	MEMWBtemp.RegWrite = EXMEM.RegWrite;
	MEMWBtemp.MemtoReg = EXMEM.MemtoReg;
	
}

void WriteBack () 
{
	cout << "WriteBack()\n"; /////////////////////////////////////////


	cout << "Memwrite: " << EXMEM.MemWrite << endl;

	cout << "Memtoreg: " << MEMWB.MemtoReg << endl;

	// MUX
	if( MEMWB.MemtoReg == 0 )
		FB.WB_MuxOutcome = MEMWB.DataMemoryResult;
	else
		FB.WB_MuxOutcome = MEMWB.ALUResult;
	cout << "WB_muxresult2: " << FB.WB_MuxOutcome << endl;

	// Write Data, Write Reg
	if( MEMWB.RegWrite == 1 )
		regFile[MEMWB.EXMEM_RegisterRd] = FB.WB_MuxOutcome;

}

void HazardDetectionUnit (char * op) 
{
	cout << "HazardDetectionUnit()\n"; /////////////////////////////////////////

	int binNum = binStringToDec(op,4);

	HAZARD.IFID_RegisterRs = IDEXtemp.IFID_RegisterRs;
	HAZARD.IFID_RegisterRt = IDEXtemp.IFID_RegisterRt_toMux;

	if ( IDEX.MemRead && ( ( IDEX.IFID_RegisterRt_toForward == HAZARD.IFID_RegisterRs) 
		|| ( IDEX.IFID_RegisterRt_toForward == HAZARD.IFID_RegisterRt) ) )
	{
		HAZARD.IFID_Write = 0; // We believe this acts the same way as IF.Flush for the Control Unit
		HAZARD.PCWrite = 0;
		HAZARD.LinetoMux = 1;
	}
	else
	{
		HAZARD.IFID_Write = 1;
		HAZARD.PCWrite = 1;
		HAZARD.LinetoMux = 0;
	}

	// RegEQ - for reduced branch delay
	if ( IDEXtemp.RegisterOne == IDEXtemp.RegisterTwo )
		HAZARD.RegEQ = 1;
	else
		HAZARD.RegEQ = 0;

	// test for nop
	if (binNum == 0)
	{
		HAZARD.IFID_Write = 0;
		HAZARD.LinetoMux = 1;
		HAZARD.Jump = 0;
	}
	else if(binNum == 14)	// PCSrc and Jump/Branch
	{
		HAZARD.PCSrc = 1;
		HAZARD.Branch = HAZARD.RegEQ;
		HAZARD.Jump = 0;
	}
	else if(binNum == 15)
	{
		HAZARD.PCSrc = 1;
		HAZARD.Jump = 1;
	}
	else
	{
		HAZARD.Jump = 0;
		HAZARD.PCSrc = 0;
	}

	HAZARD.IDEX_MemRead = IDEX.MemRead;
	HAZARD.IDEX_RegisterRt = IDEX.IFID_RegisterRt_toMux;
}

void ControlUnit (char * op)
{ 
	cout << "ControlUnit()\n"; /////////////////////////////////////////

	int binNum = binStringToDec(op,4);

	//CONTROL.IF_Flush; // Not using

	//set regwrite for nop
	if(binNum == 0)
		CONTROL.RegWrite = 0;

	// RegDst
	if(binNum <= 6)
		CONTROL.RegDst = 1;
	else
		CONTROL.RegDst = 0;

	// ALUSrc
	if( binNum <= 6 || binNum == 14)
		CONTROL.ALUSrc = 0;
	else
		CONTROL.ALUSrc = 1;

	// MemRead
	if( binNum == 12 )
		CONTROL.MemRead = 1;
	else
		CONTROL.MemRead = 0;

	// MemWrite
	if( binNum == 13 )
		CONTROL.MemWrite = 1;
	else
		CONTROL.MemWrite = 0;

	// RegWrite
	if( binNum >= 13 )
		CONTROL.RegWrite = 0;
	else
		CONTROL.RegWrite = 1;

	// MemtoReg
	if( binNum == 12 )
		CONTROL.MemtoReg = 0;
	else
		CONTROL.MemtoReg = 1;
}

void ForwardUnit ()
{
	cout << "ForwardUnit()\n"; /////////////////////////////////////////

	FORWARD.ForwardA = 0;
	FORWARD.ForwardB = 0;

	FORWARD.IDEX_RegisterRs = IDEX.IFID_RegisterRs;
	FORWARD.IDEX_RegisterRt = IDEX.IFID_RegisterRt_toForward;
	FORWARD.EXMEM_RegWrite = EXMEM.RegWrite; 
	FORWARD.EXMEM_RegisterRd = EXMEM.RegDstMuxResult;
	FORWARD.MEMWB_RegWrite = MEMWB.RegWrite;
	FORWARD.MEMWB_RegisterRd = MEMWB.EXMEM_RegisterRd;

	// EX Hazard
	if ( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult == IDEX.IFID_RegisterRs) )
		FORWARD.ForwardA = 2; // 0x02;
	if ( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult == IDEX.IFID_RegisterRt_toForward) )
		FORWARD.ForwardB = 2; // 0x02;

	// MEM Hazard
	if ( MEMWB.RegWrite && ( MEMWB.EXMEM_RegisterRd != 0 ) && ( MEMWB.EXMEM_RegisterRd == IDEX.IFID_RegisterRs) 
		&& !( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult != IDEX.IFID_RegisterRs) ))
		FORWARD.ForwardA = 1; // 0x01;
	if ( MEMWB.RegWrite && ( MEMWB.EXMEM_RegisterRd != 0 ) && ( MEMWB.EXMEM_RegisterRd == IDEX.IFID_RegisterRt_toForward) 
		&& !( EXMEM.RegWrite && ( EXMEM.RegDstMuxResult != 0 ) && ( EXMEM.RegDstMuxResult != IDEX.IFID_RegisterRt_toForward) ))
		FORWARD.ForwardB = 1; // 0x01;
}

void ALU(int opcode, int SignExtendImmediate, int ALU_A, int ALU_B)
{
	cout << "ALU()\n"; /////////////////////////////////////////
	cout << "ALU_A: " << ALU_A << "  ALU_B: " << ALU_B << endl;
	switch(opcode)
	{
		case 0: // nop
			EXMEMtemp.ALUResult = 0; // Shouldnt matter
			break;
		case 1: // add
			EXMEMtemp.ALUResult = ALU_A + ALU_B;
			break;
		case 2: // sub
			EXMEMtemp.ALUResult = ALU_A - ALU_B;
			break;
		case 3: // and
			EXMEMtemp.ALUResult = ALU_A & ALU_B;
			break;
		case 4: // or
			EXMEMtemp.ALUResult = ALU_A | ALU_B;
			break;
		case 5: // xor
			EXMEMtemp.ALUResult = ALU_A ^ ALU_B;
			break;
		case 6: // slt	
			if( ALU_A < ALU_B)
				EXMEMtemp.ALUResult = 1;
			else
				EXMEMtemp.ALUResult = 0;
			break;
		case 7: // sll
//			EXMEMtemp.ALUResult = ALU_A << SignExtendImmediate; 
			EXMEMtemp.ALUResult = ALU_A << ALU_B; 
			break;
		case 8: // srl	
//			EXMEMtemp.ALUResult = ALU_A >> SignExtendImmediate;
			EXMEMtemp.ALUResult = ALU_A >> ALU_B;
			break;
		case 9: // addi
//			EXMEMtemp.ALUResult = ALU_A + SignExtendImmediate;
			EXMEMtemp.ALUResult = ALU_A + ALU_B;
			break;
		case 10: // subi
//			EXMEMtemp.ALUResult = ALU_A - SignExtendImmediate;
			EXMEMtemp.ALUResult = ALU_A - ALU_B;
			break;
		case 11: // slti	
			if( ALU_A < ALU_B)
				EXMEMtemp.ALUResult = 1;
			else
				EXMEMtemp.ALUResult = 0;
			break;
		case 12: // lw
//			EXMEMtemp.ALUResult = ALU_A + SignExtendImmediate;
			EXMEMtemp.ALUResult = ALU_A + ALU_B;
			break;
		case 13: // sw
//			EXMEMtemp.ALUResult = ALU_A + SignExtendImmediate;
			EXMEMtemp.ALUResult = ALU_A + ALU_B;
			break;
		case 14: // beq
 			EXMEMtemp.ALUResult = ALU_A - ALU_B; // Shouldnt matter
			break;
		case 15: // j
			EXMEMtemp.ALUResult = 0; // Shouldnt matter
			break;
	}
	cout << "EXMEMtemp.ALUResult: " << EXMEMtemp.ALUResult << endl;
}

void printAll()
{
	for(int i = 0; i < REG_SIZE; ++i)
		cout << "regFile[" << i << "] = " << regFile[i] << endl;
	for(int i = 10; i < 30; ++i) // pseudocode uses dataMem locations 16 to 26, so we will print 10 to 30
		cout << "dataMem[" << i << "] = " << dataMem[i] << endl;
	for(int i = 0; i < INST_SIZE; ++i)
		cout << "instMem[" << i << "] = " << instMem[i] << endl;
}
