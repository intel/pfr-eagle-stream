`default_nettype none
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Delay Module</b>\n
    \details    Sets an output Flag whenever the internal counters reaches a count\n
                fixed by a parameter. The counters can be restarted either\n
                with an asynchronous reset or the input signal iStart.
    \file       Delay.v
    \author     amr/ricardo.ramos.contreras@intel.com
    \date       Oct 31, 2012
    \brief      $RCSfile: Delay.v.rca $
                $Date: Fri Feb  8 16:18:27 2013 $
                $Author: rramosco $
                $Revision: 1.4 $
                $Aliases:  $
                <b>Project:</b> Tazlina Glacier\n
                <b>Group:</b> BD\n
                <b>Testbench:</b> Delay_tb.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN
                                    </ol>
                <b>References:</b>  <ol>
                                        <li>
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +-------------------------------------+
 -----> |> iClk     .       .       .   oDone |----->
 -----> |  iRst     .       .       .       . |
 -----> |  iStart  .       .       .       . |
        +-------------------------------------+
                        Delay
    \endverbatim
----------------------------------------------------------------------------------
    \version
                20190116 \b rramosco - File creation\n

    \copyright Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//          Delay
//////////////////////////////////////////////////////////////////////////////////
module delay # //% Parameterizable Delay Module<br>
(
                                //% Total count
    parameter                   COUNT  =   1
)
(
                                //% Clock Input<br>
    input wire                  iClk,
                                //% Asynchronous Reset Input<br>
    input wire                  iRst,
                                //% Start Delay Input Signal<br>
    input wire                  iStart,
                                //% Clear Count signal <br>
    input wire                  iClrCnt,
                                //% Done Flag Output<br>
    output wire                 oDone
);

   
localparam TOTAL_BITS = clog2(COUNT);

reg                     rDone;
reg [(TOTAL_BITS-1):0]  rCount;

always @(posedge iClk or negedge iRst)
begin
	if (~iRst) begin						//Reset
		rDone   <=   1'b0;					//Done flag
		rCount  <=   {TOTAL_BITS{1'b0}};	//Counter
	end
	else begin
		if(~iStart || iClrCnt) begin		//If iStart is LOW or iClrCnt is HIGH, output goes low as well
			rDone	<= 1'b0;
			rCount	<= {TOTAL_BITS{1'b0}};
		end
		else if (COUNT-1 > rCount) begin	//Output set as high when counter reaches expected value
			rDone	<= 1'b0;
			rCount	<= rCount + 1'b1;
		end
		else begin							//Output low and counter increases if no conditions were met
			rDone	<= 1'b1;
			rCount	<= rCount;
		end
	end
end

assign oDone = rDone;
//base 2 logarithm, run in precompile stage
function integer clog2;
input integer value;
begin
   value = (value > 1) ? value-1 : 1;            //this avoids clog returning 0, which would cause issues 
   for (clog2=0; value>0; clog2=clog2+1)
     value = value>>1;
end

endfunction

endmodule
