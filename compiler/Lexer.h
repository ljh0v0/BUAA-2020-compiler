#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "Token.h"
#include "type.h"

using namespace std;

extern string type_code[];
extern string reserved_words[];

extern vector<Token> tokenList;
extern int tokenptr;

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

