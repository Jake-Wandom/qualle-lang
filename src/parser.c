#include "parser.h"
#include "helper.h"
#include "error_qualle.h"
#include "global_flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global variables that track the current token and last linebreak
static token *current_token;

// flags that mark, if we are in a certain program state
bool adaptive = 0;

// reserved keywords list
#define NUM_KEYWORDS 46
static const char *keywords[] = {
    "def",
    "if",
    "else",
    "for",
    "while",
    "measure",
    "MEASURE",
    "MZ",
    "mz",
    "return",
    "qubit",
    "bit",
    "bool",
    "int",
    "uint",
    "double",
    "void",
    "main",
    "=",
    "==",
    "!",
    "!=",
    "+",
    "-",
    "*",
    "/",
    "%",
    "^",
    "<",
    ">",
    "<=",
    ">=",
    "&&",
    "||",
    "?",
    ":",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    ",",
    ".",
    ";",
    " "
};
/*
creates a new node with all pointer values set to NULL
contrary to create_token, this function does not automatically append
*/
ast* create_node(enum ast_type type, int line, char *value){
    ast *new_node = calloc(1, sizeof(ast));
    if(!new_node){
        add_error_entry(FATAL, -1, "Failed to allocate memory for new node");
        return NULL;
    }
    new_node->type = type;
    new_node->branch = NULL;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->other = NULL;
    new_node->value = strdup(value);
    new_node->resolved_type = VAR_UNKOWN;
    new_node->line = line;
    new_node->index = -1;
    new_node->res_id = -1;

    return new_node;
}

/*
Traverses the token list for num tokens
If the token is NULL we exit, this should not happen
If the token is END, we return it
*/
static void switch_token(int num){
    for(int i = 0; i < num; i++){
        if(!current_token){
            add_error_entry(FATAL, -1, "Null pointer while token switch");
        }
        if(current_token->type == T_END){
            return;
        }
        current_token = current_token->next_token;
    }
}

/*
*/
static token* look_forward(int num){
    token *tok = current_token;
    for(int i = 0; i < num && tok->type != T_END; i++){
        tok = tok->next_token;
    }
    return tok;
}

/*
Matches the token to a token type and word
Both the type and word have to be correct
*/
static bool match(token *tok, enum token_type type, char *word){
    if(!word) return 0;
    if(!tok->value) return 0;
    
    int res = strcmp(tok->value, word);
    if((res == 0) && (tok->type == type)){
        return 1;
    } else {
        return 0;
    }
}

/*
If the current token is a match, it switches token and returns
*/
static bool accept(enum token_type type, char *word){
    if(!match(current_token, type, word)) return 0;
    switch_token(1);
    return 1;
}

/*
Special case of accept, where we throw an error message if it is false
*/
static bool expect(enum token_type type, char *word){
    if(accept(type, word)) return 1;
    char message[64];
    snprintf(message, 64, "Expected %s", word);
    add_error_entry(ERROR, current_token->line, message);
    return 0;
}

/*
Function that gets called after every statement parsing function
It Checks the termination conditions for a statement
*/
static bool end_statement(void){
    if(current_token->type == T_END_OF_LINE){
        switch_token(1);
        return 1;
    }
    if(current_token->type == T_END || match(current_token, T_BRACKET_CLOSE, "}") || match(current_token, T_BRACKET_CLOSE, ")")){
        return 1;
    }
    add_error_entry(ERROR, current_token->line, "Expected end of line after statement");
    return 0;
}

static bool check_keywords(char *name){
    for(int i = 0; i < NUM_KEYWORDS; i++){
        if(strcmp(keywords[i], name) == 0){
            char message[30];
            snprintf(message, 30, "%s is an invalid name", name);
            add_error_entry(ERROR, current_token->line, message);
            return 0;
        }
    }
    return 1;
}

