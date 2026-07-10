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
    VAR_DOUBLE,
    VAR_UNKOWN
};

enum error_type {
    UNEXPECTED_ERROR,
    WRONG_TYPE_ERROR,
    UNKOWN_SYMBOL_ERROR,
    NO_CONTEXT_ERROR,
    UNKOWN_TYPE_ERROR,
    NAME_CONFLICT_ERROR,
    MISSING_ERROR
};


// enum for keywords
enum keywords {
    ROOT,
    TYPE,
    NAME,
    VALUE,
    ASSIGN,
    INCLUDE,
    FUNC_DEF,
    RETURN,
    NOT_DET
};



typedef struct {
    struct abstract_syntax_tree *assignee;
    struct abstract_syntax_tree *assignor;
} assign;


typedef struct {
    char *name;
    struct abstract_syntax_tree *body;
    struct abstract_syntax_tree *variables;

} function;


// struct for the abstract syntax tree
typedef struct abstract_syntax_tree {
    enum keywords type;
    struct abstract_syntax_tree *branch;
    union {
        enum variable_type var_type;
        function func;
        assign assignment;
        char *value;
    };
} ast;


// returns the root to the generated abstract syntax tree of the given token list
ast* parse_start(ast *current_node);
ast* parse_assignor(ast *current_node);
ast* generate_ast(token *first_token);
void handle_error(token *error_token, enum error_type type, char *error_message);

#endif