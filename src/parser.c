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
        fprintf(stderr, "Unable to create new node\n");
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

ast* parse_function(bool quantum, ast *current_node, token *current_token){
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
        handle_error(current_token, UNEXPECTED_ERROR, "Expected function name");
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

    ast *var_node = parse_start(new_node, current_token);
    
    // we need to forward to the end of the function definition
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == ')')){
            break;
        }

        current_token = current_token->next_token;
        if(!current_token){
            fprintf(stderr, "Null pointer\n");
            return NULL;
        }
    }   

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

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }

    ast *second_node = malloc(sizeof(ast));
    second_node->type = ROOT;
    second_node->branch = NULL;
    current_node->func.body = second_node;

    ast *body_node = parse_start(second_node, current_token);

    // we need to forward to the end of the function body
    while(current_token != NULL){
        if((current_token->type == BRACKET_CLOSE) && (*(current_token->value) == '}')){
            break;
        }

        current_token = current_token->next_token;
        if(!current_token){
            fprintf(stderr, "Null pointer\n");
            return NULL;
        }
    }   

    if((current_token->type != BRACKET_CLOSE) || (*(current_token->value) != '}')){
        printf("CURRENT TOKEN %i, FIRST CHAR %c\n", current_token->type, *(current_token->value));
        handle_error(current_token, UNEXPECTED_ERROR, "Expected '}' after function");
        return NULL;
    }

    current_token = current_token->next_token;
    return parse_start(current_node, current_token);
}

ast* parse_include(ast *current_node, token *current_token){
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type != INDICATOR){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
    }
    current_node = create_node(current_node);
    current_node->type = INCLUDE;
    
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);
    
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if(current_token->type != DELIMITER){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a file");
        return NULL;
    }
    
    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    if((current_token->type != INDICATOR) || (strcmp(current_token->value, "ql") != 0)){
        handle_error(current_token, UNEXPECTED_ERROR, "include needs to link to a .ql file");
        return NULL;
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
    return parse_start(current_node, current_token);
}

ast* parse_variable(ast *current_node, token *current_token){
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
        //handle_error(current_token, UNKOWN_TYPE_ERROR, "Unkown type");
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

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }

    return parse_start(current_node, current_token);
}


ast* parse_indicator(ast *current_node, token *current_token){
    current_node = create_node(current_node);
    current_node->type = VAR_REF;
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    return parse_start(current_node, current_token);
}

ast* parse_number(ast *current_node, token *current_token){
    current_node = create_node(current_node);
    current_node->type = VALUE;
    size_t size = strlen(current_token->value)+1;
    current_node->value = malloc(size);
    strncpy(current_node->value, current_token->value, size);

    current_token = current_token->next_token;
    if(!current_token){
        fprintf(stderr, "Null pointer\n");
        return NULL;
    }
    return parse_start(current_node, current_token);
}

ast* parse_loop(bool for_loop, ast *current_nodee, token *current_token){
}

ast* parse_if(ast *current_node, token *current_token){
}


ast* parse_start(ast *current_node, token *current_token){
    if(!current_token){
        return current_node;
    } else if(!current_node){
        return NULL;
    }

    switch(current_token->type){
        case INDICATOR:
            if(strcmp(current_token->value, "q") == 0){
                // go to function state with quantum
                return parse_function(1, current_node, current_token);
            } else if(strcmp(current_token->value, "c") == 0){
                // go to function state with classical
                return parse_function(0, current_node, current_token);
            } else if(strcmp(current_token->value, "include") == 0){
                // go to include state
                return parse_include(current_node, current_token);
            } else if(strcmp(current_token->value, "while") == 0){
                // go to loop state
                return parse_loop(0, current_node, current_token);
            } else if(strcmp(current_token->value, "for") == 0){
                // go to loop state
                return parse_loop(1, current_node, current_token);
            } else if(strcmp(current_token->value, "for") == 0){
                // go to if state
                return parse_if(current_node, current_token);
            } else {
                // check if this is a variable definition
                ast *new_node = parse_variable(current_node, current_token);
                if(new_node != NULL){
                    return new_node;
                }
                // go to indicator state that decides what this indicator is
                return parse_indicator(current_node, current_token);
            }
            break;
        
        case NUMBER:
            return parse_number(current_node, current_token);
            break;
        
        case OPERATOR:
            break;

        case START:
        case DELIMITER:
        case COMMENT:
        case END_OF_LINE:
            current_token = current_token->next_token;
            if(!current_token){
                fprintf(stderr, "Null pointer\n");
                return NULL;
            }
            return parse_start(current_node, current_token);
            break;

        case BRACKET_CLOSE:
            /*if(*(current_token->value) == '}'){
                return current_node;
            }
            current_token = current_token->next_token;
            if(!current_token){
                fprintf(stderr, "Null pointer\n");
                return NULL;
            }
            return parse_start(current_node, current_token);*/
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

    num_of_errors = 1;

    ast *current_node = parse_start(root, first_token);
    if(current_node == NULL){
        fprintf(stderr, "handle_input returned NULL\n");
    }
    return root;
}