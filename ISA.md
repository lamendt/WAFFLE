# Overview
This ISA is not created to solve any particular problem. In fact, its design is certainly slower than comparable alternatives. Rather, it was created as a challenge.

The constraints:
- Fit all instructions within one byte of memory
- 16-bit addressing
- Able to support an OS, meaning
    - Interrupt-capable
    - Privilege levels
    - Multitasking

This has led to some unconventional design features, but it achieves these goals.
# Registers
| Acronym | Name | Size | Description |
|-|-|-|-|
| **A** | Accumulator | 8-bit | The main general-purpose register that is always operand 1 and destination for 8-bit arithmetic |
| **Rx** | General-Purpose-Registers | 8-bit | A set of 4 general purpose registers R0-R3 that contribute to the other operand for 8-bit arithmetic |
| **PC** | Program Counter | 16-bit | Contains the address of the current instruction being executed |
| **AB** | Address Bus | 16-bit | Used any time an address (absolute or relative) is referenced, such as in branches and loads |
| **RA** | Return Address | 16-bit | Stores the return address for function calls|
| **SP** | Stack Pointer | 16-bit | Points to the location of the stack in memory |
| **FR** | Flag Register | 8-bit | Stores the ALU flags |
| **IR** | Interrupt Register | 8-bit | Each bit represents a different interrupt source |
| **IJA** | Interrupt Jump Address | 16-bit | Stores where the CPU should jump to when an interrupt happens |
| **IRA** | Interrupt Return Address | 16-bit | Stores where the CPU should return to after an interrupt is handled |
| **PB** | Program Base | 16-bit | Stores the first address of the current user program |
| **PL** | Program Limit | 16-bit | Stores the last address of the current user program |

# Flags
This architecture uses the standard 4 ALU flags (Z,N,C,V)

|Flag|Meaning|
|-|-|
|Z|Result is 0|
|N|MSB of result is 1|
|C|Carry occurred / no borrow occurred|
|V|Signed overflow|

# Memory
16-byte addressable, little-endian, 8-bit word memory (64 KiB). Not all addresses are for data, however, as I/O is memory-mapped. There is no virtual memory - rather - kernel and user space are separated by the use of **PB** and **PL**. 

When the kernel gives control to a program, it must set **PB** and **PL**. In user programs, memory loads and stores are relative to **PB**. If a relative jump or memory access would be outside of these bounds, **IR[0]** is set. However, programs can freely jump, load, and store outside of these limits with absolute commands.*

**Note: This was done to minimize syscall overhead. Obviously, there are massive security risks, but since the only developer is also the ISA designer, this is not an issue.*

# Interrupts
When an interrupt occurs, the bit(s) in **IR** corresponding to the interrupt type(s) are set, **IRA** is set to **PC**, and the CPU jumps to **IJA** and switches to kernel mode. After the interrupt is handled, the kernel gives control back to the user via IRET. 

Interrupt types are encoded in **IR** as follows:

|{7:5}|{4}|{3:2}|{1}|{0}|
|-|-|-|-|-|
|Other hardware interrupts|User input (recommended) |Timer (recommended)|SEGFAULT|SYSCALL|


# Instructions
## Data Movement
### Registers
#### MV
*(Move)*

Moves data between registers. Not all register pairs are valid.

- **A** from **Rx**
`A = Rx`

Assembly: `MOV A Rx`

- **Rx** from **A**
`Rx = A`

Assembly: `MOV Rx A`

- **AB** (bottom/top) from **Rx**
`AB(bottom/top) = Rx`

Assembly: `MOV AB_bot Rx`, `MOV AB_top Rx` 

- **AB** from **R1**, **R2**  *(Pseudoinstruction)*
`AB = {R2,R1}`

Assembly: `MOV AB R1, R2`

- **A** from **IR**
`A = IR`

Assembly: `MOV A IR`

- **IR** from **A**
`IR = A`

*Note: this can only be done in kernel mode*

Assembly: `MOV IR A`

- **SP** from **AB**
`SP = AB`

Assembly: `MOV SP AB`

- **AB** from **SP**
`AB = SP`

Assembly: `MOV AB SP`

#### IMM
*(Load immediate)*

