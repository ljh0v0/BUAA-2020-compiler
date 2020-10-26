#include <iostream>
#include <string>
#include <fstream>

#include "Lexer.h"
#include "Parser.h"

using namespace std;

ifstream infile;
ofstream outfile;

bool debug;

void open_file() {
	infile.open("testfile.txt");
	if (!infile.is_open()) {
		cout << "the testfile.txt is not exit!" << endl;
	}
	outfile.open("output.txt");
}

int main() {
	Lexer lexer;
	Parser parser;
	open_file();
	lexer.lexicalAnalyse();
	parser.initial();
	parser.procedure();
	parser.getRoot().print();
	return 0;
}