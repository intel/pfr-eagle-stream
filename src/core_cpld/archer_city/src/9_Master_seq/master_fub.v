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
    input iClk,                                      //input clock signal (@2MHz)
    input iRst_n,                                    //input reset signal (active low)

    input iSysChkGood,                               //input System Check is Good. It enables the Master fub execution

    input iBmcOnCtl_n,                               //from BMC, FM_BMC_ONCTL_N (active low)
    input iFmSlp3_n,                                 //Sleep S3 signal
    input iFmSlp4_n,                                 //Sleep S4 signal. Between sleep signals (S3&S4) we can determine if we are in S0, S3 or S4
    input iFmSlpSusRsmRst_n,                         //Sleep SUS (active low) signal, coming from PCH when receiving P3V3_DSW (needed to continue with PCH seq)

    input iDediBusy_n,                               //Dedi_Busy (active low) would cause a power down sequence when asserted
   
    input iBmcDone,                                  //BMC sequencer done
    input iPchDone,                                  //PCH sequencer done
    input iPsuDone,                                  //PSu_MAIN sequencer done
    input iCpuMemDone,                               //CPU_MEM sequencer done

    input iBmcFailure,                               //failure indication from BMC sequencer
    input iPchFailure,                               //failure indication from PCH sequencer
    input iPsuFailure,                               //failure indication from PSU_MAIN sequencer
    input iCpuMemFailure,                            //failure indication from CPU_MEM sequencer

    output reg oBmcEna,                              //Enable BMC sequencer to start
    output reg oPchEna,                              //Enable PCH sequencer to start
    output reg oPsuEna,                              //Enable PSU_MAIN sequencer to start
    output reg oCpuMemEna,                           //Enable CPU_MEM sequencer to start
    output reg oAdrEna,                              //Enable for ADR FSM 

    output reg oPwrGdPchPwrOK,                       //PCH PWR-OK
    output oPwrGdSysPwrOK,                       //SYS PWR-OK

    output oS3Indication,                            //we are in S3 indication
    output oS5Indication,                            //we are in S4/S5 indication

    output oPwrDwn                                   //Power Down Indication for all sequencers 
   );


   //////////////////////////////////
   //local params declarations
   //////////////////////////////////////////////////////////////////

   localparam IDLE = 0;
   localparam BMC = 1;
   localparam PCH = 2;
   localparam PSU = 3;
   localparam CPU_MEM = 4;
   localparam PLATFORM_ON = 5;
   localparam ERROR = 6;
   
   
   //////////////////////////////////
   //internal signals declarations
   //////////////////////////////////////////////////////////////////
   
   reg [2:0] rMasterSeqFsm;

   wire    wStart100msTimer;
   wire    wDoneTimer100ms;
   wire    wDoneTimer1ms;
   

