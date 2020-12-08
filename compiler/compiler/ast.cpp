#include "ast.h"

using namespace std;

extern ofstream outfile;

AST_node::AST_node() {
	this->token = nullptr;
	this->nodelist.clear();
}

AST_node::AST_node(string name, bool isSyntax) {
	this->name = name;
	this->isSyntax = isSyntax;
	this->token = nullptr;
	this->nodelist.clear();
}

void AST_node::addToken(Token* token) {
	this->token = token;
}

void AST_node::addNode(AST_node node) {
	this->nodelist.emplace_back(node);
}

void AST_node::print() {
	vector<AST_node>::iterator iter;
	for (iter = nodelist.begin(); iter != nodelist.end(); iter++) {
		(*iter).print();
	}
	if (this->token != nullptr) {
		this->token->printToken();
	}
	if (this->isSyntax) {
		outfile << this->name << endl;
	}
}

void AST_node::setNum(int num) {
	this->number = num;
}

int AST_node::getNum() {
	return number;
}