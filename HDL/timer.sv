module timer(input logic clk, 
input logic [7:0] control,
input logic [7:0] set,
output logic [7:0] read,
output logic interrupt);

logic seg, we, intEn, periodClk;
logic periodCount = 0;
logic [4:0] period;
logic [15:0] value = 16'hffff;
logic [15:0] setVal = 0;
logic [15:0] resetVal = 16'hffff;

always_comb begin
{period,intEn,we,seg} = control[7:0];
if (seg == 0)
	read = value[7:0];
else
	read = value[15:8];
end

always_ff @(posedge clk) begin
if (periodClk)	begin
	if (value != 0) begin
		if (value <= resetVal)
			value <= value - 1;
		else
			value <= resetVal;
		interrupt <= 0;
	end
	else begin
		if (intEn)
			interrupt <= 1;
		value <= resetVal;
	end
end
else
	interrupt <= 0;
	
if (seg == 1)
	setVal[15:8] <= set;
else
	setVal[7:0] <= set;

if (we)
	resetVal <= setVal;

if (period <= periodCount) begin
	periodClk <= 1;
	periodCount <= 0;
end
else begin
	periodClk <= 0;
	periodCount <= periodCount + 1;
end

end

endmodule