#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

#include "scanner.h"
#include <stdbool.h>
#include <stdint.h>

// enum for variable types
enum variable_type {
    VAR_QBIT,
    VAR_BIT,
    VAR_VECTOR,
    VAR_INTEGER,
    VAR_NATURAL,
    VAR_RATIONAL,
    VAR_IRRATIONAL,
    VAR_COMPLEX,
    VAR_CHAR,
    VAR_STRING
};

enum error_type {
    UNEXPECTED_ERROR,
    WRONG_TYPE_ERROR,
    UNKOWN_SYMBOL_ERROR,
    NO_CONTEXT_ERROR,
    UNKOWN_TYPE_ERROR,
    NAME_CONFLICT_ERROR
};

enum operation_type {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION,
    MOD
};

enum boolean_operation_type {
    BOOL_AND,
    BOOL_OR,
    BOOL_EQUALS,
    BOOL_NOT_EQUALS,
    BOOL_GREATER,
    BOOL_LESSER,
    BOOL_GREATER_EQUALS,
    BOOL_LESSER_EQUALS
};

// enum for keywords
enum keywords {
    ROOT,
    VAR_DEF,
    ASSIGN_VAL,
    CONDITION,
    LOOP,
    INCLUDE,
    FUNC_DEF,
    RETURN,
    NOT_DET
};

// struct for variables
// variables will often be pointers so they can be referenced multiple times in the ast
typedef struct variable {
    enum variable_type type;
    char* name;
} variable;

typedef struct assignment {
    bool var_def;
    bool operation;
    variable var;
    char *value_1;

    ast *operation;
    char *value_2;
} assign;

typedef struct boolean_operation {
    enum boolean_operation_type type;

} condition;

typedef struct while_loop {
    condition condition;
    struct abstract_syntax_tree *body;
} l_while;

typedef struct function {
    bool quantum;
    struct abstract_syntax_tree *body;
    char *name;
    int num_of_variables;
    variable *variables;

} function;

typedef struct binary_operation {
    enum operation_type type;
    char *value_1;
    char *value_2;
} bin_op;


// struct for the abstract syntax tree
typedef struct abstract_syntax_tree {
    enum keywords type;
    struct abstract_syntax_tree *branch;
    union {
        variable var;
        l_while loop;
        function func;
        bin_op op;
        char *file_path;
    };
} ast;


// returns the root to the generated abstract syntax tree of the given token list
ast* generate_ast(token *first_token);
void handle_error(token *error_token, enum error_type type, char *error_message);

#endif