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

    input            iClk,               //clock for sequential logic 
    input            iRst_n,             //reset signal, resets state machine to initial state

// ----------------------------
// inputs and outputs
// ---------------------------- 
	input            iCPU_PWR_EN,                      //Cpu power sequence enable
	input            iENABLE_TIME_OUT,                 //Enable the Timeout
	input            iENABLE_HBM,                      //Enable HBM VR
	input            iFM_INTR_PRSNT,                   //Detect Interposer
    input            iFM_CPU_SKTOCC_N, 
	input            iINTR_SKU,                        //Detect Interposer SKU (1 = CLX or 0 = BRS)
	input            iPWRGD_PVPP_HBM_CPU,              //CPU VR PWRGD PVPP_HBM
	input            iPWRGD_PVCCFA_EHV_CPU,            //CPU VR PWRGD PVCCFA_EHV
	input            iPWRGD_PVCCFA_EHV_FIVRA_CPU,      //CPU VR PWRGD PVCCFA_EHV_FIVRA
	input            iPWRGD_PVCCINFAON_CPU,            //CPU VR PWRGD PVCCINFAON
	input            iPWRGD_PVNN_MAIN_CPU,             //CPU VR PWRGD PVNN
	input            imPWRGD_PVCCD_HV_CPU,             //CPU VR MEMORY PWRGD PVCCD_HV
    input            iPWRGD_PVCCIN_CPU,                //CPU VR PVCCIN

 //Inputs/Outputs for inteposer support//////////////////////////////////////////
    input            iPWRGD_VDDQ_MEM_BRS,              //SMB_S3M_CPU_SCL
    input            iPWRGD_P1V8_CPU_BRS,              //SMB_S3M_CPU_SDA
    output  reg      oCPU_PWR_FLT_VDDQ_MEM_BRS,		     //VR fault on interposer           
    output  reg      oCPU_PWR_FLT_P1V8_BRS,		         //VR fault on interposer
    input            iPWRGD_VTT,                   //Pwrgd for Interposer VR (VTT rail), this should be removed or treated for when not having CLX interposer
 ////////////////////////////////////////////////////////////////////////////////
    
    //MEMORY DDR5 Inputs/Outputs  //////////////////////////////////////////////////
    input            iMEM_S3_EN,                       //If set MEM VRs will remain ON 
    output reg       oMEM_PRW_FLT,                     //Fault Memory VR'
    output reg       oMEM_PWRGD,					             //PWRGD of MEMORY  
 ////////////////////////////////////////////////////////////////////////////////
 
    output reg       oFM_PVPP_HBM_CPU_EN,              //CPU VR EN PVPP_HBM
    output reg       oFM_PVCCFA_EHV_CPU_EN,            //CPU VR EN PVCCFA_EHV
	output reg       oFM_PVCCFA_EHV_FIVRA_CPU_EN,      //CPU VR EN PVCCFA_EHV_FIVRA
	output reg       oFM_PVCCINFAON_CPU_EN,            //CPU VR EN PVCCINFAON
	output reg       oFM_PVNN_MAIN_CPU_EN,             //CPU VR EN PVNN
	output reg       omFM_PVCCD_HV_CPU_EN,             //CPU VR EN MEMORY PVCCD_HV
	output reg       oFM_PVCCIN_CPU_EN,                //CPU VR EN PVCCIN
	output reg       oPWRGD_PLT_AUX_CPU,               //CPU PRWGD PLATFORM AUXILIARY

	output reg       oCPU_PWRGD,					   //PWRGD of all CPU VR's     
	output reg       oCPU_PWR_FLT,					   //Fault CPU VR's

	output reg       oCPU_PWR_FLT_HBM_CPU,			   //Fault  PVPP_HBM
	output reg       oCPU_PWR_FLT_EHV_CPU,			   //Fault  PVCCFA_EHV
	output reg       oCPU_PWR_FLT_EHV_FIVRA_CPU,       //Fault  PVCCFA_EHV_FIVRA
	output reg       oCPU_PWR_FLT_PVCCINFAON_CPU,	   //Fault  PVCCINFAON
	output reg       oCPU_PWR_FLT_PVNN,		           //Fault  PVNN
	output reg       oCPU_PWR_FLT_PVCCIN,		       //Fault  PVCCIN
	output reg       omFM_PWR_FLT_PVCCD_HV,		       //Fault  PVCCD_HV

	output [3:0]     oDBG_CPU_ST                       //State of CPU Secuence

);
//////////////////////////////////////////////////////////////////////////////////
// Parameters SPR CPU STATE MACHINE
//////////////////////////////////////////////////////////////////////////////////
localparam  ST_INIT                    = 4'd0;
localparam  ST_PVPP_HBM                = 4'd1;
localparam  ST_PVCCFA_EHV              = 4'd2;
localparam  ST_PVCCINFAON              = 4'd3;
localparam  ST_PVNN                    = 4'd4; 
localparam  ST_PVCCD_HV                = 4'd5;
localparam  ST_PVCCIN                  = 4'd6;
localparam  ST_PVPP_HBM_OFF            = 4'd7;
localparam  ST_PVCCFA_EHV_OFF          = 4'd8;
localparam  ST_PVCCINFAON_OFF          = 4'd9;
localparam  ST_PVNN_OFF                = 4'd10; 
localparam  ST_PVCCD_HV_OFF            = 4'd11;
localparam  ST_PVCCIN_OFF              = 4'd12;
localparam  ST_CPUDONE                 = 4'd13;
localparam  ST_PWR_DOWN                = 4'd14;
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//Parameters CLX CPU STATE MACHINE
//////////////////////////////////////////////////////////////////////////////////
localparam  ST_PVPP_MEM_ABC            = 4'd1;
localparam  ST_PVDDQ_VTT_MEM_ABC       = 4'd2;
localparam  ST_PVTT_MEMABC             = 4'd3;
localparam  ST_PVCCIO                  = 4'd4;
localparam  ST_PVCCSA                  = 4'd5;
localparam  ST_PVPP_MEM_ABC_OFF        = 4'd7;
localparam  ST_PVDDQ_VTT_MEM_ABC_OFF   = 4'd8;
localparam  ST_PVTT_MEMABC_OFF         = 4'd9;
localparam  ST_PVCCIO_OFF              = 4'd10;
localparam  ST_PVCCSA_OFF              = 4'd11;
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Parameters BRS CPU STATE MACHINE
//////////////////////////////////////////////////////////////////////////////////;
localparam  ST_PVDDQ                   = 4'd2;
localparam  ST_PV1V8                   = 4'd3;
localparam  ST_PVCCANA                 = 4'd1;
localparam  ST_PVDDQ_OFF               = 4'd7;
localparam  ST_PV1V8_OFF               = 4'd8;
localparam  ST_PVCCANA_OFF             = 4'd9;
////////////////////////////////////////////////////////////////////////////////
localparam  LOW =1'b0;
localparam  HIGH=1'b1;
localparam  T_500uS_2M  =  10'd1000;
localparam  T_2mS_2M    =  12'd4000;
//////////////////////////////////////////////////////////////////////////////////