Loads a 4-bit signed immediate into a nibble of **A** or **AB**.
However, the assembler only recognizes the following pseudoinstruction:

- **A** `A = (imm)`

Assembly: `IMM A (imm)`

- **AB** `AB = (imm)`

Assembly: `IMM AB (imm)`

- **R** `R = (imm)`

Assembly: `IMM R (imm)` (Clobbers **A**)
### Memory
For loads/stores, the addressing modes are as follows:

#### Symbol:
Uses the address of the symbol
`Address = Adr(symbol)`

Assembly: `OP dest symbol` (Clobbers **AB**)

#### Relative:
Uses **AB** + **PB** as the address
`Address = AB + PB`

Assembly: `OP dest`

#### Immediate:

Uses an immediate as the address
`Address = *(imm)*` 

Assembly: `OP dest (imm)` (Clobbers **AB**)

#### LD
*(Load)*

Loads from RAM into **A** or **Rx**: `A/Rx = *Address`

Assembly: `LD A Address`, `LD Rx Address`

#### STO
*(Store)*

Stores **A** or **Rx** into RAM at an address: `*Address = A/Rx`

Assembly: `STO A Address`, `STO Rx Address`

#### ADR
*(Load Address)*

Pseudoinstruction that can be used to load the address of a symbol into **AB** or **R1,R2**

- **AB** `AB = address(symbol)`

Assembly: `ADR symbol`

- **R1,R2** `{R2,R1} = address(symbol)`

Assembly: `ADR R1 R2 symbol` (Clobbers **A**)

#### PUSH
*(Push to stack)*

Stores a register (or a byte segment of one) into the address of **SP** and decrements **SP**. Allowed registers are as follows:

- **A** 
`*(SP--) = A`

Assembly: `PUSH A`

- **Rx** 
`*(SP--) = Rx`

Assembly: `PUSH Rx`

- **FR** 
`*(SP--) = FR`

Assembly: `PUSH FR`

- **AB**
Can push either byte of AB, but assembler only recognizes:

`*(SP--) = AB[7:0]`

Assembly: `PUSH AB_bot`

`*(SP--) = AB[7:0], *(SP--) = AB[15:8]`

Assembly: `PUSH AB`

- **RA**
Can push either byte of RA, but assembler only recognizes:

`*(SP--) = RA[7:0]`

Assembly: `PUSH RA_bot`

`*(SP--) = RA[7:0], *(SP--) = RA[15:8]`

Assembly: `PUSH RA`

#### POP
*(Pop from stack)*

Loads a register (or a byte segment of one) from the address of **SP** and increments **SP**. Allowed registers are as follows:

- **A** 
`*A = *(SP++)`

Assembly: `POP A`

- **Rx** 
`*Rx = *(SP++)`

Assembly: `POP Rx`

- **FR** 
`*FR = *(SP++)`

Assembly: `POP FR`

- **AB**
Can pop either byte of AB, but assembler only recognizes:

`AB[7:0] = *(SP++)`

Assembly: `POP AB_bot`

`AB[7:0] = *(SP++), AB[15:8] = *(SP++)`

Assembly: `POP AB`

- **RA**
Can pop either byte of RA, but assembler only recognizes:

`RA[7:0] = *(SP++)`

Assembly: `POP RA_bot`

`RA[7:0] = *(SP++), RA[15:8] = *(SP++)`

Assembly: `POP RA`

## Arithmetic
### Basic
#### ADD
*(Add)*

Adds two registers together. Adding different registers behaves differently, and not all register pairs are valid.

- **A** with **Rx**:
`A += Rx`

Assembly: `ADD A Rx` or `ADD Rx`

This operation also sets **FR** normally.

- **A** with **AB**:
`AB += A`

Assembly:
`ADD AB A`

- Increment **Rx**:
`Rx += (imm)`

Immediate can only be -1 or 1

**Note: this operation sets **FR** normally*

Assembly:
`ADD Rx (imm)`

- Increment **AB**:
`AB += (imm)`

Immediate can only be -1 or 1

Assembly:
`ADD AB (imm)`

#### SUB
*(Subtract)*

Subtracts **Rx** from **A**

