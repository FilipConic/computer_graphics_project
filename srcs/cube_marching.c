#include "evoco.h"
#include <cube_marching.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <math.h>

#define CYLIBX_ALLOC
#include <cylibx.h>

StringSlice slice_from_lit_n(size_t n, char* str) {
	if (n <= STRING_SLICE_CONTAIN) {
		StringSlice slice = { .len = n };
		memcpy(slice.in.buffer, str, n * sizeof(char));
		return slice;
	} else {
		return (StringSlice){ .out.ptr = str, .out.len = n };
	}
}
int slice_eq(const void* a, const void* b) {
	const StringSlice* s1 = a;
	const StringSlice* s2 = b;
	if (s1->len != s2->len) { return 0; }
	return strncmp(slice_buffer_by_type(s1), slice_buffer_by_type(s2), s1->len) == 0;
}
size_t slice_hash(const void* v) {
	const StringSlice* s = v;
    size_t hash = 5381;
	char* str = slice_buffer_by_type(s);
	for (size_t i = 0; i < s->len; ++i) { 
        hash = ((hash << 5) + hash) + str[i];
	}
    return hash;
}

typedef enum NodeType {
	NODE_ERROR = 0,
	NODE_UNOP,
	NODE_BINOP,
	NODE_TERNARY,
	NODE_FUNC,

	NODE_NUMBER,
	NODE_VAR,
} NodeType;
typedef enum UnopType {
	UNOP_PAREN,
	UNOP_ABS,
	UNOP_NEG = '-',
	UNOP_NOT = 'n',
} UnopType;
typedef enum BinopType {
	BINOP_SUM = '+',
	BINOP_SUB = '-',
	BINOP_MULT = '*',
	BINOP_DIV = '/',
	BINOP_POW = '^',
	BINOP_LESS = '<',
	BINOP_GREATER = '>',
	BINOP_AND = 'a',
	BINOP_OR = 'o',
} BinopType;
typedef enum FuncType {
	FUNC_ERROR,
	FUNC_SQRT,
	FUNC_SIN,
	FUNC_COS,
	FUNC_TAN,
	FUNC_LOG,
	FUNC_LOG10,
	FUNC_LN,
	FUNC_ACOS,
	FUNC_ASIN,
	FUNC_ATAN,
	FUNC_COUNT,
} FuncType;
typedef struct Node Node;
struct Node {
	NodeType type;
	union {
		struct {
			Node* eq;
			UnopType type;
		} unop;
		struct {
			Node* left;
			Node* right;
			BinopType type;
		} binop;
		struct {
			Node* cond;
			Node* first;
			Node* second;
		} ternary;
		struct {
			Node* eq;
			FuncType type;
		} func;
		double num;
		StringSlice var;
	} as;
};

static Node* node_num(EvoPool* pool, double x) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_NUMBER;
	node->as.num = x;
	return node;
}
static Node* node_var(EvoPool* pool, char* id, size_t id_len) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_VAR;
	node->as.var = slice_from_lit_n(id_len, id);
	return node;
}
static Node* node_binop(EvoPool* pool, BinopType type, Node* left, Node* right) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_BINOP;
	node->as.binop.type = type;
	node->as.binop.left = left;
	node->as.binop.right = right;	
	return node;
}
static Node* node_ternary(EvoPool* pool, Node* cond, Node* first, Node* second) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_TERNARY;
	node->as.ternary.cond = cond;
	node->as.ternary.first = first;
	node->as.ternary.second = second;
	return node;
}
#define node_sum(pool, left, right) node_binop(pool, BINOP_SUM, left, right)
#define node_sub(pool, left, right) node_binop(pool, BINOP_SUB, left, right)
#define node_mult(pool, left, right) node_binop(pool, BINOP_MULT, left, right)
#define node_div(pool, left, right) node_binop(pool, BINOP_DIV, left, right)
#define node_pow(pool, left, right) node_binop(pool, BINOP_POW, left, right)
#define node_less(pool, left, right) node_binop(pool, BINOP_LESS, left, right)
#define node_greater(pool, left, right) node_binop(pool, BINOP_GREATER, left, right)
#define node_and(pool, left, right) node_binop(pool, BINOP_AND, left, right)
#define node_or(pool, left, right) node_binop(pool, BINOP_OR, left, right)
static Node* node_unop(EvoPool* pool, UnopType type, Node* eq) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_UNOP;
	node->as.unop.type = type;
	node->as.unop.eq = eq;
	return node;
}
#define node_paren(pool, eq) node_unop(pool, UNOP_PAREN, eq)
#define node_abs(pool, eq) node_unop(pool, UNOP_ABS, eq)
#define node_neg(pool, eq) node_unop(pool, UNOP_NEG, eq)
#define node_not(pool, eq) node_unop(pool, UNOP_NOT, eq)
static Node* node_func(EvoPool* pool, FuncType type, Node* eq) {
	Node* node = evo_pool_malloc(pool, 0);
	node->type = NODE_FUNC;
	node->as.func.type = type;
	node->as.func.eq = eq;
	return node;
}
#define node_sqrt(pool, eq) node_func(pool, FUNC_SQRT, eq)
#define node_sin(pool, eq) node_func(pool, FUNC_SIN, eq)
#define node_cos(pool, eq) node_func(pool, FUNC_COS, eq)
#define node_tan(pool, eq) node_func(pool, FUNC_TAN, eq)
#define node_asin(pool, eq) node_func(pool, FUNC_ASIN, eq)
#define node_acos(pool, eq) node_func(pool, FUNC_ASIN, eq)
#define node_atan(pool, eq) node_func(pool, FUNC_ATAN, eq)
#define node_log(pool, eq) node_func(pool, FUNC_LOG, eq)
#define node_log10(pool, eq) node_func(pool, FUNC_LOG10, eq)
#define node_ln(pool, eq) node_func(pool, FUNC_LN, eq)

