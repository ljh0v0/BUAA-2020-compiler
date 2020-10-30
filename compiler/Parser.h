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
	void procedure();
	bool String();
	bool unsignedInt();
	void integer();
	void constDef();
	void constDeclare();
	void declareHead(STE* ste);
	void constant();
	void varDeclare();
	void varDef();
	void varDefNoInitial();
	void varDefWithInitial();
	void funcWithReturn();
	void funcNoReturn();
	void comStatement();
	void paramList(STE* ste);
	void mainFunc();
	bool expression();
	bool item();
	bool factor();
	void statement();
	void assignStatement();
	void ifStatement();
	void condition();
	void loopStatement();
	void step();
	void switchStatement();
	void caseList(bool ischar);
	void caseStatement(bool ischar);
	void defaultStatement();
	void callFuncWithReturn();
	void callFuncNoReturn();
	void valueParamList(string funcName);
	void statementList();
	void scanfStatement();
	void printfStatement();
	void returnStatement();
private:
	AST_node subNode;
	AST_node root;
	ArrayInfo array;
	map<string, bool> funcNames;
	bool has_return;
};

