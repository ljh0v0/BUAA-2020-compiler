#include "SymbolTable.h"

using namespace std;

STE symbolTable;

SymbolTableManager::SymbolTableManager() {
	symbolTable.name = "procedure";
	symbolTable.identType = IdentType::PROCEDURE;
	position = &symbolTable;
	curSTE = NULL;
}

void SymbolTableManager::returnRoot() {
	position = &symbolTable;
}

bool SymbolTableManager::goInto(string name) {
	map<string, STE>::iterator it = position->children.find(name);
	if (it != position->children.end()) {
		position = &(it->second);
		return true;
	}
	return false;
}

void SymbolTableManager::goOut() {
	if (position->father != NULL) {
		position = position->father;
	}
}

void SymbolTableManager::gocurSTE() {
	position = curSTE;
}

bool SymbolTableManager::find(string name) {
	STE* ps = position->children.size() == 0 ? position->father : position;
	while (ps != NULL) {
		map<string, STE>::iterator it = ps->children.find(name);
		if (it != ps->children.end()) {
			curSTE = &(it->second);
			return true;
		}
		ps = ps->father;
	}
	curSTE = NULL;
	return false;
}

bool SymbolTableManager::insert(STE ste) {
	map<string, STE>::iterator it = position->children.find(ste.name);
	if (it == position->children.end()) {
		ste.father = position;
		position->children.insert(pair<string, STE>(ste.name, ste));
		return true;
	}
	return false;
}

bool SymbolTableManager::insert(STE ste, STE* father) {
	map<string, STE>::iterator it = father->children.find(ste.name);
	if (it == father->children.end()) {
		ste.father = father;
		father->children.insert(pair<string, STE>(ste.name, ste));
		return true;
	}
	return false;
}

ValueType SymbolTableManager::getPosType() {
	return position->valueType;
}