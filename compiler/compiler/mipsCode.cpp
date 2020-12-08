#include "mipsCode.h"
#include "function.h"
#include "type.h"
#include "midCode.h"
#include "SymbolTable.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <stack>


using namespace std;

vector<mipsCode> mipsCodeTable;
stack<string> paramStack;

Reg tReg[10]; // $t0-$t2寄存器固定用于计算，$t3-$t9用于分配给临时变量；
Reg sReg[8]; // $s0-$s7用于分配给局部变量
bool firstFunc = true;

extern bool debug;
extern ofstream mipsfile;
extern vector<string> stringList;
extern vector<midCode> midCodeTable;
extern SymbolTableManager symbolTableManager;

/////////////// 寄存器池 ///////////////
void initTReg() {
	int i;
	for (i = 0; i < 10; i++) {
		tReg[i].isBusy = false;
		tReg[i].varName = "";
	}
}

void initSReg() {
	int i;
	for (i = 0; i < 8; i++) {
		sReg[i].isBusy = false;
		sReg[i].varName = "";
	}
}

int findEmptyTReg() {
	int i;
	for (i = 3; i < 10; i++) {
		if (!tReg[i].isBusy) {
			return i;
		}
	}
	return -1;
}

int findEmptySReg() {
	int i;
	for (i = 0; i < 8; i++) {
		if (!sReg[i].isBusy) {
			return i;
		}
	}
	return -1;
}

int findNameHasTReg(string name) {
	int i;
	for (i = 3; i < 10; i++) {
		if (tReg[i].varName == name && tReg[i].isBusy) {
			return i;
		}
	}
	return -1;
}

int findNameHasSReg(string name) {
	int i;
	for (i = 0; i < 8; i++) {
		if (sReg[i].varName == name && sReg[i].isBusy) {
			return i;
		}
	}
	return -1;
}

/////////////// 存取数据函数 ///////////////
void loadValue(string name, string* regName, bool value2reg, int* value, bool* isValue, bool assignSReg) {
	*isValue = false;
	if (symbolTableManager.find(name)) {		// 如果在符号表中存在
		STE* ste = symbolTableManager.curSTE;
		if (ste->identType == IdentType::CONST) {		// 是常量
			*value = ste->value;
			*isValue = true;
			if (value2reg) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, *regName, "", "", ste->value));
			}
		}
		else if (ste->isGlobal) {							// 是全局变量
			mipsCodeTable.push_back(mipsCode(mipsInstr::lw, *regName, "$gp", "", 4 * ste->addr));
		}
		else {											//	是中间变量或者是局部变量
			if (name[0] == '#') {						//	中间变量
				int find = findNameHasTReg(name);
				if (find != -1) {
					*regName = "$t" + int2string(find);
					//tReg[find].isBusy = false;
					//tReg[find].varName = "";
				}
				else
				{
					mipsCodeTable.push_back(mipsCode(mipsInstr::lw, *regName, "$fp", "", -4 * ste->addr));
				}
			}
			else										// 局部变量
			{
				int find = findNameHasSReg(name);
				if (find != -1) {
					*regName = "$s" + int2string(find);
				}
				else
				{
					if (assignSReg) {
						find = findEmptySReg();
						if (find != -1) {
							sReg[find].isBusy = true;
							sReg[find].varName = name;
							*regName = "$s" + int2string(find);
							mipsCodeTable.push_back(mipsCode(mipsInstr::lw, *regName, "$fp", "", -4 * ste->addr));
						}
						else
						{
							mipsCodeTable.push_back(mipsCode(mipsInstr::lw, *regName, "$fp", "", -4 * ste->addr));
						}
					}
					else {
						mipsCodeTable.push_back(mipsCode(mipsInstr::lw, *regName, "$fp", "", -4 * ste->addr));
					}
				}
			}
		}
	}
	else {												// 如果在符号表不存在，则为值或者$v0
		if (name == "$v0") {
			*regName = "$v0";
		}
		else {
			*value = string2int(name);
			*isValue = true;
			if (value2reg) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, *regName, "", "", *value));
			}
		}
	}
}

