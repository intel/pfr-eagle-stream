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
    input      iClk, //input clock signal (@2MHz)
    input      iRst_n, //input reset signal (active low)
    input      iAdrEna, //ADR enable (to enable FSM execution from Master sequencer)
    input      iPwrGdPSU, //PWROK from the Power Supply, used to detect surprise power-down condition
    input      iFmSlp3Pld_n, //s3 indication (from PCH), used to detect surprise power-down condition

    input      iFmAdrExtTrigger_n,  //external trigger source, comes from board. If asserted LOW, launches ADR flow by asserting trigger to PCH

    input      iFmAdrComplete, //comes from PCH and indicates the ADR flow completion from PCH perspective, when triggered by board (FPGA)

    input      iAdrRelease, //comes from master, indicates when to release the control of RST and PWRGDFAIL signals
    input      iAdrEmulated,  //comes from master, indicates we are in EMULATED ADR flow and ADR FSM should not take control of PWRGDFAIL & DIMM_RST then

    input      iFmGlblRstWarn_n,   //alarios 12-Nov-21: For CCB where ADR_ACK will be held to be asserted after ADR_COMPLETE, until a 10msec timer expires (launched by this signal)

    output reg oAcRemoval,   //indication of PSU failure or surprise AC removal (regardless of Adr is enabled or not)
   
    output     oFmAdrTrigger_n, //goes to PCH to signal the need of ADR as a surprise power down condition was detected
    output reg oFmAdrAck, //fpga uses this signal to acknoledge the PCH the reception of completion signal. FPGA will assert it 26 usec after
                                    //asserted pwrgd_fail_cpu signals, to allow NVDIMMs to move data from volatile to non-volatile memory
   
    output reg oPwrGdFailCpuAB, //power good/fail# signal for DDR5 DIMMs. Used to indicate the DIMMs to move data from volatil to non-volatil memory
    output reg oPwrGdFailCpuCD, 
    output reg oPwrGdFailCpuEF,
    output reg oPwrGdFailCpuGH,

    output     oAdrSel, //indicates when ddr5 memory module should select PWRGDFAIL and DRAM_RST_N from ADR logic instead
    output     oAdrEvent,           //signals an ADR event occured so master go into ADR state, 
    output     oDimmRst_n,          //this is to get control of the DIMM_RST signal that goes to the DIMMs when ADR_COMPLETE is asserted (1 signal for all DIMMs)

    output     oFmAdrExtTrigger_n   //This is to be used with the PDB ALERT signal (bidir) and THROTTLE logic 
 
   );

   //local params (for ADR Finite State Machine (FSM))

   localparam IDLE = 0;
   localparam INIT = 1;
   localparam START = 1;
   localparam WAIT_ACK = 1;
   localparam ADR_EVENT = 2;
   localparam ADR_COMPLETE = 2;
   localparam ASSERT = 2;
   localparam ADR_STBY = 3;
   localparam ADR_ACK = 3;


   
   //Internal signals declaration
   reg [1:0]   rAdrTrigFsm;            //ADR is splitted in 2 FSMs (1 for FPGA detecting Surprise Pwr-Dwn condition 
   reg [1:0]   rAdrCmpltFsm;           //the other is for detecting the assertion of ADR_COMPLETE, which can be asserted 
                                       // by PCH even if FPGA did not assert a trigger (due to PCH internal conditions)
   
   reg         rAdrSel;                //register used for normal usage. This is masked with AdrEmulated before output to mask that

   reg         rFmAdrTrigger_n;            //output needs to be open-drain

   reg         rAdrTEvent;     //will signal when surprise pwrdwn is detected and will remain active until commanded by master by iAdrRelease
   
   reg         rAdrAEvent;     //this signals when ADR is asserted, will remain active until commanded by master by iAdrRelease, used to generate the AdrEvent to other internal logic modules
   
   reg         rExtTrigFsm;        //used for External Trigger output
   reg         rFmAdrExtTrigger_n; //this is to be used in the FSM, then will be assigned to oFmAdrExtTrigger_n as open-drain output

   wire        wDoneExtTrigTimer;        //these signals are for filtering J5C11 so it only injects external trigger to internal logic once jumper is depopulated and populated
   reg         rAdrExtTrigFiltered_n;
   reg [1:0]   rExtTrigFilterFsm;

   wire        wDoneAdrAckTimer;         //alarios 12-Nov-21: indicates when Adr-Ack 10msec timer is expired (after iFmGlblRstWarn_n is asserted). For CCB implementation

   //used to generate an small pulse (10usec should be enough) for the EXTERNAL TRIGGER signal into the internal logic
   //launched when input from jumper J5C11 is asserted LOW (only after it was originally HIGH)
   delay #(.COUNT(20))  //10usec
     ExtTrigTimer(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(!rAdrExtTrigFiltered_n), //when asserted, start timer
               .iClrCnt(1'b0),
               .oDone(wDoneExtTrigTimer)
     );


   //alarios 12-Nov-21: For CCB where ADR_ACK will be held to be asserted after ADR_COMPLETE, until a 10msec timer expires (launched by iFmGlblRstWarn_n)
   delay #(.COUNT(20000)) //10msec
     AdrAckTimer(
                 .iClk(iClk),
                 .iRst(iRst_n),
                 .iStart(!iFmGlblRstWarn_n),
                 .iClrCnt(1'b0),
                 .oDone(wDoneAdrAckTimer)
    );

   //this FSM is to filter out when jumper J5C11 is left populated in the board incorrectly
   //a short pulse will be generated to internal logic when the jumper is, first depopulated, then populated
   //if one leaves the jumper populated, no other pulse will be injected, until depopluated and populated again
   always @(posedge iClk, negedge iRst_n)
     begin
        if(!iRst_n)
          begin
             rAdrExtTrigFiltered_n <= 1'b1;
             rExtTrigFilterFsm <= IDLE;
          end
        else
          begin
             case (rExtTrigFilterFsm)
               default: //IDLE
                 begin
                    rAdrExtTrigFiltered_n <= 1'b1;
                    if (iFmAdrExtTrigger_n)
                      rExtTrigFilterFsm <= START;
                    else
                      rExtTrigFilterFsm <= IDLE;
                 end

               START:
                 begin
                    rAdrExtTrigFiltered_n <= 1'b1;
                    if (~iFmAdrExtTrigger_n)
                      rExtTrigFilterFsm <= ASSERT;
                    else
                      rExtTrigFilterFsm <= START;
                 end

               ASSERT:
                 begin
                    rAdrExtTrigFiltered_n <= 1'b0;
                    if (wDoneExtTrigTimer)             //when timer expires, go back to IDLE, where internal ExtTrig signal will be disabled
                      rExtTrigFilterFsm <= IDLE;
                    else
                      rExtTrigFilterFsm <= ASSERT;
                 end
             endcase // case (rExtTrigFilterFsm)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   
   
   
   //ADR Finite State Machine (rAdrFsm)
   always @(posedge iClk, negedge iRst_n)
     begin
        if(!iRst_n)
          begin
             rAdrTrigFsm <= IDLE;
             oAcRemoval <= 1'b0;
             //oPwrGdFailCpuAB <= 1'b1;
             //oPwrGdFailCpuCD <= 1'b1;
             //oPwrGdFailCpuEF <= 1'b1;
             //oPwrGdFailCpuGH <= 1'b1;

             rFmAdrTrigger_n <= 1'b1;
             //oFmAdrAck_n <= 1'b1;
             rAdrTEvent <= 1'b0;
          end
        else //not (if(!rst_n) -- means clk rising edge
          begin

             case (rAdrTrigFsm)
               default:   //IDLE
                 begin
                    //oPwrGdFailCpuAB <= 1'b1;
                    //oPwrGdFailCpuCD <= 1'b1;
                    //oPwrGdFailCpuEF <= 1'b1;
                    //oPwrGdFailCpuGH <= 1'b1;
                    
                    rFmAdrTrigger_n <= 1'b1;
                    //if (iAdrEna)                                       //ONLY when MSTR Seq enables this FSM it will move to INIT to wait for a surprise pwr-dwn condition
                    //  begin                                            //otherwise, this means the ADR flow has not been enabled
                    rAdrTrigFsm <= INIT;
                    oAcRemoval <= 1'b0;
                    rAdrTEvent <= 1'b0;
                    //  end                          
                 end // case: default
               INIT:
                 begin
                    //if ((iPwrGdPSU == 1'b0 && iFmSlp3Pld_n == 1'b1) || (!iFmAdrExtTrigger_n && iAdrEna))      //condition means a surprise PwrDwn is happening OR external trigger from board happened
                    if ((iPwrGdPSU == 1'b0 && iFmSlp3Pld_n == 1'b1) || (!rAdrExtTrigFiltered_n && iAdrEna))      //condition means a surprise PwrDwn is happening OR external trigger from board happened 
                      begin
                         oAcRemoval <= 1'b1;
                         if (iAdrEna)
                           begin
                              rAdrTrigFsm <= ADR_COMPLETE;
                              rFmAdrTrigger_n <= 1'b0;                            //assert ADR trigger (active low) to signal PCH to
                              rAdrTEvent <= 1'b1;                                  //to signal an ADR flow has been enabled
                           end                        
                         else
                           begin
                              rAdrTrigFsm <= INIT;
                              rFmAdrTrigger_n <= 1'b1;
                              rAdrTEvent <= 1'b0;
                           end
                      end                                                    //indicate ADR start and go to COMPLETE state to wait for ADR_COMPLETE
                    else
                      begin
                         oAcRemoval <= 1'b0;
                         rAdrTEvent <= 1'b0;
                         rAdrTrigFsm <= INIT;                                   
                         rFmAdrTrigger_n <= 1'b1;
                      end
                 end // case: INIT
               ADR_COMPLETE:
                 begin
                    if (iFmAdrComplete) begin
                       rFmAdrTrigger_n <= 1'b1;                       //once complete is received, de-assert trigger and go to STBY to wait unti complete is de-asserted
                       rAdrTrigFsm <= ADR_STBY;                       //this should also would give time to de-assert external trigger
                    end
                 end
               ADR_STBY:
                      begin
                         if (iAdrRelease) begin
                            rAdrTrigFsm <= IDLE;
                            rAdrTEvent <= 1'b0;
                         end
                      end
             endcase // case (rAdrFsm)
          end // else: !if(!iRst_n)     
     end // always @ (posedge iClk, negedge iRst_n)
   
   
             
                 
   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             oPwrGdFailCpuAB <= 1'b1;
             oPwrGdFailCpuCD <= 1'b1;
             oPwrGdFailCpuEF <= 1'b1;
             oPwrGdFailCpuGH <= 1'b1;                   
             oFmAdrAck <= 1'b0;
             rAdrAEvent <= 1'b0;
             
             rAdrCmpltFsm <= IDLE;
             rAdrSel <= 1'b0;
          end
        else
          begin
             case (rAdrCmpltFsm)
               default: //IDLE
                 begin
                    if (iAdrRelease) begin
                       rAdrAEvent <= 1'b0;                           //also, de-asserting AdrAEvent (in case it was still held asserted)
                       
                    end
                    if (iAdrEna)                                       //ONLY when MSTR Seq enables this FSM it will move to INIT to wait for ADR_COMPLETE assertion
                      begin                                            //otherwise, this means the ADR flow has not been enabled
                         rAdrCmpltFsm <= INIT;
                      end
                    
                    oFmAdrAck <= 1'b0;                                //de-asserting AdrAck here
                 end
               INIT: 
                 begin
                    if (iAdrRelease) begin
                       rAdrAEvent <= 1'b0;                            //also, de-asserting AdrAEvent (in case it was still held asserted)
                    end

                    oFmAdrAck <= 1'b0;                                //de-asserting AdrAck here
                    
                    oPwrGdFailCpuAB <= 1'b1;
                    oPwrGdFailCpuCD <= 1'b1;
                    oPwrGdFailCpuEF <= 1'b1;
                    oPwrGdFailCpuGH <= 1'b1;

                    if (iFmAdrComplete == 1'b1)                        //PCH signals that ADR flow is complete on its side
                      begin
                         rAdrCmpltFsm <= ADR_EVENT;
                      end
                    else
                      begin
                         rAdrCmpltFsm <= INIT;
                      end
                 end // case: INIT
               ADR_EVENT:
                 begin
                    if (iAdrEmulated)
                      rAdrSel <= 1'b0;                                 //as we are in emulated flow, PWRGDFAIL & DIMM_RST should not be drive from ADR logic
                    else
                      rAdrSel <= 1'b1;                                 //means ddr5 module should select PWRGDFAIL & DIMM_RST_N signals from ADR logic
                    
                    oPwrGdFailCpuAB <= 1'b0;                           //deassert PwrGd_Fail# so NVDIMMs move data from volatil to non-volatil mem
                    oPwrGdFailCpuCD <= 1'b0;
                    oPwrGdFailCpuEF <= 1'b0;
                    oPwrGdFailCpuGH <= 1'b0;
                    if ((wDoneAdrAckTimer && !oAcRemoval) || (oAcRemoval))    //alarios 12-Nov-2 (for CCB): if not on AC-removal, wait 10msec, or don't wait if 
                      begin                                                   //                            an AC-removal, before asserting ADR-ACK
                         rAdrCmpltFsm <= ADR_ACK;
                      end
                    else 
                      begin
                         rAdrCmpltFsm <= ADR_EVENT;
                      end
                 end // case: ADR_EVENT
               ADR_ACK:
                 begin
                    oFmAdrAck <= 1'b1;                                 //assert ADR Ack to PCH so it continues with pwr down sequence
                    rAdrAEvent <= 1'b1;                                //asserting ADR Ack Event 

                    if (iFmAdrComplete == 1'b1)                             //when ADR COMPLETE keeps asserted...
                      begin
                         oPwrGdFailCpuAB <= 1'b0;                           //control these until adr_complete is de-asserted
                         oPwrGdFailCpuCD <= 1'b0;
                         oPwrGdFailCpuEF <= 1'b0;
                         oPwrGdFailCpuGH <= 1'b0;

                         if (iAdrEmulated)                                 //if in EMULATED ADR, this fsm does not control PWRGDFAIL & DRAM RST
                           rAdrSel <= 1'b0;                                 
                         else
                           rAdrSel <= 1'b1;                                //means ddr5 module should select PWRGDFAIL & DRAM_RST_N signals not from ADR logic but its own

                         rAdrCmpltFsm <= ADR_ACK;
                      end
                    
                    else                                                    //when ADR_COMPLETE is de-asserted...
                      begin
                         oPwrGdFailCpuAB <= 1'b1;                           //control these until adr_complete is de-asserted
                         oPwrGdFailCpuCD <= 1'b1;
                         oPwrGdFailCpuEF <= 1'b1;
                         oPwrGdFailCpuGH <= 1'b1;

                         rAdrSel <= 1'b0;

                         oFmAdrAck <= 1'b0;                            //ADR ACK de-asserted as ADR_COMPLETE has been also de-asserted
                         
                         rAdrCmpltFsm <= IDLE;
                      end
                 end

             endcase // case (adr_fsm)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)



   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             rFmAdrExtTrigger_n <= 1'b1;
             rExtTrigFsm <= IDLE;
          end
        else
          begin
             case (rExtTrigFsm)
               default:   //IDLE
                 begin
                    rFmAdrExtTrigger_n <= 1'b1;
                    if (!rAdrExtTrigFiltered_n && iAdrEna)
                      begin
                         rFmAdrExtTrigger_n <= 1'b0;
                         rExtTrigFsm <= WAIT_ACK;
                      end
                    else
                      begin
                         rFmAdrExtTrigger_n <= 1'b1;
                         rExtTrigFsm <= IDLE;
                      end
                 end // case: default
               WAIT_ACK:
                 begin
                    if (oFmAdrAck)
                      begin
                         rFmAdrExtTrigger_n <= 1'b1;
                         rExtTrigFsm <= IDLE;
                      end
                    else
                      begin
                         rFmAdrExtTrigger_n <= 1'b0;
                         rExtTrigFsm <= WAIT_ACK;
                      end
                 end
             endcase // case (rExtTrigFsm)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   


   assign oFmAdrTrigger_n = rFmAdrTrigger_n ? 1'bZ : 1'b0;                  //open-drain output

   assign oDimmRst_n = 1'b1;    //whenever ADR is in control, DimmRst_n needs to be hold high, until ADR releases control

   assign oAdrEvent = rAdrTEvent || rAdrAEvent || iFmAdrComplete;   //Adr Trigger Event OR Adr Ack signal (active low) will assert the ADR event to the Master Seq

   assign oAdrSel = rAdrSel;

   assign oFmAdrExtTrigger_n = rFmAdrExtTrigger_n ? 1'bZ : 1'b0;

   
endmodule // adr_fub

   

