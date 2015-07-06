#define EXPRESSION_DEBUG 2 // 0: off, 1: errors, 2: verbose
#include "expression.h"
#include "../util.h"
#include <deque>
#include <functional>
#include <cstdint>
#include <cstring>
#if defined(EXPRESSION_DEBUG) || defined(EXPRESSION_DEBUG_MAIN)
#include <iostream>
#endif

using namespace std;

namespace Parser {

	Variable::Variable(void) {}
	Variable::Variable(int32_t i) {
		type = VAR_INT;
		intVal = i;
	}

	int Variable::getSize(void) const {
		switch (type) {
		case VAR_ARR: {
			int sum = 0;
			for (const Variable &var : arrVal) {
				sum += var.getSize();
			}
			return sum;
		}

		case VAR_INT: return 4;
		}
	}

	string Variable::toString(void) const {
		switch (type) {
		case VAR_ARR: {
			string s = "";
			for (const Variable &v : arrVal) {
				s += v.toString();
			}
			return s;
		}

		case VAR_INT: return to_string(intVal);
		}
	}

#if EXPRESSION_DEBUG==2
	ostream &operator<<(ostream &os, const ExprNode &en) {
		os << &en << " (" << operatorToString(en.type) << ',';
		if (en.hasval == 1)os << en.strval;
		else if (en.hasval == 2)os << en.intval;
		os << "), LR=(" << en.left << ',' << en.right << ')';
		return os;
	}
#endif

	ExprNode::ExprNode(void)
		: type(EN_INVALID), left(NULL), right(NULL), strval(), intval(0), hasval(0) {}
	ExprNode::ExprNode(const ExprNode &n)
		: type(n.type), strval(n.strval), intval(n.intval), hasval(n.hasval) {
		if (n.left) left = new ExprNode(*n.left); else left = NULL;
		if (n.right) right = new ExprNode(*n.right); else right = NULL;
		//cerr<<"\x1B[33mCopied\x1B[0m ["<<n<<"] \x1B[33mto\x1B[0m ["<<*this<<']'<<endl;
	}
	ExprNode &ExprNode::operator=(const ExprNode &n) {
		type = n.type;
		if (left) left = new ExprNode(*n.left); else left = NULL;
		if (right) right = new ExprNode(*n.right); else right = NULL;
		strval = n.strval; intval = n.intval; hasval = n.hasval;
		//cerr<<"\x1B[33mCopied\x1B[0m ["<<n<<"] \x1B[33mto\x1B[0m ["<<*this<<']'<<endl;
		return *this;
	}
	ExprNode::ExprNode(ExprNode &&n)
		: type(n.type), left(n.left), right(n.right), strval(move(n.strval)), intval(n.intval), hasval(n.hasval) {
		//cerr<<"\x1B[33mMoved\x1B[0m ["<<n<<"] \x1B[33mto\x1B[0m ["<<*this<<']'<<endl;
		n.type = EN_INVALID;
		n.setNullChildren();
	}
	ExprNode &ExprNode::operator=(ExprNode &&n) {
		type = n.type; left = n.left; right = n.right; strval = move(n.strval); intval = n.intval; hasval = n.hasval;
		//cerr<<"\x1B[33mMoved\x1B[0m ["<<n<<"] \x1B[33mto\x1B[0m ["<<*this<<']'<<endl;
		n.type = EN_INVALID;
		n.setNullChildren();
		return *this;
	}
	ExprNode::ExprNode(ExprNodeType _t)
		: type(_t), left(NULL), right(NULL), strval(), intval(0), hasval(0) {}
	ExprNode::ExprNode(ExprNodeType _t, const string &_s)
		: type(_t), left(NULL), right(NULL), strval(_s), intval(0), hasval(1) {}
	ExprNode::ExprNode(ExprNodeType _t, const int _i)
		: type(_t), left(NULL), right(NULL), strval(), intval(_i), hasval(2) {}
	ExprNode::ExprNode(ExprNodeType _t, ExprNode *_l, ExprNode *_r, const string &_s)
		: type(_t), left(_l), right(_r), strval(_s), intval(0), hasval(1) {}
	ExprNode::ExprNode(ExprNodeType _t, ExprNode *_l, ExprNode *_r, const int _i)
		: type(_t), left(_l), right(_r), strval(), intval(_i), hasval(2) {}

