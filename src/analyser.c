#include "helper.h"
#include "error_qualle.h"
#include "global_flags.h"
#include "quantum_gates.h"
#include "analyser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// obligatory initial flag declaration explanation in global_flags.h
bool print = 0;
bool dynamic = 0;

/*
creates a new variable with all entries initialised to appropriate values
the name is duped so it has to be freed later
returns new variable
*/
variable create_var(context *ctx, enum variable_type type, char *name){
    variable new_var;
    new_var.type = type;
    new_var.status = Q_LIVE;
    new_var.scope = ctx->depth;
    new_var.active = 1;
    new_var.qubit = -1;
    new_var.arg = 0;
    new_var.length = -1;

    new_var.name = strdup(name);
    if(!new_var.name){
        add_error_entry(FATAL, -1, "Failed to duplicate variable name");
    }

    return new_var;
}

/*
enters a scope by saving the old variable count in marks
this allows us to deactivate all new variables when leaving the scope
*/
void enter_scope(context *ctx){
    if(ctx->depth > 63){
        add_error_entry(FATAL, -1, "Exceeded maximum depth of 64");
    }
    ctx->marks[ctx->depth++] = ctx->num_vars;
}

/*
leaves the scope by deactivating all scoped variables
also checks for unreleased qubits when dynamic qubits are enabled
*/
void leave_scope(context *ctx){
    for(size_t i = ctx->marks[--ctx->depth]; i < ctx->num_vars; i++){
        ctx->var_list[i].active = 0;
        if(dynamic && ctx->var_list[i].type == VAR_QUBIT && ctx->var_list[i].status != Q_RELEASED && !ctx->var_list[i].arg){
            size_t s = strlen(ctx->var_list[i].name);
            char message[60+s];
            snprintf(message, 60+s, "Qubit %s was not released before exiting scope", ctx->var_list[i].name);
            add_error_entry(ERROR, -1, message);
        }
    }
}

/*
searches for an active variable in the variable list
returns the variable position or -1/-2 on failures
*/
int lookup_var(context *ctx, char *name){
    if(!name) return -2;

    for(size_t i = ctx->num_vars; i-- > 0;){
        if(ctx->var_list[i].active && strcmp(ctx->var_list[i].name, name) == 0){
            return (int)i;
        }
    }
    // not in the list
    return -1;
}

/*
adds a variable to the variable list
increases list size if necessary and checks for uniqueness
returns variable position or -1 on failure
*/
int add_var(context *ctx, ast *node){
    int pos = lookup_var(ctx, node->value);

    if(pos >= 0 && ctx->var_list[pos].scope == ctx->depth){
        // already in the list
        return -1;
    }
    variable new_var = create_var(ctx, node->resolved_type, node->value);

    if(ctx->num_vars == ctx->list_size){
        ctx->var_list = realloc(ctx->var_list, (ctx->list_size*2)*sizeof(variable));
        if(!ctx->var_list){
            add_error_entry(FATAL, node->line, "Failed to allocate memory for variable list");
            return -1;
        }
        ctx->list_size *= 2;

    }
    node->index = ctx->num_vars;
    ctx->var_list[ctx->num_vars++] = new_var;

    return ctx->num_vars-1;
}

/*
analogue behaviour to lookup_var for functions
*/
int lookup_func(context *ctx, char *name){
    if(!name) return -2;

    for(size_t i = ctx->num_funcs; i-- > 0;){
        if(strcmp(ctx->func_list[i].name, name) == 0){
            return (int)i;
        }
    }
    // not in the list
    return -1;
}

/*
analogue behaviour to add_var for functions
*/
int add_func(context *ctx, function func){
    int pos = lookup_gate(func.name);
    if(pos >= 0){
        return -1;
    }

    pos = lookup_func(ctx, func.name);

    if(pos >= 0){
        // already in the list
        return -2;
    }

    if(ctx->num_funcs == ctx->flist_size){
        ctx->func_list = realloc(ctx->func_list, (ctx->flist_size*2)*sizeof(function));
        if(!ctx->func_list){
            add_error_entry(FATAL, -1, "Failed to allocate memory for function list");
            return -3;
        }
        ctx->flist_size *= 2;

    }
    ctx->func_list[ctx->num_funcs++] = func;

    return ctx->num_funcs-1;
}