void storeValue(string name, string regName) {
	bool isglobal = false;
	if (symbolTableManager.find(name)) {
		STE* ste = symbolTableManager.curSTE;
		if (ste->isGlobal) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::sw, regName, "$gp", "", 4 * ste->addr));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::sw, regName, "$fp", "", -4 * ste->addr));
		}
	}
	else {
		if (debug) {
			cout << "mipsCode store value error: cannot find var " << name << endl;
		}
	}
}

void assignRdReg(string name, string* regName, int* tfind, int* sfind) {
	*tfind = -1;
	*sfind = -1;
	if (name[0] == '#') {
		*tfind = findEmptyTReg();
		if (*tfind != -1) {
			tReg[*tfind].isBusy = true;
			tReg[*tfind].varName = name;
			*regName = "$t" + int2string(*tfind);
		}
	}
	else if (symbolTableManager.find(name)) {
		if (!symbolTableManager.curSTE->isGlobal) {
			*sfind = findNameHasSReg(name);
			if (*sfind != -1) {
				*regName = "$s" + int2string(*sfind);
			}
			else {
				*sfind = findEmptySReg();
				if (*sfind != -1) {
					sReg[*sfind].isBusy = true;
					sReg[*sfind].varName = name;
					*regName = "$s" + int2string(*sfind);
				}
			}
		}
	}
}

/////////////// mips代码生成 ///////////////
void geneMipsCode() {
	initTReg();
	initSReg();
	// .data段设置
	setData();
	// .text段设置
	mipsCodeTable.push_back(mipsCode(mipsInstr::text, "", "", "", 0));
	// 代码生成
	for (int i = 0; i < midCodeTable.size(); i++) {
		mid2mips(midCodeTable[i]);
	}
	// 输出mips代码
	for (int i = 0; i < mipsCodeTable.size(); i++) {
		mipsCodeTable[i].printMipsInstr();
	}

}

void setData() {
	mipsCodeTable.push_back(mipsCode(mipsInstr::data, "", "", "", 0));
	for (int i = 0; i < stringList.size(); i++) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::asciiz, "string_" + int2string(i), stringList[i], "", 0));
	}
	mipsCodeTable.push_back(mipsCode(mipsInstr::asciiz, "nextline", "\\n", "", 0));
}


/////////////// 将中间代码翻译为mips代码 ///////////////
void mid2mips(midCode mc) {
	switch (mc.op)
	{
	case midCodeOp::ADD:
		dealmidADD(mc);
		break;
	case midCodeOp::SUB:
		dealmidSUB(mc);
		break;
	case midCodeOp::MULTOP:
		dealmidMULT(mc);
		break;
	case midCodeOp::DIVOP:
		dealmidDIV(mc);
		break;
	case midCodeOp::MOVE:
		dealmidMOVE(mc);
		break;
	case midCodeOp::FUNC:
		dealmidFUNC(mc);
		break;
	case midCodeOp::SCAN:
		dealmidSCAN(mc);
		break;
	case midCodeOp::PRINT:
		dealmidPRINT(mc);
		break;
	case midCodeOp::EXIT:
		dealmidEXIT(mc);
		break;
	case midCodeOp::ARRSAVE:
		dealmidARRSAVE(mc);
		break;
	case midCodeOp::ARRLOAD:
		dealmidARRLOAD(mc);
		break;
	case midCodeOp::BEQ:
		dealmidBEQ(mc);
		break;
	case midCodeOp::BNE:
		dealmidBNE(mc);
		break;
	case midCodeOp::BLT:
		dealmidBLT(mc);
		break;
	case midCodeOp::BLE:
		dealmidBLE(mc);
		break;
	case midCodeOp::LABEL:
		dealmidLABEL(mc);
		break;
	case midCodeOp::GOTO:
		dealmidGOTO(mc);
		break;
	case midCodeOp::ENDFUNC:
		dealmidENDFUNC(mc);
		break;
	case midCodeOp::RET:
		dealmidRET(mc);
		break;
	case midCodeOp::PUSHPARAM:
		dealmidPUSHPARAM(mc);
		break;
	case midCodeOp::CALL:
		dealmidCALL(mc);
		break;
	case midCodeOp::MIDPARAM:
		dealmidPARAM(mc);
		break;
	default:
		break;
	}
}

