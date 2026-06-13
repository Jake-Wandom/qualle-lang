#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int num_of_errors = 0;

ast* create_node(ast *current_node){
    ast *new_node = malloc(sizeof(ast));
    new_node->type = NOT_DET;
    new_node->branch = NULL;

    if(current_node){
        current_node->branch = new_node;
    } else {
        fprintf(stderr, "unable to create new node\n");
        free(new_node);
        return NULL;
    }
    return new_node;
}

void handle_error(token *error_token, enum error_type type, char *error_message){
    num_of_errors++;
    token *current_token = error_token;

    switch (type){
        case UNEXPECTED_ERROR:
        case WRONG_TYPE_ERROR:
        case UNKOWN_SYMBOL_ERROR:
        case NO_CONTEXT_ERROR:
        case NO_CLONING_ERROR:
        case NO_CONSUMPTION_ERROR:
        case MISSING_VARIABLE_ERROR:
        case NAME_CONFLICT_ERROR:
    }
    printf("ERROR: %s\n", error_message);
    printf("line %i\n\n", error_token->line);
    // locate position

}

ast* handle_function(bool quantum, ast *current_node, token *current_token){
    ast *new_node = create_node(current_node);
    new_node->type = FUNC_DEF;
    new_node->func.quantum = quantum;

    current_token = current_token->next_token;
    if(!current_token) fprintf(stderr, "Null pointer\n");

    if((current_token->type != DELIMITER) || (strcmp(current_token->value, ".") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '.' in function definition");
        return NULL;
    }
    
    current_token = current_token->next_token;
    if(!current_token) fprintf(stderr, "Null pointer\n");

    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "def") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected 'def' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token) fprintf(stderr, "Null pointer\n");

    if((current_token->type != BRACKET_OPEN) || (strcmp(current_token->value, "(") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '(' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token) fprintf(stderr, "Null pointer\n");

    if(current_token->type == INDICATOR){
        
    } else if((current_token->type == BRACKET_CLOSE) && (strcmp(current_token->value, ")") == 0)){
        
    }
}

ast* handle_include(ast *current_node, token *current_token){

}

ast* handle_main(ast *current_node, token *current_token){

}

ast* handle_indicator(ast *current_node, token *current_token){
    
}


ast* start(ast *current_node, token *current_token){
    switch(current_token->type){
        case INDICATOR:
            if(strcmp(current_token->value, "q") == 0){
                // go to function state with quantum
                return handle_function(1, current_node, current_token);
            } else if(strcmp(current_token->value, "c") == 0){
                // go to function state with classical
                return handle_function(0, current_node, current_token);
            } else if(strcmp(current_token->value, "include") == 0){
                // go to include state
                return handle_include(current_node, current_token);
            } else if(strcmp(current_token->value, "main") == 0){
                // go to main state
                return handle_main(current_node, current_token);
            } else {
                // go to indicator state that decides what this indicator is
                return handle_indicator(current_node, current_token);
            }
            break;

        case COMMENT:
            while(current_token->type != END_OF_LINE){
                current_token = current_token->next_token;
            }
            return start(current_node, current_token);
            break;

        case WHITESPACE:
        case END_OF_LINE:
            current_token = current_token->next_token;
            return start(current_node, current_token);
            break;
            
        case END:
            return NULL;
            break;

        default:
            handle_error(current_token, 0, "Not recognised in this context");
    }
}

ast* generate_ast(token *first_token){
    ast *root = malloc(sizeof(ast));
    root->type = ROOT;
    root->branch = NULL;

    token *current_token = first_token;
    ast *current_node = root;
    while(current_token != NULL){
    }
    return root;
}