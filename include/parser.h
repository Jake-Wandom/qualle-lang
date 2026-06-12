#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

#include "scanner.h"
#include <stdint.h>

// enum for variable types
enum variable_type {
    VAR_QBIT,
    VAR_BIT,
    VAR_BOOL,
    VAR_INTEGER,
    VAR_NATURAL,
    VAR_RATIONAL,
    VAR_IRRATIONAL,
    VAR_COMPLEX,
    VAR_CHAR,
    VAR_STRING
};

enum loop_type {
    loop_WHILE,
    loop_FOR
};

enum function_type {
    CLASSICAL,
    QUANTUM
};

enum operation_type {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION
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
    CONDITIONAL,
    CONDITION,
    LOOP,
    FUNC_DEF,
    RETURN,
    MAIN,
    NOT_DET
};

// struct for variables
// variables will often be pointers so they can be referenced multiple times in the ast
typedef struct variable {
    enum variable_type type;
    char* name;
    char* value;
} variable;

typedef struct boolean_operation {
    enum boolean_operation_type type;

} condition;

typedef struct for_loop {
    uint16_t num_of_iterations;
    struct abstract_syntax_tree *body;

} l_for;

typedef struct while_loop {
    condition condition;
    
} l_while;

typedef struct function {
    enum function_type type;
    struct abstract_syntax_tree *body;
    int num_of_variables;
    variable *variables;

} function;

typedef struct binary_operation {
    enum operation_type type;
    variable *var_1;
    variable *var_2;
} bin_op;


// struct for the abstract syntax tree
typedef struct abstract_syntax_tree {
    enum keywords type;
    struct abstract_syntax_tree *branch;
    union {
        variable *var;
        l_for for_loop;
        function func;
        bin_op op;
    };
} ast;


// returns the root to the generated abstract syntax tree of the given token list
ast* generate_ast(token *first_token);

#endif