	ExprNode::~ExprNode(void) {
#if EXPRESSION_DEBUG==2
		cerr<<"\x1B[33mDestructing\x1B[0m exprnode "<<*this<<endl;
#endif
		if (left != NULL) delete left;
		if (right != NULL) delete right;
	}

	void ExprNode::setNullChildren(void) {
		left = right = NULL;
	}


	bool leftAssoc(ExprNodeType type) {
		switch (type) {
		case EN_NOT:
		case EN_NEGATE:
			return false;
		default:
			return true;
		}
	}

	int precedence(ExprNodeType type) { //pretty much c++ precedence
		switch (type) {
		case EN_NOT:
		case EN_NEGATE:
			return 3;
		case EN_MULTIPLY:
		case EN_DIVIDE:
			return 5;
		case EN_ADD:
		case EN_SUBTRACT:
			return 6;
		case EN_SHIFTR:
		case EN_SHIFTL:
			return 7;
		case EN_GREATER:
		case EN_GREATEREQUAL:
		case EN_LESS:
		case EN_LESSEQUAL:
			return 8;
		case EN_EQUALS:
		case EN_NOTEQUAL:
			return 9;
		case EN_BITAND:
			return 10;
		case EN_BITXOR:
			return 11;
		case EN_BITOR:
			return 12;
		case EN_AND:
			return 13;
		case EN_OR:
			return 14;

		case EN_PAREN1:
		case EN_PAREN2:
			return 999999;

		case EN_NUMBER:
		case EN_VARIABLE:
		case EN_LABEL:
		case EN_SUBTRACT_OR_NEGATE_CONFLICT:
		case EN_INVALID:
		default:
			return -1; //eh?
		}
	}

	int arity(ExprNodeType type) {
		switch (type) {
		case EN_NOT:
		case EN_NEGATE:
			return 1;

		case EN_PAREN1:
		case EN_PAREN2:
		case EN_NUMBER:
		case EN_VARIABLE:
		case EN_LABEL:
		case EN_SUBTRACT_OR_NEGATE_CONFLICT:
		case EN_INVALID:
			return 0; //eh?

		default:
			return 2;
		}
	}

	ExprNodeType interpretOperator(const string &s) {
		if (s == "+") return EN_ADD;
		//else if(s=="-") return EN_SUBTRACT;
		else if (s == "*") return EN_MULTIPLY;
		else if (s == "/") return EN_DIVIDE;
		else if (s == "&") return EN_BITAND;
		else if (s == "|") return EN_BITOR;
		else if (s == "^") return EN_BITXOR;
		else if (s == "==") return EN_EQUALS;
		else if (s == "!=") return EN_NOTEQUAL;
		else if (s == ">") return EN_GREATER;
		else if (s == ">=") return EN_GREATEREQUAL;
		else if (s == "<") return EN_LESS;
		else if (s == "<=") return EN_LESSEQUAL;
		else if (s == "&&") return EN_AND;
		else if (s == "||") return EN_OR;
		else if (s == ">>") return EN_SHIFTR;
		else if (s == "<<") return EN_SHIFTL;
		else if (s == "!") return EN_NOT;
		//else if(s=="-") return EN_NEGATE;
		else if (s == "(") return EN_PAREN1;
		else if (s == ")") return EN_PAREN2;
		else if (s == "-") return EN_SUBTRACT_OR_NEGATE_CONFLICT;

		return EN_INVALID;
	}

