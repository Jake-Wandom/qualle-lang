def f(qubit q1, qubit q2, int i): int{
    H(q1);
    H(q2);
    if(i > 0){
        measure(q1);
        measure(q2);
    }
    return i
}

qubit q1 = 0;
qubit q2 = 1
int i = 2
f(q1,q2,i)