
/*Description: This module counts until specified number, once reached it asserts "done" output, taking as reference 2MHz clock*/



module counterSW(
input iClk,iRst_n,enable,
output reg done   //Done signal given by when max count is done
);

reg [15:0] cnt; 




//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter multiplier = 2; //Don't change this unless clk freq(2MHz) is changed 
parameter UnitTime = 1000;// Units in us  -- max=65ms, min=1us
parameter target = UnitTime*multiplier;
parameter  LOW  = 1'b0;
parameter  HIGH = 1'b1;

//This block generates a divided clock multiple of reference clock (2MHz). 
//Input is 2MHz and Output is done signal when timer max is reached. 

always @(posedge iClk) begin
	if (!iRst_n || !enable) begin
		cnt <= 16'h0;
		done <= LOW;
	end   
   else if(enable && (cnt < target))
   begin
		cnt <= cnt + 16'h1;
		done <= LOW;
	end
	else begin 
		cnt <= 16'h0;
		done <= HIGH;		
		end
end


endmodule
