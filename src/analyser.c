#include "analyser.h"
#include <stdlib.h>
/*
variable *variable_list;
function *function_list;

int count_nodes(enum keywords type, ast *current_node){
    if(!current_node){
        return 0;
    }
    if(current_node->type == type){
        switch(current_node->type){
        case ASSIGN_VAL:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->assignment.assignee)+count_nodes(type, current_node->assignment.assignor);
            break;
        case FUNC_DEF:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->func.variables)+count_nodes(type, current_node->func.body);
            break;
        case BIN_OP:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->operation.operator_a)+count_nodes(type, current_node->operation.operator_b);
            break;
        case BOOL_OP:
            return 1+count_nodes(type, current_node->branch)+count_nodes(type, current_node->condition.operator_a)+count_nodes(type, current_node->condition.operator_b);
            break;
        default:
            return 1+count_nodes(type, current_node->branch);
        }
    } else {
        switch(current_node->type){
        case ASSIGN_VAL:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->assignment.assignee)+count_nodes(type, current_node->assignment.assignor);
            break;
        case FUNC_DEF:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->func.variables)+count_nodes(type, current_node->func.body);
            break;
        case BIN_OP:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->operation.operator_a)+count_nodes(type, current_node->operation.operator_b);
            break;
        case BOOL_OP:
            return count_nodes(type, current_node->branch)+count_nodes(type, current_node->condition.operator_a)+count_nodes(type, current_node->condition.operator_b);
            break;
        default:
            return count_nodes(type, current_node->branch);
        }
    }
}

int check_node(ast *current_node){

}

void analyse_ast(ast *root){
    // initialise variable and function list
    int var_counter = count_nodes(VAR_DEF, root);
    int func_counter = count_nodes(FUNC_DEF, root);
    variable_list = calloc(var_counter,sizeof(variable));
    function_list = calloc(func_counter, sizeof(function));


}
    */