	const char *operatorToString(ExprNodeType type) {
		switch (type) {
		case EN_ADD: return "EN_ADD";
		case EN_SUBTRACT: return "EN_SUBTRACT";
		case EN_MULTIPLY: return "EN_MULTIPLY";
		case EN_DIVIDE: return "EN_DIVIDE";
		case EN_BITAND: return "EN_BITAND";
		case EN_BITOR: return "EN_BITOR";
		case EN_BITXOR: return "EN_BITXOR";
		case EN_EQUALS: return "EN_EQUALS";
		case EN_NOTEQUAL: return "EN_NOTEQUAL";
		case EN_GREATER: return "EN_GREATER";
		case EN_GREATEREQUAL: return "EN_GREATEREQUAL";
		case EN_LESS: return "EN_LESS";
		case EN_LESSEQUAL: return "EN_LESSEQUAL";
		case EN_AND: return "EN_AND";
		case EN_OR: return "EN_OR";
		case EN_SHIFTR: return "EN_SHIFTR";
		case EN_SHIFTL: return "EN_SHIFTL";
		case EN_NOT: return "EN_NOT";
		case EN_NEGATE: return "EN_NEGATE";
		case EN_PAREN1: return "EN_PAREN1";
		case EN_PAREN2: return "EN_PAREN2";
		case EN_NUMBER: return "EN_NUMBER";
		case EN_VARIABLE: return "EN_VARIABLE";
		case EN_LABEL: return "EN_LABEL";
		case EN_SUBTRACT_OR_NEGATE_CONFLICT: return "EN_SUBTRACT_OR_NEGATE_CONFLICT";
		case EN_INVALID: return "EN_INVALID";
		default: return "?";
		}
	}