static bool determine_binop(op_info *op){
    if(current_token->type != T_OPERATOR) return 0;
    char op1 = current_token->value[0];
    char op2 = (look_forward(1)->type == T_OPERATOR) ? look_forward(1)->value[0] : 0;

    switch(op1){
        case '+':
            *op = (op_info){"+", 5, 0, 1};
            return 1;
        case '-':
            *op = (op_info){"-", 5, 0, 1};
            return 1;
        case '*':
            *op = (op_info){"*", 6, 0, 1};
            return 1;
        case '/':
            *op = (op_info){"/", 6, 0, 1};
            return 1;
        case '%':
            *op = (op_info){"%", 6, 0, 1};
            return 1;
        case '^':
            *op = (op_info){"^", 7, 1, 1};
            return 1;
        case '<':
            *op = (op2 == '=') ? (op_info){"<=", 4, 0, 2} : (op_info){"<", 4, 0, 1};
            return 1;
        case '>':
            *op = (op2 == '=') ? (op_info){">=", 4, 0, 2} : (op_info){">", 4, 0, 1};
            return 1;
        case '=':
            if(op2 == '='){
                *op = (op_info){"==", 3, 0, 2};
                return 1;
            }
            return 0;
        case '&':
            if(op2 == '&'){
                *op = (op_info){"&&", 2, 0, 2};
                return 1;
            }
            return 0;
        case '|':
            if(op2 == '|'){
                *op = (op_info){"||", 1, 0, 2};
                return 1;
            }
            return 0;
        case '!':
            if(op2 == '='){
                *op = (op_info){"!=", 3, 0, 2};
                return 1;
            }
            return 0;
    }

    return 0;
}

enum variable_type check_type(char *name){
    enum variable_type type = VAR_UNKOWN;

    if(!name){
        add_error_entry(INTERNAL, current_token->line, "passed null poiner to check_type()");
        return type;
    }

    if(strcmp(name, "qubit") == 0){
        type = VAR_QUBIT;
    } else if(strcmp(name, "bit") == 0){
        type = VAR_BIT;
    } else if(strcmp(name, "bool") == 0){
        type = VAR_BIT;
    } else if(strcmp(name, "int") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(name, "uint") == 0){
        type = VAR_UINTEGER;
    } else if(strcmp(name, "double") == 0){
        type = VAR_DOUBLE;
    } else if(strcmp(name, "void") == 0){
        type = VAR_VOID;
    }

    return type;
}

ast* parse_primary(void){
    token *tok = current_token;

    if(tok->type == T_NUMBER){
        enum variable_type type = VAR_INTEGER;
        if(strchr(tok->value, '.') != NULL){
            type = VAR_DOUBLE;
            if(strchr(tok->value, '.') != NULL){
                add_error_entry(ERROR, tok->line, "Double has multiple decimal points");
                return NULL;
            }
        }

        ast *new_node = create_node(VALUE, tok->line, tok->value);
        if(!new_node) return NULL;
        new_node->resolved_type = type;
        
        switch_token(1);
        return new_node;
    }
    if(tok->type == T_IDENTIFIER){
        ast *new_node = create_node(IDENTIFIER, tok->line, tok->value);
        if(!new_node) return NULL;

        switch_token(1);
        return new_node;
    }
    if(match(tok, T_BRACKET_OPEN, "(")){
        switch_token(1);
        ast *inner = parse_expression(0);
        if(!inner || !expect(T_BRACKET_CLOSE, ")")) return NULL;

        return inner;
    }

    add_error_entry(ERROR, tok->line, "Expected value or variable");
    return NULL;
}

ast* parse_unary(void){
    token *tok = current_token;
    if(match(tok, T_OPERATOR, "-") || match(tok, T_OPERATOR, "!")){
        switch_token(1);
        ast *operand = parse_unary();
        if(!operand) return NULL;

        ast *new_node = create_node(UNOP, tok->line, tok->value);
        if(!new_node) return NULL;
        new_node->left = operand;

        return new_node;
    }

    return parse_primary();
}

ast* parse_expression(int min_prec){
    ast *left = parse_unary();
    if(!left) return NULL;

    op_info op;
    while(determine_binop(&op) && op.prec >= min_prec){
        int line = current_token->line;
        switch_token(op.num_tokens);

        ast *right = parse_expression(op.right_assoc ? op.prec : op.prec + 1);
        if(!right) return NULL;

        ast *new_node = create_node(op.prec <= 4 ? BOOLOP : BINOP, line, op.value);
        new_node->left = left;
        new_node->right = right;
        left = new_node;
    }

    return left;
}

