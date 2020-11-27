#include <algorithm>
#include "Parser.h"
#include "midCode.h"
#include "function.h"

using namespace std;

extern vector<midCode> midCodeTable;

SymbolTableManager symbolTableManager;
ErrorHandler errorHandler;
Token* curToken;
vector<string> stringList;

int globalAddr = 0;
int localAddr = 0;
bool isMain = false;
bool isGlobal = false;

void Parser::initial() {
	TOKEN_BEGIN;
	curToken = &TOKEN_GET;
	has_return = false;
	stringList = vector<string>();
}

AST_node Parser::getRoot() {
	return root;
}

bool Parser::checkToken(int type, string name, AST_node* nodeptr) {
	if (!curToken->isType(type)) {
		return false;
	}
	AST_node subnode(name, false);	
	subnode.addToken(curToken);
	nodeptr->addNode(subnode);
	curToken = &TOKEN_GET;
	return true;
}

/*
<程序>    ::= ［<常量说明>］［<变量说明>］{<有返回值函数定义>|<无返回值函数定义>}<主函数>
<程序>    ::= ［<常量说明>］［<变量说明>］{ <有返回值函数定义> | <无返回值函数定义> } <主函数>

FIRST CONSTTK INTTK CHARTK INTTK CHARTK常量说明 VOIDTK VOIDTK
*/
void Parser::procedure() {
	AST_node node("<程序>", true);
	isGlobal = true;
	if (curToken->isType(CONSTTK)) {
		constDeclare();
		node.addNode(subNode);
	}
	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		varDeclare();
		node.addNode(subNode);
	}
	isGlobal = false;
	while (true)
	{
		if (TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK)) {
			funcWithReturn();
			node.addNode(subNode);
		}
		else if (TOKEN_PEEK(0).isType(VOIDTK) && TOKEN_PEEK(1).isType(IDENFR)) {
			funcNoReturn();
			node.addNode(subNode);
		}
		else
		{
			break;
		}
	}
	mainFunc();
	node.addNode(subNode);
	subNode = node;
	root = node;
}

/*
<字符串>   ::=  "｛十进制编码为32,33,35-126的ASCII字符｝"
<字符串>   ::=  [STRCON]

FIRST  STRCON
*/
bool Parser::String(string* value) {
	if (!curToken->isType(STRCON)) {
		cout << "语法分析<字符串>: 不是字符串" << endl;
		return false;
	}
	*value = curToken->getTokenStr();
	AST_node node("<字符串>", true);
	node.addToken(curToken);
	curToken = &TOKEN_GET;
	subNode = node;
	return true;
}

/*
<无符号整数>  :: = <数字>｛<数字>｝
<无符号整数>  :: = [INTCON]
FIRST INTCON
*/
bool Parser::unsignedInt() {
	if (!curToken->isType(INTCON)) {
		cout << "语法分析<无符号整数>: 不是无符号整数" << endl;
		return false;
	}
	AST_node node("<无符号整数>", true);
	node.addToken(curToken);
	node.setNum(curToken->getValue());
	curToken = &TOKEN_GET;
	subNode = node;
	return true;
}

/*
<整数>	::= ［＋｜－］<无符号整数>
<整数>	::= ［[PLUS]|[MINE]］<无符号整数>
FIRST PLUS MINU INTCON
*/
void Parser::integer() {
	AST_node node("<整数>", true);
	int negative = false;
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		if (curToken->isType(MINU)) {
			negative = true;
		}
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	unsignedInt();
	node.addNode(subNode);
	int num = negative ? -subNode.getNum() : subNode.getNum();
	node.setNum(num);
	subNode = node;
}

/*
<常量定义>  ::=	 int<标识符>＝<整数>{,<标识符>＝<整数>} | char<标识符>＝<字符>{,<标识符>＝<字符>}
<常量定义>  ::=  [INTTK]<标识符>[ASSIGN]<整数> {[COMMA]<标识符>[ASSIGN]<整数>} 
				|[CHARTK]<标识符>[ASSIGN]<字符> {[COMMA]<标识符>[ASSIGN]<字符>}
FIRST CONSTTK
*/
void Parser::constDef() {
	AST_node node("<常量定义>", true);
	STE ste;
	ste.identType = IdentType::CONST;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
	}
	int const_type = curToken->getType();
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	ste.name = curToken->getTokenStr();
	transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);

	checkToken(IDENFR, "<标识符>", &node);

	checkToken(ASSIGN, "<ASSIGN>", &node);

	if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
		integer();
		ste.value = subNode.getNum();
		node.addNode(subNode);
	}
	else if (const_type == CHARTK && curToken->isType(CHARCON)) {
		ste.value = curToken->getValue();
		ADD_TOKENNODE("<字符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		cout << "语法分析<常量定义>: 常量定义类型不一致" << endl;
	}
	// 填表
	ste.isGlobal = isGlobal;
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	while (curToken->isType(COMMA)) {
		ADD_TOKENNODE("<COMMA>", curToken, node);
		curToken = &TOKEN_GET;

		ste.name = curToken->getTokenStr();
		transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);
		checkToken(IDENFR, "<标识符>", &node);

		checkToken(ASSIGN, "<赋值符号>", &node);

		if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
			integer();
			ste.value = subNode.getNum();
			node.addNode(subNode);
		}
		else if (const_type == CHARTK && curToken->isType(CHARCON)) {
			ste.value = curToken->getValue();
			ADD_TOKENNODE("<字符>", curToken, node);
			curToken = &TOKEN_GET;
		}
		else {
			cout << "语法分析<常量定义>: 常量定义类型不一致" << endl;
		}
		// 填表
		ste.isGlobal = isGlobal;
		if (!symbolTableManager.insert(ste)) {
			// 错误处理：定义重名
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}
	}
	subNode = node;
}