	vector<ExprToken> tokeniseExpression(const string &s) {
		const char *const whitespace =
			"\x01\x02\x03\x04\x05\x06\a\b\t\x0a\v\f\x0d\x0e\x0f\n\x11\x12\r\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ";
		const char *const wordchars =
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
		const char *const numberchars = "0123456789";
		int start, end, cursor = 0, i;
		vector<ExprToken> tkns;

		while (true) {
			start = s.find_first_not_of(whitespace, cursor);
			if (start == (int)string::npos) {
				return tkns;
			}

			ExprToken token;
			if (strchr(whitespace, s[start]) != NULL) {
				cursor = s.find_first_not_of(whitespace, start + 1);
				continue;
			} else if (s[start] == '@') {
				token.type = ETT_LABEL;
				end = s.find_first_not_of(wordchars, start + 1);
			} else if (strchr(wordchars, s[start]) != NULL) {
				token.type = ETT_WORD;
				end = s.find_first_not_of(wordchars, start + 1);
			} else if (strchr(numberchars, s[start]) != NULL) {
				token.type = ETT_NUMBER;
				end = s.find_first_not_of(numberchars, start + 1);
			} else {
				token.type = ETT_SYMBOL;

				for (i = start + 1; i < (int)s.size() && i - start <= 2; i++) {
					if (strchr(wordchars, s[i]) != NULL ||
							strchr(numberchars, s[i]) != NULL ||
							strchr(whitespace, s[i]) != NULL)break;
				}

				for (; i > start + 1; i--)
					if (interpretOperator(s.substr(start, i - start)) != EN_INVALID) break;

				end = i;
			}

			token.val = s.substr(start, end - start);

			if (token.type == ETT_SYMBOL && interpretOperator(token.val) == EN_INVALID) {
				char *buf;
				asprintf(&buf, "Invalid operator '%s'", token.val.c_str());
				throw buf;
			}

			tkns.push_back(token);
			if (end == (int)s.size())break;
			cursor = end;
		}

		return tkns;
	}

#if EXPRESSION_DEBUG==2
	void printstack(const vector<ExprNode> &st) {
		const ExprNode *node;

		for (int i = 0; i < (int)st.size(); i++) {
			node = &st[i];
			cerr << i << ": " << operatorToString(node->type) << ' ';
			if (node->hasval == 1)cerr << node->strval;
			else if (node->hasval == 2)cerr << node->intval;
			cerr << endl;
		}
	}
	void printdeque(const deque<ExprNode> &deq) {
		const ExprNode *node;

		for (int i = 0; i < (int)deq.size(); i++) {
			node = &deq[i];
			cerr << i << ": " << operatorToString(node->type) << ' ';
			if (node->hasval == 1)cerr << node->strval;
			else if (node->hasval == 2)cerr << node->intval;
			cerr << endl;
		}
	}
#endif
	void parseExpression(/*out*/ExprNode *root, const vector<ExprToken> &tkns) {
		// This implements Dijkstra's Shunting Yard algorithm, skipping functions.
		deque<ExprNode> nodedeq;
		vector<ExprNode> opstack;
		ExprNodeType type;
		int i, prec;
		bool lastWasOperator = true, foundLeftParen, isleftassoc, weGotALabel = false;

		for (i = 0; i < (int)tkns.size(); i++) {
			const ExprToken &token = tkns[i];

			switch (token.type) {
			case ETT_NUMBER:
				nodedeq.emplace_back(EN_NUMBER, stoi(token.val));
				lastWasOperator = false;
				break;

			case ETT_WORD:
				nodedeq.emplace_back(EN_VARIABLE, token.val);
				lastWasOperator = false;
				break;

			case ETT_LABEL:
				if (i > 0 || weGotALabel) {
					char *buf;
					asprintf(&buf, "Invalid label '@%s' in expression", token.val.c_str());
					throw buf;
				}
				weGotALabel = true;
				nodedeq.emplace_back(EN_LABEL, token.val);
				lastWasOperator = false;
				break;

			case ETT_SYMBOL:
				type = interpretOperator(token.val);
				if (type == EN_INVALID) {
					char *buf;
					asprintf(&buf, "Invalid operator '%s'", token.val.c_str());
					throw buf;
				}

				if (type == EN_SUBTRACT_OR_NEGATE_CONFLICT) {
					if (lastWasOperator)type = EN_NEGATE;
					else type = EN_SUBTRACT;
				}

				if (type == EN_PAREN1) {
					opstack.emplace_back(type);
					lastWasOperator = true;
				} else if (type == EN_PAREN2) {
					foundLeftParen = false;
					while (opstack.size()) {
						const ExprNode opnode = opstack[opstack.size() - 1];
						opstack.pop_back();
						if (opnode.type == EN_PAREN1) {
							foundLeftParen = true;
							break;
						}
						nodedeq.push_back(opnode);
					}
					if (!foundLeftParen) {
						char *buf;
						asprintf(&buf, "Closing parenthesis without matching open '('");
						throw buf;
					}
					lastWasOperator = false;
				} else {
					isleftassoc = leftAssoc(type);
					prec = precedence(type);
					//cerr<<"prec="<<prec<<" type="<<operatorToString(type)<<endl;
					while (opstack.size()) {
						const ExprNode opnode = opstack[opstack.size() - 1];
						//below condition inequality operators are reversed because it BREAKS if true, AND because precedence numbers are reversed from the source wikipedia article. Watch your steps.
						if (isleftassoc ?
								prec < precedence(opnode.type) :
								prec <= precedence(opnode.type))
							break;
						nodedeq.push_back(opnode);
						opstack.pop_back();
					}
					opstack.emplace_back(type);
					lastWasOperator = true;
				}
				break;
			}
		}

		while (opstack.size()) {
			nodedeq.push_back(opstack[opstack.size() - 1]);
			opstack.pop_back();
		}

#if EXPRESSION_DEBUG==2
		cerr << "Opstack:" << endl;
		printstack(opstack);
		cerr << "Nodedeq:" << endl;
		printdeque(nodedeq);
		cerr << "------------" << endl;
#endif
		if (nodedeq.size() == 0) {
#if EXPRESSION_DEBUG>0
			cerr << "\t\t--- nodedeq.size() == 0" << endl;
#endif
			root->type = EN_INVALID;
			return;
		}

		for (i = 0; i < (int)nodedeq.size(); i++) {
			//for(ExprNode &n : nodedeq)cerr<<operatorToString(n.type)<<' '; cerr<<endl;
			const ExprNode &node = nodedeq[i];
			if (node.type == EN_NUMBER || node.type == EN_VARIABLE || node.type == EN_LABEL)continue;
			const int ar = arity(node.type);
			if (ar == 0) {
				char *buf;
				asprintf(&buf, "Invalid operator in stack: %s", operatorToString(node.type));
				throw buf;
			}
			if (ar == 1) {
				if (i < 1) {
					char *buf;
					asprintf(&buf, "Not enough arguments for unary %s on stack", operatorToString(node.type));
					throw buf;
				}
				if (leftAssoc(node.type)) nodedeq[i].left = new ExprNode(move(nodedeq[i - 1]));
				else nodedeq[i].right = new ExprNode(move(nodedeq[i - 1]));
				//nodedeq[i - 1].setNullChildren();
				nodedeq.erase(nodedeq.begin() + (i - 1));
				i--;
			} else if (ar == 2) {
				if (i < 2) {
					char *buf;
					asprintf(&buf, "Not enough arguments for %s on stack", operatorToString(node.type));
					throw buf;
				}
				nodedeq[i].left = new ExprNode(move(nodedeq[i - 2]));
				//nodedeq[i - 2].setNullChildren();
				nodedeq[i].right = new ExprNode(move(nodedeq[i - 1]));
				//nodedeq[i - 1].setNullChildren();
				//cerr<<"\x1B[33mi="<<i<<"...\x1B[0m (="<<&nodedeq[i]<<';'<<nodedeq[i].left<<','<<nodedeq[i].right<<')'<<endl;
				//cerr<<"\x1B[33mErasing\x1B[0m "<<nodedeq[i-2]<<" \x1B[33mand\x1B[0m "<<nodedeq[i-1]<<" \x1B[33mleaving\x1B[0m "<<nodedeq[i]<<"...";
				//cerr<<" \x1B[33m(size="<<nodedeq.size()<<")\x1B[0m"<<endl;
				nodedeq.erase(nodedeq.begin() + (i - 2), nodedeq.begin() + i);
				//cerr<<"\x1B[33mdone.\x1B[0m"<<endl;
				i -= 2;
			}
		}
		if (nodedeq.size() != 1) {
			char *buf;
			asprintf(&buf, "Excess items on expression stack!");
			throw buf;
		}
		//cerr<<"\x1B[33mReturning now...\x1B[0m"<<endl;
		*root = nodedeq[0];
		nodedeq[0].setNullChildren();
	}


