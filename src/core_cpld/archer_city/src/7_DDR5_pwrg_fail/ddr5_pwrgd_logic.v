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
	input 		  iClk,						  //Clock input.
	input 	    iRst_n,					  //Reset enable on high.
	
  input       iFM_INTR_PRSNT,   //Detect Interposer 
  input       iINTR_SKU,        //Detect Interposer SKU (1 = CLX or 0 = BRS)

	input  	    iPWRGD_PS_PWROK,			     //PWRGD with generate by Power Supply.
	input 		  iPWRGD_DRAMPWRGD_DDRIO,    //DRAMPWRGD for valid DIMM FAIL detection
	input       iMC_RST_N,                 //Reset from memory controller
	input       iADR_LOGIC,                //ADR controls pwrgd/fail output
	inout 		  ioPWRGD_FAIL_CH_DIMM_CPU,	 //PWRGD_FAIL bidirectional signal
	      
	output reg  oDIMM_MEM_FLT, 			       //MEM Fault 
	output    	oPWRGD_DRAMPWRGD_OK,       //DRAM PWR OK
	output      oFPGA_DIMM_RST_N           //Reset to DIMMs
);

//////////////////////////////////////////////////////////////////////////////////
// States for root state machine
//////////////////////////////////////////////////////////////////////////////////
     localparam ST_IDLE              = 4'd0;
     localparam ST_PSU               = 4'd1;
     localparam ST_DDRIO             = 4'd2;
     localparam ST_LINK              = 4'd3;
     localparam ST_FAULT             = 4'd4;
//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
     localparam  LOW  = 1'b0;
     localparam  HIGH = 1'b1;

     reg [2:0] root_state;                        //main sequencing state machine
     reg       rPWRGD_FAIL_CH_DIMM_CPU_EN;
     reg       rPWRGD_DRAMPWRGD_OK;
     wire      wPWRGD_FAIL_AND_ADR_LOGIC;
     wire      wFPGA_DIMM_RST_N;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments    /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
assign ioPWRGD_FAIL_CH_DIMM_CPU  = wPWRGD_FAIL_AND_ADR_LOGIC ? 1'bz : LOW;        //ouput pwrgd fail low if adr logic is low. Set as open drain only in ST_DDRIO or ST_LINK state
assign wFPGA_DIMM_RST_N          = (rPWRGD_DRAMPWRGD_OK && ioPWRGD_FAIL_CH_DIMM_CPU)? iMC_RST_N : LOW; //dimm out of reset if MC is high AND pwrgd_fail input is high 
assign wPWRGD_FAIL_AND_ADR_LOGIC = (rPWRGD_FAIL_CH_DIMM_CPU_EN) && iADR_LOGIC;    //ADR logic and reg fault will enable pwrgd fail output as high impedance

//Interposer support//////////////////////////////////////////////////////////////
//CLX uses DDR4 so MEM RST needs to be bypassed
assign oFPGA_DIMM_RST_N    = (iFM_INTR_PRSNT && iINTR_SKU) ? iMC_RST_N :wFPGA_DIMM_RST_N;
assign oPWRGD_DRAMPWRGD_OK = (iFM_INTR_PRSNT && iINTR_SKU) ? iPWRGD_DRAMPWRGD_DDRIO :rPWRGD_DRAMPWRGD_OK; 


//////////////////////////////////////////////////////////////////////////////////
     //State machine for DDR5 MEM PWRDG FAIL
     always @ (posedge iClk, negedge iRst_n)
     begin
          if(!iRst_n)                             //asynchronous, negative-edge reset
          begin
      			rPWRGD_FAIL_CH_DIMM_CPU_EN  <= HIGH;
      			oDIMM_MEM_FLT               <= LOW;   //MEM Fault 
      			rPWRGD_DRAMPWRGD_OK         <= LOW;   //DRAM PWR OK
          end
          else
          begin
               case(root_state)
               ST_IDLE: 
               begin
                    if(iPWRGD_PS_PWROK)                               //Condition to start DDR5 MEM power up sequence
                         begin
                         	root_state <= ST_PSU;                       //The FSM shall transition to the ST_PSU  state when PSU supply is valid                   
                         end      
               end
               
               ST_PSU:
               begin
                    if(!iPWRGD_PS_PWROK)                              //if power down flag set, skip to power ST_IDLE
                    		root_state <= ST_IDLE;
                    else if(iPWRGD_PS_PWROK && iPWRGD_DRAMPWRGD_DDRIO)//iThe FSM shall transition to ST_DDRIO state when iMC VR13 supply is valid
                    begin
                    		root_state <= ST_DDRIO;                       //go to next state
                    end
               end
               
               ST_DDRIO: 
               begin
               		rPWRGD_FAIL_CH_DIMM_CPU_EN <= HIGH;                 //FPGA will float its internal pull-down driver (PWRGD_FAIL[n:0] output pin)
                    if(!iPWRGD_PS_PWROK)                              //if power down flag set, skip to power ST_IDLE
                    		root_state <= ST_IDLE;
                    else if(iPWRGD_PS_PWROK && iPWRGD_DRAMPWRGD_DDRIO && ioPWRGD_FAIL_CH_DIMM_CPU) 
                    begin
                    		root_state <= ST_LINK;                        //go to next state
                    end
               end

               ST_LINK: 
               begin
                    rPWRGD_DRAMPWRGD_OK  <= HIGH;                      //All DIMMs in Memory Controller are up and running
                    if(!iPWRGD_PS_PWROK)                               //if power down flag set, skip to power ST_IDLE
                    		root_state <= ST_IDLE;
                    else if(iPWRGD_PS_PWROK && (~iPWRGD_DRAMPWRGD_DDRIO || ~ioPWRGD_FAIL_CH_DIMM_CPU)) // if the iMC VR or DIMM PMIC drops out of regulation... 
                    		begin                                          //...(an exclusive OR condition), the FSM will set error bit = ‘0’ and transition to ST_FAULT state
                    			root_state <= ST_FAULT;                           
                    		end
               end

               ST_FAULT: 
               begin
                    rPWRGD_FAIL_CH_DIMM_CPU_EN <= LOW;
                    rPWRGD_DRAMPWRGD_OK        <= LOW;
                    oDIMM_MEM_FLT              <= HIGH;               //The only way to get out of reset is AC cycle
               end
               
               default: 
               begin
                    root_state <= ST_IDLE;
               end
               endcase     
          end
     end

endmodule
