#include "helper.h"
#include <stdlib.h>

void print_man_page(void){
    printf("quallcom does not have a man page yet T_T\n");
}

void zero_buffer(char* buffer, size_t size){
    for(size_t i = 0; i < size; i++){
        buffer[i] = 0;
    }
}

static int current_indentation = 1;

void printprefix(int level) {
    for (int i = 0; i < level - 1; ++i)
        printf("    ");
    
    if (level > 0)
        printf(" -> ");
}

void print_ast(ast *root, int level){
    if (root == NULL) return;
    // print current level
    printprefix(level);
    switch(root->type){
        case ROOT:
        printf("ROOT");
        break;
        case VAR_DEF:
        printf("VAR ");
        break;
        case VAR_REF:
        printf("REF ");
        break;
        case VALUE:
        printf("VAL ");
        break;
        case ASSIGN_VAL:
        printf("ASGN");
        break;
        case BIN_OP:
        printf("BIN ");
        break;
        case BOOL_OP:
        printf("BOOL");
        break;
        case LOOP:
        printf("LOOP");
        break;
        case INCLUDE:
        printf("INCL");
        break;
        case FUNC_DEF:
        printf("FUNC");
        break;
        case RETURN:
        printf("RET ");
        break;
        case NOT_DET:
        printf("UN  ");
        default:
    }
    
    print_ast(root->branch, 1);
    
    // recurse sub-tree
    switch(root->type){
        case ASSIGN_VAL:
            current_indentation++;
            printf("\n    ");
            print_ast(root->assignment.assignee, current_indentation);
            printf("\n    ");
            print_ast(root->assignment.assignor, current_indentation);
            break;
        case FUNC_DEF:
            current_indentation++;
            printf("\n    ");
            print_ast(root->func.variables, current_indentation);
            printf("\n    ");
            print_ast(root->func.body, current_indentation);
            break;
        case BIN_OP:
            current_indentation++;
            printf("\n    ");
            print_ast(root->operation.operator_a, current_indentation);
            printf("\n    ");
            print_ast(root->operation.operator_b, current_indentation);
            break;
        case BOOL_OP:
            current_indentation++;
            printf("\n    ");
            print_ast(root->condition.operator_a, current_indentation);
            printf("\n    ");
            print_ast(root->condition.operator_b, current_indentation);
            break;
        default:
    }
}

void print_token_list(token* first_token){
    while(first_token != NULL){
        switch(first_token->type){
            case INDICATOR:
                printf("[IND %s]", first_token->value);
                break;
            
            case NUMBER:
                printf("[NUM %s]", first_token->value);
                break;
            
            case END_OF_LINE:
                printf("[EOL %c]\n", *(first_token->value));
                break;

            case DELIMITER:
                printf("[DEL %c]", *(first_token->value));
                break;

            case COMMENT:
                printf("[COM %c]", *(first_token->value));
                break;

            case BRACKET_CLOSE:
                printf("[BC %c]", *(first_token->value));
                break;

            case BRACKET_OPEN:
                printf("[BO %c]", *(first_token->value));
                break;

            case OPERATOR:
                printf("[OP %c]", *(first_token->value));
                break;

            case START:
                printf("\n[START]\n");
                break;

            case END:
                if(first_token->prev_token->type != END_OF_LINE){
                    printf("\n");
                }
                printf("[END]\n\n");
                break;
            
            case UNKOWN:
                if(first_token->value == NULL) printf("[UN]");
                else printf("[UN %c]", *(first_token->value));
                break;
            
            default:
                fprintf(stderr, "Unkown token type %i", first_token->type);
        }
        first_token = first_token->next_token;
    }
}

void free_token_list(token* first_token){
    while(first_token != NULL){
        free(first_token->value);
        if(first_token->next_token == NULL){
            free(first_token);
            break;
        }
        first_token = first_token->next_token;
        free(first_token->prev_token);
    }
}

void free_ast(ast *root){
    ast *current_node = root;
    if(!current_node){
        return;
    }

    switch(current_node->type){
        case VAR_DEF:
            free(current_node->var.name);
            break;
        case VAR_REF:
            free(current_node->value);
            break;
        case VALUE:
            free(current_node->value);
            break;
        case ASSIGN_VAL:
            free_ast(current_node->assignment.assignee);
            free_ast(current_node->assignment.assignor);
            break;
        case FUNC_DEF:
            free(current_node->func.name);
            free_ast(current_node->func.body);
            free_ast(current_node->func.variables);
            break;
        case IF:
            free_ast(current_node->iff.condition);
            free_ast(current_node->iff.if_body);
            free_ast(current_node->iff.else_body);
            break;
        case BIN_OP:
            free_ast(current_node->operation.operator_a);
            free_ast(current_node->operation.operator_b);
            break;
        case BOOL_OP:
            free_ast(current_node->condition.operator_a);
            free_ast(current_node->condition.operator_b);
            break;
        case INCLUDE:
            free(current_node->value);
        default:
    }
    root = current_node->branch;
    free(current_node);
    if(root != NULL){
        free_ast(root);
    }
}