	function<int(int, int)> exprnode_functions[] = {
		[](int a, int b) { return a +  b; }, //EN_ADD
		[](int a, int b) { return a -  b; }, //EN_SUBTRACT
		[](int a, int b) { return a *  b; }, //EN_MULTIPLY
		[](int a, int b) { return a /  b; }, //EN_DIVIDE
		[](int a, int b) { return a &  b; }, //EN_BITAND
		[](int a, int b) { return a |  b; }, //EN_BITOR
		[](int a, int b) { return a ^  b; }, //EN_BITXOR
		[](int a, int b) { return a == b; }, //EN_EQUALS
		[](int a, int b) { return a != b; }, //EN_NOTEQUAL
		[](int a, int b) { return a >  b; }, //EN_GREATER
		[](int a, int b) { return a >= b; }, //EN_GREATEREQUAL
		[](int a, int b) { return a <  b; }, //EN_LESS
		[](int a, int b) { return a <= b; }, //EN_LESSEQUAL
		[](int a, int b) { return a && b; }, //EN_AND
		[](int a, int b) { return a || b; }, //EN_OR
		[](int a, int b) { return a >> b; }, //EN_SHIFTR
		[](int a, int b) { return a << b; }, //EN_SHIFTL
		[](int _, int b) { return !b;     }, //EN_NOT
		[](int _, int b) { return -b;     }, //EN_NEGATE
	};

