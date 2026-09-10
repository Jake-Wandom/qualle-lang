#include "parser.h"
#include "error_qualle.h"
#include "global_flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global variables that track the current token and last linebreak
static token *current_token;
static ast *last_eol;

// flags that mark, if we are in a certain program state
bool adaptive = 0;

static bool in_parameters;
static bool in_body;
static bool in_assign;
static bool in_loop;
static bool in_if;


/*
creates a new node with all pointer values set to NULL
contrary to create_token, this function does not automatically append
*/
ast* create_node(void){
    ast *new_node = calloc(1, sizeof(ast));
    if(!new_node){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for new node", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    new_node->type = ROOT;
    new_node->branch = NULL;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->name = NULL;
    new_node->llvm =  NULL;
    new_node->line = current_token->line;

    return new_node;
}

/*
Traverses the token list for num tokens
If the token is NULL we exit, this should not happen
If the token is END, we return it
*/
void switch_token(int num){
    for(int i = 0; i < num; i++){
        if(!current_token){
            diagnose d = {.line = -1, .message = "NULL pointer while token switch", .type = FATAL};
            add_error_entry(d);
        }
        if(current_token->type == END){
            return;
        }
        current_token = current_token->next_token;
    }
}

/*
This rather complicated function ensures the correct syntax for a function
The parameters and body are parsed recursively, since closed brackets are a terminator for parse_start
*/
ast* parse_function(ast *current_node){
    ast *new_node = create_node();
    new_node->type = FUNCTION;
    current_node->branch = new_node;

    switch_token(1);

    if(current_token->type != INDICATOR){
        diagnose d = {.line = current_token->line, .message = "Expected function name", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    // store function name any double definitions are handled later
    size_t size = strlen(current_token->value)+1;
    new_node->name = calloc(1, size);
    if(!new_node->name){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node name", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    strncpy(new_node->name, current_token->value, size);


    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        diagnose d = {.line = current_token->line, .message = "Expected '(' in function definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    // create subtree with all variables
    ast *temp_node = create_node();
    
    in_parameters = 1;
    parse_start(temp_node);
    in_parameters = 0;
    
    new_node->left = temp_node->branch;

    // we need to forward to the end of the function definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        diagnose d = {.line = current_token->line, .message = "Expected ')' in function definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        diagnose d = {.line = current_token->line, .message = "Expected '{' in function definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    temp_node->branch = NULL;
    temp_node->value = NULL;
    
    in_body = 1;
    parse_start(temp_node);
    in_body = 0;
    
    new_node->right = temp_node->branch;
    free(temp_node);

    // we need to forward to the end of the function body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        diagnose d = {.line = current_token->line, .message = "Expected '}' in function definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);
    return parse_start(new_node);
}

/*
This function parses both for and while loops
The loop body is parsed in the right branch and the condition is parsed in the left branch
*/
ast* parse_loop(ast *current_node){
    ast *new_node = create_node();
    new_node->type = LOOP;
    current_node->branch = new_node;

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        diagnose d = {.line = current_token->line, .message = "Expected '(' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    // create subtree with boolean logic
    ast *temp_node = create_node();
    
    // we use last_eol as an anchor for tree logic
    last_eol = temp_node;
    in_loop = 1;
    parse_start(temp_node);
    in_loop = 0;
    
    new_node->left = temp_node->branch;

    // we need to forward to the end of the conditional definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        diagnose d = {.line = current_token->line, .message = "Expected ')' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        diagnose d = {.line = current_token->line, .message = "Expected '{' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    temp_node->branch = NULL;
    temp_node->value = NULL;
    
    in_body = 1;
    parse_start(temp_node);
    in_body = 0;
    
    new_node->right = temp_node->branch;
    free(temp_node);

    // we need to forward to the end of the conditional body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        diagnose d = {.line = current_token->line, .message = "Expected '}' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);
    return parse_start(new_node);
}

/*
This function parses an if conditional
The if body is parsed in the right branch and the condition is parsed in the left branch
*/
ast* parse_if(ast *current_node){
    ast *new_node = create_node();
    new_node->type = CONDITIONAL;
    current_node->branch = new_node;

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        diagnose d = {.line = current_token->line, .message = "Expected '(' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    // create subtree with boolean logic
    ast *temp_node = create_node();
    
    // we use last_eol as an anchor for tree logic
    last_eol = temp_node;
    in_if = 1;
    parse_start(temp_node);
    in_if = 0;
    
    new_node->left = temp_node->branch;

    // we need to forward to the end of the conditional definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        diagnose d = {.line = current_token->line, .message = "Expected ')' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        diagnose d = {.line = current_token->line, .message = "Expected '{' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    temp_node->branch = NULL;
    temp_node->value = NULL;
    
    in_body = 1;
    parse_start(temp_node);
    in_body = 0;
    
    new_node->right = temp_node->branch;
    free(temp_node);

    // we need to forward to the end of the conditional body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }
        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        diagnose d = {.line = current_token->line, .message = "Expected '}' in conditional definition", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);
    return parse_start(new_node);
}

/*
this function parses an include and makes sure, that it has a proper file name
atm includes do not work at the lower levels but they get parsed for now
*/
ast* parse_include(ast *current_node){
    switch_token(1);

    if(current_token->type != INDICATOR){
        diagnose d = {.line = current_token->line, .message = "include doesn't link to a file", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    ast *new_node = create_node();
    new_node->type = INCLUDE;
    current_node->branch = new_node;
    
    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    strncpy(new_node->value, current_token->value, size);
    
    switch_token(1);

    if(current_token->type != DELIMITER){
        diagnose d = {.line = current_token->line, .message = "include doesn't link to a file", .type = ERROR};
        add_error_entry(d);
         return NULL;
    }
    
    switch_token(1);

    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "ql") != 0)){
        diagnose d = {.line = current_token->line, .message = "include doesn't link to a .ql file", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    switch_token(1);

    if(current_token->type != END_OF_LINE){
        diagnose d = {.line = current_token->line, .message = "No linebreak after include", .type = WARNING};
        add_error_entry(d);
    }
    return parse_start(new_node);
}

/*
this function determines if the given indicator token is a type
if it is a type, it checks if it is part of a variable declaration and handles that
*/
ast* parse_type(ast *current_node){
    enum variable_type type;

    if(current_token->type != INDICATOR){
        diagnose d = {.line = current_token->line, .message = "Variable type has to be an identifier", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    if(strcmp(current_token->value, "qubit") == 0){
        type = VAR_QUBIT;
    } else if(strcmp(current_token->value, "bit") == 0){
        type = VAR_BIT;
    } else if(strcmp(current_token->value, "int") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "uint") == 0){
        type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "double") == 0){
        type = VAR_DOUBLE;
    } else if(strcmp(current_token->value, "vector") == 0){
        type = VAR_VECTOR;
    } else if(strcmp(current_token->value, "void") == 0){
        type = VAR_VOID;
    } else {
        return NULL;
    }
    
    switch_token(1);
    
    if(current_token->type != INDICATOR) {
        ast *new_node = create_node();
        new_node->type = TYPE;
        new_node->resolved_type = type;
        current_node->branch = new_node;
        return parse_start(new_node);
    } else {
    // we assume this is a variable declaration
        ast *name_node = create_node();
        name_node->type = NAME;
        name_node->resolved_type = type;

        size_t size = strlen(current_token->value)+1;
        name_node->name = calloc(1, size);
        if(!name_node->name){
            diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node name", .type = FATAL};
            add_error_entry(d);
            return NULL;
        }
        strncpy(name_node->name, current_token->value, size);

        current_node->branch = name_node;
        switch_token(1);

        return parse_start(name_node);
    }
}

/*
this function parses the left and right side of an assign and rearranges the ast
the right branch is parsed recursively and the left branch is extracted from last_eol
*/
ast *parse_assign(ast *current_node){
    if(last_eol == NULL){
        diagnose d = {.line = current_token->line, .message = "Cannot parse assign", .type = ERROR};
        add_error_entry(d);
        return NULL;
    } else if(in_assign){
        diagnose d = {.line = current_token->line, .message = "Cannot call assign in another assign", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    // we use last_eol as an anchor point to append our new node and use the old nodes as left branch
    ast *left = last_eol->branch;
    ast *temp_node = create_node();

    ast *new_node = create_node();
    new_node->type = ASSIGN;
    new_node->value = malloc(1);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    *(new_node->value) = *(current_token->value);
    new_node->left = left;
    new_node->right = temp_node;

    last_eol->branch = new_node;
    last_eol = new_node;
    
    switch_token(1);
    
    in_assign = 1;
    parse_start(temp_node);
    in_assign = 0;
    
    new_node->right = temp_node->branch;
    free(temp_node);

    return parse_start(new_node);
}

/*
this parses binary operations these are only supported in the adaptive profile
*/
ast* parse_binop(ast *current_node){
    // binary operations are only supported in adaptive profile and in assigns.
    if(!adaptive){
        diagnose d = {.line = current_token->line, .message = "Binary operations are only supported in the adaptive profile", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    if(!in_assign){
        diagnose d = {.line = current_token->line, .message = "Operation outside of an assign are not supported", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    if(last_eol == NULL){
        diagnose d = {.line = current_token->line, .message = "Cannot parse operation", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    // we use last_eol as an anchor point to append our new node and use the old nodes as left branch
    // since we are in an assign we assume last_eol points to the last operation node.
    ast *left = last_eol->right->branch;
    ast *temp_node = create_node();

    ast *new_node = create_node();
    new_node->type = BINOP;
    new_node->value = malloc(1);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    *(new_node->value) = *(current_token->value);
    new_node->left = left;
    new_node->right = temp_node;

    last_eol->right->branch = new_node;
    last_eol = new_node;
    
    switch_token(1);
    
    parse_start(temp_node);
    
    new_node->right = temp_node->branch;
    free(temp_node);

    return parse_start(new_node);
}

/*
this function parses a boolean operation
*/
ast* parse_boolop(ast *current_node){
    if(!adaptive){
        diagnose d = {.line = current_token->line, .message = "Boolean operations are only supported in the adaptive profile", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    if(!in_if && !in_loop){
        diagnose d = {.line = current_token->line, .message = "Boolean operations are not supported in this context", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    if(last_eol == NULL){
        diagnose d = {.line = current_token->line, .message = "Cannot parse operation", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    // we use last_eol as an anchor point to append our new node and use the old nodes as left branch
    // since we are in an if conditional we assume last_eol points to the last operation node.
    
    ast *left;
    if(last_eol->type == ROOT){
        if(last_eol->branch == NULL){
            fprintf(stderr, "OHNOOOOOOOOOOOOOOOOOOOO\n");
        }
        left = last_eol->branch;
    } else if(last_eol->type == BOOLOP){
        left = last_eol->right->branch;
    } else {
        fprintf(stderr, "TODO!!!\n");
        // TODO
    }
    ast *temp_node = create_node();

    ast *new_node = create_node();
    new_node->type = BOOLOP;
    new_node->value = malloc(1);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    *(new_node->value) = *(current_token->value);
    new_node->left = left;
    new_node->right = temp_node;

    if(last_eol->type == ROOT){
        last_eol->branch = new_node;
    } else if(last_eol->type == BOOLOP){
        last_eol->right->branch = new_node;
    } else {
        fprintf(stderr, "TODO!!!\n");
        // TODO
    }
    last_eol = new_node;
    
    switch_token(1);
    
    parse_start(temp_node);
    
    new_node->right = temp_node->branch;
    free(temp_node);
    
    return parse_start(new_node);
}

/*
this parses operators, these can be operations as well as assigns
*/
ast* parse_operator(ast *current_node){
    diagnose d;
    switch(*(current_token->value)){
        case '=':
            if(current_token->next_token->type == OPERATOR){
                if(*(current_token->next_token->value) == *(current_token->value)){    
                    return parse_boolop(current_node);
                } else {
                    d = (diagnose){.line = current_token->next_token->line, .message = "Two different operators are not supported back to back", .type = ERROR};
                    add_error_entry(d);
                    return NULL;
                }
            }
            return parse_assign(current_node);
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
        case '%':
            return parse_binop(current_node);
        case '|':
        case '&':
        case '<':
        case '>':
        case '!':
            return parse_boolop(current_node);
        default:
            d = (diagnose){.line = current_token->line, .message = "This operation is currently not supported", .type = ERROR};
            add_error_entry(d);
            return NULL;
    }
}

/*
measure is not parsed as a normal function
since we can only measure one qubit we store the qubit name and LLVMValueRef in this node directly
*/
ast* parse_measure(ast *current_node){
    if(current_token->next_token->type != BRACKET_OPEN){
        diagnose d = {.line = current_token->line, .message = "Expected open bracket in measure call'", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    if(*(current_token->next_token->value) != '('){
        diagnose d = {.line = current_token->line, .message = "Expected '(' bracket", .type = WARNING};
        add_error_entry(d);
    }

    ast *new_node = create_node();
    new_node->type = MEASURE;

    current_node->branch = new_node;
    switch_token(2);

    if(current_token->type != INDICATOR){
        diagnose d = {.line = current_token->line, .message = "Expected qubit name to measure", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }

    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    strncpy(new_node->value, current_token->value, size);

    switch_token(1);

    if(current_token->type != BRACKET_CLOSE){
        diagnose d = {.line = current_token->line, .message = "Expected closed bracket in measure call", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    if(*(current_token->value) != ')'){
        diagnose d = {.line = current_token->line, .message = "Expected ')' bracket", .type = WARNING};
        add_error_entry(d);
    }

    switch_token(1);

    return parse_start(new_node);
}

/*
function that parses a function call. for now this can not parse custom functions
it parses parameters, which can only be identifiers and numbers
*/
ast* parse_call(ast *current_node){
    if(current_token->next_token->type != BRACKET_OPEN) return NULL;
    if(*(current_token->next_token->value) != '('){
        diagnose d = {.line = current_token->line, .message = "Expected '(' bracket", .type = WARNING};
        add_error_entry(d);
    }

    ast *new_node = create_node();
    new_node->type = CALL;

    size_t size = strlen(current_token->value)+1;
    new_node->name = calloc(1, size);
    if(!new_node->name){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node name", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    strncpy(new_node->name, current_token->value, size);

    current_node->branch = new_node;
    switch_token(2);

    ast *temp_node = create_node();
    new_node->left = temp_node;
    while(current_token != NULL){
        if(current_token->type == BRACKET_CLOSE){
            if(*(current_token->value) != ')'){
                diagnose d = {.line = current_token->line, .message = "Expected ')' bracket", .type = WARNING};
                add_error_entry(d);
            }
            switch_token(1);
            break;

        } else if(current_token->type == NUMBER){
            ast *number_node = create_node();
            number_node->type = VALUE;
            temp_node->branch = number_node;

            size_t number_size = strlen(current_token->value)+1;
            number_node->value = calloc(1, number_size);
            if(!new_node->value){
                diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
                add_error_entry(d);
                return NULL;
            }
            strncpy(number_node->value, current_token->value, number_size);

            switch_token(1);

            // seems as though we have encountered a double/float
            if(current_token->type == DELIMITER){
                switch_token(1);
                if(current_token->type == NUMBER){
                    number_node->value = realloc(number_node->value, size+strlen(current_token->value));
                    if(!number_node->value){
                        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
                        add_error_entry(d);
                        return NULL;
                    }
                    strcat(number_node->value, current_token->value);
                    switch_token(1);
                }
            }
            temp_node = number_node;
            continue;
            
        } else if(current_token->type == INDICATOR){
            ast *iden_node = create_node();
            iden_node->type = IDENTIFIER;

            size_t name_size = strlen(current_token->value)+1;
            iden_node->name = calloc(1, name_size);
            if(!iden_node->name){
                diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node name", .type = FATAL};
                add_error_entry(d);
                return NULL;
            }
            strncpy(iden_node->name, current_token->value, name_size);

            temp_node->branch = iden_node;
            switch_token(1);
            temp_node = iden_node;
            continue;

        } else if(current_token->type == DELIMITER){
            switch_token(1);
        } else if(current_token->type == END_OF_LINE){
            diagnose d = {.line = current_token->line, .message = "Missing ')' bracket", .type = ERROR};
            add_error_entry(d);
            return NULL;
        } else {
            diagnose d = {.line = current_token->line, .message = "Only variable names or numbers allowed in function call", .type = ERROR};
            add_error_entry(d);
            return NULL;
        }

    }
    if(new_node->left->branch != NULL){
        temp_node = new_node->left;
        new_node->left = new_node->left->branch;
        free(temp_node);
    }

    return parse_start(new_node);
}

/*
this function is the starting point for all string based commands
it determines what the string means and tries to send it to the apropiate function
*/
ast* parse_indicator(ast *current_node){
    if(current_token->value == NULL){
        diagnose d = {.line = current_token->line, .message = "parse_indicator() has no string to parse", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    // check if its a type
    ast *res = parse_type(current_node);
    if(res != NULL){
        return res;
    }
    
    // check if its a function definition
    if(strcmp(current_token->value, "def") == 0){
        return parse_function(current_node);
    }

    // check if its a loop
    if((strcmp(current_token->value, "for") == 0) || (strcmp(current_token->value, "while") == 0)){
        return parse_loop(current_node);
    }

    // check if its an if conditional 
    if(strcmp(current_token->value, "if") == 0){
        return parse_if(current_node);
    }
    
    // check if its a measure
    if((strcmp(current_token->value, "measure") == 0) || (strcmp(current_token->value, "MEASURE") == 0) || (strcmp(current_token->value, "MZ") == 0) || strcmp(current_token->value, "mz") == 0){
        return parse_measure(current_node);
    }
    
    // check if its an include
    if(strcmp(current_token->value, "include") == 0){
        return parse_include(current_node);
    }
    
    // check if its a return
    if(strcmp(current_token->value, "return") == 0){
        ast *new_node = create_node();
        new_node->type = RETURN;
        current_node->branch = new_node;
        
        switch_token(1);
        return parse_start(current_node);
    }
    
    // check if its a function call
    res = parse_call(current_node);
    if(res != NULL){
        return res;
    }

    // now we assume we are handling a variable reference aka an identifier
    ast *new_node = create_node();
    new_node->type = IDENTIFIER;
    
    size_t size = strlen(current_token->value)+1;
    new_node->name = calloc(1, size);
    if(!new_node->name){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node name", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    strncpy(new_node->name, current_token->value, size);

    current_node->branch = new_node;
    switch_token(1);

    return parse_start(new_node);
}

/*
this parses a number
since we use strings at this stage this is a very simple function
*/
ast* parse_number(ast *current_node){
    ast *new_node = create_node();
    new_node->type = VALUE;
    new_node->resolved_type = VAR_INTEGER;
    current_node->branch = new_node;

    size_t size = strlen(current_token->value)+1;
    new_node->value = calloc(1, size);
    if(!new_node->value){
        diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }

    strncpy(new_node->value, current_token->value, size);

    switch_token(1);

    // seems as though we have encountered a double/float
    if(current_token->type == DELIMITER){
        new_node->resolved_type = VAR_DOUBLE;
        switch_token(1);
        if(current_token->type == NUMBER){
            new_node->value = realloc(new_node->value, size+strlen(current_token->value));
            if(!new_node->value){
                diagnose d = {.line = current_token->line, .message = "Failed to allocate memory for node value", .type = FATAL};
                add_error_entry(d);
                return NULL;
            }
            strcat(new_node->value, current_token->value);
            switch_token(1);
        }
    }

    return parse_start(new_node);
}




/*
The start for our recursive decent parser
parse_start branches out to the other recursive functions
the END and BRACKET_CLOSE(under certain conditions) tokens are terminating
*/
ast* parse_start(ast *current_node){
    if(!current_token){
        return current_node;
    } else if(!current_node){
        return NULL;
    }

    diagnose d;
    switch(current_token->type){
        case INDICATOR:
            return parse_indicator(current_node);
        
        case NUMBER:
            return parse_number(current_node);

        case END_OF_LINE:
            last_eol = current_node;
            if(in_assign){
                return current_node;
            } else if(in_parameters){
                d = (diagnose){.line = current_token->line, .message = "Unrecognised symbol as a function parameter", .type = ERROR};
                add_error_entry(d);
                return NULL;
            }
            switch_token(1);
            return parse_start(current_node);
        
        case OPERATOR:
            if(in_parameters){
                d = (diagnose){.line = current_token->line, .message = "Unrecognised symbol as a function parameter", .type = ERROR};
                add_error_entry(d);
                return NULL;
            }
            return parse_operator(current_node);

        case START:
        case COMMENT:
            if(in_parameters){
                d = (diagnose){.line = current_token->line, .message = "Unrecognised symbol as a function parameter", .type = ERROR};
                add_error_entry(d);
                return NULL;
            }
            switch_token(1);
            return parse_start(current_node);
        
        case BRACKET_OPEN:
            if(in_assign){
                // TODO
            }
            d = (diagnose){.line = current_token->line, .message = "Open Bracket without context", .type = WARNING};
            add_error_entry(d);
            switch_token(1);
            return parse_start(current_node);

        case BRACKET_CLOSE:
            if(in_parameters || in_if || in_loop){
                if(*(current_token->value) == ')'){
                    return current_node;
                }
            } else if(in_body){
                if(*(current_token->value) == '}'){
                    return current_node;
                }
            }

            d = (diagnose){.line = current_token->line, .message = "Missing open bracket", .type = WARNING};
            add_error_entry(d);
            return NULL;
            
        case END:
            if(in_parameters){
                d = (diagnose){.line = current_token->line, .message = "Missing ')' in function parameters", .type = ERROR};
                add_error_entry(d);
                return NULL;
            } else if(in_body){
                d = (diagnose){.line = current_token->line, .message = "Missing '}' in function body", .type = ERROR};
                add_error_entry(d);
                return NULL;
            }
            return current_node;

            default:
            d = (diagnose){.line = current_token->line, .message = "Not recognised in this context", .type = ERROR};
            add_error_entry(d);
            if(in_parameters || in_body || in_assign){
                return NULL;
            }
        }
    return current_node;
}

/*
this is the access function for main
it resets the global variables
*/
ast* generate_ast(token *first_token){
    // initialisations
    in_body = 0;
    in_parameters = 0;
    in_assign = 0;

    current_token = first_token;
    ast *root = create_node();
    last_eol = root;

    if(parse_start(root) == NULL){
        diagnose d = {.line = current_token->line, .message = "parse_start() returned NULL", .type = ERROR};
        add_error_entry(d);
        return NULL;
    }
    check_errors();
    return root;
}