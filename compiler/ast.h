#pragma once
#include "Lexer.h"

#define TOKEN_BEGIN (tokenptr = -1)
#define TOKEN_GET (tokenList[++tokenptr])
#define TOKEN_RETRACT (tokenptr--)
#define TOKEN_PEEK(off) (tokenList[tokenptr + off])

using namespace std;

class AST_node {
public:
	AST_node();
	AST_node(string name, bool isSyntax);
	void addToken(Token* token);
	void addNode(AST_node node);
	void print();
	void setNum(int num);
	int getNum();
private:
	string name;
	Token* token = NULL;
	vector<AST_node> nodelist;
	bool isSyntax = false;
	int number;
};