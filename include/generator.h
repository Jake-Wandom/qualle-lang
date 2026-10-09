#ifndef H_GENERATOR_QUALLE
#define H_GENERATOR_QUALLE

#include "analyser.h"
#include <stdio.h>
#include <llvm-c/Core.h>

typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMBuilderRef alloca_builder;

    LLVMBasicBlockRef entry_block;

    LLVMValueRef *valref; // same indexing as var_list 
    LLVMValueRef *result_list; // store result pointer

    LLVMTypeRef *func_type;
    LLVMValueRef *func_fn;

    LLVMTypeRef *gate_type;
    LLVMValueRef *gate_fn;
} qir_context;

void generate_statement(qir_context qir, ast *node);
void ast_walk(qir_context qir, ast *node);
FILE *generate_QIR(ast *root);

#endif