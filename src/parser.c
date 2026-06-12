#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ast* create_node(ast* current_node){
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

ast* check_ind_context(ast *current_node, token *current_token){
    switch(current_node->type){
        case ROOT:
        case VAR_DEF:
        case CONDITIONAL:
        case CONDITION:
        case LOOP:
        case FUNC_DEF:

        case RETURN:
            break;
        case MAIN:
            break;
        // this should not happen
        case NOT_DET:
            break;
        default:
    }
}

ast* establish_context(ast *current_node, token *current_token){
    if(current_node->type != NOT_DET){
        fprintf(stderr, "unable to establish context, node already possesses it\n");
        return NULL;
    } else if(current_token->value == NULL){
        fprintf(stderr, "unable to establish context, token has no value\n");
        return NULL;
    }

    if(strcmp(current_token->value, "q") == 0){
        // since we have no context, we interpret q as the start of a quantum function
        // we are now walking forward in the  token list to check if this is the case
        if((current_token->next_token->type == DELIMITER) && (current_token->next_token->next_token->type == INDICATOR)){
            if((strcmp(current_token->next_token->value, ".") == 0) && (strcmp(current_token->next_token->next_token->value, "def") == 0)){
                current_node->type = FUNC_DEF;
                current_node->func.type = QUANTUM;
            }
        }

    } else if(strcmp(current_token->value, "c" == 0)){

    } else if(strcmp(current_token->value, "if") == 0){

    } else if(strcmp(current_token->value, "else") == 0){

    } else if(strcmp(current_token->value, "for") == 0){

    } else if(strcmp(current_token->value, "while") == 0){

    } else if(strcmp(current_token->value, "return") == 0){

    } else if(strcmp(current_token->value, "main") == 0){

    } else if(strcmp(current_token->value, "qbit") == 0){
        
    }
}

ast* evaluate_token(ast *current_node, token *current_token){
    if((current_token == NULL) || (current_node == NULL)){
        fprintf(stderr, "check_node has not recieved a valid token");
        return NULL;
    }

    switch (current_token->type){
            case INDICATOR:
                if(current_node->type != NOT_DET){

                }
                break;
            case NUMBER:
                break;
            case OPERATOR:
                break;
            case COMMENT:
                break;
            case WHITESPACE:
                break;
            case BRACKET_OPEN:
                break;
            case BRACKET_CLOSE:
                break;
            case DELIMITER:
                break;
            case END_OF_LINE:
                break;
            case END:
                break;
            case UNKOWN:
                break;
            default:
                fprintf(stderr, "unkown token type\n");
        }
}

ast* generate_ast(token *first_token){
    ast *root = malloc(sizeof(ast));
    root->type = ROOT;
    root->branch = NULL;

    token *current_token = first_token;
    ast *current_node = root;
    while(current_token != NULL){
        current_node = check_node(current_node, current_token);
    }
    return root;
}