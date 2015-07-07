#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int genid(void);

vector<string> split(const string&, const char, int numsplits=-1);
vector<string> split(const string &s, const char* delims, int numsplits);

string trim(const string&);

bool is_numeric(const string&);

void throw_error(int, const char*);

void to_lower(string&);

template <typename T>
void print_vector(const vector<T>&, ostream &os=cout);
