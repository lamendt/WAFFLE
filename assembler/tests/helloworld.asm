:start
ADR R0 R1 string        // Load address of first character
IMM AB 0                // Make sure AB doesn't have junk

:loop
MV AB_bot R0            // Move R0 into AB
LD R1                   // Load character at AB into R1
IMM A 0                 // Load null into A      
SUB R1                  // Compare to current character
BEQ end                 // If equal jump to end
STO R1 800              // Store character into display buffer
ADD R0 1                // Increment R0
J loop                  // Jump back to loop

:end

:string
'Hello World!\0