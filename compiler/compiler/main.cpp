#include <iostream>
#include <string>
#include <fstream>

#include "Lexer.h"
#include "Parser.h"
#include "midCode.h"
#include "mipsCode.h"

using namespace std;

ifstream infile;
ofstream outfile;
ofstream errfile;
ofstream midcodefile;
ofstream mipsfile;

bool debug = false;

void open_file() {
	infile.open("testfile.txt");
	if (!infile.is_open()) {
		cout << "the testfile.txt is not exit!" << endl;
	}
	outfile.open("output.txt");
	errfile.open("error.txt");
	midcodefile.open("midcodefile.txt");
	mipsfile.open("mips.txt");
}

int main() {
	Lexer lexer;
	Parser parser;
	open_file();
	lexer.lexicalAnalyse();
	parser.initial();
	parser.procedure();
	outputMidCode();
	geneMipsCode();
	//parser.getRoot().print();
	return 0;
}