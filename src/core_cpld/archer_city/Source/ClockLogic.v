//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Clock Logic </b>
	\file     ClockLogic.v
	\details    <b>Image of the Block:</b>
				\image html  ClockLogic.png
                
				This module allow to turn on the clock generator:\n

				FM_PLD_CLKS_OE_N used enable the output. \n
				FM_CPU1/2_MCP_CLK_OE_N used to turn on MCP  clocks
			 
				
    \brief  <b>Last modified</b> 
            $Date:   Jun 19, 2018 $
            $Author:  David.bolanos@intel.com $            
            Project            : Wilson City RP
            Group            : BD
    \version    
             20180118 \b  David.bolanos@intel.com - File creation\n
			 20180522 \b  jorge.juarez.campos@intel.com - Modified for different MCP packages support\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module ClockLogic
(
	input			iClk,//%Clock input
	input			iRst_n,//%Reset enable on high
		
	input           iMainVRPwrgd,//% PWRGD of Main VR's from MainVR_Seq.v module
	input           iMCP_EN_CLK,	//%MCP Clock Enable 1
	input           PWRGD_PCH_PWROK,//%PCH PWROK from PCH

	output  reg     FM_PLD_CLKS_OE_N,	//%OE# enable if iMainVRPwrgd && PWRGD_PCH_PWROK = 1, OE# is asserted.
	output  reg     FM_CPU_BCLK5_OE_N	//%OE# enable if iMainVRPwrgd && PWRGD_PCH_PWROK = 1, OE# is asserted.
);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////
always @ ( posedge iClk) 
begin 
	if (  !iRst_n  )   
	begin
		FM_PLD_CLKS_OE_N       		<= HIGH;
		FM_CPU_BCLK5_OE_N			<= HIGH;
	end
	else 
	begin
		FM_CPU_BCLK5_OE_N   		<=  !(PWRGD_PCH_PWROK && iMainVRPwrgd && iMCP_EN_CLK);
		FM_PLD_CLKS_OE_N       		<=  !(iMainVRPwrgd && PWRGD_PCH_PWROK);				
	end
end  


//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////


endmodule