`A -= Rx`

Assembly:
`SUB Rx`

This operation also sets **FR** normally.

### Shifts
#### SLA
*(Shift left arithmetic)*

Shifts **A** to the left by the value in **Rx** or a 3-bit immediate and zero-extends

`A = A <<< Rx`

Assembly: `SLA Rx`

`A = A <<< (imm)`

Assembly: `SLA (imm)`

These shifts set **C** = 1 when a 1 is shifted past bit 7. **Note, this only works reliably when the shift is <= 8.*

#### SRA
*(Shift right arithmetic)*

Shifts **A** to the right by the value in **Rx** or a 3-bit immediate and zero-extends

`A = A >>> Rx`

Assembly: `SRA Rx`

`A = A >>> (imm)`

Assembly: `SRA (imm)`

These shifts set **C** = 1 when a 1 is shifted past bit 0. **Note, this only works reliably when the shift is <= 8.*

#### SRL
*(Shift right logical)*

Shifts **A** to the right by the value in **Rx** and sign-extends
`A = A >> Rx`

Assembly: `SRL Rx`

These shifts set **C** = 1 when a 1 is shifted past bit 0. **Note, this only works reliably when the shift is <= 8.*

### Logic
#### AND
*(Bitwise and)*

Performs a bitwise and on **A** with the value in **Rx**
`A &= Rx`

Assembly: `AND Rx`

This operation sets **Z** = 1 when the result is 0.

#### OR
*(Bitwise or)*

Performs a bitwise or on **A** with the value in **Rx**
`A |= Rx`

Assembly: `OR Rx`

This operation sets **Z** = 1 when the result is 0.

#### XOR
*(Bitwise xor)*

Performs a bitwise xor on **A** with the value in **Rx**
`A ^= Rx`

Assembly: `XOR Rx`

This operation sets **Z** = 1 when the result is 0.

## Control Flow
### Branches
For conditional branches, the flags are read from **FR**. For all branches, the addressing modes are as follows:

#### Symbol:

Jumps to the address of the symbol
`PC = Adr(symbol)`

Assembly: `OP symbol` (Clobbers **AB**)

#### Relative:

Jumps to **PC** + **AB**
`PC = PC + AB `

Assembly: `OP REL`

#### Absolute:

Jumps to **AB**
`PC = AB`

Assembly: `OP ABS`

#### Immediate:

Jumps to (imm)
`PC = *(imm)*` 

Assembly: `OP (imm)` (Clobbers **AB**)
#### BEQ
*(Branch on equal)*

Branches on **Z** = 1 and does not save **RA**

`if Z == 1: PC = Address`

Assembly: `BEQ Address`

*Note: Z = 1 when two equal registers are subtracted*
#### BNE
*(Branch on not equal)*

Branches on **Z** = 0 and does not save **RA**

`if Z == 0: PC = Address`

Assembly: `BNE Address`

*Note: Z = 0 when two non-equal registers are subtracted*
#### BLTU
*(Branch on less than unsigned)*

Branches on **C** = 0 and does not save **RA**

`if C == 0: PC = Address`

Assembly: `BLTU Address`

*Note: C = 0 when unsigned values are subtracted where r1 < r2. It is also true when unsigned addition does not carry or when borrow occurs from unsigned subtraction*
#### BGEU
*(Branch on greater than or equal to unsigned)*

Branches on **C** = 1 and does not save **RA**

`if C == 1: PC = Address`

Assembly: `BGEU Address`

*Note: C = 1 when unsigned values are subtracted where r1 >= r2. It is also true when unsigned addition carries or when borrow does not occur from unsigned subtraction*
#### BLTS
*(Branch on less than signed)*

Branches on **N** != **V** and does not save **RA**

`if N != V: PC = Address`

Assembly: `BLTS Address`

*Note: N != V when signed values are subtracted and r1 < r2*
#### BGES
*(Branch on greater than or equal to signed)*

Branches on **N** = **V** and does not save **RA**

`if N == V: PC = Address`

Assembly: `BGES Address`

*Note: N = V when signed values are subtracted and r1 >= r2*
#### J
*(Jump)*

Unconditionally jumps to address and does not save **RA**

