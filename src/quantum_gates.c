#include "quantum_gates.h"

#include <string.h>

int lookup_gate(char *name){
    if(!name) return -2;

    for(size_t i = 0; i < NUM_GATES; i++){
        if(strcmp(q_gates[i].gate_name, name) == 0){
            return i;
        }
    }
    return -1;
}

// don't forget to edit the NUM_GATES define
const gate q_gates[] = {
    {"H", "__quantum__qis__h__body", 1, 0},
    {"X", "__quantum__qis__x__body", 1, 0},
    {"Y", "__quantum__qis__y__body", 1, 0},
    {"Z", "__quantum__qis__z__body", 1, 0},
    {"reset", "__quantum__qis__reset__body", 1, 0},
    {"release", "__quantum__rt__qubit_release", 1, 0},
    {"record", "__quantum__rt__result_record_output", 2, 0},
    {"RX", "__quantum__qis__rx__body", 1, 1},
    {"RY", "__quantum__qis__ry__body", 1, 1},
    {"RZ", "__quantum__qis__rz__body", 1, 1},
    {"S", "__quantum__qis__s__body", 1, 1},
    {"T", "__quantum__qis__t__body", 1, 0},
    {"CX", "__quantum__qis__cx__body", 2, 0},
    {"CY", "__quantum__qis__cy__body", 2, 0},
    {"CZ", "__quantum__qis__cz__body", 2, 0},
    {"SWAP", "__quantum__qis__swap__body", 2, 0},
    {"RXX", "__quantum__qis__rxx__body", 2, 1},
    {"RYY", "__quantum__qis__ryy__body", 2, 1},
    {"RZZ", "__quantum__qis__rzz__body", 2, 1}
};