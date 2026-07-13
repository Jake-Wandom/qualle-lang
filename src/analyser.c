#include "analyser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

variable *variable_list;
int num_of_vars;
function *function_list;
int num_of_funcs;

int count_nodes(enum keywords type, ast *current_node){
    if(!current_node){
        return 0;
    }
    if(current_node->type == type){
        switch(current_node->type){
        case ASSIGN:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->assign.left)+count_nodes(type, current_node->assign.right);
            break;
        case FUNC_DEF:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->func.param)+count_nodes(type, current_node->func.body);
            break;
        default:
            return 1+count_nodes(type, current_node->branch);
        }
    } else {
        switch(current_node->type){
        case ASSIGN:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->assign.left)+count_nodes(type, current_node->assign.right);
            break;
        case FUNC_DEF:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->func.param)+count_nodes(type, current_node->func.body);
            break;
        default:
            return count_nodes(type, current_node->branch);
        }
    }
}

int check_variable_list(char *name){
    for(int i = 0; i < num_of_vars; i++){
        if((strcmp(variable_list[i].name, name)) == 0){
            return i;
        }
    }
    return -1;
}

int check_function_list(char *name){
    for(int i = 0; i < num_of_funcs; i++){
        if((strcmp(function_list[i].name, name)) == 0){
            return i;
        }
    }
    return -1;
}

enum quantum_functions check_quantum(char *name){
    if(strcmp(name, "H") == 0){
        return HADAMARD;
    } else if(strcmp(name, "CNOT") == 0){
        return CNOT;
    } else if(strcmp(name, "CX") == 0){
        return CX;
    } else if(strcmp(name, "X") == 0){
        return X;
    } else if(strcmp(name, "Y") == 0){
        return Y;
    } else if(strcmp(name, "Z") == 0){
        return Z;
    } else if(strcmp(name, "measure") == 0){
        return MEASURE;
    } else if(strcmp(name, "reset") == 0){
        return RESET;
    }   
    return -1;
}


variable analyse_type(ast *node){
    variable var;
    var.name = NULL;
    var.type = node->var_type;

    if(node->branch->type == NAME){
        var.name = node->branch->value;

    } else {
        // TODO Error handling

    }
    return var;
}

int analyse_func(ast *node){
    int pos = -2;
    pos = check_function_list(node->value);
    if(pos < 0){
        if(check_quantum(node->value) == -1){
            return -1;
        }
        return -2;
    } else {
        return pos;
    }
}

int determine_value(ast *node){

    while((node->type != END_OF_LINE) && (node->type != END)){
        if(node->type == VALUE){
            return strtol(node->value, NULL, 10);
        }
        node = node->branch;
    }
}

instruction* analyse_assign(ast *node, instruction *instr){
    // analyse left
    ast *left_node = node->assign.left;
    bool new_var = 1;
    int pos = -2;

    if(left_node->type == TYPE){
        variable var = analyse_type(left_node);
        if(!var.name) return NULL;
        if(check_variable_list(var.name) > 1){
            // TODO Error handling
            return NULL;
        }

        variable_list[num_of_vars] = var;
        pos = num_of_vars;
        num_of_vars++;
    } else if(left_node->type == NAME){
        pos = check_variable_list(left_node->value);
        new_var = 0;
        if(pos == -1){
            // TODO Error handling
            return NULL;
        }
    } else {
        // TODO Error handling
        return NULL;
    }

    // analyse right
    int res = determine_value(node->assign.right);
    if(pos < 0){
        // TODO Error handling
        return NULL;
    }
    variable_list[pos].value = res;
    
    instruction *new_instr = malloc(sizeof(instruction));
    new_instr->next_instr = NULL;
    if(new_var){
        new_instr->type = DEFINE_VAR;
    } else {
        new_instr->type = ASSIGN_VAR;
    }
    new_instr->var = &variable_list[pos];

    instr->next_instr = new_instr;
    return new_instr;
}

instruction* analyse_qfunc(ast *node, enum quantum_functions type){
    instruction *new_instr = malloc(sizeof(instruction));
    new_instr->type = CALL_Q_FUNC;
    new_instr->q_func = type;
    new_instr->next_instr = NULL;

    switch (type){
        case HADAMARD:
            new_instr->func.num_of_param = 1;
            new_instr->func.parameter = malloc(sizeof(variable*));
            
            if(node->branch->type != NAME){
                // TODO Error handling
                return NULL;
            }
            int res = check_variable_list(node->branch->value);
            if(res < 0){
                // TODO Error handling
                return NULL;
            }
            new_instr->func.parameter[0] = &variable_list[res];
            break;
        default:
            free(new_instr);
            return NULL;
    }
}


void walk_tree(ast *node, instruction *instr){
    if(!node || !instr){
        return;
    }
    instruction *new_instr;
    switch (node->type){
        case ROOT:
            walk_tree(node->branch, instr);
            break;
        case ASSIGN:
            new_instr = analyse_assign(node, instr);
            if(!new_instr) break;
            walk_tree(node->branch, new_instr);
            break;
        case TYPE:
            variable var = analyse_type(node);
            if(!var.name) break;
            if(check_variable_list(var.name) > 1){
                // TODO Error handling
                break;
            }

            variable_list[num_of_vars] = var;
            num_of_vars++;

            new_instr = malloc(sizeof(instruction));
            new_instr->type = DEFINE_VAR;
            new_instr->var = &variable_list[num_of_vars-1];
            new_instr->next_instr = NULL;
            instr->next_instr = new_instr;
            
            walk_tree(node->branch->branch, new_instr);
            break;
        case NAME:
            int res = analyse_func(node);
            if(res == -2){
                new_instr = malloc(sizeof(instruction));
                new_instr->type = CALL_Q_FUNC;
                new_instr->q_func = check_quantum(node->value);
                new_instr->next_instr = NULL;
                instr->next_instr = new_instr;
                walk_tree(node->branch->branch, new_instr);
            } else {
                // TODO
                walk_tree(node->branch, new_instr);
            }
            break;
        default:
            printf("not expected\n");
    }
}

instruction* analyse_ast(ast *root){
    // initialise variable and function list
    int var_counter = count_nodes(TYPE, root);
    int func_counter = count_nodes(FUNC_DEF, root);

    instruction *start_instr = malloc(sizeof(instruction));
    variable_list = calloc(var_counter,sizeof(variable));
    function_list = calloc(func_counter, sizeof(function));
    num_of_vars = 0;
    num_of_funcs = 0;
    printf("vars: %i, funcs: %i\n", var_counter, func_counter);
    
    start_instr->next_instr = NULL;
    start_instr->type = ORIGIN;
    walk_tree(root, start_instr);
    printf("var 1: %s\n", variable_list[0].name);

    //free(variable_list);
    //free(function_list);
    return start_instr;
}
