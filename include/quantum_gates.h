#ifndef H_QUANTUM_GATES_QUALLE
#define H_QUANTUM_GATES_QUALLE

#include <llvm-c/Core.h>

#define NUM_GATES 16

typedef struct {
    const char *gate_name, *qir_name;
    int args;
    int angles;
} gate;

extern const gate q_gates[];
int lookup_gate(char *name);

#endif