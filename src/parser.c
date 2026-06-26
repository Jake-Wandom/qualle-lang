#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int num_of_errors = 0;
static token *current_token;

ast* create_node(ast *current_node){
    ast *new_node = malloc(sizeof(ast));
    new_node->type = NOT_DET;
    new_node->branch = NULL;
    new_node->value = NULL;

    if(current_node){
        current_node->branch = new_node;
    } else {
        fprintf(stderr, "Unable to create new node\n");
        free(new_node);
        return NULL;
    }
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
            fprintf(stderr, "Null pointer while token switch\n");
            exit(1);
        }
        if(current_token->type == END){
            return;
        }
        current_token = current_token->next_token;
    }
}

void handle_error(token *error_token, enum error_type type, char *error_message){
    num_of_errors++;

    switch (type){
        case UNEXPECTED_ERROR:
            printf("UNEXPECTED ERROR: %s\n", error_message);
            break;
        case WRONG_TYPE_ERROR:
            printf("WRONG TYPE ERROR: %s\n", error_message);
            break;
        case UNKOWN_SYMBOL_ERROR:
            printf("UNKOWN SYMBOL ERROR: %s\n", error_message);
            break;
        case NO_CONTEXT_ERROR:
            printf("NO CONTEXT ERROR: %s\n", error_message);
            break;
        case UNKOWN_TYPE_ERROR:
            printf("MISSING VARIABLE ERROR: %s\n", error_message);
            break;
        default:
            printf("ERROR: %s\n", error_message);
    }
    // locate position
    printf("line %i ", error_token->line);
    if((error_token->type == INDICATOR) || (error_token->type == NUMBER)){
        printf("->%s<-\n\n", error_token->value);
    } else if((error_token->type != START) && (error_token->type != END)){
        printf("->%c<-\n\n", *(error_token->value));
    }
}

ast* parse_function(ast *current_node){
    current_node = create_node(current_node);
    current_node->type = FUNC_DEF;
    current_node->func.name = NULL;
    current_node->func.variables = NULL;

    switch_token(1);

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected function name");
        return NULL;
    }

    // store function name any double definitions are handled later
    size_t size = strlen(current_token->value)+1;
    current_node->func.name = malloc(size);
    strncpy(current_node->func.name, current_token->value, size);


    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '(' in function definition");
        return NULL;
    }

    switch_token(1);

    // create subtree with all variables
    ast *new_node = malloc(sizeof(ast));
    new_node->type = ROOT;
    new_node->branch = NULL;
    current_node->func.variables = new_node;

    parse_start(new_node);
    
    // we need to forward to the end of the function definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }

        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ')' in function definition");
        return NULL;
    }

    switch_token(1);

    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '{' in function definition");
        return NULL;
    }

    switch_token(1);

    ast *second_node = malloc(sizeof(ast));
    second_node->type = ROOT;
    second_node->branch = NULL;
    current_node->func.body = second_node;

    parse_start(second_node);

    // we need to forward to the end of the function body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }

        switch_token(1);
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '}' after function");
        return NULL;
    }

    switch_token(1);
    return parse_start(current_node);
}

ast* parse_include(ast *current_node){
    switch_token(1);

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
    }
    current_node = create_node(current_node);
    current_node->type = INCLUDE;
    
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);
    
    switch_token(1);

    if(current_token->type != DELIMITER){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
        return NULL;
    }
    
    switch_token(1);

    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "ql") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a .ql file");
        return NULL;
    }

    switch_token(1);

    if(current_token->type != END_OF_LINE){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ';' or linebreak at the end of include");
        return NULL;
    }
    return parse_start(current_node);
}

ast* parse_type(ast *current_node){
    enum variable_type type;

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected variable type");
        return NULL;
    }

    if(strcmp(current_token->value, "qbit") == 0){
        type = VAR_QBIT;
    } else if(strcmp(current_token->value, "bit") == 0){
        type = VAR_BIT;
    } else if(strcmp(current_token->value, "N") == 0){
        type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "Z") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "int") == 0){
        type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "uint") == 0){
        type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "double") == 0){
        type = VAR_DOUBLE;
    } else if(strcmp(current_token->value, "vector") == 0){
        type = VAR_VECTOR;
    } else {
        //handle_error(current_token, UNKOWN_TYPE_ERROR, "Unkown type");
        return NULL;
    }

    
    current_node = create_node(current_node);
    current_node->type = TYPE;
    current_node->var_type = type;

    switch_token(1);

    return parse_start(current_node);
}


ast* parse_indicator(ast *current_node){
    current_node = create_node(current_node);
    current_node->type = NAME;
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);

    switch_token(1);

    if((current_token->type == BRACKET_OPEN) && (*(current_token->value) == '(')){

        switch_token(1);
        current_node = parse_start(current_node);
    
        // we need to forward to the end of the function definition
        while(current_token != NULL){
            if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
                break;
            }

            switch_token(1);
        }   

        if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
            handle_error(current_token, UNEXPECTED_ERROR, "Expected ')' in function reference");
            return NULL;
        }
        switch_token(1);
    }

    return parse_start(current_node);
}

ast* parse_number(ast *current_node){
    current_node = create_node(current_node);
    current_node->type = VALUE;
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);

    switch_token(1);

    return parse_start(current_node);
}



ast* parse_start(ast *current_node){
    if(!current_token){
        return current_node;
    } else if(!current_node){
        return NULL;
    }

    switch(current_token->type){
        case INDICATOR:
            if(strcmp(current_token->value, "def") == 0){
                // go to function state with quantum
                return parse_function(current_node);
            } else if(strcmp(current_token->value, "include") == 0){
                // go to include state
                return parse_include(current_node);
            } else {
                // check if this is a variable definition
                ast *new_node = parse_type(current_node);
                if(new_node != NULL){
                    return new_node;
                }
                // go to indicator state that decides what this indicator is
                return parse_indicator(current_node);
            }
            break;
        
        case NUMBER:
            return parse_number(current_node);
            break;
        
        case OPERATOR:
            switch_token(1);
            return current_node;
            break;

        case START:
        case DELIMITER:
        case COMMENT:
        case END_OF_LINE:
            switch_token(1);
            return parse_start(current_node);
            break;

        case BRACKET_CLOSE:
            return current_node;
            break;
        
        case END:
            return current_node;
            break;
        
        default:
            handle_error(current_token, 0, "Not recognised in this context");
    }
    return current_node;
}

ast* generate_ast(token *first_token){
    ast *root = malloc(sizeof(ast));
    root->type = ROOT;
    root->branch = NULL;

    num_of_errors = 0;
    current_token = first_token;

    ast *current_node = parse_start(root);
    if(current_node == NULL){
        fprintf(stderr, "handle_input returned NULL\n");
    }
    return root;
}