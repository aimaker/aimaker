#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;

int genid(void);

vector<string> split(const string&, const char, int numsplits=-1);
vector<string> split(const string &s, const char* delims, int numsplits);

string trim(const string&);

bool is_numeric(const string&);

void throw_error(int lineNumber, const char *message);
template <typename... Args>
void throw_error(int lineNumber, const char *message, Args... parameters){
	char *error;
	asprintf(&error, message, parameters...);
	throw_error(lineNumber, error);
}

void to_lower(string&);

template <typename T>
ostream& operator<<(ostream &os, const vector<T> &vec) {
	for (const T &x : vec) {
		os << x << endl;
	}
	return os;
}

int mod(int a, int b);