`PC = Address`

Assembly: `J Address`
#### CALL
*(Call)*

Unconditionally jumps to address and saves **RA**

`PC = Address, RA = PC + 1`

Assembly: `CALL Address`
### Other
#### RET
*(Return)*

Sets **PC** to **RA**

`PC = RA`

Assembly: `RET`
#### NOP
*(No operation)*

Does nothing. State of all registers and memory is preserved to next cycle.

Assembly: `NOP`
## System
### Privileges
#### KERNEL
*(Kernel mode)*

Puts the CPU into kernel mode.
`mode = kernel`

 A brief summary of kernel mode:

Interrupts are disabled, **IR** can be modified, relative loads/stores are from address 0, SYSCALL is disabled, registers **PB**, **PL**, **IRA**, and **IJA** can be modified, IRET can be called, and there are no segfaults.

Assembly: `KERNEL`

#### USER
*(User mode)*

Puts the CPU into user mode. 
`mode = user`

A brief summary of user mode:

Interrupts are enabled, **IR** cannot be modified except through SYSCALL, relative loads/stores are from address **PB**, registers **PB**, **PL**, **IRA**, and **IJA** cannot be changed, IRET cannot be called, and segfaults occur when relative instructions try to index out-of-bounds data.

Assembly: `USER`

### Programs
#### PB
*(Set **PB**)*

When in kernel mode, sets **PB** to **AB**
`PB = AB`

Assembly: `PB` or `MV PB AB`

#### PL
*(Set **PL**)*

When in kernel mode, sets **PL** to **AB**
`PL = AB`

Assembly: `PL` or `MV PL AB`

### Interrupts
#### IJA
*(Set **IJA**)*

When in kernel mode, sets **IJA** to **AB**
`IJA = AB`

Assembly: `IJA` or `MV IJA AB`

#### IRA
*(Set **IRA**)*

When in kernel mode, sets **IRA** to **AB**
`IRA = AB`

Assembly: `IRA` or `MV IRA AB`

#### IRET
*(Interrupt return)*

When in kernel mode, jumps to the address in **IRA** and sets mode to user
`PC = IRA, mode = user`

Assembly: `IRET`

#### SYSCALL
*(System call)*

When in user mode, sets **IR[0]**, thus triggering an interrupt
`IR[0] = 1`

Assembly: `SYSCALL`

# Appendix
## Encoding Tables
`MV A Rx`, `MV Rx A`
|{7:3}|{2}|{1:0}|
|-|-|-|
|11000|Direction (0 = to A, 1 = to Rx)|Register #

`MV AB Rx`
|{7:3}|{2}|{1:0}|
|-|-|-|
|11011|Segment (0 = bottom, 1 = top) |Register #

`MV A IR`, `MV IR A`
|{7:1}|{0}|
|-|-|
|1110111|Direction (0 = to A, 1 = to IR)|

`MV AB SP`, `MV SP AB`
|{7:1}|{0}|
|-|-|
|1111100|Direction (0 = to SP, 1 = to AB)|

`IMM A (imm)`
|{7:5}|{4}|{3:0}|
|-|-|-|
|010|Segment (0 = bottom, 1 = top) | Immediate

`IMM AB (imm)`
|{7:6}|{5:4}|{3:0}|
|-|-|-|
|00|Segment (00 = bottom, 11 = top) | Immediate

`LD A`, `STO A` (Relative to **PB**)
|{7:1}|{0}|
|-|-|
|1110100|Direction (0 = to A, 1 = to RAM)|

`LD A`, `STO A` (Absolute)
|{7:1}|{0}|
|-|-|
|1111000|Direction (0 = to A, 1 = to RAM)|

`LD Rx`, `STO Rx` (Relative to **PB**)
|{7:3}|{2}|{1:0}|
|-|-|-|
|11100|Direction (0 = to Rx, 1 = to RAM)|Register #

`LD Rx`, `STO Rx` (Absolute)
|{7:3}|{2}|{1:0}|
|-|-|-|
|11001|Direction (0 = to Rx, 1 = to RAM)|Register #

`PSH A`, `POP A`
|{7:1}|{0}|
|-|-|
|1111001|Direction (0 = pop, 1 = push)|