void dealmidADD(midCode mc) {
	string rd = "$t0", rs = "$t1", rt = "$t2";
	bool isvalue1 = false, isvalue2 = false;
	int tfind = -1, sfind = -1, value1 = 0, value2 = 0;
	assignRdReg(mc.z, &rd, &tfind, &sfind);
	loadValue(mc.x, &rs, false, &value1, &isvalue1);
	loadValue(mc.y, &rt, false, &value2, &isvalue2);
	if (isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", value1 + value2));
	}
	else if (isvalue1 && !isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, rd, rt, "", value1));
	}
	else if (!isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, rd, rs, "", value2));
	}
	else {
		mipsCodeTable.push_back(mipsCode(mipsInstr::add, rd, rs, rt, 0));
	}
	if (mc.z[0] == '#') {
		if (tfind == -1) {
			storeValue(mc.z, rd);
		}
	}
	else if (symbolTableManager.find(mc.z)) {
		if (symbolTableManager.curSTE->isGlobal || sfind == -1) {
			storeValue(mc.z, rd);
		}
	}
}

void dealmidSUB(midCode mc) {
	string rd = "$t0", rs = "$t1", rt = "$t2";
	bool isvalue1 = false, isvalue2 = false;
	int tfind = -1, sfind = -1, value1 = 0, value2 = 0;
	assignRdReg(mc.z, &rd, &tfind, &sfind);
	loadValue(mc.x, &rs, false, &value1, &isvalue1);
	loadValue(mc.y, &rt, false, &value2, &isvalue2);
	if (isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", value1 - value2));
	}
	else if (isvalue1 && !isvalue2) { // value1 - $t2
		if (value1 == 0) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::sub, rd, "$0", rt, 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::addi, rd, rt, "", -value1));
			mipsCodeTable.push_back(mipsCode(mipsInstr::sub, rd, "$0", rd, 0));
		}
		
	}
	else if (!isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, rd, rs, "", -value2));
	}
	else {
		mipsCodeTable.push_back(mipsCode(mipsInstr::sub, rd, rs, rt, 0));
	}
	if (mc.z[0] == '#') {
		if (tfind == -1) {
			storeValue(mc.z, rd);
		}
	}
	else if (symbolTableManager.find(mc.z)) {
		if (symbolTableManager.curSTE->isGlobal || sfind == -1) {
			storeValue(mc.z, rd);
		}
	}
}

void dealmidMULT(midCode mc) {
	string rd = "$t0", rs = "$t1", rt = "$t2";
	bool isvalue1 = false, isvalue2 = false;
	int tfind = -1, sfind = -1, value1 = 0, value2 = 0;
	assignRdReg(mc.z, &rd, &tfind, &sfind);
	loadValue(mc.x, &rs, false, &value1, &isvalue1);
	loadValue(mc.y, &rt, false, &value2, &isvalue2);
	if (isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", value1 * value2));
	}
	else if (isvalue1 && !isvalue2) { // value1 * $t2
		if (value1 == 0) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", 0));
		}
		else if (value1 == 1) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::move, rd, rt, "", 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rs, "", "", value1));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mul, rd, rs, rt, 0));
		}

	}
	else if (!isvalue1 && isvalue2) {
		if (value2 == 0) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", 0));
		}
		else if (value2 == 1) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::move, rd, rs, "", 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rt, "", "", value2));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mul, rd, rs, rt, 0));
		}
	}
	else {
		mipsCodeTable.push_back(mipsCode(mipsInstr::mul, rd, rs, rt, 0));
	}
	if (mc.z[0] == '#') {
		if (tfind == -1) {
			storeValue(mc.z, rd);
		}
	}
	else if (symbolTableManager.find(mc.z)) {
		if (symbolTableManager.curSTE->isGlobal || sfind == -1) {
			storeValue(mc.z, rd);
		}
	}
}

