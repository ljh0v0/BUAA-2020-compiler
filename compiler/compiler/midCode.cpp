#include <iostream>
#include <fstream>
#include <vector>
#include "midCode.h"

using namespace std;

extern ofstream midcodefile;
vector<midCode> midCodeTable;

void outputMidCode(){
	vector<midCode>::iterator iter = midCodeTable.begin();
	while (iter != midCodeTable.end()) {
		midCode mc = *iter;
		switch (mc.op)
		{
		case midCodeOp::ADD:
			midcodefile << (mc.z) << "=" << (mc.x) << "+" << (mc.y) << endl;
			break;
		case midCodeOp::SUB:
			midcodefile << (mc.z) << " = " << (mc.x) << " - " << (mc.y) << endl;
			break;
		case midCodeOp::MULTOP:
			midcodefile << (mc.z) << " = " << (mc.x) << " * " << (mc.y) << endl;
			break;
		case midCodeOp::DIVOP:
			midcodefile << (mc.z) << " = " << (mc.x) << " / " << (mc.y) << endl;
			break;
		case midCodeOp::ARRSAVE:
			midcodefile << (mc.z) << "[" << (mc.x) << "] = " << (mc.y) << endl;
			break;
		case midCodeOp::ARRLOAD:
			midcodefile << (mc.z) << " = " << (mc.x) << "[" << (mc.y) << "]" << endl;
			break;
		case midCodeOp::MOVE:
			midcodefile << (mc.z) << " = " << (mc.x) << endl;
			break;
		case midCodeOp::GOTO:
			midcodefile << "goto " << (mc.z) << endl;
			break;
		case midCodeOp::BEQ:
			midcodefile << "goto " << (mc.z) << " if " << (mc.x) << " = " << (mc.y) << endl;
			break;
		case midCodeOp::BNE:
			midcodefile << "goto " << (mc.z) << " if " << (mc.x) << " != " << (mc.y) << endl;
			break;
		case midCodeOp::BLT:
			midcodefile << "goto " << (mc.z) << " if " << (mc.x) << " < " << (mc.y) << endl;
			break;
		case midCodeOp::BLE:
			midcodefile << "goto " << (mc.z) << " if " << (mc.x) << " <= " << (mc.y) << endl;
			break;
		case midCodeOp::MIDVAR:
			midcodefile << "var " << (mc.z) << endl;
			break;
		case midCodeOp::MIDPARAM:
			midcodefile << "param " << (mc.z) << endl;
			break;
		case midCodeOp::PUSHPARAM:
			midcodefile << "push param " << (mc.z) << endl;
			break;
		case midCodeOp::CALL:
			midcodefile << "call func " << (mc.z) << endl;
			break;
		case midCodeOp::FUNC:
			midcodefile << "func " << (mc.z) << " :" << endl;
			break;
		case midCodeOp::ENDFUNC:
			midcodefile << "endfunc"<< endl;
			break;
		case midCodeOp::RET:
			midcodefile << "return"<< endl;
			break;
		case midCodeOp::EXIT:
			midcodefile << "exit"<< endl;
			break;
		case midCodeOp::SCAN:
			midcodefile << "scan " << (mc.x) << " " << (mc.z) << endl;
			break;
		case midCodeOp::PRINT:
			midcodefile << "print " << (mc.x) << " " << (mc.z) << endl;
			break;
		case midCodeOp::LABEL:
			midcodefile << (mc.z) << " :" << endl;
			break;
		default:
			break;
		}
		iter++;
	}
}
