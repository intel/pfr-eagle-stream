//This module is to generate not really a clock, but a enable pulse of a iClk period width
//with a desired frequency specified by the 1/(DIV param * iClk_period)

//Intended for logic that need to run at a fraction of iClk and instead of generating an internal clk signal
//(not coming from an internal PLL or an external clk source), and also sync'ed with iClk

module ClkDiv
  #(parameter DIV = 249999) //based on a 2MHz clk, this value would deliver a 125msec periodic signal
    (
    input iClk,   //2MHz clk
    input iRst_n, //active low reset
     
    output reg div_clk  //output pulse at DIV*clk_period secs : i.e. for a 2MHz clock freq => 250M * 500 ns = 125msecs 
     );
   
   ///////////////////////////////
   //// local params declaration
   /////////////////////////////////////////////////////////////////////////////////////
         
   localparam  CNT_SIZE = clog2(DIV);


   ///////////////////////////////
   //// local function
   /////////////////////////////////////////////////////////////////////////////////////
   function integer clog2;
      input    integer value;
      begin
         value = value-1;
         for (clog2=0; value>0; clog2=clog2+1)
           value = value>>1;
      end
   endfunction

   ///////////////////////////////
   //// internal signals declaration
   /////////////////////////////////////////////////////////////////////////////////////
         
   reg [CNT_SIZE-1:0] rCnt;      //counter for the divider

   ///////////////////////////////
   //// clock divider always module
   /////////////////////////////////////////////////////////////////////////////////////
   
   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)                //asynch active-low reset
          begin
             div_clk <= 1'b0; 
             rCnt <= 0;
          end
        else
          begin
             if (rCnt == DIV)           //when counter reaches the top count specified in DIV param
               begin
                  div_clk <= 1'b1;      //we toggle the divided clock signal
                  rCnt <= 0;            //and reset the counter for the next toggle
               end
             else
               begin
                  div_clk <= 1'b0;       //otherwise, we increment the count and keep the divided clock value steady
                  rCnt <= rCnt + 1'b1;          
               end
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk)
endmodule // ClkDiv

   
   