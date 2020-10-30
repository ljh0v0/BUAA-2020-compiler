#include "Parser.h"

using namespace std;

SymbolTableManager symbolTableManager;
ErrorHandler errorHandler;
Token* curToken;

void Parser::initial() {
	TOKEN_BEGIN;
	curToken = &TOKEN_GET;
	has_return = false;
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
bool Parser::procedure() {
	AST_node node("<程序>", true);
	if (curToken->isType(CONSTTK)) {
		if (!constDeclare()) {
			cout << "语法分析<程序>: 常量分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDeclare()) {
			cout << "语法分析<程序>: 变量分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	while (true)
	{
		if (TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK)) {
			if (!funcWithReturn()) {
				cout << "语法分析<程序>: 有返回值的函数错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else if (TOKEN_PEEK(0).isType(VOIDTK) && TOKEN_PEEK(1).isType(IDENFR)) {
			if (!funcNoReturn()) {
				cout << "语法分析<程序>: 无返回值的函数错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else
		{
			break;
		}
	}
	if (!mainFunc()) {
		cout << "语法分析<程序>: 主函数错误" << endl;
		return false;
	}
	node.addNode(subNode);
	subNode = node;
	root = node;
	return true;
}

/*
<字符串>   ::=  "｛十进制编码为32,33,35-126的ASCII字符｝"
<字符串>   ::=  [STRCON]

FIRST  STRCON
*/
bool Parser::String() {
	if (!curToken->isType(STRCON)) {
		cout << "语法分析<字符串>: 不是字符串" << endl;
		return false;
	}
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
bool Parser::integer() {
	AST_node node("<整数>", true);
	int negative = false;
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		if (curToken->isType(MINU)) {
			negative = true;
		}
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	if (!unsignedInt()) {
		cout << "语法分析<无符号整数>: 不是整数" << endl;
		return false;
	}
	node.addNode(subNode);
	int num = negative ? -subNode.getNum() : subNode.getNum();
	node.setNum(num);
	subNode = node;
	return true;
}

/*
<常量定义>  ::=	 int<标识符>＝<整数>{,<标识符>＝<整数>} | char<标识符>＝<字符>{,<标识符>＝<字符>}
<常量定义>  ::=  [INTTK]<标识符>[ASSIGN]<整数> {[COMMA]<标识符>[ASSIGN]<整数>} 
				|[CHARTK]<标识符>[ASSIGN]<字符> {[COMMA]<标识符>[ASSIGN]<字符>}
FIRST CONSTTK
*/
bool Parser::constDef() {
	AST_node node("<常量定义>", true);
	STE ste;
	ste.identType = IdentType::CONST;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
		return false;
	}
	int const_type = curToken->getType();
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	ste.name = curToken->getTokenStr();
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		cout << "语法分析<常量定义>: 缺少标识符" << endl;
		return false;
	}

	if (!checkToken(ASSIGN, "<赋值符号>", &node)) {
		cout << "语法分析<常量定义>: 缺少赋值符号" << endl;
		return false;
	}

	if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
		if (!integer()) {
			cout << "语法分析<常量定义>: 整数赋值错误" << endl;
			return false;
		}
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
		return false;
	}
	// 填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	while (curToken->isType(COMMA)) {
		ADD_TOKENNODE("<COMMA>", curToken, node);
		curToken = &TOKEN_GET;

		ste.name = curToken->getTokenStr();
		if (!checkToken(IDENFR, "<标识符>", &node)) {
			cout << "语法分析<常量定义>: 缺少标识符" << endl;
			return false;
		}

		if (!checkToken(ASSIGN, "<赋值符号>", &node)) {
			cout << "语法分析<常量定义>: 缺少赋值符号" << endl;
			return false;
		}

		if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
			if (!integer()) {
				cout << "语法分析<常量定义>: 整数赋值错误" << endl;
				return false;
			}
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
			return false;
		}
		// 填表
		if (!symbolTableManager.insert(ste)) {
			// 错误处理：定义重名
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}
	}
	subNode = node;
	return true;
}

/*
<常量说明>	::=  const<常量定义>;{ const<常量定义>;}
<常量说明>	::=  [CONSTTK] <常量定义> [SEMICN ] {[CONSTTK]<常量定义>[SEMICN]}
*/
bool Parser::constDeclare() {
	AST_node node("<常量说明>", true);

	if (!checkToken(CONSTTK, "<CONST>", &node)) {
		cout << "语法分析<常量说明>: 缺少const" << endl;
		return false;
	}

	if (!constDef()) {
		cout << "语法分析<常量说明>: 缺少常量定义" << endl;
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(SEMICN))
		checkToken(SEMICN, "<SEMICN>", &node);
	else
		errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

	while (curToken->isType(CONSTTK)) {
		ADD_TOKENNODE("<CONST>", curToken, node);
		curToken = &TOKEN_GET;

		if (!constDef()) {
			cout << "语法分析<常量说明>: 缺少常量定义" << endl;
			return false;
		}
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
	return true;
}

/*
<声明头部>   ::=  int<标识符> |char<标识符>
<声明头部>   ::=  [INTTK]<标识符> | [CHARTK]<标识符>
FIRST INTTK CHARTK
*/
bool Parser::declareHead(STE* ste) {
	AST_node node("<声明头部>", true);

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<声明头部>: 缺少类型标识符" << endl;
		return false;
	}
	ste->valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (curToken->isType(IDENFR)) {
		funcNames.insert(make_pair(curToken->getTokenStr(), true));
		ste->name = curToken->getTokenStr();
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		cout << "语法分析<声明头部>: 缺少标识符" << endl;
		return false;
	}

	subNode = node;
	return true;
}

/*
<常量>   ::=  <整数>|<字符>
<常量>   ::=  <整数>|<字符>
FIRST PLUS MINU INTCON CHARCON
*/
bool Parser::constant() {
	AST_node node("<常量>", true);
	if (curToken->isType(CHARCON)) {
		ADD_TOKENNODE("<字符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		if (!integer()) {
			cout << "语法分析<常量>: 未识别的常量" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	subNode = node;
	return true;
}

/*
<变量说明>  ::= <变量定义>;{<变量定义>;}
<变量说明>  ::= <变量定义>[SEMICN] {<变量定义>[SEMICN]}
FIRST INTTK CHARTK
*/
bool Parser::varDeclare() {
	AST_node node("<变量说明>", true);

	if (!varDef()) {
		cout << "语法分析<变量说明>: 变量定义错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(SEMICN))
		checkToken(SEMICN, "<SEMICN>", &node);
	else
		errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

	while ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDef()) {
			cout << "语法分析<变量说明>: 变量定义错误" << endl;
			return false;
		}
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
	return true;
}

/*
<变量定义>	::= <变量定义无初始化>|<变量定义及初始化>
<变量定义>	::= <变量定义无初始化> | <变量定义及初始化>
FIRST INTTK CHARTK
*/
bool Parser::varDef() {
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
			if (!varDefWithInitial()) {
				cout << "语法分析<变量定义>: 变量定义及初始化错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else {
			if (!varDefNoInitial()) {
				cout << "语法分析<变量定义>: 变量定义无初始化错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
	}
	else {
		cout << "语法分析<变量定义>: 变量定义错误" << endl;
		return false;
	}
	subNode = node;
	return true;
}

/*
<变量定义无初始化>  ::= <类型标识符>(<标识符>|<标识符>'['<无符号整数>']'|<标识符>'['<无符号整数>']''['<无符号整数>']')
						{,(<标识符>|<标识符>'['<无符号整数>']'|<标识符>'['<无符号整数>']''['<无符号整数>']' )}
<变量定义无初始化>  ::= <类型标识符><标识符>
						( <空> |(LBRACK]<无符号整数>[RBRACK](< 空> | [LBRACK]<无符号整数>[RBRACK])))
						{[COMMA]<标识符>(<空> |([LBRACK]<无符号整数>([RBRACK] |[RBRACK][LBRACK]<无符号整数>[RBRACK])))}
*/
bool Parser::varDefNoInitial() {
	AST_node node("<变量定义无初始化>", true);
	STE ste;
	ste.identType = IdentType::VAR;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		cout << "语法分析<变量定义无初始化>: 缺少类型标识符" << endl;
		return false;
	}
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	ste.name = curToken->getTokenStr();
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		cout << "语法分析<变量定义无初始化>: 缺少标识符" << endl;
		return false;
	}

	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		ste.identType = IdentType::ARRAY;
		ste.arrayInfo.array_dim = 1;

		if (!unsignedInt()) {
			cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
			return false;
		}
		ste.arrayInfo.array_col = subNode.getNum();
		node.addNode(subNode);

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.arrayInfo.array_dim = 2;

			if (!unsignedInt()) {
				cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
				return false;
			}
			ste.arrayInfo.array_row = ste.arrayInfo.array_col;
			ste.arrayInfo.array_col = subNode.getNum();
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
		}
	}

	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	while (curToken->isType(COMMA)) {
		checkToken(COMMA, "<COMMA>", &node);
		ste.identType = IdentType::VAR;

		ste.name = curToken->getTokenStr();
		if (!checkToken(IDENFR, "<标识符>", &node)) {
			cout << "语法分析<变量定义无初始化>: 缺少标识符" << endl;
			return false;
		}

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.identType = IdentType::ARRAY;
			ste.arrayInfo.array_dim = 1;

			if (!unsignedInt()) {
				cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
				return false;
			}
			ste.arrayInfo.array_col = subNode.getNum();
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);
				ste.arrayInfo.array_dim = 2;

				if (!unsignedInt()) {
					cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
					return false;
				}
				ste.arrayInfo.array_row = ste.arrayInfo.array_col;
				ste.arrayInfo.array_col = subNode.getNum();
				node.addNode(subNode);

				if (curToken->isType(RBRACK))
					checkToken(RBRACK, "<RBRACK>", &node);
				else
					errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			}
		}
		// 填表
		if (!symbolTableManager.insert(ste)) {
			// 错误处理：定义重名
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}
	}

	subNode = node;
	return true;
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
bool Parser::varDefWithInitial() {
	int col_count = 0, row_count = 0;
	bool num_unmatch = false;
	bool type_unmatch = false;
	AST_node node("<变量定义及初始化>", true);
	STE ste;
	ste.identType = IdentType::VAR;

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		return false;
	}
	ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	ste.name = curToken->getTokenStr();
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		return false;
	}

	if (curToken->isType(ASSIGN)) {
		checkToken(ASSIGN, "ASSIGN", &node);

		if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
			|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
			type_unmatch = true;
		if (!constant()) {
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		ste.identType = IdentType::ARRAY;
		ste.arrayInfo.array_dim = 1;


		if (!unsignedInt()) {
			return false;
		}
		ste.arrayInfo.array_col = subNode.getNum();
		node.addNode(subNode);
		

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(ASSIGN)) {
			checkToken(ASSIGN, "ASSIGN", &node);

			if (!checkToken(LBRACE, "<LBRACE>", &node)) {
				return false;
			}
			if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
				|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
				type_unmatch = true;
			if (!constant()) {
				return false;
			}
			node.addNode(subNode);
			col_count++;
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				if (!constant()) {
					return false;
				}
				node.addNode(subNode);
				col_count++;
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				return false;
			}
			if (col_count != ste.arrayInfo.array_col) {
				num_unmatch = true;
			}
		}
		else if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			ste.arrayInfo.array_dim = 2;

			if (!unsignedInt()) {
				return false;
			}
			ste.arrayInfo.array_row = ste.arrayInfo.array_col;
			ste.arrayInfo.array_col = subNode.getNum();
			node.addNode(subNode);

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			
			if (!checkToken(ASSIGN, "<ASSING>", &node) || !checkToken(LBRACE, "<LBRACE>", &node) 
				|| !checkToken(LBRACE, "<LBRACE>", &node)) {
				return false;
			}
			if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
				|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
				type_unmatch = true;
			if (!constant()) {
				return false;
			}
			node.addNode(subNode);
			col_count++;
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				if (!constant()) {
					return false;
				}
				node.addNode(subNode);
				col_count++;
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				return false;
			}
			if (col_count != ste.arrayInfo.array_col) {
				num_unmatch = true;
			}
			row_count++;
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if (!checkToken(LBRACE, "<LBRACE>", &node)) {
					return false;
				}
				col_count = 0;
				if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
					|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
					type_unmatch = true;
				if (!constant()) {
					return false;
				}
				node.addNode(subNode);
				col_count++;
				while (curToken->isType(COMMA)) {
					checkToken(COMMA, "<COMMA>", &node);
					if ((ste.valueType == ValueType::CHAR && !curToken->isType(CHARCON))
						|| (ste.valueType == ValueType::INTEGER && curToken->isType(CHARCON)))
						type_unmatch = true;
					if (!constant()) {
						return false;
					}
					node.addNode(subNode);
					col_count++;
				}
				if (!checkToken(RBRACE, "<RBRACE>", &node)) {
					return false;
				}
				if (col_count != ste.arrayInfo.array_col) {
					num_unmatch = true;
				}
				row_count++;
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				return false;
			}
			if (row_count != ste.arrayInfo.array_row) {
				num_unmatch = true;
			}
		}
	}
	else {
		return false;
	}
	if (num_unmatch)
		errorHandler.printError(ARRAY_INIT_NUM_UNMATCH, TOKEN_PEEK(-1).getLinenum());
	if (type_unmatch)
		errorHandler.printError(CONST_TYPE_UNMATCH, TOKEN_PEEK(-1).getLinenum());
	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	subNode = node;
	return true;
}

