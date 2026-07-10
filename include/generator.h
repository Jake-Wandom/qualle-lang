#ifndef H_GENERATOR_QUALLE
#define H_GENERATOR_QUALLE

#include "parser.h"
#include <stdio.h>
#include <llvm-c/Core.h>

enum quantum_functions {
    HADAMARD,
    CNOT,
    X,
    Y,
    Z,
    MEASURE
};

typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
} qir_context;

FILE *generate_QIR(bool bitcode, ast *root);

#endif