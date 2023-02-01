module ddr5_pwrgd_logic_modular #(
parameter MC_SIZE  = 4
)(
input 	iClk,				//Clock input.
input	iRst_n,				//Reset enable on high.
input	iFM_INTR_PRSNT,     //Detect Interposer 
input	iINTR_SKU,          //Detect Interposer SKU (1 = CLX or 0 = BRS)
input  	iPWRGD_PS_PWROK,	//PWRGD with generate by Power Supply.

input [MC_SIZE-1:0] iPWRGD_DRAMPWRGD_DDRIO, 	//DRAMPWRGD for valid DIMM FAIL detection, input from MC VR
input [MC_SIZE-1:0] iMC_RST_N,              	//Reset from memory controller
input [MC_SIZE-1:0] iADR_LOGIC,             	//ADR controls pwrgd/fail output
inout [MC_SIZE-1:0] ioPWRGD_FAIL_CH_DIMM_CPU,	//PWRGD_FAIL bidirectional signal

output [MC_SIZE-1:0] oDIMM_MEM_FLT,			//MEM Fault 
output [MC_SIZE-1:0] oPWRGD_DRAMPWRGD_OK,	//DRAM PWR OK
output [MC_SIZE-1:0] oFPGA_DIMM_RST_N       //Reset to DIMMs
);

genvar i;

generate//Generate DDR5 modules for all CPU MC
	for (i = 0; i < MC_SIZE; i = i + 1) begin :MC
		ddr5_pwrgd_logic ddr5_pwrgd_logic_inst(
		.iClk(iClk),
		.iRst_n(iRst_n),
		.iFM_INTR_PRSNT(iFM_INTR_PRSNT),
		.iINTR_SKU(iINTR_SKU),
		.iPWRGD_PS_PWROK(iPWRGD_PS_PWROK),

		.iPWRGD_DRAMPWRGD_DDRIO(iPWRGD_DRAMPWRGD_DDRIO[i]),
		.iMC_RST_N(iMC_RST_N[i]),
		.iADR_LOGIC(iADR_LOGIC[i]),
		.ioPWRGD_FAIL_CH_DIMM_CPU(ioPWRGD_FAIL_CH_DIMM_CPU[i]),

		.oDIMM_MEM_FLT(oDIMM_MEM_FLT[i]),
		.oPWRGD_DRAMPWRGD_OK(oPWRGD_DRAMPWRGD_OK[i]),
		.oFPGA_DIMM_RST_N(oFPGA_DIMM_RST_N[i])
		);
	end
endgenerate

endmodule