//   wire    wS0Indication;
   wire    wS3Indication;
   wire    wS4Indication;

   wire    wFailure;
   wire    wFailure2;
   
   wire    wPwrDwn;
   
   
   

   //////////////////////////////////
   //sub modules instantiation
   //////////////////////////////////////////////////////////////////
   
   //100-msec Timer, started when MAIN rails are on. Output is ANDed with CpuMemDone signal (last turned-on VRs) to generate PWRGD_SYS_PWROK & PWRGD_PCH_PWROK
   delay #(.COUNT(200000)) 
     Timer100ms(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(wStart100msTimer),
               .iClrCnt(1'b0),
               .oDone(wDoneTimer100ms)
               );


   //1-msec Timer, started when S3 is asserted, cleared when S3 is HIGH
   delay #(.COUNT(2000)) 
     Timer1ms(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(!iFmSlp3_n && iFmSlp4_n),
               .iClrCnt(1'b0),
               .oDone(wDoneTimer1ms)
               );


   //11-msec Timer, started when S3 is asserted, cleared when S3 is HIGH
   delay #(.COUNT(22000)) 
     Timer11ms(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(oPwrGdPchPwrOK),
               .iClrCnt(1'b0),
               .oDone(oPwrGdSysPwrOK)
               );



   //////////////////////////////////
   //internal signals assignments
   //////////////////////////////////////////////////////////////////
   

   //Power Supply PWR OK signal is used to start the 100

   //when PSU_MAIN is done, P3V3 main is on and a 100ms timer starts
   assign  wStart100msTimer = iPsuDone;  //iPwrGdPsPwrOK;


   //
   //assign  wS0Indication = iFmSlp3_n && iFmSlp4_n;                              //if both in '1', means we are into S0 state or in the way to S0
   assign  wS3Indication = !iFmSlp3_n && iFmSlp4_n && wDoneTimer1ms;             //if Slp3 asserted (active low) and Slp4 deasserted during 1ms, we are in S3 state
   
   assign  wS4Indication = !iFmSlp4_n;                                            //Slp4 asserted (active low), we are in S4/S5 state


   //we distinguish if it is a BMC or PCH VR failure vs the rest of sequencers. If BMC/PCH VR fails, FPGA will need to turn off all VRs, while if any of the other fails, PCH & BMC VRs will remain on (in S5 state)
   assign  wFailure = iPsuFailure || iCpuMemFailure;    //PSU_MAIN or CPU_MEM sequencers failure detection will generate a failure indication for pwr down

   assign  wFailure2 = iBmcFailure || iPchFailure;      //BMC or PCH sequencers failure detection will generate a failure indication for pwr down
      
   
   //PwrDwn indication depends if any Sequencer asserted failure indication or we are going to S3/S4

   assign  wPwrDwn = wS3Indication || wS4Indication || wFailure || wFailure2 || iDediBusy_n;            //PwrDwn indication depends if any Sequencer asserted failure indication or we are going to S3/S4
 
   


   //////////////////////////////////
   //Master Sequencer FSM
   //////////////////////////////////////////////////////////////////
   
   always @ (posedge iClk)
     begin
        if (!iRst_n)     //synchronous reset condition
          begin

             rMasterSeqFsm <= IDLE;
             
             oBmcEna <= 1'b0;
             oPchEna <= 1'b0;
             oPsuEna <= 1'b0;
             oCpuMemEna <= 1'b0;
             oAdrEna <= 1'b0;
             oPwrGdPchPwrOK <= 1'b0;
             //oPwrGdSysPwrOK <= 1'b0;
          end // if (!iRst_n)
        else if (iSysChkGood)
          begin
             case(rMasterSeqFsm)
               default: begin         //IDLE
                  if (!wFailure2)
                    begin
                       oBmcEna <= 1'b1;                                  //Enable for BMC sequencer
                       rMasterSeqFsm <= BMC;
                    end
                       else if (wFailure)
                         begin
                            rMasterSeqFsm <= ERROR;
                         end
               end // case: default
               
               BMC: begin
                  if (wFailure2)                                    //Power Down Condition detected
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
                  if (wFailure2)                                    //Power Down Condition detected due to a BMC or PCH failure
                    begin
                       oPchEna <= 1'b0;                             //disabling PCH Sequencer
                       if (iPchDone == 1'b0)                        //if PCH Seq is done with pwr-dwn, go to next power down state
                         begin
                            rMasterSeqFsm <= BMC;                   //next pwr dwn state is BMC 
                            oBmcEna <= 1'b0;                        
                         end
                    end //if (wFailure2)
                  else if (iPchDone && ~iBmcOnCtl_n && iFmSlp3_n && ~wPwrDwn)     //not in regular pwrdown, then pwrup, when PCH seq finishes and BMC asserts On-Ctl signal... also not in any sleep state (redunant check added for VRTB)
                    begin
                       oPsuEna <= 1'b1;                                           //Enable for PSU_Main sequencer
                       rMasterSeqFsm <= PSU;
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
               end // case: PSU
               
               CPU_MEM: begin
                  if (wPwrDwn)                                      //Power Down Condition detected
                    begin
                       oCpuMemEna <= 1'b0;                          //disabling CpuMem Sequencer
                       oPwrGdPchPwrOK <= 1'b0;                      //deasserting PCH & SYS PWR OK signals as we are going down
                       //oPwrGdSysPwrOK <= 1'b0;
                       if (iCpuMemDone == 1'b0)                     //if CpuMem Seq is done with pwr-dwn, go to next power down state
                         begin
                            rMasterSeqFsm <= PSU;                   //next pwr dwn state is Psu-Main 
                            oPsuEna <= 1'b0;
                            oAdrEna <= 1'b0;                        //disabling ADR FSM
                         end
                    end
                  else if (iCpuMemDone == 1'b1 /*&& wDoneTimer100ms == 1'b1*/)
                    begin
                       rMasterSeqFsm <= PLATFORM_ON;                //All VRs turned-on and 100msec timer expired so we assert PCH and SYS PWR OK signals
                       oPwrGdPchPwrOK <= 1'b1;
                       //oPwrGdSysPwrOK <= 1'b1;
                       oAdrEna <= 1'b1;                             //enable ADR FSM
                    end
               end // case: CPU_MEM
               
               PLATFORM_ON: begin                                   //Only monitoring for a power down condition (a failure or going to S3 or S5
                  if (wPwrDwn)                                      //Power Down Condition detected
                    begin
                       rMasterSeqFsm <= CPU_MEM;
                       oCpuMemEna <= 1'b0;                          //going back for powering down, starting with CPU
                    end
               end //case: PLATFORM_ON

               
               ERROR: begin                                         //if you fall into an error condition, only way out is a reset (AC cycle)
                  rMasterSeqFsm <= ERROR;
                  
               end //case: ERROR
             endcase // case (rMasterSeqFsm)
          end // if (iSysChkGood)       
     end // always @ (posedge iClk)
   
   //////////////////////////////////
   //combinational outputs
   //////////////////////////////////////////////////////////////////
   
   
   assign oPwrDwn = wPwrDwn;
   assign oS3Indication = wS3Indication;
   assign oS5Indication = wS4Indication;
   
   
endmodule // master_fub