static void node_print(FILE*, Node*);
static void node_ternary_print(FILE* file, Node* node) {
	assert(node->type == NODE_TERNARY);
	fprintf(file, "( (");
	node_print(file, node->as.ternary.cond);
	fprintf(file, ") ? (");
	node_print(file, node->as.ternary.first);
	fprintf(file, ") : (");
	node_print(file, node->as.ternary.second);
	fprintf(file, ") )");
}
static void node_binop_print(FILE* file, Node* node) {
	assert(node->type == NODE_BINOP);
	if (node->as.binop.type == BINOP_SUM) {
		fprintf(file, "("); 
		node_print(file, node->as.binop.left);
		fprintf(file, " + ");	
		node_print(file, node->as.binop.right);
		fprintf(file, ")"); 
	} else if (node->as.binop.type == BINOP_SUB) {
		fprintf(file, "("); 
		node_print(file, node->as.binop.left);
		fprintf(file, " - ");	
		node_print(file, node->as.binop.right);
		fprintf(file, ")"); 
	} else if (node->as.binop.type == BINOP_MULT) {
		fprintf(file, "("); 
		node_print(file, node->as.binop.left);
		fprintf(file, " * ");	
		node_print(file, node->as.binop.right);
		fprintf(file, ")"); 
	} else if (node->as.binop.type == BINOP_DIV) {
		fprintf(file, "("); 
		node_print(file, node->as.binop.left);
		fprintf(file, " / ");	
		node_print(file, node->as.binop.right);
		fprintf(file, ")"); 
	} else if (node->as.binop.type == BINOP_POW) {
		fprintf(file, "pow("); 
		node_print(file, node->as.binop.left);
		fprintf(file, ", ");	
		node_print(file, node->as.binop.right);
		fprintf(file, ")"); 
	} else if (node->as.binop.type == BINOP_AND) {
		fprintf(file, "((");
		node_print(file, node->as.binop.left);
		fprintf(file, ") && (");
		node_print(file, node->as.binop.right);
		fprintf(file, "))");
	} else if (node->as.binop.type == BINOP_OR) {
		fprintf(file, "((");
		node_print(file, node->as.binop.left);
		fprintf(file, ") || (");
		node_print(file, node->as.binop.right);
		fprintf(file, "))");
	} else if (node->as.binop.type == BINOP_LESS) {
		fprintf(file, "(");
		node_print(file, node->as.binop.left);
		fprintf(file, " < ");
		node_print(file, node->as.binop.right);
		fprintf(file, ")");
	} else if (node->as.binop.type == BINOP_GREATER) {
		fprintf(file, "(");
		node_print(file, node->as.binop.left);
		fprintf(file, " > ");
		node_print(file, node->as.binop.right);
		fprintf(file, ")");
	} 
}
static void node_unop_print(FILE* file, Node* node) {
	assert(node->type == NODE_UNOP);
	switch (node->as.unop.type) {
		case UNOP_NEG:		fprintf(file, "-"); node_print(file, node->as.unop.eq); break;
		case UNOP_PAREN:	fprintf(file, "("); node_print(file, node->as.unop.eq); fprintf(file, ")"); break;
		case UNOP_ABS:		fprintf(file, "fabs("); node_print(file, node->as.unop.eq); fprintf(file, ")"); break;
		case UNOP_NOT: 		fprintf(file, "!("); node_print(file, node->as.unop.eq); fprintf(file, ")");
	}
}
static void node_func_print(FILE* file, Node* node) {
	assert(node->type == NODE_FUNC);
	switch (node->as.func.type) {
		case FUNC_SQRT:  fprintf(file, "sqrt"); break;
		case FUNC_COS:  fprintf(file, "cos"); break;
		case FUNC_SIN:  fprintf(file, "sin"); break;
		case FUNC_TAN:  fprintf(file, "tan"); break;
		case FUNC_ACOS:  fprintf(file, "acos"); break;
		case FUNC_ASIN:  fprintf(file, "asin"); break;
		case FUNC_ATAN:  fprintf(file, "atan"); break;
		case FUNC_LOG:  fprintf(file, "log2"); break;
		case FUNC_LOG10:  fprintf(file, "log10"); break;
		case FUNC_LN:  fprintf(file, "log"); break;
		default: assert(0 && "UNREACHABLE");
	}
	fprintf(file, "(");
	node_print(file, node->as.func.eq);
	fprintf(file, ")");
}
static void node_print(FILE* file, Node* root) {
	if (!root) {
		fprintf(stderr, "ERROR:\tNULL node reached!\n");
		return;
	}
	switch(root->type) {
		case NODE_NUMBER:	fprintf(file, "%.2lf", root->as.num); break;
		case NODE_VAR:		fprintf(file, CYX_STR_FMT, SLICE_UNPACK(&root->as.var)); break;
		case NODE_TERNARY: 	node_ternary_print(file, root); break;
		case NODE_BINOP:	node_binop_print(file, root); break;
		case NODE_UNOP:		node_unop_print(file, root); break;
		case NODE_FUNC:		node_func_print(file, root); break;
		default: assert(0 && "UNREACHABLE");
	}
}

