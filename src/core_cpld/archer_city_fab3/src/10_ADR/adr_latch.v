/////////////////////////////////////////////////////////////////////
/*
 
     Author      : Alejandro Larios
     email       : a.larios@intel.com
     Description : 15-Feb-21 -> This module was created to latch and keep values stable for ADR_MODEs and ADR_COMPLETE signals 
                                that come from PCH as the used GPIOs do not hold their values after some reset flows, creating
                                issues to the fpga internal logic. The fpga require these signals to hold their values to work 
                                properly. 

 */

module adr_latch
  (
   input iClk,                     //input clock (2Mhz)
   input iRst_n,                   //input reset signal (active-low)

   input iAdrMode0,                //input ADR_MODE0 coming from PCH
   input iAdrMode1,                //input ADR_MODE1 coming from PCH
   input iAdrComplete,              //input ADR_COMPLETE coming from PCH

   input [7:0] iBiosPostCodes,     //Bios PostCodes from BMC SGPIO will serve as the latching event
   input iSlpS5_n,                 //SleepS5 signal directly from PCH, active low and used to know when to filter ADR_COMPLETE


   input iAdrAck,                  //ADR ACK signal, coming from adr_fub

   input iRsmRst_n,                //Resume Reset (active low), used to know when to change Adr Ack from Hw Strap to PCH, to really be ADR ACK


   output reg oAdrMode0,               //output - latched ADR Mode 0
   output reg oAdrMode1,               //output - latched ADR Mode 1
   output     oAdrComplete,            //output - latched ADR Complete

   output     oAdrAck                  //output - Adr Ack output going to baseboard (PCH) after being filtered with postcode 0x48
   
   );

   //localparam BIOS_LATCH_CODE = 8'h48;    //this value is sent by BIOS after GPIO configuration is DONE.
   localparam BIOS_LATCH_CODE = 8'hB0;      //alarios July-8th-21 : 0x48 post code not always seen. 0xB0 happens after 0x48 (GPIO config) and before MRC

   reg        rAdrCompleteFilter;                 //will use this to know when desired postcode happens so we can monitor ADR_COMPLETE
   reg        rBiosDone;

   reg        rRsmRstD1_n, rRsmRstD2_n;
   

   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             oAdrMode0 <= 1'b0;               //reset all signals to a default known value
             oAdrMode1 <= 1'b0;
             rAdrCompleteFilter <= 1'b0;
             rBiosDone <= 1'b0;
             rRsmRstD1_n <= 1'b0;
             rRsmRstD2_n <= 1'b0;
          end
        else
          begin

             rRsmRstD1_n <= iRsmRst_n;
             rRsmRstD2_n <= rRsmRstD1_n;
             
             if (!iSlpS5_n)   //if PCH signals going to S5, rAdrCompleteFilter is disabled and this will cause ADR_COMPLETE from PCH to be filtered
               begin
                  rAdrCompleteFilter <= 1'b0;
               end
             else if (iBiosPostCodes == BIOS_LATCH_CODE)         //when BIOS sends desired post code, ADR MODE are captured and start monitoring ADR_COMPLETE
               begin
                  oAdrMode0 <= iAdrMode0;
                  oAdrMode1 <= iAdrMode1;
                  rAdrCompleteFilter <= 1'b1;                 //this enables ADR_COMPLETE to be seen by fpga logic (not filtered)
                  rBiosDone <= 1'b1;
               end
             else
               begin
                  oAdrMode0 <= oAdrMode0;
                  oAdrMode1 <= oAdrMode1;
                  rBiosDone <= rBiosDone;
                  rAdrCompleteFilter <= rAdrCompleteFilter;
               end
          end
     end // always @ (posedge iClk, negedge iRst_n)

   //using rBiosDone as a mask iAdrComplete (which comes from PCH) into oAdrComplete (which goes to internal FPGA logic)
   assign oAdrComplete = iAdrComplete && rAdrCompleteFilter;

   //for adr_ack, as PCH uses the same GPIO as a STRAP, and then, by default, it could configure it as output, etc, before rBiosDone is asserted, adr_ack is tri-stated so the board
   //can set the strap as required (HIGH or LOW) and FPGA won't drive, so if it is configured as output, there is no contention. Once the rBiosDone is asserted, FPGA can drive

   assign oAdrAck = rRsmRstD2_n ?  iAdrAck : 1'b1;

endmodule // adr_latch