/*
determines if a variable is numeric
returns true if numeric
*/
bool is_numeric(enum variable_type type){
    switch(type){
        case VAR_INTEGER:
        case VAR_UINTEGER:
        case VAR_DOUBLE:
            return 1;
        default:
            return 0;
    }
}

/*
determines if variable types are compatible for an assignment
returns true if assignable
*/
bool assignable_check(enum variable_type left, enum variable_type right, ast *node){
    if(left == right) return 1;
    bool qubit_check = (node->type == VALUE && right == VAR_INTEGER && (strcmp(node->value, "0") == 0 || strcmp(node->value, "1") == 0));
    if(left == VAR_QUBIT) return qubit_check;
    if(left == VAR_BIT) return qubit_check;

    if(left == VAR_DOUBLE) return (right == VAR_INTEGER || right == VAR_UINTEGER || right == VAR_BIT);
    if(left == VAR_INTEGER) return (right == VAR_UINTEGER || right == VAR_BIT);
    if(left == VAR_UINTEGER) return ((right == VAR_INTEGER && node->type == VALUE) || VAR_BIT);
    return 0;
}

/*
determines the type of an operation
name says binop but is for all operations (will change later)
this does not loose information, if an operation would do that it returns VAR_UNKOWN
returns the operation type
*/
enum variable_type binop_type(char *op, enum variable_type type1, enum variable_type type2){
    enum variable_type type = VAR_UNKOWN;

    if(strcmp("+", op) == 0 || strcmp("-", op) == 0 || strcmp("*", op) == 0 || strcmp("/", op) == 0){
        if(is_numeric(type1) && is_numeric(type2)){
            if(type1 == VAR_DOUBLE || type2 == VAR_DOUBLE){
                type = VAR_DOUBLE;
            } else if(type1 == VAR_INTEGER || type2 == VAR_INTEGER){
                type = VAR_INTEGER;
            } else {
                type = VAR_UINTEGER;
            }
        }
    } else if(strcmp("%", op) == 0){
        if((type1 == VAR_INTEGER || type1 == VAR_UINTEGER) && (type2 == VAR_INTEGER || type2 == VAR_UINTEGER)){
            if(type1 == VAR_INTEGER || type2 == VAR_INTEGER){
                type = VAR_INTEGER;
            } else {
                type = VAR_UINTEGER;
            }
        }
    } else if(strcmp("^", op) == 0){
        if(is_numeric(type1) && (type2 == VAR_INTEGER || type2 == VAR_UINTEGER)){
            if(type1 == VAR_DOUBLE || type2 == VAR_INTEGER){
                type = VAR_DOUBLE;
            } else if(type1 == VAR_INTEGER){
                type = VAR_INTEGER;
            } else {
                type = VAR_UINTEGER;
            }
        }
    } else if(strcmp("<", op) == 0 || strcmp("<=", op) == 0 || strcmp(">", op) == 0 || strcmp(">=", op) == 0){
        if(is_numeric(type1) && is_numeric(type1)){
            type = VAR_BIT;
        }
    } else if(strcmp("==", op) == 0 || strcmp("!=", op) == 0){
        if(is_numeric(type1) && is_numeric(type1)){
            type = VAR_BIT;
        } else if(type1 == VAR_BIT && type2 == VAR_BIT){
            type = VAR_BIT;
        }
    } else if(strcmp("&&", op) == 0 || strcmp("||", op) == 0){
        if(type1 == VAR_BIT && type2 == VAR_BIT){
            type = VAR_BIT;
        }
    }

    return type;
}

