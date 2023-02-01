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
    input      iClk, //used for the delayed version of CATERR & RMCA output. Depends on the freq, is the parameter used, we need 500 us delay
    input      iRst_n, //synch reset input (active low)
   
    input      iCpuCatErr_n, //CPU_CATERR_N comming from both CPUs
    input      iCatErrFilterEvent,   //used as the filtering event for iCpuCatErr_n, to make sure it remains at HIGH until this input is asserted
    //input iCpuRmca_n,                       //CPU_RMCA_N coming from both CPUs   //alarios: 13-ago-20 Removed per Arch request (no need to be monitored by PCH or BMC)

    input      iFmBmcCrashLogTrig_n, //these 2, combined with the CATERR, generate the CRASHLOG_TRIG_N signal to PCH
    input      iFmGlbRstWarn_n,

    input      iIrqPchCpuNmiEvent_n, //PCH NMI event to CPU
    input      iIrqBmcCpuNmi, //BMC NMI event to CPU
    input      iBmcNmiPchEna, //alarios 13-ago-20 Selects if BMC NMI is fwded to CPUs(LOW) or PCH(HIGH)

    output     oCpuCatErr_n, //combined CATERR & RMCA output  (alarios 13-ago-20 RMCA removed)
    //output reg oCpuCatErrDly_n, //delayed output of CATERR & RMCA (alarios 13-ago-20 RMCA removed)

    output     oFmPchCrashlogTrig_n, //Crashlog trigger asserted when any of CATERR, BMC_CRASHLOG_TRIGGER or GLB_RST_WARN goes low

    output     oIrqBmcPchNmi,  //goes to PCH NMI input. Asserted when BMC NMI is asserted and PCH selected as destination
    output     oCpuNmi         //CPU NMI asserted when BMC NMI (if selected) or PCH NMI are asserted

   );

   //////////////////////////////////
   //internal signal declaration
   //////////////////////////////////////////////////////////////////
   
   //reg    rStart500usTimer;   //used to capture and hold the start condition until timer is done     //alarios: 20-ago-20 removed as no longer used as CaterrDly_n has been removed
    
   wire   wDoneTimer500us;    //timer done signal
   
   wire   wCpuCatErr_n;      //used for output without delay and using internally for the delayed output as well



   //////////////////////////////////
   //modules instantiation
   //////////////////////////////////////////////////////////////////

/* -----\/----- EXCLUDED -----\/-----
   
   //500-usec Timer for the CATERR_RMCA_DLY_N output
   delay #(.COUNT(1000)) 
     Timer500us(
                .iClk(iClk),
                .iRst(iRst_n),
                .iStart(rStart500usTimer),
                .iClrCnt(1'b0),
                .oDone(wDoneTimer500us)
                );
 -----/\----- EXCLUDED -----/\----- */
   
   

   //////////////////////////////////
   //internal signals assignments
   //////////////////////////////////////////////////////////////////

   
   //assign  wCpuCatErr_n = iCpuCatErr_n /*&& iCpuRmca_n*/;              //regular output is the AND of both CPU_CATERR & CPU_RMCA
                                                               //alarios: 13-ag0-20: RMCA not required to be monitored by PCH or BMC 

   assign wCpuCatErr_n = iCatErrFilterEvent ? iCpuCatErr_n : 1'b1;     //alarios: 26th-July-21: filtering CATERR input until related filtering event is asserted
   
   //////////////////////////////////
   //sequential logic for the delay control
   //////////////////////////////////////////////////////////////////
   
   

/* -----\/----- EXCLUDED -----\/-----

   always @(posedge iClk, negedge iRst_n)
     begin
        if (~iRst_n)                         //synch reset (active low)
          begin
             rStart500usTimer <= 1'b0;
             oCpuCatErrDly_n <= 1;
          end
        else
          begin
             if (~wCpuCatErr_n)                //when input is asserted (active low)
               begin
                  rStart500usTimer <= 1'b1;        //assert start-timer 
               end

             if (wDoneTimer500us)                  //if timer is done, assert delayed output
               begin
                  oCpuCatErrDly_n <= 1'b0;
                  if ( wCpuCatErr_n)           //if also input is de-asserted, clear start-timer signal
                    begin                          //that will clear the timer-done signal as well
                       rStart500usTimer <= 1'b0;
                    end
               end
             if (rStart500usTimer <= 1'b0)         //if start-timer is de-asserted, make sure your dlyd output is also 
               oCpuCatErrDly_n <= 1'b1;        //de-asserted
             
          end // else: !if(~iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
 -----/\----- EXCLUDED -----/\----- */
   

   

   //////////////////////////////////
   //output assignments
   //////////////////////////////////////////////////////////////////

   assign oCpuCatErr_n = wCpuCatErr_n;                        //regularr output is the AND output
   
   //assign oFmPchCrashlogTrig_n = iCpuCatErr_n && iFmBmcCrashLogTrig_n && iFmGlbRstWarn_n;     //Pch Crashlog Trigger (active low, when any of the 3 inputs goes low)

   //assign oFmPchCrashlogTrig_n = iCpuCatErr_n ? iFmGlbRstWarn_n : iFmBmcCrashLogTrig_n;
   assign oFmPchCrashlogTrig_n = wCpuCatErr_n ? iFmGlbRstWarn_n : iFmBmcCrashLogTrig_n;        //alarios: July-26th-21: to use the filtered version of CATERR

   //alarios 13-ago-20: Changed per Arch request
   //BMC NMI fwded to CPU(LOW) or PCH(HIGH) depending on selecting signal iBmcNmiPchEna
   //assign oCpuNmi = ~iIrqPchCpuNmiEvent_n || iIrqBmcCpuNmi;           //PCH2CPU NMI Event (active low) or BMC 2 CPU Event generates an event to CPU_NMI

   //if iBmCNmiPchEna == 0, BMC NMI is fwded to CPUs, ORed with NMI from PCH
   //marcoara 12-dec-22: Removed NMI coming from PCH per arch request as solution to HSD #14017648874
   //assign oCpuNmi = (iBmcNmiPchEna ? 1'b0 : iIrqBmcCpuNmi) || ~(iIrqPchCpuNmiEvent_n);
   assign oCpuNmi = iBmcNmiPchEna ? 1'b0 : iIrqBmcCpuNmi;
   //if iBmCNmiPchEna == 0, BMC NMI is fwded to PCH, else set to LOW
   assign oIrqBmcPchNmi = iBmcNmiPchEna ? iIrqBmcCpuNmi: 1'b0;
   
   
   
   /*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

  

endmodule // MiscLogic





   