/*
<有返回值函数定义>  ::=  <声明头部>'('<参数表>')' '{'<复合语句>'}'
<有返回值函数定义>  ::=  <声明头部>[LPARENT]<参数表>[RPARENT] [LBRACE]<复合语句>[RBRACE]
FIRST INTTK CHARTK
*/
bool Parser::funcWithReturn() {
	AST_node node("<有返回值函数定义>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;
	has_return = false;

	if (!declareHead(&ste)) {
		cout << "语法分析<有返回值函数定义>: 声明头部错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		cout << "语法分析<有返回值函数定义>: 缺少左括号" << endl;
		return false;
	}

	if (!paramList(&ste)) {
		cout << "语法分析<有返回值函数定义>: 参数列表错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		cout << "语法分析<有返回值函数定义>: 缺少左大括号" << endl;
		return false;
	}
	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	if (!comStatement()) {
		cout << "语法分析<有返回值函数定义>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!has_return)
		errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		cout << "语法分析<有返回值函数定义>: 缺少右大括号" << endl;
		return false;
	}

	// 符号表退出子层
	symbolTableManager.goOut();

	subNode = node;
	return true;
}

/*
<无返回值函数定义>  ::= void<标识符>'('<参数表>')''{'<复合语句>'}'
<无返回值函数定义>  ::= [VOIDTK]<标识符>[LPARENT]<参数表>[RPARENT][LBRACE]<复合语句>[RBRACE]
FIRST VOIDTK
*/

bool Parser::funcNoReturn() {
	AST_node node("<无返回值函数定义>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;

	if (!checkToken(VOIDTK, "<VOIDTK>", &node)) {
		cout << "语法分析<无返回值函数定义>: 缺少void" << endl;
		return false;
	}
	ste.valueType = ValueType::VOID;

	if (curToken->isType(IDENFR)) {
		ste.name = curToken->getTokenStr();
		funcNames.insert(make_pair(curToken->getTokenStr(), false));
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		cout << "语法分析<<无返回值函数定义>: 缺少标识符" << endl;
		return false;
	}

	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		cout << "语法分析<无返回值函数定义>: 缺少左括号" << endl;
		return false;
	}

	if (!paramList(&ste)) {
		cout << "语法分析<无返回值函数定义>: 参数列表错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}

	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		cout << "语法分析<无返回值函数定义>: 缺少左大括号" << endl;
		return false;
	}

	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	if (!comStatement()) {
		cout << "语法分析<无返回值函数定义>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		cout << "语法分析<无返回值函数定义>: 缺少右大括号" << endl;
		return false;
	}

	// 符号表退出子层
	symbolTableManager.goOut();

	subNode = node;
	return true;
}

/*
<复合语句>	::=  ［<常量说明>］［<变量说明>］<语句列>
<复合语句>	::=  ［<常量说明>］［<变量说明>］<语句列>
FIRST CONSTTK INTTK CHARTK WHILETK FORTK IFTK IDENFR IDENFR IDENFR SCANFTK PRINTFTK SWITCHTK SEMICN RETURNTK LBRACE <空>
*/
bool Parser::comStatement() {
	AST_node node("<复合语句>", true);
	if (curToken->isType(CONSTTK)) {
		if (!constDeclare()) {
			cout << "语法分析<复合语句>:  常量说明错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}

	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDeclare()) {
			cout << "语法分析<复合语句>:  变量说明错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}

	if (!statementList()) {
		cout << "语法分析<复合语句>:  语句列错误" << endl;
		return false;
	}
	node.addNode(subNode);

	subNode = node;
	return true;
}

/*
<参数表>	::=  <类型标识符><标识符>{,<类型标识符><标识符>}| <空>
<参数表>	::= <类型标识符><标识符>{[COMMA]<类型标识符><标识符>} |<空>
FIRST INTTK CHARTK EMPTY
*/
bool Parser::paramList(STE* ste) {
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
		if (!checkToken(IDENFR, "<标识符>", &node)) {
			cout << "语法分析<参数表>: 缺少标识符" << endl;
			return false;
		}
		args.emplace_back(argu);
		if(!symbolTableManager.insert(param_ste, ste))
			errorHandler.printError(DUP_DEFINE, curToken->getLinenum());

		while (curToken->isType(COMMA)) {
			checkToken(COMMA, "<COMMA>", &node);

			if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
				cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
				return false;
			}
			argu.type = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
			param_ste.valueType = curToken->isType(INTTK) ? ValueType::INTEGER : ValueType::CHAR;
			ADD_TOKENNODE("<类型标识符>", curToken, node);
			curToken = &TOKEN_GET;

			argu.name = curToken->getTokenStr();
			param_ste.name = curToken->getTokenStr();
			if (!checkToken(IDENFR, "<标识符>", &node)) {
				cout << "语法分析<参数表>: 缺少标识符" << endl;
				return false;
			}
			args.emplace_back(argu);
			if (!symbolTableManager.insert(param_ste, ste))
				errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
		}

	}

	//填表
	ste->args = args;

	subNode = node;
	return true;
}

/*
<主函数>    ::= void main‘(’‘)’ ‘{’<复合语句>‘}’
<主函数>    ::= [VOIDTK] [MAINTK][LPARENT][RPARENT] [LBRACE]<复合语句>[RBRACE]
FIRST VOIDTK
*/
bool Parser::mainFunc() {
	AST_node node("<主函数>", true);
	STE ste;
	ste.identType = IdentType::FUNCTION;

	if (!checkToken(VOIDTK, "<VOIDTK>", &node) || !checkToken(MAINTK, "<MAINTK>", &node) 
		|| !checkToken(LPARENT, "<LPARENT>", &node)) {
		cout << "语法分析<主函数>: 函数头错误" << endl;
		return false;
	}

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		cout << "语法分析<主函数>: 函数头错误" << endl;
			return false;
	}

	ste.name = "main";
	ste.valueType = ValueType::VOID;
	//填表
	if (!symbolTableManager.insert(ste)) {
		// 错误处理：定义重名
		errorHandler.printError(DUP_DEFINE, curToken->getLinenum());
	}
	//符号表进入下一层
	symbolTableManager.goInto(ste.name);

	if (!comStatement()) {
		cout << "语法分析<主函数>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		cout << "语法分析<主函数>: 缺号左括号" << endl;
		return false;
	}

	// 符号表退出子层
	symbolTableManager.goOut();

	subNode = node;
	return true;
}