static int node_is_double(Node* node) {
	switch (node->type) {
		case NODE_BINOP: switch (node->as.binop.type) {
			case BINOP_SUB: case BINOP_DIV: case BINOP_SUM:
			case BINOP_POW: case BINOP_MULT: return node_is_double(node->as.binop.left) && node_is_double(node->as.binop.right);
			case BINOP_AND: case BINOP_OR:
			case BINOP_GREATER: case BINOP_LESS: return 0;
		} break;
		case NODE_TERNARY: return node_is_double(node->as.ternary.first) && node_is_double(node->as.ternary.second);
		case NODE_UNOP: switch (node->as.unop.type) {
			case UNOP_NEG: case UNOP_PAREN: case UNOP_ABS:
				return node_is_double(node->as.unop.eq);
			case UNOP_NOT:
				return 0;
		} break;
		case NODE_FUNC: return node_is_double(node->as.func.eq);
		case NODE_VAR:
		case NODE_NUMBER: return 1;
		default: assert(0 && "UNREACHABLE");
	}
	return 0;
}
static int node_is_bool(Node* node) {
	switch (node->type) {
		case NODE_BINOP: switch (node->as.binop.type) {
			case BINOP_SUB: case BINOP_DIV: case BINOP_SUM:
			case BINOP_POW: case BINOP_MULT: return 0;
			case BINOP_AND: case BINOP_OR: return node_is_bool(node->as.binop.left) && node_is_bool(node->as.binop.right);
			case BINOP_GREATER: case BINOP_LESS: return node_is_double(node->as.binop.left) && node_is_double(node->as.binop.right);
		} break;
		case NODE_TERNARY: return node_is_double(node->as.ternary.first) && node_is_double(node->as.ternary.second);
		case NODE_UNOP: switch (node->as.unop.type) {
			case UNOP_NEG: case UNOP_ABS:
				return 0;
			case UNOP_PAREN: case UNOP_NOT:
				return node_is_bool(node->as.unop.eq);
		} break;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_NUMBER: return 0;
		default: assert(0 && "UNREACHABLE");
	}
	return 0;
}
static int node_typecheck(Node* node) {
	switch (node->type) {
		case NODE_TERNARY:
			return node_is_bool(node->as.ternary.cond) && node_is_double(node->as.ternary.first) && node_is_double(node->as.ternary.second);
		case NODE_NUMBER:
		case NODE_VAR:
			return 1;
		case NODE_BINOP: switch (node->as.binop.type) {
			case BINOP_SUB: case BINOP_SUM:
			case BINOP_DIV: case BINOP_MULT:
			case BINOP_POW: case BINOP_LESS:
			case BINOP_GREATER:
				return node_is_double(node->as.binop.left) && node_is_double(node->as.binop.right);
			case BINOP_AND: case BINOP_OR:
				return node_is_bool(node->as.binop.left) && node_is_bool(node->as.binop.right);
		} break;
		case NODE_FUNC:
			return node_is_double(node->as.func.eq);
		case NODE_UNOP: switch (node->as.unop.type) {
			case UNOP_PAREN:
				return node_typecheck(node->as.unop.eq);
			case UNOP_NOT:
				return node_is_bool(node->as.unop.eq);
			case UNOP_ABS: case UNOP_NEG:
				return node_is_double(node->as.unop.eq);
		} break;
		default: break;
	}
	return 0;
}

typedef enum {
	ERROR_OK,

	ERROR_NON_NUM_PASSED,
	ERROR_UNDER_OR_OVERFLOW,
	ERROR_MULT_ZEROS,

	ERROR_NOT_ID,
	ERROR_MINUS_BEFORE_ZERO,
	ERROR_NOT_CLOSED_PAREN,
	ERROR_NOT_CLOSED_ABS,
	ERROR_FUNC_NO_LPAREN,
	ERROR_FUNC_NO_RPAREN,
} ParseLexError;
typedef enum {
	TOKEN_FUNC_ID,
	TOKEN_OPERAND,
	TOKEN_IF,
	TOKEN_THEN,
	TOKEN_ELSE,
	TOKEN_END,
	TOKEN_OPERATOR,
	TOKEN_EOF,
} TokenType;
typedef enum {
	OP_PLUS = '+',
	OP_MINUS = '-',
	OP_STAR = '*',
	OP_SLASH = '/',
	OP_CARROT = '^',
	OP_LPAREN = '(',
	OP_RPAREN = ')',
	OP_ABS = '|',
	OP_LESS = '<',
	OP_GREATER = '>',
	OP_AND = 'a',
	OP_OR = 'o',
	OP_NOT = 'n',
} OperatorType;
typedef union {
	TokenType type;
	struct {
		TokenType type;
		FuncType func;
	} func_id;
	struct {
		TokenType type;
		Node* node;
	} operand;
	struct {
		TokenType type;
		OperatorType op_type;
	} operator;
} Token;