/*
<常量说明>	::=  const<常量定义>;{ const<常量定义>;}
<常量说明>	::=  [CONSTTK] <常量定义> [SEMICN ] {[CONSTTK]<常量定义>[SEMICN]}
*/
void Parser::constDeclare() {
	AST_node node("<常量说明>", true);

	checkToken(CONSTTK, "<CONST>", &node);

	constDef();
	node.addNode(subNode);

	if (curToken->isType(SEMICN))
		checkToken(SEMICN, "<SEMICN>", &node);
	else
		errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

	while (curToken->isType(CONSTTK)) {
		ADD_TOKENNODE("<CONST>", curToken, node);
		curToken = &TOKEN_GET;

		constDef();
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
}

/*
<声明头部>   ::=  int<标识符> |char<标识符>
<声明头部>   ::=  [INTTK]<标识符> | [CHARTK]<标识符>
FIRST INTTK CHARTK
*/
void Parser::declareHead(STE* ste) {
	AST_node node("<声明头部>", true);

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<声明头部>: 缺少类型标识符" << endl;
	}
	ste->valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (curToken->isType(IDENFR)) {
		funcNames.insert(make_pair(curToken->getTokenStr(), true));
		ste->name = curToken->getTokenStr();
		transform(ste->name.begin(), ste->name.end(), ste->name.begin(), ::tolower);
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		cout << "语法分析<声明头部>: 缺少标识符" << endl;
	}

	subNode = node;
}

/*
<常量>   ::=  <整数>|<字符>
<常量>   ::=  <整数>|<字符>
FIRST PLUS MINU INTCON CHARCON
*/
void Parser::constant(int* value) {
	AST_node node("<常量>", true);
	if (curToken->isType(CHARCON)) {
		*value = curToken->getValue();
		ADD_TOKENNODE("<字符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		integer();
		*value = subNode.getNum();
		node.addNode(subNode);
	}
	subNode = node;
}

/*
<变量说明>  ::= <变量定义>;{<变量定义>;}
<变量说明>  ::= <变量定义>[SEMICN] {<变量定义>[SEMICN]}
FIRST INTTK CHARTK
*/
void Parser::varDeclare() {
	AST_node node("<变量说明>", true);

	varDef();
	node.addNode(subNode);

	if (curToken->isType(SEMICN))
		checkToken(SEMICN, "<SEMICN>", &node);
	else
		errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

	while ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		varDef();
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
}

/*
<变量定义>	::= <变量定义无初始化>|<变量定义及初始化>
<变量定义>	::= <变量定义无初始化> | <变量定义及初始化>
FIRST INTTK CHARTK
*/
void Parser::varDef() {
	AST_node node("<变量定义>", true);
	bool withInitial = false;
	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK)) 
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		int i = 2;
		while (!TOKEN_PEEK(i).isType(SEMICN)) {
			if (TOKEN_PEEK(i).isType(ASSIGN)) {
				withInitial = true;
				break;
			}
			i++;
		}
		if (withInitial) {
			varDefWithInitial();
			node.addNode(subNode);
		}
		else {
			varDefNoInitial();
			node.addNode(subNode);
		}
	}
	else {
		cout << "语法分析<变量定义>: 变量定义错误" << endl;
	}
	subNode = node;
}

/*
<变量定义无初始化>  ::= <类型标识符>(<标识符>|<标识符>'['<无符号整数>']'|<标识符>'['<无符号整数>']''['<无符号整数>']')
						{,(<标识符>|<标识符>'['<无符号整数>']'|<标识符>'['<无符号整数>']''['<无符号整数>']' )}
<变量定义无初始化>  ::= <类型标识符><标识符>
						( <空> |(LBRACK]<无符号整数>[RBRACK](< 空> | [LBRACK]<无符号整数>[RBRACK])))
						{[COMMA]<标识符>(<空> |([LBRACK]<无符号整数>([RBRACK] |[RBRACK][LBRACK]<无符号整数>[RBRACK])))}
*/
void Parser::varDefNoInitial() {
	AST_node node("<变量定义无初始化>", true);
	STE ste;
	ste.identType = IdentType::VAR;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<变量定义无初始化>: 缺少类型标识符" << endl;
	}
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	ste.name = curToken->getTokenStr();
	transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);
	checkToken(IDENFR, "<标识符>", &node);
	ste.length = 1;

	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		ste.identType = IdentType::ARRAY;
		ste.arrayInfo.array_dim = 1;

		unsignedInt();
		ste.arrayInfo.array_col = subNode.getNum();
		ste.length = ste.arrayInfo.array_col;
		node.addNode(subNode);

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.arrayInfo.array_dim = 2;

			unsignedInt();
			ste.arrayInfo.array_row = ste.arrayInfo.array_col;
			ste.arrayInfo.array_col = subNode.getNum();
			ste.length = ste.arrayInfo.array_row * ste.arrayInfo.array_col;
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
		}
	}

	if (isGlobal) {
		ste.addr = globalAddr;
		globalAddr = globalAddr + ste.length;
	}
	else {
		ste.addr = localAddr;
		localAddr = localAddr + ste.length;
	}
	//填表
	ste.isGlobal = isGlobal;
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	while (curToken->isType(COMMA)) {
		checkToken(COMMA, "<COMMA>", &node);
		ste.identType = IdentType::VAR;

		ste.name = curToken->getTokenStr();
		transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);
		ste.length = 1;
		checkToken(IDENFR, "<标识符>", &node);

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.identType = IdentType::ARRAY;
			ste.arrayInfo.array_dim = 1;

			unsignedInt();
			ste.arrayInfo.array_col = subNode.getNum();
			ste.length = ste.arrayInfo.array_col;
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);
				ste.arrayInfo.array_dim = 2;

				unsignedInt();
				ste.arrayInfo.array_row = ste.arrayInfo.array_col;
				ste.arrayInfo.array_col = subNode.getNum();
				ste.length = ste.arrayInfo.array_row * ste.arrayInfo.array_col;
				node.addNode(subNode);

				if (curToken->isType(RBRACK))
					checkToken(RBRACK, "<RBRACK>", &node);
				else
					errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			}
		}
		if (isGlobal) {
			ste.addr = globalAddr;
			globalAddr = globalAddr + ste.length;
		}
		else {
			ste.addr = localAddr;
			localAddr = localAddr + ste.length;
		}
		// 填表
		ste.isGlobal = isGlobal;
		if (!symbolTableManager.insert(ste)) {
			// 错误处理：定义重名
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}
	}

	subNode = node;
}