/*
goes through an expression and analyses its sub-components
values and identifiers are directly analysed while the other cases call sub-routines
*/
enum variable_type analyse_expression(context *ctx, ast *node){
    enum variable_type type = VAR_UNKOWN;
    enum variable_type res = VAR_UNKOWN;
    enum variable_type left = VAR_UNKOWN;
    enum variable_type right = VAR_UNKOWN;
    int pos = -1;

    switch(node->type){
        case VALUE:
            type = node->resolved_type;
            break;
        case IDENTIFIER:
            pos = lookup_var(ctx, node->value);
            if(pos < 0){
                char message[51+strlen(node->value)];
                snprintf(message, 51+strlen(node->value), "Variable with name %s was not previously declared", node->value);
                add_error_entry(ERROR, node->line, message);
                break;
            }
            node->index = pos;
            type = ctx->var_list[pos].type;
            break;
        case UNOP:
            res = analyse_expression(ctx, node->left);
            if(res == VAR_UNKOWN) break;
            if(node->value[0] == '-'){
                if(is_numeric(res)){
                    if(res == VAR_UINTEGER) type = VAR_INTEGER;
                    else type = res;
                } else {
                    add_error_entry(ERROR, node->line, "The '-' operation can only be applied to signed integers and doubles");
                }
            } else {
                if(res == VAR_BIT){
                    type = VAR_BIT;
                } else if(res == VAR_INTEGER && (strcmp(node->left->value, "0") == 0 || strcmp(node->left->value, "1") == 0)){
                    node->left->resolved_type = VAR_BIT;
                    type = VAR_BIT;
                } else {
                    add_error_entry(ERROR, node->line, "The '!' operator can only be applied to boolean values");
                }
            }
            break;
        case BINOP:
        case BOOLOP:
            if(!ctx->adaptive){
                add_error_entry(ERROR, node->line, "Operation not allowed in the QIR Base profile");
                return type;
            }

            left = analyse_expression(ctx, node->left);
            right = analyse_expression(ctx, node->right);
            if(left == VAR_UNKOWN || right == VAR_UNKOWN) break;
            if(left == VAR_QUBIT || right == VAR_QUBIT){
                add_error_entry(ERROR, node->line, "Operation can not be performed on qubits");
                break;
            }

            type = binop_type(node->value, left, right);
            if(type == VAR_UNKOWN){
                add_error_entry(ERROR, node->line, "Operation contains incompatible types");
            }
            break;
        case MEASURE:
            analyse_measure(ctx, node);
            break;
        case CALL:
            analyse_call(ctx, node);
            break;
        default:
            add_error_entry(ERROR, node->line, "Expected expression");
    }
    node->resolved_type = type;
    return type;
}

/*
analyses a body by entering a scope before walking through it
*/
void analyse_body(context *ctx, ast *node){
    enter_scope(ctx);

    walk_ast(ctx, node);

    leave_scope(ctx);
}

/*
analyses declare by trying to add the new variable to the list
does extra work for qubits
*/
void analyse_declare(context *ctx, ast *node){
    int res = add_var(ctx, node);
        
    if(res < 0){
        char message[38+strlen(node->value)];
        snprintf(message, 38+strlen(node->value), "Variable with name %s already exists", node->value);
        add_error_entry(ERROR, node->line, message);
        return;
    }

    if(ctx->var_list[res].type == VAR_QUBIT){
        if(ctx->depth != 0 && !dynamic){
            add_error_entry(ERROR, node->line, "qubits can only be declared in a global context");
            return;
        }
        ctx->var_list[res].qubit = ctx->num_qubits;
        ctx->num_qubits++;
    } else if(ctx->var_list[res].type == VAR_VOID){
        add_error_entry(ERROR, node->line, "Void is an invalid type for variables");
    }
}

