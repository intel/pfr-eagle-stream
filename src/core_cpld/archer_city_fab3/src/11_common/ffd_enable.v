/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Brief    : parametrized Flip-flop with enable
//Details  : Provides a parametrized output coming from a D type Flip-Flop, with active high enable and a parametrized reset value
//           using rising edge of the clock signal and an active-low reset
//Author   : a.larios@intel.com
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

module ffd_enable
  #(
    parameter RST_VALUE = 0,    //reset value
    parameter WIDTH = 1         //number of FFs required
    )
   
   (
    input iClk,               //clock input
    input iRst_n,             //active low asynch reset
    input iEna,                //active high synch enable
    input [WIDTH-1 : 0] iD,   //input data D
    output reg [WIDTH-1 : 0] oQ    //output data Q
    );


   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             oQ <= RST_VALUE;
          end
        else
          begin
             if (iEna == 1'b1)
               oQ <= iD;
             else
               oQ <= oQ;
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   
endmodule // ff_enable
