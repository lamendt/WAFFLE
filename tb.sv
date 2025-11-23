module tb;
    logic clk = 0;
	 logic [7:0] SW = 0;
	 logic [7:0] LED;
    always #5 clk = ~clk;
	 always #100 SW = ~SW;
    Top dut (clk, SW, LED);
endmodule