#ifndef H_PARSER_QUALLE
#define H_PARSER_QUALLE

#include "lexer.h"

#include <stdbool.h>
#include <stdint.h>

// enum for variable types
enum variable_type {
    VAR_QUBIT,
    VAR_BIT,
    VAR_INTEGER,
    VAR_UINTEGER,
    VAR_DOUBLE,
    VAR_VOID,
    VAR_UNKOWN
};

// enum for keywords
enum ast_type {
    ROOT,
    TYPE,
    NAME,
    IDENTIFIER,
    VALUE,
    ASSIGN,
    BINOP,
    BOOLOP,
    UNOP,
    FUNCTION,
    CALL,
    CONDITIONAL,
    FOR_LOOP,
    WHILE_LOOP,
    MEASURE,
    INCLUDE,
    RETURN
};


// struct for the abstract syntax tree
// tbh this implementation is more like a linked list with extra steps
typedef struct abstract_syntax_tree{
    enum ast_type type;

    // pointer to the next branches
    struct abstract_syntax_tree *branch;

    struct abstract_syntax_tree *left;
    struct abstract_syntax_tree *right;
    struct abstract_syntax_tree *other;
    
    char *value;
    
    // line of the first token that was parsed to this node
    int line;
    
    // relevant for the analyser
    enum variable_type resolved_type;
    int index; // index in the variable list
    int res_id; // index in the result list
    
} ast;

// struct for determining the operation
typedef struct {
    char *value;
    int prec;
    bool right_assoc;
    int num_tokens;
} op_info;


ast* parse_statement(void);
ast* parse_body(bool *ok);
ast* parse_expression(int min_prec);
ast* parse_primary(void);
ast* parse_unary(void);
// returns the root to the generated abstract syntax tree of the given token list
ast* generate_ast(token *first_token);

#endif