ast* parse_body(bool *ok){
    ast *body_root = create_node(ROOT, current_token->line, "");
    ast *current_node = body_root;
    token *last_token;
    *ok = 1;

    while(!match(current_token, T_BRACKET_CLOSE, "}")){
        while(current_token->type == T_END_OF_LINE || current_token->type == T_COMMENT || current_token->type == T_START) switch_token(1);
        if(match(current_token, T_BRACKET_CLOSE, "}")) break;

        if(current_token->type == T_END){
            add_error_entry(ERROR, current_token->line, "Missing '}'");
            *ok = 0;
            return NULL;
        }

        last_token = current_token;

        ast *node = parse_statement();
        if(!node){
            *ok = 0;
            return NULL;
        }

        current_node->branch = node;
        current_node = current_node->branch;

        if(last_token == current_token){
            add_error_entry(INTERNAL, current_token->line, "Infinite loop detected");
            *ok = 0;
            switch_token(1);
        }
    }
    current_node = body_root->branch;
    free(body_root->value);
    free(body_root);
    

    return current_node;
}

ast* parse_function(void){
    switch_token(1);
    ast *new_node;

    if(current_token->type != T_IDENTIFIER){
        add_error_entry(ERROR, current_token->line, "Missing function name");
        new_node = create_node(FUNCTION, current_token->line, "missing_name");
    } else { 
        if(!check_keywords(current_token->value)){
            return NULL;
        }
        new_node = create_node(FUNCTION, current_token->line, current_token->value);
    }
    switch_token(1);

    if(!expect(T_BRACKET_OPEN, "(")) return NULL;

    ast **tail = &new_node->left;
    if(!match(current_token, T_BRACKET_CLOSE, ")")){
        do {
            if(check_type(current_token->value) != VAR_UNKOWN && look_forward(1)->type == T_IDENTIFIER){
                ast *name = create_node(NAME, current_token->line, look_forward(1)->value);
                if(!name) return NULL;
                name->resolved_type = check_type(current_token->value);

                *tail = name;
                tail = &name->branch;

                switch_token(2);
            } else {
                add_error_entry(ERROR, current_token->line, "Expected parameter form 'type' 'name'");
                return NULL;
            }
        } while(accept(T_DELIMITER, ","));
    }
    if(!expect(T_BRACKET_CLOSE, ")")) return NULL;

    enum variable_type return_type = VAR_VOID;
    if(accept(T_OPERATOR, ":")){
        if(current_token->type != T_IDENTIFIER){
            add_error_entry(ERROR, current_token->line, "Missing return type in function declaration");
            return NULL;
        }

        return_type = check_type(current_token->value);
        if(return_type == VAR_UNKOWN){
            add_error_entry(ERROR, current_token->line, "Unknown type as return type");
            return NULL;
        }
        switch_token(1);
    }
    ast *return_node = create_node(TYPE, current_token->line, "return");
    if(!return_node) return NULL;
    return_node->resolved_type = return_type;
    new_node->other = return_node;

    if(!expect(T_BRACKET_OPEN, "{")) return NULL;

    bool ok;
    ast *subtree = parse_body(&ok);
    if(!subtree && !ok) return NULL;
    new_node->right = subtree;

    if(!expect(T_BRACKET_CLOSE, "}")) return NULL;

    return end_statement() ? new_node : NULL;
}

ast* parse_loop(bool for_loop){
    ast *new_node;
    ast *latch;
    if(for_loop){
        new_node = create_node(FOR_LOOP, current_token->line, "for");
        switch_token(1);

        if(!expect(T_BRACKET_OPEN, "(")) return NULL;

        ast *init = parse_statement();
        if(!init) return NULL;
        new_node->left = init;

        ast *condition = parse_expression(0);
        if(!condition) return NULL;
        new_node->other = condition;

        if(!expect(T_END_OF_LINE, ";")) return NULL;

        latch = parse_statement();
        if(!latch) return NULL;

    } else {
        new_node = create_node(WHILE_LOOP, current_token->line, "while");
        switch_token(1);

        if(!expect(T_BRACKET_OPEN, "(")) return NULL;

        ast *condition = parse_expression(0);
        if(!condition) return NULL;
        new_node->left = condition;

    }
    if(!expect(T_BRACKET_CLOSE, ")")) return NULL;

    if(!expect(T_BRACKET_OPEN, "{")) return NULL;

    bool ok;
    ast *subtree = parse_body(&ok);
    if(!subtree && !ok) return NULL;
    if(!subtree) new_node->right = latch;
    else new_node->right = subtree;

    // append latch at the end of subtree
    if(for_loop){
        while(subtree->branch != NULL) subtree = subtree->branch;
        subtree->branch = latch;
    }

    if(!expect(T_BRACKET_CLOSE, "}")) return NULL;

    return end_statement() ? new_node : NULL;
}

