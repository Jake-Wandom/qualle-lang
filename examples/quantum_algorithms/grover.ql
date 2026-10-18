# This is a 2-qubit implementation of the grover search algorithm
qubit q0 = 0
qubit q1 = 0

# Oracle
CZ(q0,q1)

H(q0); H(q1)
X(q0); X(q1)

CZ(q0,q1)

X(q0); X(q1)
H(q0); H(q1)

measure(q0)
measure(q1)