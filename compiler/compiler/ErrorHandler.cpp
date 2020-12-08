#include "ErrorHandler.h"
#include "Parser.h"
#include <fstream>

using namespace std;

extern ofstream errfile;

void ErrorHandler::printError(char error, int lineNumber) {
	errfile << lineNumber << " " << error << endl;
}

void ErrorHandler::skip(vector<int> symbols) {
	vector<int>::iterator iter = symbols.begin();
	while (iter != symbols.end()) {
		if (*iter == curToken->getType())
			break;
		iter++;
	}
	while (iter == symbols.end()) {
		curToken = &TOKEN_GET;
		while (iter != symbols.end()) {
			if (*iter == curToken->getType())
				break;
			iter++;
		}
	}
}