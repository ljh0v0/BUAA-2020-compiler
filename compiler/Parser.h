#pragma once

#include "ast.h"
#include <map>

#define ADD_TOKENNODE(name, tokenptr, father)	\
{									\
	AST_node subnode(name, false);	\
	subnode.addToken(tokenptr);		\
	father.addNode(subnode);		\
}

struct ArrayInfo
{
	int array_row;
	int array_col;
	int array_dim;
};

class Parser
{
public:
	void initial();
	AST_node getRoot();
	bool checkToken(int type, string name, AST_node* nodeptr);
	bool procedure();
	bool String();
	bool unsignedInt();
	bool integer();
	bool constDef();
	bool constDeclare();
	bool declareHead();
	bool constant();
	bool varDeclare();
	bool varDef();
	bool varDefNoInitial();
	bool varDefWithInitial();
	bool funcWithReturn();
	bool funcNoReturn();
	bool comStatement();
	bool paramList();
	bool mainFunc();
	bool expression();
	bool item();
	bool factor();
	bool statement();
	bool assignStatement();
	bool ifStatement();
	bool condition();
	bool loopStatement();
	bool step();
	bool switchStatement();
	bool caseList();
	bool caseStatement();
	bool defaultStatement();
	bool callFuncWithReturn();
	bool callFuncNoReturn();
	bool valueParamList();
	bool statementList();
	bool scanfStatement();
	bool printfStatement();
	bool returnStatement();
private:
	Token* curToken;
	AST_node subNode;
	AST_node root;
	ArrayInfo array;
	map<string, bool> has_return;
};

