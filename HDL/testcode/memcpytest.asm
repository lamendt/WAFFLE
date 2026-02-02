:start
IMM AB 1000
MV SP AB
LD A len
PUSH A
ADR R0 R1 src
ADR R2 R3 dst
:memCpy
:loop1
MV AB R0 R1
LD A ABS
MV AB R2 R3
STO A ABS
ADD R0 1
BLTU if1
ADD R1 1
:if1
ADD R2 1
BLTU if2
ADD R3 1
:if2
POP A
PUSH A
SUB R0
BNE loop1
POP A
STO A temp
POP A
PUSH A
SUB R0
LD A temp
PUSH A
BNE loop1
RET
:temp
$00
:len
$000E
:src
"Hello, World!\0"
:dst
