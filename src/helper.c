#include "helper.h"
#include <stdlib.h>

void print_man_page(void){
    printf("quallcom [FLAGS] [FILE] ... [FILE]\n\nFLAGS:\n  -h or --help: print this page\n  -p: print information like file content, token lists and abstract syntax tree\n  -o: activate optimisations\n  -l: generates a readable .ll file instead of bitcode");
}

void zero_buffer(char* buffer, size_t size){
    for(size_t i = 0; i < size; i++){
        buffer[i] = 0;
    }
}

void printprefix(int level) {
    for (int i = 0; i < level - 1; ++i)
        printf("|  ");
}

void print_ast(ast *root, int level){
    if (root == NULL) return;
    // print current level
    if((level > 1) && (root->type != ROOT)) printprefix(level);
    switch(root->type){
        case ROOT:
            if(level == 1){
                printf("├─> ROOT\n");
            }
            break;
        case TYPE:
            printf("├── TYPE: %i\n", root->var_type);
            break;
        case NAME:
            printf("├── NAME: %s\n",root->value);
            break;
        case VALUE:
        printf("├── VALUE: %s\n", root->value);
        break;
        case ASSIGN:
        printf("├── ASSIGN: %c\n", root->value);
        break;
        case INCLUDE:
        printf("├── INCLUDE: %s\n", root->value);
        break;
        case FUNC_DEF:
        printf("├── FUNCTION: %s\n", root->func.name);
        break;
        case RETURN:
        printf("├── RETURN\n");
        break;
        case NOT_DET:
        printf("├── UNKNOWN\n");
        default:
            break;
    }
    
    
    // recurse sub-tree
    switch(root->type){
        case ASSIGN:
        printprefix(level+1);
        printf("├─> Assignee:\n");
        print_ast(root->assignment.assignee, level+1);
        printprefix(level+1);
        printf("├─> Assignor:\n");
        print_ast(root->assignment.assignor, level+1);
        break;
        case FUNC_DEF:
        printprefix(level+1);
        printf("├─> Parameters:\n");
        print_ast(root->func.variables, level+1);
        printprefix(level+1);
        printf("├─> Body:\n");
        print_ast(root->func.body, level+1);
        break;
        default:
        break;
    }

    print_ast(root->branch, level);
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
        case INCLUDE:
        case VALUE:
        case NAME:
            free(current_node->value);
            break;
        case ASSIGN:
            free_ast(current_node->assignment.assignee);
            free_ast(current_node->assignment.assignor);
            free(current_node->value);
            break;
        case FUNC_DEF:
            free(current_node->func.name);
            free_ast(current_node->func.body);
            free_ast(current_node->func.variables);
            break;
        default:
            break;
    }
    root = current_node->branch;
    free(current_node);
    if(root != NULL){
        free_ast(root);
    }
}
