//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%ADR </b>
    \file     ADR.v
    \details    <b>Image of the Block:</b>
                \image html ADR.png

                 <b>Description:</b> \n
                ADR NVDIMM features is to allow the system to backup important information to its
                non-volatile memory section when a power supply failure is detected with minimal baseboard level HW or FW support.\n\n 

                The assertion of FM_ADR_TRIGGER from Main PLD would trigger the ADR (Asynchronous DIMM Self-Refresh) 
                function of LBG which would instruct CPU to save required RAID information to system memory and put 
                memory into Self-Refresh mode. \n\n

                After ADR timer expired (maximum 100us), LBG would assert ADR_COMPLETE signal which is connected 
                to SAVE# pin of each DDR4 slot after inversion. There is a reserved implemented in PLD to have additional 
                delay on ADR_COMPLETE in case 100us was determined as not sufficient for CPU to complete all the ADR operation.\n

                At the falling edge of SAVE# pin, NVDIMM would back up the content on DDR Chips onto the NVM (Typically NAND Flash) on NVDIMM. 
                During this back-up operation, system power is not required to be valid, as the power will be sourced from its own battery.\n\n
                


                <b>Signal function:</b>\n
                The signal FM_ADR_TRIGGER_N /FM_ADR_SMI_GPIO_N is a copy of inverted of PSU Fault  also will be de-asserted if PWRGD_CPUPWRGD go to LOW\n

                FM_PS_PWROK_DLY_SEL control the power down delay added for PWRGD_PS_PWROK:
                    0 -> 15mS delay for eADR mode
                    1 -> 600uS delay for Legacy ADR mode\n

                PWRGD_PS_PWROK_DLY_ADR is a copy of PWRGD_PS_PWROK but will the ADR delay added. This will be used to delay the deassertion of PCH_PWROK\n
                FM_AUX_SW_EN_DLY_ADR is a copy of FM_AUX_SW_EN but will the ADR delay added.\n

                FM_DIS_PS_PWROK_DLY and FM_PLD_PCH_DATA will Disable delay from PWRGD_PS (0 DLY power-down condition)  \n


          

                
    \brief  <b>Last modified</b> 
            $Date:   Jan 4, 2018 $
            $Author:  David.bolanos@intel.com $         
            Project         : Wilson City RP 
            Group           : BD
    \version    
             20180104 \b  David.bolanos@intel.com - File creation\n
             20181405 \b  jorge.juarez.campos@intel.com - ADR Module Fix\n
               
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module ADR
(
    input  iClk,//%Clock input
    input  iRst_n,//%Reset enable on high
    input  i10uSCE, //% 10 uS Clock Enable

    input   FM_PS_EN, //%
    input   PWRGD_PS_PWROK, //%  PWRGD with delay from PSU_Seq module.
    input   PWRGD_CPUPWRGD, //% Pwrgd CPU from PCH, without delay

    input   FM_PS_PWROK_DLY_SEL,  //%from PCH, 0 delay 15ms, 1 delay 600us(default)
    input   FM_DIS_PS_PWROK_DLY, //% Disable delay from PWRGD_PS (0 DLY power-down condition)   
   
    input   FM_ADR_COMPLETE,  //%ADR complete from PCH  
    input   FM_PLD_PCH_DATA,  //%DATAfrom PCH, 0 to disable NVDIMM function, 1 to enable NVDIMM(default)    
    input   FM_AUX_SW_EN, //%FM_AUX_SW_EN from MainVR_Seq without delay
    input   FM_P5V_EN, //%FM_P5V_EN from MainVR_Seq without delay
    input   iPsuPwrFlt, //% Power Supply fault detection

    input   PWRGD_PCH_PWROK,    // input
    input   FM_SLPS4_N, // input 

    output  FM_ADR_SMI_GPIO_N,  //% GPIO to PCH
    output  FM_ADR_TRIGGER_N,  //% ADR Trigger# to PCH
    output  FM_ADR_COMPLETE_DLY,//% copy of FM_ADR_COMPLETE (no delay added)
    output  reg PWRGD_PS_PWROK_DLY_ADR, //% PWRGD_PS_PWROK delay add acording ADR logic SEL 0 delay 15ms, 1 delay 600us(default)
    output  FM_AUX_SW_EN_DLY_ADR, //% Delay for AUX_SW_EN 100us for PWR down.
    output  FM_P5V_EN_DLY //% Delay for FM_P5V_EN 1Ss for PWR down after PCH_PWROK de-assertion.
);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter   T_600US_2M  =  32'd1200;  
parameter   T_15MS_2M   =  32'd30000;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
wire            wPsPwrok_dly_600us_n;
wire            wPsPwrok_dly_15ms_n;
wire            wFM_P12V_AUX_SW_EN_DLY;
wire            wFM_P5V_EN_DLY;
wire            wPsPwrok_F_Edge;

reg             rPsPwrok_dly_sel_gated;
reg             rPchData_gated;
reg     [1:0]   rvPsPwrOkQ;                 // Internal copy -> 2-bits to detect edge
reg             rPsPwrok_F_Edge;
reg             rFM_AUX_SW_EN_1;
reg             rFM_P5V_EN_1;


