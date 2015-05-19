#include <unordered_map>
#include <cstring>
#include "expression.h"
#ifdef EXPRESSION_DEBUG_MAIN
#include <iostream>
#endif

using namespace std;

namespace Parser {

	ExprNode::ExprNode(void)
		: type(EN_INVALID), left(NULL), right(NULL), strval(), intval(0), hasval(0) {}
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
		if (left)delete left;
		if (right)delete right;
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
		if (s == "+")return EN_ADD;
		//if(s=="-")return EN_SUBTRACT;
		if (s == "*")return EN_MULTIPLY;
		if (s == "/")return EN_DIVIDE;
		if (s == "&")return EN_BITAND;
		if (s == "|")return EN_BITOR;
		if (s == "^")return EN_BITXOR;
		if (s == "==")return EN_EQUALS;
		if (s == "!=")return EN_NOTEQUAL;
		if (s == ">")return EN_GREATER;
		if (s == ">=")return EN_GREATEREQUAL;
		if (s == "<")return EN_LESS;
		if (s == "<=")return EN_LESSEQUAL;
		if (s == "&&")return EN_AND;
		if (s == "||")return EN_OR;
		if (s == ">>")return EN_SHIFTR;
		if (s == "<<")return EN_SHIFTL;
		if (s == "!")return EN_NOT;
		//if(s=="-")return EN_NEGATE;
		if (s == "(")return EN_PAREN1;
		if (s == ")")return EN_PAREN2;
		if (s == "-")return EN_SUBTRACT_OR_NEGATE_CONFLICT;
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
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
		const char *const numberchars = "0123456789";
		int start, end, cursor = 0, i;
		vector<ExprToken> tkns;
		while (true) {
			start = s.find_first_not_of(whitespace, cursor);
			if (start == (int)string::npos)return tkns;
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
					if (interpretOperator(s.substr(start, i - start)) != EN_INVALID)break;
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

#ifdef EXPRESSION_DEBUG_MAIN
	void printstack(const vector<ExprNode> &stack) {
		const ExprNode *node;
		for (int i = 0; i < (int)stack.size(); i++) {
			node = &stack[i];
			cerr << i << ": " << operatorToString(node->type) << ' ';
			if (node->hasval == 1)cerr << node->strval;
			else if (node->hasval == 2)cerr << node->intval;
			cerr << endl;
		}
	}
#endif
	ExprNode parseExpression(const vector<ExprToken> &tkns) {
		//this implements Dijkstra's Shunting Yard algorithm, skipping functions.
		vector<ExprNode> nodestack, opstack;
		ExprNodeType type;
		int i;
		bool lastWasOperator = true, foundLeftParen, isleftassoc;
		int prec;
		bool weGotALabel = false;
		for (i = 0; i < (int)tkns.size(); i++) {
			const ExprToken token = tkns[i];
			switch (token.type) {
			case ETT_NUMBER:
				nodestack.emplace_back(EN_NUMBER, stoi(token.val));
				lastWasOperator = false;
				break;
			case ETT_WORD:
				nodestack.emplace_back(EN_VARIABLE, token.val);
				lastWasOperator = false;
				break;
			case ETT_LABEL:
				if (i > 0 || weGotALabel) {
					char *buf;
					asprintf(&buf, "Invalid label '@%s' in expression", token.val.c_str());
					throw buf;
				}
				weGotALabel = true;
				nodestack.emplace_back(EN_LABEL, token.val);
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
						nodestack.push_back(opnode);
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
						nodestack.push_back(opnode);
						opstack.pop_back();
					}
					opstack.emplace_back(type);
					lastWasOperator = true;
				}
				break;
			}
		}
		while (opstack.size()) {
			nodestack.push_back(opstack[opstack.size() - 1]);
			opstack.pop_back();
		}
#ifdef EXPRESSION_DEBUG_MAIN
		cerr << "Opstack:" << endl;
		printstack(opstack);
		cerr << "Nodestack:" << endl;
		printstack(nodestack);
		cerr << "------------" << endl;
#endif
		if (nodestack.size() == 0)return ExprNode(EN_INVALID);
		for (i = 0; i < (int)nodestack.size(); i++) {
			ExprNode node = nodestack[i];
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
				if (leftAssoc(node.type))node.left = new ExprNode(nodestack[i - 1]);
				else node.right = new ExprNode(nodestack[i - 1]);
				nodestack.erase(nodestack.begin() + (i - 1));
				i--;
			} else if (ar == 2) {
				if (i < 2) {
					char *buf;
					asprintf(&buf, "Not enough arguments for %s on stack", operatorToString(node.type));
					throw buf;
				}
				nodestack[i].left = new ExprNode(nodestack[i - 2]);
				nodestack[i].right = new ExprNode(nodestack[i - 1]);
				nodestack.erase(nodestack.begin() + (i - 2), nodestack.begin() + i);
				i -= 2;
			}
		}
		if (nodestack.size() != 1) {
			char *buf;
			asprintf(&buf, "Excess items on expression stack!");
			throw buf;
		}
		return nodestack[0];
	}

	int evaluateExpression(const ExprNode &root, const unordered_map<string, int> &vars) {
		if (root.hasval == 1) {
			unordered_map<string, int>::const_iterator varit = vars.find(root.strval);
			if (varit == vars.end()) {
				char *buf;
				asprintf(&buf, "Variable '%s' not found", root.strval.c_str());
				throw buf;
			}
			return varit->second;
		}
		if (root.hasval == 2)return root.intval;
		switch (root.type) {

#define EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(en_,op) \
	case en_: \
		if(root.left==NULL||root.right==NULL){ \
			char *buf; \
			asprintf(&buf,"Dyadic operator " #op " doesn't have two arguments in tree (internal error)"); \
			throw buf; \
		} \
		return evaluateExpression(*root.left,vars) op evaluateExpression(*root.right,vars);

			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_ADD, +)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_SUBTRACT, -)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_MULTIPLY, *)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_DIVIDE, / )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_BITAND, &)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_BITOR, | )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_BITXOR, ^)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_EQUALS, == )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_NOTEQUAL, != )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_GREATER, > )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_GREATEREQUAL, >= )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_LESS, < )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_LESSEQUAL, <= )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_AND, &&)
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_OR, || )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_SHIFTR, >> )
			EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION(EN_SHIFTL, << )

