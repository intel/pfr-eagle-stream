//////////////////////////////////////////////////////////////////////////////////
// Company:    Intel GDC
// Engineer:  Humberto Martinez Aldrete (humberto.martinez.aldrete@intel.com)
// 
// Create Date:   08/09/2019
// Design Name: 
// Module Name:    PERST
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module perst(iClk, iRst_n, PWRG_CPUPWRGD_LVC1,RST_PLTRST_N,FM_RST_PERST_BIT,RST_PCIE_CPU0_DEV_PERST_N,RST_PCIE_CPU1_DEV_PERST_N,RST_PCIE_PCH_DEV_PERST_N,RST_PLTRST_PLD_B_N);

input iClk;
input iRst_n;
   
input PWRG_CPUPWRGD_LVC1;             //input coming from PCH
input RST_PLTRST_N;						  //input coming from PCH
input [2:0] FM_RST_PERST_BIT;			  //input set by straps on the platform (jumpers)

output reg RST_PCIE_CPU0_DEV_PERST_N; //output signals going to CPU0 PCIe slots
output reg RST_PCIE_CPU1_DEV_PERST_N; //output signals going to CPU1 PCIe slots
output reg RST_PCIE_PCH_DEV_PERST_N;  //output signals going to PCH PCIe slots
output 	  RST_PLTRST_PLD_B_N;		  //Buffered signal of  RST_PLTRST_N
 
parameter  LOW  = 1'b0;
parameter  HIGH = 1'b1;


   wire   wDoneCpuPwrgdTimer;

   delay #(.COUNT(1300000))
     TimerCpuPwrgd(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(PWRG_CPUPWRGD_LVC1),
               .iClrCnt(1'b0),
               .oDone(wDoneCpuPwrgdTimer)
               );


always @ (*) begin               //alarios: to facilitate maintenance, used "*" instead of specific signals in sensitivity list
   //if(FM_RST_PERST_BIT[0] == HIGH) begin   //--> alarios: 15-Feb-2021 : Inverted the value on where options are selected, now PLTRST is with a LOW and CPUPWRGD HIGH (per Arch Request)
   if(FM_RST_PERST_BIT[0] == LOW) begin        
		 RST_PCIE_CPU0_DEV_PERST_N = RST_PLTRST_N;
	end else begin
	   RST_PCIE_CPU0_DEV_PERST_N = wDoneCpuPwrgdTimer; //PWRG_CPUPWRGD_LVC1;
	end
   //if(FM_RST_PERST_BIT[1] == HIGH) begin
   if(FM_RST_PERST_BIT[1] == LOW) begin       
		RST_PCIE_CPU1_DEV_PERST_N = RST_PLTRST_N;
	end else begin
	   RST_PCIE_CPU1_DEV_PERST_N = wDoneCpuPwrgdTimer; //PWRG_CPUPWRGD_LVC1;
	end	
	//if(FM_RST_PERST_BIT[2] == HIGH) begin            //--> alarios: 15-Feb-2021 : For PCH devices, no more choice, it is always from PLTRST_N
   RST_PCIE_PCH_DEV_PERST_N = RST_PLTRST_N;  
	//end else begin
	//   RST_PCIE_PCH_DEV_PERST_N = PWRG_CPUPWRGD_LVC1;
	//end	
end

assign RST_PLTRST_PLD_B_N = RST_PLTRST_N;

endmodule



