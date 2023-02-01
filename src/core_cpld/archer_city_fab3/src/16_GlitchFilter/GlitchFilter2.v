//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Glitch Filter</b>\n
    \details    3-tab sycn, with a glitchfilter (comparator between tab1 and 2)\n
    \file       GlitchFilter2.v
    \author     a.larios@intel..com
    \date       Aug 24th, 2021
    \brief      $RCSfile: GlitchFilter2.v.rca $
                $Date: Tue Aug 24 16:48:00 2021 $
                $Author: alarios $
                $Revision: 1.0 $
                $Aliases: ArcherCity$
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Block Diagram:</b>
    \verbatim
        +---------------------------------+
 -----> |> iClk     .   oFilteredSignals  |----->
 -----> |  iRst_n   .       .       .     |
 -----> |  iSignal  .       .       .     |
        +---------------------------------+
                   GlitchFilter2
    \endverbatim

    \version
                20120916 \b alarios - File creation\n
    \copyright Intel Proprietary -- Copyright 2021 Intel -- All rights reserved
*/

module GlitchFilter2 #(
	parameter	NUMBER_OF_SIGNALS	= 1,
    parameter   RST_VALUE = {NUMBER_OF_SIGNALS{1'b0}})       //reset value for all filter stages
  (
    input                            iClk, //% Clock Input<br>
    input                            iARst_n,//% Asynchronous Reset Input<br>
    input                            iSRst_n, //Synchronous Reset Input
    input                            iEna, //%enable signal (only executes when this is HIGH)
    input [NUMBER_OF_SIGNALS-1 : 0]  iSignal,//% Input Signals<br>
    output [NUMBER_OF_SIGNALS-1 : 0] oFilteredSignals//%Glitchless Signal<br>
   );
   
   //Internal signals
   reg [NUMBER_OF_SIGNALS-1 : 0]     rFilter;
   reg [NUMBER_OF_SIGNALS-1 : 0]     rFilter2;
   reg [NUMBER_OF_SIGNALS-1 : 0]     rFilteredSignals;

   integer                           i;
   
   
   always @(posedge iClk, negedge iARst_n) begin
      if (!iARst_n) begin //asynch active-low Reset condition
         rFilter            <= RST_VALUE;     //{NUMBER_OF_SIGNALS{1'b0}};
         rFilter2           <= RST_VALUE;     //{NUMBER_OF_SIGNALS{1'b0}};
         rFilteredSignals   <= RST_VALUE;     //{NUMBER_OF_SIGNALS{1'b0}};
      end
      else begin
         if (!iSRst_n)
           begin
              rFilteredSignals <= RST_VALUE;
              rFilter          <= RST_VALUE;
              rFilter2         <= RST_VALUE;
           end
         else if (iEna)          //if this module requires a slower than core clock, we generate a pulse with proper 
           begin            //frequency and feed to iEna input signal, otherwise it can be HIGH all the time
	          rFilter <= iSignal; //Input signal flip flop
              rFilter2 <= rFilter;

              for (i=0; i<=NUMBER_OF_SIGNALS-1; i=i+1)
                begin
	               if (rFilter2[i] == rFilter[i]) //if previous and current signal are the same output is enabled
                     begin 
		                rFilteredSignals[i] <= rFilter2[i];    //3rd tab only assigned if tab1 and tab2 are the same value at sampling edge
                     end
	            end
           end // if (iEna)
         else
           begin
              rFilteredSignals <= rFilteredSignals;
           end
         
      end // else: !if(!iARst_n)
   end // always @ (posedge iClk, negedge iARst_n)
   
   //Output assignment
   assign oFilteredSignals = rFilteredSignals;
   
endmodule
