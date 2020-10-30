#pragma once

#include "type.h"

using namespace std;

extern STE symbolTable;

class SymbolTableManager
{
private:
	STE* position;
public:
	STE* curSTE;
	SymbolTableManager();
	void returnRoot();
	bool goInto(string name);
	void goOut();
	void gocurSTE();
	bool find(string name);
	bool insert(STE ste);
	bool insert(STE ste, STE* father);
	ValueType getPosType();
};

