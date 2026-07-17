#include "analyser.h"
#include "helper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static variable *variable_list;
static size_t list_size;

variable create_var(enum variable_type type, char *name){
    variable new_var;
    new_var.type = type;
    new_var.value = NULL;
    new_var.llvm = calloc(1, sizeof(LLVMValueRef));
    size_t size = strlen(name)+1;
    new_var.name = calloc(1, size);
    strncpy(new_var.name, name, size);

    return new_var;
}

int lookup_var(char *name){
    for(int i = 0; i < list_size; i++){
        if(strcmp(variable_list[i].name, name) == 0){
            return i;
        }
    }
    // not in the list
    return -1;
}

int add_var(variable new_var){
    int pos = lookup_var(new_var.name);
    if(pos >= 0){
        // already in the list
        return -1;
    }
    list_size++;
    realloc(variable_list, list_size);
    variable_list[list_size-1] = new_var;
    return list_size-1;
}

enum variable_type check_type(char *value){

}

variable analyse_type(ast *node){
    if(node->branch->type != NAME){
        return (variable){-1};
    }
    // create a basic variable without a value and add it to the list
    variable new_var = create_var(node->var_type, node->branch->name);
    int pos = add_var(new_var);
    if(pos == -1){
        fprintf(stderr, "Variable with the same name already declared\n");
        return (variable){-1};
    }

    return new_var;
}

int check_parameters(ast *node, int num_param){
    for(int i = 0; i < num_param; i++){
        if(node->type == IDENTIFIER){
            int pos = lookup_var(node->name);
            if(pos == -1){
                fprintf(stderr, "Unkown variable\n");
                return -1;
            }
            node->llvm = variable_list[pos].llvm;
            node->resolved_type = variable_list[pos].type;
        } else if(node->type == NUMBER){
            node->resolved_type = check_type(node->value);
        } else {
            fprintf(stderr, "Function call should only have identifiers and numbers");
            return -1;
        }
        node = node->branch;
    }
    return 0;
}

int analyse_call(ast *node){
    // TODO custom functions
    if(strcmp(node->name, "H") == 0){
        int res = check_parameters(node->left, 1);
        if(res == -1) return -1;
        return 0;
        
    } else if(strcmp(node->name, "CNOT") == 0){
        int res = check_parameters(node->left, 2);
        if(res == -1) return -1;
        return 0;
    }
}

int walk_ast(ast *node){
    if(node == NULL) return 0;

    switch(node->type){
        case TYPE:
            // check if we can define a new variable
            variable new_var = analyse_type(node);
            if(new_var.type == -1) return -1;
            node->branch->llvm = new_var.llvm;
            return walk_ast(node->branch->branch);
            break;
        case CALL:
            int res = analyse_call(node);
            if(res == -1) return -1;
            return walk_ast(node->branch);
            break;
        case FUNCTION:
            break;
        default:
            return walk_ast(node->branch);
    }
}

void analyse_ast(ast *root){
    variable_list = NULL;
    list_size = 0;

    int res = walk_ast(root);
    if(res != 0){
        fprintf(stderr, "Something went wrong during analyse\n");
    }

    // release the list!
    free_var_list(variable_list, list_size);
    free(variable_list);
}
