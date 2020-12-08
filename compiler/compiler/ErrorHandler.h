#pragma once
#include "type.h"

#include <vector>

using namespace std;

class ErrorHandler
{
public:
	void printError(char error, int lineNumber);
	void skip(vector<int> symbols);
};

