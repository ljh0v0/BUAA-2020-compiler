#include "Lexer.h"

using namespace std;

extern ifstream infile;

string type_code[] = { "IDENFR", "INTCON", "CHARCON", "STRCON", "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK", "IFTK",
					"ELSETK", "SWITCHTK", "CASETK", "DEFAULTTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK", "PLUS",
					"MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "COLON",
					"ASSIGN", "SEMICN", "COMMA", "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE" };

string reserved_words[] = { "const", "int", "char", "void", "main", "if", "else", "switch", "case", "default", "while",
							"for", "scanf", "printf", "return" };

vector<Token> tokenList;
int tokenptr;

Lexer::Lexer() {
	this->tokenbuf.clear();
	this->linenumber = 1;
	this->number = 0;
	this->symbol = -1;
}

void Lexer::clearToken() {
	tokenbuf.clear();
}

void Lexer::retract() {
	infile.seekg(-1, ios::cur);
}

bool Lexer::isDigit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

bool Lexer::isLetter(char c) {
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
		return true;
	return false;
}

int Lexer::reserver() {
	int i;
	string lowerToken;
	lowerToken.resize(tokenbuf.length());
	transform(tokenbuf.begin(), tokenbuf.end(), lowerToken.begin(), ::tolower);
	for (i = 0; i < 15; i++) {
		if (reserved_words[i].compare(lowerToken) == 0) {
			return i;
		}
	}
	return -1;
}

int Lexer::transNum() {
	int i, value = 0;
	for (i = 0; tokenbuf[i] != '\0'; i++) {
		value = value * 10 + tokenbuf[i] - '0';
	}
	return value;
}

void Lexer::dealLetter() {
	int resultvalue;
	while (isLetter(ch) || isDigit(ch)) {
		tokenbuf.push_back(ch);
		ch = infile.get();
	}
	retract();
	resultvalue = reserver();
	if (resultvalue == -1) symbol = IDENFR;
	else symbol = resultvalue + 4;
}

void Lexer::dealDigit() {
	while (isDigit(ch)) {
		tokenbuf.push_back(ch);
		ch = infile.get();
	}
	retract();
	number = transNum();
	symbol = INTCON;
}

void Lexer::dealCharConst() {
	if (ch == '\'') {
		ch = infile.get();
		if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || isLetter(ch) || isDigit(ch)) {
			tokenbuf.push_back(ch);
			symbol = CHARCON;
		}
		else {
			symbol = ERROR;
		}
		ch = infile.get();
		if (ch != '\'') {
			symbol = ERROR;
		}
	}
}

void Lexer::dealStrConst() {
	if (ch == '\"') {
		ch = infile.get();
		while (ch == 32 || ch == 33 || (ch >= 35 && ch <= 126)) {
			tokenbuf.push_back(ch);
			ch = infile.get();
		}
		if (ch != '\"') {
			symbol = ERROR;
			return;
		}
		symbol = STRCON;
	}
}

void Lexer::getsym() {
	clearToken();
	ch = infile.get();
	if (ch == EOF) {
		symbol = EOFSYM;
		return;
	}
	while (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
		if (ch == '\n' || ch == '\r') {
			linenumber++;
		}
		ch = infile.get();
	}
	if (isLetter(ch)) {
		dealLetter();
	}
	else if (isDigit(ch)) {
		dealDigit();
	}
	else if (ch == '\'') {
		dealCharConst();
	}
	else if (ch == '\"') {
		dealStrConst();
	}
	else if (ch == '<') {
		tokenbuf.push_back(ch);
		ch = infile.get();
		if (ch == '=') {
			tokenbuf.push_back(ch);
			symbol = LEQ;
		}
		else {
			retract();
			symbol = LSS;
		}
	}
	else if (ch == '>') {
		tokenbuf.push_back(ch);
		ch = infile.get();
		if (ch == '=') {
			tokenbuf.push_back(ch);
			symbol = GEQ;
		}
		else {
			retract();
			symbol = GRE;
		}
	}
	else if (ch == '=') {
		tokenbuf.push_back(ch);
		ch = infile.get();
		if (ch == '=') {
			tokenbuf.push_back(ch);
			symbol = EQL;
		}
		else {
			retract();
			symbol = ASSIGN;
		}
	}
	else if (ch == '!') {
		tokenbuf.push_back(ch);
		ch = infile.get();
		if (ch == '=') {
			tokenbuf.push_back(ch);
			symbol = NEQ;
		}
		else {
			retract();
			symbol = ERROR;
		}
	}
	else {
		tokenbuf.push_back(ch);
		switch (ch)
		{
		case '+':
			symbol = PLUS;
			break;
		case '-':
			symbol = MINU;
			break;
		case '*':
			symbol = MULT;
			break;
		case '/':
			symbol = DIV;
			break;
		case ':':
			symbol = COLON;
			break;
		case ';':
			symbol = SEMICN;
			break;
		case ',':
			symbol = COMMA;
			break;
		case '(':
			symbol = LPARENT;
			break;
		case ')':
			symbol = RPARENT;
			break;
		case '[':
			symbol = LBRACK;
			break;
		case ']':
			symbol = RBRACK;
			break;
		case '{':
			symbol = LBRACE;
			break;
		case '}':
			symbol = RBRACE;
			break;
		default:
			clearToken();
			symbol = ERROR;
			break;
		}
	}
}

void Lexer::lexicalAnalyse() {
	while (true)
	{
		getsym();
		if (symbol == EOFSYM) {
			Token token(symbol, tokenbuf, linenumber);
			tokenList.push_back(token);
			break;
		}
		else if (symbol == ERROR) {
			/// TODO: 错误处理：未识别的符号
			cout << "Lexer: 未识别的符号" << endl;
		}
		else {
			Token token(symbol, tokenbuf, linenumber);
			// token.printToken();
			tokenList.push_back(token);
		}
	}
}