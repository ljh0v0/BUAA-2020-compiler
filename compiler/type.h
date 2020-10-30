#pragma once

#include <string>
#include <vector>
#include <map>

using namespace std;

#define ERROR -1

#define IDENFR	0	// 标识符
#define INTCON	1	// 整形常量
#define CHARCON	2	// 字符常量
#define STRCON	3	// 字符串

// reserved words
#define CONSTTK	4	// const
#define INTTK	5	// int
#define CHARTK	6	// char
#define VOIDTK	7	// void
#define MAINTK	8	// main
#define IFTK	9	// if
#define ELSETK	10	// else
#define SWITCHTK	11	// switch
#define CASETK	12	// case
#define DEFAULTTK	13	// default
#define WHILETK	14	// while
#define FORTK	15	// for
#define SCANFTK	16	// scanf
#define PRINTFTK	17	// printf
#define RETURNTK	18	// return

#define PLUS	19	// +
#define MINU	20	// -
#define MULT	21	// *
#define DIV		22	// /
#define LSS		23	// <
#define LEQ		24	// <=
#define GRE		25	// >
#define GEQ		26	// >=
#define EQL		27	// ==
#define NEQ		28	// !=
#define COLON	29	// :
#define ASSIGN	30	// =
#define SEMICN	31	// ;
#define COMMA	32	// ,
#define LPARENT	33	// (
#define RPARENT	34	// )
#define LBRACK	35	// [
#define RBRACK	36	// ]
#define LBRACE	37	// {
#define RBRACE	38	// }
#define EOFSYM	39	// EOF

// 错误类型
#define NO_ERROR		0
#define ILLEGAL_SYMBOL	'a'
#define DUP_DEFINE		'b'
#define NO_DEFINE		'c'
#define PARAMS_NUM_UNMATCH		'd'
#define PARAMS_TYPE_UNMATCH		'e'
#define ILLEGAL_CONDITION_TYPE	'f'
#define NO_RETURN_FUNC_ERROR	'g'
#define RETURN_FUNC_ERROR		'h'
#define ARRAY_INDEX_TYPE_UNMATCH	'i'
#define ALTER_CONST_VALUE		'j'
#define MISSING_SEMICN			'k'
#define MISSING_RPARENT			'l'
#define MISSING_RBRACK			'm'
#define ARRAY_INIT_NUM_UNMATCH	'n'
#define CONST_TYPE_UNMATCH		'o'
#define MISSING_DEFAULT			'p'			

enum IdentType
{
	CONST,
	VAR,
	ARRAY,
	FUNCTION,
	PROCEDURE,
	PARAM
};

enum ValueType
{
	INTEGER,
	CHAR,
	VOID
};

struct Argument
{
	string name;
	ValueType type;
};

struct ArrayInfo
{
	int array_row;
	int array_col;
	int array_dim;
};

struct STE
{
	string name;			//标识符名
	IdentType identType;	//标识符类型
	ValueType valueType;	//值类型
	int value;				// 常量的值
	ArrayInfo arrayInfo;	//数组维度
	vector<Argument> args;	//参数表
	STE* father;			//父节点
	map<string, STE> children;	//子节点列表

public:
	STE() :father(NULL) {};
};