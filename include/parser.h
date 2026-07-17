#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

#include "lexer.h"

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stdint.h>

// enum for variable types
enum variable_type {
    VAR_QUBIT,
    VAR_BIT,
    VAR_VECTOR,
    VAR_INTEGER,
    VAR_NATURAL,
    VAR_DOUBLE,
    VAR_VOID
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
enum ast_type {
    ROOT,
    TYPE,
    NAME,
    IDENTIFIER,
    VALUE,
    ASSIGN,
    FUNCTION,
    CALL,
    MEASURE,
    INCLUDE,
    RETURN
};


// struct for the abstract syntax tree
// tbh this implementation is more like a linked list with extra steps
typedef struct abstract_syntax_tree {
    enum ast_type type;

    // pointer to the next branch
    struct abstract_syntax_tree *branch;
    // line of the first token that was parsed to this node
    int line;

    // left and right subbranch for functions and operations
    struct abstract_syntax_tree *left;
    struct abstract_syntax_tree *right;

    // union that contains possible variables for different node types
    union {
        enum variable_type var_type;
        char *name;
        char *value;
    };

    // This is for the analyser to fill in
    enum variable_type resolved_type;
    bool consumed;
    LLVMValueRef *llvm;
    
} ast;


// returns the root to the generated abstract syntax tree of the given token list
ast* parse_start(ast *current_node);
ast* parse_between(ast *current_node);
ast* parse_assignor(ast *current_node);
ast* generate_ast(token *first_token);
void handle_error(token *error_token, enum error_type type, char *error_message);

#endif