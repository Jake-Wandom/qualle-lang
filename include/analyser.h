#ifndef H_ANALYSER_QUALLE
#define H_ANALYSER_QUALLE

#include "parser.h"
#include "quantum_gates.h"
#include <stdio.h>

enum status {
    Q_LIVE,
    Q_MEASURED,
    Q_RELEASED
};

typedef struct {
    // type name and linear status
    enum variable_type type;
    char *name;

    // extra information for qubits and arrays
    enum status status;
    bool arg;
    int length;

    // depth at declaration
    int scope;
    bool active; // false once scope is left

    // qubit index
    int qubit; 
} variable;

typedef struct {
    char *name;
    enum variable_type return_type;

    enum variable_type param_types[64];
    int num_param;
} function;

typedef struct {
    // variable list variables 
    variable *var_list;
    size_t num_vars, list_size;

    // scope variables
    size_t marks[64];
    int depth;

    // function variables
    function *func_list;
    size_t num_funcs, flist_size;
    enum variable_type return_type;
    bool in_function;

    // relevant for generator
    int num_qubits, num_results;
    bool measured, adaptive, ir_functions, mult_target_branching, mult_return_points; 
} context;

enum variable_type binop_type(char *op, enum variable_type type1, enum variable_type type2);
int lookup_func(context *ctx, char *name);
void analyse_statement(context *ctx, ast *node);
void analyse_call(context *ctx, ast *node);
void analyse_measure(context *ctx, ast *node);
void walk_ast(context *ctx, ast *node);
context* analyse_ast(ast *root);

#endif