/*
<变量定义及初始化>  ::= <类型标识符><标识符>=<常量>
						|<类型标识符><标识符>'['<无符号整数>']'='{'<常量>{,<常量>}'}'
						|<类型标识符><标识符>'['<无符号整数>']''['<无符号整数>']'='{''{'<常量>{,<常量>}'}'{, '{'<常量>{,<常量>}'}'}'}'
<变量定义及初始化>  ::=  <类型标识符><标识符>([ASSIGN]<常量> 
						|[LBRACK]<无符号整数> [RBRACK]([ASSIGN][LBRACE]<常量> {[COMMA]<常量>}[RBRACE]
						|[LBRACK]<无符号整数>[RBRACK][ASSIGN][LBRACE][LBRACE]
						<常量> {[COMMA]<常量>}[RBRACE]{[COMMA] [LBRACE]<常量>{[COMMA]<常量>}[RBRACE]}[RBRACE]))
*/
void Parser::varDefWithInitial() {
	int col_count = 0, row_count = 0;
	bool num_unmatch = false;
	bool type_unmatch = false;
	int value;
	AST_node node("<变量定义及初始化>", true);
	STE ste;
	ste.identType = IdentType::VAR;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<变量定义及初始化>: 缺少类型标识符" << endl;
	}
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (curToken->isType(IDENFR)) {
		ste.name = curToken->getTokenStr();
		transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);
		checkToken(IDENFR, "<标识符>", &node);
	}
	ste.length = 1;

	if (curToken->isType(ASSIGN)) {
		checkToken(ASSIGN, "ASSIGN", &node);

		if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
			|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
			type_unmatch = true;
		constant(&value);
		midCodeTable.push_back(midCode(MOVE, ste.name, int2string(value)));
		node.addNode(subNode);
	}
	else if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		ste.identType = IdentType::ARRAY;
		ste.arrayInfo.array_dim = 1;


		unsignedInt();
		ste.arrayInfo.array_col = subNode.getNum();
		ste.length = ste.arrayInfo.array_col;
		node.addNode(subNode);
		

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(ASSIGN)) {
			checkToken(ASSIGN, "ASSIGN", &node);

			if (!checkToken(LBRACE, "<LBRACE>", &node)) {
				cout << "语法分析<变量定义及初始化>: 缺少左大括号" << endl;
			}
			if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
				|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
				type_unmatch = true;
			constant(&value);
			node.addNode(subNode);
			col_count++;
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				constant(&value);
				node.addNode(subNode);
				col_count++;
			}
			checkToken(RBRACE, "<RBRACE>", &node);
			if (col_count != ste.arrayInfo.array_col) {
				num_unmatch = true;
			}
		}
		else if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.arrayInfo.array_dim = 2;

			unsignedInt();
			ste.arrayInfo.array_row = ste.arrayInfo.array_col;
			ste.arrayInfo.array_col = subNode.getNum();
			ste.length = ste.arrayInfo.array_row * ste.arrayInfo.array_col;
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			
			checkToken(ASSIGN, "<ASSING>", &node);
			checkToken(LBRACE, "<LBRACE>", &node);
			checkToken(LBRACE, "<LBRACE>", &node);

			if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
				|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
				type_unmatch = true;
			constant(&value);
			node.addNode(subNode);
			col_count++;

			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				constant(&value);
				node.addNode(subNode);
				col_count++;
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				cout << "语法分析<变量定义及初始化>: 缺少左大括号" << endl;
			}
			if (col_count != ste.arrayInfo.array_col) {
				num_unmatch = true;
			}
			row_count++;
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				checkToken(LBRACE, "<LBRACE>", &node);
				col_count = 0;

				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				constant(&value);
				node.addNode(subNode);
				col_count++;

				while (curToken->isType(COMMA)) {
					checkToken(COMMA, "<COMMA>", &node);
					if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
						|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
						type_unmatch = true;
					constant(&value);
					node.addNode(subNode);
					col_count++;
				}
				checkToken(RBRACE, "<RBRACE>", &node);
				if (col_count != ste.arrayInfo.array_col) {
					num_unmatch = true;
				}
				row_count++;
			}
			checkToken(RBRACE, "<RBRACE>", &node);
			if (row_count != ste.arrayInfo.array_row) {
				num_unmatch = true;
			}
		}
	}
	else {
	cout << "语法分析<变量定义及初始化>: 变量定义初始化错误" << endl;
	}
	if (num_unmatch)
		errorHandler.printError(ARRAY_INIT_NUM_UNMATCH, TOKEN_PEEK(-1).getLinenum());
	if (type_unmatch)
		errorHandler.printError(CONST_TYPE_UNMATCH, TOKEN_PEEK(-1).getLinenum());
	//填表
	if (isGlobal) {
		ste.addr = globalAddr;
		globalAddr = globalAddr + ste.length;
	}
	else {
		ste.addr = localAddr;
		localAddr = localAddr + ste.length;
	}
	ste.isGlobal = isGlobal;
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	subNode = node;
}

