qubit q = 0;
qubit b = 0;
H(q);
CNOT(q,b);
measure(q);
measure(b);