static StringSlice tokens[] = {
	['o'] = { .in.len = 2, .in.buffer = "or" },
	['a'] = { .in.len = 3, .in.buffer = "and" },
	['i'] = { .in.len = 2, .in.buffer = "if" },
	['t'] = { .in.len = 4, .in.buffer = "then" },
	['e'] = { .in.len = 4, .in.buffer = "else" },
	['E'] = { .in.len = 3, .in.buffer = "end" },
	['n'] = { .in.len = 3, .in.buffer = "not" },
};
static StringSlice func_ids[FUNC_COUNT] = {
	[FUNC_ERROR] = { .in.buffer = "", .in.len = 0 },
	[FUNC_SQRT] =  { .in.buffer = "sqrt", .in.len = 4 },
	[FUNC_SIN] =   { .in.buffer = "sin", .in.len = 3 },
	[FUNC_COS] =   { .in.buffer = "cos", .in.len = 3 },
	[FUNC_TAN] =   { .in.buffer = "tan", .in.len = 3 },
	[FUNC_ASIN] =  { .in.buffer = "asin", .in.len = 4 },
	[FUNC_ACOS] =  { .in.buffer = "acos", .in.len = 4 },
	[FUNC_ATAN] =  { .in.buffer = "atan", .in.len = 4 },
	[FUNC_LOG] =   { .in.buffer = "log", .in.len = 3 },
	[FUNC_LOG10] = { .in.buffer = "log10", .in.len = 5 },
	[FUNC_LN] =    { .in.buffer = "ln", .in.len = 2 },
};
static FuncType is_func_id(StringSlice* s) {
	for (size_t i = 1; i < FUNC_COUNT; ++i) {
		if (slice_eq(s, &func_ids[i])) { return i; }
	}

	return FUNC_ERROR;
}

typedef struct {
	size_t len;
	char* str;
	size_t curr;

	Token* tokens;
	size_t curr_token;

	EvoArena arena;
	EvoAllocator alloc;

	EvoPool pool;
} Lexer;