void dealmidDIV(midCode mc) {
	string rd = "$t0", rs = "$t1", rt = "$t2";
	bool isvalue1 = false, isvalue2 = false;
	int tfind = -1, sfind = -1, value1 = 0, value2 = 0;
	assignRdReg(mc.z, &rd, &tfind, &sfind);
	loadValue(mc.x, &rs, false, &value1, &isvalue1);
	loadValue(mc.y, &rt, false, &value2, &isvalue2);
	if (isvalue1 && isvalue2) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", value1 / value2));
	}
	else if (isvalue1 && !isvalue2) { // value1 - $t2
		if (value1 == 0) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rs, "", "", value1));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mipsdiv, "", rs, rt, 0));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mflo, rd, "", "", 0));
		}

	}
	else if (!isvalue1 && isvalue2) {
		if (value2 == 1) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::move, rd, rs, "", 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::li, rt, "", "", value2));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mipsdiv, "", rs, rt, 0));
			mipsCodeTable.push_back(mipsCode(mipsInstr::mflo, rd, "", "", 0));
		}
	}
	else {
		mipsCodeTable.push_back(mipsCode(mipsInstr::mipsdiv, "", rs, rt, 0));
		mipsCodeTable.push_back(mipsCode(mipsInstr::mflo, rd, "", "", 0));
	}
	if (mc.z[0] == '#') {
		if (tfind == -1) {
			storeValue(mc.z, rd);
		}
	}
	else if (symbolTableManager.find(mc.z)) {
		if (symbolTableManager.curSTE->isGlobal || sfind == -1) {
			storeValue(mc.z, rd);
		}
	}
}

void dealmidMOVE(midCode mc) {
	int value;
	string rd, _rd;
	int tfind, sfind;
	bool isvalue, isglobal;
	if (mc.z[0] == '#') {				//	中间变量
		tfind = findEmptyTReg();
		if (tfind != -1) {
			tReg[tfind].isBusy = true;
			tReg[tfind].varName = mc.z;
			rd = "$t" + int2string(tfind);
			_rd = rd;
			loadValue(mc.x, &rd, true, &value, &isvalue);
			if (rd != _rd) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::move, _rd, rd, "", 0));
			}
		}
		else {
			rd = "$t0";
			loadValue(mc.x, &rd, true, &value, &isvalue);
			storeValue(mc.z, rd);
		}
	}
	else if (symbolTableManager.find(mc.z)) {
		STE* ste = symbolTableManager.curSTE;
		if (!ste->isGlobal) {				// 局部变量
			sfind = findNameHasSReg(mc.z);
			if (sfind != -1) {
				rd = "$s" + int2string(sfind);
				_rd = rd;
				loadValue(mc.x, &rd, true, &value, &isvalue);
				if (rd != _rd) {
					mipsCodeTable.push_back(mipsCode(mipsInstr::move, _rd, rd, "", 0));
				}
			}
			else {
				sfind = findEmptySReg();
				if (sfind != -1) {
					sReg[sfind].isBusy = true;
					sReg[sfind].varName = mc.z;
					rd = "$s" + int2string(sfind);
					_rd = rd;
					loadValue(mc.x, &rd, true, &value, &isvalue);
					if (rd != _rd) {
						mipsCodeTable.push_back(mipsCode(mipsInstr::move, _rd, rd, "", 0));
					}
				}
				else {
					rd = "$t0";
					loadValue(mc.x, &rd, true, &value, &isvalue);
					storeValue(mc.z, rd);
				}
			}
		}
		else {						// 全局变量
			rd = "$t0";
			loadValue(mc.x, &rd, true, &value, &isvalue);
			storeValue(mc.z, rd);
		}
	}
	else if (mc.z == "$v0") {		// $v0
		rd = "$v0";
		loadValue(mc.x, &rd, true, &value, &isvalue);
		if (rd != "$v0") {
			mipsCodeTable.push_back(mipsCode(mipsInstr::move, "$v0", rd, "", 0));
		}
	}
}

