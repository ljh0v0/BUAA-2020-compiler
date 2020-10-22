#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "Token.h"

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

using namespace std;


extern vector<Token> tokenList;
extern string type_code[];
extern string reserved_words[];

class Lexer
{
public:
	Lexer();
	void clearToken();
	void retract();
	bool isDigit(char c);
	bool isLetter(char c);
	int reserver();
	int transNum();
	void dealLetter();
	void dealDigit();
	void dealCharConst();
	void dealStrConst();
	void getsym();
	void lexicalAnalyse();
private:
	string tokenbuf;
	int number;
	int symbol;
	int linenumber;
	char ch;
};

