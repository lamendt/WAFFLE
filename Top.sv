module Top (input logic MAX10_CLK1_50, 
input logic [7:0] SW,
output logic [7:0] LEDR
);

logic clk, we;
logic [1:0] state = 0;
logic [7:0] R [3:0];
logic [7:0] A, FR, IF, instruction, din, dout;
logic [7:0] IR = 8'b0;
logic [15:0] PC = 0;
logic [15:0] AB, RA, SP, IJA, IRA, addr;
logic branch;
logic [7:0] ALUout;

assign clk = MAX10_CLK1_50;

(* ramstyle = "M10K" *) logic [7:0] RAM [1023:0];

initial begin
    $readmemb("testcode/for.bin", RAM);
end

always_comb begin
	ALUout = 0;
	branch = 0;
	addr = AB;
	we = 0;
	din = 0;
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
		8'b111000??:								//R <-> [AB + PC]										
			addr = AB + PC;
		8'b111001??: begin																		
			we = 1;
			addr = AB + PC;
			din = R[instruction[1:0]]; end
		8'b11101000:								//A <-> [AB + PC]
			addr = AB + PC;
		8'b11101001: begin							
			we = 1;
			addr = AB + PC;
			din = A; end
		8'b11110001: begin
			we = 1;
			addr = AB;
			din = A; end
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
	//Load instruction
	if (state == 1)
		instruction <= dout;

	//Main execution
	if (state == 2) begin
		if (IR != 0) begin //interrupt!
			PC <= IJA;
			IRA <= PC;
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
				else
					PC <= PC + AB;
				if(instruction[0] == 1)
					RA <= PC+1;
			end
			else
				PC <= PC + 1; end
		8'b100?????: begin								//ALU ops
			A <= ALUout;
			if(instruction[4:2] == 3'b000) begin
				FR[0] <= ALUout == 0;
				FR[1] <= ALUout[7];
				FR[2] <= ALUout < R[instruction[1:0]];
				FR[3] <= (A[7] == R[instruction[1:0]][7]) && (ALUout[7] != A[7]);
			end
			else if(instruction[4:2] == 3'b001) begin
				FR[0] <= ALUout == 0;
				FR[1] <= ALUout[7];
				FR[2] <= A >= R[instruction[1:0]];
				FR[3] <= (A[7] != R[instruction[1:0]][7]) && (ALUout[7] != A[7]);
			end end
		8'b10100???:								//<<</>> IMM
			A <= A <<< instruction[2:0];
		8'b10101???:
			A <= A >>> instruction[2:0];
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
		8'b111000??:								//R <-> [AB + PC]										
			R[instruction[1:0]] <= dout;
		8'b111001??:																			
			A <= A;
		8'b11101000:								//A <-> [AB + PC]
			A <= dout;
		8'b11101001:							
			A <= A;
		8'b11101010:								//PC <- IRA
			PC <= IRA;
		8'b11101011:								//Unused
			A <= A;
		8'b11101100:								//A -> AB
			AB <= {{8{A[7]}},A};
		8'b11101101:
			AB <= {A,AB[7:0]};
		8'b11101110:								//A <-> IR
			A <= IR;
		8'b11101111:
			IR <= A;
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
		8'b11111011:								//PC -> AB
			AB <= PC;
		8'b11111100:								//NOP
			A <= A;
		8'b11111101:								//HALT 
			A <= A;
		8'b11111110:								//AB += A
			AB <= AB + {{8{A[7]}},A};
		8'b11111111:								//AB -> IJA
			IJA <= AB; 
		default:
			A <= A;
		endcase
		
		if (instruction[7:5] != 3'b011 && instruction != 8'b11111010)
			PC <= PC + 1;
		end
	end
	
	//Memory
	if (we) begin
		if (addr < 900)
			RAM[addr] <= din;
		else if (addr == 999)
			LEDR <= din;
	end
	if (addr < 900)
		dout <= RAM[addr];
	else if (addr == 998)
		dout <= SW;
		
	state <= state + 1;
end

//always_ff @(posedge SW)
//IR <= 8'b00000001;


endmodule