/*
<有返回值函数定义>  ::=  <声明头部>'('<参数表>')' '{'<复合语句>'}'
<有返回值函数定义>  ::=  <声明头部>[LPARENT]<参数表>[RPARENT] [LBRACE]<复合语句>[RBRACE]
FIRST INTTK CHARTK
*/
void Parser::funcWithReturn() {
	AST_node node("<有返回值函数定义>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;
	has_return = false;
	localAddr = 0;

	declareHead(&ste);
	node.addNode(subNode);

	checkToken(LPARENT, "<LPARENT>", &node);

	paramList(&ste);
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	//填表
	ste.isGlobal = true;
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	checkToken(LBRACE, "<LBRACE>", &node);
	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	comStatement();
	node.addNode(subNode);

	if (!has_return)
		errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());

	checkToken(RBRACE, "<RBRACE>", &node);

	//填写函数长度
	symbolTableManager.find(ste.name);
	symbolTableManager.curSTE->length = localAddr;
	localAddr = 0;

	subNode = node;
}

/*
<无返回值函数定义>  ::= void<标识符>'('<参数表>')''{'<复合语句>'}'
<无返回值函数定义>  ::= [VOIDTK]<标识符>[LPARENT]<参数表>[RPARENT][LBRACE]<复合语句>[RBRACE]
FIRST VOIDTK
*/

void Parser::funcNoReturn() {
	AST_node node("<无返回值函数定义>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;

	checkToken(VOIDTK, "<VOIDTK>", &node);
	ste.valueType = ValueType::VOID;

	if (curToken->isType(IDENFR)) {
		ste.name = curToken->getTokenStr();
		transform(ste.name.begin(), ste.name.end(), ste.name.begin(), ::tolower);
		funcNames.insert(make_pair(curToken->getTokenStr(), false));
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		cout << "语法分析<<无返回值函数定义>: 缺少标识符" << endl;
	}

	checkToken(LPARENT, "<LPARENT>", &node);

	paramList(&ste);
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	checkToken(LBRACE, "<LBRACE>", &node);


	//填表
	ste.isGlobal = true;
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	comStatement();
	node.addNode(subNode);

	checkToken(RBRACE, "<RBRACE>", &node);

	// 符号表退出子层
	symbolTableManager.goOut();

	//填写函数长度
	symbolTableManager.find(ste.name);
	symbolTableManager.curSTE->length = localAddr;
	localAddr = 0;

	subNode = node;
}

/*
<复合语句>	::=  ［<常量说明>］［<变量说明>］<语句列>
<复合语句>	::=  ［<常量说明>］［<变量说明>］<语句列>
FIRST CONSTTK INTTK CHARTK WHILETK FORTK IFTK IDENFR IDENFR IDENFR SCANFTK PRINTFTK SWITCHTK SEMICN RETURNTK LBRACE <空>
*/
void Parser::comStatement() {
	AST_node node("<复合语句>", true);
	if (curToken->isType(CONSTTK)) {
		constDeclare();
		node.addNode(subNode);
	}

	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		varDeclare();
		node.addNode(subNode);
	}

	statementList();
	node.addNode(subNode);

	subNode = node;
}

/*
<参数表>	::=  <类型标识符><标识符>{,<类型标识符><标识符>}| <空>
<参数表>	::= <类型标识符><标识符>{[COMMA]<类型标识符><标识符>} |<空>
FIRST INTTK CHARTK EMPTY
*/
void Parser::paramList(STE* ste) {
	AST_node node("<参数表>", true);
	vector<Argument> args;
	STE param_ste;
	Argument argu;
	param_ste.identType = IdentType::PARAM;

	if (curToken->isType(INTTK) || curToken->isType(CHARTK)) {
		argu.type = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
		param_ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
		ADD_TOKENNODE("<类型标识符>", curToken, node);
		curToken = &TOKEN_GET;

		argu.name = curToken->getTokenStr();
		param_ste.name = curToken->getTokenStr();
		transform(param_ste.name.begin(), param_ste.name.end(), param_ste.name.begin(), ::tolower);
		checkToken(IDENFR, "<标识符>", &node);

		// 填表
		param_ste.length = 1;
		param_ste.addr = localAddr;
		localAddr++;
		args.emplace_back(argu);
		param_ste.isGlobal = false;
		if(!symbolTableManager.insert(param_ste, ste))
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());

		while (curToken->isType(COMMA)) {
			checkToken(COMMA, "<COMMA>", &node);

			if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
				cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
			}
			argu.type = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
			param_ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
			ADD_TOKENNODE("<类型标识符>", curToken, node);
			curToken = &TOKEN_GET;

			argu.name = curToken->getTokenStr();
			param_ste.name = curToken->getTokenStr();
			transform(param_ste.name.begin(), param_ste.name.end(), param_ste.name.begin(), ::tolower);
			checkToken(IDENFR, "<标识符>", &node);

			// 填表
			param_ste.isGlobal = false;
			param_ste.length = 1;
			param_ste.addr = localAddr;
			localAddr++;
			args.emplace_back(argu);
			if (!symbolTableManager.insert(param_ste, ste))
				errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}

	}

	//填表
	ste->args = args;

	subNode = node;
}

/*
<主函数>    ::= void main‘(’‘)’ ‘{’<复合语句>‘}’
<主函数>    ::= [VOIDTK] [MAINTK][LPARENT][RPARENT] [LBRACE]<复合语句>[RBRACE]
FIRST VOIDTK
*/
void Parser::mainFunc() {
	AST_node node("<主函数>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;
	localAddr = 0;
	isMain = true;

	checkToken(VOIDTK, "<VOIDTK>", &node);
	checkToken(MAINTK, "<MAINTK>", &node);
	checkToken(LPARENT, "<LPARENT>", &node);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	checkToken(LBRACE, "<LBRACE>", &node);

	ste.name = "main";
	ste.valueType = ValueType::VOID;
	ste.isGlobal = true;
	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}
	midCodeTable.push_back(midCode(midCodeOp::FUNC, "main", "", ""));

	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	comStatement();
	node.addNode(subNode);

	checkToken(RBRACE, "<RBRACE>", &node);

	// 符号表退出子层
	symbolTableManager.goOut();
	isMain = false;

	//填写函数长度
	symbolTableManager.find(ste.name);
	symbolTableManager.curSTE->length= localAddr;
	localAddr = 0;

	subNode = node;
}