void dealmidFUNC(midCode mc) {
	if (firstFunc) {
		mipsCodeTable.push_back(mipsCode(mipsInstr::j, "main", "", "", 0));		// 跳转到主函数
		firstFunc = false;
	}
	mipsCodeTable.push_back(mipsCode(mipsInstr::label, mc.z, "", "", 0));
	if (mc.z == "main") {
		symbolTableManager.find("main");
		int length = symbolTableManager.curSTE->length;
		mipsCodeTable.push_back(mipsCode(mipsInstr::move, "$fp", "$sp", "", 0));
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$sp", "$sp", "", -4 * length - 8));
	}
	symbolTableManager.gotoFunc(mc.z);
	// 新函数定义，清空ST寄存器
	initTReg();
	initSReg();
}

void dealmidENDFUNC(midCode mc) {
	symbolTableManager.returnRoot();
	mipsCodeTable.push_back(mipsCode(mipsInstr::jr, "$ra", "", "", 0));
}

void dealmidSCAN(midCode mc) {
	if (symbolTableManager.find(mc.z)) {
		STE* ste = symbolTableManager.curSTE;
		if (ste->isGlobal) {
			if (mc.x == "int") {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 5));
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 12));
			}
			mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
			mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$v0", "$gp", "", 4 * ste->addr));
		}
		else {
			string rd;
			if (mc.x == "int") {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 5));
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 12));
			}
			mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
			int sfind = findNameHasSReg(mc.z);
			if (sfind != -1) {
				rd = "$s" + int2string(sfind);
				mipsCodeTable.push_back(mipsCode(mipsInstr::move, rd, "$v0", "", 0));
			}
			else {
				sfind = findEmptySReg();
				if (sfind != -1) {
					sReg[sfind].isBusy = true;
					sReg[sfind].varName = mc.z;
					rd = "$s" + int2string(sfind);
					mipsCodeTable.push_back(mipsCode(mipsInstr::move, rd, "$v0", "", 0));
				}
				else {
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$v0", "$fp", "", -4 * ste->addr));
				}
			}
		}
	}
}

void dealmidPRINT(midCode mc) {
	int value;
	bool isvalue;
	if (mc.x == "int" || mc.x == "char") {
		string rd = "$a0";
		loadValue(mc.z, &rd, true, &value, &isvalue);
		if (rd != "$a0") {
			mipsCodeTable.push_back(mipsCode(mipsInstr::move, "$a0", rd, "", 0));
		}
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", (mc.x == "int" ? 1 : 11)));
		mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
	}
	else if (mc.x == "string") {
		for (int i = 0; i < stringList.size(); i++) {
			if (stringList[i] == mc.z) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::la, "$a0", "string_"+int2string(i), "", 0));
				break;
			}
		}
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 4));
		mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
	}
	else if (mc.x == "nextline") {
		mipsCodeTable.push_back(mipsCode(mipsInstr::la, "$a0", "nextline", "", 0));
		mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 4));
		mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
	}
}

void dealmidEXIT(midCode mc) {
	mipsCodeTable.push_back(mipsCode(mipsInstr::li, "$v0", "", "", 10));
	mipsCodeTable.push_back(mipsCode(mipsInstr::syscall, "", "", "", 0));
}

