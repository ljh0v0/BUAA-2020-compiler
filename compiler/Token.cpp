#include "Token.h"

using namespace std;

extern ofstream outfile;
extern string type_code[];

Token::Token(int symbol, string tokenbuf, int linenumber) {
	this->symbol = symbol;
	this->tokenStr = tokenbuf;
	this->lineNumber = linenumber;
}

bool Token::isType(int symbol) {
	return this->symbol == symbol;
}

void Token::setValue(int value) {
	this->value = value;
}

void Token::printToken() {
	outfile << type_code[this->symbol] << " " << this->tokenStr.c_str() << endl;
}

int Token::getType() {
	return symbol;
}

string Token::getTokenStr() {
	return tokenStr;
}