/*
<表达式>    ::= ［＋｜－］<项>{<加法运算符><项>}
<表达式>    ::= ［[PLUS] | [MINU]］<项> {<加法运算符><项>}
FIRST PLUS MINU IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::expression(string* ansTemp) {
	AST_node node("<表达式>", true);
	bool isChar = false;
	bool hasFistOp = false;
	int op;
	string z, x, y;
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		hasFistOp = true;
		op = curToken->getType();
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	isChar = item(&x) && !hasFistOp;
	if (hasFistOp && op == MINU) {
		z = getTempVar();
		symbolTableManager.insert(STE(z, localAddr, VAR, INTEGER, 0, 1));
		localAddr++;
		midCodeTable.push_back(midCode(midCodeOp::SUB, z, int2string(0), x));
		x = z;
	}
	node.addNode(subNode);
	while (curToken->isType(PLUS) || curToken->isType(MINU)) {
		isChar = false;
		op = curToken->getType();
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		item(&y);
		node.addNode(subNode);

		z = getTempVar();
		symbolTableManager.insert(STE(z, localAddr, VAR, INTEGER, 0, 1));
		localAddr++;
		midCodeTable.push_back(midCode((op == PLUS ? ADD : SUB), z, x, y));
		x = z;
	}
	*ansTemp = x;
	subNode = node;
	return isChar;
}

/*
<项>	::= <因子>{<乘法运算符><因子>}
<项>	::= <因子> { <乘法运算符><因子> }
FIRST IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::item(string* ansTemp) {
	bool isChar = false;
	int op;
	string z, x, y;
	AST_node node("<项>", true);
	isChar = factor(&x);
	node.addNode(subNode);
	while (curToken->isType(MULT) || curToken->isType(DIV))
	{
		isChar = false;
		op = curToken->getType();
		ADD_TOKENNODE("<乘法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		factor(&y);
		node.addNode(subNode);

		z = getTempVar();
		symbolTableManager.insert(STE(z, localAddr, VAR, INTEGER, 0, 1));
		localAddr++;
		midCodeTable.push_back(midCode((op == MULT ? midCodeOp::MULTOP : midCodeOp::DIVOP), z, x, y));
		x = z;
	}
	*ansTemp = x;
	subNode = node;
	return isChar;
}

/*
<因子>    ::= <标识符>
				|<标识符>'['<表达式>']'
				|<标识符>'['<表达式>']''['<表达式>']'
				|'('<表达式>')'
				|<整数>|<字符>
				|<有返回值函数调用语句>
<因子>     ::=  <标识符> (<空> 
					| [LBRACK]<表达式>[RBRACK] 
					| [LBRACK]<表达式>[RBRACK][LBRACK]<表达式>[RBRACK]) 
				|[LPARENT]<表达式>[RPARENT] 
				|<整数> 
				|<字符> 
				|<有返回值函数调用语句>
FIRST IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::factor(string* ansTemp) {
	bool isChar = false;
	AST_node node("<因子>", true);
	if (TOKEN_PEEK(0).isType(IDENFR) && TOKEN_PEEK(1).isType(LPARENT)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		else {
			STE* ste = symbolTableManager.curSTE;
			if (ste->valueType == ValueType::CHAR) {
				isChar = true;
			}
		}
		callFuncWithReturn();
		node.addNode(subNode);
	}
	else if (curToken->isType(IDENFR)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		else {
			STE* ste = symbolTableManager.curSTE;
			if (ste->valueType == ValueType::CHAR) {
				isChar = true;
			}
			if (ste->identType == IdentType::CONST)
				*ansTemp = int2string(ste->value);
			else
				*ansTemp = ste->name;
		}
		checkToken(IDENFR, "<标识符>", &node);

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);

			string temp;
			bool ischar = expression(&temp);
			node.addNode(subNode);
			if (ischar) {
				errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
			}

			
			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);

				string temp;
				bool ischar = expression(&temp);
				node.addNode(subNode);
				if (ischar) {
					errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
				}

				if (curToken->isType(RBRACK))
					checkToken(RBRACK, "<RBRACK>", &node);
				else
					errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			}
		}
	}
	else if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);
		expression(ansTemp);
		node.addNode(subNode);

		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU)) {
		integer();
		node.addNode(subNode);
		*ansTemp = int2string(subNode.getNum());
	}
	else if (curToken->isType(CHARCON)) {
		isChar = true;
		checkToken(CHARCON, "<CHARCON>", &node);
		*ansTemp = int2string(curToken->getValue());
	}
	else
	{
		cout << "语法分析<因子>: 未识别的因子" << endl;
	}

	subNode = node;
	return isChar;
}

/*
<语句>    ::= <循环语句>｜<条件语句>| <有返回值函数调用语句>;  |<无返回值函数调用语句>;｜<赋值语句>;｜<读语句>;｜<写语句>;｜<情况语句>｜<空>;|<返回语句>; | '{'<语句列>'}'
<语句>    ::= <循环语句> |
				<条件语句> |
				<有返回值函数调用语句> [SEMICN]  |
				<无返回值函数调用语句> [SEMICN] |
				<赋值语句> [SEMICN] |
				<读语句> [SEMICN] |
				<写语句> [SEMICN] |
				<情况语句> |
				<空> [SEMICN] |
				<返回语句> [SEMICN] |
				[LBRACE] <语句列> [RBRACE]

FIRST WHILETK FORTK IFTK IDENFR IDENFR IDENFR SCANFTK PRINTFTK SWITCHTK SEMICN RETURNTK LBRACE
*/
void Parser::statement() {
	AST_node node("<语句>", true);
	if (curToken->isType(WHILETK) || curToken->isType(FORTK)) {
		loopStatement();
		node.addNode(subNode);
	}
	else if (curToken->isType(IFTK)) {
		ifStatement();
		node.addNode(subNode);
	}
	else if (curToken->isType(IDENFR)) {
		if (TOKEN_PEEK(1).isType(LPARENT) && funcNames[curToken->getTokenStr()]) {
			callFuncWithReturn();
			node.addNode(subNode);
		}
		else if(TOKEN_PEEK(1).isType(LPARENT) && !funcNames[curToken->getTokenStr()]){
			callFuncNoReturn();
			node.addNode(subNode);
		}
		else
		{
			assignStatement();
			node.addNode(subNode);
		}

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(SCANFTK)) {
		scanfStatement();
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(PRINTFTK)) {
		printfStatement();
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(SWITCHTK)) {
		switchStatement();
		node.addNode(subNode);
	}
	else if (curToken->isType(RETURNTK)) {
		returnStatement();
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(LBRACE)) {
		checkToken(LBRACE, "<LBRACE>", &node);
		statementList();
		node.addNode(subNode);
		checkToken(RBRACE, "<RBRACE>", &node);
	}
	else {
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
}

/*
<赋值语句>   ::=  <标识符>＝<表达式>|<标识符>'['<表达式>']'=<表达式>|<标识符>'['<表达式>']''['<表达式>']' =<表达式>
<赋值语句>  ::=  <标识符>[ASSIGN]<表达式>|
				  <标识符>[LBRACK]<表达式>[RBRACK][ASSIGN]<表达式>|
				  <标识符>[LBRACK]<表达式>[RBRACK][LBRACK]<表达式>[RBRACK] [ASSIGN]<表达式>
FIRST IDENFR
*/
void Parser::assignStatement() {
	AST_node node("<赋值语句>", true);
	string z, x;
	string temp;

	if (curToken->isType(IDENFR)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		STE* ste = symbolTableManager.curSTE;
		if (ste->identType == IdentType::CONST) {
			errorHandler.printError(ALTER_CONST_VALUE, curToken->getLinenum());
		}
		z = ste->name;
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		cout << "语法分析<赋值语句>: 标识符错误" << endl;
	}

	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);

		bool ischar = expression(&temp);
		node.addNode(subNode);
		if (ischar) {
			errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
		}

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);

			bool ischar = expression(&temp);
			node.addNode(subNode);
			if (ischar) {
				errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
			}

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
		}
	}
	checkToken(ASSIGN, "<ASSIGN>", &node);
	expression(&x);
	midCodeTable.push_back(midCode(midCodeOp::MOVE, z, x, ""));
	node.addNode(subNode);
	subNode = node;
}

