`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Asynchronous Data Refresh (ADR) Functional Unit Block (fub)</b> 
    \details    
 
    \file       adr_fub.v
    \author     amr/a.larios@intel.com
    \date       Aug 19, 2019
    \brief      $RCSfile: NGSX.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                
                <b>Project:</b> Archer City (EagleStream)\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   
                <b>Block Diagram:</b>
 
    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/


module adr_fub
  (
    input iClk,                                     //input clock signal (@2MHz)
    input iRst_n,                                   //input reset signal (active low)
    input iAdrEna,                                  //ADR enable (to enable FSM execution from Master sequencer)
    input iPwrGdPchPwrOk,                           //PWROK from the Power Supply, used to detect surprise power-down condition
    input iFmSlp3Pld_n,                             //s3 indication (from PCH), used to detect surprise power-down condition

    input iFmAdrComplete,                           //comes from PCH and indicates the ADR flow completion from PCH perspective, when triggered by board (FPGA)    
    output reg oFmAdrTrigger_n,                         //goes to PCH to signal the need of ADR as a surprise power down condition was detected
    output reg oFmAdrAck_n,                             //fpga uses this signal to acknoledge the PCH the reception of completion signal. FPGA will assert it 26 usec after
                                                    //asserted pwrgd_fail_cpu signals, to allow NVDIMMs to move data from volatile to non-volatile memory
   
    output reg oPwrGdFailCpuAB,                       //power good/fail# signal for DDR5 DIMMs. Used to indicate the DIMMs to move data from volatil to non-volatil memory
    output reg oPwrGdFailCpuCD,                       
    output reg oPwrGdFailCpuEF,
    output reg oPwrGdFailCpuGH
 
   );

   //local params (for ADR Finite State Machine (FSM))

   localparam IDLE = 0;
   localparam INIT = 1;
   localparam ADR_EVENT = 2;
   localparam ADR_COMPLETE = 3;
   localparam ADR_ACK = 4;


   
   //Internal signals declaration
   reg [2:0]  rAdrFsm;

   reg rStart20usTimer;

   wire wDoneTimer20us;
   

   //20-usec Timer, used to extend the deassertion of pwrgd_fail when in ADR_COMPLETE
   delay #(.COUNT(39)) 
     Timer20us(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(rStart20usTimer),
               .iClrCnt(1'b0),
               .oDone(wDoneTimer20us)
     );
   
   
   
   
   
   //ADR Finite State Machine (rAdrFsm)
   always @(posedge iClk)
     begin
        if(!iRst_n)
          begin
             rAdrFsm <= IDLE;
             rStart20usTimer <= 0;
             oPwrGdFailCpuAB <= 1'b1;
             oPwrGdFailCpuCD <= 1'b1;
             oPwrGdFailCpuEF <= 1'b1;
             oPwrGdFailCpuGH <= 1'b1;

             oFmAdrTrigger_n <= 1'b1;
             oFmAdrAck_n <= 1'b1;
          end
        else //not (if(!rst_n) -- means clk rising edge
          begin
             case (rAdrFsm)
               INIT:
                 begin
                    if (iPwrGdPchPwrOk == 1'b1 && iFmSlp3Pld_n == 1'b1)      //condition means a surprise PwrDwn is happening
                      begin
                         rAdrFsm <= ADR_EVENT;
                         oFmAdrTrigger_n <= 1'b0;                            //assert ADR trigger (active low) to signal PCH to
                      end                                                    //indicate ADR start
                    else
                      begin
                         rAdrFsm <= INIT;                                   
                         oFmAdrTrigger_n <= 1'b1;
                      end
                 end
               ADR_EVENT:
                 begin
                    oFmAdrTrigger_n <= 1'b1;
                    if (iFmAdrComplete == 1'b1)                             //PCH signals that ADR flow is complete on its side
                      begin
                         rAdrFsm <= ADR_COMPLETE;
                         oPwrGdFailCpuAB <= 1'b0;                           //deassert PwrGd_Fail so NVDIMMs move data from volatil to non-volatil mem
                         oPwrGdFailCpuCD <= 1'b0;
                         oPwrGdFailCpuEF <= 1'b0;
                         oPwrGdFailCpuGH <= 1'b0;
                         
                         rStart20usTimer <= 1'b1;                           //launch 20usec timer 
                      end
                 end
               ADR_COMPLETE:
                 begin
                    if(wDoneTimer20us)
                      begin
                         rAdrFsm <= ADR_ACK;
                         oPwrGdFailCpuAB <= 1'b1;                           //deassert PwrGd_Fail so NVDIMMs move data from volatil to non-volatil mem
                         oPwrGdFailCpuCD <= 1'b1;
                         oPwrGdFailCpuEF <= 1'b1;
                         oPwrGdFailCpuGH <= 1'b1;
                         oFmAdrAck_n <= 1'b0;                               //assert ADR Ack to PCH so it continues with pwr down sequence
                      end
                 end
               ADR_ACK:
                 begin
                    oFmAdrAck_n <= 1'b1;
                    if (iFmSlp3Pld_n == 1'b0)
                      begin
                         
                         rAdrFsm <= IDLE;
                      end
                 end
               default:   //IDLE
                 begin
                    rStart20usTimer <= 0;
                    oPwrGdFailCpuAB <= 1'b1;
                    oPwrGdFailCpuCD <= 1'b1;
                    oPwrGdFailCpuEF <= 1'b1;
                    oPwrGdFailCpuGH <= 1'b1;
                    
                    oFmAdrTrigger_n <= 1'b1;
                    oFmAdrAck_n <= 1'b1;
                    if (iAdrEna)
                      begin
                         rAdrFsm <= INIT;
                      end
                 end // case: default
             endcase // case (adr_fsm)
          end // else: !if(!rst_n)
     end // always @ (posedge clk)
endmodule // adr_fub

   

