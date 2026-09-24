#include "helper.h"
#include "error_qualle.h"
#include "global_flags.h"
#include "analyser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool print = 0;

extern LLVMTypeRef i64_type;
extern LLVMTypeRef float_type;

int count_nodes(ast *root){
    if(root == NULL) return 0;
    switch(root->type){
        case NAME:
            return count_nodes(root->branch)+1;
        case ASSIGN:
            return count_nodes(root->left)+count_nodes(root->branch);
        case LOOP:
        case CONDITIONAL:
            return count_nodes(root->right)+count_nodes(root->branch);
        default:
            return count_nodes(root->branch);
    }
    return -1;
}

variable create_var(enum variable_type type, char *name){
    variable new_var;
    new_var.type = type;
    new_var.llvm = calloc(1, sizeof(LLVMValueRef));
    if(!new_var.llvm){
        diagnose d = {.line = -1, .message = "Failed to allocate memory llvm pointer", .type = FATAL};
        add_error_entry(d);
    }
    size_t size = strlen(name)+1;
    new_var.name = calloc(1, size);
    if(!new_var.name){
        diagnose d = {.line = -1, .message = "Failed to allocate memory for variable name", .type = FATAL};
        add_error_entry(d);
    }
    strncpy(new_var.name, name, size);

    return new_var;
}

int lookup_var(char *name, variable *variable_list, size_t size){
    if(variable_list == NULL) return -1;

    for(size_t i = 0; i < size; i++){
        if(variable_list[i].name == NULL){
            break;
        }

        if(strcmp(variable_list[i].name, name) == 0){
            return i;
        }
    }
    // not in the list
    return -1;
}

int add_var(variable new_var, variable *variable_list, size_t size){
    if(new_var.name == NULL){
        diagnose d = {.line = -1, .message = "variable is missing a name", .type = WARNING};
        add_error_entry(d);
        new_var.name = "MISSING_NAME";
    }
    int pos = lookup_var(new_var.name, variable_list, size);
    if(pos >= 0){
        // already in the list
        char *message = malloc(strlen(new_var.name)+52);
        sprintf(message, "Variable with the name '%s' is already in the list", new_var.name);
        diagnose d = {.line = -1, .message = message, .type = ERROR};
        add_error_entry(d);
        return -1;
    }
    
    // finding the next variable slot
    for(size_t i = 0; i < size; i++){
        if(variable_list[i].name == NULL){
            pos = i;
            break;
        }
    }

    variable_list[pos] = new_var;
    return pos;
}

int check_type(ast *node, variable *variable_list, size_t size){
    if(node->type == IDENTIFIER){
        int pos = lookup_var(node->name, variable_list, size);
        if(pos < 0){
                char *message = malloc(strlen(node->name)+40);
                sprintf(message, "Variable with the name '%s' is unknown", node->name);
                diagnose d = {.line = node->line, .message = message, .type = ERROR};
                add_error_entry(d);
                return -1;
            }
        node->resolved_type = variable_list[pos].type;
        node->llvm = variable_list[pos].llvm;
        return pos;

    } else if(node->type == VALUE){
        // we give every value a llvm value in the analyser
        char *pos = strchr(node->value, '.');
        if(pos == NULL){ // integer
            node->resolved_type = VAR_INTEGER;
            node->llvm = calloc(1, sizeof(LLVMValueRef));
            if(!(node->llvm)){
                diagnose d = {.line = -1, .message = "Failed to allocate memory llvm pointer", .type = FATAL};
                add_error_entry(d);
            }
            long val = strtol(node->value, NULL, 10);
            *(node->llvm) = LLVMConstInt(i64_type, val, 0);
        } else { // floating point number
            node->resolved_type = VAR_DOUBLE;
            node->llvm = calloc(1, sizeof(LLVMValueRef));
            if(!(node->llvm)){
                diagnose d = {.line = -1, .message = "Failed to allocate memory llvm pointer", .type = FATAL};
                add_error_entry(d);
            }
            double val = strtof(node->value, NULL);
            *(node->llvm) = LLVMConstInt(float_type, val, 0);
        }
        return 0;

    } else{
        diagnose d = {.line = -1, .message = "Can only check the type of references and values", .type = INTERNAL};
        add_error_entry(d);
        return -1;
    }
}

variable analyse_name(ast *node, variable *variable_list, size_t size){
    if(node->type != NAME){
        diagnose d = {.line = node->line, .message = "Expected variable definition", .type = ERROR};
        add_error_entry(d);
        return (variable){.type = -1, .name = NULL, .llvm = NULL};
    }

    // create a basic variable without a value and add it to the list
    variable new_var = create_var(node->resolved_type, node->name);
    int pos = add_var(new_var, variable_list, size);
    if(pos == -1){
        free(new_var.name);
        free(new_var.llvm);
        return (variable){.type = -1, .name = NULL, .llvm = NULL};
    }
    node->llvm = new_var.llvm;
    
    return new_var;
}

int check_parameters(ast *node, int num_param, variable *variable_list, size_t size){
    for(int i = 0; i < num_param; i++){
        if(node->type == IDENTIFIER){
            int r = check_type(node, variable_list, size);
            if(r < 0) return -1;
        } else if(node->type == VALUE){
            int r = check_type(node, variable_list, size);
            if(r != 0) return -1;
        } else {
            diagnose d = {.line = node->line, .message = "Function call can only contain Identifiers and Values", .type = ERROR};
            add_error_entry(d);
            return -1;
        }
        node = node->branch;
    }
    return 0;
}

