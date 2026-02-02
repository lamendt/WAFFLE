//mult(byte a, byte b)
//    j = 0
//    c = 0
//    for i++ from 0 to 7
//        if a[0]
//            push b <<< i
//            j ++
//        a >>> 1
//    for i++ from 0 to j
//        pop b
//        c += b
//    return c

:smallMult
//R0 = a, R1 = b
IMM R2 0         // i = 0 (in R2)
IMM R3 0         // j = 0 (in R3)

:loop1
IMM A 1         // if a[0]
AND R0
BEQ if1
MV A R1         // push b <<< i 
SLA R2
PUSH A
ADD R3 1        // j++
:if1
MV A R0         // a >> 1
SRA 1
MV R0 A
ADD R2 1        // i++
IMM A 7         // loop until i == 7
SUB R2
BNE loop1

// R0 = c
IMM R0 0
IMM R2 0         // i = 0 (in R2) 
:loop2
POP R1          // pop b
MV A R0         // c += b
ADD R1
MV R0 A
ADD R2 1        // i++
MV A R3         // loop until i == j
SUB R2
BNE loop2

MV A R0         // return c
RET