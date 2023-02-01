//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Master Sequencer Functional Unit Block</b> 
    \details    
 
    \file       master_fub.v
    \author     amr/a.larios@intel.com
    \date       Aug 19, 2019
    \brief      $RCSfile: master_fub.v.rca $
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


module master_fub
  (
    input        iClk, //input clock signal (@2MHz)
    input        iRst_n, //input reset signal (active low)

    input        iSysChkGood, //input System Check is Good. It enables the Master fub execution

    //input        iAdrEna, //input from board logic (jumper) to config if ADR flow is enabled or not
    input        iFmAdrMode0, //These indicates if ADr is enabled and what mode
    input        iFmAdrMode1, //(Disabled=2'b00, Legacy=2'b01, Emulated DDR5=2'b10,  Emulated Copy to Flash=2'b11)

    input        iAdrEvent, //signald from ADR fub when an ADR event was signaled

    input        iBmcOnCtl_n, //from BMC, FM_BMC_ONCTL_N (active low)
   
    input        iFmSlp3_n, //Sleep S3 signal
    input        iFmSlp4_n, //Sleep S4 signal. Between sleep signals (S3&S4) we can determine if we are in S0, S3 or S4
    input        iFmSlpSusRsmRst_n, //Sleep SUS (active low) signal, coming from PCH when receiving P3V3_DSW (needed to continue with PCH seq)

    input        iDediBusy_n, //Dedi_Busy (active low) would cause a power down sequence when asserted
   
    input        iBmcDone, //BMC sequencer done
    input        iPchDone, //PCH sequencer done
    input        iPsuDone, //PSu_MAIN sequencer done
    input        iCpuMemDone, //CPU_MEM sequencer done

    input        iBmcFailure, //failure indication from BMC sequencer
    input        iPchFailure, //failure indication from PCH sequencer
    input        iPsuFailure, //failure indication from PSU_MAIN sequencer (if PSU, P5V or P3V3 fail)
    input        iPsuPwrOkFail, //with this signal we distinguish if the PSU sequencer failure is due to PSU
    input        iCpuMemFailure, //failure indication from CPU_MEM sequencer

    input        iFmGlbRstWarn_n, //used, together with PLTRST to flag the CPUs Sequencers to de-assert PWRGD_PLT_AUX signal or not

    input        iPltRst_n, //RST_PLTRST_N input signal to FPGA, de-asserted by PCH indicating platform is out of reset

    input        iPwrBttn_n,   //Power Button input 

    output reg   oBmcEna, //Enable BMC sequencer to start
    output reg   oPchEna, //Enable PCH sequencer to start
    output reg   oPsuEna, //Enable PSU_MAIN sequencer to start
    output reg   oCpuMemEna, //Enable CPU_MEM sequencer to start
    output reg   oAdrEna, //Enable for ADR FSM
    output reg   oAdrRelease, //this if for ADR logic to indicate ddr5 logic when to use its own logic or ADR logic to drive PWRGDFAIL & DRAM_RST_N
                              //it is asserted when in PCH state and conditions are met to move to PSU state
                              //de-asserted when in PSU state and condtions are met to move to CPU_MEM state
    output       oAdrEmulated,   //this is to signal that ADR emulated flow is in place, so ADR fsm does not take control of the PWRGDFAIL and DIMM_RST signals

    //output reg   oPwrGdPchPwrOK, //PCH PWR-OK
    //output       oPwrGdSysPwrOK, //SYS PWR-OK

    output       oS3Indication, //we are in S3 indication
    output       oS5Indication, //we are in S4/S5 indication

    output reg   oPltAuxPwrGdFlag, //signals CPU_SEQs that PWRGD_PLT_AUX_CPU signal needs to go low when powering-down
                                   //as it was caused by a GLB_RST_N condition from PCH

    output       oDonePsuOnTimer,   //this signal goes to CPU_SEQs, to warrant 200msec have passed from when PSU_ON was asserted (before PchPwrOK is asserted to PCH)

    output       oPwrDwn, //Power Down Indication for all sequencers

    output [3:0] oMasterPostCode,                          //to the 7-Seg Displays to know the Master-Seq state

    output oFailId
   );


   //////////////////////////////////
   //local params declarations
   //////////////////////////////////////////////////////////////////

   localparam IDLE = 0;
   localparam BMC = 1;
   localparam PCH = 2;
   localparam PSU = 3;
   localparam CPU_MEM = 4;
   localparam PLTRST = 5;
   localparam PLATFORM_ON = 6;
   localparam ERROR = 7;
   localparam ADR = 8;
   localparam PWR_DWN = 9;

   //for the SLP indications, depending if we are in EMULATED R-DIMM ADR FLOW && pwrdwn is due to FPGA triggered ADR or no
   parameter SLP_INIT = 0;
   parameter SLP_ADR = 1;
   parameter SLP_HOLD = 2;

   //////////////////////////////////
   //internal signals declarations
   //////////////////////////////////////////////////////////////////
   
   reg [3:0] rMasterSeqFsm;

   reg [1:0] rSlpFsm;

   //reg       rSysPwrOkEna;                 //basically it is used to de-assert SysPwrOk before PchPwrOk. In sequence, PCHPWROK is asserted first and it causes SysPwrOk to be asserted some time later
                                           //on the power-down sequence, SysPwrOk needs to be de-asserted first. This signal helps to achieve it due to the delay module we use to insert the required time
                                           //between PchPwrOk assertion and SysPwrOk

   wire    wStartPsuOnTimer;
             //alarios 31-May-2021 : Changing the name as duration has changed from 100 to 200 msec
   wire    wDoneSysPwrOkTimer;
              //alarios 31-May-2021 : Changing the name as duration has changed from 100 to 200 msec
   wire    wDoneTimer1ms;
   wire    wDoneCpuPwrGdTimer;

   wire    wCpuMemDoneTimer;               //alarios 8-Dec-2021: Adding to filter if CPU SEQ has finished powering down before moving to PSU state during a power down seq (and avoid get stuck when a CPU VR fails)
   
   

