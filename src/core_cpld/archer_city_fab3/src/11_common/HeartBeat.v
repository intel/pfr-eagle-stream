
//This module is intended to generate a freq divider of the input Clk to use as an FPGA output
//(HeartBeat signal, for instance)
//Not intended to use as a slower frequency clock as that it may cause timing issues inside the FPGA

module HeartBeat
  #(parameter DIV = 249999) //based on a 2MHz clk, this value would deliver a 250msec periodic signal
    (
    input iClk,   //2MHz clk
    input iRst_n, //active low reset
     
    output reg oHeartBeat  //HeartBeat output at DIV*clk_period secs (based on a 2MHz clock freq => 250M * .5nsec = 125msec)
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
        if (!iRst_n)                      //asynch active-low reset condition
          begin
             oHeartBeat <= 1'b0;
             rCnt <= 0;
          end
        else
          begin
             if (rCnt == DIV)               //when counter reaches the top count specified in DIV param
               begin
                  oHeartBeat <= ~oHeartBeat;      //we toggle the HeartBeat signal
                  rCnt <= 0;                //and reset the counter for the next toggle
               end
             else
               begin
                  oHeartBeat <= oHeartBeat;       //otherwise, we increment the count and keep the HeartBeat value steady
                  rCnt <= rCnt + 1'b1;          
               end
          end
     end // always @ (posedge iClk)
endmodule // ClkDiv

   
   