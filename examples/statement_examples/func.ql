# this program demonstrates the use of functions

# this function creates a bell state for two qubits
def bell(qubit q1, qubit q2){
    H(q1)
    CX(q1, q2)
}

qubit q1 = 0
qubit q2 = 0

bell(q1, q2)

measure(q1)
measure(q2)