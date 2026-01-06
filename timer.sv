module timer(input logic clk, 
input logic [7:0] control
input logic [7:0] set,
output logic [7:0] read,
output logic interrupt);

logic seg, we, intEn, periodClk;
logic periodCount = 0;
logic [4:0] period;
logic [15:0] value, resetVal, setVal;

always_comb
{period,intEn,we,seg} = control[7:0];
if (seg == 0)
	read = setVal[7:0];
else
	read = setVal[15:8];
end

always_ff @(posedge clk)
if (!we) begin
	if (periodClk)	begin
		if (value != 0) begin
			value <= value - 1;
			interrupt <= 0;
		end
		else begin
			interrupt <= 1;
			value <= resetVal;
		end
	end
	if (seg == 0)
		setVal[7:0] <= set;
	else
		setVal[15:8] <= set;
end
else begin
	resetVal <= setVal;
	interrupt <= 0;
end

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