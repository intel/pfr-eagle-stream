/* ==========================================================================================================
 = Project     :  Archer City Power Sequencing
 = Title       :  CPU Power Sequence State Machine
 = File        :  ac_pu_seq.v
 ==========================================================================================================
 = Author      :  Pedro Saldana
 = Division    :  BD GDC
 = E-mail      :  pedro.a.saldana.zepeda@intel.com
 ==========================================================================================================
 = Updated by  :  
 = Division    :  
 = E-mail      :  
 ==========================================================================================================
 = Description :  This module will enable CPU and memory VRs for Safari Rapid, Granary Rapid, Cascade Lake as well as Breton sound CPUs (SPR, GNR, CLX and BRS). 
 
 = Constraints :  
 
 = Notes       :  
 
 ==========================================================================================================
 = Revisions   :  
 July 15, 2019;    0.1; Initial release
 ========================================================================================================== */
module ac_cpu_seq
  (
   // ------------------------
   // Clock and Reset signals
   // ------------------------
   
   input        iClk, //clock for sequential logic 
   input        iRst_n, //reset signal, resets state machine to initial state
   
   // ----------------------------
   // inputs and outputs
   // ----------------------------
   input        iCPUPWRGD,    //this is PCH CPUPWRGD required to be asserted before enabling VPP_HBM 
   input        iCPU_PWR_EN, //Cpu power sequence enable
   input        iENABLE_TIME_OUT, //Enable the Timeout
   input        iENABLE_HBM, //Enable HBM VR
   input        iFM_INTR_PRSNT, //Detect Interposer
   input        iFM_CPU_SKTOCC_N, 
   input        iINTR_SKU, //Detect Interposer SKU (1 = CLX or 0 = BRS)
   input        iPWRGD_PVPP_HBM_CPU, //CPU VR PWRGD PVPP_HBM
   input        iPWRGD_PVCCFA_EHV_CPU, //CPU VR PWRGD PVCCFA_EHV
   input        iPWRGD_PVCCFA_EHV_FIVRA_CPU, //CPU VR PWRGD PVCCFA_EHV_FIVRA
   input        iPWRGD_PVCCINFAON_CPU, //CPU VR PWRGD PVCCINFAON
   input        iPWRGD_PVNN_MAIN_CPU, //CPU VR PWRGD PVNN
   input        imPWRGD_PVCCD_HV_CPU, //CPU VR MEMORY PWRGD PVCCD_HV
   input        iPWRGD_PVCCIN_CPU, //CPU VR PVCCIN

   //from Master_Seq
   input        iPltAuxPwrGdFlag, //alarios: added for PLT_AUX_PWRGD handling
   input        iDonePsuOnTimer,  //alarios: added as indication that at least 100msec have passed from when PSU is ON
   
   //Inputs/Outputs for inteposer support//////////////////////////////////////////
   input        iPWRGD_VDDQ_MEM_BRS, //SMB_S3M_CPU_SCL
   input        iPWRGD_P1V8_CPU_BRS, //SMB_S3M_CPU_SDA
   output reg   oCPU_PWR_FLT_VDDQ_MEM_BRS, //VR fault on interposer           
   output reg   oCPU_PWR_FLT_P1V8_BRS, //VR fault on interposer
   input        iPWRGD_VTT, //Pwrgd for Interposer VR (VTT rail), this should be removed or treated for when not having CLX interposer
   ////////////////////////////////////////////////////////////////////////////////
   
   //MEMORY DDR5 Inputs/Outputs  //////////////////////////////////////////////////
   input        iMEM_S3_EN, //If set MEM VRs will remain ON
   input        iS5,        //sleep s5 indication

   //alarios 28-Feb-2022: removed, no longer required due to last pwr dwn flow changes
   //input        iContinuePwrDwn,    //alarios Jun-28-21 Added to know when to continue with PWRDWN seq (when it is safe to powerdown VCCD, for CPS WA)

   //alarios 28-Feb-2022: Added these 3 due to latest Arch flow changes on PwrDwn
   input         iDimmRst_n,
   input         iPltRst_n,
   input         iSlpS3_n,
   input         iDramPwrOK,
   input         iFault,
   
   
   output reg   oMEM_PRW_FLT, //Fault Memory VR'
   output reg   oMEM_PWRGD, //PWRGD of MEMORY
   output reg   oMemPwrGdFail, //if any of the Memory VRs fail... (the ones used for MEM_PWRGD)
   ////////////////////////////////////////////////////////////////////////////////

   output reg   oCpuPwrGd,    //now handled here. This goes to CPUs after VPP_HBM is ON (or if not, after PCH sends it asserted)   
   output reg   oFM_PVPP_HBM_CPU_EN, //CPU VR EN PVPP_HBM
   output reg   oFM_PVCCFA_EHV_CPU_EN, //CPU VR EN PVCCFA_EHV
   output reg   oFM_PVCCFA_EHV_FIVRA_CPU_EN, //CPU VR EN PVCCFA_EHV_FIVRA
   output reg   oFM_PVCCINFAON_CPU_EN, //CPU VR EN PVCCINFAON
   output reg   oFM_PVNN_MAIN_CPU_EN, //CPU VR EN PVNN
   output reg   omFM_PVCCD_HV_CPU_EN, //CPU VR EN MEMORY PVCCD_HV
   output reg   oFM_PVCCIN_CPU_EN, //CPU VR EN PVCCIN
   output reg   oPWRGD_PLT_AUX_CPU, //CPU PRWGD PLATFORM AUXILIARY
   
   output reg   oCPU_PWR_DONE, //PWRGD of all CPU VR's     
   output reg   oCPU_PWR_FLT, //Fault CPU VR's
   
   output reg   oCPU_PWR_FLT_HBM_CPU, //Fault  PVPP_HBM
   output reg   oCPU_PWR_FLT_EHV_CPU, //Fault  PVCCFA_EHV
   output reg   oCPU_PWR_FLT_EHV_FIVRA_CPU, //Fault  PVCCFA_EHV_FIVRA
   output reg   oCPU_PWR_FLT_PVCCINFAON_CPU, //Fault  PVCCINFAON
   output reg   oCPU_PWR_FLT_PVNN, //Fault  PVNN
   output reg   oCPU_PWR_FLT_PVCCIN, //Fault  PVCCIN
   output reg   omFM_PWR_FLT_PVCCD_HV, //Fault  PVCCD_HV

   output reg   oPchPwrOK,                //alarios 15-Feb-21 due to changes in power seq, we now handle these here
   output       oSysPwrOK,
   output reg   oCpuRst_n,                //alarios 28-Feb-22, more changes in power seq, we now handle this here
   
   output [3:0] oDBG_CPU_ST               //State of CPU Secuence
   
   );
   //////////////////////////////////////////////////////////////////////////////////
    // Parameters SPR CPU STATE MACHINE
   //////////////////////////////////////////////////////////////////////////////////
   localparam  ST_INIT                    = 5'd0;
   localparam  ST_PVCCFA_EHV              = 5'd1;
   localparam  ST_PVCCINFAON              = 5'd2;
   localparam  ST_PVNN                    = 5'd3; 
   localparam  ST_PVCCD_HV                = 5'd4;
   localparam  ST_PVCCIN                  = 5'd5;

   localparam  ST_PCHPWROK                = 5'd6;
   
   localparam  ST_PVPP_HBM                = 5'd7;

   localparam  ST_CPUDONE                 = 5'd8;
   
   localparam  ST_PVPP_HBM_OFF            = 5'd9;
   localparam  ST_PVCCIN_OFF              = 5'd10;
   localparam  ST_PVNN_OFF                = 5'd11;
   localparam  ST_PVCCINFAON_OFF          = 5'd12;
   localparam  ST_PVCCFA_EHV_OFF          = 5'd13;
   localparam  ST_PVCCD_HV_OFF            = 5'd14;

   localparam  ST_PWRDWN_FAIL             = 5'd15;
   localparam  ST_PWR_DOWN                = 5'd16;
   //////////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////////
   //Parameters CLX CPU STATE MACHINE
   //////////////////////////////////////////////////////////////////////////////////
   localparam  ST_PVPP_MEM_ABC            = 5'd1;
   localparam  ST_PVDDQ_VTT_MEM_ABC       = 5'd2;
   localparam  ST_PVTT_MEMABC             = 5'd3;
   localparam  ST_PVCCIO                  = 5'd4;
   localparam  ST_PVCCSA                  = 5'd5;
   localparam  ST_PVPP_MEM_ABC_OFF        = 5'd7;
   localparam  ST_PVDDQ_VTT_MEM_ABC_OFF   = 5'd8;
   localparam  ST_PVTT_MEMABC_OFF         = 5'd9;
   localparam  ST_PVCCIO_OFF              = 5'd10;
   localparam  ST_PVCCSA_OFF              = 5'd11;
   //////////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////////
   // Parameters BRS CPU STATE MACHINE
   //////////////////////////////////////////////////////////////////////////////////;
   localparam  ST_PVDDQ                   = 5'd2;
   localparam  ST_PV1V8                   = 5'd3;
   localparam  ST_PVCCANA                 = 5'd1;
   localparam  ST_PVDDQ_OFF               = 5'd7;
   localparam  ST_PV1V8_OFF               = 5'd8;
   localparam  ST_PVCCANA_OFF             = 5'd9;
   ////////////////////////////////////////////////////////////////////////////////
   localparam  LOW =1'b0;
   localparam  HIGH=1'b1;
   localparam  T_500uS_2M  =  10'd1000;
   localparam  T_2mS_2M    =  12'd4000;
   //////////////////////////////////////////////////////////////////////////////////
   
   reg [4:0]    root_state;                            //main sequencing state machine
   reg [3:0]    rPREVIOUS_STATE = ST_INIT;             //previous sequencing state machin   
   reg          rCPU_FLT;
   
   reg          rPWRGD_PVPP_HBM_CPU_FF1;
   reg          rPWRGD_PVCCFA_EHV_CPU_FF1;
   reg          rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1;
   reg          rPWRGD_PVCCINFAON_CPU_FF1;
   reg          rPWRGD_PVNN_MAIN_CPU_FF1;
   reg          rPWRGD_PVCCIN_CPU_FF1;
   reg          rPWRGD_PVCCD_HV_MEM_FF1;
   //BRS Interposer
   reg			rPWRGD_VDDQ_MEM_BRS_FF1;
   reg			rPWRGD_P1V8_CPU_BRS_FF1;

   reg          rSysPwrOKEna;              //alarios 15-feb-21: basically it is used to de-assert SysPwrOk before PchPwrOk. In sequence, PCHPWROK is asserted first and it causes SysPwrOk to be asserted some 
                                           //                   time later. On the power-down sequence, SysPwrOk needs to be de-asserted first. This signal helps to achieve it due to the delay module we use 
                                           //                   to insert the required time between PchPwrOk assertion and SysPwrOk. SysPwrOk gets de-asserted as soon as this goes low and next state PchPwrOk 
                                           //                   will be de-asserted
   
   //////////////////////////////////////////////////////////////////////////////////
   wire         wTIME_OUT_500uS;   //delay of 500uS to turn on the CPU
   wire         wDLY_2mS_EN;     
   wire         wDLY_500uS;
   wire         wDLY_2mS;         //delay of 2mS per power up sequence interposer spec
   wire         wCNT_RST_N;

   //////////////////////////////////////////////////
   wire         wDoneVnn2VccinfaonTimer;               //to wait between VNN is off and before disabling VCCINFAON
   wire         wDoneVccinOffTimer;                    //to wait before disabling VCCIN at power down

   wire         wDoneWarmRstTimer;
   reg          rWarmRst_id;

   ///////////////////////////////////////////////////////////////////////////////////
   // module instances
   ///////////////////////////////////////////////////////////////////////////////////


   //Timer, started when oPwrGdPchPwrOk is asserted, cleared when oPwrGdPchPwrOk or rSysPwrOkEna are de-asserted
   //a delay of 11msec was specified for LBG, EBG however, does not specify a delay (yet)
   //module instantiation is leaved in case we need a delay later on but with a COUNT value of 1 (minimum delay)
   delay #(.COUNT(1)) 
     TimerSyspwrOK(
                   .iClk(iClk),
                   .iRst(iRst_n),
                   .iStart(oPchPwrOK && rSysPwrOKEna),
                   .iClrCnt(1'b0),
                   .oDone(oSysPwrOK)
                   );
   

   //timer for power down to ensure VNN is off 15ms before VCCINFAON starts to power off
   //This is to ensure VNN is less than 200mV before VCCINFAON drops

   //15-msec Timer, started when VNN PWRGDFAIL is dropped in the power down seq. Output is ANDed with CpuMemDone signal (last turned-on VRs) to generate PWRGD_SYS_PWROK & PWRGD_PCH_PWROK
   delay #(.COUNT(30000)) 
     Timer15ms(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(!(iPWRGD_PVNN_MAIN_CPU || oFM_PVNN_MAIN_CPU_EN)),
               .iClrCnt(1'b0),
               .oDone(wDoneVnn2VccinfaonTimer)
               );

   //This timer is for PWRDWN sequence,
   delay #(.COUNT(40))    //20 usec timer
     TimerVccinOff(
                   .iClk(iClk),
                   .iRst(iRst_n),
                   .iStart(!oCpuPwrGd),   //start timer when CpuPwrGd to SKT is de-asserted
                   .iClrCnt(1'b0),
                   .oDone(wDoneVccinOffTimer)
                   );

   //timer to filter if it is a WarmRst or not (vs CPUPWRGD signal comming from the PCH)
   delay #(.COUNT(2000))   //1msec
     TimerWarmRst(
                  .iClk(iClk),
                  .iRst(iRst_n),
                  .iStart(!iPltRst_n && iCPUPWRGD),
                  .iClrCnt(1'b0),
                  .oDone(wDoneWarmRstTimer)
                  );

   //alarios 01-March-2022: this is to distinguish if there is a warm rst flow (where CPU_RST needs to follow PLTRST) or no
   //                       and then CPU_RST needs to wait until DIMM_RST is asserted

   always @ (posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)
          begin
             rWarmRst_id <= LOW;
          end
        else
          begin
             if (wDoneWarmRstTimer && oCpuRst_n)
               begin
                  rWarmRst_id <= HIGH;
               end
             else
               begin
                  rWarmRst_id <= LOW;
               end
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   
           
   
   //////////////////////////////////////////////////////////////////////////////////
   // Continuous assignments    /////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////////
   assign  oDBG_CPU_ST     	= root_state;
   assign  wCNT_RST_N       	= (root_state == rPREVIOUS_STATE);
   assign  wTIME_OUT_500uS	    = wDLY_500uS && wCNT_RST_N && iENABLE_TIME_OUT;
   //state machine for CPU Power-up sequence
   always @ (posedge iClk, negedge iRst_n)
     begin
        if(!iRst_n)                                     //asynchronous, negative-edge reset
          begin             
    		 oFM_PVPP_HBM_CPU_EN          <= LOW;          //CPU VR EN PVPP_HBM
    		 oFM_PVCCFA_EHV_CPU_EN        <= LOW;          //CPU VR EN PVCCFA_EHV
    		 oFM_PVCCFA_EHV_FIVRA_CPU_EN  <= LOW;          //CPU VR EN PVCCFA_EHV_FIVRA
    		 oFM_PVCCINFAON_CPU_EN        <= LOW;          //CPU VR EN PVCCINFAON
    		 oFM_PVNN_MAIN_CPU_EN         <= LOW;          //CPU VR EN PVNN
             omFM_PVCCD_HV_CPU_EN         <= LOW;          //CPU VR EN PVCCD_HV
             oFM_PVCCIN_CPU_EN            <= LOW;          //CPU VR EN PVCCIN
             oPWRGD_PLT_AUX_CPU           <= LOW;          //PLT AUX requiered per PAS
    		 oCPU_PWR_DONE                <= LOW;	       //PWRGD of all CPU VR's      
             
             root_state                   <= ST_INIT;      //reset state machines to initial states

             oPchPwrOK                    <= LOW;          //alarios 15-Feb-21: PCH PWR OK & SYS PWR OK enable
             rSysPwrOKEna                 <= LOW;
             oCpuPwrGd                    <= LOW;          //alarios 15-Feb-12: CPU PWRGD going to the CPUs

             oCpuRst_n                    <= LOW;
             
          end
        else
          begin
             
             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
             ////////////        SPR CPU SEQUENCE          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           
             if(!iFM_INTR_PRSNT && !iFM_CPU_SKTOCC_N)          //verify if CPU interposer is not present
               begin
                  case(root_state)  
                    ST_INIT:                                 //All CPU VRs are OFF
                      begin
                         oFM_PVPP_HBM_CPU_EN          <= LOW;          //CPU VR EN PVPP_HBM
    		             oFM_PVCCFA_EHV_CPU_EN        <= LOW;          //CPU VR EN PVCCFA_EHV
    		             oFM_PVCCFA_EHV_FIVRA_CPU_EN  <= LOW;          //CPU VR EN PVCCFA_EHV_FIVRA
    		             oFM_PVCCINFAON_CPU_EN        <= LOW;          //CPU VR EN PVCCINFAON
    		             oFM_PVNN_MAIN_CPU_EN         <= LOW;          //CPU VR EN PVNN
                         //if (!iMEM_S3_EN)                                                                           //alarios: commented out because for S3 this needs to be HIGH and if conditioned here, S3 signal will be gone before
                         //  omFM_PVCCD_HV_CPU_EN         <= LOW;          //CPU VR EN PVCCD_HV                       //exiting this state, causing an issue
                         //else
                         //  omFM_PVCCD_HV_CPU_EN         <= HIGH;         //CPU VR EN PVCCD_HV ON @ S3

                         if (iS5)                                                              //alarios 27-Oct-2021: added as if in S3, and suddenly want to go to S5, PVCCD needs to be disabled or CPLD would see a VR error when
                           omFM_PVCCD_HV_CPU_EN         <= LOW;          //CPU VR EN PVCCD_HV               //                     moving to S5 (shutting down PSU and keeping enable HIGH) as PWRGD will be dropped
                           
                         oFM_PVCCIN_CPU_EN            <= LOW;          //CPU VR EN PVCCIN
                         oPWRGD_PLT_AUX_CPU           <= LOW;          //PLT AUX requiered per PAS
    		             oCPU_PWR_DONE                <= LOW;	       //PWRGD of all CPU VR's      
                         
                         oPchPwrOK                    <= LOW;          //alarios 15-Feb-21: PCH PWR OK & SYS PWR OK enable
                         rSysPwrOKEna                 <= LOW;
                         
                         oCpuPwrGd                    <= LOW;

                         oCpuRst_n                    <= LOW;

                         if(iCPU_PWR_EN)                        //if CPU sequencer is enabled by Master seq
                           begin
                              root_state <= ST_PVCCD_HV;        //go to 1st VR state to be powered-on
                           end
                      end // case: ST_INIT

                    ST_PVCCD_HV: 
                      begin    
                         omFM_PVCCD_HV_CPU_EN         <= HIGH;         //assert CPU PVCCD_HV enable
                         oCpuPwrGd                    <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)                   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              root_state <= ST_PWRDWN_FAIL;
	                       end 
  	                     else if (imPWRGD_PVCCD_HV_CPU && iCPU_PWR_EN) begin                 //VR is on and package does not have HBM
  	                   	    root_state <= ST_PVCCFA_EHV;                                     //go to 
	                     end 
                      end
                    
                    ST_PVCCFA_EHV: 
                      begin
                         oFM_PVCCFA_EHV_CPU_EN        <= HIGH;                //CPU VR EN PVCCFA_EHV
					     oFM_PVCCFA_EHV_FIVRA_CPU_EN  <= HIGH;                //CPU VR EN PVCCFA_EHV_FIVRA
                         oCpuPwrGd                    <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              root_state <= ST_PWRDWN_FAIL;
	                       end 
	                     else if (iPWRGD_PVCCFA_EHV_CPU && iPWRGD_PVCCFA_EHV_FIVRA_CPU) begin
	                      	root_state <= ST_PVCCINFAON;                //Go to ST_PVCCINFAON state
	                     end 
                      end
                    
                    ST_PVCCINFAON: 
                      begin
                         oFM_PVCCINFAON_CPU_EN        <= HIGH;               //CPU VR EN PVCCINFAON
                         oCpuPwrGd                    <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)                     //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              root_state <= ST_PWRDWN_FAIL;
	                       end 
	                     else if (iPWRGD_PVCCINFAON_CPU) 
	                       begin
	                          root_state <= ST_PVNN;                       //Go to next state (ST_PVNN)
	                       end 
                      end
                    
                    ST_PVNN:                                            //PVNN PWRGD and Enable goes in parrallel (for both CPUs)
                      begin
					     oFM_PVNN_MAIN_CPU_EN         <= HIGH;
                         oCpuPwrGd                    <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS) //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              root_state <= ST_PWRDWN_FAIL;
	                       end 
	                     else if (iPWRGD_PVNN_MAIN_CPU) begin               //If PVNN
                            root_state <= ST_PVCCIN;                      //Go to next state (ST_VCCIN)
	                     end 
                      end

                    ST_PVCCIN: 
                      begin
                         oFM_PVCCIN_CPU_EN            <= HIGH;                //CPU VR EN PVCCIN
                         oPWRGD_PLT_AUX_CPU           <= HIGH;                //alarios: moved here since PVCCD_HV is now the 1st in the sequence and PWRGD_PLT_AUX should be asserted after PVNN is ON
                         oCpuPwrGd                    <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              root_state <= ST_PWRDWN_FAIL;
	                       end

                         //alarios Feb-12_2021: Power Seq has changed. If pkg with HBM, VPP_HBM is last to be ON and only after PCH CPUPWRGD is asserted
	                     //else if (iPWRGD_PVCCIN_CPU) begin
	                     // 	root_state <= ST_CPUDONE;                         //Go to ST_PVCST_CPUDONE state

                         //else if (iPWRGD_PVCCIN_CPU && iENABLE_HBM && iCPUPWRGD)
                         else if (iPWRGD_PVCCIN_CPU && iDonePsuOnTimer)
                           begin
                              root_state <= ST_PCHPWROK;
                           end
	                  end // case: ST_PVCCIN

                    ST_PCHPWROK:
                      begin
                         oPchPwrOK    <= 1'b1;
                         rSysPwrOKEna <= 1'b1;                                //alarios 15-Feb-21: together with oPchPwrOK, enables oSysPwrOK to be asserted after timer
                         oCpuPwrGd    <= LOW;
                         if (!iCPU_PWR_EN || rCPU_FLT)
                           begin
                              rSysPwrOKEna <= LOW;              //let PCH know about failure by de-asserting SYS_PWROK
                              root_state <= ST_PWRDWN_FAIL;
                           end
                         else if (iCPUPWRGD && iENABLE_HBM)
                           root_state <= ST_PVPP_HBM;                         //alarios 15-Feb-21: if HBM PKG, go turn VPP HBM on
                         else if (iCPUPWRGD && !iENABLE_HBM)
                           root_state <= ST_CPUDONE;                          //alarios 15-Feb-21: if no HBM PKG, pwr up seq is done
                      end
                    

                    ST_PVPP_HBM:
                      begin
                         oFM_PVPP_HBM_CPU_EN <= HIGH;                         //CPU VR EN PVPP_HBM 
                         oCpuPwrGd           <= LOW;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
                              rSysPwrOKEna <= LOW;              //let PCH know about failure by de-asserting SYS_PWROK
                              root_state <= ST_PWRDWN_FAIL;
	                       end 
	                     else if (iPWRGD_PVPP_HBM_CPU) begin
	                      	root_state <= ST_CPUDONE;                         //PwrSeq is done
	                     end 
                      end
                    
                    
                    ST_CPUDONE: 
                      begin                         
                         oCPU_PWR_DONE   <= HIGH;	                          //PWRGD of all CPU VR's
                         if (iCPUPWRGD == HIGH)                               //alarios 30-Sep-2021: if CPUPWRGD from PCH is asserted, assert it to SKTs, if no, keep previous val. So at PWR-DWN, Skts will see it de-asserted only after 600usec timer expires
                           oCpuPwrGd       <= HIGH;                           //alarios 15-Feb-21: Now CPU PWRGD is handled here as if PKG with HBM, CPU PWRGD to CPUs must be asserted only after VPP HBM VR is ON
                         else
                           oCpuPwrGd       <= oCpuPwrGd;

                         if (iFault || rCPU_FLT)                 //if fault from other or this sequencer
                           begin
                              rSysPwrOKEna <= LOW;              //let PCH know about failure by de-asserting SYS_PWROK
                              root_state <= ST_PWRDWN_FAIL;
                           end
                         else if (!iCPU_PWR_EN && (!iDimmRst_n || iMEM_S3_EN))    //no failure, but pwr down, wait for DimmRst or if we go to S3
                           begin
                              oCpuRst_n   <= LOW;  //asserting CPU RST after DIMM_RST is asserted, or if going to S3
                              root_state <= ST_PWR_DOWN;
                           end
                         else if (iPltRst_n || rWarmRst_id)
                           oCpuRst_n <= iPltRst_n; //this condition intends to make CpuRst to follow PLTRST during power on, while if in pwrdwn
                                                   //this assignment won't happen and will happen the above, if normal pwr-dwn (not due to a failure)
                         
                           
                      end // case: ST_CPUDONE


                    ST_PWRDWN_FAIL:
                      begin
                         rSysPwrOKEna <= LOW;
                         oPchPwrOK <= oSysPwrOK;      //de-assert PCH PWROK right after SYS_PWROK
                         if(!iCPU_PWR_EN && !iDimmRst_n)    //not going to S3 for sure, so wait for DimmRst to be asserted LOW
                           begin
                              oCpuRst_n  <= LOW;   //after DimmRst goes low
                              root_state <= ST_PWR_DOWN;
                           end
                      end
                    

                    ST_PWR_DOWN: 
                      begin
                         oCpuPwrGd <= LOW;         //de-asserted as we arrive to this state (after oCpuRst_n)
                         oCPU_PWR_DONE   <= HIGH; //this is to avoid a dead-lock in master seq
                         rSysPwrOKEna <= LOW;     //sequence for a normal or strait to S5 flow, if from Failure, deasserted before
                         oPchPwrOK <= oSysPwrOK;   //if SYS_PWR_OK is pulled LOW, next cycle, PCH_PWROK will be LOW too
                         if (iENABLE_HBM && wDoneVccinOffTimer && (!iDramPwrOK || iMEM_S3_EN))  //dropping CPUPWRGD launches 20 usec timer before PVPP should be disabled (requirement is 11usec, we use same timer as VCCIN req)
                           root_state <= ST_PVPP_HBM_OFF;
                         else if (wDoneVccinOffTimer && (!iDramPwrOK || iMEM_S3_EN) )           //dropping CPUPWRGD launches 20 usec timer before PVCCIN should be disabled
                           root_state <= ST_PVCCIN_OFF;
                         else
                           root_state <= ST_PWR_DOWN;
                      end // case: ST_PWR_DOWN


                    //Add a 20usec between ST_PWR_DOWN and before PVCCIN_OFF or VPP_OFF

                    ST_PVPP_HBM_OFF: 
                      begin
                         oPchPwrOK <= LOW;                      //sequence for a normal or strait to S5 flow, if from Failure, deasserted before
                         oFM_PVPP_HBM_CPU_EN <= LOW;            //deassert enable to power down 
                         
                         if(!iPWRGD_PVPP_HBM_CPU)               //wait for PVPP _HBM VR to go off
	                       begin
                              root_state <= ST_PVCCIN_OFF;      //go to next pwr-dwn state
	                       end
                      end
                    
                    ST_PVCCIN_OFF:
                      begin
                         oPchPwrOK <= LOW;
                         oFM_PVCCIN_CPU_EN            <= LOW;                 //deassert enable to power down after timer expires

                         //alarios: we only de-assert PWRGD_PLT_AUX_CPU if the flag is HIGH (meaining we are going down do to a GLB RST)
                         if (iPltAuxPwrGdFlag) begin
                           oPWRGD_PLT_AUX_CPU           <= LOW;
                         end
                         
                         if(!iPWRGD_PVCCIN_CPU)                               //if current stage is off
	                       begin
                              root_state <= ST_PVNN_OFF;

                              //alarios: commented out from here, as PVCCD_HV is in a different sequence now (last VR to be OFF)
	                          //if(iMEM_S3_EN)                                       //S3 support
	                          //  begin
	                    	  //     root_state <= ST_PVNN_OFF;                   //go to ST_PVNN_OFF and dont turn off memory
	                          //  end
	                          //else begin
	                          //   //root_state <= ST_PVCCD_HV_OFF;               //now go to next power down state
	                          //end     
	                       end
                      end
                   
                    ST_PVNN_OFF: 
                      begin
                         oFM_PVNN_MAIN_CPU_EN         <= LOW;                    //deassert enable to power down
                         
                         //if(!iPWRGD_PVNN_MAIN_CPU)                               //if current stage is off
                         if(wDoneVnn2VccinfaonTimer)                               //modifed as we need to wait 15ms before moving to next state
	                       begin
	                          root_state <= ST_PVCCINFAON_OFF;               //now go to next power down state 
	                       end
                      end
                    
                    ST_PVCCINFAON_OFF: 
                      begin
                         oFM_PVCCINFAON_CPU_EN        <= LOW;                     //deassert enable to power down 
                         
                         if(!iPWRGD_PVCCINFAON_CPU)                               //if current stage is off
                           
	                       begin
	                          root_state <= ST_PVCCFA_EHV_OFF;                //now go to next power down state
	                       end
                      end
                    
                    ST_PVCCFA_EHV_OFF: 
                      begin
                         oFM_PVCCFA_EHV_CPU_EN         <= LOW;          
					     oFM_PVCCFA_EHV_FIVRA_CPU_EN  <= LOW;                       //deassert enable to power down 
                         
                         if (!iPWRGD_PVCCFA_EHV_CPU && !iPWRGD_PVCCFA_EHV_FIVRA_CPU)  //else if HBM is not present in package, and VRs are OFF 
                           begin
                              root_state <= ST_PVCCD_HV_OFF;                                               //go to power off PVCCD_HV VR
                           end
                      end
                    
                    ST_PVCCD_HV_OFF:                                                //alarios: changed sequence per Architecture request. PVCCD_HV is the last to turn off
                      begin
                         if (iS5/*iMEM_S3_EN*/)                                           //iMEM_S3_EN == HIGH means PVCCD_HV needs to remain ON
                           begin
                              omFM_PVCCD_HV_CPU_EN         <= LOW;                  //deassert enable to power down the VR
                              if(!imPWRGD_PVCCD_HV_CPU)                             //if VR already power down
	                            begin
	                               root_state <= ST_INIT;                           //now go back to INIT ST (waiting for powering up again)
	                            end
                           end
                         else
                           begin
                              root_state <= ST_INIT;                                //PVCCD_HV needs to remain powered, go directly to ST_INIT
                           end
                      end // case: ST_PVCCD_HV_OFF
                    
                    default: 
                      begin
                         root_state <= ST_INIT;
                      end
                  endcase
                  rPREVIOUS_STATE <= root_state;    
               end                                                                 //End of SPR CPU state machine
             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
             ////////////        CLX CPU SEQUENCE          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           
             else if(iFM_INTR_PRSNT && iINTR_SKU && !iFM_CPU_SKTOCC_N)  //CLX State Machine verify if CPU interposer is present
               begin
                  case(root_state)  
                    ST_INIT:                                //All CPU VRs are OFF
                      begin
                         oCPU_PWR_DONE                <= LOW;     //PWRGD DOWN
                         oPchPwrOK <= LOW;
	                     
                         if(iCPU_PWR_EN) begin
	                      	root_state <= ST_PVPP_MEM_ABC;    //go to ST_PVPP_HBM state
	                     end 
                      end
                    
                    ST_PVPP_MEM_ABC:
                      begin
                         oFM_PVCCFA_EHV_FIVRA_CPU_EN   <= HIGH;
                         //oFM_PVPP_HBM_CPU_EN          <= HIGH;             //alarios: CPU VR EN PVPP_HBM (turning on VPP to test some stuff, do not commit)
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          root_state <= ST_PVCCSA_OFF;                     //Power Down
	                       end 
	                     else if (iPWRGD_PVCCFA_EHV_FIVRA_CPU && wDLY_2mS) begin
	                      	root_state <= ST_PVDDQ_VTT_MEM_ABC;           
	                     end 
                      end
                    
                    ST_PVDDQ_VTT_MEM_ABC: 
                      begin
                         omFM_PVCCD_HV_CPU_EN        <= HIGH;        
                         oPWRGD_PLT_AUX_CPU          <= HIGH;                 //Turns on VTT VR on interposer                 
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          root_state <= ST_PVCCSA_OFF;                    //Power Down
	                       end 
	                     else if (imPWRGD_PVCCD_HV_CPU && iPWRGD_VTT) begin
	                      	root_state <= ST_PVCCIO;               
                         end
                      end
                    
                    ST_PVCCIO:                                            
                      begin
          				 oFM_PVCCINFAON_CPU_EN       <= HIGH;
                         oFM_PVNN_MAIN_CPU_EN        <= HIGH;                //VPU board side band needs this power to work
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          root_state <= ST_PVCCSA_OFF;                   //Power Down
	                       end 
	                     else if (iPWRGD_PVCCINFAON_CPU) begin               
	                      	root_state <= ST_PVCCIN;                
	                     end 
                      end
                    
                    ST_PVCCIN: 
                      begin
                         oFM_PVCCIN_CPU_EN            <= HIGH;               //CPU VR EN PVCCIN
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          root_state <= ST_PVCCSA_OFF;                   //Power Down
	                       end 
	                     else if (iPWRGD_PVCCIN_CPU) begin
	                      	root_state <= ST_PVCCSA;                   
	                     end 
                      end
                    
                    ST_PVCCSA: 
                      begin
                         oFM_PVCCFA_EHV_CPU_EN        <= HIGH;              
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          root_state <= ST_PVCCSA_OFF;                      //Power Down
	                       end 
	                     else if (iPWRGD_PVCCFA_EHV_CPU) begin
	                        root_state <= ST_CPUDONE;                        
	                     end 
                      end
                    
                    ST_CPUDONE: 
                      begin
                         oCPU_PWR_DONE   			 <= HIGH;	            //PWRGD of all CPU VR's
                         oPchPwrOK <= HIGH;
                         
                         if((!iCPU_PWR_EN) || rCPU_FLT)                      //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                       begin
	                          //root_state <= ST_PWR_DOWN;                   //Power Down
                              root_state <= ST_PVCCSA_OFF;                   //alarios: changed as we don't need an extra Power Down state
                              rSysPwrOKEna <= LOW;
	                       end
                         else
                           rSysPwrOKEna <= HIGH;
                      end
                    
                    //ST_PWR_DOWN: 
                    //  begin
                         // if (rCPU_FLT && iCPU_PWR_EN)                        //If fail but not power down indication then go to fault state                    
                         //  begin
                    //     root_state <= ST_PVCCSA_OFF;                //Start power down sequence
                         //end
                         //else if ((!rCPU_FLT) && (!iCPU_PWR_EN))             //If not fail but  power down indication then go to power down sequence                    
                         //  begin
                         //     root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                         //end
                    //  end
                    
                    ST_PVCCSA_OFF: 
                      begin
                         oPchPwrOK <= LOW;
                         oFM_PVCCFA_EHV_CPU_EN       <= LOW;              //deassert enable to power down 
                         
                         if(!iPWRGD_PVCCFA_EHV_CPU)                       //if current stage is off
	                       begin
	                          root_state <=   ST_PVCCIN_OFF;            //now go to next power down state
	                       end
                      end
                    
                    ST_PVCCIN_OFF: 
                      begin
                         oFM_PVCCIN_CPU_EN           <= LOW;          
                         
                         if(!iPWRGD_PVCCIN_CPU)                           //if current stage is off
	                       begin
	                          root_state <= ST_PVCCIO_OFF;                //now go to state of waiting for power-up
	                       end
                      end
                    
                    ST_PVCCIO_OFF: 
                      begin
                         oFM_PVCCINFAON_CPU_EN        <= LOW;               //deassert enable to power down
                         oFM_PVNN_MAIN_CPU_EN         <= LOW;               //CPU board side band needs this power to work
                         
                         if(!iPWRGD_PVCCINFAON_CPU) begin
					        if(iMEM_S3_EN && LOW)                           //S3 is not supported due to CLX Inteposer issues, VPP rail is on P12V main rail 
	                          begin
	                    	     root_state <= ST_INIT;                      //go to ST_INIT and dont turn off memory
	                          end
	                        else begin
	                           root_state <= ST_PVDDQ_VTT_MEM_ABC_OFF;   //if not go to next power down state
	                        end     
                         end       
                      end
                    
                    ST_PVDDQ_VTT_MEM_ABC_OFF: 
                      begin
                         omFM_PVCCD_HV_CPU_EN        <= LOW;        
                         oPWRGD_PLT_AUX_CPU          <= LOW;                //Turns off VTT VR on interposer 
                         //VTT PWRGD signal is only available for CPU0, it is hardcode for turn off, 
                         //CPU1 is hardcode to turn on (main file writes "1") and turn off
                         if(!imPWRGD_PVCCD_HV_CPU) 
	                       begin
	                          root_state <= ST_PVPP_MEM_ABC_OFF;          //now go to next power down state
	                       end
                      end
                    
                    ST_PVPP_MEM_ABC_OFF:
                      begin
                         oFM_PVCCFA_EHV_FIVRA_CPU_EN   <= LOW;                 
                         
                         if(!iPWRGD_PVCCFA_EHV_FIVRA_CPU)                               //if current stage is off 
	                       begin
	                    	  root_state <= ST_INIT;      
	                       end
                      end
                    
                    default: 
                      begin
                         root_state <= ST_INIT;
                      end
                  endcase
                  rPREVIOUS_STATE <= root_state;    
               end                                                 //End of CLX CPU state machine
             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
             ////////////        BRS CPU SEQUENCE          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           
             else if(iFM_INTR_PRSNT && !iINTR_SKU && !iFM_CPU_SKTOCC_N)    ////BRS State Machine verify if CPU interposer is present
             begin
                case(root_state)  
                  ST_INIT:                                 //All CPU VRs are OFF
                    begin
                       oCPU_PWR_DONE               <= LOW;     //PWRGD DOWN
                       oPchPwrOK <= LOW;
                       
	                   if(iCPU_PWR_EN) begin
	                      root_state <= ST_PVDDQ;         //go to ST_VDDQ
	                   end 
                    end
                  
                  ST_PVDDQ:
                    begin
                       omFM_PVCCD_HV_CPU_EN        <= HIGH;             
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)  //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                   //Power Down
	                     end 
	                   else if (iPWRGD_VDDQ_MEM_BRS && wDLY_2mS) begin
	                      root_state <= ST_PVCCIO;           
	                   end 
                    end
                  
                  ST_PVCCIO: 
                    begin
          			   oFM_PVCCINFAON_CPU_EN       <= HIGH;
                       oFM_PVNN_MAIN_CPU_EN        <= HIGH;                  //VPU board side band needs this power to work              
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                      //Power Down
	                     end 
	                   else if (iPWRGD_PVCCINFAON_CPU) begin
	                      root_state <= ST_PV1V8;               
                       end
                    end
                  
                  ST_PV1V8:                                            
                    begin
					   oPWRGD_PLT_AUX_CPU       <= HIGH;
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                   //Power Down
	                     end 
	                   else if (iPWRGD_P1V8_CPU_BRS) begin               
	                      root_state <= ST_PVCCANA;                
	                   end 
                    end
                  
                  ST_PVCCANA:                                            
                    begin
					   oFM_PVCCFA_EHV_FIVRA_CPU_EN       <= HIGH;
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                   //Power Down
	                     end 
	                   else if (iPWRGD_PVCCFA_EHV_FIVRA_CPU) begin               
	                      root_state <= ST_PVCCIN;                
	                   end 
                    end
                  
                  ST_PVCCIN: 
                    begin
                       oFM_PVCCIN_CPU_EN            <= HIGH;               //CPU VR EN PVCCIN
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                   //Power Down
	                     end 
	                   else if (iPWRGD_PVCCIN_CPU) begin
	                      root_state <= ST_PVCCSA;                   
	                   end 
                    end
                  
                  ST_PVCCSA: 
                    begin
                       oFM_PVCCFA_EHV_CPU_EN        <= HIGH;              
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                     begin
	                        root_state <= ST_PVCCSA_OFF;                      //Power Down
	                     end 
	                   else if (iPWRGD_PVCCFA_EHV_CPU) begin
	                      root_state <= ST_CPUDONE;                        
	                   end 
                    end
                  
                  ST_CPUDONE: 
                    begin
                       oCPU_PWR_DONE				 <= HIGH;	            //PWRGD of all CPU VR's
                       oPchPwrOK <= HIGH;
                       
                       if((!iCPU_PWR_EN) || rCPU_FLT)                      //if PWR DOWN or PWR FAULT
	                     begin
	                        //root_state <= ST_PWR_DOWN;                   //Power Down
                            root_state <= ST_PVCCSA_OFF;                   //alarios: changed as we don't need an extra Power Down state
                            rSysPwrOKEna <= LOW;
	                     end
                       else
                         rSysPwrOKEna <= HIGH;
                    end
                  
                  //ST_PWR_DOWN: 
                  //  begin
                  //     if (rCPU_FLT && iCPU_PWR_EN)                        //If fail but not power down indication then go to power down                   
                  //      begin
                  //          root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                  //       end
                  //     else if ((!rCPU_FLT) && (!iCPU_PWR_EN))             //If not fail but  power down indication then go to power down sequence                    
                  //       begin
                  //          root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                  //       end
                  //  end
                  
                  ST_PVCCSA_OFF: 
                    begin
                       oFM_PVCCFA_EHV_CPU_EN        <= LOW;                //deassert enable to power down 
                       oPchPwrOK <= LOW;
                       
                       if(!iPWRGD_PVCCFA_EHV_CPU)                          //if current stage is off
	                     begin
	                        root_state <= ST_PVCCIN_OFF;                 //now go to next power down state
	                     end
                    end
                  
                  ST_PVCCIN_OFF: 
                    begin
                       oFM_PVCCIN_CPU_EN            <= LOW;          
                       
                       if(!iPWRGD_PVCCIN_CPU)                               //if current stage is off
	                     begin
	                        root_state <= ST_PVCCANA_OFF;                //now go to state of waiting for power-up
	                     end
                    end
                  
                  ST_PVCCANA_OFF:
                    begin
                       oFM_PVCCFA_EHV_FIVRA_CPU_EN   <= LOW;                 
                       
                       if(!iPWRGD_PVCCFA_EHV_FIVRA_CPU)                    //if current stage is off 
	                     begin
	                    	root_state <= ST_PV1V8_OFF;      
	                     end
                    end
                  
                  ST_PV1V8_OFF:                                            
                    begin
					   oPWRGD_PLT_AUX_CPU           <= LOW;
                       
  	                   if (!iPWRGD_P1V8_CPU_BRS) begin               
  	                      root_state <= ST_PVCCIO_OFF;                
  	                   end 
                    end
                  
                  ST_PVCCIO_OFF: 
                    begin
          			   oFM_PVCCINFAON_CPU_EN       <= LOW;
                       oFM_PVNN_MAIN_CPU_EN        <= LOW;                 //CPU board side band needs this power to work
                       
                       if(!iPWRGD_PVCCINFAON_CPU) begin
					      if(iMEM_S3_EN && LOW)                                  //S3 support
    	                    begin
    	                       root_state <= ST_INIT;                     //go to ST_INIT and dont turn off memory
    	                    end
	                      else begin
	                         root_state <= ST_PVDDQ_OFF;                  //if not go to next power down state
	                      end     
                       end       
                    end
                  
                  ST_PVDDQ_OFF: 
                    begin
                       omFM_PVCCD_HV_CPU_EN        <= LOW;        
                       
                       if(!iPWRGD_VDDQ_MEM_BRS)                         //if current stage is off
	                     begin
	                        root_state <= ST_INIT;                  //now go to next power down state
	                     end
                    end
                  
                  default: 
                    begin
                       root_state <= ST_INIT;
                    end
                endcase
                rPREVIOUS_STATE <= root_state;    
             end // if (iFM_INTR_PRSNT && !iINTR_SKU && !iFM_CPU_SKTOCC_N)  //End of CLX CPU state machine
             
             
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)
   
   
   
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //FAULT LOGIC
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   always @( posedge iClk, negedge iRst_n) 
     begin 
	    if (  !iRst_n ) 
		  begin						
			 rPWRGD_PVPP_HBM_CPU_FF1            <= LOW;
			 rPWRGD_PVCCFA_EHV_CPU_FF1          <= LOW;
			 rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1    <= LOW;
			 rPWRGD_PVCCINFAON_CPU_FF1          <= LOW;
			 rPWRGD_PVNN_MAIN_CPU_FF1           <= LOW;
			 rPWRGD_PVCCIN_CPU_FF1              <= LOW;
			 rPWRGD_PVCCD_HV_MEM_FF1            <= LOW;
			 //BRS Interposer
			 rPWRGD_VDDQ_MEM_BRS_FF1            <= LOW;
			 rPWRGD_P1V8_CPU_BRS_FF1            <= LOW;
			 oCPU_PWR_FLT_P1V8_BRS              <= LOW;
			 oCPU_PWR_FLT_VDDQ_MEM_BRS          <= LOW;
             /////////////////////////////////////////
			 omFM_PWR_FLT_PVCCD_HV              <= LOW;
			 oCPU_PWR_FLT_HBM_CPU               <= LOW;
			 oCPU_PWR_FLT_EHV_CPU               <= LOW;
			 oCPU_PWR_FLT_EHV_FIVRA_CPU         <= LOW;
			 oCPU_PWR_FLT_PVCCINFAON_CPU        <= LOW;
			 oCPU_PWR_FLT_PVNN                  <= LOW;
			 oCPU_PWR_FLT_PVCCIN                <= LOW;
			 oCPU_PWR_FLT                       <= LOW;
			 rCPU_FLT		    	                 <= LOW;
			 
		  end
	    else 
		  begin	
			 
			 rPWRGD_PVPP_HBM_CPU_FF1            <= iPWRGD_PVPP_HBM_CPU;
			 rPWRGD_PVCCFA_EHV_CPU_FF1          <= iPWRGD_PVCCFA_EHV_CPU;
			 rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1    <= iPWRGD_PVCCFA_EHV_FIVRA_CPU;
			 rPWRGD_PVCCINFAON_CPU_FF1          <= iPWRGD_PVCCINFAON_CPU;
			 rPWRGD_PVNN_MAIN_CPU_FF1           <= iPWRGD_PVNN_MAIN_CPU;
			 rPWRGD_PVCCIN_CPU_FF1              <= iPWRGD_PVCCIN_CPU;
			 rPWRGD_PVCCD_HV_MEM_FF1            <= imPWRGD_PVCCD_HV_CPU;
             //BRS Interposer
			 rPWRGD_VDDQ_MEM_BRS_FF1            <= iPWRGD_VDDQ_MEM_BRS;
			 rPWRGD_P1V8_CPU_BRS_FF1            <= iPWRGD_P1V8_CPU_BRS;
			 oCPU_PWR_FLT_VDDQ_MEM_BRS          <= (omFM_PVCCD_HV_CPU_EN        && rPWRGD_VDDQ_MEM_BRS_FF1           && !iPWRGD_VDDQ_MEM_BRS)         ?  HIGH: oCPU_PWR_FLT_VDDQ_MEM_BRS;	   //VR fault on interposer           
			 oCPU_PWR_FLT_P1V8_BRS              <= (oPWRGD_PLT_AUX_CPU          && rPWRGD_P1V8_CPU_BRS_FF1           && !iPWRGD_P1V8_CPU_BRS)         ?  HIGH: oCPU_PWR_FLT_P1V8_BRS;		   //VR fault on interposer   
			 //////////////////////////////////////////////////////////
			 omFM_PWR_FLT_PVCCD_HV              <= (omFM_PVCCD_HV_CPU_EN        && rPWRGD_PVCCD_HV_MEM_FF1           && !imPWRGD_PVCCD_HV_CPU         && !rCPU_FLT)  ?  HIGH: omFM_PWR_FLT_PVCCD_HV;
			 oCPU_PWR_FLT_HBM_CPU               <= (oFM_PVPP_HBM_CPU_EN         && rPWRGD_PVPP_HBM_CPU_FF1           && !iPWRGD_PVPP_HBM_CPU)         && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_HBM_CPU;
			 oCPU_PWR_FLT_EHV_CPU               <= (oFM_PVCCFA_EHV_CPU_EN       && rPWRGD_PVCCFA_EHV_CPU_FF1         && !iPWRGD_PVCCFA_EHV_CPU)       && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_EHV_CPU;
			 oCPU_PWR_FLT_EHV_FIVRA_CPU         <= (oFM_PVCCFA_EHV_FIVRA_CPU_EN && rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1   && !iPWRGD_PVCCFA_EHV_FIVRA_CPU) && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_EHV_FIVRA_CPU;
			 oCPU_PWR_FLT_PVCCINFAON_CPU        <= (oFM_PVCCINFAON_CPU_EN       && rPWRGD_PVCCINFAON_CPU_FF1         && !iPWRGD_PVCCINFAON_CPU)       && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_PVCCINFAON_CPU;
			 oCPU_PWR_FLT_PVNN                  <= (oFM_PVNN_MAIN_CPU_EN        && rPWRGD_PVNN_MAIN_CPU_FF1          && !iPWRGD_PVNN_MAIN_CPU)        && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_PVNN;
			 oCPU_PWR_FLT_PVCCIN                <= (oFM_PVCCIN_CPU_EN           && rPWRGD_PVCCIN_CPU_FF1             && !iPWRGD_PVCCIN_CPU)           && !rCPU_FLT   ?  HIGH: oCPU_PWR_FLT_PVCCIN;

             //alarios: PDG states that PVCCD is considered as MEM_VR_FAILURE, not CPU_VR_FAILURE, hence, it is removed from rCPU_FLT equation
			 //rCPU_FLT     <= (oCPU_PWR_FLT_HBM_CPU || oCPU_PWR_FLT_EHV_CPU || oCPU_PWR_FLT_EHV_FIVRA_CPU || oCPU_PWR_FLT_PVCCINFAON_CPU || oCPU_PWR_FLT_PVNN || oCPU_PWR_FLT_PVCCIN || omFM_PWR_FLT_PVCCD_HV) ? HIGH: rCPU_FLT;
             rCPU_FLT     <= (oCPU_PWR_FLT_HBM_CPU || oCPU_PWR_FLT_EHV_CPU || oCPU_PWR_FLT_EHV_FIVRA_CPU || oCPU_PWR_FLT_PVCCINFAON_CPU || oCPU_PWR_FLT_PVNN || oCPU_PWR_FLT_PVCCIN) ? HIGH: rCPU_FLT;
             oCPU_PWR_FLT <= (oCPU_PWR_FLT_HBM_CPU || oCPU_PWR_FLT_EHV_CPU || oCPU_PWR_FLT_EHV_FIVRA_CPU || oCPU_PWR_FLT_PVCCINFAON_CPU || oCPU_PWR_FLT_PVNN || oCPU_PWR_FLT_PVCCIN || omFM_PWR_FLT_PVCCD_HV) ? HIGH: oCPU_PWR_FLT;
             
		  end
     end 
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   always @(posedge iClk, negedge iRst_n) //Mux to select VR MEMORY Fault for S3
     begin
        if (  !iRst_n ) 
          begin
	         oMEM_PRW_FLT            <= LOW;
          end
        else if (!iCPU_PWR_EN && iMEM_S3_EN && !iFM_CPU_SKTOCC_N) begin        
  	       case ({iFM_INTR_PRSNT, iINTR_SKU})
             
             2'b00   : oMEM_PRW_FLT <= omFM_PWR_FLT_PVCCD_HV ;                                       //SPR 
             2'b11   : oMEM_PRW_FLT <= (oCPU_PWR_FLT_EHV_FIVRA_CPU  || omFM_PWR_FLT_PVCCD_HV);       //CLX
             2'b10   : oMEM_PRW_FLT <= (omFM_PWR_FLT_PVCCD_HV       || oCPU_PWR_FLT_VDDQ_MEM_BRS);   //BRS
             default : oMEM_PRW_FLT <= LOW; 
           endcase
        end
     end
   
   always @(posedge iClk, negedge iRst_n) //oMEM_PWRGD
     begin
        if (  !iRst_n ) begin						
	       oMEM_PWRGD            <= LOW;
           oMemPwrGdFail         <= 1'b0;
        end
        else if (!iFM_CPU_SKTOCC_N)begin
           case ({(iFM_INTR_PRSNT), iINTR_SKU})
             2'b00   : begin
                oMEM_PWRGD <=  imPWRGD_PVCCD_HV_CPU;                                                   //SPR
                oMemPwrGdFail <= omFM_PWR_FLT_PVCCD_HV;
             end
             2'b11   : begin 
                oMEM_PWRGD <= (iPWRGD_PVCCFA_EHV_FIVRA_CPU && imPWRGD_PVCCD_HV_CPU && iPWRGD_VTT);     //CLX
                oMemPwrGdFail <= omFM_PWR_FLT_PVCCD_HV || oCPU_PWR_FLT_EHV_FIVRA_CPU || (oMEM_PWRGD && ~iPWRGD_VTT && oPWRGD_PLT_AUX_CPU);
             end
             
             2'b10   : begin 
                oMEM_PWRGD <= (imPWRGD_PVCCD_HV_CPU        /*&& iPWRGD_VDDQ_MEM_BRS*/ );                   //BRS
                oMemPwrGdFail <= omFM_PWR_FLT_PVCCD_HV;
             end
             default : oMEM_PWRGD <= LOW; 
           endcase
        end
        
     end // always @ (posedge iClk)
   
   
   //////////////////////////////////////////////////////////////////////
   // Instances 
   //////////////////////////////////////////////////////////////////////
   counter2 #( .MAX_COUNT(T_500uS_2M) ) ForceOff_500usDly
     ( .iClk         (iClk), 
       .iRst_n       	(iRst_n),
       
       .iCntRst_n  	(wCNT_RST_N),
       .iCntEn      	(wCNT_RST_N),
       .iLoad        (!(wCNT_RST_N)),
       .iSetCnt    	(T_500uS_2M[9:0]),  // set the time out time as 500us
       
       .oDone        	(wDLY_500uS),
       .oCntr         	( ) 
       );
   
   
   //is CLX CPU?  then "oFM_PVCCFA_EHV_FIVRA_CPU_EN" else BRS "omFM_PVCCD_HV_CPU_EN". Memory should be up after 2mS per power up spec
   assign  wDLY_2mS_EN = (iFM_INTR_PRSNT && iINTR_SKU)? oFM_PVCCFA_EHV_FIVRA_CPU_EN : omFM_PVCCD_HV_CPU_EN ;
   
   counter2 #( .MAX_COUNT(T_2mS_2M) ) Start_2msDly
     ( .iClk         (iClk), 
       .iRst_n       	(iRst_n),
       
       .iCntRst_n  	(wDLY_2mS_EN),
       .iCntEn      	(wDLY_2mS_EN),
       .iLoad        (!(wDLY_2mS_EN)),
       .iSetCnt    	(T_2mS_2M[11:0]),  // set the time out time as 2mS
       .oDone        (wDLY_2mS),
       .oCntr         	( ) 
       );
endmodule