ast* parse_conditional(void){
    ast *new_node = create_node(CONDITIONAL, current_token->line, "if");
    switch_token(1);

    if(!expect(T_BRACKET_OPEN, "(")) return NULL;

    if(match(current_token, T_IDENTIFIER, "measure") || match(current_token, T_IDENTIFIER, "MEASURE") || match(current_token, T_IDENTIFIER, "mz") || match(current_token, T_IDENTIFIER, "MZ")){
        switch_token(1);
        if(!expect(T_BRACKET_OPEN, "(")) return NULL;

        if(current_token->type != T_IDENTIFIER){
            add_error_entry(ERROR, current_token->line, "Expected Identifer as measure argument");
            return NULL;
        }

        ast *m_node = create_node(MEASURE, current_token->line, current_token->value);
        if(!m_node) return NULL;
        new_node->left = m_node;
        switch_token(1);

        if(!expect(T_BRACKET_CLOSE, ")")) return NULL;
    } else {
        ast *condition = parse_expression(0);
        if(!condition) return NULL;
        new_node->left = condition;
    }

    if(!expect(T_BRACKET_CLOSE, ")")) return NULL;

    if(!expect(T_BRACKET_OPEN, "{")) return NULL;

    bool ok;
    ast *if_body = parse_body(&ok);
    if(!if_body && !ok) return NULL;
    new_node->right = if_body;

    if(!expect(T_BRACKET_CLOSE, "}")) return NULL;

    if(current_token->type == T_END_OF_LINE && match(look_forward(1), T_IDENTIFIER, "else")) switch_token(1);

    if(accept(T_IDENTIFIER, "else")){
        if(!expect(T_BRACKET_OPEN, "{")) return NULL;

        ast *else_body = parse_body(&ok);
        if(!else_body || !ok) return NULL;
        new_node->other = else_body;

        if(!expect(T_BRACKET_CLOSE, "}")) return NULL;
    }
    return end_statement() ? new_node : NULL;
}

ast* parse_measure(void){
    switch_token(1);
    if(!expect(T_BRACKET_OPEN, "(")) return NULL;

    if(current_token->type != T_IDENTIFIER){
        add_error_entry(ERROR, current_token->line, "Expected Identifer as measure argument");
        return NULL;
    }

    ast *new_node = create_node(MEASURE, current_token->line, current_token->value);
    if(!new_node) return NULL;
    switch_token(1);

    if(!expect(T_BRACKET_CLOSE, ")")) return NULL;

    return end_statement() ? new_node : NULL;
}

ast* parse_return(void){
    ast *new_node = create_node(RETURN, current_token->line, "return");
    if(!new_node) return NULL;
    switch_token(1);

    if(current_token->type != T_END_OF_LINE){
        ast* expr_node = parse_expression(0);
        if(!expr_node) return NULL;
        new_node->left = expr_node;
    }

    return end_statement() ? new_node : NULL;
}

ast* parse_assign(void){
    ast *new_node;
    if(check_type(current_token->value) != VAR_UNKOWN){
        if(!check_keywords(look_forward(1)->value)) return NULL;

        new_node = create_node(ASSIGN, current_token->line, look_forward(2)->value);
        if(!new_node) return NULL;
        // creating name node for the new variable
        new_node->left = create_node(NAME, current_token->line, look_forward(1)->value);
        new_node->left->resolved_type = check_type(current_token->value);

        switch_token(3);
    } else {
        if(!check_keywords(current_token->value)) return NULL;

        new_node = create_node(ASSIGN, current_token->line, look_forward(1)->value);
        if(!new_node) return NULL;
        // creating identifier node for assign
        new_node->left = create_node(IDENTIFIER, current_token->line, current_token->value);
        switch_token(2);
    }

    ast *subtree = parse_expression(0);
    if(!subtree) return NULL;

    new_node->right = subtree;

    return end_statement() ? new_node : NULL;
}