void dealmidARRSAVE(midCode mc) {
	// mc.z[mc.x] = mc.y
	// mc.x = $t0
	// mc.y = $t1
	string rs = "$t0", rt = "$t1";
	bool isvalue1 = false, isvalue2 = false;
	int value1 = 0, value2 = 0;
	loadValue(mc.x, &rs, false, &value1, &isvalue1);
	loadValue(mc.y, &rt, false, &value2, &isvalue2);
	if (symbolTableManager.find(mc.z)) {
		STE* ste = symbolTableManager.curSTE;
		string baseAddr = ste->isGlobal ? "$gp" : "$fp";
		int off = ste->isGlobal ? 4 : -4;
		if (!isvalue1) {	// 数组下标保存在rs寄存器中
			mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$t2", baseAddr, "", off * ste->addr));
			mipsCodeTable.push_back(mipsCode(mipsInstr::sll, "$t0", rs, "", 2));
			if (ste->isGlobal) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::add, "$t2", "$t2", "$t0", 0));
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::sub, "$t2", "$t2", "$t0", 0));
			}

			if (isvalue2) {
				if (value2 == 0) {
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$0", "$t2", "", 0));
				}
				else {
					mipsCodeTable.push_back(mipsCode(mipsInstr::li, rt, "", "", value2));
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rt, "$t2", "", 0));
				}
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rt, "$t2", "", 0));
			}
		}
		else {
			if (isvalue2) {
				if (value2 == 0) {
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$0", baseAddr, "", off * (ste->addr + value1)));
				}
				else {
					mipsCodeTable.push_back(mipsCode(mipsInstr::li, rt, "", "", value2));
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rt, baseAddr, "", off * (ste->addr + value1)));
				}
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rt, baseAddr, "", off * (ste->addr + value1)));
			}
		}
	}
}

void dealmidARRLOAD(midCode mc) {
	// mc.z = mc.x[mc.y]
	// mc.z = $t0
	// mc.y = $t1
	string rd = "$t0", rt = "$t1";
	int value;
	bool isvalue = false;
	int tfind = -1, sfind = -1;
	assignRdReg(mc.z, &rd, &tfind, &sfind);
	loadValue(mc.y, &rt, false, &value, &isvalue);
	if (symbolTableManager.find(mc.x)) {
		STE* ste = symbolTableManager.curSTE;
		string baseAddr = ste->isGlobal ? "$gp" : "$fp";
		int off = ste->isGlobal ? 4 : -4;
		if (!isvalue) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$t2", baseAddr, "", off * ste->addr));
			mipsCodeTable.push_back(mipsCode(mipsInstr::sll, "$t1", rt, "", 2));
			if (ste->isGlobal) {
				mipsCodeTable.push_back(mipsCode(mipsInstr::add, "$t2", "$t2", "$t1", 0));
			}
			else {
				mipsCodeTable.push_back(mipsCode(mipsInstr::sub, "$t2", "$t2", "$t1", 0));
			}
			mipsCodeTable.push_back(mipsCode(mipsInstr::lw, rd, "$t2", "", 0));
		}
		else {
			mipsCodeTable.push_back(mipsCode(mipsInstr::lw, rd, baseAddr, "", off * (ste->addr + value)));
		}
		if (mc.z[0] == '#') {
			if (tfind == -1) {
				storeValue(mc.z, rd);
			}
		}
		else if (symbolTableManager.find(mc.z)) {
			if (symbolTableManager.curSTE->isGlobal || sfind == -1) {
				storeValue(mc.z, rd);
			}
		}
	}
}

void dealmidBEQ(midCode mc) {
	// goto mc.z if mc.x = mc.y
	string rs = "$t0", rt = "$t1";
	bool isvalue1 = false, isvalue2 = false;
	int value1 = 0, value2 = 0;
	loadValue(mc.x, &rs, true, &value1, &isvalue1);
	loadValue(mc.y, &rt, true, &value2, &isvalue2);
	mipsCodeTable.push_back(mipsCode(mipsInstr::beq, mc.z, rs, rt, 0));
}

