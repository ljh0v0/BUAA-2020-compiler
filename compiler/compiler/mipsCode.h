#pragma once
#include "midCode.h"
#include <string>
using namespace std;

enum mipsInstr {
	add,
	addi,
	sub,
	mul,
	mipsdiv,
	mfhi,
	mflo,
	sll,
	j,
	jal,
	jr,
	lw,
	sw,
	syscall,
	li,
	la,
	move,
	beq,
	bne,
	blt,
	ble,
	data,
	text,
	asciiz,
	label
};

class mipsCode
{
public:
	mipsInstr op;
	string rs;
	string rt;
	string rd;
	int imm; // 立即数
	mipsCode(mipsInstr o, string rd, string rs, string rt, int i) :
		op(o), rd(rd), rs(rs), rt(rt), imm(i) {};
	void printMipsInstr();
};

/////////////// 寄存器池 ///////////////
void initTReg();
void initSReg();
int findEmptyTReg();
int findEmptySReg();
int findNameHasTReg(string name);
int findNameHasSReg(string name);

/////////////// 存取数据函数 ///////////////
void loadValue(string name, string* regName, bool value2reg, int* value, bool* isValue, bool assignSReg = true);
void storeValue(string name, string regName);
void assignRdReg(string name, string* regName, int* tfind, int* sfind);

/////////////// mips代码生成 ///////////////
void geneMipsCode();
void setData();

/////////////// 将中间代码翻译为mips代码 ///////////////
void mid2mips(midCode mc);
void dealmidADD(midCode mc);
void dealmidSUB(midCode mc);
void dealmidMULT(midCode mc);
void dealmidDIV(midCode mc);
void dealmidMOVE(midCode mc);
void dealmidFUNC(midCode mc);
void dealmidSCAN(midCode mc);
void dealmidPRINT(midCode mc);
void dealmidEXIT(midCode mc);
void dealmidARRSAVE(midCode mc);
void dealmidARRLOAD(midCode mc);
void dealmidBEQ(midCode mc);
void dealmidBNE(midCode mc);
void dealmidBLT(midCode mc);
void dealmidBLE(midCode mc);
void dealmidLABEL(midCode mc);
void dealmidGOTO(midCode mc);
void dealmidENDFUNC(midCode mc);
void dealmidRET(midCode mc);
void dealmidPUSHPARAM(midCode mc);
void dealmidCALL(midCode mc);
void dealmidPARAM(midCode mc);

