# this program demonstrates the conditional branching on measurements
# First we create a qubit with a 50/50 probabilty and measure
# Depending on the outcome, we then apply the X gate
qubit q = 0
qubit q1 = 0
qubit q2 = 0
H(q)

bit b = measure(q)
if(b){
    X(q1)
} else {
    X(q2)
}

measure(q1)
measure(q2)