//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////
assign FM_ADR_TRIGGER_N  = (!PWRGD_CPUPWRGD) ? 1'b1 : ~iPsuPwrFlt; 
assign FM_ADR_SMI_GPIO_N = (!PWRGD_CPUPWRGD) ? 1'b1 : ~iPsuPwrFlt; 
assign FM_ADR_COMPLETE_DLY = FM_ADR_COMPLETE;
assign FM_AUX_SW_EN_DLY_ADR = ~rPsPwrok_F_Edge ? rFM_AUX_SW_EN_1 : wFM_P12V_AUX_SW_EN_DLY; //falling edge detection is present
assign FM_P5V_EN_DLY        = ~rPsPwrok_F_Edge ? rFM_P5V_EN_1    : wFM_P5V_EN_DLY; //falling edge detection is present
// PS_PWROK falling edge detection
assign  wPsPwrok_F_Edge =   ( rvPsPwrOkQ == 2'b10 );

   
//////////////////////////////////////////////////////////////////////////////////
// Combination logic for PS_PWROK Delay
//////////////////////////////////////////////////////////////////////////////////
always @ (*)
begin
    // if (!FM_DIS_PS_PWROK_DLY || !rPchData_gated) //Add support for jumper disable dly
    //     PWRGD_PS_PWROK_DLY_ADR = PWRGD_PS_PWROK;
    if (iPsuPwrFlt)   // surprised AC off
        // PWRGD_PS_PWROK_DLY_ADR = rPsPwrok_dly_sel_gated ? ~wPsPwrok_dly_600us_n : ~wPsPwrok_dly_15ms_n ; 
        PWRGD_PS_PWROK_DLY_ADR = wPsPwrok_dly_600us_n; 
    else 
        PWRGD_PS_PWROK_DLY_ADR = PWRGD_PS_PWROK;
end


//////////////////////////////////////////////////////////////////////////////////
// Sequencial Logic for PS_PWROK falling detection and latch PCH DATA & PWROK_DLY_SEL
//////////////////////////////////////////////////////////////////////////////////
always @ ( posedge iClk)
begin
    if  ( !iRst_n )
    begin
        rvPsPwrOkQ              <=  2'b00;
        rPsPwrok_dly_sel_gated  <=  1'b1;
        rPchData_gated          <=  1'b1;
		rPsPwrok_F_Edge 		<= 	1'b0;
        rFM_AUX_SW_EN_1         <=  1'b0;
		rFM_P5V_EN_1 		    <=  1'b0;
    end
    else
    begin
        // PS_PWROK FFs for edge detection
        rvPsPwrOkQ[1]           <= rvPsPwrOkQ[0];
        rvPsPwrOkQ[0]           <= PWRGD_PS_PWROK;
		
		rPsPwrok_F_Edge 		<= ( (rvPsPwrOkQ == 2'b10) && FM_PS_EN ) ? 1'b1 : rPsPwrok_F_Edge;
        rFM_AUX_SW_EN_1         <=  FM_AUX_SW_EN;
		rFM_P5V_EN_1 		    <=  FM_P5V_EN;
		

        if (wPsPwrok_F_Edge) // if a falling edge detection is present
        begin
            rPsPwrok_dly_sel_gated  <=FM_PS_PWROK_DLY_SEL;
            rPchData_gated          <=FM_PLD_PCH_DATA;
        end
        else
        begin
            rPsPwrok_dly_sel_gated  <=rPsPwrok_dly_sel_gated;
            rPchData_gated          <=rPchData_gated;
        end

    end
end


//////////////////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////////////////
//Delay for 100us on ADR for AUX SW EN after PCH_PWROK de-asserts
SignalValidationDelay#
(
    
    .VALUE      ( 1'b0 ),
    .TOTAL_BITS ( 3'd4 ),
    .POL        ( 1'b0 )
)mFM_AUX_SW_EN_ADR_DLR
(
    .iClk       ( iClk ),
    .iRst       ( ~iRst_n ),
    .iCE        ( i10uSCE ),
    .ivMaxCnt   ( 4'd10 ),
	.iStart     ( PWRGD_PCH_PWROK ),
    .oDone      ( wFM_P12V_AUX_SW_EN_DLY )
);

SignalValidationDelay#
(
    
    .VALUE      ( 1'b0 ),
    .TOTAL_BITS ( 3'd4 ),
    .POL        ( 1'b0 )
)mFM_P5V_EN_DLY
(
    .iClk       ( iClk ),
    .iRst       ( ~iRst_n ),
    .iCE        ( i10uSCE ),
    .ivMaxCnt   ( 4'd1 ),
    .iStart     ( PWRGD_PCH_PWROK ),
    .oDone      ( wFM_P5V_EN_DLY )
);

//% 15ms Delay for eADR
SignalValidationDelay#
(
    .VALUE                  ( 1'b0 ),
    .TOTAL_BITS             ( 4'd11 ),
    .POL                    ( 1'b0 )
)mPSPwrgdDlyUp          
(           
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iCE                    ( i10uSCE ),
    .ivMaxCnt               ( 11'd1500 ),        //15ms Hold. 1500 units
    .iStart                 ( PWRGD_PS_PWROK ),
    .oDone                  ( wPsPwrok_dly_15ms_n )
);

//% 600us Delay for Legacy ADR
SignalValidationDelay#
(
    .VALUE                  ( 1'b0 ),
    .TOTAL_BITS             ( 4'd8 ),
    .POL                    ( 1'b0 )
) mPSPwrgdDlyDwn          
(           
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iCE                    ( i10uSCE ),
    .ivMaxCnt               ( 8'd60 ),          //600us Hold. 60 units
    .iStart                 ( PWRGD_PS_PWROK ),
    .oDone                  ( wPsPwrok_dly_600us_n )
);

endmodule