/*
<条件语句>  ::= if '('<条件>')'<语句>［else<语句>］
<条件语句>  ::= [IFTK] [LPARENT]<条件>[RPARENT]<语句>［[ELSETK]<语句>］
FIRST IFTK
*/
void Parser::ifStatement() {
	AST_node node("<条件语句>", true);

	checkToken(IFTK, "<IFTK>", &node);
	checkToken(LPARENT, "<LPARENT>", &node);

	condition();
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	statement();
	node.addNode(subNode);
	if (curToken->isType(ELSETK)) {
		checkToken(ELSETK, "<ELSETK>", &node);
		statement();
		node.addNode(subNode);
	}
	subNode = node;
}

/*
<条件>    ::=  <表达式><关系运算符><表达式>
<条件>    ::=  <表达式><关系运算符><表达式>
FIRST PLUS MINU IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
(symbol == LSS || symbol == LEQ || symbol == GRE
		|| symbol == GEQ || symbol == EQL || symbol == NEQ)
*/
void Parser::condition() {
	AST_node node("<条件>", true);
	bool isChar = false;
	bool ischar;

	string temp;
	ischar = expression(&temp);
	node.addNode(subNode);
	isChar = ischar ? true: isChar;

	if (curToken->isType(LSS) || curToken->isType(LEQ) || curToken->isType(GRE)
		|| curToken->isType(GEQ) || curToken->isType(EQL) || curToken->isType(NEQ)) {
		ADD_TOKENNODE("<关系运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		cout << "语法分析<条件>: 关系运算符" << endl;
	}

	ischar = expression(&temp);
	node.addNode(subNode);
	isChar = ischar ? true : isChar;

	// 错误处理：不合法类型
	if (isChar)
		errorHandler.printError(ILLEGAL_CONDITION_TYPE, curToken->getLinenum());

	subNode = node;
}

/*
<循环语句>   ::=  while '('<条件>')'<语句>| for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
<循环语句>   ::=  [WHILETK] [LPARENT]<条件>[RPARENT]<语句>|
				 [FORTK][LPARENT]<标识符>[ASSIGN]<表达式>[SEMICN]<条件>[SEMICN]<标识符> [ASSIGN]<标识符>([PLUS]|[MINU])<步长>[RPARENT]<语句>

FIRST WHILETK FORTK
*/
void Parser::loopStatement() {
	AST_node node("<循环语句>", true);
	if (curToken->isType(WHILETK)) {
		checkToken(WHILETK, "<WHILETK>", &node);
		checkToken(LPARENT, "<LPARENT>", &node);

		condition();
		node.addNode(subNode);

		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

		statement();
		node.addNode(subNode);
	}
	else {
		checkToken(FORTK, "<FORTK>", &node);
		checkToken(LPARENT, "<LPARENT>", &node);

		if (curToken->isType(IDENFR)) {
			if (!symbolTableManager.find(curToken->getTokenStr())) {
				errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			}
			checkToken(IDENFR, "<标识符>", &node);
		}

		checkToken(ASSIGN, "<ASSIGN>", &node);

		string temp;
		expression(&temp);
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

		condition();
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(IDENFR)) {
			if (!symbolTableManager.find(curToken->getTokenStr())) {
				errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			}
			checkToken(IDENFR, "<标识符>", &node);
		}

		checkToken(ASSIGN, "<ASSIGN>", &node);

		if (curToken->isType(IDENFR)) {
			if (!symbolTableManager.find(curToken->getTokenStr())) {
				errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			}
			checkToken(IDENFR, "<标识符>", &node);
		}

		if (curToken->isType(PLUS) || curToken->isType(MINU)) {
			ADD_TOKENNODE("<加法运算符>", curToken, node);
			curToken = &TOKEN_GET;
		}

		step();
		node.addNode(subNode);

		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

		statement();
		node.addNode(subNode);
	}
	subNode = node;
}

