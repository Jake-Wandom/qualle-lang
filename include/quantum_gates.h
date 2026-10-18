#ifndef H_QUANTUM_GATES_QUALLE
#define H_QUANTUM_GATES_QUALLE

#define NUM_GATES 19

// struct that stores all important information about a quantum gate in QIR
// if QUALLE ever should support other custom backend functions that are not gates or gate related, this will have to be adapted
typedef struct {
    const char *gate_name, *qir_name;
    int args;
    int angles;
} gate;

extern const gate q_gates[];
int lookup_gate(char *name);

#endif