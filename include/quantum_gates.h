#ifndef H_QUANTUM_GATES_QUALLE
#define H_QUANTUM_GATES_QUALLE

#include <llvm-c/Core.h>

typedef struct {
    const char *gate_name, *qir_name;
    int args;
    LLVMTypeRef type;
    LLVMValueRef func;
} gate;

gate lookup_gate(char *name);

#endif