/*
<步长>::= <无符号整数>
<步长>::= <无符号整数>
*/
void Parser::step() {
	AST_node node("<步长>", true);
	unsignedInt();
	node.addNode(subNode);
	subNode = node;
}

/*
<情况语句>  ::=  switch ‘(’<表达式>‘)’ ‘{’<情况表><缺省>‘}’
<情况语句>  ::=  [SWITCHTK] [LPARENT]<表达式>[RPARENT] [LBRACE]<情况表><缺省>[RBRACE]

FIRST SWITCHTK
*/
void Parser::switchStatement() {
	AST_node node("<情况语句>", true);
	checkToken(SWITCHTK, "<SWITCHTK>", &node);
	checkToken(LPARENT, "<LPARENT>", &node);

	string temp;
	bool ischar = expression(&temp);
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	checkToken(LBRACE, "<LBRACE>", &node);

	caseList(ischar);
	node.addNode(subNode);

	if (curToken->isType(DEFAULTTK)) {
		defaultStatement();
		node.addNode(subNode);
	}
	else
		errorHandler.printError(MISSING_DEFAULT, curToken->getLinenum());

	checkToken(RBRACE, "<RBRACE>", &node);

	subNode = node;
}

/*
<情况表>   ::=  <情况子语句>{<情况子语句>}
<情况表>   ::=  <情况子语句>{<情况子语句>}

FIRST CASETK
*/
void Parser::caseList(bool ischar) {
	AST_node node("<情况表>", true);
	caseStatement(ischar);
	node.addNode(subNode);
	while (curToken->isType(CASETK)) {
		caseStatement(ischar);
		node.addNode(subNode);
	}
	subNode = node;
}

/*
<情况子语句>  ::=  case<常量>：<语句>
<情况子语句>  ::=  [CASETK]<常量>[COLON]<语句>

FIRST CASETK
*/
void Parser::caseStatement(bool ischar) {
	int value;
	AST_node node("<情况子语句>", true);
	checkToken(CASETK, "<CASETK>", &node);

	if ((ischar && !curToken->isType(CHARCON)) || (!ischar && curToken->isType(CHARCON)))
		errorHandler.printError(CONST_TYPE_UNMATCH, curToken->getLinenum());

	constant(&value);
	node.addNode(subNode);

	checkToken(COLON, "<COLON>", &node);

	statement();
	node.addNode(subNode);

	subNode = node;
}

/*
<缺省>   ::=  default :<语句>
<缺省>   ::=  [DEFAULTTK] [COLON] <语句>

FIRST DEFAULTTK
*/
void Parser::defaultStatement() {
	AST_node node("<缺省>", true);
	checkToken(DEFAULTTK, "<DEFAULTTK>", &node);
	checkToken(COLON, "<COLON>", &node);
	statement();
	node.addNode(subNode);
	subNode = node;
}

/*
<有返回值函数调用语句> ::= <标识符>'('<值参数表>')'
<有返回值函数调用语句> ::=<标识符>[LPARENT]<值参数表>[RPARENT]

FIRST IDENFR
*/
void Parser::callFuncWithReturn() {
	string funcName;
	AST_node node("<有返回值函数调用语句>", true);
	if (curToken->isType(IDENFR)) {
		funcName = curToken->getTokenStr();
		if (!symbolTableManager.find(funcName)) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			while (!curToken->isType(SEMICN)) {
				curToken = &TOKEN_GET;
			}
			goto END;
		}
		checkToken(IDENFR, "<标识符>", &node);
	}

	checkToken(LPARENT, "<LPARENT>", &node);

	valueParamList(funcName);
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	subNode = node;
	END:
	return;
}

/*
<无返回值函数调用语句> ::= <标识符>'('<值参数表>')'
<无返回值函数调用语句> ::=<标识符>[LPARENT]<值参数表>[RPARENT]

FIRST IDENFR
*/
void Parser::callFuncNoReturn() {
	string funcName;
	AST_node node("<无返回值函数调用语句>", true);
	if (curToken->isType(IDENFR)) {
		funcName = curToken->getTokenStr();
		if (!symbolTableManager.find(funcName)) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			while (!curToken->isType(SEMICN)) {
				curToken = &TOKEN_GET;
			}
			goto END;
		}
		checkToken(IDENFR, "<标识符>", &node);
	}

	checkToken(LPARENT, "<LPARENT>", &node);

	valueParamList(funcName);
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	subNode = node;
	END:
	return;
}