ast* parse_declaration(void){
    // creating name node for the new variable
    if(!check_keywords(look_forward(1)->value)) return NULL;
    ast *new_node = create_node(NAME, current_token->line, look_forward(1)->value);
    new_node->resolved_type = check_type(current_token->value);

    switch_token(2);

    return end_statement() ? new_node : NULL;
}

ast* parse_call(void){ 
    ast *new_node = create_node(CALL, current_token->line, current_token->value);
    if(!new_node) return NULL;
    
    switch_token(2);
    
    ast **tail = &new_node->left;
    if(!match(current_token, T_BRACKET_CLOSE, ")")){
        do {
            ast *arg = parse_expression(0);
            if(!arg) return NULL;

            *tail = arg;
            tail = &arg->branch;
        } while(accept(T_DELIMITER, ","));
    }


    if(!expect(T_BRACKET_CLOSE, ")")) return NULL;

    return end_statement() ? new_node : NULL;
}

ast* parse_statement(void){
    if(current_token->type != T_IDENTIFIER){
        add_error_entry(ERROR, current_token->line, "Expected an Identifer");
        switch_token(1);
        return NULL;
    }

    if(match(current_token, T_IDENTIFIER, "def")){
        return parse_function();
    }

    if(match(current_token, T_IDENTIFIER, "while")){
        return parse_loop(0);
    }

    if(match(current_token, T_IDENTIFIER, "for")){
        return parse_loop(1);
    }

    if(match(current_token, T_IDENTIFIER, "if")){
        return parse_conditional();
    }

    if(match(current_token, T_IDENTIFIER, "measure") || match(current_token, T_IDENTIFIER, "MEASURE") || match(current_token, T_IDENTIFIER, "mz") || match(current_token, T_IDENTIFIER, "MZ")){
        return parse_measure();
    }

    if(match(current_token, T_IDENTIFIER, "return")){
        return parse_return();
    }

    bool typed = check_type(current_token->value) != VAR_UNKOWN && look_forward(1)->type == T_IDENTIFIER;

    if(typed && match(look_forward(2), T_OPERATOR, "=") && !match(look_forward(3), T_OPERATOR, "=")){
        return parse_assign();
    }

    if(typed) return parse_declaration();

    if(match(look_forward(1), T_OPERATOR, "=") && !match(look_forward(2), T_OPERATOR, "=")){
        return parse_assign();
    }

    if(match(look_forward(1), T_BRACKET_OPEN, "(")){
        return parse_call();
    }

    add_error_entry(ERROR, current_token->line, "Unable to find a statement");
    switch_token(1);
    return NULL;
}

int parse_start(ast *current_node){
    int res = 0;
    token *last_token;

    while(1){
        while(current_token->type == T_END_OF_LINE || current_token->type == T_COMMENT || current_token->type == T_START) switch_token(1);
        if(current_token->type == T_END) break;

        last_token = current_token;

        ast *node = parse_statement();
        if(!node){
            res = 1;
            while(current_token->type != T_END && current_token->type != T_END_OF_LINE) switch_token(1);
        }
        else {
            current_node->branch = node;
            current_node = current_node->branch;
        }

        if(last_token == current_token){
            add_error_entry(INTERNAL, current_token->line, "Infinite loop detected");
            switch_token(1);
        }
    }
    return res;
}

/*
this is the access function for main
it resets the global variables
*/
ast* generate_ast(token *first_token){
    ast *root = create_node(ROOT, 0, "root");
    current_token = first_token;

    int res = parse_start(root);

    if(res != 0){
        char message[40];
        snprintf(message, 40, "parse_start() exited with error code %d", res);
        add_error_entry(INTERNAL, current_token->line, message);
        free_ast(root);
    }
    check_errors();
    return root;
}