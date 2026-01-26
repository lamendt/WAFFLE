:setup
ADR user
PB
IRA
IMM AB 200
PL
ADR ija
IJA

IMM AB 991 //timer 1 set
IMM A 50
STO A

IMM AB 990 //timer 1 ctrl
IMM A %00000110 //write and interrupts
STO A

IRET

:ija
IMM A 0
MV IR A
IRET

:user
J user