#define is_alpha_or_under(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') || ((c) == '_'))
#define is_alpha_under_or_num(c) (is_alpha_or_under(c) || ('0' <= (c) && (c) <= '9'))
static double lexer_lex_double(Lexer* lex, int* err, char** err_msg) {
	if (!('0' <= lex->str[lex->curr] && lex->str[lex->curr] <= '9')) {
		if (err_msg) {
			cyx_str_append_lit(err_msg, "ERROR:\tThe first digit is not a number!\n");
		}
		if (err) { *err = 1; }
		return 0;
	}
	if (lex->str[lex->curr] == '0' && lex->len > lex->curr + 1 && lex->str[lex->curr + 1] == '0') {
		if (err_msg) {
			cyx_str_append_lit(err_msg, "ERROR:\tZero after a zero in a number!\n");
		} 
		if (err) { *err = 1; }
		return 0;
	}

	char* endptr;
	double ret = strtold(lex->str + lex->curr, &endptr);

	while (lex->str + lex->curr != endptr) { lex->curr++; }
	--lex->curr;
	if (errno == ERANGE) {
		if (err_msg) {
			cyx_str_append_lit(err_msg, "ERROR:\tUnder or overflow happened while trying to parse a number!\n");
		}
		if (err) { *err = 1; }
		return 0;
	}

	if (err) { *err = 0; }
	return ret;
}
static StringSlice lexer_lex_id(Lexer* lex, int* err, char** err_msg) {
	if (!is_alpha_or_under(lex->str[lex->curr])) {
		if (err_msg) {
			cyx_str_append_lit(err_msg, "ERROR:\tTrying to parse something as ID that is not an ID!\n");
		}
		if (err) { *err = 1; }
		return (StringSlice){ 0 };
	}

	size_t to_cp = 1;
	while (is_alpha_under_or_num(lex->str[lex->curr + to_cp])) { ++to_cp; }

	StringSlice ret = slice_from_lit_n(to_cp, lex->str + lex->curr);
	lex->curr += to_cp - 1;
	if (err) { *err = 0; }
	return ret;
}
static int lexer_lex(Lexer* lex, char* str, size_t len, char** err_msg) {
	*lex = (Lexer){
		.str = str,
		.len = len,
		.curr = 0,

		.arena = evo_arena_new(EVO_KB(8)),
		.pool = evo_pool_new(sizeof(Node)),

		.curr_token = 0,
	};
	lex->alloc = evo_allocator_arena(&lex->arena);

	lex->tokens = cyx_array_new(Token, &lex->alloc);

	for (; lex->curr < lex->len; ++lex->curr) {
		if (isspace(lex->str[lex->curr]) || lex->str[lex->curr] == '\n') { continue; }

		Token t = { 0 };
		if (lex->str[lex->curr] == '+') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_PLUS;
		} else if (lex->str[lex->curr] == '-') {
			if (lex->curr + 1 < lex->len && '0' <= lex->str[lex->curr + 1] && lex->str[lex->curr + 1] <= '9') {
				if (lex->str[lex->curr + 1] == '0' && lex->curr + 2 < lex->len && lex->str[lex->curr + 2] != '.') {
					if (err_msg) {
						cyx_str_append_lit(err_msg, "ERROR:\tStarted a number with a zero but then didn't put a points ['.'] after it!\n");
					}
					return 0;
				}

				++lex->curr;

				int err = 0;
				double num = -lexer_lex_double(lex, &err, err_msg);
				if (err) { return 0; }

				t.type = TOKEN_OPERAND;
				t.operand.node = node_num(&lex->pool, num);
			} else {
				t.type = TOKEN_OPERATOR;
				t.operator.op_type = OP_MINUS;
			}
		} else if (lex->str[lex->curr] == '*') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_STAR;
		} else if (lex->str[lex->curr] == '/') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_SLASH;
		} else if (lex->str[lex->curr] == '^') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_CARROT;
		} else if (lex->str[lex->curr] == '(') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_LPAREN;
		} else if (lex->str[lex->curr] == ')') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_RPAREN;
		} else if (lex->str[lex->curr] == '|') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_ABS;
		} else if (lex->str[lex->curr] == '<') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_LESS;
		} else if (lex->str[lex->curr] == '>') {
			t.type = TOKEN_OPERATOR;
			t.operator.op_type = OP_GREATER;
		} else if ('0' <= lex->str[lex->curr] && lex->str[lex->curr] <= '9') {
			int err = 0;
			double num = lexer_lex_double(lex, &err, err_msg);
			if (err) { return 0; }

			t.type = TOKEN_OPERAND;
			t.operand.node = node_num(&lex->pool, num);
		} else if (is_alpha_or_under(lex->str[lex->curr])) {

			int err = 0;
			StringSlice s = lexer_lex_id(lex, &err, err_msg);
			if (err) { return 0; }

			FuncType f = is_func_id(&s);
			if (f == FUNC_ERROR) {
				if (slice_eq(&s, &tokens['o'])) {
					t.type = TOKEN_OPERATOR;
					t.operator.op_type = OP_OR;
				} else if (slice_eq(&s, &tokens['a'])) {
					t.type = TOKEN_OPERATOR;
					t.operator.op_type = OP_AND;
				} else if (slice_eq(&s, &tokens['n'])) {
					t.type = TOKEN_OPERATOR;
					t.operator.op_type = OP_NOT;
				} else if (slice_eq(&s, &tokens['i'])) {
					t.type = TOKEN_IF;
				} else if (slice_eq(&s, &tokens['t'])) {
					t.type = TOKEN_THEN;
				} else if (slice_eq(&s, &tokens['e'])) {
					t.type = TOKEN_ELSE;
				} else if (slice_eq(&s, &tokens['E'])) {
					t.type = TOKEN_END;
				} else {
					t.type = TOKEN_OPERAND;
					t.operand.node = node_var(&lex->pool, slice_buffer_by_type(&s), s.len);
				}
			} else {
				t.type = TOKEN_FUNC_ID;
				t.func_id.func = f;
			}
		} else if (lex->str[lex->curr] == '\0') {
			t.type = TOKEN_EOF;
		} else {
			assert(0 && "UNREACHABLE");
		}

		cyx_array_append(lex->tokens, t);
	}
	if (cyx_array_top(lex->tokens)->type != TOKEN_EOF) {
		cyx_array_append(lex->tokens, ((Token){ .type = TOKEN_EOF }));
	}

	// for (size_t i = 0; i < cyx_array_length(lex->tokens); ++i) {
	// 	Token t = lex->tokens[i];
	// 	switch (t.type) {
	// 		case TOKEN_FUNC_ID: printf("TOKEN_FUNC_ID:\t"CYX_STR_FMT"\n", SLICE_UNPACK(&func_ids[t.func_id.func])); break;
	// 		case TOKEN_OPERAND:
	// 			printf("TOKEN_OPERAND:\t");
	// 			node_print(stdout, t.operand.node);
	// 			putchar('\n');
	// 			break;
	// 		case TOKEN_OPERATOR: printf("TOKEN_OPERATOR:\t%c\n", t.operator.op_type); break;
	// 		case TOKEN_EOF: printf("TOKEN_EOF\n"); break;
	// 	}
	// }

	return 1;
}
static Token* lexer_next(Lexer* lex) {
	if (lex->curr_token == cyx_array_length(lex->tokens)) {
		return NULL;
	}
	Token* t = &lex->tokens[lex->curr_token];
	++lex->curr_token;
	return t;
}
static Token* lexer_peek(Lexer* lex) {
	if (lex->curr_token == cyx_array_length(lex->tokens)) {
		return NULL;
	}
	Token* t = &lex->tokens[lex->curr_token];
	return t;
}
static void lexer_free(Lexer* lex) {
	evo_arena_destroy(&lex->arena);
	evo_pool_destroy(&lex->pool);
}

