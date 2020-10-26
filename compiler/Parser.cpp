#include "Parser.h"

void Parser::initial() {
	TOKEN_BEGIN;
	curToken = &TOKEN_GET;
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
			// TODO: 错误处理
			cout << "语法分析<程序>: 常量分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDeclare()) {
			// TODO: 错误处理
			cout << "语法分析<程序>: 变量分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	while (true)
	{
		if (TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK)) {
			if (!funcWithReturn()) {
				// TODO: 错误处理
				cout << "语法分析<程序>: 有返回值的函数错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else if (TOKEN_PEEK(0).isType(VOIDTK) && TOKEN_PEEK(1).isType(IDENFR)) {
			if (!funcNoReturn()) {
				// TODO: 错误处理
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
		// TODO: 错误处理
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
		// TODO: 错误处理
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
		// TODO: 错误处理
		cout << "语法分析<无符号整数>: 不是无符号整数" << endl;
		return false;
	}
	AST_node node("<无符号整数>", true);
	node.addToken(curToken);
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
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	if (!unsignedInt()) {
		// TODO: 错误处理
		cout << "语法分析<无符号整数>: 不是整数" << endl;
		return false;
	}
	node.addNode(subNode);
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

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		// TODO: 错误处理
		cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
		return false;
	}
	int const_type = curToken->getType();
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (!checkToken(IDENFR, "<标识符>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<常量定义>: 缺少标识符" << endl;
		return false;
	}

	if (!checkToken(ASSIGN, "<赋值符号>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<常量定义>: 缺少赋值符号" << endl;
		return false;
	}

	if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
		if (!integer()) {
			// TODO: 错误处理
			cout << "语法分析<常量定义>: 整数赋值错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (const_type == CHARTK && curToken->isType(CHARCON)) {
		ADD_TOKENNODE("<字符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		// TODO: 错误处理
		cout << "语法分析<常量定义>: 常量定义类型不一致" << endl;
		return false;
	}

	while (curToken->isType(COMMA)) {
		ADD_TOKENNODE("<COMMA>", curToken, node);
		curToken = &TOKEN_GET;

		if (!checkToken(IDENFR, "<标识符>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<常量定义>: 缺少标识符" << endl;
			return false;
		}

		if (!checkToken(ASSIGN, "<赋值符号>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<常量定义>: 缺少赋值符号" << endl;
			return false;
		}

		if (const_type == INTTK && (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU))) {
			if (!integer()) {
				// TODO: 错误处理
				cout << "语法分析<常量定义>: 整数赋值错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else if (const_type == CHARTK && curToken->isType(CHARCON)) {
			ADD_TOKENNODE("<字符>", curToken, node);
			curToken = &TOKEN_GET;
		}
		else {
			// TODO: 错误处理
			cout << "语法分析<常量定义>: 常量定义类型不一致" << endl;
			return false;
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
		// TODO: 错误处理
		cout << "语法分析<常量说明>: 缺少const" << endl;
		return false;
	}

	if (!constDef()) {
		// TODO: 错误处理
		cout << "语法分析<常量说明>: 缺少常量定义" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(SEMICN, "<SEMICN>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<常量说明>: 缺少分号" << endl;
		return false;
	}

	while (curToken->isType(CONSTTK)) {
		ADD_TOKENNODE("<CONST>", curToken, node);
		curToken = &TOKEN_GET;

		if (!constDef()) {
			// TODO: 错误处理
			cout << "语法分析<常量说明>: 缺少常量定义" << endl;
			return false;
		}
		node.addNode(subNode);

		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<常量说明>: 缺少分号" << endl;
			return false;
		}
	}
	subNode = node;
	return true;
}

/*
<声明头部>   ::=  int<标识符> |char<标识符>
<声明头部>   ::=  [INTTK]<标识符> | [CHARTK]<标识符>
FIRST INTTK CHARTK
*/
bool Parser::declareHead() {
	AST_node node("<声明头部>", true);

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		// TODO: 错误处理
		cout << "语法分析<声明头部>: 缺少类型标识符" << endl;
		return false;
	}
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (curToken->isType(IDENFR)) {
		has_return.insert(make_pair(curToken->getTokenStr(), true));
		checkToken(IDENFR, "<标识符>", &node);
	}
	else {
		// TODO: 错误处理
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
			// TODO: 错误处理
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
		// TODO: 错误处理
		cout << "语法分析<变量说明>: 变量定义错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(SEMICN, "<SEMICN>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<变量说明>: 缺少分号" << endl;
		return false;
	}

	while ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDef()) {
			// TODO: 错误处理
			cout << "语法分析<变量说明>: 变量定义错误" << endl;
			return false;
		}
		node.addNode(subNode);

		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<变量说明>: 缺少分号" << endl;
			return false;
		}
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
	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK)) 
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (TOKEN_PEEK(2).isType(ASSIGN) 
			|| (TOKEN_PEEK(2).isType(LBRACK) && TOKEN_PEEK(3).isType(INTCON) && TOKEN_PEEK(4).isType(RBRACK) 
				&& TOKEN_PEEK(5).isType(ASSIGN))
			|| (TOKEN_PEEK(2).isType(LBRACK) && TOKEN_PEEK(3).isType(INTCON) && TOKEN_PEEK(4).isType(RBRACK)
				&& TOKEN_PEEK(5).isType(LBRACK) && TOKEN_PEEK(6).isType(INTCON) && TOKEN_PEEK(7).isType(RBRACK)
				&& TOKEN_PEEK(8).isType(ASSIGN))) {
			if (!varDefWithInitial()) {
				// TODO: 错误处理
				cout << "语法分析<变量定义>: 变量定义及初始化错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else {
			if (!varDefNoInitial()) {
				// TODO: 错误处理
				cout << "语法分析<变量定义>: 变量定义无初始化错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
	}
	else {
		// TODO: 错误处理
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

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		// TODO: 错误处理
		cout << "语法分析<变量定义无初始化>: 缺少类型标识符" << endl;
		return false;
	}
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (!checkToken(IDENFR, "<标识符>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<变量定义无初始化>: 缺少标识符" << endl;
		return false;
	}

	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		if (!unsignedInt()) {
			// TODO: 错误处理
			cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RBRACK, "<RBRACK>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<变量定义无初始化>: 缺少左括号" << endl;
			return false;
		}

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			if (!unsignedInt()) {
				// TODO: 错误处理
				cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
				return false;
			}
			node.addNode(subNode);
			if (!checkToken(RBRACK, "<RBRACK>", &node)) {
				// TODO: 错误处理
				cout << "语法分析<变量定义无初始化>: 缺少左括号" << endl;
				return false;
			}
		}
	}

	while (curToken->isType(COMMA)) {
		checkToken(COMMA, "<COMMA>", &node);

		if (!checkToken(IDENFR, "<标识符>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<变量定义无初始化>: 缺少标识符" << endl;
			return false;
		}

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			if (!unsignedInt()) {
				// TODO: 错误处理
				cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
				return false;
			}
			node.addNode(subNode);
			if (!checkToken(RBRACK, "<RBRACK>", &node)) {
				// TODO: 错误处理
				cout << "语法分析<变量定义无初始化>: 缺少左括号" << endl;
				return false;
			}

			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);
				if (!unsignedInt()) {
					// TODO: 错误处理
					cout << "语法分析<变量定义无初始化>: 数组定义缺少维度" << endl;
					return false;
				}
				node.addNode(subNode);
				if (!checkToken(RBRACK, "<RBRACK>", &node)) {
					// TODO: 错误处理
					cout << "语法分析<变量定义无初始化>: 缺少左括号" << endl;
					return false;
				}
			}
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
	AST_node node("<变量定义及初始化>", true);

	if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
		// TODO: 错误处理
		return false;
	}
	ADD_TOKENNODE("<类型标识符>", curToken, node);
	curToken = &TOKEN_GET;

	if (!checkToken(IDENFR, "<标识符>", &node)) {
		// TODO: 错误处理
		return false;
	}

	if (curToken->isType(ASSIGN)) {
		checkToken(ASSIGN, "ASSIGN", &node);

		if (!constant()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		if (!unsignedInt()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RBRACK, "<RBRACK>", &node)) {
			// TODO: 错误处理
			return false;
		}

		if (curToken->isType(ASSIGN)) {
			checkToken(ASSIGN, "ASSIGN", &node);
			if (!checkToken(LBRACE, "<LBRACE>", &node)) {
				// TODO: 错误处理
				return false;
			}
			if (!constant()) {
				// TODO: 错误处理
				return false;
			}
			node.addNode(subNode);
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if (!constant()) {
					// TODO: 错误处理
					return false;
				}
				node.addNode(subNode);
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				// TODO: 错误处理
				return false;
			}
		}
		else if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			if (!unsignedInt()) {
				// TODO: 错误处理
				return false;
			}
			node.addNode(subNode);
			if (!checkToken(RBRACK, "<RBRACK>", &node) || !checkToken(ASSIGN, "<ASSING>", &node) 
				|| !checkToken(LBRACE, "<LBRACE>", &node) || !checkToken(LBRACE, "<LBRACE>", &node)) {
				// TODO: 错误处理
				return false;
			}
			if (!constant()) {
				// TODO: 错误处理
				return false;
			}
			node.addNode(subNode);
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if (!constant()) {
					// TODO: 错误处理
					return false;
				}
				node.addNode(subNode);
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				// TODO: 错误处理
				return false;
			}
			while (curToken->isType(COMMA)) {
				checkToken(COMMA, "<COMMA>", &node);
				if (!checkToken(LBRACE, "<LBRACE>", &node)) {
					// TODO: 错误处理
					return false;
				}
				if (!constant()) {
					// TODO: 错误处理
					return false;
				}
				node.addNode(subNode);
				while (curToken->isType(COMMA)) {
					checkToken(COMMA, "<COMMA>", &node);
					if (!constant()) {
						// TODO: 错误处理
						return false;
					}
					node.addNode(subNode);
				}
				if (!checkToken(RBRACE, "<RBRACE>", &node)) {
					// TODO: 错误处理
					return false;
				}
			}
			if (!checkToken(RBRACE, "<RBRACE>", &node)) {
				// TODO: 错误处理
				return false;
			}
		}
	}
	else {
		// TODO: 错误处理
		return false;
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
	if (!declareHead()) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 声明头部错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 缺少左括号" << endl;
		return false;
	}

	if (!paramList()) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 参数列表错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 缺少右括号" << endl;
		return false;
	}

	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 缺少左大括号" << endl;
		return false;
	}

	if (!comStatement()) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<有返回值函数定义>: 缺少右大括号" << endl;
		return false;
	}

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
	if (!checkToken(VOIDTK, "<VOIDTK>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 缺少void" << endl;
		return false;
	}

	if (curToken->isType(IDENFR)) {
		checkToken(IDENFR, "<标识符>", &node);
		has_return.insert(make_pair(curToken->getTokenStr(), false));
	}
	else {
		// TODO: 错误处理
		cout << "语法分析<<无返回值函数定义>: 缺少标识符" << endl;
		return false;
	}

	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 缺少左括号" << endl;
		return false;
	}

	if (!paramList()) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 参数列表错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 缺少右括号" << endl;
		return false;
	}

	if (!checkToken(LBRACE, "<LBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 缺少左大括号" << endl;
		return false;
	}

	if (!comStatement()) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<无返回值函数定义>: 缺少右大括号" << endl;
		return false;
	}

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
			// TODO: 错误处理
			cout << "语法分析<复合语句>:  常量说明错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}

	if ((TOKEN_PEEK(0).isType(INTTK) || TOKEN_PEEK(0).isType(CHARTK))
		&& TOKEN_PEEK(1).isType(IDENFR) && !TOKEN_PEEK(2).isType(LPARENT)) {
		if (!varDeclare()) {
			// TODO: 错误处理
			cout << "语法分析<复合语句>:  变量说明错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}

	if (!statementList()) {
		// TODO: 错误处理
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
bool Parser::paramList() {
	AST_node node("<参数表>", true);

	if (curToken->isType(INTTK) || curToken->isType(CHARTK)) {
		ADD_TOKENNODE("<类型标识符>", curToken, node);
		curToken = &TOKEN_GET;

		if (!checkToken(IDENFR, "<标识符>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<参数表>: 缺少标识符" << endl;
			return false;
		}

		while (curToken->isType(COMMA)) {
			checkToken(COMMA, "<COMMA>", &node);

			if (!curToken->isType(INTTK) && !curToken->isType(CHARTK)) {
				// TODO: 错误处理
				cout << "语法分析<常量定义>: 缺少类型标识符" << endl;
				return false;
			}
			ADD_TOKENNODE("<类型标识符>", curToken, node);
			curToken = &TOKEN_GET;

			if (!checkToken(IDENFR, "<标识符>", &node)) {
				// TODO: 错误处理
				cout << "语法分析<参数表>: 缺少标识符" << endl;
				return false;
			}
		}

	}

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
	if (!checkToken(VOIDTK, "<VOIDTK>", &node) || !checkToken(MAINTK, "<MAINTK>", &node) 
		|| !checkToken(LPARENT, "<LPARENT>", &node) || !checkToken(RPARENT, "<RPARENT>", &node)
		|| !checkToken(LBRACE, "<LBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<主函数>: 函数头错误" << endl;
		return false;
	}

	if (!comStatement()) {
		// TODO: 错误处理
		cout << "语法分析<主函数>: 复合语句错误" << endl;
		return false;
	}
	node.addNode(subNode);

	if (!checkToken(RBRACE, "<RBRACE>", &node)) {
		// TODO: 错误处理
		cout << "语法分析<主函数>: 缺号左括号" << endl;
		return false;
	}

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
	if (curToken->isType(PLUS) || curToken->isType(MINU)) {
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	if (!item()) {
		// TODO: 错误处理
		cout << "语法分析<表达式>: 项分析错误" << endl;
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(PLUS) || curToken->isType(MINU)) {
		ADD_TOKENNODE("<加法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		if (!item()) {
			// TODO: 错误处理
			cout << "语法分析<表达式>: 项分析错误" << endl;
			return false;
		}
		node.addNode(subNode);
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
		// TODO: 错误处理
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(MULT) || curToken->isType(DIV))
	{
		ADD_TOKENNODE("<乘法运算符>", curToken, node);
		curToken = &TOKEN_GET;

		if (!factor()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
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
		if (!callFuncWithReturn()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(IDENFR)) {
		checkToken(IDENFR, "<标识符>", &node);

		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			if (!expression()) {
				// TODO: 错误处理
				return false;
			}
			node.addNode(subNode);
			if (!checkToken(RBRACK, "<RBRACK>", &node)) {
				// TODO: 错误处理
				return false;
			}
			if (curToken->isType(LBRACK)) {
				checkToken(LBRACK, "<LBRACK>", &node);
				if (!expression()) {
					// TODO: 错误处理
					return false;
				}
				node.addNode(subNode);
				if (!checkToken(RBRACK, "<RBRACK>", &node)) {
					// TODO: 错误处理
					return false;
				}
			}
		}
	}
	else if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);
		if (!expression()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RPARENT, "<RPARENT>", &node)) {
			// TODO: 错误处理
			return false;
		}
	}
	else if (curToken->isType(INTCON) || curToken->isType(PLUS) || curToken->isType(MINU)) {
		if (!integer()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(CHARCON)) {
		if (!checkToken(CHARCON, "<CHARCON>", &node)) {
			// TODO: 错误处理
			return false;
		}
	}
	else
	{
		// TODO: 错误处理
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
			// TODO: 错误处理
			cout << "语法分析<语句>: 循环语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(IFTK)) {
		if (!ifStatement()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 条件语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(IDENFR)) {
		if (TOKEN_PEEK(1).isType(LPARENT) && has_return[curToken->getTokenStr()]) {
			if (!callFuncWithReturn()) {
				// TODO: 错误处理
				cout << "语法分析<语句>: 有返回值函数调用语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else if(TOKEN_PEEK(1).isType(LPARENT) && !has_return[curToken->getTokenStr()]){
			if (!callFuncNoReturn()) {
				// TODO: 错误处理
				cout << "语法分析<语句>: 无返回值函数调用语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}
		else
		{
			if (!assignStatement()) {
				// TODO: 错误处理
				cout << "语法分析<语句>: 赋值语句错误" << endl;
				return false;
			}
			node.addNode(subNode);
		}

		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 缺少分号" << endl;
			return false;
		}
	}
	else if (curToken->isType(SCANFTK)) {
		if (!scanfStatement()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 读语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 读语句缺少分号" << endl;
			return false;
		}
	}
	else if (curToken->isType(PRINTFTK)) {
		if (!printfStatement()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 写语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 写语句缺少分号" << endl;
			return false;
		}
	}
	else if (curToken->isType(SWITCHTK)) {
		if (!switchStatement()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 情况语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
	}
	else if (curToken->isType(RETURNTK)) {
		if (!returnStatement()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 返回语句错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 返回语句缺少分号" << endl;
			return false;
		}
	}
	else if (curToken->isType(LBRACE)) {
		checkToken(LBRACE, "<LBRACE>", &node);
		if (!statementList()) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 语句列错误" << endl;
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RBRACE, "<RBRACE>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 语句列缺少左括号" << endl;
			return false;
		}
	}
	else {
		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			// TODO: 错误处理
			cout << "语法分析<语句>: 空语句缺少分号" << endl;
			return false;
		}
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
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		// TODO: 错误处理
		return false;
	}
	if (curToken->isType(LBRACK)) {
		checkToken(LBRACK, "<LBRACK>", &node);
		if (!expression()) {
			// TODO: 错误处理
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RBRACK, "<RBRACK>", &node)) {
			// TODO: 错误处理
			return false;
		}
		if (curToken->isType(LBRACK)) {
			checkToken(LBRACK, "<LBRACK>", &node);
			if (!expression()) {
				// TODO: 错误处理
				return false;
			}
			node.addNode(subNode);
			if (!checkToken(RBRACK, "<RBRACK>", &node)) {
				// TODO: 错误处理
				return false;
			}
		}
	}
	if (!checkToken(ASSIGN, "<ASSIGN>", &node)) {
		// TODO: 错误处理
		return false;
	}
	if (!expression()) {
		// TODO: 错误处理
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
		// TODO: 错误处理
		return false;
	}
	if (!condition()) {
		// TODO: 错误处理
		return false;
	}
	node.addNode(subNode);
	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		// TODO: 错误处理
		return false;
	}
	if (!statement()) {
		// TODO: 错误处理
		return false;
	}
	node.addNode(subNode);
	if (curToken->isType(ELSETK)) {
		checkToken(ELSETK, "<ELSETK>", &node);
		if (!statement()) {
			// TODO: 错误处理
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
	if (!expression()) {
		// TODO: 错误处理
		return false;
	}
	node.addNode(subNode);

	if (curToken->isType(LSS) || curToken->isType(LEQ) || curToken->isType(GRE)
		|| curToken->isType(GEQ) || curToken->isType(EQL) || curToken->isType(NEQ)) {
		ADD_TOKENNODE("<关系运算符>", curToken, node);
		curToken = &TOKEN_GET;
	}
	else {
		// TODO: 错误处理
		return false;
	}

	if (!expression()) {
		// TODO: 错误处理
		return false;
	}
	node.addNode(subNode);

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
		if (!checkToken(RPARENT, "<RPARENT>", &node)) {
			return false;
		}
		if (!statement()) {
			return false;
		}
		node.addNode(subNode);
	}
	else {
		if (!checkToken(FORTK, "<FORTK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)
			|| !checkToken(IDENFR, "<IDENFR>", &node) || !checkToken(ASSIGN, "<ASSIGN>", &node)) {
			return false;
		}
		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(SEMICN, "<SEMICN>", &node)) {
			return false;
		}
		if (!condition()) {
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(SEMICN, "<SEMICN>", &node)|| !checkToken(IDENFR, "<IDENFR>", &node)
			|| !checkToken(ASSIGN, "<ASSIGN>", &node) || !checkToken(IDENFR, "<IDENFR>", &node)) {
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
		if (!checkToken(RPARENT, "<RPARENT>", &node)) {
			return false;
		}
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
	AST_node node("<情况语句>", true);
	if (!checkToken(SWITCHTK, "<SWITCHTK>", &node) || !checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!expression()) {
		return false;
	}
	node.addNode(subNode);
	if (!checkToken(RPARENT, "<RPARENT>", &node) || !checkToken(LBRACE, "<LBRACE>", &node)) {
		return false;
	}
	if (!caseList()) {
		return false;
	}
	node.addNode(subNode);
	if (!defaultStatement()) {
		return false;
	}
	node.addNode(subNode);
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
bool Parser::caseList() {
	AST_node node("<情况表>", true);
	if (!caseStatement()) {
		return false;
	}
	node.addNode(subNode);
	while (curToken->isType(CASETK)) {
		if (!caseStatement()) {
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
bool Parser::caseStatement() {
	AST_node node("<情况子语句>", true);
	if (!checkToken(CASETK, "<CASETK>", &node)) {
		return false;
	}
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
	AST_node node("<有返回值函数调用语句>", true);
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!valueParamList()) {
		return false;
	}
	node.addNode(subNode);
	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		return false;
	}
	subNode = node;
	return true;
}

/*
<无返回值函数调用语句> ::= <标识符>'('<值参数表>')'
<无返回值函数调用语句> ::=<标识符>[LPARENT]<值参数表>[RPARENT]

FIRST IDENFR
*/
bool Parser::callFuncNoReturn() {
	AST_node node("<无返回值函数调用语句>", true);
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		return false;
	}
	if (!checkToken(LPARENT, "<LPARENT>", &node)) {
		return false;
	}
	if (!valueParamList()) {
		return false;
	}
	node.addNode(subNode);
	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		return false;
	}
	subNode = node;
	return true;
}

/*
<值参数表>   ::= <表达式>{,<表达式>}｜<空>
<值参数表>   ::= <表达式> {[COMMA]<表达式>}|<空>

FIRST PLUS MINU IDENFR LPARENT PLUS MINU INTCON CHARCON IDENFR
*/
bool Parser::valueParamList() {
	AST_node node("<值参数表>", true);
	if (!curToken->isType(RPARENT)) {
		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
		while (curToken->isType(COMMA))
		{
			checkToken(COMMA, "<COMMA>", &node);
			if (!expression()) {
				return false;
			}
			node.addNode(subNode);
		}
	}
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
	if (!checkToken(IDENFR, "<标识符>", &node)) {
		return false;
	}
	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		return false;
	}
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
	if (!checkToken(RPARENT, "<RPARENT>", &node)) {
		return false;
	}
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
	if (curToken->isType(LPARENT)) {
		checkToken(LPARENT, "<LPARENT>", &node);
		if (!expression()) {
			return false;
		}
		node.addNode(subNode);
		if (!checkToken(RPARENT, "<RPARENT>", &node)) {
			return false;
		}
	}
	subNode = node;
	return true;
}