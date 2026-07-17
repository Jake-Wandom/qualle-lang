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
            printf("├─> ROOT\n");
            break;
        case TYPE:
            char *str = "default";
            switch(root->var_type){
                case VAR_QUBIT:
                    str = "qubit";
                    break;
                case VAR_BIT:
                    str = "bit";
                    break;
                case VAR_VOID:
                    str = "void";
                    break;
                default:
            }
            printf("├── TYPE: '%s'\n", str);
            break;
        case NAME:
            printf("├── NAME: '%s'\n",root->name);
            break;
        case IDENTIFIER:
            printf("├── IDENTFIER: '%s'\n",root->name);
            break;
        case CALL:
            printf("├── CALL: '%s'\n",root->value);
            break;
        case VALUE:
            printf("├── VALUE: '%s'\n", root->value);
            break;
        case ASSIGN:
            printf("├── ASSIGN: '%c'\n", *(root->value));
            break;
        case INCLUDE:
            printf("├── INCLUDE: '%s'\n", root->value);
            break;
        case FUNCTION:
            printf("├── FUNCTION: '%s'\n", root->name);
            break;
        case MEASURE:
            printf("├── MEASURE: '%s'\n", root->name);
            break;
        case RETURN:
            printf("├── RETURN\n");
            break;
        default:
            printf("├── UNKNOWN\n");
            break;
    }
    
    
    // recurse sub-tree
    switch(root->type){
        case ASSIGN:
            printprefix(level+1);
            printf("├─> Left:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Right:\n");
            print_ast(root->right, level+1);
            break;
        case FUNCTION:
            printprefix(level+1);
            printf("├─> Parameters:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Body:\n");
            print_ast(root->right, level+1);
            break;
        case CALL:
            printprefix(level+1);
            printf("├─> Parameters:\n");
            print_ast(root->left, level+1);
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

void print_var_list(variable *var_list, size_t size){
    printf("\n");
    for(int i = 0; i < size; i++){
        printf("Var %i: '%s' = %s\n", i, var_list[i].name, var_list[i].value);
    }
}

void free_token_list(token* first_token){
    while(first_token != NULL){
        free(first_token->value);
        if(first_token->next_token == NULL){
            free(first_token);
            break;
        }
        token* temp_token = first_token->next_token;
        free(first_token);
        first_token = temp_token;
    }
}

void free_ast(ast *root){
    if(!root){
        return;
    }

    free(root->llvm);
    switch(root->type){
        case CALL:
            free(root->name);
            free_ast(root->left);
            break;
        case MEASURE:
        case IDENTIFIER:
        case INCLUDE:
        case NAME:
            free(root->name);
            break;
        case VALUE:
            free(root->value);
            break;
        case ASSIGN:
            free(root->value);
            free_ast(root->left);
            free_ast(root->right);
            break;
        case FUNCTION:
            free(root->name);
            free_ast(root->left);
            free_ast(root->right);
            break;
        default:
            break;
    }
    ast *next = root->branch;
    free(root);
    free_ast(next);
}

void free_var_list(variable *var_list, size_t size){
    for(int i = 0; i < size; i++){
        free(var_list[i].name);
        free(var_list[i].value);
    }
}