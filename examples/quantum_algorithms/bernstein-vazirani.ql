# This is a static 3-qubit implementation of the bernstein-vazirani algorithm
qubit q0 = 0
qubit q1 = 0
qubit q2 = 0
qubit ancilla = 0

X(ancilla)
H(q0); H(q1); H(q2);
H(ancilla)

# Oracle: We apply CX for every bit that is one in s
# In our case, s is 101 = 3
CX(q0, ancilla)
CX(q2, ancilla)

H(q0); H(q1); H(q2)

measure(q0)
measure(q1)
measure(q2)