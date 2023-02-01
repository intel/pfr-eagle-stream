module ddr5_pwrgd_logic_modular #(
parameter MC_SIZE  = 4)
(
 input                iClk, //Clock input.
 input                iRst_n, //Reset enable on high.
 input                iFM_INTR_PRSNT, //Detect Interposer 
 input                iINTR_SKU, //Detect Interposer SKU (1 = CLX or 0 = BRS)
 input                iSlpS5Id,  //Sleep S5 indication (from master fub)
 input                iCpuPwrGd, //Output to CPUs, used to know when to de-assert DRAMPWROK
 input                iAcRemoval,
 input                iDimmPwrOK, //PWRGD with generate by Power Supply. //alarios 21-Sept-2021. Changed the name from iPWRGD_PS_PWROK as we will be using PWRGD from P3V3 MAIN instead of the PS PWROK (a more general name will suit better)
 input                iAdrSel, //selector for ddr5 logic to know if drive PWRGDFAIL & DIMM_RST from internal or ADR logic
 input                iAdrEvent, //used as filter to avoid wrong error flagging due to PWRGFAIL when ADR flow happens
 input                iAdrDimmRst_n, //DIMM_RST_N signal from ADR logic
 input                iMemPwrGdFail, //in case any of the DRAMPWRGD_DDRIO VRs fail...

 //alarios 07-March-2022 : No need to differentiate ADR from non-ADR during pwr-dwn 
 //input                iAdrComplete, //alarios Jun-28-21 for CPS workaround where DIMM_RST needs to be asserted 600us before DRAMPWROK
 
 input [MC_SIZE-1:0]  iPWRGD_DRAMPWRGD_DDRIO, //DRAMPWRGD for valid DIMM FAIL detection, input from MC VR
 input [MC_SIZE-1:0]  iMC_RST_N, //Reset from memory controller
 input [MC_SIZE-1:0]  iADR_PWRGDFAIL, //ADR controls pwrgd/fail output
 inout [MC_SIZE-1:0]  ioPWRGD_FAIL_CH_DIMM_CPU, //PWRGD_FAIL bidirectional signal
 
 output [MC_SIZE-1:0] oDIMM_MEM_FLT, //MEM Fault 
 output [MC_SIZE-1:0] oPWRGD_DRAMPWRGD_OK, //DRAM PWR OK
 output [MC_SIZE-1:0] oFPGA_DIMM_RST_N       //Reset to DIMMs
 //output               oContinuePwrDwn    //alarios 28-Feb-2022 : removed as no longer required
 );

   wire [MC_SIZE-1:0] wContPwrDwn;
   
   genvar             i;

   generate//Generate DDR5 modules for all CPU MC
	  for (i = 0; i < MC_SIZE; i = i + 1) begin :MC
		 ddr5_pwrgd_logic ddr5_pwrgd_logic_inst(
		                                        .iClk(iClk),
		                                        .iRst_n(iRst_n),
		                                        .iFM_INTR_PRSNT(iFM_INTR_PRSNT),
		                                        .iINTR_SKU(iINTR_SKU),
                                                .iSlpS5Id(iSlpS5Id),
                                                .iCpuPwrGd(iCpuPwrGd),
                                                .iAcRemoval(iAcRemoval),
		                                        .iDimmPwrOK(iDimmPwrOK),
                                                .iAdrSel(iAdrSel),
                                                .iAdrEvent(iAdrEvent),
                                                .iAdrDimmRst_n(iAdrDimmRst_n),
                                                .iMemPwrGdFail(iMemPwrGdFail),

                                                //alarios 07-March-2022 : No need to differentiate ADR from non-ADR during pwr-dwn 
                                                //.iAdrComplete(iAdrComplete),
                                                
		                                        .iPWRGD_DRAMPWRGD_DDRIO(iPWRGD_DRAMPWRGD_DDRIO[i]),
		                                        .iMC_RST_N(iMC_RST_N[i]),
		                                        .iADR_PWRGDFAIL(iADR_PWRGDFAIL[i]),
		                                        .ioPWRGD_FAIL_CH_DIMM_CPU(ioPWRGD_FAIL_CH_DIMM_CPU[i]),
                                                
		                                        .oDIMM_MEM_FLT(oDIMM_MEM_FLT[i]),
		                                        .oPWRGD_DRAMPWRGD_OK(oPWRGD_DRAMPWRGD_OK[i]),
		                                        .oFPGA_DIMM_RST_N(oFPGA_DIMM_RST_N[i])
                                                //.oContinuePwrDwn(wContPwrDwn[i])
		                                        );
	  end
   endgenerate

   //assign oContinuePwrDwn = &(wContPwrDwn);

   
endmodule // ddr5_pwrgd_logic_modular
