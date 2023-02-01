
/*Description: This module counts until specified number, once reached it asserts "done" output, taking as reference 2MHz clock*/



module counterPSU(
input iClk,iRst_n,enable,
input isel,
output reg done
);

reg [20:0] cnt; 

wire [20:0] targetF;


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW  = 1'b0;
parameter  HIGH = 1'b1;

//This block generates a divided clock multiple of reference clock (2MHz). 
//The freq of divided clock is given by counter
//Input is 2MHz and Output is done signal (timer)

always @(posedge iClk) begin
	if (!iRst_n || !enable) begin
		cnt <= 21'h0;
		done <= LOW;
	end   
   else if(enable && (cnt < targetF))
   begin
		cnt <= cnt + 21'h1;
		done <= LOW;
	end
	else begin 
		cnt <= 21'h0;
		done <= HIGH;		
		end
end


 assign targetF = (isel) ? 21'd100000:21'd2000000; //50ms:1s

endmodule
