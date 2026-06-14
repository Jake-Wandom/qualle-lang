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
            printf("UNEXPECTED ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
            break;
        case WRONG_TYPE_ERROR:
            printf("WRONG TYPE ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
            break;
        case UNKOWN_SYMBOL_ERROR:
            printf("UNKOWN SYMBOL ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
            break;
        case NO_CONTEXT_ERROR:
            printf("NO CONTEXT ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
            break;
        case UNKOWN_TYPE_ERROR:
            printf("MISSING VARIABLE ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
            break;
        default:
            printf("ERROR: %s\n", error_message);
            printf("line %i\n\n", error_token->line);
    }
    // locate position

}

variable* handle_variable(ast *current_node, token *current_token){
    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected variable type or name");
        return NULL;
    }

    variable *var = malloc(sizeof(variable));

    if(strcmp(current_token->value, "qbit") == 0){
        var->type = VAR_QBIT;
    } else if(strcmp(current_token->value, "bit") == 0){
        var->type = VAR_BIT;
    } else if(strcmp(current_token->value, "N") == 0){
        var->type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "Z") == 0){
        var->type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "Q") == 0){
        var->type = VAR_RATIONAL;
    } else if(strcmp(current_token->value, "R") == 0){
        var->type = VAR_IRRATIONAL;
    } else if(strcmp(current_token->value, "C") == 0){
        var->type = VAR_COMPLEX;
    } else if(strcmp(current_token->value, "vector") == 0){
        var->type = VAR_VECTOR;
    } else {
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type == INDICATOR) && (current_token->value != NULL)){
        var->name = malloc(strlen(current_token->value)*sizeof(char));
        var->name = current_token->value;
    } else {
        return NULL;
    }
}

ast* handle_function(bool quantum, ast *current_node, token *current_token){
    current_node = create_node(current_node);
    current_node->type = FUNC_DEF;
    current_node->func.quantum = quantum;

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != DELIMITER) || (strcmp(current_token->value, ".") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '.' in function definition");
        return NULL;
    }
    
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "def") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected 'def' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Excpected function name");
        return NULL;
    }

    // store function name any double definitions are handled later
    size_t size = strlen(current_token->value);
    current_node->func.name = malloc(size*sizeof(char));
    strncpy(current_node->func.name, current_token->value, size);


    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != BRACKET_OPEN) || (strcmp(current_token->value, "(") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '(' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type == INDICATOR){
        
    } else if((current_token->type == BRACKET_CLOSE) && (strcmp(current_token->value, ")") == 0)){
        
    }
}

ast* handle_include(ast *current_node, token *current_token){
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type == INDICATOR){
        size_t size = strlen(current_token->value);
        current_node = create_node(current_node);

        current_node->type = INCLUDE;
        current_node->file_path = malloc(size*sizeof(char));
        strncpy(current_node->file_path, current_token->value, size);
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type != END_OF_LINE){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ';' or ' ' at the end of include");
        return NULL;
    }
    return current_node;
}

ast* handle_indicator(ast *current_node, token *current_token){
    variable *var = handle_variable(current_node, current_token);
    if(var){
        current_node = create_node(current_node);
        current_node->var.type = var->type;
        current_node->var.name = malloc(strlen(var->name)*sizeof(char));
        
    }
}

ast* handle_loop(bool for_loop, ast *current_nodee, token *current_token){
    if(for_loop){

    } else {

    }
}


ast* handle_input(ast *current_node, token *current_token){
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
            } else if(strcmp(current_token->value, "while") == 0){
                // go to loop state
                return handle_loop(0, current_node, current_token);
            } else if(strcmp(current_token->value, "for") == 0){
                // go to loop state
                return handle_loop(1, current_node, current_token);
            } else {
                // go to indicator state that decides what this indicator is
                return handle_indicator(current_node, current_token);
            }
            break;

        case COMMENT:
        case END_OF_LINE:
            current_token = current_token->next_token;
            return handle_input(current_node, current_token);
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

    num_of_errors = 0;
    token *current_token = first_token;
    ast *current_node = root;
    while(current_token != NULL){
        current_node = handle_input(current_node, current_token);
        if(current_node == NULL){
            fprintf(stderr, "handle_input returned NULL");
            break;
        }
        current_token = current_token->next_token;
    }
    return root;
}