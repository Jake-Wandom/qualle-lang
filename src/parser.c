#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int num_of_errors = 0;

ast* create_node(ast *current_node){
    ast *new_node = malloc(sizeof(ast));
    new_node->type = NOT_DET;
    new_node->branch = NULL;
    new_node->value = NULL;

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

ast* handle_variable(ast *current_node, token *current_token){
    variable var;
    var.name = NULL;
    var.type = VAR_UNKOWN;

    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected variable type or name");
        return NULL;
    }


    if(strcmp(current_token->value, "qbit") == 0){
        var.type = VAR_QBIT;
    } else if(strcmp(current_token->value, "bit") == 0){
        var.type = VAR_BIT;
    } else if(strcmp(current_token->value, "N") == 0){
        var.type = VAR_NATURAL;
    } else if(strcmp(current_token->value, "Z") == 0){
        var.type = VAR_INTEGER;
    } else if(strcmp(current_token->value, "Q") == 0){
        var.type = VAR_RATIONAL;
    } else if(strcmp(current_token->value, "R") == 0){
        var.type = VAR_IRRATIONAL;
    } else if(strcmp(current_token->value, "C") == 0){
        var.type = VAR_COMPLEX;
    } else if(strcmp(current_token->value, "vector") == 0){
        var.type = VAR_VECTOR;
    } else {
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type == INDICATOR) && (current_token->value != NULL)){
        size_t size = strlen(current_token->value)+1;
        var.name = malloc(size);
        strcpy(var.name, current_token->value);
    }
    current_node = create_node(current_node);
    current_node->type = VAR_DEF;
    current_node->var = var;
    return current_node;
}

ast* handle_function(bool quantum, ast *current_node, token *current_token){
    current_node = create_node(current_node);
    current_node->type = FUNC_DEF;
    current_node->func.quantum = quantum;
    current_node->func.name = NULL;
    current_node->func.variables = NULL;

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != DELIMITER) || (*(current_token->value) != '.')){
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
    size_t size = strlen(current_token->value)+1;
    current_node->func.name = malloc(size);
    strncpy(current_node->func.name, current_token->value, size);


    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '(')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '(' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    // create subtree with all variables
    ast *new_node = malloc(sizeof(ast));
    new_node->type = ROOT;
    new_node->branch = NULL;
    current_node->func.variables = new_node;

    current_node = handle_input(new_node, current_token);
    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ')' in function definition");
        return NULL;
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != BRACKET_OPEN) || (*(current_token->value) != '{')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '{' in function definition");
        return NULL;
    }

    ast *second_node = malloc(sizeof(ast));
    second_node->type = ROOT;
    second_node->branch = NULL;
    current_node->func.body = second_node;

    current_node = handle_input(second_node, current_token);
    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != ')')){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '}' after function");
        return NULL;
    }

    current_token = current_token->next_token;
    return handle_input(current_node, current_token);
}

ast* handle_include(ast *current_node, token *current_token){
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type == INDICATOR){
        size_t size = strlen(current_token->value)+1;
        current_node = create_node(current_node);

        current_node->type = INCLUDE;
        current_node->value = malloc(size);
        strncpy(current_node->value, current_token->value, size);
    }

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type != END_OF_LINE){
        handle_error(current_token, UNEXPECTED_ERROR, "Expected ';' or linebreak at the end of include");
        return NULL;
    }
    return handle_input(current_node, current_token);
}

ast* handle_indicator(ast *current_node, token *current_token){
    if(handle_variable(current_node, current_token) != NULL){
        current_node = handle_variable(current_node, current_token);
        current_token = current_token->next_token;
        if(!current_token){
            fprintf(stderr, "Null pointer\n");
            return NULL;
        }
        current_token = current_token->next_token;
        if(!current_token){
            fprintf(stderr, "Null pointer\n");
            return NULL;
        }
    }
    return handle_input(current_node, current_token);
}

ast* handle_loop(bool for_loop, ast *current_nodee, token *current_token){
    if(for_loop){

    } else {

    }
}


ast* handle_input(ast *current_node, token *current_token){
    if(!current_token){
        return current_node;
    } else if(!current_node){
        return NULL;
    }

    switch(current_token->type){
        case INDICATOR:
            printf("HANDLE_INPUT detects IND\n");
            if(strcmp(current_token->value, "q") == 0){
                // go to function state with quantum
                current_node = handle_function(1, current_node, current_token);
            } else if(strcmp(current_token->value, "c") == 0){
                // go to function state with classical
                current_node = handle_function(0, current_node, current_token);
            } else if(strcmp(current_token->value, "include") == 0){
                // go to include state
                current_node = handle_include(current_node, current_token);
            } else if(strcmp(current_token->value, "while") == 0){
                // go to loop state
                current_node = handle_loop(0, current_node, current_token);
            } else if(strcmp(current_token->value, "for") == 0){
                // go to loop state
                current_node = handle_loop(1, current_node, current_token);
            } else {
                // go to indicator state that decides what this indicator is
                current_node = handle_indicator(current_node, current_token);
            }
            break;
        
        case NUMBER:
            printf("HANDLE_INPUT detects NUM\n");
            break;

        case START:
        case DELIMITER:
        case COMMENT:
        case END_OF_LINE:
            printf("HANDLE_INPUT detects EOL, etc.\n");
            current_token = current_token->next_token;
            if(!current_token){
                fprintf(stderr, "Null pointer\n");
                return NULL;
            }
            return handle_input(current_node, current_token);
            break;

        case BRACKET_CLOSE:
            printf("HANDLE_INPUT detects BC\n");
            return current_node;
            break;
        
        case END:
            printf("HANDLE_INPUT detects END\n");
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

    ast *current_node = handle_input(root, first_token);
    if(current_node == NULL){
        fprintf(stderr, "handle_input returned NULL");
    }
    return root;
}