
/*Description: This module counts until specified number, once reached it asserts "done" output, taking as reference 2MHz clock*/



module counterSW
  #(parameter multiplier = 2, UnitTime = 1000)    //don't change unless input clk freq(2MHz) changes   //for 1 mSec (unit in usecs)
  (
   input      iClk,iRst_n,enable,
   output reg done   //Done signal given by when max count is done
   );


   
   


   //////////////////////////////////////////////////////////////////////////////////
  // Parameters
   //////////////////////////////////////////////////////////////////////////////////
   //localparam multiplier = 2; //Don't change this unless clk freq(2MHz) is changed 
   //localparam UnitTime = 1000;// Units in us  -- max=65ms, min=1us
   localparam target = UnitTime*multiplier;
   localparam LOW  = 1'b0;
   localparam HIGH = 1'b1;

   reg [19:0] cnt; 
   
   //This block generates a divided clock multiple of reference clock (2MHz). 
   //Input is 2MHz and Output is done signal when timer max is reached. 
   
   always @(posedge iClk, negedge iRst_n)  //making it asynch reset by PSG recommendation
     begin
	    if (!iRst_n) 
          begin
		     cnt <= 12'h0;
		     done <= LOW;
	      end   
        else
          begin
             if (enable)
               begin
                  if(cnt < target)
                    begin
		               cnt <= cnt + 12'h1;
		               done <= LOW;
	                end
	              else
                    begin 
		               cnt <= 12'h0;
		               done <= HIGH;		
	                end
               end // if (enable)
             else
               begin
                  cnt <= 12'h0;
		          done <= LOW;
               end // else: !if(enable)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk)
   
endmodule // counterSW

