module RAM (input logic clk, we,
input logic [15:0] addr,
input logic [7:0] din,
output logic [7:0] dout);

(* ramstyle = "M10K" *) logic [7:0] RAM [1023:0];

initial begin
    $readmemb("for.bin", RAM);
end

always_ff @(posedge clk) begin
if (we)
	RAM[addr] <= din;
dout <= RAM[addr];
end

endmodule