//   wire    wS0Indication;
   reg    rS3Indication;
   reg    rS4Indication;

   wire    wFailure;
   wire    wFailure2;
   
   wire    wPwrDwn;

   reg     rAdrEna;            //alarios: to internal logic to indicate when ADR has been enabled by PCH
   reg     rAdrEmulated;       //alarios: if a power down happens due to an ADR, this signal will indicate if system should go to S3 or S5 sort of state
                               //This is to support the emulated ADR Flow with R-Dimms (only when ADR_MODE[1:0] = 2'b10)
          
   
   reg     rfaultSt;           //set when reach the ERROR state
   
   

   //////////////////////////////////
   //sub modules instantiation
   //////////////////////////////////////////////////////////////////
   
   //100-msec Timer, started when MAIN rails are on. Output is ANDed with CpuMemDone signal (last turned-on VRs) to generate PWRGD_SYS_PWROK & PWRGD_PCH_PWROK
   delay #(.COUNT(400000)) 
     TimerSysPwrOk(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(wStartPsuOnTimer),
               .iClrCnt(1'b0),
               .oDone(oDonePsuOnTimer)
               );


   //1-msec Timer, started when S3 is asserted, cleared when S3 is HIGH
 //alarios jun-30-21: changed to be 100usec -> alarios: sept-21: changed back to 1msec
   delay #(.COUNT(2000)) 
     Timer1ms(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(!iFmSlp3_n && iFmSlp4_n),
               .iClrCnt(1'b0),
               .oDone(wDoneTimer1ms)
               );

   //this small timer should help giving enough time to wait for CPU SEQ FMS to move to power down state (and assert CPU DONE, before checking if we need to move to PSU state during power down (avoiding shutting PSU down before CPU VRs, etc)
   delay #(.COUNT(30))
   TimerCpuMemDone(
                   .iClk(iClk),
                   .iRst(iRst_n),
                   .iStart(!oCpuMemEna),
                   .iClrCnt(1'b0),
                   .oDone(wCpuMemDoneTimer)
                   );
   
   //////////////////////////////////
   //internal signals assignments
   //////////////////////////////////////////////////////////////////
   
   //Power Supply PWR OK signal is used to start the 100

   //when PSU_MAIN is done, P3V3 main is on and a 200ms timer starts
   assign  wStartPsuOnTimer = iPsuDone;  //iPwrGdPsPwrOK;


   //
   //assign  wS0Indication = iFmSlp3_n && iFmSlp4_n;                             //if both in '1', means we are into S0 state or in the way to S0

   //alarios: next eq is as follows
   //   if Slp3 from PCH is asserted and Slp4 is not for 1 ms, we take it as SLP3 state
   //   if Slp3 from PCH is asseretd and Slp4 is asserted, we are in Slp4 state
   //   but if we are in AdrEmulated and AdrAck is asserted, means pwr-dwn was caused by ADR flow, then we assert S3Indication instead of S4Indication to support the emulated ADR flow with R-DIMMs
   //assign  wS3Indication = (!iFmSlp3_n && iFmSlp4_n && wDoneTimer1ms) || (!iFmSlp4_n && rAdrEmulated && iAdrAck);             //if Slp3 asserted (active low) and Slp4 deasserted during 1ms, we are in S3 state

   //alarios: next eq is as follows
   //   if in AdrEmulated mode, and we go to a pwr-dwn due to an ADR flow, S4Indication should not be activated (S3Indication instaed will be asserted)
   //   if other condition, S4Indication should be asserted
   
   //assign  wS4Indication = !iFmSlp4_n && !(rAdrEmulated && iAdrAck);                                                           //Slp4 asserted (active low), we are in S4/S5 state

   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n) //asynch reset
          begin
             rS3Indication <= 1'b0;
             rS4Indication <= 1'b1;
             rSlpFsm <= SLP_INIT;
          end
        else      //if no rst, means rising clock edge
          begin
             case(rSlpFsm)
               default: begin      //INIT STATE
                  rS3Indication <= (!iFmSlp3_n && iFmSlp4_n && wDoneTimer1ms);
                  rS4Indication <= !iFmSlp4_n;
                  if (rAdrEmulated && iAdrEvent) //we are in emulated R-DIMM ADR flow AND the pwr down is due to FPGA has triggered ADR
                    begin
                       rSlpFsm <= SLP_ADR;
                    end
                  else
                    begin
                       rSlpFsm <= SLP_INIT;
                    end
               end // case: default

               SLP_ADR: begin
                  rS3Indication <= !iFmSlp4_n;
                  rS4Indication <= 1'b0;
                  if (oAdrRelease)
                    begin
                       rSlpFsm <= SLP_HOLD;
                    end
                  else
                    begin
                       rSlpFsm <= SLP_ADR;
                    end
               end // case: SLP_ADR

               SLP_HOLD: begin
                  if (!oAdrRelease)
                    begin
                       rSlpFsm <= SLP_INIT;
                    end
               end
             endcase
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   
        
   


   //we distinguish if it is a BMC or PCH VR failure vs the rest of sequencers. If BMC/PCH VR fails, FPGA will need to turn off all VRs, while if any of the other fails, PCH & BMC VRs will remain on (in S5 state)

   
   //for handling a PSU failure, We need to check if Adr is enabled in the board (rAdrEna), if enabled, we should ignore a PSU failure, ADR flow will take care of it
   // if not, we need to treat it as a normal failure, power-down, etc (regular flow)
   //Now, PSUSeq failure might be due to a PSU or either P5V/P3V3 VRs failure, to distinguish, we use the iPsuPwrOkFail Signal (when HIGH, it was PSU related failure)
   //if failure was caused by P5V/P3V3 VRs, we need to treat the failure no matter what

   assign  wFailure = (iPsuFailure && !(rAdrEna && iPsuPwrOkFail)) || iCpuMemFailure;  //When ADR is not enabled or NOT a PSU failure) or CPU_MEM sequencers failure detection will generate a failure indication for pwr down
  

   assign  wFailure2 = iBmcFailure || iPchFailure;      //BMC or PCH sequencers failure detection will generate a failure indication for pwr down

   
   //PwrDwn indication depends if any Sequencer asserted failure indication or we are going to S3/S4
   assign  wPwrDwn = rS3Indication || rS4Indication || wFailure || wFailure2 || iDediBusy_n;            //PwrDwn indication depends if any Sequencer asserted failure indication or we are going to S3/S4
   
 


   always @ (*)
     begin
        case({iFmAdrMode1,iFmAdrMode0})
    
          default:    //2'b00       //ADR not enabled by PCH
            begin
               rAdrEna <= 1'b0;
               rAdrEmulated <= 1'b0;
            end
          2'b01:                    //ADR PLATFORM LEGACY MODE
            begin
               rAdrEna <= 1'b1;
               rAdrEmulated <= 1'b0;
            end
          2'b10:                    //ADR PLATFORM EMULATED DDR5 BBU ADR FLOW (not supported yet)
            begin
               rAdrEna <= 1'b1;
               rAdrEmulated <= 1'b1;
            end
          
        endcase // case ({iFmAdrMode1,iFmAdrMode0})
        
     end
   


   //////////////////////////////////
   //Master Sequencer FSM
   //////////////////////////////////////////////////////////////////
   
   always @ (posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)     //Asynchronous reset condition
          begin

             rMasterSeqFsm <= IDLE;
             
             oBmcEna <= 1'b0;
             oPchEna <= 1'b0;
             oPsuEna <= 1'b0;
             oCpuMemEna <= 1'b0;
             oAdrEna <= 1'b0;
             //oPwrGdPchPwrOK <= 1'b0;
             //rSysPwrOkEna <= 1'b0;
             oPltAuxPwrGdFlag <= 1'b0;
             oAdrRelease <= 1'b0;
             rfaultSt <= 1'b0;
             //oPwrGdSysPwrOK <= 1'b0;
          end // if (!iRst_n)
        else
          begin
             if (iSysChkGood)                                            //configuration is OK (PKGID, SKOCC, INTR_PRSNT)
               begin
                  case(rMasterSeqFsm)
                    default: begin         //IDLE
                       oPsuEna <= 1'b0;
                       oCpuMemEna <= 1'b0;
                       oAdrEna <= 1'b0;
                       //oPwrGdPchPwrOK <= 1'b0;
                       //rSysPwrOkEna <= 1'b0;
                       oPltAuxPwrGdFlag <= 1'b0;
                       oAdrRelease <= 1'b0;
                       rfaultSt <= 1'b0;
                       if (!wFailure2)
                         begin
                            oBmcEna <= 1'b1;                             //Enable for BMC sequencer
                            oPchEna <= 1'b1;                             //Enable also PCH sequencer, so it can power on at the same time as BMC sequencer                             
                            rMasterSeqFsm <= BMC;
                         end
                       else //if (wFailure2)                              //if wFailure2 is asserted, we should go to ERROR, otherwise turn on BMC and PCH
                         begin
                            rMasterSeqFsm <= ERROR;
                         end
                    end // case: default
                    
                    BMC: begin
                       if (wFailure2)                                    //Power Down Condition detected due to a failure on BMC or PCH VRs
                         begin
                            oBmcEna <= 1'b0;                             //disabling BMC Sequencer
                            if (iBmcDone == 1'b0)                        //if BMC Seq is done with pwr-dwn, we need to distinguish if it was a Failure or normal pwr down
                              begin
                                 rMasterSeqFsm <= ERROR;                 //for failure next pwr dwn state is ERROR
                              end
                         end // if (wFailure2)
                       else if (iBmcDone == 1'b1 && iFmSlpSusRsmRst_n == 1'b1)    //not in pwrdown, and SLP_SUS is deasserted, then pwr up PCH
                         begin
                            oPchEna <= 1'b1;                             //Enable for PCH sequencer
                            rMasterSeqFsm <= PCH;
                         end
                    end // case: BMC
                    
                    PCH: begin
                       oAdrEna <= 1'b0;                        //disabling ADR FSM
                       oAdrRelease <= 1'b0;                    //
                       if (iAdrEvent) /*&& rAdrEmulated)*/      //Power down was due an ADR flow and we are in ADREmulated config
                         begin
                            rMasterSeqFsm <= ADR;                            
                         end
                       else if (wFailure2)                               //Power Down Condition detected due to a BMC or PCH VR failure
                         begin
                            oPchEna <= 1'b0;                             //disabling PCH Sequencer
                            if (iPchDone == 1'b0)                        //if PCH Seq is done with pwr-dwn, go to next power down state
                              begin
                                 rMasterSeqFsm <= BMC;                   //next pwr dwn state is BMC 
                                 oBmcEna <= 1'b0;                        
                              end
                         end //if (wFailure2)
                       else if (wFailure)
                         rMasterSeqFsm <= ERROR;
                       else if (iPchDone && ~iBmcOnCtl_n && iFmSlp3_n && ~wPwrDwn)     //not in regular pwrdown, then pwrup, when PCH seq finishes and BMC asserts On-Ctl signal... 
                                                                                       // also not in any sleep state (redunant check added for VRTB)
                         begin
                            oPsuEna <= 1'b1;                                           //Enable for PSU_Main sequencer
                            rMasterSeqFsm <= PSU;
                            oPltAuxPwrGdFlag <= 1'b0;                    //This flag is cleared as we are powering up
                         end
                       
                    end // case: PCH
                    
                    PSU: begin
                       if (wPwrDwn)                                      //Power Down Condition detected
                         begin
                            oPsuEna <= 1'b0;                             //disabling Psu-Main Sequencer
                            if ((iPsuDone == 1'b0) && (wFailure2 == 1'b1))   //if Psu-Main Seq is done with pwr-dwn, and it was a BMC/PCH VR failure, go to next power down state, else go to ERROR state
                              begin
                                 rMasterSeqFsm <= PCH;                   //next pwr dwn state is PCH 
                                 oPchEna <= 1'b0;                        
                              end
                            else if (iPsuDone == 1'b0 && wFailure == 1'b1)
                              begin
                                 rMasterSeqFsm <= ERROR;                 //next pwr dwn state is ERROR as it was a failure not from BMC/PCH VRs 
                              end
                            else if (iPsuDone == 1'b0)
                              begin
                                 rMasterSeqFsm <= PCH;                   //if it was a normal power down sequence, FSM goes to PCH state without disabling PCH sequencer, 
                              end                                        //waiting for BmcOnCtl signal to be asserted again to come-back here in a power-up sequence again or for power to go-off
                         end // if (wPwrDwn)
                       else if (iPsuDone == 1'b1)
                         begin
                            oCpuMemEna <= 1'b1;                          //Enable for CPU_MEM sequencer
                            rMasterSeqFsm <= CPU_MEM;
                         end
                       else
                         oPsuEna <= 1'b1;
                       
                    end // case: PSU
                    
                    CPU_MEM: begin
                       if (wPwrDwn)                                      //Power Down Condition detected
                         begin
                            oCpuMemEna <= 1'b0;                          //disabling CpuMem Sequencer
                            //oPwrGdPchPwrOK <= 1'b0;                      //deasserting PCH & SYS PWR OK signals as we are going down
                            //oPwrGdSysPwrOK <= 1'b0;
                            if (iCpuMemDone == 1'b0 && wCpuMemDoneTimer)                     //if CpuMem Seq is done with pwr-dwn, go to next power down state
                              begin
                                 rMasterSeqFsm <= PSU;                   //next pwr dwn state is Psu-Main 
                                 oPsuEna <= 1'b0;
                              end
                         end
                       else if (iCpuMemDone == 1'b1 /*&& wDoneTimer100ms == 1'b1*/)
                         begin
                            rMasterSeqFsm <= PLTRST; //PLATFORM_ON;      //All VRs turned-on and 100msec timer expired so we assert PCH and SYS PWR OK signals
                            //oPwrGdPchPwrOK <= 1'b1;                      //This triggers oSysPwrOk to be asserted after some delay
                            //rSysPwrOkEna <= 1'b1;
                            //oPwrGdSysPwrOK <= 1'b1;
                            //oAdrEna <= rAdrEna;                          //enable ADR FSM if jumper is not set in board
                         end
 // UNMATCHED !!
                       else
                         oCpuMemEna <= 1'b1;
                    end // case: CPU_MEM

                    PLTRST:begin
                       
                       if (wPwrDwn)
                         begin
                            rMasterSeqFsm <= CPU_MEM;
                            oCpuMemEna <= 1'b0;
                            //rSysPwrOkEna <= 1'b0;                        //this causes oPwrGdSysPwrOk to be deasserted
                         end
                       else if (iPltRst_n)                               //If PCH de-asserts PLT_RST_N, means platform is OUT of RESET and we can move to
                         begin                                           //PLATFORM is ON state. This should happen after FPGA asserts PCH and SYS PWROK
                            rMasterSeqFsm <= PLATFORM_ON;
                            oAdrEna <= rAdrEna;                          //enable ADR FSM if jumper is not set in board
                         end
                       else
                         oCpuMemEna <= 1'b1;
                    end
                    
                    PLATFORM_ON: begin                                   //Only monitoring for a power down condition (a failure or going to S3 or S5
                       oAdrEna <= rAdrEna;                               //enable ADR FSM if jumper is not set in board
                       if (!iFmGlbRstWarn_n) begin
                         oPltAuxPwrGdFlag <= 1'b1;                       //Flag is asserted only if we receive a GLB_RST_WARN_N. This is passed to CPU_SEQ to know if it needs to leave PLT_AUX_PWRGD asserted or not
                       end                                               //in case of a power-down condition
                       
                       if (wPwrDwn)                                      //Power Down Condition detected
                         begin
                            rMasterSeqFsm <= PWR_DWN;
                            oCpuMemEna <= 1'b0;
                            //oCpuMemEna <= 1'b0;                          //going back for powering down, starting with CPU
                            //rSysPwrOkEna <= 1'b0;                        //this causes oSysPwrOk to be deasserted
                         end
                    end //case: PLATFORM_ON

                    PWR_DWN: begin
                       //oPwrGdPchPwrOK <= 1'b0;
                       oCpuMemEna <= 1'b0;                                //going back for powering down, starting with CPU
                       if (!iFmSlp3_n || !iFmSlp4_n)                       //alarios july 09-21: seems timer between CPUPWRGD and SLPS5 was not really implemented but also I don't think it is required
                                                                           //as CPUPWRGD de-assergint first to CPU would cause DDRT_CLK to be lost earlier and we want the opposite for CPS
                                                                           //will power down with SLP signals and we wait for 600usec before we drop CPUPWRGD to CPUs (timer in ddr5 module and pwr-seq stop in cpu fsm)
                         begin
                            rMasterSeqFsm <= CPU_MEM;
                         end
                    end
                    
                    ERROR: begin                                         //if you fall into an error condition, only way out is a reset (AC cycle)
                       rMasterSeqFsm <= ERROR;
                       rfaultSt <= 1'b1;
                       
                    end //case: ERROR
                    
                    ADR: begin                                                                 //if you fall into an ADR state, 
                       oAdrRelease <= 1'b1;                                                    //means ADR logic should de-assert the ADR_ACK signal now
                       if ((rAdrEmulated && !iPwrBttn_n) || (!rAdrEmulated && iFmSlp3_n))      //but it could be emulated and wanted to recover only after pressing powerbutton, or not emulated and follow PCH 
                         begin
                            rMasterSeqFsm <= PCH;                                              //going back to PCH as sequence is powering up back again
                         end
                       else
                         rMasterSeqFsm <= ADR;                             
                    end
                  endcase // case (rMasterSeqFsm)
               end // if (iSysChkGood)
          end // else: !if(!iRst_n)        
     end // always @ (posedge iClk)
        
   //////////////////////////////////
   //combinational outputs
   //////////////////////////////////////////////////////////////////
   
   
   assign oPwrDwn = wPwrDwn;
   assign oS3Indication = rS3Indication; //alarios: || (!iEmulAdrRDimm_n && wS4Indication);       //added iDimm12VCpsS5_n in the equation, we assert S3 indication 
                                                                                      // if we are in S5/S4 but the jumper is placed (iDimm12VcpsS5_n asserted)
   assign oS5Indication = rS4Indication;
   
   assign oMasterPostCode = rMasterSeqFsm;     //assign the FSM value to output to redirect to 7-seg displays
   
   assign oFailId = rfaultSt || wFailure || wFailure2;    //if CPU/MEM, PSU, failure is detected, signal to S3_SW logic to avoid re-enable if PCH tries to reboot
                                                          //added wFailure2 (PCH or BMC failure) to pass this also to CPU sequencer
   
   assign oAdrEmulated = rAdrEmulated;        //When ADR_MODE[1:0] = 2'b10, this one is asserted and indicates EMULATED ADR FLOW (with R-DIMMs)
endmodule // master_fub