/*
analyses the declared variable and the to be assigned expression
checks assignability and qubit copying
*/
void analyse_assign(context *ctx, ast *node){
    enum variable_type expr_type = analyse_expression(ctx, node->right);

    if(node->left->type == NAME){
        analyse_declare(ctx, node->left);
    } else if(node->left->type == IDENTIFIER){
        int pos = lookup_var(ctx, node->left->value);

        if(pos < 0){
            char message[51+strlen(node->left->value)];
            snprintf(message, 51+strlen(node->left->value), "Variable with name %s was not previously declared", node->left->value);
            add_error_entry(ERROR, node->line, message);
            return;
        }

        if(ctx->var_list[pos].type == VAR_QUBIT){
            add_error_entry(ERROR, node->line, "Qubits can not be redefined");
            return;
        }

        node->left->index = pos;
        node->left->resolved_type = ctx->var_list[pos].type;
    }

    if(expr_type == VAR_UNKOWN) return;
    if(node->left->resolved_type == VAR_QUBIT && expr_type == VAR_QUBIT){
        add_error_entry(ERROR, node->line, "Copying a qubit is forbidden under the no-cloning theorem");
    }
    if(!assignable_check(node->left->resolved_type, expr_type, node->right)){
        add_error_entry(ERROR, node->line, "Assign contains incompatible types");
    }
}

/*
analyses a function call by checking the parameters of a given gate/function
*/
void analyse_call(context *ctx, ast *node){
    int num_param = 0;
    enum variable_type *param;

    if(strcmp(node->value, "CNOT") == 0){
        free(node->value);
        node->value = strdup("CX");
    }

    int pos = lookup_gate(node->value);
    if(pos < 0){
        pos = lookup_func(ctx, node->value);
        
        if(pos < 0){
            add_error_entry(ERROR, node->line, "Call references unknown gate or function");
            return;
        }
        num_param = ctx->func_list[pos].num_param;
        param = calloc(num_param, sizeof(enum variable_type));
        for(int i = 0; i < num_param; i++){
            param[i] = ctx->func_list[pos].param_types[i];
        }
        node->index = pos+NUM_GATES;
        node->resolved_type = ctx->func_list[pos].return_type;
    } else {
        num_param = q_gates[pos].args+q_gates[pos].angles;
        param = calloc(num_param, sizeof(enum variable_type));
        for(int i = 0; i < num_param; i++){
            if(i < q_gates[pos].angles){
                param[i] = VAR_DOUBLE;
            } else {
                param[i] = VAR_QUBIT;
            }
        }
        if(pos == 6){ // record function
            node->res_id = ctx->num_results-1;
        }
        node->index = pos;
        node->resolved_type = VAR_VOID; 
    }

    ast *current_node = node->left;
    for(int i = 0; i < num_param; i++){
        if(!current_node){
            add_error_entry(ERROR, node->line, "Too few arguments in function call");
            break;
        }

        analyse_expression(ctx, current_node);
    
        if(!assignable_check(param[i], current_node->resolved_type, current_node)){
            char message[63];
            snprintf(message, 63, "%d. argument has an incompatible type in function call", i+1);
            add_error_entry(ERROR, current_node->line, message);
        }
        if(strcmp(node->value, "reset") == 0){
            ctx->var_list[current_node->index].status = Q_LIVE;
        } else if(strcmp(node->value, "release") == 0){
            ctx->var_list[current_node->index].status = Q_RELEASED;
        } else if(current_node->type == IDENTIFIER && ctx->var_list[current_node->index].status == Q_MEASURED){
            add_error_entry(ERROR, node->line, "Tried to use consumed qubit. To use this qubit again, call the reset gate");
        } else if(current_node->type == IDENTIFIER && ctx->var_list[current_node->index].status == Q_RELEASED){
            add_error_entry(ERROR, node->line, "Tried to use released qubit");
        }

        current_node = current_node->branch;
    }
    if(current_node){
        add_error_entry(ERROR, node->line, "Too many arguments in function call");
    }
    free(param);
}

