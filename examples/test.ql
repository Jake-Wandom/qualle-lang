qubit n;
qubit nn;
qubit nnn;
qubit a = 1;
H(n);
H(nn);
H(nnn);
CNOT(n,a);
CNOT(nnn,a);
H(n);
H(nn);
H(nnn);
measure(n);
measure(nn)
measure(nnn)