void dealmidBNE(midCode mc) {
	// goto mc.z if mc.x = mc.y
	string rs = "$t0", rt = "$t1";
	bool isvalue1 = false, isvalue2 = false;
	int value1 = 0, value2 = 0;
	loadValue(mc.x, &rs, true, &value1, &isvalue1);
	loadValue(mc.y, &rt, true, &value2, &isvalue2);
	mipsCodeTable.push_back(mipsCode(mipsInstr::bne, mc.z, rs, rt, 0));
}

void dealmidBLT(midCode mc) {
	// goto mc.z if mc.x = mc.y
	string rs = "$t0", rt = "$t1";
	bool isvalue1 = false, isvalue2 = false;
	int value1 = 0, value2 = 0;
	loadValue(mc.x, &rs, true, &value1, &isvalue1);
	loadValue(mc.y, &rt, true, &value2, &isvalue2);
	mipsCodeTable.push_back(mipsCode(mipsInstr::blt, mc.z, rs, rt, 0));
}

void dealmidBLE(midCode mc) {
	// goto mc.z if mc.x = mc.y
	string rs = "$t0", rt = "$t1";
	bool isvalue1 = false, isvalue2 = false;
	int value1 = 0, value2 = 0;
	loadValue(mc.x, &rs, true, &value1, &isvalue1);
	loadValue(mc.y, &rt, true, &value2, &isvalue2);
	mipsCodeTable.push_back(mipsCode(mipsInstr::ble, mc.z, rs, rt, 0));
}

void dealmidLABEL(midCode mc) {
	mipsCodeTable.push_back(mipsCode(mipsInstr::label, mc.z, "", "", 0));
}

void dealmidGOTO(midCode mc) {
	mipsCodeTable.push_back(mipsCode(mipsInstr::j, mc.z, "", "", 0));
}

void dealmidRET(midCode mc) {
	mipsCodeTable.push_back(mipsCode(mipsInstr::jr, "$ra", "", "", 0));
}

void dealmidPUSHPARAM(midCode mc) {
	paramStack.push(mc.z);
}

void dealmidCALL(midCode mc) {
	string rd;
	int value;
	bool isvalue = false;
	if (symbolTableManager.find(mc.z)) {
		STE* ste = symbolTableManager.curSTE;
		int paramSize = ste->args.size();
		while (paramSize)
		{
			rd = "$t0";
			paramSize--;
			if (paramStack.empty()) {
				cout << "mips: 函数调用参数错误" << endl;
			}
			else {
				string param = paramStack.top();
				paramStack.pop();
				loadValue(param, &rd, false, &value, &isvalue);
				if (isvalue) {
					if (value == 0) {
						mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$0", "$sp", "", -4 * paramSize));
					}
					else {
						mipsCodeTable.push_back(mipsCode(mipsInstr::li, rd, "", "", value));
						mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rd, "$sp", "", -4 * paramSize));
					}
				}
				else {
					mipsCodeTable.push_back(mipsCode(mipsInstr::sw, rd, "$sp", "", -4 * paramSize));
				}
			}
		}

		vector<string> usedReg;
		for (int i = 3; i < 10; i++) {
			if (tReg[i].isBusy) {
				usedReg.push_back("$t" + int2string(i));
			}
		}
		for (int i = 0; i < 8; i++) {
			if (sReg[i].isBusy) {
				usedReg.push_back("$s" + int2string(i));
			}
		}
		// 保存现场
		int len = 4 * ste->length + 4 * usedReg.size() + 8;
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$sp", "$sp", "", -len));
		mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$ra", "$sp", "", 4));
		mipsCodeTable.push_back(mipsCode(mipsInstr::sw, "$fp", "$sp", "", 8));
		for (int i = 0; i < usedReg.size(); i++) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::sw, usedReg[i], "$sp", "", 8 + 4 * (i + 1)));
		}
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$fp", "$sp", "", len));
		// 调用函数
		mipsCodeTable.push_back(mipsCode(mipsInstr::jal, mc.z, "", "", 0));
		// 恢复现场
		for (int i = 0; i < usedReg.size(); i++) {
			mipsCodeTable.push_back(mipsCode(mipsInstr::lw, usedReg[i], "$sp", "", 8 + 4 * (i + 1)));
		}
		mipsCodeTable.push_back(mipsCode(mipsInstr::lw, "$fp", "$sp", "", 8));
		mipsCodeTable.push_back(mipsCode(mipsInstr::lw, "$ra", "$sp", "", 4));
		mipsCodeTable.push_back(mipsCode(mipsInstr::addi, "$sp", "$sp", "", len));
	}
}