int analyse_call(ast *node, variable *variable_list, size_t size){
    // TODO custom functions
    if(strcmp(node->name, "H") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
        
    } else if(strcmp(node->name, "X") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "Y") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "Z") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RX") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RY") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RZ") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "S") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "T") == 0){
        if(check_parameters(node->left, 1, variable_list, size) == -1) return -1;
        return 0;
    } else if((strcmp(node->name, "CNOT") == 0) || (strcmp(node->name, "CX") == 0)){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "CY") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "CZ") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "SWAP") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RXX") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RYY") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    } else if(strcmp(node->name, "RZZ") == 0){
        if(check_parameters(node->left, 2, variable_list, size) == -1) return -1;
        return 0;
    }

    char *message = malloc(strlen(node->name)+27);
    sprintf(message, "Unkown function name '%s'", node->name);
    diagnose d = {.line = node->line, .message = message, .type = ERROR};
    add_error_entry(d);

    return -1;
}

int analyse_left(ast *node, variable *variable_list, size_t size){
    if(node->type == IDENTIFIER){
        int pos = check_type(node, variable_list, size);
        if(pos < 0) return -1;
        return pos;
        
    } else if(node->type == NAME){
        variable new_var = analyse_name(node, variable_list, size);
        if((int)new_var.type == -1) return -1;
        
        int pos = lookup_var(node->name, variable_list, size);
        
        if(pos < 0){
            char *message = malloc(strlen(node->name)+59);
            sprintf(message, "A problem occurred while adding variable '%s' to the list", node->name);
            diagnose d = {.line = node->line, .message = message, .type = INTERNAL};
            add_error_entry(d);
            return -1;
        }
        else {
            return pos;
        }
        
    } else {
        diagnose d = {.line = node->line, .message = "Expected Variable definition or reference on the left side of an assign", .type = ERROR};
        add_error_entry(d);
        return -1;
    }
}

int analyse_operations(ast *node, variable *variable_list, size_t size){
    if((variable_list == NULL) || (size == 0)) return -1;

    int r;
    diagnose d;
    switch (node->type){
        case BINOP:
            r = analyse_operations(node->left, variable_list, size);
            if(r < 0) return -1;
            r = analyse_operations(node->right, variable_list, size);
            if(r < 0) return -1;
            return 0;

        case VALUE:
            r = check_type(node, variable_list, size);
            if(r != 0) return -1;
            return r;

        case IDENTIFIER:
            r = check_type(node, variable_list, size);
            if(r < 0) return -1;   
            return r;

        default:
            d = (diagnose){.line = node->line, .message = "Unable to binary operation", .type = ERROR};
            add_error_entry(d);
            return -1;
    }
}

int analyse_right(ast *node, variable *variable_list, size_t size){
    if((variable_list == NULL) || (size == 0)) return -1;

    int r;
    diagnose d;
    switch (node->type){
        case VALUE:
            r = check_type(node, variable_list, size);
            if(r != 0) return -1;
            return r;

        case IDENTIFIER:
            r = check_type(node, variable_list, size);
            if(r < 0) return -1;   
            return r;

        case BINOP:
            return analyse_operations(node, variable_list, size);

        default:
            d = (diagnose){.line = node->line, .message = "Unable to analyse right expression", .type = ERROR};
            add_error_entry(d);
            return -1;
    }
}

int walk_ast(ast *node, variable *variable_list, size_t size){
    if(node == NULL) return 0;
    int res;
    int pos;
    diagnose d;
    variable new_var;

    switch(node->type){
        case TYPE:
            d = (diagnose){.line = node->line, .message = "The TYPE node type is currently not supported", .type = FATAL};
            add_error_entry(d);
            return -1;
            break;

        case NAME:
            // check if we can define a new variable
            new_var = analyse_name(node, variable_list, size);
            if((int)new_var.type == -1) return -1;
            return walk_ast(node->branch->branch, variable_list, size);
            break;

        case CALL:
            res = analyse_call(node, variable_list, size);
            if(res == -1) return -1;
            return walk_ast(node->branch, variable_list, size);
            break;

        case ASSIGN:
            res = analyse_left(node->left, variable_list, size);
            if(res == -1) return -1;

            res = analyse_right(node->right, variable_list, size);
            
            return walk_ast(node->branch, variable_list, size);
            break;

        case FUNCTION:
            break;
        
        case MEASURE:
        case IDENTIFIER:
            pos = lookup_var(node->name, variable_list, size);
            if(pos < 0){
                char *message = malloc(64);
                sprintf(message, "Variable with the name '%s' is unknown", node->name);
                d = (diagnose){.line = node->line, .message = message, .type = ERROR};
                add_error_entry(d);
                return -1;
            }
            node->llvm = variable_list[pos].llvm;
            return walk_ast(node->branch, variable_list, size);

        default:
            return walk_ast(node->branch, variable_list, size);
    }
    return -1;
}

variable* analyse_ast(ast *root){
    
    size_t size = count_nodes(root);
    variable *variable_list = calloc(size, sizeof(variable));
    if(!variable_list){
        diagnose d = {.line = -1, .message = "Failed to allocate memory for variable list", .type = FATAL};
        add_error_entry(d);
        return NULL;
    }
    
    int res = walk_ast(root, variable_list, size);
    if(res != 0){
        diagnose d = {.line = -1, .message = "ERROR during analysis", .type = ERROR};
        add_error_entry(d);
        free_var_list(variable_list, size);
    }
    check_errors();

    if(print) print_var_list(variable_list, size);
    
    return variable_list;
}