reg [3:0]  root_state;                            //main sequencing state machine
reg [3:0]  rPREVIOUS_STATE = ST_INIT;             //previous sequencing state machine

reg         rCPU_FLT;

reg 		rPWRGD_PVPP_HBM_CPU_FF1;
reg 		rPWRGD_PVCCFA_EHV_CPU_FF1;
reg 		rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1;
reg 		rPWRGD_PVCCINFAON_CPU_FF1;
reg 		rPWRGD_PVNN_MAIN_CPU_FF1;
reg 		rPWRGD_PVCCIN_CPU_FF1;
reg     rPWRGD_PVCCD_HV_MEM_FF1;
//BRS Interposer
reg			rPWRGD_VDDQ_MEM_BRS_FF1;
reg			rPWRGD_P1V8_CPU_BRS_FF1;
//////////////////////////////////////////////////////////////////////////////////
wire	   wTIME_OUT_500uS;   //delay of 500uS to turn on the CPU
wire	   wDLY_2mS_EN;     
wire	   wDLY_500uS;
wire	   wDLY_2mS;         //delay of 2mS per power up sequence interposer spec
wire	   wCNT_RST_N;
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
    			oCPU_PWRGD					         <= LOW;	        //PWRGD of all CPU VR's      

            root_state      <= ST_INIT;                      //reset state machines to initial states
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
                      oCPU_PWRGD                   <= LOW;   //PWRGD DOWN  

  	                  if(iENABLE_HBM && iCPU_PWR_EN) begin
  	                      	root_state <= ST_PVPP_HBM;       //go to ST_PVPP_HBM state
  	                      end 
  	                  else if (iCPU_PWR_EN) begin
  	                   	    root_state <= ST_PVCCFA_EHV;     //else go to next state
  	                  end 
                    end
                    
                    ST_PVPP_HBM:
                    begin
                    oFM_PVPP_HBM_CPU_EN          <= HIGH;             //CPU VR EN PVPP_HBM 

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS) //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
	                 else if (iPWRGD_PVPP_HBM_CPU) begin
	                      	root_state <= ST_PVCCFA_EHV;                //Go to ST_PVPP_HBM state
	                      end 
	                 else begin
	                   	    root_state <= ST_PVCCFA_EHV;                //Else go to next state
	                   end  
                    end
                    
                    ST_PVCCFA_EHV: 
                    begin
                    oFM_PVCCFA_EHV_CPU_EN        <= HIGH;                //CPU VR EN PVCCFA_EHV
					          oFM_PVCCFA_EHV_FIVRA_CPU_EN  <= HIGH;                //CPU VR EN PVCCFA_EHV_FIVRA

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)    //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                    //Power Down
	                  end 
	                  else if (iPWRGD_PVCCFA_EHV_CPU && iPWRGD_PVCCFA_EHV_FIVRA_CPU) begin
	                      	root_state <= ST_PVCCINFAON;                //Go to ST_PVCCINFAON state
	                      end 
                    end

                    ST_PVCCINFAON: 
                    begin
                    oFM_PVCCINFAON_CPU_EN        <= HIGH;               //CPU VR EN PVCCINFAON

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)                     //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
	                 else if (iPWRGD_PVCCINFAON_CPU) 
	                  begin
	                       root_state <= ST_PVNN;                       //Go to ST_PVPP_HBM state
	                  end 
                    end
          
                    ST_PVNN:                                            //PVNN PWRGD and Enable goes in parrallel (for both CPUs)
                    begin
					          oFM_PVNN_MAIN_CPU_EN         <= HIGH;
                    
                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS) //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
	                 else if (iPWRGD_PVNN_MAIN_CPU) begin               //If PVNN
	                      	root_state <= ST_PVCCD_HV;                  //Go to ST_PVPP_HBM state
	                      end 
                    end

                    ST_PVCCD_HV: 
                    begin
                    omFM_PVCCD_HV_CPU_EN         <= HIGH;               //CPU VR EN PVCCD_HV
                    oPWRGD_PLT_AUX_CPU           <= HIGH;

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)                     //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                    root_state <= ST_PWR_DOWN;                      //Power Down
	                  end 
	                 else if (imPWRGD_PVCCD_HV_CPU) begin
	                    root_state <= ST_PVCCIN;                        //go to ST_PVCCIN state
	                      end 
                    end

                    ST_PVCCIN: 
                    begin
                    oFM_PVCCIN_CPU_EN            <= HIGH;               //CPU VR EN PVCCIN

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)                     //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
	                 else if (iPWRGD_PVCCIN_CPU) begin
	                      	root_state <= ST_CPUDONE;                   //Go to ST_PVCST_CPUDONE state
	                      end 
                    end

                    ST_CPUDONE: 
                    begin
                    oCPU_PWRGD					 <= HIGH;	            //PWRGD of all CPU VR's

                    if((!iCPU_PWR_EN) ||  rCPU_FLT)                     //if PWR DOWN or PWR FAULT
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
                    end

                    ST_PWR_DOWN: 
                    begin
                    if (rCPU_FLT && iCPU_PWR_EN)                        //If fail but not power down indication then go to power down                   
                         begin
                              root_state <= ST_PVCCIN_OFF;              //Start power down sequence
                         end
                    else if ((!rCPU_FLT) && (!iCPU_PWR_EN))             //If not fail but  power down indication then go to power down sequence                    
                         begin
                              root_state <= ST_PVCCIN_OFF;              //Start power down sequence
                         end
                    end

                    ST_PVCCIN_OFF:
                    begin
                    oFM_PVCCIN_CPU_EN            <= LOW;                 //deassert enable to power down 
                    oPWRGD_PLT_AUX_CPU           <= LOW;

                    if(!iPWRGD_PVCCIN_CPU)                               //if current stage is off
	                begin
	                 if(iMEM_S3_EN)                                       //S3 support
	                    begin
	                    	root_state <= ST_PVNN_OFF;                   //go to ST_PVNN_OFF and dont turn off memory
	                    end
	                else begin
	                        root_state <= ST_PVCCD_HV_OFF;               //now go to next power down state
	                     end     
	                 end
                    end

                    ST_PVCCD_HV_OFF: 
                    begin
                    omFM_PVCCD_HV_CPU_EN         <= LOW;                  //deassert enable to power down 

                    if(!imPWRGD_PVCCD_HV_CPU)                             //if current stage is off
	                    begin
	                         root_state <= ST_PVNN_OFF;                   //now go to next power down state
	                    end
                    end

                    ST_PVNN_OFF: 
                    begin
                    oFM_PVNN_MAIN_CPU_EN         <= LOW;                    //deassert enable to power down

                    if(!iPWRGD_PVNN_MAIN_CPU)                               //if current stage is off
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

                    if((!iPWRGD_PVCCFA_EHV_CPU) && (!iPWRGD_PVCCFA_EHV_FIVRA_CPU) && iENABLE_HBM)  //if current stage is off
	                    begin
	                       root_state <= ST_PVPP_HBM_OFF;                      //now go to state of waiting for power-up
	                    end
	                 else if (!iENABLE_HBM) begin
	                 	   root_state <= ST_INIT;                              //now go to state of waiting for power-up
	                 end
                    end

                    ST_PVPP_HBM_OFF: 
                    begin
                    oFM_PVPP_HBM_CPU_EN          <= LOW;                         //deassert enable to power down 

                    if(!iPWRGD_PVPP_HBM_CPU)                                     //wait for PVPP _HBM VR to go off
	                    begin
	                         root_state <= ST_INIT;                              //now go to state of waiting for power-up
	                    end
                    end

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
                    oCPU_PWRGD                  <= LOW;     //PWRGD DOWN  
	                
                    if(iCPU_PWR_EN) begin
	                      	root_state <= ST_PVPP_MEM_ABC;    //go to ST_PVPP_HBM state
	                      end 
                    end
                    
                    ST_PVPP_MEM_ABC:
                    begin
                    oFM_PVCCFA_EHV_FIVRA_CPU_EN   <= HIGH;              

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)   //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                     //Power Down
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
	                       root_state <= ST_PWR_DOWN;                    //Power Down
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
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                    root_state <= ST_PWR_DOWN;                      //Power Down
	                  end 
	                 else if (iPWRGD_PVCCFA_EHV_CPU) begin
	                    root_state <= ST_CPUDONE;                        
	                      end 
                    end

                    ST_CPUDONE: 
                    begin
                    oCPU_PWRGD					 <= HIGH;	            //PWRGD of all CPU VR's

                    if((!iCPU_PWR_EN) || rCPU_FLT)                      //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
                    end

                    ST_PWR_DOWN: 
                    begin
                   // if (rCPU_FLT && iCPU_PWR_EN)                        //If fail but not power down indication then go to fault state                    
                       //  begin
                              root_state <= ST_PVCCSA_OFF;                //Start power down sequence
                         //end
                    //else if ((!rCPU_FLT) && (!iCPU_PWR_EN))             //If not fail but  power down indication then go to power down sequence                    
                       //  begin
                         //     root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                         //end
                    end

                    ST_PVCCSA_OFF: 
                    begin
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
                    oCPU_PWRGD                   <= LOW;     //PWRGD DOWN  

	                if(iCPU_PWR_EN) begin
	                      	root_state <= ST_PVDDQ;         //go to ST_VDDQ
	                      end 
                    end
                    
                    ST_PVDDQ:
                    begin
                    omFM_PVCCD_HV_CPU_EN        <= HIGH;             

                    if((!iCPU_PWR_EN) || rCPU_FLT || wTIME_OUT_500uS)  //if PWR DOWN or PWR FAULT or watchDog timeout is set then Fault 
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                       root_state <= ST_PWR_DOWN;                      //Power Down
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
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                       root_state <= ST_PWR_DOWN;                   //Power Down
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
	                    root_state <= ST_PWR_DOWN;                      //Power Down
	                  end 
	                 else if (iPWRGD_PVCCFA_EHV_CPU) begin
	                    root_state <= ST_CPUDONE;                        
	                      end 
                    end

                    ST_CPUDONE: 
                    begin
                    oCPU_PWRGD					 <= HIGH;	            //PWRGD of all CPU VR's

                    if((!iCPU_PWR_EN) || rCPU_FLT)                      //if PWR DOWN or PWR FAULT
	                  begin
	                       root_state <= ST_PWR_DOWN;                   //Power Down
	                  end 
                    end

                    ST_PWR_DOWN: 
                    begin
                    if (rCPU_FLT && iCPU_PWR_EN)                        //If fail but not power down indication then go to power down                   
                         begin
                              root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                         end
                    else if ((!rCPU_FLT) && (!iCPU_PWR_EN))             //If not fail but  power down indication then go to power down sequence                    
                         begin
                              root_state <= ST_PVCCSA_OFF;              //Start power down sequence
                         end
                    end

                    ST_PVCCSA_OFF: 
                    begin
                    oFM_PVCCFA_EHV_CPU_EN        <= LOW;                //deassert enable to power down 

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
               end                                                 //End of CLX CPU state machine
          end
     end


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FAULT LOGIC
////////////////////////////////////////////////////////////////////////////////////////////////////////////
always @( posedge iClk) 
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
				oCPU_PWR_FLT_VDDQ_MEM_BRS          <= (omFM_PVCCD_HV_CPU_EN      && rPWRGD_VDDQ_MEM_BRS_FF1           && !iPWRGD_VDDQ_MEM_BRS)         ?  HIGH: oCPU_PWR_FLT_VDDQ_MEM_BRS;	   //VR fault on interposer           
				oCPU_PWR_FLT_P1V8_BRS              <= (oPWRGD_PLT_AUX_CPU        && rPWRGD_P1V8_CPU_BRS_FF1           && !iPWRGD_P1V8_CPU_BRS)         ?  HIGH: oCPU_PWR_FLT_P1V8_BRS;		   //VR fault on interposer   
				//////////////////////////////////////////////////////////
				omFM_PWR_FLT_PVCCD_HV              <= (omFM_PVCCD_HV_CPU_EN        && rPWRGD_PVCCD_HV_MEM_FF1           && !imPWRGD_PVCCD_HV_CPU)         ?  HIGH: omFM_PWR_FLT_PVCCD_HV;
				oCPU_PWR_FLT_HBM_CPU               <= (oFM_PVPP_HBM_CPU_EN         && rPWRGD_PVPP_HBM_CPU_FF1           && !iPWRGD_PVPP_HBM_CPU)          ?  HIGH: oCPU_PWR_FLT_HBM_CPU;
				oCPU_PWR_FLT_EHV_CPU               <= (oFM_PVCCFA_EHV_CPU_EN       && rPWRGD_PVCCFA_EHV_CPU_FF1         && !iPWRGD_PVCCFA_EHV_CPU)        ?  HIGH: oCPU_PWR_FLT_EHV_CPU;
				oCPU_PWR_FLT_EHV_FIVRA_CPU         <= (oFM_PVCCFA_EHV_FIVRA_CPU_EN && rPWRGD_PVCCFA_EHV_FIVRA_CPU_FF1   && !iPWRGD_PVCCFA_EHV_FIVRA_CPU)  ?  HIGH: oCPU_PWR_FLT_EHV_FIVRA_CPU;
				oCPU_PWR_FLT_PVCCINFAON_CPU        <= (oFM_PVCCINFAON_CPU_EN       && rPWRGD_PVCCINFAON_CPU_FF1         && !iPWRGD_PVCCINFAON_CPU)        ?  HIGH: oCPU_PWR_FLT_PVCCINFAON_CPU;
				oCPU_PWR_FLT_PVNN                  <= (oFM_PVNN_MAIN_CPU_EN        && rPWRGD_PVNN_MAIN_CPU_FF1          && !iPWRGD_PVNN_MAIN_CPU)         ?  HIGH: oCPU_PWR_FLT_PVNN;
				oCPU_PWR_FLT_PVCCIN                <= (oFM_PVCCIN_CPU_EN           && rPWRGD_PVCCIN_CPU_FF1             && !iPWRGD_PVCCIN_CPU)            ?  HIGH: oCPU_PWR_FLT_PVCCIN;

				rCPU_FLT     <= (oCPU_PWR_FLT_HBM_CPU || oCPU_PWR_FLT_EHV_CPU || oCPU_PWR_FLT_EHV_FIVRA_CPU || oCPU_PWR_FLT_PVCCINFAON_CPU || oCPU_PWR_FLT_PVNN || oCPU_PWR_FLT_PVCCIN || omFM_PWR_FLT_PVCCD_HV) ? HIGH: rCPU_FLT;
        oCPU_PWR_FLT <= (oCPU_PWR_FLT_HBM_CPU || oCPU_PWR_FLT_EHV_CPU || oCPU_PWR_FLT_EHV_FIVRA_CPU || oCPU_PWR_FLT_PVCCINFAON_CPU || oCPU_PWR_FLT_PVNN || oCPU_PWR_FLT_PVCCIN || omFM_PWR_FLT_PVCCD_HV) ? HIGH: oCPU_PWR_FLT;

			end
end 
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk) //Mux to select VR MEMORY Fault for S3
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
   
   always @(posedge iClk) //oMEM_PWRGD
     begin
        if (  !iRst_n ) begin						
	       oMEM_PWRGD            <= LOW;
        end
        else if (!iFM_CPU_SKTOCC_N)begin
           case ({(iFM_INTR_PRSNT), iINTR_SKU})
             2'b00   : oMEM_PWRGD <=  imPWRGD_PVCCD_HV_CPU;                                                   //SPR 
             2'b11   : oMEM_PWRGD <= (iPWRGD_PVCCFA_EHV_FIVRA_CPU && imPWRGD_PVCCD_HV_CPU && iPWRGD_VTT);     //CLX
             2'b10   : oMEM_PWRGD <= (imPWRGD_PVCCD_HV_CPU        /*&& iPWRGD_VDDQ_MEM_BRS*/ );                   //BRS
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
