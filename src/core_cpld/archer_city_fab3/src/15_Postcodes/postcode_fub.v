//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>This FUB is in charge of collecting Master Sequence State progress in post-codes, and also </b>
                <b>failures from each secondary sequencer and decode to 2 7-segments displays</b> 

    \details    Besides master-sequencer states, each VR can have 2 types of failure coded. VR enabled and never asserted pwrgd (timer expires) or VR asserted
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
    input            iClk,
    input            iRst_n,

    input [3:0]      iMstrPostCode, //Master Sequence State post-code
    input [4:0]      iBmcErrorCode, //BMC Error Code (for 4 VRs, 8 error id's)
    input [3:0]      iPchErrorCode, //PCH Error Code (for 3 VRs, 6 error id's)
    input [2:0]      iPsuErrorCode, //PSU-MAIN Error Code (for 2 VRs, 4 error id's)
    input [7:0]      iCpu0ErrorCode, //CPU0 Error Code (for 7 VRs, 14 error id's)
    input [7:0]      iCpu1ErrorCode, //CPU1 Error Code (for 7 VRs, 14 error id's)
    input [8:0]      iMemErrorCode, //MEM Error Code (for 8 PWRGD_FAIL signals, 16 error id's)

    output           oBmcFail, //BMC failure indication for Master Seq
    output           oPchFail, //PCH failure indication for Master Seq
    output           oPsuFail, //PSU_MAIN failure indication for Master Seq
    output           oCpuMemFail, //CPU_MEM failure indication for Master Seq

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
   reg           rClrCntr;                            //Clear counter signal, to restart the timer (for the post-codes transitions)

   
   wire          wBmcFail;                            //bit-wise OR with input BMC error code to identify there was a failure in BMC
   wire          wPchFail;                            //bit-wise OR with input PCH error code to identify there was a failure in PCH
   wire          wPsuFail;                            //bit-wise OR with input PSU-MAIN error code to identify there was a failure in PSU or Main VR
   wire          wCpu0Fail;                           //bit-wise OR with input CPU0 error code to identify there was a failure in CPU0
   wire          wCpu1Fail;                           //bit-wise OR with input CPU1 error code to identify there was a failure in CPU1
   wire          wMemFail;                            //bit-wise OR with input MEM error code to identify there was a failure in MEM

   wire          wTimer1sDone;                        //done signal from 1-sec timer (when count reaches 1 sec)
   
   reg [7:0]     rToMonitor;        //alarios: dbg info, Fpga PostCodes
   

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
   

   always @(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             rPostCodeFSM <= INIT;
             rStartTimer1s <= 1'b1;
             rClrCntr <= 1'b0;
             oDisplay1 <= 8'hBF;     //.-
             oDisplay2 <= 8'hBF;     //.-
             
             rToMonitor <= 8'h00;    //to internal monitor
             
          end
        else
          case (rPostCodeFSM)
            default: begin                          //INIT case where the MSTR post-codes should be displayed
               rClrCntr <= 1'b0;

               rToMonitor <= {4'h0, iMstrPostCode};    //to internal monitor
               
               casex (iMstrPostCode)
                 4'b000: begin                   //Mstr-Seq in IDLE
                    oDisplay1 <= 8'h40;          //Display .0.0
                    oDisplay2 <= 8'h40;
                 end

                 4'b001: begin                   //Mstr-Seq in BMC
                    oDisplay1 <= 8'h79;          //Display .0.1
                    oDisplay2 <= 8'h40;
                 end

                 4'b010: begin
                    oDisplay1 <= 8'h24;          //Mstr-Seq in PCH
                    oDisplay2 <= 8'h40;          //Display .0.2
                 end

                 4'b011: begin
                    oDisplay1 <= 8'h30;          //Mstr-Seq in PSU
                    oDisplay2 <= 8'h40;          //Display .0.3
                 end

                 4'b100: begin
                    oDisplay1 <= 8'h19;          //Mstr-Seq in CPU_MEM
                    oDisplay2 <= 8'h40;          //Display .0.4
                 end

                 4'b101: begin
                    oDisplay1 <= 8'h12;          //Mstr-Seq in PLTRST 
                    oDisplay2 <= 8'h40;          //Display .0.5
                 end

                 4'b110: begin
                    oDisplay1 <= 8'h02;          //Mstr-Seq in PLATFORM_ON
                    oDisplay2 <= 8'h40;          //Display .0.6
                 end

                 4'b111: begin
                    oDisplay1 <= 8'h78;          //Mstr-Seq in ERROR
                    oDisplay2 <= 8'h40;          //Display .0.7
                 end

                 4'b1000: begin
                    oDisplay1 <= 8'h00;          //Mstr-Seq in ADR
                    oDisplay2 <= 8'h40;          //Display .0.8
                 end

                 4'b1001: begin
                    oDisplay1 <= 8'h20;          //Mstr-Seq in PWR-DWN
                    oDisplay1 <= 8'h40;          //Display .0.9
                 end
                 
                 default:
                   begin
                      oDisplay1 <= 8'hFE;        //Display . .
                      oDisplay2 <= 8'hFE;
                   end
                 
               endcase // casex (iMstrPostCode)
               
               if (iMstrPostCode == 4'b0111) begin                     //if Mstr-Seq in ERROR state, then check Secondary Sequencers for error codes to be displayed instead
               
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
                    default: begin                                            //no failures
                       rPostCodeFSM <= INIT;
                       rStartTimer1s <= 1'b0;
                    end
                  endcase // casex ({wCpuMemFail, wPsuFail, wPchFail, wBmcFail})
               end // if (iMstrPostCode == 4'b0111)
            end // case: default
            
           
            BMC: begin
               casex (iBmcErrorCode)
                 default:                      //5'h01: VR P2V5_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h79;      //display .1.1

                      rToMonitor <= 8'h11 ;    //to internal monitor
                   end
                 5'b0XX10:                        //VR P1V0_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h24;      //display .1.2
                      
                      rToMonitor <= 8'h12;     //to internal monitor
                   end
                 5'b0X100:                        //VR P1V2_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h30;      //display .1.3
                      
                      rToMonitor <= 8'h13;     //to internal monitor
                   end
                 5'b01000:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h19;      //display .1.4
                      
                      rToMonitor <= 8'h14;     //to internal monitor
                   end


                 5'b1XXX1:                        //VR P2V5_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h40;      //display .3.0
                      
                      rToMonitor <= 8'h30;     //to internal monitor
                   end
                 5'b1XX10:                        //VR P1V0_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h79;      //display .3.1

                      rToMonitor <= 8'h31;     //to internal monitor
                   end
                 5'b1X100:                        //VR P1V2_BMC_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h24;      //display .3.2

                      rToMonitor <= 8'h32;     //to internal monitor
                   end
                 5'b11000:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h30;      //display .3.3

                      rToMonitor <= 8'h33;     //to internal monitor
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
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h12;      //display .1.5

                      rToMonitor <= 8'h15;     //to internal monitor
                   end
                 4'b0X10:                        //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h02;      //display .1.6

                      rToMonitor <= 8'h16;     //to internal monitor
                   end
                 4'b0100:                        //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h78;      //display .1.7

                      rToMonitor <= 8'h17;     //to internal monitor
                   end

                 
                 4'b1XX1:                        //VR P1V8_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h19;      //display .3.4

                      rToMonitor <= 8'h34;     //to internal monitor
                   end
                 4'b1X10:                        //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h12;      //display .3.5

                      rToMonitor <= 8'h35;     //to internal monitor
                   end
                 4'b1100:                        //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h02;      //display .3.6

                      rToMonitor <= 8'h36;     //to internal monitor
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
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h00;      //display .1.8

                      rToMonitor <= 8'h18;     //to internal monitor
                   end
                 3'b010:                         //VR PVNN_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h20;      //display .1.9

                      rToMonitor <= 8'h19;     //to internal monitor
                   end

                 
                 3'b101:                         //VR P1V05_PCH_AUX was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h78;      //display .3.7

                      rToMonitor <= 8'h37;     //to internal monitor
                   end
                 3'b110:                         //PS_PWROK was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h00;      //display .3.8

                      rToMonitor <= 8'h38;     //to internal monitor
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
                 8'b0X0XXXX1:                  //8'h1: PVPP_HBM_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h08;      //display .1.A

                      rToMonitor <= 8'h1A;     //to internal monitor
                   end
                 8'b0X0XXX10:                         //VR PVCCFA_EHV_CPU0  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h46;      //display .1.C

                      rToMonitor <= 8'h1C;     //to internal monitor
                   end
                 8'b0X0XX100:                         //VR PVCCFA_EHV_FIVRA_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h06;      //display .1.E

                      rToMonitor <= 8'h1E;     //to internal monitor
                   end

                 
                 8'b0X0X1000:                         //VR PVCCINFAON_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h40;      //display .2.0

                      rToMonitor <= 8'h20;     //to internal monitor
                   end
                 8'b0X010000:                         //VR PVNN_MAIN_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h24;      //display .2.2

                      rToMonitor <= 8'h22;     //to internal monitor
                   end
                 8'b0X1XXXXX:                         //VR PVCCD_HV_CPU0 was on and went down (VR failure)   -- first in the sequence now
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h19;      //display .2.4

                      rToMonitor <= 8'h24;     //to internal monitor
                   end
                 8'b01000000:                         //VR PVCCIN_CPU0 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h02;      //display .2.6

                      rToMonitor <= 8'h26;     //to internal monitor
                   end

                 
                 8'b1X0XXXX1:                        //PVPP_HBM_CPU0 was never asserted (timer expired)
                 begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h10;      //display .3.9

                      rToMonitor <= 8'h39;     //to internal monitor
                   end
                 8'b1X0XXX10:                         //VR PVCCFA_EHV_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h03;      //display .3.b

                      rToMonitor <= 8'h3B;     //to internal monitor
                   end
                 8'b1X0XX100:                         //VR PVCCFA_EHV_FIVRA_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h21;      //display .3.d

                      rToMonitor <= 8'h3D;     //to internal monitor
                   end
                 8'b1X0X1000:                         //VR PVCCINFAON_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h0E;      //display .3.F

                      rToMonitor <= 8'h3F;     //to internal monitor
                   end
                 8'b1X010000:                         //VR PVNN_MAIN_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h79;      //display .4.1

                      rToMonitor <= 8'h41;     //to internal monitor
                   end
                 8'b1X1XXXXX:                         //VR PVCCD_HV_CPU0 was never asserted (timer expired)   -- first in the sequence now
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h30;      //display .4.3
                      rToMonitor <= 8'h43;     //to internal monitor
                   end
                 8'b11000000:                         //VR PVCCIN_CPU0 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h12;      //display .4.5
                      rToMonitor <= 8'h45;     //to internal monitor
                   end

                 default:
                   begin
                      oDisplay2 <= 8'hFF;
                      oDisplay1 <= 8'hFF;      //display nothing (both displays turned-off)
                      rToMonitor <= 8'h00;     //to internal monitor
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
                 8'b0X0XXXX1:                  //8'h1: PVPP_HBM_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h03;      //display .1.b

                      rToMonitor <= 8'h1B;     //to internal monitor
                   end
                 8'b0X0XXX10:                         //VR PVCCFA_EHV_CPU1  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h21;      //display .1.d

                      rToMonitor <= 8'h1D;     //to internal monitor
                   end
                 8'b0X0XX100:                         //VR PVCCFA_EHV_FIVRA_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h79;
                      oDisplay1 <= 8'h0E;      //display .1.F

                      rToMonitor <= 8'h1F;     //to internal monitor
                   end

                 
                 8'b0X0X1000:                         //VR PVCCINFAON_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h79;      //display .2.1

                      rToMonitor <= 8'h21;     //to internal monitor
                   end
                 8'b0X010000:                         //VR PVNN_MAIN_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h30;      //display .2.3

                      rToMonitor <= 8'h23;     //to internal monitor
                   end
                 8'b0X1XXXXX:                         //VR PVCCD_HV_CPU1 was on and went down (VR failure)   -- first in the sequence now
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h12;      //display .2.5

                      rToMonitor <= 8'h25;     //to internal monitor
                   end
                 8'b01000000:                         //VR PVCCIN_CPU1 was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h78;      //display .2.7

                      rToMonitor <= 8'h27;     //to internal monitor
                   end

                 
                 8'b1X0XXXX1:                        //PVPP_HBM_CPU1 was never asserted (timer expired)
                 begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h08;      //display .3.A

                      rToMonitor <= 8'h3A;     //to internal monitor
                   end
                 8'b1X0XXX10:                         //VR PVCCFA_EHV_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h30;
                      oDisplay1 <= 8'h46;      //display .3.C

                      rToMonitor <= 8'h3C;     //to internal monitor
                   end
                 8'b1X0XX100:                         //VR PVCCFA_EHV_FIVRA_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'H30;
                      oDisplay1 <= 8'h06;      //display .3.E

                      rToMonitor <= 8'h3E;     //to internal monitor
                   end
                 8'b1X0X1000:                         //VR PVCCINFAON_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h40;      //display .4.0

                      rToMonitor <= 8'h40;     //to internal monitor
                   end
                 8'b1X010000:                         //VR PVNN_MAIN_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h24;      //display .4.2

                      rToMonitor <= 8'h42;     //to internal monitor
                   end
                 8'b1X1XXXXX:                         //VR PVCCD_HV_CPU1 was never asserted (timer expired)   -- first in the sequence now
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h19;      //display .4.4

                      rToMonitor <= 8'h44;     //to internal monitor
                   end
                 8'b11000000:                         //VR PVCCIN_CPU1 was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h02;      //display .4.6

                      rToMonitor <= 8'h46;     //to internal monitor
                   end
                 
                 default:
                   begin
                      oDisplay2 <= 8'hFF;
                      oDisplay1 <= 8'hFF;      //display nothing (both displays turned-off)
                      
                      rToMonitor <= 8'h00;     //to internal monitor
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
                 9'b0XXXXXXX1:                          //VR PWRGD_FAIL_CPU0_AB was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h00;      //display .2.8

                      rToMonitor <= 8'h28;     //to internal monitor
                   end
                 9'b0XXXXXX10:                         //VR PWRGD_FAIL_CPU1_AB  was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h10;      //display .2.9

                      rToMonitor <= 8'h29;     //to internal monitor
                   end
                 9'b0XXXXX100:                         //VR PWRGD_FAIL_CPU0_CD was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h08;      //display .2.A

                      rToMonitor <= 8'h2A;     //to internal monitor
                   end
                 9'b0XXXX1000:                         //VR PWRGD_FAIL_CPU1_CD was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h03;      //display .2.b

                      rToMonitor <= 8'h2B;     //to internal monitor
                   end
                 9'b0XXX10000:                         //VR PWRGD_FAIL_CPU0_EF was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h46;      //display .2.C

                      rToMonitor <= 8'h2C;     //to internal monitor
                   end
                 9'b0XX100000:                         //VR PWRGD_FAIL_CPU1_EF was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h21;      //display .2.d

                      rToMonitor <= 8'h2D;     //to internal monitor
                   end
                 9'b0X1000000:                         //VR PWRGD_FAIL_CPU0_GH was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h06;      //display .2.E

                      rToMonitor <= 8'h2E;     //to internal monitor
                   end
                 9'b010000000:                         //VR PWRGD_FAIL_CPU1_GH was on and went down (VR failure)
                   begin
                      oDisplay2 <= 8'h24;
                      oDisplay1 <= 8'h0E;      //display .2.F

                      rToMonitor <= 8'h2F;     //to internal monitor
                   end

                 
                 9'b1XXXXXXX1:                       //PWRGD_FAIL_CPU0_AB was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h78;      //display .4.7

                      rToMonitor <= 8'h47;     //to internal monitor
                   end
                 9'b1XXXXXX10:                         //VR PWRGD_FAIL_CPU1_AB  was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h00;      //display .4.8

                      rToMonitor <= 8'h48;     //to internal monitor
                   end
                 9'b1XXXXX100:                         //VR PWRGD_FAIL_CPU0_CD was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h10;      //display .4.9

                      rToMonitor <= 8'h49;     //to internal monitor
                   end
                 9'b1XXXX1000:                         //VR PWRGD_FAIL_CPU1_CD was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h08;      //display .4.A

                      rToMonitor <= 8'h4A;     //to internal monitor
                   end
                 9'b1XXX10000:                         //VR PWRGD_FAIL_CPU0_EF was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h03;      //display .4.b

                      rToMonitor <= 8'h4B;     //to internal monitor
                   end
                 9'b1XX100000:                         //VR PWRGD_FAIL_CPU1_EF was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h46;      //display .4.C

                      rToMonitor <= 8'h4C;     //to internal monitor
                   end
                 9'b1X1000000:                         //VR PWRGD_FAIL_CPU0_GH was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h21;      //display .4.d

                      rToMonitor <= 8'h4D;     //to internal monitor
                   end
                 9'b110000000:                         //VR PWRGD_FAIL_CPU1_GH was never asserted (timer expired)
                   begin
                      oDisplay2 <= 8'h19;
                      oDisplay1 <= 8'h06;      //display .4.E

                      rToMonitor <= 8'h4E;     //to internal monitor
                   end

                 default:
                   begin
                      oDisplay2 <= 8'hFF;
                      oDisplay1 <= 8'hFF;      //display nothing (both displays turned-off)

                      rToMonitor <= 8'h00;     //to internal monitor
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

   
   //Debug Monitor for Fpga PostCodes
   DbgPostCodes u2 (
                    .probe(rToMonitor)
                    );
   


   
endmodule // postcode_fub
