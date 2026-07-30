#ifndef H_GENERATOR_QUALLE
#define H_GENERATOR_QUALLE

#include "analyser.h"
#include <stdio.h>
#include <llvm-c/Core.h>

typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
} qir_context;

FILE *generate_QIR(ast *root);

#endif