	int evaluateExpression(
		const ExprNode &root,
		const int lineNumber,
		const unordered_map<string, Variable> &vars,
		const LabelMap &labels
	) {
		if (root.hasval == 1 && root.type == EN_VARIABLE) {
			unordered_map<string, Variable>::const_iterator varit = vars.find(root.strval);
			if (varit == vars.end()) {
				char *buf;

				cerr << "vars size: " << vars.size() << endl;
				for (const auto &x : vars) {
					cerr << x.first << ", " << x.second.toString() << endl;
				}

				asprintf(&buf, "Variable '%s' not found", root.strval.c_str());
				throw_error(lineNumber, buf);
			} else if (varit->second.type != Variable::VAR_INT) {
				throw_error(lineNumber, "Exprected an int variable.");
			} else {
				return varit->second.intVal;
			}
		} else if (root.hasval == 1 && root.type == EN_LABEL) {
			unordered_map<string, LabelInfo>::const_iterator labit = labels.find(root.strval);
			if (labit == labels.end()) {
				char *buf;
				asprintf(&buf, "Label '@%s' not found", root.strval.c_str());
				throw_error(lineNumber, buf);
			}
			return labit->second.intval;
		}

		if (root.hasval == 2) return root.intval;
		if (root.type > sizeof(exprnode_functions) / sizeof(exprnode_functions[0])) {
			char *buf;
			asprintf(&buf, "Non-normal operator %s in expression tree (internal error)", operatorToString(root.type));
			throw_error(lineNumber, buf);
		}

		if (root.type > EN_SHIFTL) {
			if (root.right == NULL) {
				char *buf;
				asprintf(&buf, "Unary-right operator %s doesn't have a right argument in tree (internal error)", operatorToString(root.type));
				throw_error(lineNumber, buf);
			}
		} else {
			if (root.left == NULL || root.right == NULL) {
				char *buf;
				asprintf(&buf, "Dyadic operator %s doesn't have two arguments in tree (internal error)", operatorToString(root.type));
				throw_error(lineNumber, buf);
			}
		}

		return exprnode_functions[root.type](
			root.left != NULL ? evaluateExpression(*root.left, lineNumber, vars, labels) : 0,
			root.right != NULL ? evaluateExpression(*root.right, lineNumber, vars, labels) : 0
		);
	}

};

#ifdef EXPRESSION_DEBUG_MAIN
#include <sstream>

int uniqid(void) {
	static int $ = 0;
	return $++;
}

string nodeToString(const Parser::ExprNode &node, const int thisid) {
	stringstream ss;
	ss << thisid << "\\n" << operatorToString(node.type);

	if (node.hasval == 1)ss << "\\n(" << node.strval << ')';
	else if (node.hasval == 2)ss << "\\n(" << node.intval << ')';

	return ss.str();
}

void printtree__node(const Parser::ExprNode &node, const int thisid) {
	int leftid = uniqid(), rightid = uniqid();

	if (node.left) {
		cout << "\t\"" << nodeToString(node, thisid) << "\"->\"" << nodeToString(*node.left, leftid) << '"' << endl;
		printtree__node(*node.left, leftid);
	}

	if (node.right) {
		cout << "\t\"" << nodeToString(node, thisid) << "\"->\"" << nodeToString(*node.right, rightid) << '"' << endl;
		printtree__node(*node.right, rightid);
	}
}
void printtree(const Parser::ExprNode &root) {
	cout << "digraph G{" << endl;
	printtree__node(root, uniqid());
	cout << "}" << endl;
}

int main(void) {
	try {
		Parser::ExprNode root;
		Parser::parseExpression(&root, Parser::tokeniseExpression("1+(2-3)==a&&b<=-c"));
		cerr << "Done parsing" << endl;
		printtree(root);
	} catch (char *exc) {
		cerr << "EXCEPTION: " << exc << endl;
	}
}
#endif
