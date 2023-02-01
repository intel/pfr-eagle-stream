/* ==========================================================================================================
 = Project     :  Archer City Power Sequencing
 = Title       :  DDR5 PWRGD_FAIL
 = File        :  DDR5_pwrgd_logic.v
 ==========================================================================================================
 = Author      :  Pedro Saldana
 = Division    :  BD GDC
 = E-mail      :  pedro.a.saldana.zepeda@intel.com
 ==========================================================================================================
 = Updated by  :
 = Division    :
 = E-mail      :
 ==========================================================================================================
 = Description :  This module controls the behavior of PWRGD and FAIL for every DDR5 Memory controller
 
 
 = Constraints :
 
 = Notes       :
 
 ==========================================================================================================
 = Revisions   :
 July 15, 2019;    0.1; Initial release
 ========================================================================================================== */
module ddr5_pwrgd_logic
  (
   input      iClk, //Clock input.
   input      iRst_n, //Reset enable on high.
   input      iFM_INTR_PRSNT, //Detect Interposer
   input      iINTR_SKU, //Detect Interposer SKU (1 = CLX or 0 = BRS)

   input      iSlpS5Id, //Sleep S5 indication Signal (active high)
   input      iCpuPwrGd, //CPUPWRGD from CPU_SEQ. This is the output to the CPUs and it is used to know when to deassert DRAMPWROK
   
   input      iAcRemoval, //indication that AcRemoval happened
   
   input      iDimmPwrOK, //PWRGD with generate by Power Supply.
   input      iAdrSel, //selector for ddr5 logic to know if drive PWRGDFAIL & DRAM_RST from internal or ADR logic
   input      iAdrEvent, //this signals when an ADR flow was launched. Is used here to mask the PWRGDFAIL de-assertion when it happens to avoid false error flag
   input      iAdrDimmRst_n, //DIMM_RST_N signal from ADR logic
   input      iMemPwrGdFail, //in case any of the DRAMPWRGD_DDRIO VRs fail...

   //alarios 07-March-2022 : No need to differentiate ADR from non-ADR during pwr-dwn 
   //input      iAdrComplete,   //alarios Jun-28-21: for the CPS workaround of asserting DIMM Rst first on either SLPS5 indication or AdrComplete
   
   input      iPWRGD_DRAMPWRGD_DDRIO, //DRAMPWRGD for valid DIMM FAIL detection
   input      iMC_RST_N, //Reset from memory controller
   input      iADR_PWRGDFAIL, //ADR controls pwrgd/fail output
   inout      ioPWRGD_FAIL_CH_DIMM_CPU, //PWRGD_FAIL bidirectional signal
   
   output reg oDIMM_MEM_FLT, //MEM Fault
   output     oPWRGD_DRAMPWRGD_OK, //DRAM PWR OK
   output     oFPGA_DIMM_RST_N           //Reset to DIMMs

   //output     reg oContinuePwrDwn            //alarios Jun-28-21 : required for the CPS workaround, asserted when it is OK to continue PWRDWN for CPU sequence
   );
   
   //////////////////////////////////////////////////////////////////////////////////
    // States for root state machine
   //////////////////////////////////////////////////////////////////////////////////
    
   localparam ST_IDLE              = 3'd0;
   localparam ST_PSU               = 3'd1;
   localparam ST_DDRIO             = 3'd2;
   localparam ST_LINK              = 3'd3;
   localparam ST_PWRDWN            = 3'd4;
   //localparam ST_PWRDWN_ADR        = 3'd5;
   //localparam ST_FAULT             = 3'd6;
   localparam ST_FAULT             = 3'd5;
   
   //////////////////////////////////////////////////////////////////////////////////
   // Parameters
   //////////////////////////////////////////////////////////////////////////////////
   
   localparam  LOW  = 1'b0;
   localparam  HIGH = 1'b1;

   //////////////////////////////////////////////////////////////////////////////////
   // Internal registers & wires
   //////////////////////////////////////////////////////////////////////////////////
   
   
   reg [2:0]  root_state;                        //main sequencing state machine
   reg        rPWRGD_FAIL_CH_DIMM_CPU_EN;
   reg        rPWRGD_DRAMPWRGD_OK;
   reg        rDimmRstFromFSM_n;

   reg        rPwrGdFailFlag;                   //to flag a DIMM failure

   wire       wPwrGdFail;                        //PwrGdFail sychronized input (from pin)              
   wire       wPWRGD_FAIL_AND_ADR_LOGIC;       //PWRGDFAIL from ADR Logic
   wire       wDIMM_RST_N;
   wire       wFPGA_DIMM_RST_N;

   wire       wDoneTimerDimmRst,wDoneTimerDimmRst2;
     

   wire       wPwrGdFailDbg;
   //////////////////////////////////////////////////////////////////////////////////
   // Instances
   //////////////////////////////////////////////////////////////////////////////////

   //600-usec Timer, started when rDimmRstFromFSM_n is dropped (based on a 500ns clock period)
   delay #(.COUNT(1200))
     TimerDimmRst
       (
        .iClk(iClk),
        .iRst(iRst_n),
        .iStart(!rDimmRstFromFSM_n),
        .iClrCnt(1'b0),
        .oDone(wDoneTimerDimmRst)
        );

   
   delay #(.COUNT(50000))
   TimerDimmRst2
     (
      .iClk(iClk),
      .iRst(iRst_n),
      .iStart(!rDimmRstFromFSM_n),
      .iClrCnt(1'b0),
      .oDone(wDoneTimerDimmRst2)
      );
   
   //3-tab sycn with comparator between tabs 1 and 2 (if the same, then tab 3 captures from tab 2, otherwise, stays the same)
   GlitchFilter2 #
     (
      .NUMBER_OF_SIGNALS (1),
      .RST_VALUE(0)
      ) 
   PWRGDFAIL_GF
     (
      .iClk (iClk),
      .iARst_n(iRst_n),
      .iSRst_n(iPWRGD_DRAMPWRGD_DDRIO),
      .iEna(1'b1),
      .iSignal (ioPWRGD_FAIL_CH_DIMM_CPU),
      .oFilteredSignals(wPwrGdFail)
      );
       
   //////////////////////////////////////////////////////////////////////////////////
   // Continuous assignments    /////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////////
   assign ioPWRGD_FAIL_CH_DIMM_CPU  = wPWRGD_FAIL_AND_ADR_LOGIC ? 1'bz : LOW;        //ouput pwrgd fail low if adr logic is low. Set as open drain only in ST_DDRIO or ST_LINK state
   assign wDIMM_RST_N          = (rPWRGD_DRAMPWRGD_OK && ioPWRGD_FAIL_CH_DIMM_CPU && rDimmRstFromFSM_n)? iMC_RST_N : LOW; //dimm out of reset if MC is high AND pwrgd_fail input is high
   
   //Interposer support//////////////////////////////////////////////////////////////
   //CLX uses DDR4 so MEM RST needs to be bypassed
   assign wFPGA_DIMM_RST_N    = (iFM_INTR_PRSNT && iINTR_SKU) ? iMC_RST_N :wDIMM_RST_N;   
   assign oPWRGD_DRAMPWRGD_OK = (iFM_INTR_PRSNT && iINTR_SKU) ? iPWRGD_DRAMPWRGD_DDRIO :rPWRGD_DRAMPWRGD_OK;


   //if iAdrSel asserted, need to drive from ADR logic, otherwise from ddr5 logic
   //assign oFPGA_DIMM_RST_N = iAdrSel ? iAdrDimmRst_n : wFPGA_DIMM_RST_N;
   assign oFPGA_DIMM_RST_N = wFPGA_DIMM_RST_N;   //alarios 31-Mar-2021 : Workaround to support CPS during a direct-to-S5 flow (where the CPS doesn't get the S5 message)
   
   //assign wPWRGD_FAIL_AND_ADR_LOGIC = iAdrSel ? iADR_PWRGDFAIL : rPWRGD_FAIL_CH_DIMM_CPU_EN;


   
   
   //alarios: for debug, adding an AND with an In-system-sources-and-probes signal to drive it low when desired
   //DbgPwrGdFail DbgPwrGdFail
   //  (.source(wPwrGdFailDbg)
      //.source_clk(iClk)
   //   );
   
   assign wPWRGD_FAIL_AND_ADR_LOGIC = rPWRGD_FAIL_CH_DIMM_CPU_EN;
   //assign wPWRGD_FAIL_AND_ADR_LOGIC = rPWRGD_FAIL_CH_DIMM_CPU_EN && wPwrGdFailDbg;
   
   //////////////////////////////////////////////////////////////////////////////////
   //State machine for DDR5 MEM PWRDG FAIL
   always @ (posedge iClk, negedge iRst_n)
     begin
        if(!iRst_n)          //asynchronous, negative-edge reset
          begin
      		 rPWRGD_FAIL_CH_DIMM_CPU_EN <= LOW;      //FPGA will LOW its internal pull-down driver (PWRGD_FAIL[n:0] output pin)
      		 oDIMM_MEM_FLT              <= LOW;      //MEM Fault
      		 rPWRGD_DRAMPWRGD_OK        <= LOW;      //DRAM PWR OK

             rDimmRstFromFSM_n          <= HIGH;     //for power down, new requirement for CPS to drop first the reset, making sure no to drop DRAMPWROK until after ~600 us
             //oContinuePwrDwn            <= HIGH;     //alarios Jun-28-21: When the timer expires, the CPU can continue with CPU pwr dwn (to block turning off PVCCD-HV

             rPwrGdFailFlag             <= 1'b0;     //turned on when PwrGdFail goes unexpectedly low after being high (in LINK st), considered a DIMM failure (PMIC)
             
             root_state                 <= ST_IDLE;  //to give FSM proper reset value
          end // if (!iRst_n)
        else
          begin
             case(root_state)
               ST_IDLE:
                 begin
                    rPwrGdFailFlag             <= 1'b0;
                    rDimmRstFromFSM_n          <= HIGH;
                    
                    rPWRGD_DRAMPWRGD_OK        <= LOW;       //DRAM PWR OK
				    rPWRGD_FAIL_CH_DIMM_CPU_EN <= LOW;       //FPGA will LOW its internal pull-down driver (PWRGD_FAIL[n:0] output pin)
                    if(iDimmPwrOK && !iSlpS5Id)              //Condition to start DDR5 MEM power up sequence
                      begin
                         root_state            <= ST_DDRIO;  //The FSM shall transition to the ST_DDRIO  state when iDimmPwrOK is asserted
                      end
                 end // case: ST_IDLE

               ST_DDRIO:
                 begin
                    rPwrGdFailFlag             <= 1'b0;
                    rPWRGD_FAIL_CH_DIMM_CPU_EN <= HIGH;   //FPGA will float its internal pull-down driver (PWRGD_FAIL[n:0] output pin)
                    //oContinuePwrDwn <= HIGH;              //alarios Jun-28-21: When the timer expires, the CPU can continue with CPU pwr dwn (to block turning off PVCCD-HV
                    if(iSlpS5Id)                          //if power down flag set, skip to power ST_IDLE
                      root_state <= ST_IDLE;
                    else if(iDimmPwrOK && iPWRGD_DRAMPWRGD_DDRIO && wPwrGdFail)
                      begin
                    	 root_state <= ST_LINK;           //go to next state
                      end
                 end // case: ST_DDRIO
               
               ST_LINK:
                 begin
                    rDimmRstFromFSM_n <= HIGH;
                    rPWRGD_DRAMPWRGD_OK  <= HIGH;     //All DIMMs in Memory Controller are up and running
                    //oContinuePwrDwn      <= LOW;      //alarios Jun-28-21: When the timer expires, the CPU can continue with CPU pwr dwn (to block turning off PVCCD-HV 
                    
                    //alarios: 07-March-2022 : For converged flow, we are removing the differentiation for ADR vs non-ADR, we simply wait for S5 id to be asserted to do power down
                    //                         This requires a FW change in CPS to use ADR COMPLETE notification to switch from external clock to internal clock
                    //if(iAdrComplete)                         //if power down is due to ADR flow
                    //  begin
                    //     root_state        <= ST_PWRDWN_ADR;
                    //     rDimmRstFromFSM_n <= LOW;           //this should trigger PLI event on CPS PMIC
                    //     rPWRGD_DRAMPWRGD_OK <= LOW;         //alarios 04-March-22: After meeting with Russ/Juan/Kai, seems this would make CPS to co-exist with Crashlog Trigger CCB
                    //  end
                    /*else*/
                    if (iSlpS5Id)
                      begin
                         root_state        <= ST_PWRDWN;
                         rDimmRstFromFSM_n <= LOW;           //besides the PLI on CPS this also triggers a timer, before deasserting DRAMPWRGD and moving from PWRDWN to IDLE 
                      end
                    else if (iDimmPwrOK && !wPwrGdFail && !iAdrEvent) // if the DIMM PMIC drops unexpectedly...
                      begin                                          
                         rPwrGdFailFlag    <= 1'b1;          //flag the failure and go to PWRDWN for a gracefull shutdown and assert DIMM RST
                         rDimmRstFromFSM_n <= LOW;
                         oDIMM_MEM_FLT     <= HIGH;          //alarios 21-Feb-22 This is to notify earlier to Master Seq, so it does the same to CPU_SEQ to deassert SYSPWROK and PCHPWROK sooner 
                         root_state        <= ST_PWRDWN;
                      end
                    else if (iMemPwrGdFail)                  //if MC VR fails, assert DIMM RST and go to FAULT state
                      begin
                         rDimmRstFromFSM_n <= LOW;
                         root_state <= ST_FAULT;
                      end
                    else
                      begin
                         root_state <= ST_LINK;
                      end
                 end // case: ST_LINK

               ST_PWRDWN:
                 begin
                    rDimmRstFromFSM_n  <= LOW;
                    

                    //alarios 21-02-22: 2 usec afterrst max, drampwrok has to be de-asserted. We are removing 10msec  and 600usec timers as this violates that requirement
                    //if (wDoneTimerDimmRst)    //600 usec timer before deasserting DRAMPWRGD
                     // begin
                         //rPWRGD_DRAMPWRGD_OK <= LOW;
                         //oContinuePwrDwn     <= HIGH;  //alarios Jun-28-21: When the timer expires, the CPU can continue with CPU pwr dwn (to block turning off PVCCD-HV 
                         
                         if (rPwrGdFailFlag)           //if PMIC failure flag, go to FAULT
                           begin
                              root_state          <= ST_FAULT;
                              oDIMM_MEM_FLT       <= HIGH;               // alarios 21-Feb-22 --> review this
                           end
                         else if (!iCpuPwrGd)          //regular pwrdwn, wait for CPUPWRGD to go to IDLE
                           begin
                              rPWRGD_DRAMPWRGD_OK <= LOW;   //happens after CpuPwrgd is low
                              root_state          <= ST_IDLE;
                           end
                      //end // if ((iAcRemoval && wDoneTimerDimmRst) || (!iAcRemoval && wDoneTimerDimmRst2))
                 end // case: ST_PWRDWN

               //alarios 07-March-2022: This state is no longer required as we don't differentiate ADR from non-ADR pwr-dwn flows
               //ST_PWRDWN_ADR:
               //  begin
               //     rDimmRstFromFSM_n <= LOW;
               //     rPWRGD_DRAMPWRGD_OK <= LOW;
               //     if (iSlpS5Id && !iCpuPwrGd)           //wait for SLP_S4/S5 & CPUPWRGD before deasserting DRAMPWROK 
               //       begin
               //          root_state <= ST_IDLE;
               //          //rPWRGD_DRAMPWRGD_OK <= LOW;     //happens after CpuPwrgd is low    : alarios 04-March-22 : No need to have it here since we put it out of the IF
               //       end
               //  end // case: ST_PWRDWN_ADR
               
               ST_FAULT:                              //No way to go out from here other than AC cycle
                 begin
                    rDimmRstFromFSM_n          <= LOW;
                    rPWRGD_FAIL_CH_DIMM_CPU_EN <= LOW;
                    rPWRGD_DRAMPWRGD_OK        <= LOW;
                    //oContinuePwrDwn            <= HIGH;                 //to avoid cpu_seq to hang at PWRDWN state
                    if (iMemPwrGdFail)
                      oDIMM_MEM_FLT              <= LOW;                //if fault condition is caused by iMC VR, don't flag a DIMM fault
                    else
                      oDIMM_MEM_FLT              <= HIGH;               //if fault condition is caused by DIMM, flag DIMM fault
                    //The only way to get out of reset is AC cycle
                 end // case: ST_FAULT
               
               default:
                 begin
                    root_state <= ST_IDLE;
                 end
               
             endcase // case (root_state)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)

endmodule // ddr5_pwrgd_logic