`PSH Rx`, `POP Rx`
|{7:3}|{2}|{1:0}|
|-|-|-|
|10110|Direction (0 = pop, 1 = push)|Register #

`PSH FR`, `POP FR`
|{7:1}|{0}|
|-|-|
|1111010|Direction (0 = pop, 1 = push)|

`PSH RA`, `POP RA`, `PSH AB`, `POP AB`
|{7:3}|{2}|{1}|{0}|
|-|-|-|-|
|10111|Direction (0 = pop, 1 = push)|Destination (0 = RA, 1 = AB) | Segment (0 = bottom, 1 = top)

`ADD A Rx`, `SUB Rx`, `SLA Rx`, `SRA Rx`, `SRL Rx`, `AND Rx`, `OR Rx`, `XOR Rx`
|{7:5}|{4:2}|{1:0}|
|-|-|-|
|100|Operation (000 = add, 001 = subtract, 010 = shift left arithmetic, 011 = shift right arithmetic, 100 = shift right logical, 101 = bitwise and, 110 = bitwise or, 111 = bitwise xor)|Register #

`SLA (imm)`, `SRA (imm)`
|{7:4}|{3}|{2:0}|
|-|-|-|
|1010| Direction (0 = left, 1 = right)|Immediate

`ADD AB A`
|{7:0}|
|-|
|11111110|

`ADD R (imm)`
|{7:3}|{2}|{1:0}|
|-|-|-|
|11010|Immediate (0 = -1, 1 = 1)|Register #

`ADD AB (imm)`
|{7:1}|{0}|
|-|-|
|1111011|Immediate (0 = -1, 1 = 1)|

`BEQ Address`, `BNE Address`, `BLTU Address`, `BGEU Address`, `BLTS Address`,  `BGES Address`, `J Address`, `CALL Address`
|{7:5}|{4:2}|{1}|{0}|
|-|-|-|-|
|011|Condition (000 = EQ, 001 = NEQ, 010 = LTU, 011 = GEU, 100 = LTS, 101 = GES, 110 = unused, 111 = always)|Absolute/Relative (0 = Absolute, 1 = Relative to PC) | Link (0 = Do not link RA, 1 = link RA)

`RET`
|{7:0}|
|-|
|11111010|

`NOP`
|{7:0}|
|-|
|11111100|

`KERNEL`
|{7:0}|
|-|
|11101010|

`USER`
|{7:0}|
|-|
|11101011|

`PB`
|{7:0}|
|-|
|11101100|

`PL`
|{7:0}|
|-|
|11111011|

`IJA`
|{7:0}|
|-|
|11111111|

`IRA`
|{7:0}|
|-|
|11111101|

`IRET`
|{7:0}|
|-|
|11101101|

`SYSCALL`
|{7:0}|
|-|
|11101111|


## Assembly
### Instructions
All instructions start with the operation (uppercase), and then varying amounts of parameters. There is currently no support for illegal operations/parameters, so be careful.

### Symbols
Symbols must start with a lowercase letter and must not contain "//" or a space. When declaring a symbol, use a ":" in front, but when calling it, omit this. Example:

```
CALL symbol

:symbol
RET
```

### Immediates
All immediates (except in SLA and SRA) are interpreted as signed. Immediates can be in decimal (no prefix), hex (prefixed by $), or binary (prefixed by %). However, increments to registers will only accept "-1" and "1" Example:

```
IMM A -10
IMM AB $ffff
STO R0 %1111
ADD AB -1
```

### Data
There are two types of data that are recognized:
- Values

Can use one or two bytes in the same format as immediates. 

- Strings

Must be prefixed by an apostrophe. Newline is represented by "\n", null is represented by "\0", and a backslash is represented by "\\". Trailing spaces will be ignored.

Example:
```
:byte
23
:address
$ffff
:string
'Hello World!
```

### Comments
Comments must be prefixed by "//". They can be on their own line, or at the end of a line (with whitespace separating them)

Example:
```
//This comment has its own line
ADD A R0 //This comment is inline with an instruction
```

### Example Program (Hello World)
```
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
'Hello World!\0\
```
