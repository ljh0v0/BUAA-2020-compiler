#include <iostream>
#include <string>
#include <fstream>

#include "Lexer.h"

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
	open_file();
	lexer.lexicalAnalyse();
	return 0;
}