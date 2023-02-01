/////////////////////////////////////////////////////////////////////////////////////////////////
// Project: Archer City RP Board, Main FPGA
// Description: This module is intended for Co-Simulation with Memory Controller from CPU
//              (SPR) and the DIMM controller (Crow-Valley, from Crow-Pass)
//              Integrates both, the ddr5_pwrgdlogic AND the ADR logic
//              A few configuration inputs are being tied to proper value for SPR usage case
//              iAdr_Release for ADR logic has been tied to 0 to consider it irrelevant
//              at least for initial simulation scenarios
//
// Designer :   Alejandro Larios
// email    :   a.larios@intel.com
// date     :   May-05-2020
//
/////////////////////////////////////////////////////////////////////////////////////////////////


module ddr5_top
  #(parameter MC_SIZE = 4)
  (
   input                iClk, //input clock signal @2MHz
   input                iRst_n, //Reset signal active low
   input                iPsuPwrOk, //Power Supply Unit PWR OK indication
   input                iSlpS5Id, //SLP_S5 indication (active high)
   input                iFmSlp3Pld_n, //SLP_S3_N signal from PCH
   input                iMemPwrGdFail, //asserted if the DRAMPWRGD_DDRIO VR fails...

   input                iAdrEna, //to enable ADR logic
   input                iFmAdrComplete, //ADR COMPLETE signal from PCH

   input [MC_SIZE-1:0]  iDramPwrgdDDRIO, //MEMORY VR PWRGD (from board) D for valid DIMM FAIL detection, input from MC VR
   input [MC_SIZE-1:0]  iMcRst_n, //Reset from memory controller
   inout [MC_SIZE-1:0]  ioPwrGdFailChDimmCpu, //PWRGD_FAIL bidirectional signal
 
   output [MC_SIZE-1:0] oDimmMemFlt, //MEM Fault 
   output [MC_SIZE-1:0] oDramPwrOk, //DRAM PWR OK
   output [MC_SIZE-1:0] oFpgaDimmRst_n, //Reset to DIMMs

   output               oFmAdrTrigger_n, //ADR_TRIGGER_N active low signal to PCH (when a surprise pwr-dwn condition is detected)
   output               oFmAdrAck_n      //ADR ACKNOWLEDGE active low siganl to PCH (after receive ADR COMPLETION, timer is run thru expiration before asserting ADR ACK to allow DIMMs to receive the SAVE command
   
   );

   wire   wAdrSel;
   wire   wADRDimmRst_n;
   wire   wPwrGdFailCpuAB;
   wire   wAdrEvent;
        

   

   ddr5_pwrgd_logic_modular 
     #(.MC_SIZE(MC_SIZE))
   ddr5_module
     (
      .iClk(iClk),                                       //Clock input.
      .iRst_n(iRst_n),                                   //Reset enable on high.
      .iFM_INTR_PRSNT(1'b0),                             //Detect Interposer 
      .iINTR_SKU(1'b0),                                  //Detect Interposer SKU (1 = CLX or 0 = BRS)
      .iSlpS5Id(iSlpS5Id),                               //Sleep S5 Indication (active high)
      .iPWRGD_PS_PWROK(iPsuPwrOk),                       //PWRGD with generate by Power Supply.
      .iAdrSel(wAdrSel),                                 //selector for ddr5 logic to know if drive PWRGDFAIL & DIMM_RST from internal or ADR logic
      .iAdrEvent(wAdrEvent),                             //let the FSM know an ADR event happened (PWRGDFAIL going low is not an issue here)
      .iAdrDimmRst_n(wADRDimmRst_n),                     //DIMM_RST_N signal from ADR logic
      .iMemPwrGdFail(iMemPwrGdFail),                     //in case any of the DRAMPWRGD_DDRIO VRs fail...
      
      .iPWRGD_DRAMPWRGD_DDRIO(iDramPwrgdDDRIO),          //DRAMPWRGD for valid DIMM FAIL detection, input from MC VR
      .iMC_RST_N(iMcRst_n),                              //Reset from memory controller
      .iADR_PWRGDFAIL({MC_SIZE{wPwrGdFailCpuAB}}),        //ADR controls pwrgd/fail output
      .ioPWRGD_FAIL_CH_DIMM_CPU(ioPwrGdFailChDimmCpu),   //PWRGD_FAIL bidirectional signal
      
      .oDIMM_MEM_FLT(oDimmMemFlt),                       //MEM Fault 
      .oPWRGD_DRAMPWRGD_OK(oDramPwrOk),                  //DRAM PWR OK
      .oFPGA_DIMM_RST_N(oFpgaDimmRst_n)                  //Reset to DIMMs
      );


   adr_fub adr
     (
      .iClk(iClk),                                       //input clock signal (@2MHz)
      .iRst_n(iRst_n),                                   //input reset signal (active low)
      .iAdrEna(iAdrEna),                                 //ADR enable (to enable FSM execution from Master sequencer)
      .iPwrGdPSU(iPsuPwrOk),                             //PWROK from the Power Supply, used to detect surprise power-down condition
      .iFmSlp3Pld_n(iFmSlp3Pld_n),                       //s3 indication (from PCH), used to detect surprise power-down condition

      .iFmAdrExtTrigger_n(1'b1),                         //External trigger (active low) for ADR. Normally comes from a jumper in the board (for now, tied to 1)
      
      .iFmAdrComplete(iFmAdrComplete),                   //comes from PCH and indicates the ADR flow completion from PCH perspective, when triggered by board (FPGA)
      
      .iAdrRelease(1'b0),                                //comes from master, indicates when to release the control of RST and PWRGDFAIL signals
      .iAdrEmulated(1'b0),                               //this indicates if we want the LEGACY ADR flow or the special EMULATED ADR for RDIMMs
      
      .oFmAdrTrigger_n(oFmAdrTrigger_n),                 //goes to PCH to signal the need of ADR as a surprise power down condition was detected
      .oFmAdrAck_n(oFmAdrAck_n),                         //fpga uses this signal to acknoledge the PCH the reception of completion signal. FPGA will assert it 26 usec after
                                                         //asserted pwrgd_fail_cpu signals, to allow NVDIMMs to move data from volatile to non-volatile memory
      
      .oPwrGdFailCpuAB(wPwrGdFailCpuAB),                 //power good/fail# signal for DDR5 DIMMs. Used to indicate the DIMMs 
      .oPwrGdFailCpuCD(),                                // to move data from volatil to non-volatil memory
      .oPwrGdFailCpuEF(),
      .oPwrGdFailCpuGH(),
      
      .oAdrSel(wAdrSel),                                 //indicates when ddr5 memory module should select PWRGDFAIL and DRAM_RST_N from ADR logic instead
      .oAdrEvent(wAdrEvent),                             //signals when an ADR event happened
      .oDimmRst_n(wADRDimmRst_n)                         //this is to get control of the DIMM_RST signal that goes to the DIMMs when ADR_COMPLETE is asserted (1 signal for all DIMMs)
   
   
 );

endmodule // ddr5_top
