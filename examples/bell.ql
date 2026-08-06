# This is an example program that creates a bell state and measures it
qubit q = 0;
qubit b = 0;
H(q);
CNOT(q,b);
measure(q);
measure(b);