/*
<表达式>    ::= ［＋｜－］<项>{<加法运算符><项>}
<表达式>    ::= ［[PLUS] | [MINU]］<项> {<加法运算符><项>}
FIRST PLUS MINU IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::expression() {
	AST_node node("<表达式>", true);
	isChar = false;
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	if (!item()) {
		cout << "语法分析<表达式>: 项分析错误" << endl;
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(PLUS) || curToken->isType(MINU)) {
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		if (!item()) {
			cout << "语法分析<表达式>: 项分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
		isChar = false;
	}
	subNode = node;
	return true;
}

/*
<项>	::= <因子>{<乘法运算符><因子>}
<项>	::= <因子> { <乘法运算符><因子> }
FIRST IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::item() {
	AST_node node("<项>", true);
	if (!factor()) {
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(MULT) || curToken->isType(DIV))
	{
		ADD_TOKENNODE("<乘法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		if (!factor()) {
			return false;
		}
		node.addNode(subNode);
		isChar = false;
	}
	subNode = node;
	return true;
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
bool Parser::factor() {
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
		bool old_ischar = isChar;
		if (!callFuncWithReturn()) {
			return false;
		}
		node.addNode(subNode);
		isChar = old_ischar;
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
		}
		checkToken(IDENFR, "<标识符>", &node);

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);

			bool old_ischar = isChar;
			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
			if (isChar) {
				errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
			}
			isChar = old_ischar;

			
			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);

				bool old_ischar = isChar;
				if (!expression()) {
					return false;
				}
				node.addNode(subNode);
				if (isChar) {
					errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
				}
				isChar = old_ischar;

				if (curToken->isType(RBRACK))
					checkToken(RBRACK, "<RBRACK>", &node);
				else
					errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
			}
		}
	}
	else if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);
		if (!expression()) {
			return false;
		}
		node.addNode(subNode);

		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU)) {
		if (!integer()) {
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(CHARCON)) {
		isChar = true;
		if (!checkToken(CHARCON, "<CHARCON>", &node)) {
			return false;
		}
	}
	else
	{
		return false;
	}

	subNode = node;
	return true;
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
bool Parser::statement() {
	AST_node node("<语句>", true);
	if (curToken->isType(WHILETK) || curToken->isType(FORTK)) {
		if (!loopStatement()) {
			cout << "语法分析<语句>: 循环语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(IFTK)) {
		if (!ifStatement()) {
			cout << "语法分析<语句>: 条件语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(IDENFR)) {
		if (TOKEN_PEEK(1).isType(LPARENT) && funcNames[curToken->getTokenStr()]) {
			if (!callFuncWithReturn()) {
				cout << "语法分析<语句>: 有返回值函数调用语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else if(TOKEN_PEEK(1).isType(LPARENT) && !funcNames[curToken->getTokenStr()]){
			if (!callFuncNoReturn()) {
				cout << "语法分析<语句>: 无返回值函数调用语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else
		{
			if (!assignStatement()) {
				cout << "语法分析<语句>: 赋值语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(SCANFTK)) {
		if (!scanfStatement()) {
			cout << "语法分析<语句>: 读语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(PRINTFTK)) {
		if (!printfStatement()) {
			cout << "语法分析<语句>: 写语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(SWITCHTK)) {
		if (!switchStatement()) {
			cout << "语法分析<语句>: 情况语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(RETURNTK)) {
		if (!returnStatement()) {
			cout << "语法分析<语句>: 返回语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	else if (curToken->isType(LBRACE)) {
		checkToken(LBRACE, "<LBRACE>", &node);
		if (!statementList()) {
			cout << "语法分析<语句>: 语句列错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RBRACE, "<RBRACE>", &node)) {
			cout << "语法分析<语句>: 语句列缺少左括号" << endl;
			return false;
		}
	}
	else {
		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());
	}
	subNode = node;
	return true;
}

/*
<赋值语句>   ::=  <标识符>＝<表达式>|<标识符>'['<表达式>']'=<表达式>|<标识符>'['<表达式>']''['<表达式>']' =<表达式>
<赋值语句>  ::=  <标识符>[ASSIGN]<表达式>|
				  <标识符>[LBRACK]<表达式>[RBRACK][ASSIGN]<表达式>|
				  <标识符>[LBRACK]<表达式>[RBRACK][LBRACK]<表达式>[RBRACK] [ASSIGN]<表达式>
FIRST IDENFR
*/
bool Parser::assignStatement() {
	AST_node node("<赋值语句>", true);

	if (curToken->isType(IDENFR)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		STE* ste = symbolTableManager.curSTE;
		if (ste->identType == IdentType::CONST) {
			errorHandler.printError(ALTER_CONST_VALUE, curToken->getLinenum());
		}
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		return false;
	}

	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);

		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
		if (isChar) {
			errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
		}

		if (curToken->isType(RBRACK))
			checkToken(RBRACK, "<RBRACK>", &node);
		else
			errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);

			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
			if (isChar) {
				errorHandler.printError(ARRAY_INDEX_TYPE_UNMATCH, curToken->getLinenum());
			}

			if (curToken->isType(RBRACK))
				checkToken(RBRACK, "<RBRACK>", &node);
			else
				errorHandler.printError(MISSING_RBRACK, TOKEN_PEEK(-1).getLinenum());
		}
	}
	if (!checkToken(ASSIGN, "<ASSIGN>", &node)) {
		return false;
	}
	if (!expression()) {
		return false;
	}
	node.addNode(subNode);
	subNode = node;
	return true;
}