#undef EVALUATEEXPRESSION_DYADIC_OPERATOR_CASE_REPETITION

#define EVALUATEEXPRESSION_UNARY_RIGHT_OPERATOR_CASE_REPETITION(en_,op) \
	case en_: \
		if(root.right==NULL){ \
			char *buf; \
			asprintf(&buf,"Unary-right operator " #op " doesn't have a right argument in tree (internal error)"); \
			throw buf; \
		} \
		return op evaluateExpression(*root.right,vars);

			EVALUATEEXPRESSION_UNARY_RIGHT_OPERATOR_CASE_REPETITION(EN_NOT, !)
			EVALUATEEXPRESSION_UNARY_RIGHT_OPERATOR_CASE_REPETITION(EN_NEGATE, -)

#undef EVALUATEEXPRESSION_UNARY_RIGHT_OPERATOR_CASE_REPETITION

		case EN_PAREN1:
		case EN_PAREN2:
		case EN_NUMBER:
		case EN_VARIABLE:
		case EN_LABEL:
		case EN_SUBTRACT_OR_NEGATE_CONFLICT:
		case EN_INVALID:
		default: {
			char *buf;
			asprintf(&buf, "Non-normal operator %s in expression tree (internal error)", operatorToString(root.type));
			throw buf;
		}

		}
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
		Parser::ExprNode root = Parser::parseExpression(Parser::tokeniseExpression("(1+2)-3==a&&b<=-c"));
		printtree(root);
	} catch (char *exc) {
		cerr << "EXCEPTION: " << exc << endl;
	}
}
#endif