/*
analyses a measurement call by checking the input argument and setting it to measured
*/
void analyse_measure(context *ctx, ast *node){
    int pos = lookup_var(ctx, node->value);
    if(pos < 0){
        add_error_entry(ERROR, node->line, "Unkown variable passed during measure");
        return;
    }
    if(ctx->var_list[pos].type != VAR_QUBIT){
        add_error_entry(ERROR, node->line, "Measurement arguments must be of type qubit");
        return;
    }

    node->index = pos;
    node->res_id = ctx->num_results;
    node->resolved_type = VAR_BIT;

    ctx->var_list[pos].status = Q_MEASURED;
    ctx->num_results++;
    ctx->measured = 1;
}

/*
analyses if by analysing the condition and body
*/
void analyse_if(context *ctx, ast *node){
    if(node->left->type == MEASURE){
        analyse_measure(ctx, node->left);
    } else {
        enum variable_type expr = analyse_expression(ctx, node->left);
        if(expr != VAR_BIT && expr != VAR_UNKOWN){
            add_error_entry(ERROR, node->line, "Expected boolean expression in conditional");
        }
    }
    
    analyse_body(ctx, node->right);

    if(node->other){
        analyse_body(ctx, node->other);
    }
}

/*
analyses for by analysing the condition and body
the function parameters and body are in two separate scopes
*/
void analyse_for(context *ctx, ast *node){
    enter_scope(ctx);
    analyse_statement(ctx, node->left);

    enum variable_type expr = analyse_expression(ctx, node->other);
    if(expr != VAR_BIT && expr != VAR_UNKOWN){
        add_error_entry(ERROR, node->line, "Expected boolean expression in conditional");
    }

    analyse_body(ctx, node->right);
    leave_scope(ctx);
}

/*
analyses while by analysing the condition and body
*/
void analyse_while(context *ctx, ast *node){

    enum variable_type expr = analyse_expression(ctx, node->left);
    if(expr != VAR_BIT && expr != VAR_UNKOWN){
        add_error_entry(ERROR, node->line, "Expected boolean expression in conditional");
    }

    analyse_body(ctx, node->right);
}

/*
analyses function definition
creates a new function and tries to add it to the list
analyses the parameters and the body
*/
void analyse_function(context *ctx, ast *node){
    if(ctx->depth != 0 || ctx->in_function){
        add_error_entry(ERROR, node->line, "Function declarations only at top level");
        return;
    }

    function func = {0};
    enum variable_type og_return_type = ctx->return_type;
    ctx->in_function = 1;
    ctx->ir_functions = 1;
    ctx->return_type = node->other->resolved_type;

    func.name = strdup(node->value);
    func.return_type = node->other->resolved_type;
    func.num_param = 0;
    ast *current_node = node->left;

    enter_scope(ctx);
    while(current_node){
        if(func.num_param > 63){
            add_error_entry(ERROR, current_node->line, "Too many parameters");
            ctx->in_function = 0;
            ctx->return_type = og_return_type;
            return;
        }

        if(current_node->type != NAME){
            add_error_entry(ERROR, current_node->line, "Function definition allows only variable declarations");
        } else {
            int res = add_var(ctx, current_node);
        
            if(res < 0){
                char message[38+strlen(current_node->value)];
                snprintf(message, 38+strlen(current_node->value), "Variable with name %s already exists", current_node->value);
                add_error_entry(ERROR, current_node->line, message);
                return;
            }

            if(ctx->var_list[res].type == VAR_VOID){
                add_error_entry(ERROR, current_node->line, "Void is an invalid type for variables");
            }

            ctx->var_list[res].arg = 1;

            func.param_types[func.num_param++] = current_node->resolved_type;
        }

        current_node = current_node->branch;
    }

    int res = add_func(ctx, func);
    if(res == -1){
        add_error_entry(ERROR, node->line, "Quantum Gate with same name exists");
    }
    if(res == -2){
        add_error_entry(ERROR, node->line, "Function with same name already declared");
    }

    node->index = res;

    walk_ast(ctx, node->right);

    leave_scope(ctx);

    ctx->in_function = 0;
    ctx->return_type = og_return_type;
}