static Node* parse_expr_bp(Lexer* lex, uint8_t bp, char** err_msg);
#define parse_expr(lex, err_msg) parse_expr_bp(lex, 0, err_msg)
typedef struct {
	uint8_t left, right;
	uint8_t exists : 1;
} BindingPower;
static BindingPower infix_bp(char op) {
	switch (op) {
		case '+': case '-':
			return (BindingPower){ 5, 6, 1 };
		case '*': case '/':
			return (BindingPower){ 7, 8, 1};
		case '^':
			return (BindingPower){ 10, 9, 1 };
		case '<': case '>':
			return (BindingPower) { 3, 4, 1 };
		case 'a': case 'o':
			return (BindingPower) { 1, 2, 1 };
		default:
			return (BindingPower){ 0 };
	}
}
static BindingPower prefix_bp(char op) {
	switch (op) {
		case '+': case '-':
			return (BindingPower){ 0, 12, 1 };
		case 'n':
			return (BindingPower){ 0, 14, 1 };
		default:
			fprintf(stderr, "ERROR:\tUnknown operator: [%c]\n", op);
			exit(1);
	}
}
static Node* parse_expr_bp(Lexer* lex, uint8_t min_bp, char** err_msg) {
	Token* t = lexer_next(lex);
	Node* lhs = NULL;
	switch (t->type) {
		case TOKEN_OPERAND: lhs = t->operand.node; break;
		case TOKEN_IF: {
			Node* cond = parse_expr_bp(lex, 0, err_msg);
			if (!cond) { return NULL; }

			Token* next = lexer_next(lex);
			if (next->type != TOKEN_THEN) {
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tMissing 'then' token in an 'if' expression!\n");
				}
				return NULL;
			}

			Node* first = parse_expr_bp(lex, 0, err_msg);
			if (!first) { return NULL; }

			next = lexer_next(lex);
			if (next->type != TOKEN_ELSE) {
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tMissing 'else' token in an 'if' expression!\n");
				}
				return NULL;
			}

			Node* second = parse_expr_bp(lex, 0, err_msg);
			if (!second) { return NULL; }

			next = lexer_next(lex);
			if (next->type != TOKEN_END) {
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tMissing 'end' token in an 'if' expression!\n");
				}
				return NULL;
			}

			lhs = node_ternary(&lex->pool, cond, first, second);
		} break;
		case TOKEN_OPERATOR: {
			if (t->operator.op_type == '(') {
				lhs = parse_expr_bp(lex, 0, err_msg);
				if (!lhs) { return NULL; }

				Token* next = lexer_next(lex);
				if (next->type != TOKEN_OPERATOR || next->operator.op_type != ')') {
					if (err_msg) {
						cyx_str_append_lit(err_msg, "ERROR:\tMissing the closing parentheses!\n");
					}
					return NULL;
				}
			} else if (t->operator.op_type == '|') {
				lhs = parse_expr_bp(lex, 0, err_msg);
				if (!lhs) { return NULL; }

				Token* next = lexer_next(lex);
				if (next->type != TOKEN_OPERATOR || next->operator.op_type != '|') {
					if (err_msg) {
						cyx_str_append_lit(err_msg, "ERROR:\tMissing the closing absolute value!\n");
					}
					return NULL;
				}

				lhs = node_abs(&lex->pool, lhs);
			} else if (t->operator.op_type == '-') {
				BindingPower bp = prefix_bp(t->operator.op_type);

				lhs = parse_expr_bp(lex, bp.right, err_msg);
				if (!lhs) { return NULL; }

				lhs = node_neg(&lex->pool, lhs);
			} else if (t->operator.op_type == 'n') {
				BindingPower bp = prefix_bp(t->operator.op_type);

				lhs = parse_expr_bp(lex, bp.right, err_msg);
				if (!lhs) { return NULL; }

				lhs = node_not(&lex->pool, lhs);
			}
		} break;
		case TOKEN_FUNC_ID: {
			Token* next = lexer_next(lex);
			if (next->type != TOKEN_OPERATOR || next->operator.op_type != '(') {
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tMissing the opening parentheses after function call!\n");
				}
				return NULL;
			}

			lhs = parse_expr_bp(lex, 0, err_msg);
			if (!lhs) { return NULL; }

			next = lexer_next(lex);
			if (next->type != TOKEN_OPERATOR || next->operator.op_type != ')') {
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tMissing the closing parentheses after function call!\n");
				}
				return NULL;
			}

			lhs = node_func(&lex->pool, t->func_id.func, lhs);
		} break;
		case TOKEN_ELSE: {
			if (err_msg) {
				cyx_str_append_lit(err_msg, "ERROR:\tToken 'else' without an 'if' expression!\n");
			}
			return NULL;
		}
		case TOKEN_END: {
			if (err_msg) {
				cyx_str_append_lit(err_msg, "ERROR:\tToken 'end' without an 'if' expression!\n");
			}
			return NULL;
		}
		case TOKEN_THEN: {
			if (err_msg) {
				cyx_str_append_lit(err_msg, "ERROR:\tToken 'then' without an 'if' expression!\n");
			}
			return NULL;
		}
		case TOKEN_EOF: {
			if (err_msg) {
				cyx_str_append_lit(err_msg, "ERROR:\tEmpty expression!\n");
			}
			return NULL;
		}
	}

	for (;;) {
		Token* op = lexer_peek(lex);
		switch (op->type) {
			case TOKEN_EOF: goto stop;
			case TOKEN_OPERATOR: break;
			case TOKEN_IF: case TOKEN_THEN: case TOKEN_ELSE: case TOKEN_END: goto stop;
			default: 
				if (err_msg) {
					cyx_str_append_lit(err_msg, "ERROR:\tExpected an operator after an operand!\n");
					return NULL;
				}
		}

		BindingPower bp = infix_bp(op->operator.op_type);
		if (!bp.exists) { goto stop; }

		if (bp.left < min_bp) { goto stop; }
		lexer_next(lex);

		Node* rhs = parse_expr_bp(lex, bp.right, err_msg);
		if (!rhs) { return NULL; }

		lhs = node_binop(&lex->pool, (char)op->operator.op_type, lhs, rhs);
	}
