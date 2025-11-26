:start
IMM A 10
MV R1 A
IMM A 0
MV R0 A
:loop
MV A R0
SUB R1
B GTEU NOLINK end
ADD R0 1
J loop
:end