/*
<值参数表>   ::= <表达式>{,<表达式>}｜<空>
<值参数表>   ::= <表达式> {[COMMA]<表达式>}|<空>

FIRST IDENFR LPARENT PLUS MINU INTCON CHARCON
*/
void Parser::valueParamList(string funcName) {
	AST_node node("<值参数表>", true);
	string temp;

	STE* ste = symbolTableManager.curSTE;
	vector<Argument> args = ste->args;
	int argsNum = args.size();
	int count = 0;
	bool typeUnmach = false;
	vector<Argument>::iterator iter = args.begin();

	if (curToken->isType(IDENFR) || curToken->isType(LPARENT) || curToken->isType(PLUS)
		|| curToken->isType(MINU) || curToken->isType(INTCON) || curToken->isType(CHARCON)) {


		bool ischar = expression(&temp);
		node.addNode(subNode);
		if (iter != args.end()) {
			if (((*iter).type == ValueType::CHAR && !ischar)
				|| ((*iter).type == ValueType::INTEGER && ischar))
				typeUnmach = true;
			iter++;
		}
		count++;


		while (curToken->isType(COMMA))
		{
			checkToken(COMMA, "<COMMA>", &node);

			bool ischar = expression(&temp);
			node.addNode(subNode);
			if (iter != args.end()) {
				if (((*iter).type == ValueType::CHAR && !ischar)
					|| ((*iter).type == ValueType::INTEGER && ischar))
					typeUnmach = true;
				iter++;
			}
			count++;
		}
	}


	if (count != argsNum)
		errorHandler.printError(PARAMS_NUM_UNMATCH, curToken->getLinenum());
	else if (typeUnmach)
		errorHandler.printError(PARAMS_TYPE_UNMATCH, curToken->getLinenum());

	subNode = node;
}

/*
<语句列>   ::= ｛<语句>｝
<语句列>   ::= ｛<语句>｝

FIRST WHILETK FORTK IFTK IDENFR IDENFR IDENFR SCANFTK PRINTFTK SWITCHTK SEMICN RETURNTK LBRACE <空>
*/
void Parser::statementList() {
	AST_node node("<语句列>", true);
	while (!curToken->isType(RBRACE)) {
		statement();
		node.addNode(subNode);
	}
	subNode = node;
}

/*
<读语句>    ::=  scanf '('<标识符>')'
<读语句>    ::=  [SCANFTK] [LPARENT]<标识符> [RPARENT]

FIRST SCANFTK
*/
void Parser::scanfStatement() {
	string z, x;
	AST_node node("<读语句>", true);
	checkToken(SCANFTK, "<SCANFTK>", &node);

	checkToken(LPARENT, "<LPARENT>", &node);

	if (curToken->isType(IDENFR)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		STE* ste = symbolTableManager.curSTE;
		if (ste->identType == IdentType::CONST) {
			errorHandler.printError(ALTER_CONST_VALUE, curToken->getLinenum());
		}
		z = ste->name;
		x = ste->valueType == ValueType::INTEGER ? "int" : "char";
		checkToken(IDENFR, "<标识符>", &node);
	}

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	midCodeTable.push_back(midCode(midCodeOp::SCAN, z, x, ""));
	subNode = node;
}

/*
<写语句>    ::= printf '(' <字符串>,<表达式> ')'| printf '('<字符串> ')'| printf '('<表达式>')'
<写语句>    ::= [PRINTFTK] [LPARENT] ( <字符串>[COMMA]<表达式> [RPARENT] | <字符串> [RPARENT] | <表达式>[RPARENT] )

FIRST PRINTFTK
*/
void Parser::printfStatement() {
	string z, x;
	bool ischar;
	AST_node node("<写语句>", true);
	checkToken(PRINTFTK, "<PRINTFTK>", &node);
	checkToken(LPARENT, "<LPARENT>", &node);

	if (curToken->isType(STRCON)) {
		String(&z);
		stringList.push_back(z);
		midCodeTable.push_back(midCode(midCodeOp::PRINT, z, "string", ""));
		node.addNode(subNode);
		if (curToken->isType(COMMA)) {
			checkToken(COMMA, "COMMA", &node);
			ischar = expression(&z);
			node.addNode(subNode);
			midCodeTable.push_back(midCode(midCodeOp::PRINT, z, (ischar ? "char" : "int"), ""));
		}
		midCodeTable.push_back(midCode(midCodeOp::PRINT, "\\n", "nextline", ""));
	}
	else {
		ischar = expression(&z);
		node.addNode(subNode);
		midCodeTable.push_back(midCode(midCodeOp::PRINT, z, (ischar ? "char" : "int"), ""));
		midCodeTable.push_back(midCode(midCodeOp::PRINT, "\\n", "nextline", ""));
	}
	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	subNode = node;
}

/*
<返回语句>   ::=  return['('<表达式>')']
<返回语句>   ::=  [RETURNTK] [ [LPARENT] <表达式> [RPARENT] ]

FIRST RETURNTK
*/
void Parser::returnStatement() {
	string temp;
	AST_node node("<返回语句>", true);
	checkToken(RETURNTK, "<RETURNTK>", &node);
	has_return = true;
	if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);

		if (symbolTableManager.getPosType() == ValueType::VOID)
			errorHandler.printError(NO_RETURN_FUNC_ERROR, curToken->getLinenum());

		if (curToken->isType(RPARENT) && symbolTableManager.getPosType() != ValueType::VOID) {
			errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());
		}
		else{
			bool ischar = expression(&temp);
			node.addNode(subNode);
			if ((symbolTableManager.getPosType() == ValueType::INTEGER && ischar)
				|| (symbolTableManager.getPosType() == ValueType::CHAR && !ischar)) {
				errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());
			}
		}
		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	}
	else {
		if (symbolTableManager.getPosType() != ValueType::VOID)
			errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());
		if (isMain) {
			midCodeTable.push_back(midCode(midCodeOp::EXIT, "", "", ""));
		}
	}
	subNode = node;
}