/*
<条件语句>  ::= if '('<条件>')'<语句>［else<语句>］
<条件语句>  ::= [IFTK] [LPARENT]<条件>[RPARENT]<语句>［[ELSETK]<语句>］
FIRST IFTK
*/
bool Parser::ifStatement() {
	AST_node node("<条件语句>", true);
	if (!checkToken(IFTK, "<IFTK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!condition()) {
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());

	if (!statement()) {
		return false;
	}
	node.addNode(subNode);
	if (curToken->isType(ELSETK)) {
		checkToken(ELSETK, "<ELSETK>", &node);
		if (!statement()) {
			return false;
		}
		node.addNode(subNode);
	}
	subNode = node;
	return true;
}

/*
<条件>    ::=  <表达式><关系运算符><表达式>
<条件>    ::=  <表达式><关系运算符><表达式>
FIRST PLUS MINU IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
(symbol == LSS || symbol == LEQ || symbol == GRE
		|| symbol == GEQ || symbol == EQL || symbol == NEQ)
*/
bool Parser::condition() {
	AST_node node("<条件>", true);
	bool ischar = false;

	if (!expression()) {
		return false;
	}
	node.addNode(subNode);
	ischar = isChar ? true: ischar;

	if (curToken->isType(LSS) || curToken->isType(LEQ) || curToken->isType(GRE)
		|| curToken->isType(GEQ) || curToken->isType(EQL) || curToken->isType(NEQ)) {
		ADD_TOKENNODE("<关系运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		return false;
	}

	if (!expression()) {
		return false;
	}
	node.addNode(subNode);
	ischar = isChar ? true : ischar;

	// 错误处理：不合法类型
	if (ischar)
		errorHandler.printError(ILLEGAL_CONDITION_TYPE, curToken->getLinenum());

	subNode = node;
	return true;
}

/*
<循环语句>   ::=  while '('<条件>')'<语句>| for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
<循环语句>   ::=  [WHILETK] [LPARENT]<条件>[RPARENT]<语句>|
				 [FORTK][LPARENT]<标识符>[ASSIGN]<表达式>[SEMICN]<条件>[SEMICN]<标识符> [ASSIGN]<标识符>([PLUS]|[MINU])<步长>[RPARENT]<语句>

FIRST WHILETK FORTK
*/
bool Parser::loopStatement() {
	AST_node node("<循环语句>", true);
	if (curToken->isType(WHILETK)) {
		if (!checkToken(WHILETK, "<WHILETK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)) {
			return false;
		}
		if (!condition()) {
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
		if (!statement()) {
			return false;
		}
		node.addNode(subNode);
	}
	else {
		if (!checkToken(FORTK, "<FORTK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)) {
			return false;
		}

		if (curToken->isType(IDENFR)) {
			if (!symbolTableManager.find(curToken->getTokenStr())) {
				errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			}
			checkToken(IDENFR, "<标识符>", &node);
		}
		else {
			return false;
		}

		if (!checkToken(ASSIGN, "<ASSIGN>", &node)) {
			return false;
		}

		if (!expression()) {
			return false;
		}
		node.addNode(subNode);

		if (curToken->isType(SEMICN))
			checkToken(SEMICN, "<SEMICN>", &node);
		else
			errorHandler.printError(MISSING_SEMICN, TOKEN_PEEK(-1).getLinenum());

		if (!condition()) {
			return false;
		}
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
		else {
			return false;
		}

		if (!checkToken(ASSIGN, "<ASSIGN>", &node)) {
			return false;
		}

		if (curToken->isType(IDENFR)) {
			if (!symbolTableManager.find(curToken->getTokenStr())) {
				errorHandler.printError(NO_DEFINE, curToken->getLinenum());
			}
			checkToken(IDENFR, "<标识符>", &node);
		}
		else {
			return false;
		}

		if (curToken->isType(PLUS) || curToken->isType(MINU)) {
			ADD_TOKENNODE("<加法运算符>", curToken, node);
			curToken = &TOKEN_GET;
		}
		if (!step()) {
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(RPARENT))
			checkToken(RPARENT, "<RPARENT>", &node);
		else
			errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
		if (!statement()) {
			return false;
		}
		node.addNode(subNode);
	}
	subNode = node;
	return true;
}

/*
<步长>::= <无符号整数>
<步长>::= <无符号整数>
*/
bool Parser::step() {
	AST_node node("<步长>", true);
	if (!unsignedInt()) {
		return false;
	}
	node.addNode(subNode);
	subNode = node;
	return true;
}

/*
<情况语句>  ::=  switch ‘(’<表达式>‘)’ ‘{’<情况表><缺省>‘}’
<情况语句>  ::=  [SWITCHTK] [LPARENT]<表达式>[RPARENT] [LBRACE]<情况表><缺省>[RBRACE]

FIRST SWITCHTK
*/
bool Parser::switchStatement() {
	bool ischar = false;
	AST_node node("<情况语句>", true);
	if (!checkToken(SWITCHTK, "<SWITCHTK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!expression()) {
		return false;
	}
	node.addNode(subNode);
	ischar = isChar;
	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		return false;
	}
	if (!caseList(ischar)) {
		return false;
	}
	node.addNode(subNode);
	if (curToken->isType(DEFAULTTK)) {
		if (!defaultStatement()) {
			return false;
		}
		node.addNode(subNode);
	}
	else
		errorHandler.printError(MISSING_DEFAULT, curToken->getLinenum());
	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		return false;
	}
	subNode = node;
	return true;
}

/*
<情况表>   ::=  <情况子语句>{<情况子语句>}
<情况表>   ::=  <情况子语句>{<情况子语句>}

FIRST CASETK
*/
bool Parser::caseList(bool ischar) {
	AST_node node("<情况表>", true);
	if (!caseStatement(ischar)) {
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(CASETK)) {
		if (!caseStatement(ischar)) {
			return false;
		}
		node.addNode(subNode);
	}
	subNode = node;
	return true;
}

/*
<情况子语句>  ::=  case<常量>：<语句>
<情况子语句>  ::=  [CASETK]<常量>[COLON]<语句>

FIRST CASETK
*/
bool Parser::caseStatement(bool ischar) {
	AST_node node("<情况子语句>", true);
	if (!checkToken(CASETK, "<CASETK>", &node)) {
		return false;
	}
	if ((ischar && !curToken->isType(CHARCON)) || (!ischar && curToken->isType(CHARCON)))
		errorHandler.printError(CONST_TYPE_UNMATCH, curToken->getLinenum());
	if (!constant()) {
		return false;
	}
	node.addNode(subNode);
	if (!checkToken(COLON, "<COLON>", &node)) {
		return false;
	}
	if (!statement()) {
		return false;
	}
	node.addNode(subNode);
	subNode = node;
	return true;
}

/*
<缺省>   ::=  default :<语句>
<缺省>   ::=  [DEFAULTTK] [COLON] <语句>

FIRST DEFAULTTK
*/
bool Parser::defaultStatement() {
	AST_node node("<缺省>", true);
	if (!checkToken(DEFAULTTK, "<DEFAULTTK>", &node)) {
		return false;
	}
	if (!checkToken(COLON, "<COLON>", &node)) {
		return false;
	}
	if (!statement()) {
		return false;
	}
	node.addNode(subNode);
	subNode = node;
	return true;
}

/*
<有返回值函数调用语句> ::= <标识符>'('<值参数表>')'
<有返回值函数调用语句> ::=<标识符>[LPARENT]<值参数表>[RPARENT]

FIRST IDENFR
*/
bool Parser::callFuncWithReturn() {
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
	else {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!valueParamList(funcName)) {
		return false;
	}
	node.addNode(subNode);
	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	subNode = node;
	END:
	return true;
}

/*
<无返回值函数调用语句> ::= <标识符>'('<值参数表>')'
<无返回值函数调用语句> ::=<标识符>[LPARENT]<值参数表>[RPARENT]

FIRST IDENFR
*/
bool Parser::callFuncNoReturn() {
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
	else {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!valueParamList(funcName)) {
		return false;
	}
	node.addNode(subNode);
	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	subNode = node;
	END:
	return true;
}

/*
<值参数表>   ::= <表达式>{,<表达式>}｜<空>
<值参数表>   ::= <表达式> {[COMMA]<表达式>}|<空>

FIRST IDENFR LPARENT PLUS MINU INTCON CHARCON
*/
bool Parser::valueParamList(string funcName) {
	AST_node node("<值参数表>", true);

	STE* ste = symbolTableManager.curSTE;
	vector<Argument> args = ste->args;
	int argsNum = args.size();
	int count = 0;
	bool typeUnmach = false;
	vector<Argument>::iterator iter = args.begin();

	if (curToken->isType(IDENFR) || curToken->isType(LPARENT) || curToken->isType(PLUS)
		|| curToken->isType(MINU) || curToken->isType(INTCON) || curToken->isType(CHARCON)) {


		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
		if (iter != args.end()) {
			if (((*iter).type == ValueType::CHAR && !isChar)
				|| ((*iter).type == ValueType::INTEGER && isChar))
				typeUnmach = true;
			iter++;
		}
		count++;


		while (curToken->isType(COMMA))
		{
			checkToken(COMMA, "<COMMA>", &node);

			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
			if (iter != args.end()) {
				if (((*iter).type == ValueType::CHAR && !isChar)
					|| ((*iter).type == ValueType::INTEGER && isChar))
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
	return true;
}

/*
<语句列>   ::= ｛<语句>｝
<语句列>   ::= ｛<语句>｝

FIRST WHILETK FORTK IFTK IDENFR IDENFR IDENFR SCANFTK PRINTFTK SWITCHTK SEMICN RETURNTK LBRACE <空>
*/
bool Parser::statementList() {
	AST_node node("<语句列>", true);
	while (!curToken->isType(RBRACE)) {
		if (!statement()) {
			return false;
		}
		node.addNode(subNode);
	}
	subNode = node;
	return true;
}

/*
<读语句>    ::=  scanf '('<标识符>')'
<读语句>    ::=  [SCANFTK] [LPARENT]<标识符> [RPARENT]

FIRST SCANFTK
*/
bool Parser::scanfStatement() {
	AST_node node("<读语句>", true);
	if (!checkToken(SCANFTK, "<SCANFTK>", &node)) {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}

	if (curToken->isType(IDENFR)) {
		if (!symbolTableManager.find(curToken->getTokenStr())) {
			errorHandler.printError(NO_DEFINE, curToken->getLinenum());
		}
		STE* ste = symbolTableManager.curSTE;
		if (ste->identType == IdentType::CONST) {
			errorHandler.printError(ALTER_CONST_VALUE, curToken->getLinenum());
		}
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		return false;
	}

	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	subNode = node;
	return true;
}

/*
<写语句>    ::= printf '(' <字符串>,<表达式> ')'| printf '('<字符串> ')'| printf '('<表达式>')'
<写语句>    ::= [PRINTFTK] [LPARENT] ( <字符串>[COMMA]<表达式> [RPARENT] | <字符串> [RPARENT] | <表达式>[RPARENT] )

FIRST PRINTFTK
*/
bool Parser::printfStatement() {
	AST_node node("<写语句>", true);
	if (!checkToken(PRINTFTK, "<PRINTFTK>", &node)) {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (curToken->isType(STRCON)) {
		if (!String()) {
			return false;
		}
		node.addNode(subNode);
		if (curToken->isType(COMMA)) {
			checkToken(COMMA, "COMMA", &node);
			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
		}
	}
	else {
		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
	}
	if (curToken->isType(RPARENT))
		checkToken(RPARENT, "<RPARENT>", &node);
	else
		errorHandler.printError(MISSING_RPARENT, TOKEN_PEEK(-1).getLinenum());
	subNode = node;
	return true;
}

/*
<返回语句>   ::=  return['('<表达式>')']
<返回语句>   ::=  [RETURNTK] [ [LPARENT] <表达式> [RPARENT] ]

FIRST RETURNTK
*/
bool Parser::returnStatement() {
	AST_node node("<返回语句>", true);
	if (!checkToken(RETURNTK, "<RETURNTK>", &node)) {
		return false;
	}
	has_return = true;
	if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);

		if (symbolTableManager.getPosType() == ValueType::VOID)
			errorHandler.printError(NO_RETURN_FUNC_ERROR, curToken->getLinenum());

		if (curToken->isType(RPARENT) && symbolTableManager.getPosType() != ValueType::VOID) {
			errorHandler.printError(RETURN_FUNC_ERROR, curToken->getLinenum());
		}
		else{
			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
			if ((symbolTableManager.getPosType() == ValueType::INTEGER && isChar)
				|| (symbolTableManager.getPosType() == ValueType::CHAR && !isChar)) {
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
	}
	subNode = node;
	return true;
}