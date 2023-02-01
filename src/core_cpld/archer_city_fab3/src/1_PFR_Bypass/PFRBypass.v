module PFRBypass 
  (
   input  iClk,          //2MHz
   input  iclk_Bypass,   //25MHz
   input  iclk_Bypass2,  //50MHz
   input  iclk_Bypass3,  //1MHz
   input  iRst_n,
   
   input  iRsmRst_n, //alarios: added to filter SPI_PCH_TPM_CS_N before RsmRst is de-asserted
   
   //PFR Dedicated Signals
   input  FM_PFR_GLB_ACK,
   input  FM_AC_MOD_PRSTN,
   output FM_PFR_LOCAL_SYNC,
   input  FM_IS_LEGACY_NODE,
   
   
   output FAN_BMC_PWM_R,
   output FM_PFR_DSW_PWROK_N,
   input  FM_ME_AUTHN_FAIL,
   input  FM_ME_BT_DONE,
   input  FM_PFR_TM1_HOLD_N,
   input  FM_PFR_POSTCODE_SEL_N,
   input  FM_PFR_FORCE_RECOVERY_N,
   //output	FM_PFR_MUX_OE_CTL_PLD,
   output RST_PFR_EXTRST_R_N,
   output RST_PFR_OVR_RTC_R,
   output RST_PFR_OVR_SRTC_R,
   output FM_PFR_ON_R,
   output FM_PFR_SLP_SUS_EN,
   output FP_ID_LED_PFR_N,
   input  FP_ID_LED_N,
   output FP_LED_STATUS_AMBER_PFR_N,
   input  FP_LED_STATUS_AMBER_N,
   output FP_LED_STATUS_GREEN_PFR_N,
   input  FP_LED_STATUS_GREEN_N,
   //SPI BYPASS
   output FM_SPI_PFR_PCH_MASTER_SEL_R,
   output FM_SPI_PFR_BMC_BT_MASTER_SEL_R,
   output RST_SPI_PFR_BMC_BOOT_N,
   output RST_SPI_PFR_PCH_N,
   input  SPI_BMC_BOOT_CS_N,
   output SPI_PFR_BMC_BT_SECURE_CS_R_N,
   input  SPI_PCH_PFR_CS0_N,
   output SPI_PFR_PCH_SECURE_CS0_R_N,
   input  SPI_PCH_CS1_N,
   output SPI_PFR_PCH_SECURE_CS1_N,
   input  SPI_BMC_BT_MUXED_MON_CLK,
   input  SPI_BMC_BT_MUXED_MON_MISO,
   input  SPI_BMC_BT_MUXED_MON_MOSI,
   input  SPI_BMC_BT_MUXED_MON_IO2,
   input  SPI_BMC_BT_MUXED_MON_IO3,
   input  SPI_PCH_MUXED_MON_CLK, //alarios: BEGIN
   input  SPI_PCH_MUXED_MON_IO0, //            changed from INOUT to INPUT as they are not really INOUT
   input  SPI_PCH_MUXED_MON_IO1, //
   input  SPI_PCH_MUXED_MON_IO2, //
   input  SPI_PCH_MUXED_MON_IO3, //alarios: END
   inout  SPI_PFR_BMC_FLASH1_BT_CLK,
   inout  SPI_PFR_BMC_FLASH1_BT_MOSI,
   inout  SPI_PFR_BMC_FLASH1_BT_MISO,
   inout  SPI_PFR_BMC_BOOT_R_IO2,
   inout  SPI_PFR_BMC_BOOT_R_IO3,
   inout  SPI_PFR_PCH_R_CLK,
   
   //inout  SPI_PFR_PCH_R_IO0,    //alarios: changed to be inputs for testing
   //inout  SPI_PFR_PCH_R_IO1,
   //inout  SPI_PFR_PCH_R_IO2,
   //inout  SPI_PFR_PCH_R_IO3,
   
   input  SPI_PFR_PCH_R_IO0, //alarios: changed to be inputs for testing
   input  SPI_PFR_PCH_R_IO1,
   input  SPI_PFR_PCH_R_IO2,
   input  SPI_PFR_PCH_R_IO3,
   
   //SMBus
   inout  SMB_BMC_HSBP_STBY_LVC3_SCL,
   inout  SMB_BMC_HSBP_STBY_LVC3_SDA,
   inout  SMB_PFR_HSBP_STBY_LVC3_SCL,
   inout  SMB_PFR_HSBP_STBY_LVC3_SDA,
   inout  SMB_PFR_PMB1_STBY_LVC3_SCL,
   inout  SMB_PFR_PMB1_STBY_LVC3_SDA,
   inout  SMB_PMBUS_SML1_STBY_LVC3_SDA, 
   inout  SMB_PMBUS_SML1_STBY_LVC3_SCL,
   
   inout  SMB_PCH_PMBUS2_STBY_LVC3_SCL,
   inout  SMB_PCH_PMBUS2_STBY_LVC3_SDA,
   inout  SMB_PFR_PMBUS2_STBY_LVC3_SCL,
   inout  SMB_PFR_PMBUS2_STBY_LVC3_SDA,
   
   output SMB_PFR_RFID_STBY_LVC3_SCL,
   inout  SMB_PFR_RFID_STBY_LVC3_SDA,
   //Deltas for PFR
   inout  SMB_S3M_CPU0_SDA_LVC1, //Used for BRS interposer
   input  SMB_S3M_CPU0_SCL_LVC1, //Used for BRS interposer
   output H_S3M_CPU0_SMBUS_ALERT_LVC1_R_N,
   inout  SMB_S3M_CPU1_SDA_LVC1, //Used for BRS interposer
   input  SMB_S3M_CPU1_SCL_LVC1, //Used for BRS interposer
   output H_S3M_CPU1_SMBUS_ALERT_LVC1_R_N,
   inout  SMB_PCIE_STBY_LVC3_B_SDA,
   output SMB_PCIE_STBY_LVC3_B_SCL,
   input  SPI_PCH_TPM_CS_N,
   output SPI_PFR_TPM_CS_R2_N,
   output FM_PFR_RNDGEN_AUX
   );

   ////////////////////////////////////////////////////////////
   /////local parameter declarations
   ////////////////////////////////////
   
   localparam Z = 1'bz;
   localparam HIGH = 1'b1;
   localparam LOW = 1'b0;

   ////////////////////////////////////////////////////////////
   /////Smbus bypass instantiations
   ///////////////////////////////////
   
   //SMB SDA Bypass
   BidirBuffBus #(.BUS_SIZE(2))           //ALARIOS (changed from 6 to 4)
   SMB_SDA_bypass_inst
     (
      .iClk(iclk_Bypass),                //25MHz
      .iRst_n(iRst_n),
      .IOAbus({
	           SMB_BMC_HSBP_STBY_LVC3_SCL,
	           SMB_BMC_HSBP_STBY_LVC3_SDA//,
	           //SMB_PMBUS_SML1_STBY_LVC3_SCL,     //ALARIOS
	           //SMB_PMBUS_SML1_STBY_LVC3_SDA,
	           //SMB_PCH_PMBUS2_STBY_LVC3_SCL,
	           //SMB_PCH_PMBUS2_STBY_LVC3_SDA
	           }),
      .IOBbus({
	           SMB_PFR_HSBP_STBY_LVC3_SCL,
	           SMB_PFR_HSBP_STBY_LVC3_SDA//,
	           //SMB_PFR_PMB1_STBY_LVC3_SCL,       //ALARIOS
	           //SMB_PFR_PMB1_STBY_LVC3_SDA,
	           //SMB_PFR_PMBUS2_STBY_LVC3_SCL,
	           //SMB_PFR_PMBUS2_STBY_LVC3_SDA
	           })
      );

   BidirBuffBus #(.BUS_SIZE(1))
   SMB_SCL_bypass_inst50M
     (
      .iClk(iclk_Bypass2),     //50MHz clk
      .iRst_n(iRst_n),
      .IOAbus(SMB_PCH_PMBUS2_STBY_LVC3_SCL),
              // SMB_PCH_PMBUS2_STBY_LVC3_SDA),
      .IOBbus(SMB_PFR_PMBUS2_STBY_LVC3_SCL)
               //SMB_PFR_PMBUS2_STBY_LVC3_SDA)
      );


   BidirBuffBus #(.BUS_SIZE(1))
   SMB_SDA_bypass_inst1M
     (
      .iClk(iclk_Bypass3),     //1MHz clk
      .iRst_n(iRst_n),
      .IOAbus(//SMB_PCH_PMBUS2_STBY_LVC3_SCL),
              SMB_PCH_PMBUS2_STBY_LVC3_SDA),
      .IOBbus(//SMB_PFR_PMBUS2_STBY_LVC3_SCL)
              SMB_PFR_PMBUS2_STBY_LVC3_SDA)
      );
   
   
   

   BidirBuffBus #(.BUS_SIZE(3))    //ALARIOS: added to use a different clock signal for the PMBUS bypass (2MHz instead of the 25MHz used in other buses)
   SMB_SDA_bypass_inst2                //2MHz clk
     (
      .iClk(iClk),
      .iRst_n(iRst_n),
      .IOAbus({
	           SMB_PMBUS_SML1_STBY_LVC3_SCL,     //ALARIOS
	           SMB_PMBUS_SML1_STBY_LVC3_SDA,
               //SMB_PCH_PMBUS2_STBY_LVC3_SCL,
	           SMB_PCH_PMBUS2_STBY_LVC3_SDA
	           }),
      .IOBbus({
               
	           SMB_PFR_PMB1_STBY_LVC3_SCL,       //ALARIOS
	           SMB_PFR_PMB1_STBY_LVC3_SDA,
               //SMB_PFR_PMBUS2_STBY_LVC3_SCL,
	           SMB_PFR_PMBUS2_STBY_LVC3_SDA
	           })
      );

   //Direct bypass assigning outputs to their inputs
   assign	FP_ID_LED_PFR_N					= FP_ID_LED_N;
   assign	FP_LED_STATUS_AMBER_PFR_N		= FP_LED_STATUS_AMBER_N;
   assign	FP_LED_STATUS_GREEN_PFR_N		= FP_LED_STATUS_GREEN_N;
   
   assign	SPI_PFR_BMC_BT_SECURE_CS_R_N	= SPI_BMC_BOOT_CS_N;
   assign	SPI_PFR_PCH_SECURE_CS0_R_N		= SPI_PCH_PFR_CS0_N;
   assign	SPI_PFR_PCH_SECURE_CS1_N		= SPI_PCH_CS1_N;
   assign	SPI_PFR_BMC_FLASH1_BT_CLK		= SPI_BMC_BT_MUXED_MON_CLK;
   assign	SPI_PFR_PCH_R_CLK				= SPI_PCH_MUXED_MON_CLK;
   
   assign	SPI_PFR_TPM_CS_R2_N				= iRsmRst_n ? SPI_PCH_TPM_CS_N : 1'b1;
   //SMB clock bypass
   
   
   //Outputs High Z due to lack of knoledge of default state
   //eSPI bypassed by Muxes and Quick Switches
   //assign	SPI_BMC_BT_MUXED_MON_MISO		= Z;                //alarios: commented-out as they don't really need to be INOUTs
   //assign	SPI_BMC_BT_MUXED_MON_MOSI		= Z;                //they become now inputs only
   //assign	SPI_BMC_BT_MUXED_MON_IO2		= Z;
   //assign	SPI_BMC_BT_MUXED_MON_IO3		= Z;
   assign	SPI_PFR_BMC_FLASH1_BT_MOSI		= Z;
   assign	SPI_PFR_BMC_FLASH1_BT_MISO		= Z;
   assign	SPI_PFR_BMC_BOOT_R_IO2			= Z;
   assign	SPI_PFR_BMC_BOOT_R_IO3			= Z;
   //assign  SPI_PCH_MUXED_MON_IO0			= Z;
   //assign  SPI_PCH_MUXED_MON_IO1			= Z;
   //assign  SPI_PCH_MUXED_MON_IO2			= Z;
   //assign  SPI_PCH_MUXED_MON_IO3			= Z;
   
   //assign  SPI_PFR_PCH_R_IO0				= Z;    //alarios:commented and changed to be inputs for now
   //assign  SPI_PFR_PCH_R_IO1				= Z;
   //assign  SPI_PFR_PCH_R_IO2				= Z;
   //assign  SPI_PFR_PCH_R_IO3				= Z;
   
   //PFR RFID SMB Master
   assign	SMB_PFR_RFID_STBY_LVC3_SCL		= Z;
   assign	SMB_PFR_RFID_STBY_LVC3_SDA		= Z;
   //PFR SM3 CPU0 SMB Slave
   assign	SMB_S3M_CPU0_SDA_LVC1			= Z;
   assign	H_S3M_CPU0_SMBUS_ALERT_LVC1_R_N	= Z;
   //PFR SM3 CPU1 SMB Slave
   assign	SMB_S3M_CPU1_SDA_LVC1			= Z;
   assign	H_S3M_CPU1_SMBUS_ALERT_LVC1_R_N	= Z;
   //PFR one of the SMB multimaster for PCIE
   assign	SMB_PCIE_STBY_LVC3_B_SCL		= Z;
   assign	SMB_PCIE_STBY_LVC3_B_SDA		= Z;
   
   //Misc Signals,
   assign	FM_PFR_RNDGEN_AUX				= Z;
   assign	FAN_BMC_PWM_R					= Z;
   assign	FM_PFR_DSW_PWROK_N				= Z;
   // FM_ME_AUTHN_FAIL,
   // FM_ME_BT_DONE,
   // FM_PFR_TM1_HOLD_N,
   // FM_PFR_POSTCODE_SEL_N,
   // FM_PFR_FORCE_RECOVERY_N,
   //assign	FM_PFR_MUX_OE_CTL_PLD			= 1'b1;    //alarios: changed from Z to HIGH to enable PCH SPI mux, with Z, it was disabled
   assign	RST_PFR_EXTRST_R_N				= Z;
   assign	RST_PFR_OVR_RTC_R				= Z;
   assign	RST_PFR_OVR_SRTC_R				= Z;
   assign	FM_PFR_ON_R						= Z;
   assign	FM_PFR_SLP_SUS_EN				= 1'b1;  //Z;  //alarios: 
   
   assign	FM_SPI_PFR_PCH_MASTER_SEL_R		= Z;
   assign	FM_SPI_PFR_BMC_BT_MASTER_SEL_R	= Z;
   assign	RST_SPI_PFR_BMC_BOOT_N			= Z;
   assign	RST_SPI_PFR_PCH_N				= Z;
   
   assign   FM_PFR_LOCAL_SYNC               = 1'b0;            //this is used for Modular design
   
   
endmodule // PFRBypass

