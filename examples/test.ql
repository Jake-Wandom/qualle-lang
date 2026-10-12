def f(int a, int b): int{
    return a + b
}

qubit q = 0

int a = 5
int b = 5
int c = f(a,b)
if(c == 10){
H(q)
}

bit boolean = measure(q)
reset(q)
if(boolean){
    X(q)
}
measure(q)