stop:
	return lhs;
}

static void node_to_file(Node* node, VariableKV* vars, const char* file_path) {
	FILE* out = fopen(file_path, "w+");

	fprintf(out, "#include <math.h>\n\n");
	if (vars) {
		cyx_hashmap_foreach(var, vars) {
			if (var->key.len == 1 && (var->key.in.buffer[0] == 'x' || var->key.in.buffer[0] == 'y' || var->key.in.buffer[0] == 'z')) {
				continue;
			}
			fprintf(out, "static double "CYX_STR_FMT" = %lf;\n", SLICE_UNPACK(&var->key), var->value);
		}
	}
	fprintf(out, "double formula_calculate(double x, double y, double z) {\n");
	fprintf(out, "\treturn ");
	node_print(out, node);
	fprintf(out, ";\n");
	fprintf(out, "}\n");
	fclose(out);
}

typedef double (*Func)(double x, double y, double z);
#include <isosurfaces.h>
/*
 *         6-------------7            +------6------+   
 *        /|            /|          11|           10|   
 *       / |           / |          / 7           / 5
 *      2-------------3  |         +------2------+  |   
 *      |  4----------|--5         |  +------4---|--+   
 *      | /           | /          3 8           1 9
 *      |/            |/           |/            |/       
 *      0-------------1            +------0------+         
 */
typedef struct {
	double x, y, z;
} Vec3;
static Vec3 vec3_lerp(Vec3 a, Vec3 b, double t) {
	return (Vec3){
		.x = a.x + (b.x - a.x) * t,
		.y = a.y + (b.y - a.y) * t,
		.z = a.z + (b.z - a.z) * t,
	};
}

