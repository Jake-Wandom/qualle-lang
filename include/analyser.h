#ifndef H_ANALYSER_QUALLE
#define H_ANALYSER_QUALLE

#include "parser.h"
#include <llvm-c/Core.h>

typedef struct {
    enum variable_type type;
    char *name;
    char *value;
    LLVMValueRef *llvm;
} variable;

int count_nodes(ast *root);
variable* analyse_ast(ast *root);

#endif