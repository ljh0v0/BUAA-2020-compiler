#include "function.h"

#include <string>
#include <sstream>
#include <iostream>

using namespace std;

static int tempVarId = 0;
static int labelId = 0;
static int stringId = 0;

string int2string(int t) {
	stringstream ss;
	ss << t;
	return ss.str();
}

int string2int(string s) {
	stringstream ss;
	ss << s;
	int t;
	ss >> t;
	return t;
}

string getTempVar() {
	return "#t" + int2string(tempVarId++);
}

string getLabel() {
	return "Label" + int2string(labelId++);
}

string getStingLabel() {
	return "string_" + int2string(stringId++);
}