/*
analyses return by checking for correct types
*/
void analyse_return(context *ctx, ast *node){
    if(!ctx->in_function){
        //add_error_entry(ERROR, node->line, "Can only return out of function");
    }
    node->resolved_type = ctx->return_type;
    
    if(ctx->return_type == VAR_VOID && node->left == NULL){
        return;
    } else if(node->left == NULL){
        add_error_entry(ERROR, node->line, "Missing return value for function");
        return;
    } else if(ctx->return_type == VAR_VOID){
        add_error_entry(ERROR, node->line, "Void function should not return anything");
        return;
    }

    enum variable_type expr = analyse_expression(ctx, node->left);

    if(!assignable_check(expr, ctx->return_type, node->left) && expr != VAR_UNKOWN){
        add_error_entry(ERROR, node->line, "Wrong return type");
        return;
    }
}

/*
calls the different sub-routines for analysis
*/
void analyse_statement(context *ctx, ast *node){
    if(!ctx->adaptive && ctx->measured && node->type != MEASURE){
        add_error_entry(ERROR, node->line, "No other statements allowed after measurement in the QIR Base profile");
        return;
    }

    switch(node->type){
        case NAME:
            analyse_declare(ctx, node);
            break;
        case ASSIGN:
            analyse_assign(ctx, node);
            break;
        case CALL:
            analyse_call(ctx, node);
            break;
        case MEASURE:
            analyse_measure(ctx, node);
            break;
        case CONDITIONAL:
            if(!ctx->adaptive){
                add_error_entry(ERROR, node->line, "Statement not allowed in the QIR Base profile");
                break;
            }
            analyse_if(ctx, node);
            break;
        case FOR_LOOP:
            if(!ctx->adaptive){
                add_error_entry(ERROR, node->line, "Statement not allowed in the QIR Base profile");
                break;
            }
            analyse_for(ctx, node);
            break;
        case WHILE_LOOP:
            if(!ctx->adaptive){
                add_error_entry(ERROR, node->line, "Statement not allowed in the QIR Base profile");
                break;
            }
            analyse_while(ctx, node);
            break;
        case FUNCTION:
            if(!ctx->adaptive){
                add_error_entry(ERROR, node->line, "Statement not allowed in the QIR Base profile");
                break;
            }
            analyse_function(ctx, node);
            break;
        case RETURN:
            analyse_return(ctx, node);
            break;
        case ROOT:
            analyse_statement(ctx, node->branch);
            break;
        default:
            add_error_entry(ERROR, node->line, "Expected a statement");
    }
}

/*
calls analyse_statement for every top-level node
*/
void walk_ast(context *ctx, ast *node){
    while(node){
        analyse_statement(ctx, node);
        node = node->branch;
    }
}

/*
creates context and calls walk_ast
returns the context
*/
context* analyse_ast(ast *root){
    context *ctx = calloc(1, sizeof(context));
    if(!ctx){
        add_error_entry(FATAL, -1, "Failed to allocate memory");
        return NULL;
    }
    ctx->var_list = calloc(8, sizeof(variable));
    if(!ctx->var_list){
        add_error_entry(FATAL, -1, "Failed to allocate memory");
        return NULL;
    }
    ctx->num_vars = 0;
    ctx->list_size = 8;

    ctx->depth = 0;

    ctx->return_type = VAR_INTEGER;
    ctx->func_list = calloc(8, sizeof(function));
    if(!ctx->func_list){
        add_error_entry(FATAL, -1, "Failed to allocate memory");
        return NULL;
    }
    ctx->num_funcs = 0;
    ctx->flist_size = 8;

    ctx->num_qubits = 0;
    ctx->num_results = 0;
    ctx->measured = 0;
    ctx->adaptive = adaptive;
    ctx->ir_functions = 0;
    ctx->mult_return_points = 0;
    ctx->mult_target_branching = 0;

    walk_ast(ctx, root->branch);
    check_errors();

    if(print) print_ast(root, 1);
    if(print) print_var_list(ctx->var_list, ctx->num_vars);

    return ctx;
}
