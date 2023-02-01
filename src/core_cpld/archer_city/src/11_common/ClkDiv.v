
module ClkDiv
  #(parameter DIV = 249999) //based on a 2MHz clk, this value would deliver a 250msec periodic signal
    (
    input iClk,   //2MHz clk
    input iRst_n, //active low reset
     
    output reg div_clk  //divided clock output at 250msec (based on a 2MHz clock freq)
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
   
   always @(posedge iClk)
     begin
        if (!iRst_n)
          begin
             div_clk <= 1'b0;
             rCnt <= 0;
          end
        else
          begin
             if (rCnt == DIV)               //when counter reaches the top count specified in DIV param
               begin
                  div_clk <= ~div_clk;      //we toggle the divided clock signal
                  rCnt <= 0;                //and reset the counter for the next toggle
               end
             else
               begin
                  div_clk <= div_clk;       //otherwise, we increment the count and keep the divided clock value steady
                  rCnt <= rCnt + 1'b1;          
               end
          end
     end // always @ (posedge iClk)
endmodule // ClkDiv

   
   