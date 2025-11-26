module tb;
    logic clk = 0;
	 logic [7:0] SW = 0;
	 logic [7:0] LED;
    always #1 clk = ~clk;
	 always #147 SW = ~SW;
    Top dut (clk, SW, LED);
endmodule