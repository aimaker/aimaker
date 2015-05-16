#pragma once

#include <vector>
#include <utility>
#include <unordered_map>
#include <string>

using namespace std;

namespace Parser {

	enum Instruction {
		INSTR_MOVE,
		INSTR_ROT,
		INSTR_NOP,
		INSTR_IFGOTO,
		INSTR_STO,
		INSTR_TRANS,
		INSTR_PAGE,

		INSTR_INVALID=255
	};

	struct Argument {
		enum Type {
			ARGT_NUMBER,
			ARGT_VARIABLE
		};
		Type type;
		int intVal;
		string stringVal;
	};

	struct Statement {
		Instruction instr;
		vector<Argument> args;
	};

	typedef vector<Statement> Codepage;
	typedef pair<int, int> Position;

	struct Program {
		int id;
		string name;
		string author;
		vector<Codepage> pages;
		unordered_map<string, Position> labels;
	};

	Instruction convertInstruction(string word);

	Program parse(const char *const,const vector<string>&);

}
