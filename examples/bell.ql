# This is an example program that creates a bell state and measures it
qubit q1 = 0;
qubit q2 = 0;
H(q1);
CNOT(q1,q2);
measure(q1);
measure(q2);