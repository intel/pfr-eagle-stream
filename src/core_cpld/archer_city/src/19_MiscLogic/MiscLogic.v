//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>MiscLogic Functional Unit Block</b> 
    \details    it is mostly 'glue' logic, and implements a 500us delay from when CATERR_RMCA_N signal is asserted
                As it is only required to delay the assertion, a timer is used and some logic to control that delayed 
                assertion happens after 500us, even if after that some changes happen in the 'source'
 
                The CPU-NMI and PCH-CrashLog glue-logic is also implemented here
 
    \file       MiscLogic.v
    \author     amr/a.larios@intel.com
    \date       Sept 7, 2019
    \brief      $RCSfile: MiscLogic.v.rca $
                $Date:  $
                $Author:  Alejandro Larios$
                $Revision:  $
                $Aliases: $
                
                <b>Project:</b> Archer City (EagleStream)\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   
                <b>Block Diagram:</b>
 
    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/


module MiscLogic
  
  (
    input iClk,                                   //used for the delayed version of CATERR & RMCA output. Depends on the freq, is the parameter used, we need 500 us delay
    input iRst_n,                                 //synch reset input (active low)
   
    input iCpuCatErr_n,                           //CPU_CATERR_N comming from both CPUs
    input iCpuRmca_n,                             //CPU_RMCA_N coming from both CPUs

    input iFmBmcCrashLogTrig_n,                   //these 2, combined with the CATERR, generate the CRASHLOG_TRIG_N signal to PCH
    input iFmGlbRstWarn_n,

    input iIrqPchCpuNmiEvent_n,                   //PCH NMI event to CPU
    input iIrqBmcCpuNmi,                          //BMC NMI event to CPU

    output     oCpuRmcaCatErr_n,                  //combined CATERR & RMCA output
    output reg oCpuRmcaCatErrDly_n,               //delayed output of CATERR & RMCA

    output oFmPchCrashlogTrig_n,                  //Crashlog trigger asserted when any of CATERR, BMC_CRASHLOG_TRIGGER or GLB_RST_WARN goes low

    output oCpuNmi                                //CPU NMI asserted when BMC NMI or PCH NMI are asserted

   );

   //////////////////////////////////
   //internal signal declaration
   //////////////////////////////////////////////////////////////////
   
   reg    rStart500usTimer;   //used to capture and hold the start condition until timer is done   
    
   wire   wDoneTimer500us;    //timer done signal
   
   wire   wCpuRmcaCatErr_n;      //used for output without delay and using internally for the delayed output as well



   //////////////////////////////////
   //modules instantiation
   //////////////////////////////////////////////////////////////////

   
   //500-usec Timer for the CATERR_RMCA_DLY_N output
   delay #(.COUNT(1000)) 
     Timer500us(
                .iClk(iClk),
                .iRst(iRst_n),
                .iStart(rStart500usTimer),
                .iClrCnt(1'b0),
                .oDone(wDoneTimer500us)
                );
   
   

   //////////////////////////////////
   //internal signals assignments
   //////////////////////////////////////////////////////////////////
   
   assign  wCpuRmcaCatErr_n = iCpuCatErr_n && iCpuRmca_n;              //regular output is the AND of both CPU_CATERR & CPU_RMCA


   
   //////////////////////////////////
   //sequential logic for the delay control
   //////////////////////////////////////////////////////////////////
   
   


   always @(posedge iClk)
     begin
        if (~iRst_n)                         //synch reset (active low)
          begin
             rStart500usTimer <= 1'b0;
             oCpuRmcaCatErrDly_n <= 1;
          end
        else
          begin
             if (~wCpuRmcaCatErr_n)                //when input is asserted (active low)
               begin
                  rStart500usTimer <= 1'b1;        //assert start-timer 
               end

             if (wDoneTimer500us)                  //if timer is done, assert delayed output
               begin
                  oCpuRmcaCatErrDly_n <= 1'b0;
                  if ( wCpuRmcaCatErr_n)           //if also input is de-asserted, clear start-timer signal
                    begin                          //that will clear the timer-done signal as well
                       rStart500usTimer <= 1'b0;
                    end
               end
             if (rStart500usTimer <= 1'b0)         //if start-timer is de-asserted, make sure your dlyd output is also 
               oCpuRmcaCatErrDly_n <= 1'b1;        //de-asserted
             
          end // else: !if(~iRst_n)
     end // always @ (posedge clk)
 


   

   //////////////////////////////////
   //output assignments
   //////////////////////////////////////////////////////////////////

   assign oCpuRmcaCatErr_n = wCpuRmcaCatErr_n;                        //regularr output is the AND output
   
   assign oFmPchCrashlogTrig_n = iCpuCatErr_n && iFmBmcCrashLogTrig_n && iFmGlbRstWarn_n;     //Pch Crashlog Trigger (active low, when any of the 3 inputs goes low)

   
   assign oCpuNmi = ~iIrqPchCpuNmiEvent_n || iIrqBmcCpuNmi;           //PCH2CPU NMI Event (active low) or BMC 2 CPU Event generates an event to CPU_NMI
   
   
   /*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

  

endmodule // MiscLogic





   
