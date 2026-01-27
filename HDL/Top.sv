module Top (input logic MAX10_CLK1_50, 
input logic [7:0] SW,
output logic [7:0] LEDR
);

logic clk, we;
logic [1:0] state = 0;
logic [7:0] R [3:0];
logic [7:0] A, FR, instruction, din, dout;
logic [7:0] IR = 8'b0;
logic [15:0] PC = 0;
logic [15:0] AB, RA, SP, IJA, IRA, PB, PL, addr;
logic branch;
logic mode = 0;
logic [7:0] ALUout, shiftedOut;

logic timer1Int, timer2Int, keyInt;
logic i0 = 0;
logic i1 = 0;
logic i2 = 0;

logic [7:0] timer1Ctrl, timer2Ctrl, timer1Set, timer2Set, timer1Read, timer2Read; 

timer timer1(clk, timer1Ctrl, timer1Set, timer1Read, timer1Int);
timer timer2(clk, timer2Ctrl, timer2Set, timer2Read, timer2Int);

assign clk = MAX10_CLK1_50;

(* ramstyle = "M10K" *) logic [7:0] RAM [1023:0];

initial begin
    $readmemb("../assembler/tests/timertest.bin", RAM);
end

always_comb begin
	
	ALUout = 0;
	branch = 0;
	addr = AB;
	we = 0;
	din = 0;
	shiftedOut = 0;
	if (state == 0)
		addr = PC;
	if (state == 2) begin
		casez(instruction)
		8'b011?????: 								//Branch
			case(instruction[4:2])
			3'b000:
				branch = FR[0] == 1;
			3'b001:
				branch = FR[0] == 0;
			3'b010:
				branch = FR[2] == 0;
			3'b011:
				branch = FR[1] != FR[3];
			3'b100:
				branch = FR[2] == 1;
			3'b101:
				branch = FR[1] == FR[3];
			3'b111:
				branch = 1;
			default:
				branch = 0;
			endcase
		8'b100?????:								//ALU ops
			case(instruction[4:2])
			3'b000:
				ALUout = A + R[instruction[1:0]];
			3'b001:
				ALUout = A - R[instruction[1:0]];
			3'b010:
				ALUout = A <<< R[instruction[1:0]];
			3'b011:
				ALUout = A >>> R[instruction[1:0]];
			3'b100:
				ALUout = A >> R[instruction[1:0]];
			3'b101:
				ALUout = A & R[instruction[1:0]];
			3'b110:
				ALUout = A | R[instruction[1:0]];
			3'b111:
				ALUout = A ^ R[instruction[1:0]];
			default:
				ALUout = 0;
			endcase
		8'b10100???:								//<<</>> IMM
			shiftedOut = A >> R[instruction[1:0]];
		8'b10101???:
			shiftedOut = A << R[instruction[1:0]];
		8'b101100??: 								//R <-> S
			addr = SP;
		8'b101101??: begin
			we = 1;
			addr = SP - 1;
			din = R[instruction[1:0]]; end
		8'b101110??:								//RA/AB <-> S
			addr = SP;
		8'b10111100: begin						
			we = 1;
			addr = SP - 1;
			din = RA[7:0]; end
		8'b10111101:  begin
			we = 1;
			addr = SP - 1;
			din = RA[15:8]; end
		8'b10111110: begin						
			we = 1;
			addr = SP - 1;
			din = AB[7:0]; end
		8'b10111111: begin
			we = 1;
			addr = SP - 1;
			din = AB[15:8]; end
		8'b110010??:								//R <-> [AB]
			addr = AB;
		8'b110011??: begin
			we = 1;
			addr = AB;
			din = R[instruction[1:0]]; end
		8'b11110000:								//A <-> [AB]
			addr = AB;
		8'b11110001: begin								
			we = 1;
			addr = AB;
			din = A; end
		8'b111000??: begin						//R <-> [AB + PB]										
			if (mode == 1)
				addr = AB + PB;
			else
				addr = AB; end
		8'b111001??: begin																		
			if (!((PB + AB > PL || PB + AB < PB) && mode == 1))
				we = 1;
			if (mode == 1)
				addr = AB + PB;
			else
				addr = AB;
			din = R[instruction[1:0]]; end
		8'b11101000: begin						//A <-> [AB + PB]
			if (mode == 1)
				addr = AB + PB;
			else
				addr = AB; end
		8'b11101001: begin							
			if (!((PB + AB > PL || PB + AB < PB) && mode == 1))
				we = 1;
			if (mode == 1)
				addr = AB + PB;
			else
				addr = AB;
			din = A; end
		8'b11110001: begin						//A <-> [AB]
			we = 1;
			addr = AB;
			din = A; end
		8'b11110000: begin
			addr = AB; end
		8'b11110010:								//A <-> S
			addr = SP;
		8'b11110011: begin
			we = 1;
			addr = SP - 1;
			din = A; end
		8'b11110100:								//FR <-> S
			addr = SP;
		8'b11110101: begin
			we = 1;
			addr = SP - 1;
			din = FR; end
		default:
			ALUout = 0;
		endcase
	end
