qubit q0 = 0
qubit q1 = 0
qubit q2 = 0
qubit q3 = 0
qubit q4 = 0
qubit q5 = 0

H(q0); CX(q0, q1)
H(q2); CX(q2, q4)
H(q3); CX(q3, q5)
CX(q1, q2)
H(q1)

if(measure(q1)){
    Z(q4)
}
reset(q1)

if(measure(q2)){
    X(q4)
}
reset(q2)

CX(q4, q3); H(q4)
if(measure(q4)){
    Z(q5)
}
reset(q4)

if(measure(q3)){
    X(q5)
}
reset(q3)

measure(q0)
reset(q0)
measure(q5)
reset(q5)