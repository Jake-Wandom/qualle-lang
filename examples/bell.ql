qubit q;
qubit b;
H(q);
CNOT(q,b);
measure(q);
measure(b);