void dealmidPARAM(midCode mc) {
	// 对于函数的参数，若能分配S寄存器，则先分配寄存器
	int sfind = findEmptySReg();
	if (sfind != -1) {
		sReg[sfind].isBusy = true;
		sReg[sfind].varName = mc.z;
		string rd = "$s" + int2string(sfind);
		symbolTableManager.find(mc.z);
		int addr = symbolTableManager.curSTE->addr;
		mipsCodeTable.push_back(mipsCode(mipsInstr::lw, rd, "$fp", "", -4 * addr));
	}
}

/////////////// 输出mips指令 ///////////////
void mipsCode::printMipsInstr() {
	switch (this->op)
	{
	case mipsInstr::add:
		mipsfile << "addu " << rd << ", " << rs << ", " << rt << endl;
		break;
	case mipsInstr::addi:
		mipsfile << "addiu " << rd << ", " << rs << ", " << int2string(imm) << endl;
		break;
	case mipsInstr::sub:
		mipsfile << "subu " << rd << ", " << rs << ", " << rt << endl;
		break;
	case mipsInstr::mul:
		mipsfile << "mul " << rd << ", " << rs << ", " << rt << endl;
		break;
	case mipsInstr::mipsdiv:
		mipsfile << "div "  << rs << ", " << rt << endl;
		break;
	case mipsInstr::mfhi:
		mipsfile << "mfhi " << rd  << endl;
		break;
	case mipsInstr::mflo:
		mipsfile << "mflo " << rd << endl;
		break;
	case mipsInstr::sll:
		mipsfile << "sll " << rd << ", " << rs << ", " << int2string(imm) << endl;
		break;
	case mipsInstr::j:
		mipsfile << "j " << rd << endl;
		break;
	case mipsInstr::jal:
		mipsfile << "jal " << rd << endl;
		break;
	case mipsInstr::jr:
		mipsfile << "jr " << rd << endl;
		break;
	case mipsInstr::lw:
		mipsfile << "lw " << rd << ", " << imm << "(" << rs << ")" << endl;
		break;
	case mipsInstr::sw:
		mipsfile << "sw " << rd << ", " << imm << "(" << rs << ")" << endl;
		break;
	case mipsInstr::syscall:
		mipsfile << "syscall" << endl;
		break;
	case mipsInstr::li:
		mipsfile << "li " << rd << ", " << imm << endl;
		break;
	case mipsInstr::la:
		mipsfile << "la " << rd << ", " << rs << endl;
		break;
	case mipsInstr::move:
		mipsfile << "move " << rd << ", " << rs << endl;
		break;
	case mipsInstr::beq:
		mipsfile << "beq " << rs << ", " << rt << ", " << rd << endl;
		break;
	case mipsInstr::bne:
		mipsfile << "bne " << rs << ", " << rt << ", " << rd << endl;
		break;
	case mipsInstr::blt:
		mipsfile << "blt " << rs << ", " << rt << ", " << rd << endl;
		break;
	case mipsInstr::ble:
		mipsfile << "ble " << rs << ", " << rt << ", " << rd << endl;
		break;
	case mipsInstr::data:
		mipsfile << ".data" << endl;
		break;
	case mipsInstr::text:
		mipsfile << ".text" << endl;
		break;
	case mipsInstr::asciiz:
		mipsfile << rd << ": .asciiz \"" << rs << "\"" << endl;
		break;
	case mipsInstr::label:
		mipsfile << rd << ":" << endl;
		break;
	default:
		break;
	}
}