end

always_ff @(posedge clk) begin
	//trigger interrupt flags
	if (timer1Int && i0 == 0)
		i0 <= 1;
	if (timer2Int && i1 == 0)
		i1 <= 1;
	if (keyInt && i2 == 0)
		i2 <= 1;
		
	if (state == 1) begin
		//Load instruction
		instruction <= dout;
		
		//trigger interrupts
		if (i0) begin
			IR[2] <= 1;
			i0 <= 0; end
		if (i1) begin
			IR[3] <= 1;
			i1 <= 0; end
		if (i2) begin
			IR[4] <= 1;
			i2 <= 0; end
	end

	//Main execution
	else if (state == 2) begin
		if (IR != 0 && mode == 1) begin //interrupt!
			PC <= IJA;
			IRA <= PC;
			mode <= 0;
		end
		
		else begin
		casez(instruction)
		8'b0000????: 								//IMM AB
			AB <= {{12{instruction[3]}},instruction[3:0]};
		8'b0001????:	
			AB <= {{8{instruction[3]}},instruction[3:0],AB[3:0]};
		8'b0010????:	
			AB <= {{4{instruction[3]}},instruction[3:0],AB[7:0]};
		8'b0011????:	
			AB <= {instruction[3:0],AB[11:0]};
		8'b0100????: 								//IMM A
			A <= {{4{instruction[3]}},instruction[3:0]};
		8'b0101????:
			A <= {instruction[3:0],A[3:0]};
		8'b011?????: begin 								//Branch
			if (branch) begin
				if(instruction[1] == 0)
					PC <= AB;
				else begin
				if (((PC + AB > PL || PC + AB < PB) && mode == 1))
					IR[1] <= 1;
				else
					PC <= PC + AB;
				end
				if(instruction[0] == 1)
					RA <= PC+1;
			end
			else
				PC <= PC + 1; end
		8'b100?????: begin								//ALU ops
			A <= ALUout;
			case(instruction[4:2])
			3'b000: begin
				FR[0] <= ALUout == 0;
				FR[1] <= ALUout[7];
				FR[2] <= ALUout < R[instruction[1:0]];
				FR[3] <= (A[7] == R[instruction[1:0]][7]) && (ALUout[7] != A[7]);
			end
			3'b001: begin
				FR[0] <= ALUout == 0;
				FR[1] <= ALUout[7];
				FR[2] <= A >= R[instruction[1:0]];
				FR[3] <= (A[7] != R[instruction[1:0]][7]) && (ALUout[7] != A[7]);
			end
			endcase
		end
		8'b10100???: begin								//<<</>> IMM
			A <= A <<< instruction[2:0];
			FR[2] <= |shiftedOut; end
		8'b10101???: begin
			A <= A >>> instruction[2:0];
			FR[2] <= |shiftedOut; end
		8'b101100??: begin 						//R <-> S
			R[instruction[1:0]] <= dout;
			SP <= SP + 1; end
		8'b101101??:
			SP <= SP - 1;
		8'b10111000: begin						//RA/AB <-> S
			RA <= {{8{dout}},dout};
			SP <= SP + 1; end
		8'b10111001: begin
			RA <= {dout,RA[7:0]};
			SP <= SP + 1; end
		8'b10111010: begin						
			AB <= {{8{dout}},dout};
			SP <= SP + 1; end
		8'b10111011: begin
			AB <= {dout,AB[7:0]};
			SP <= SP + 1; end
		8'b101111??:						
			SP <= SP - 1;
		8'b110000??:								//A <-> R
			A <= R[instruction[1:0]];
		8'b110001??:
			R[instruction[1:0]] <= A;
		8'b110010??:								//R <-> [AB]
			R[instruction[1:0]] <= dout;
		8'b110011??:
			A <= A;								
		8'b110100??:								//R -/+ 1
			R[instruction[1:0]] <= R[instruction[1:0]] - 1;
		8'b110101??:								
			R[instruction[1:0]] <= R[instruction[1:0]] + 1;
		8'b110110??:								//R -> AB
			AB <= {{8{R[instruction[1:0]][7]}},R[instruction[1:0]]};
		8'b110111??:
			AB <= {R[instruction[1:0]],AB[7:0]};
		8'b111000??: begin						//R <-> [AB + PB]										
			if (((PB + AB > PL || PB + AB < PB) && mode == 1))
				IR[1] <= 1;
			else
				R[instruction[1:0]] <= dout;	
			end
		8'b111001??: begin																			
			if (((PB + AB > PL || PB + AB < PB) && mode == 1))
				IR[1] <= 1;
			end
		8'b11101000: begin								//A <-> [AB + PB]
			if (((PB + AB > PL || PB + AB < PB) && mode == 1))
				IR[1] <= 1;
			else
				A <= dout;	
			end
		8'b11101001: begin						
			if (((PB + AB > PL || PB + AB < PB) && mode == 1))
				IR[1] <= 1;
			end
		8'b11101010:								//Kernel
			mode = 0;
		8'b11101011:								//User
			mode = 1'b1;
		8'b11101100:								//PB <- AB
			PB <= AB;
		8'b11101101: begin						//Interrupt ret
			PC <= IRA;
			mode <= 1; end
		8'b11101110:								//A <-> IR
			if (mode == 0) 
				A <= IR;
		8'b11101111:
			if (mode == 0) 
				IR <= A;
			else
				IR[0] <= 1;
		8'b11110000:								//A <-> [AB]
			A <= dout;
		8'b11110001:
			A <= A;
		8'b11110010: begin						//A <-> S
			A <= dout;
			SP <= SP + 1; end
		8'b11110011:
			SP <= SP - 1;
		8'b11110100: begin						//FR <-> S
			FR <= dout;
			SP <= SP + 1; end
		8'b11110101:
			SP <= SP - 1;
		8'b11110110:								//AB -/+ 1
			AB <= AB - 1;
		8'b11110111:								
			AB <= AB + 1;
		8'b11111000:								//SP <-> AB
			SP <= AB;
		8'b11111001:
			AB <= SP;
		8'b11111010:								//RA -> PC
			PC <= RA;
		8'b11111011:								//PL <- AB
			PL <= AB;
		8'b11111100:								//NOP
			A <= A;
		8'b11111101:								//IRA <- AB
			IRA <= AB;
		8'b11111110:								//AB += A
			AB <= AB + {{8{A[7]}},A};
		8'b11111111:								//AB -> IJA
			IJA <= AB; 
		default:
			A <= A;
		endcase
		
		if (instruction[7:5] != 3'b011 && instruction != 8'b11111010 && instruction != 8'b11101101)
			PC <= PC + 1;
		end
	end
	
	//Memory
	if (we) begin
		if (addr < 900)
			RAM[addr] <= din;
		else if (addr == 999)
			LEDR <= din;
		else if (addr == 990)
			timer1Ctrl <= din;
		else if (addr == 991)
			timer1Set <= din;
		else if (addr == 993)
			timer2Ctrl <= din;
		else if (addr == 994)
			timer2Set <= din;
	end
	if (addr < 900)
		dout <= RAM[addr];
	else if (addr == 998)
		dout <= SW;
	else if (addr == 992)
		dout <= timer1Read;
	else if (addr == 995)
		dout <= timer2Read;
		
	state <= state + 1;
end

//always_ff @(posedge SW)
//IR <= 8'b00000001;


endmodule