static void cube_marching(uint32_t** indicies, float** triangles, Func f, int res, double left, double right, double bottom, double top, double near, double far) {
	assert(res > 0);

	double w = (right - left) / res;
	double h = (top - bottom) / res;
	double d = (far - near) / res;

	for (size_t k = 0; (int)k < res - 1; ++k) {
		double z_0 = k * d + near;
		double z_1 = (k + 1) * d + near;
		for (size_t j = 0; (int)j < res - 1; ++j) {
			double y_0 = j * h + bottom;
			double y_1 = (j + 1) * h + bottom;
			for (size_t i = 0; (int)i < res - 1; ++i) {
				double x_0 = i * w + left;
				double x_1 = (i + 1) * w + left;
				Vec3 vecs[] = {
					(Vec3){x_0, y_0, z_0},
					(Vec3){x_1, y_0, z_0},
					(Vec3){x_0, y_1, z_0},
					(Vec3){x_1, y_1, z_0},
					(Vec3){x_0, y_0, z_1},
					(Vec3){x_1, y_0, z_1},
					(Vec3){x_0, y_1, z_1},
					(Vec3){x_1, y_1, z_1},
				};

				uint8_t mask = 0;
				for (uint8_t count = 0; count < 8; ++count) {
					Vec3 vec = vecs[count];
					if (f(vec.x, vec.y, vec.z) < 0) { mask |= 0x1 << count; }
				}
				uint16_t edge_mask = edge_masks[mask];

				uint32_t edge_ids[12] = { 0 };
				for (size_t idx = 0; idx < 12 && edge_mask; ++idx, edge_mask >>= 1) {
					uint8_t is_edge = edge_mask & 0b1;
					if (!is_edge) { continue; }

					const uint8_t* vertices = edge_vertex_indicies[idx];
					
					Vec3 vec1 = vecs[vertices[0]];
					double val1 = f(vec1.x, vec1.y, vec1.z);
					Vec3 vec2 = vecs[vertices[1]];
					double val2 = f(vec2.x, vec2.y, vec2.z);
					
					double t = val1 / (val1 - val2);
					Vec3 edge =  vec3_lerp(vec1, vec2, t);

					int found = -1;
					for (size_t tri = 0; tri + 5 < cyx_array_length(*triangles); tri += 6) {
						if (fabs((*triangles)[tri + 0] - edge.x) < 1e-5 &&
							fabs((*triangles)[tri + 1] - edge.y) < 1e-5 &&
							fabs((*triangles)[tri + 2] - edge.z) < 1e-5) {
							found = (int)tri;
							break;
						}
					}
					
					if (found == -1) {
						edge_ids[idx] = cyx_array_length(*triangles) / 6;
						cyx_array_append_mult(*triangles, edge.x, edge.y, edge.z, 0, 0, 0);
					} else {
						edge_ids[idx] = found / 6;
					}
				}

				const int8_t* arr = triangle_table[mask];
				for (; *arr != -1; arr++) {
					int idx = *arr;
					cyx_array_append(*indicies, edge_ids[idx]);
				}
			}
		}
	}

	assert(cyx_array_length(*triangles) % 6 == 0);

	for (size_t i = 0; i + 2 < cyx_array_length(*indicies); i += 3) {
		uint32_t idx1 = (*indicies)[i];
		uint32_t idx2 = (*indicies)[i + 1];
		uint32_t idx3 = (*indicies)[i + 2];
		Vec3 p1 = {
			.x = (*triangles)[6 * idx1 + 0],
			.y = (*triangles)[6 * idx1 + 1],
			.z = (*triangles)[6 * idx1 + 2]
		};
		Vec3 p2 = {
			.x = (*triangles)[6 * idx2 + 0],
			.y = (*triangles)[6 * idx2 + 1],
			.z = (*triangles)[6 * idx2 + 2]
		};
		Vec3 p3 = {
			.x = (*triangles)[6 * idx3 + 0],
			.y = (*triangles)[6 * idx3 + 1],
			.z = (*triangles)[6 * idx3 + 2]
		};
		Vec3 e1 = { .x = p2.x - p1.x, .y = p2.y - p1.y, .z = p2.z - p1.z, };
		Vec3 e2 = { .x = p3.x - p1.x, .y = p3.y - p1.y, .z = p3.z - p1.z, };

		Vec3 n = {
			.x = e1.y * e2.z - e1.z * e2.y,
			.y = e1.z * e2.x - e1.x * e2.z,
			.z = e1.x * e2.y - e1.y * e2.x
		};
		float magn = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
		if (fabsf(magn) < 1e-5) {
			continue;
		}
		n = (Vec3){
			.x = n.x / magn,
			.y = n.y / magn,
			.z = n.z / magn,
		};

		(*triangles)[6 * idx1 + 3] += n.x;
		(*triangles)[6 * idx1 + 4] += n.y;
		(*triangles)[6 * idx1 + 5] += n.z;
		(*triangles)[6 * idx2 + 3] += n.x;
		(*triangles)[6 * idx2 + 4] += n.y;
		(*triangles)[6 * idx2 + 5] += n.z;
		(*triangles)[6 * idx3 + 3] += n.x;
		(*triangles)[6 * idx3 + 4] += n.y;
		(*triangles)[6 * idx3 + 5] += n.z;
	}
	for (size_t i = 0; i + 5 < cyx_array_length(*triangles); i += 6) {
		float x = (*triangles)[i + 3];
		float y = (*triangles)[i + 4];
		float z = (*triangles)[i + 5];
		float magn = sqrtf(x * x + y * y + z * z);
		if (fabsf(magn) < 1e-5) {
			continue;
		}
		(*triangles)[i + 3] /= magn;
		(*triangles)[i + 4] /= magn;
		(*triangles)[i + 5] /= magn;
	}

	// printf("triangles[%zu] = {", cyx_array_length(*triangles));
	// for (size_t i = 0; i < cyx_array_length(*triangles); ++i) {
	// 	if (i % 6 == 0) printf("\n\t");
	// 	printf("%.2f, ", (*triangles)[i]);
	// }
	// printf("\n}\n");
	// printf("indices[%zu] = {", cyx_array_length(*indicies));
	// for (size_t i = 0; i < cyx_array_length(*indicies); ++i) {
	// 	if (i % 3 == 0) printf("\n\t");
	// 	printf("%u, ", (*indicies)[i]);
	// }
	// printf("\n}\n");
}

int cube_march(uint32_t** indicies, float** triangles, char* equation, VariableKV* vars, CubeMarchDefintions defs, char** err_msg) {
	Lexer lex = { 0 };
	if (!lexer_lex(&lex, equation, cyx_str_length(equation), err_msg)) { return 0; }

	Node* root = parse_expr(&lex, err_msg);
	if (!root) { return 0; }

	if (!node_typecheck(root)) {
		cyx_array_clear(*indicies);
		cyx_array_clear(*triangles);

		cyx_str_append_lit(err_msg, "ERROR:\tFound error while typechecking!\n");
		return 0;
	}
	node_to_file(root, vars, "./build/formula.c");

	pid_t pid = fork();
	if (pid == 0) {
		execlp("gcc", "gcc", "./build/formula.c", "-O3", "-shared", "-fPIC", "-o", "./build/libformula.so", "-lm", NULL);
	} else if (pid < 0) {
		cyx_str_append_lit(err_msg, "ERROR:\tUnable to fork and compile the function!\n");
		return 0;
	} else {

		int status = 0;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			// printf("Child exited normally!\n");
		} else if (WIFSIGNALED(status)) {
			psignal(WTERMSIG(status), "Exit signal");
		}
	}
	lexer_free(&lex);

	void* handle = dlopen("./build/libformula.so", RTLD_NOW);
	if (handle) {
		Func formula_calculate = dlsym(handle, "formula_calculate");
		cube_marching(indicies, triangles, formula_calculate, defs.res, defs.left, defs.right, defs.bottom, defs.top, defs.near, defs.far);
		dlclose(handle);
	} else {
		cyx_str_append_lit(err_msg, "ERROR:\tUnable to open a shared object file!\n");
		return 0;
	}

	return 1;
}
