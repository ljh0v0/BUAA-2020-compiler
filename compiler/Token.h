#pragma once

#include <string>
#include <fstream>

using namespace std;

class Token
{
public:
	Token(int symbol, string tokenbuf, int linenumber);
	bool isType(int symbol);
	void setValue(int value);
	void printToken();
	int getType();
	string getTokenStr();
private:
	int symbol;
	string tokenStr;
	int lineNumber;
	int value;
};

