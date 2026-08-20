#include "helper.h"
#include "global_flags.h"
#include "analyser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool print = 0;
bool adaptive = 0;

void analyse_error(ast *error_node, enum error_type type, char *error_message){
    fprintf(stderr, "DURING ANALYSIS: ");
    switch (type){
        case UNEXPECTED_ERROR:
            fprintf(stderr, "UNEXPECTED ERROR: %s\n", error_message);
            break;
        case WRONG_TYPE_ERROR:
            fprintf(stderr, "WRONG TYPE ERROR: %s\n", error_message);
            break;
        case UNKOWN_SYMBOL_ERROR:
            fprintf(stderr, "UNKOWN SYMBOL ERROR: %s\n", error_message);
            break;
        case NO_CONTEXT_ERROR:
            fprintf(stderr, "NO CONTEXT ERROR: %s\n", error_message);
            break;
        case UNKOWN_TYPE_ERROR:
            fprintf(stderr, "MISSING VARIABLE ERROR: %s\n", error_message);
            break;
        case FORBIDDEN_ERROR:
            fprintf(stderr, "FORBIDDEN ERROR: %s\n", error_message);
            break;
        default:
            fprintf(stderr, "ERROR: %s\n", error_message);
    }
    // locate position
    if((error_node->type == IDENTIFIER) || (error_node->type == NAME) || (error_node->type == VALUE)){
        fprintf(stderr, "line %i    ->%s<-\n\n", error_node->line, error_node->value);
    } else {
        fprintf(stderr, "line %i\n\n", error_node->line);
    }
}

int count_nodes(ast *root){
    if(root == NULL) return 0;
    switch(root->type){
        case TYPE:
            return count_nodes(root->branch)+1;
        case ASSIGN:
            return count_nodes(root->left)+count_nodes(root->right)+count_nodes(root->branch);
        case CALL:
            return count_nodes(root->left)+count_nodes(root->branch);
        default:
            return count_nodes(root->branch);
    }
    return -1;
}

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
    if(new_var.name == NULL) fprintf(stderr, "Variable has name NULL\n");
    int pos = lookup_var(new_var.name, variable_list, size);
    if(pos >= 0){
        // already in the list
        return -1;
    }
    
    // finding the next variable slot
    for(size_t i = 0; i < size; i++){
        if(variable_list[i].name == NULL){
            pos = i;
            break;
        }
    }

    if(pos < 0) return -1;

    variable_list[pos] = new_var;
    return pos;
}

enum variable_type check_type(char *value){
    // TODO
    if(value == NULL) return -1;
    return VAR_VOID;
}

variable analyse_type(ast *node, variable *variable_list, size_t size){
    if(node->branch->type != NAME){
        return (variable){.type = -1, .name = NULL, .llvm = NULL, .value = NULL};
    }
    // create a basic variable without a value and add it to the list
    variable new_var = create_var(node->var_type, node->branch->name);
    int pos = add_var(new_var, variable_list, size);
    if(pos == -1){
        analyse_error(node->branch, NAME_CONFLICT_ERROR, "Variable with the same name already declared");
        return (variable){.type = -1, .name = NULL, .llvm = NULL, .value = NULL};
    }
    node->branch->llvm = new_var.llvm;
    node->branch->resolved_type = node->var_type;
    
    return new_var;
}

int check_parameters(ast *node, int num_param, variable *variable_list, size_t size){
    for(int i = 0; i < num_param; i++){
        if(node->type == IDENTIFIER){
            int pos = lookup_var(node->name, variable_list, size);
            if(pos < 0){
                analyse_error(node, MISSING_ERROR, "Unkown Variable");
                return -1;
            }
            node->llvm = variable_list[pos].llvm;
            node->resolved_type = variable_list[pos].type;

        } else if(node->type == VALUE){
            node->resolved_type = check_type(node->value);

        } else {
            analyse_error(node, FORBIDDEN_ERROR, "Function call should only have identifiers and numbers");
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
    return -1;
}

int analyse_left(ast *node, variable *variable_list, size_t size){
    if(node->type == IDENTIFIER){
        int pos = lookup_var(node->name, variable_list, size);
        if(pos < 0) return -1;

        return pos;

    } else if(node->type == TYPE){
        variable new_var = analyse_type(node, variable_list, size);
        if((int)new_var.type == -1) return -1;
        
        int pos = lookup_var(node->branch->name, variable_list, size);

        if(pos < 0) return -1;
        else return pos;

    } else {
        analyse_error(node, UNEXPECTED_ERROR, "Expected Variable definition or reference left of assign");
        return -1;
    }
}

char* analyse_right(ast *node, variable *variable_list, size_t size){
    // MUCH TODO HERE
    if((variable_list == NULL) || (size == 0)) return NULL;

    if(node->type == VALUE){
        return node->value;
    } else {
        fprintf(stderr, "Unable to analyse right side of assign\n");
        return NULL;
    }
}

int walk_ast(ast *node, variable *variable_list, size_t size){
    if(node == NULL) return 0;
    int res;
    int pos;
    variable new_var;

    switch(node->type){
        case TYPE:
            // check if we can define a new variable
            new_var = analyse_type(node, variable_list, size);
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

            char *value = analyse_right(node->right, variable_list, size);
            size_t value_size = strlen(value)+1;
            variable_list[res].value = malloc(value_size);
            strncpy(variable_list[res].value, value, value_size);
            
            return walk_ast(node->branch, variable_list, size);
            break;

        case FUNCTION:
            break;
        
        case MEASURE:
        case IDENTIFIER:
            pos = lookup_var(node->name, variable_list, size);
            if(pos < 0){
                analyse_error(node, MISSING_ERROR, "Unknown Variable");
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
    
    int res = walk_ast(root, variable_list, size);
    if(res != 0){
        fprintf(stderr, "\nERROR DURING ANALYSIS\n");
        return NULL;
    }
    
    if(print) print_var_list(variable_list, size);
    
    return variable_list;
}
