qubit q0 = 0
qubit q1 = 0
qubit q2 = 0
qubit q3 = 0
qubit q4 = 0
qubit q5 = 0

bell(q0,q1)
bell(q2,q4)
bell(q3,q5)
CX(q1,q2)
H(q1)
bit b0 = measure(q1)
if(b0){
    Z(q4)
}

bit b1 = measure(q2)
if(b1){
    X(q4)
}

CX(q4,q3)
H(q4)

bit b2 = measure(q4)
if(b2){
    Z(q5)
}

bit b3 = measure(q3)
if(b3){
    X(q5)
}

measure(q0)
measure(q5)