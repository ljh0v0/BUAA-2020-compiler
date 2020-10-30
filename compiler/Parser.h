#pragma once

#include "ast.h"
#include "SymbolTable.h"
#include "ErrorHandler.h"
#include <map>


#define ADD_TOKENNODE(name, tokenptr, father)	\
{									\
	AST_node subnode(name, false);	\
	subnode.addToken(tokenptr);		\
	father.addNode(subnode);		\
}

using namespace std;

extern SymbolTableManager symbolTableManager;
extern Token* curToken;

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
	bool declareHead(STE* ste);
	bool constant();
	bool varDeclare();
	bool varDef();
	bool varDefNoInitial();
	bool varDefWithInitial();
	bool funcWithReturn();
	bool funcNoReturn();
	bool comStatement();
	bool paramList(STE* ste);
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
	bool caseList(bool ischar);
	bool caseStatement(bool ischar);
	bool defaultStatement();
	bool callFuncWithReturn();
	bool callFuncNoReturn();
	bool valueParamList(string funcName);
	bool statementList();
	bool scanfStatement();
	bool printfStatement();
	bool returnStatement();
private:
	AST_node subNode;
	AST_node root;
	ArrayInfo array;
	map<string, bool> funcNames;
	bool has_return;
	bool isChar;
};

