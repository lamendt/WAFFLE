:start
ADR string R0 R0        // Load address of first character
IMM AB 0                // Make sure AB doesn't have junk

:loop
IMM A 0                 // Load null into A      
SUB R0                  // Compare to current character
BEQ end                 // If equal jump to end
MV AB_bot R0            // Move R0 into AB
LD A                    // Load character at AB into A
STO A $ff00             // Store character into display buffer
ADD R0 1                // Increment R0
J loop                  // Jump back to loop

:end

:string
'Hello World!