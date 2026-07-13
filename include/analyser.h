#ifndef H_ANALYSER_QUALLE
#define H_ANALYSER_QUALLE

#include "parser.h"
#include <llvm-c/Core.h>

enum quantum_functions {
    HADAMARD,
    CNOT,
    CX,
    X,
    Y,
    Z,
    RX,
    RY,
    RZ,
    MEASURE,
    RESET,
    INIT
};

enum instruction_type {
    ORIGIN,
    DEFINE_VAR,
    DEFINE_FUNC,
    ASSIGN_VAR,
    CALL_FUNC,
    CALL_Q_FUNC,
    RESULT
};

typedef struct {
    enum variable_type type;
    char *name;
    int value;
    LLVMValueRef llvm;
} variable;

typedef struct {
    enum variable_type type;
    char *name;
    variable **parameter;
    int num_of_param;
} func;

typedef struct instruction{
    enum instruction_type type;
    struct instruction *next_instr;
    enum quantum_functions q_func;
    union {
        func func;
        variable *var;
    };
} instruction;

instruction* analyse_ast(ast *root);

#endif