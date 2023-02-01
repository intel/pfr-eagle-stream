//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>This FUB is in charge of collecting failures from each sequencer and decode to 2 7-segments displays</b> 
    \details    Each VR can have 2 types of failure coded. VR enabled and never asserted pwrgd (timer expires) or VR asserted
                pwrGd signal and then deasserted without being disabled (PwrGd Failure)
                The MSb of each ErrorCode input is used as an indication if it was a timer issue (1'b1) or a PwrGd issue (1'b0)
 
    \file       postcode_fub.v
    \author     amr/a.larios@intel.com
    \date       Aug 27, 2019
    \brief      $RCSfile: postcode_fub.v.rca $
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


module postcode_fub
  (
    input iClk,
    input iRst_n,
   
    input [4:0] iBmcErrorCode,                        //BMC Error Code (for 4 VRs, 8 error id's)
    input [3:0] iPchErrorCode,                        //PCH Error Code (for 3 VRs, 6 error id's)
    input [2:0] iPsuErrorCode,                        //PSU-MAIN Error Code (for 2 VRs, 4 error id's)
    input [7:0] iCpu0ErrorCode,                       //CPU0 Error Code (for 7 VRs, 14 error id's)
    input [7:0] iCpu1ErrorCode,                       //CPU1 Error Code (for 7 VRs, 14 error id's)
    input [8:0] iMemErrorCode,                        //MEM Error Code (for 8 PWRGD_FAIL signals, 16 error id's)

    output      oBmcFail,                             //BMC failure indication for Master Seq
    output      oPchFail,                             //PCH failure indication for Master Seq
    output      oPsuFail,                             //PSU_MAIN failure indication for Master Seq
    output      oCpuMemFail,                          //CPU_MEM failure indication for Master Seq

    output reg [7:0] oDisplay1,
    output reg [7:0] oDisplay2
   );


   //////////////////////////////////
   //local params definitios for FSM
   //////////////////////////////////////////////////////////////////

   localparam    INIT = 0;
   localparam    BMC = 1;
   localparam    PCH = 2;
   localparam    PSU_MAIN = 3;
   localparam    CPU0 = 4;
   localparam    CPU1 = 5;
   localparam    MEM = 6;
   

   

   //////////////////////////////////
   //internal signals declarations
   //////////////////////////////////////////////////////////////////


   reg           [2:0] rPostCodeFSM;                        //post codes FSM
   reg           rStartTimer1s;                       //Start signal for 1 sec timer (to switch between post codes when more than 1 sequencer sends an error code <> 00h
   reg           rClrCntr;                            //Clear counter signal

   
   wire          wBmcFail;                            //bit-wise OR with input BMC error code to identify there was a failure in BMC
   wire          wPchFail;                            //bit-wise OR with input PCH error code to identify there was a failure in PCH
   wire          wPsuFail;                            //bit-wise OR with input PSU-MAIN error code to identify there was a failure in PSU or Main VR
   wire          wCpu0Fail;                           //bit-wise OR with input CPU0 error code to identify there was a failure in CPU0
   wire          wCpu1Fail;                           //bit-wise OR with input CPU1 error code to identify there was a failure in CPU1
   wire          wMemFail;                            //bit-wise OR with input MEM error code to identify there was a failure in MEM

   wire          wTimer1sDone;                        //done signal from 1-sec timer (when count reaches 1 sec)
   
   

   //////////////////////////////////
   //sub modules instantiation
   //////////////////////////////////////////////////////////////////
   
   //1-sec Timer, started when MAIN rails are on. Output is ANDed with CpuMemDone signal (last turned-on VRs) to generate PWRGD_SYS_PWROK & PWRGD_PCH_PWROK
   delay #(.COUNT(2000000)) 
     Timer1sec(
               .iClk(iClk),                            //2MHz clock signal
               .iRst(iRst_n),
               .iStart(rStartTimer1s),
               .iClrCnt(rClrCntr),                     //clears the count regardless of Start signal
               .oDone(wTimer1sDone)
               );
   

   //////////////////////////////////
   //Combinational logic to get failure indication from each sequencer
   //////////////////////////////////////////////////////////////////
   
   assign        wBmcFail = |(iBmcErrorCode);
   assign        wPchFail = |(iPchErrorCode);
   assign        wPsuFail = |(iPsuErrorCode);
   assign        wCpu0Fail = |(iCpu0ErrorCode);
   assign        wCpu1Fail = |(iCpu1ErrorCode);
   assign        wMemFail = |(iMemErrorCode);

    

   //////////////////////////////////
   //FSM
   //////////////////////////////////////////////////////////////////
   

   always @(posedge iClk)
     begin
        if (!iRst_n)
          begin
             rPostCodeFSM <= INIT;
             rStartTimer1s <= 1'b1;
             rClrCntr <= 1'b0;
             oDisplay1 <= 8'hBF;     //.-
             oDisplay2 <= 8'hBF;     //.-
          end
        else
          case (rPostCodeFSM)
            default: begin                          //INIT case
               rClrCntr <= 1'b0;
               casex ({wMemFail,wCpu1Fail,wCpu0Fail, wPsuFail, wPchFail, wBmcFail})
                 6'bXXXXX1:begin                                           //BMC failure
                    rPostCodeFSM <= BMC;
                    rStartTimer1s <= 1'b1;
                 end
                 6'bXXXX10:begin                                           //PCH failure
                    rPostCodeFSM <= PCH;
                    rStartTimer1s <= 1'b1;
                 end
                 6'bXXX100:begin                                           //PSU_MAIN failure
                    rPostCodeFSM <= PSU_MAIN;
                    rStartTimer1s <= 1'b1;
                 end
                 6'bXX1000:begin                                           //CPU0 failure
                    rPostCodeFSM <= CPU0;
                    rStartTimer1s <= 1'b1;
                 end
                 6'bX10000:begin                                           //CPU1 failure
                    rPostCodeFSM <= CPU1;
                    rStartTimer1s <= 1'b1;
                 end
                 6'b100000:begin                                           //MEM failure
                    rPostCodeFSM <= MEM;
                    rStartTimer1s <= 1'b1;
                 end
                 default: begin                                          //no failures
                    rPostCodeFSM <= INIT;
                    rStartTimer1s <= 1'b0;
                 end
               endcase // casex ({wCpuMemFail, wPsuFail, wPchFail, wBmcFail})
            end // case: default
            
            BMC: begin
               casex (iBmcErrorCode)
                 default:                      //5'h01: VR P2V5_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h79;      //display .0.1
                   end
                 5'b0XX10:                        //VR P1V0_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h24;      //display .0.2
                   end
                 5'b0X100:                        //VR P1V2_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h30;      //display .0.3
                   end
                 5'b01000:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h19;      //display .0.4
                   end
                 5'b1XXX1:                        //VR P2V5_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h40;      //display .2.0
                   end
                 5'b1XX10:                        //VR P1V0_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h79;      //display .2.1
                   end
                 5'b1X100:                        //VR P1V2_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h24;      //display .2.2
                   end
                 5'b11000:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h30;      //display .2.3
                   end
               endcase // case (iBmcErrorCode)
               if (wTimer1sDone)
                 begin
                    rClrCntr <= 1'b1;         //clear count to restartit
                    casex ({wMemFail,wCpu1Fail,wCpu0Fail, wPsuFail, wPchFail})
                      5'bXXXX1:begin                                            //PCH failure
                         rPostCodeFSM <= PCH;
                         rStartTimer1s <= 1'b1;
                      end
                      5'bXXX10:begin                                           //PSU_MAIN failure
                         rPostCodeFSM <= PSU_MAIN;
                         rStartTimer1s <= 1'b1;
                      end
                      5'bXX100:begin                                           //CPU0 failure
                         rPostCodeFSM <= CPU0;
                         rStartTimer1s <= 1'b1;
                      end
                      5'bX1000:begin                                           //CPU1 failure
                         rPostCodeFSM <= CPU1;
                         rStartTimer1s <= 1'b1;
                      end
                      5'b10000:begin                                           //MEM failure
                         rPostCodeFSM <= MEM;
                         rStartTimer1s <= 1'b1;
                      end
                      default: begin                                          //no failures
                         rPostCodeFSM <= INIT;
                         rStartTimer1s <= 1'b0;
                      end
                    endcase // casex ({wCpuMemFail, wPsuFail, wPchFail})
                 end // if (wTimer1sDone)
            end // case: BMC


            PCH: begin
               rClrCntr <= 1'b0;
               casex (iPchErrorCode)
                 default:                      //4'h01: VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h12;      //display .0.5
                   end
                 4'b0X10:                        //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h02;      //display .0.6
                   end
                 4'b0100:                        //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h78;      //display .0.7
                   end
                 4'b1XX1:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h19;      //display .2.4
                   end
                 4'b1X10:                        //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h12;      //display .2.5
                   end
                 4'b1100:                        //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h02;      //display .2.6
                   end
               endcase // case (iPCHErrorCode)
                 
               if (wTimer1sDone && rClrCntr == 1'b0)
                 begin
                    rClrCntr <= 1'b1;        //clear count to restart it
                    casex ({wMemFail,wCpu1Fail,wCpu0Fail, wPsuFail})
                      4'bXXX1:begin                                           //PSU_MAIN failure
                         rPostCodeFSM <= PSU_MAIN;
                         rStartTimer1s <= 1'b1;
                      end
                      4'bXX10:begin                                           //CPU0 failure
                         rPostCodeFSM <= CPU0;
                         rStartTimer1s <= 1'b1;
                      end
                      4'bX100:begin                                           //CPU1 failure
                         rPostCodeFSM <= CPU1;
                         rStartTimer1s <= 1'b1;
                      end
                      4'b1000:begin                                           //MEM failure
                         rPostCodeFSM <= MEM;
                         rStartTimer1s <= 1'b1;
                      end
                      default: begin                                          //no failures
                         rPostCodeFSM <= INIT;
                         rStartTimer1s <= 1'b0;
                      end
                    endcase // casex ({wCpuMemFail, wPsuFail})
                 end // if (wTimer1sDone)
            end // case: PCH


            PSU_MAIN: begin
               rClrCntr <= 1'b0;
               case (iPsuErrorCode)
                 default:                      //3'h1: PSU_PWROK was on and went down (PSU failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h00;      //display .0.8
                   end
                 3'b010:                         //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h20;      //display .0.9
                   end
                 3'b101:                         //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h78;      //display .2.7
                   end
                 3'b110:                         //PS_PWROK was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h00;      //display .2.8
                   end                         //VR P1V05_PCH_AUX was never asserted (timer expired)

               endcase // case (iPsuErrorCode)
                 
               if (wTimer1sDone && rClrCntr == 1'b0)
                 begin
                    rClrCntr <= 1'b1;         //clear count to restart it
                    casex ({wMemFail,wCpu1Fail,wCpu0Fail})
                      3'bXX1:begin                                            //CPU failure
                         rPostCodeFSM <= CPU0;
                         rStartTimer1s <= 1'b1;
                      end
                      3'bX10:begin                                            //CPU failure
                         rPostCodeFSM <= CPU1;
                         rStartTimer1s <= 1'b1;
                      end
                      3'b100:begin                                            //CPU failure
                         rPostCodeFSM <= MEM;
                         rStartTimer1s <= 1'b1;
                      end
                      default: begin                               
                         rPostCodeFSM <= INIT;
                         rStartTimer1s <= 1'b0;
                      end
                    endcase // casex ({wCpuMemFail})
                 end // if (wTimer1sDone)
            end // case: PSU_MAIN

            CPU0: begin
               rClrCntr <= 1'b0;
               casex (iCpu0ErrorCode)
                 default:                      //3'h1: PVPP_HBM_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h08;      //display .0.A
                   end
                 8'b0XXXXX10:                         //VR PVCCFA_EHV_CPU0  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h46;      //display .0.C
                   end
                 8'b0XXXX100:                         //VR PVCCFA_EHV_FIVRA_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h06;      //display .0.E
                   end
                 8'b0XXX1000:                         //VR PVCCINFAON_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h40;      //display .1.0
                   end
                 8'b0XX10000:                         //VR PVNN_MAIN_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h24;      //display .1.2
                   end
                 8'b0X100000:                         //VR PVCCD_HV_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h19;      //display .1.4
                   end
                 8'b01000000:                         //VR PVCCIN_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h02;      //display .1.6
                   end

                 
                 8'b1XXXXXX1:                        //PVPP_HBM_CPU0 was never asserted (timer expired)
                 begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h10;      //display .2.9
                   end
                 8'b1XXXXX10:                         //VR PVCCFA_EHV_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h03;      //display .2.b
                   end
                 8'b1XXXX100:                         //VR PVCCFA_EHV_FIVRA_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h21;      //display .2.d
                   end
                 8'b1XXX1000:                         //VR PVCCINFAON_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h0E;      //display .2.F
                   end
                 8'b1XX10000:                         //VR PVNN_MAIN_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h79;      //display .3.1
                   end
                 8'b1X100000:                         //VR PVCCD_HV_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h30;      //display .3.3
                   end
                 8'b11000000:                         //VR PVCCIN_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h12;      //display .3.5
                   end

               endcase // case (iCpu0Error)
               
               
               if (wTimer1sDone && rClrCntr == 1'b0)
                 begin
                    rClrCntr <= 1'b1;         //clear count to restart it
                    casex ({wMemFail,wCpu1Fail})
                      2'bX1:begin                                            //CPU failure
                         rPostCodeFSM <= CPU1;
                         rStartTimer1s <= 1'b1;
                      end
                      2'b10:begin                                            //CPU failure
                         rPostCodeFSM <= MEM;
                         rStartTimer1s <= 1'b1;
                      end
                      default: begin                               
                         rPostCodeFSM <= INIT;
                         rStartTimer1s <= 1'b0;
                      end
                    endcase // casex ({wCpuMemFail})
                 end // if (wTimer1sDone)
            end // case: CPU0


            CPU1: begin
               rClrCntr <= 1'b0;
               casex (iCpu1ErrorCode)
                 default:                      //8'h1: PVPP_HBM_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h03;      //display .0.b
                   end
                 8'b0XXXXX10:                         //VR PVCCFA_EHV_CPU1  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h21;      //display .0.d
                   end
                 8'b0XXXX100:                         //VR PVCCFA_EHV_FIVRA_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h40;
                      oDisplay1 <= 8'h0E;      //display .0.F
                   end
                 8'b0XXX1000:                         //VR PVCCINFAON_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h79;      //display .1.1
                   end
                 8'b0XX10000:                         //VR PVNN_MAIN_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h30;      //display .1.3
                   end
                 8'b0X100000:                         //VR PVCCD_HV_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h12;      //display .1.5
                   end
                 8'b01000000:                         //VR PVCCIN_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h78;      //display .1.7
                   end

                 
                 8'b1XXXXXX1:                        //PVPP_HBM_CPU1 was never asserted (timer expired)
                 begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h08;      //display .2.A
                   end
                 8'b1XXXXX10:                         //VR PVCCFA_EHV_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h46;      //display .2.C
                   end
                 8'b1XXXX100:                         //VR PVCCFA_EHV_FIVRA_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h06;      //display .2.E
                   end
                 8'b1XXX1000:                         //VR PVCCINFAON_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h40;      //display .3.0
                   end
                 8'b1XX10000:                         //VR PVNN_MAIN_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h24;      //display .3.2
                   end
                 8'b1X100000:                         //VR PVCCD_HV_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h19;      //display .3.4
                   end
                 8'b11000000:                         //VR PVCCIN_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h02;      //display .3.6
                   end

               endcase // case (iCpu1Error)
               
               if (wTimer1sDone && rClrCntr == 1'b0)
                 begin
                    rClrCntr <= 1'b1;         //clear count to restart it
                    casex (wMemFail)
                      2'b1:begin                                            //CPU failure
                         rPostCodeFSM <= MEM;
                         rStartTimer1s <= 1'b1;
                      end
                      default: begin                               
                         rPostCodeFSM <= INIT;
                         rStartTimer1s <= 1'b0;
                      end
                    endcase // casex (wMemFail)
                 end // if (wTimer1sDone)
            end // case: CPU1
            

            MEM: begin
               rClrCntr <= 1'b0;
               casex (iMemErrorCode)
                 default:                      //8'h1: PWRGD_FAIL_CPU0_AB was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h00;      //display .1.8
                   end
                 9'b0XXXXXX10:                         //VR PWRGD_FAIL_CPU1_AB  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h10;      //display .1.9
                   end
                 9'b0XXXXX100:                         //VR PWRGD_FAIL_CPU0_CD was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h08;      //display .1.A
                   end
                 9'b0XXXX1000:                         //VR PWRGD_FAIL_CPU1_CD was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h03;      //display .1.b
                   end
                 9'b0XXX10000:                         //VR PWRGD_FAIL_CPU0_EF was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h46;      //display .1.C
                   end
                 9'b0XX100000:                         //VR PWRGD_FAIL_CPU1_EF was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h21;      //display .1.d
                   end
                 9'b0X1000000:                         //VR PWRGD_FAIL_CPU0_GH was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h06;      //display .1.E
                   end
                 9'b010000000:                         //VR PWRGD_FAIL_CPU1_GH was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h0E;      //display .1.F
                   end

                 
                 9'b1XXXXXXX1:                       //PWRGD_FAIL_CPU0_AB was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h78;      //display .3.7
                   end
                 9'b1XXXXXX10:                         //VR PWRGD_FAIL_CPU1_AB  was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h00;      //display .3.8
                   end
                 9'b1XXXXX100:                         //VR PWRGD_FAIL_CPU0_CD was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h10;      //display .3.9
                   end
                 9'b1XXXX1000:                         //VR PWRGD_FAIL_CPU1_CD was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h08;      //display .3.A
                   end
                 9'b1XXX10000:                         //VR PWRGD_FAIL_CPU0_EF was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h03;      //display .3.b
                   end
                 9'b1XX100000:                         //VR PWRGD_FAIL_CPU1_EF was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h46;      //display .3.C
                   end
                 9'b1X1000000:                         //VR PWRGD_FAIL_CPU0_GH was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h21;      //display .3.d
                   end
                 9'b110000000:                         //VR PWRGD_FAIL_CPU1_GH was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h06;      //display .3.E
                   end

               endcase // case (iCpu0Error)
               
               
               if (wTimer1sDone && rClrCntr == 1'b0)
                 begin
                    rClrCntr <= 1'b1;         //clear count to restart it
                    rPostCodeFSM <= INIT;
                    rStartTimer1s <= 1'b0;
                 end // if (wTimer1sDone)
            end // case: MEM

          endcase // case (rPostCodeFSM)
     end // always @ (posedge iClk)



   //////////////////////////////////
   //Combinational outputs
   //////////////////////////////////////////////////////////////////
   
   assign        oBmcFail = wBmcFail;
   assign        oPchFail = wPchFail;
   assign        oPsuFail = wPsuFail;
   assign        oCpuMemFail = wCpu0Fail || wCpu1Fail || wMemFail;              //merged together to connect with master sequence


   
endmodule // postcode_fub
