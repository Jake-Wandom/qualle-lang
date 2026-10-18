#ifndef H_GENERATOR_QUALLE
#define H_GENERATOR_QUALLE

#include "analyser.h"
#include <stdio.h>
#include <llvm-c/Core.h>

// context struct for the generator
typedef struct {
    // LLVM context variables
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    // entry block of the current function
    LLVMBasicBlockRef entry_block;

    LLVMValueRef *valref; // same indexing as var_list 
    LLVMValueRef *result_list; // store result pointer

    // same indexing as function list
    LLVMTypeRef *func_type;
    LLVMValueRef *func_fn;
    bool returned;
    
    LLVMTypeRef *gate_type;
    LLVMValueRef *gate_fn;
} qir_context;

void generate_statement(qir_context *qir, ast *node);
void generate_call(qir_context *qir, ast *node);
void generate_measure(qir_context *qir, ast *node);
void ast_walk(qir_context *qir, ast *node);
FILE *generate_QIR(ast *root);

#endif