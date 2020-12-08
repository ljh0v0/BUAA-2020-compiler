#pragma once
#include "type.h"

using namespace std;

enum midCodeOp {
	ADD,	// z = x + y
	SUB,	// z = x - y
	MULTOP,	// z = x * y
	DIVOP,	// z = x / y
	ARRSAVE,	// z[x] = y
	ARRLOAD,	// z = x[y]
	MOVE,	// z = x 如果z = $v0 则为函数返回结果；如果 x = &v0，则为移出函数返回值
	GOTO,	// 无条件跳转至 z, goto z
	BEQ,	// GOTO z IF x = y
	BNE,	// GOTO z IF x != y
	BLT,	// GOTO z IF x < y
	BLE,	// GOTO z IF x <= y
	MIDVAR,	// var z(name)
	MIDPARAM,	// param z(name)
	PUSHPARAM,	//函数参数压栈 push z
	CALL,	//函数调用 call z
	FUNC,	//函数定义 func z:
	ENDFUNC,	//函数定义结束 endfunc
	RET,	// 函数返回 return;
	EXIT,	// 程序结束（main函数返回） exit
	SCAN,	// 读取变量z，类型为x，scan x z
	PRINT,	// 打印变量z，类型为x，print x  z
	LABEL,	//设置标签z， z:
};

class midCode	// z = x op y
{
public:
	midCodeOp op;
	string z;
	string x;
	string y;
	midCode(midCodeOp o, string z1 = "", string x1 = "", string y1 = "") :
		op(o), z(z1), x(x1), y(y1){}
};

void outputMidCode();