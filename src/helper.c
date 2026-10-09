#include "helper.h"
#include <stdlib.h>

void print_man_page(void){
    printf("quallcom [FLAGS] [FILE] ... [FILE]\n\nFLAGS:\n  -h or --help: print this page\n  -p or --print: print information like file content, token lists and abstract syntax tree\n  -a or --adaptive: compile to the adaptive QIR profile\n -o or --optimise: activate optimisations\n  -l: generates a readable .ll file instead of bitcode");
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

char* type_to_str(enum variable_type type){
    char *str = "unknown";
    switch(type){
        case VAR_QUBIT:
            str = "qubit";
            break;
        case VAR_BIT:
            str = "bit";
            break;
        case VAR_VOID:
            str = "void";
            break;
        case VAR_DOUBLE:
            str = "double";
            break;
        case VAR_INTEGER:
            str = "integer";
            break;
        case VAR_UINTEGER:
            str = "unsigned integer";
            break;
        default:
            break;
    }
    return str;
}

void print_ast(ast *root, int level){
    if (root == NULL) return;
    // print current level
    while(root){
    if((level > 1) && (root->type != ROOT)) printprefix(level);
    switch(root->type){
        case ROOT:
            printf("├─> ROOT\n");
            break;
        case TYPE:
            printf("├── TYPE: '%s'\n", type_to_str(root->resolved_type));
            break;
        case NAME:
            printf("├── NAME: '%s'\n",root->value);
            break;
        case IDENTIFIER:
            printf("├── IDENTFIER: '%s'\n",root->value);
            break;
        case CALL:
            printf("├── CALL: '%s'\n",root->value);
            break;
        case VALUE:
            printf("├── VALUE: '%s'\n", root->value);
            break;
        case ASSIGN:
            printf("├── ASSIGN: '%s'\n", root->value);
            break;
        case BINOP:
            printf("├── BINOP: '%s'\n", root->value);
            break;
        case BOOLOP:
            printf("├── BOOLOP: '%s'\n", root->value);
            break;
        case UNOP:
            printf("├── UNOP: '%s'\n", root->value);
            break;
        case CONDITIONAL:
            printf("├── IF: \n");
            break;
        case FOR_LOOP:
            printf("├── FOR LOOP: \n");
            break;
        case WHILE_LOOP:
            printf("├── WHILE LOOP: \n");
            break;
        case INCLUDE:
            printf("├── INCLUDE: '%s'\n", root->value);
            break;
        case FUNCTION:
            printf("├── FUNCTION: '%s'\n", root->value);
            break;
        case MEASURE:
            printf("├── MEASURE: '%s'\n", root->value);
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
        case BOOLOP:
        case BINOP:
        case ASSIGN:
            printprefix(level+1);
            printf("├─> Left:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Right:\n");
            print_ast(root->right, level+1);
            break;
        case UNOP:
            printprefix(level+1);
            printf("├─> Operand:\n");
            print_ast(root->left, level+1);
            break;
        case FOR_LOOP:
            printprefix(level+1);
            printf("├─> Initialisation:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Condition:\n");
            print_ast(root->other, level+1);
            printprefix(level+1);
            printf("├─> Body + Latch:\n");
            print_ast(root->right, level+1);
            break;
        case WHILE_LOOP:
            printprefix(level+1);
            printf("├─> Condition:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Body:\n");
            print_ast(root->right, level+1);
            break;
        case FUNCTION:
            printprefix(level+1);
            printf("├─> Return Type:\n");
            print_ast(root->other, level+1);
            printprefix(level+1);
            printf("├─> Parameters:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Body:\n");
            print_ast(root->right, level+1);
            break;
        case CONDITIONAL:
            printprefix(level+1);
            printf("├─> Boolean logic:\n");
            print_ast(root->left, level+1);
            printprefix(level+1);
            printf("├─> Body:\n");
            print_ast(root->right, level+1);
            printprefix(level+1);
            printf("├─> Else:\n");
            print_ast(root->other, level+1);
            break;
        case CALL:
            printprefix(level+1);
            printf("├─> Parameters:\n");
            print_ast(root->left, level+1);
            break;
        case RETURN:
            printprefix(level+1);
            printf("├─> Expression:\n");
            print_ast(root->left, level+1);
            break;
        default:
            break;
    }

    root = root->branch;
    }
}

void print_token_list(token* first_token){
    while(first_token != NULL){
        switch(first_token->type){
            case T_IDENTIFIER:
                printf("[IND %s]", first_token->value);
                break;
            
            case T_NUMBER:
                printf("[NUM %s]", first_token->value);
                break;
            
            case T_END_OF_LINE:
                printf("[EOL %s]\n", first_token->value);
                break;

            case T_DELIMITER:
                printf("[DEL %s]", first_token->value);
                break;

            case T_COMMENT:
                printf("[COM %s]", first_token->value);
                break;

            case T_BRACKET_CLOSE:
                printf("[BC %s]", first_token->value);
                break;

            case T_BRACKET_OPEN:
                printf("[BO %s]", first_token->value);
                break;

            case T_OPERATOR:
                printf("[OP %s]", first_token->value);
                break;

            case T_START:
                printf("[START]\n");
                break;

            case T_END:
                printf("[END]\n");
                break;
            
            case T_UNKOWN:
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
    printf("SIZE: %lu\n", size);
    for(size_t i = 0; i < size; i++){
        printf("Var %lu: %s '%s'\n", i, type_to_str(var_list[i].type), var_list[i].name);
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
    while(root){
        ast *next = root->branch;
        free_ast(root->left);
        free_ast(root->right);
        free_ast(root->other);
        free(root->value);
        free(root);
        
        root = next;
    }
}

void free_var_list(variable *var_list, size_t size){
    for(size_t i = 0; i < size; i++){
        free(var_list[i].name);
    }
    free(var_list);
}

void free_func_list(function *func_list, size_t size){
    for(size_t i = 0; i < size; i++){
        free(func_list[i].name);
    }
    free(func_list);
}

void free_context(context *ctx){
    free_var_list(ctx->var_list, ctx->num_vars);
    free_func_list(ctx->func_list, ctx->num_funcs);
    free(ctx);
}