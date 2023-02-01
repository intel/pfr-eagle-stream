//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>VR Bypass Unit Block</b> 
    \details    
 
    \file       vr_bypass.v
    \author     amr/a.larios@intel.com
    \date       Aug 19, 2019
    \brief      $RCSfile: vr_bypass.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                
                <b>Project:</b> Archer City (EagleStream)\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   
                <b>Block Diagram:</b>
 
    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/


module vr_bypass
  (
    input iClk,                                      //input clock signal (@2MHz)
    input iRst_n,                                    //input reset signal (active low)

                                                     //these 3 are used as protection to avoid override if CPUs or PCH are present in board
    input iCpu0SktOcc_n,                             //Cpu0 socket occupy (active low)
    input iCpu1SktOcc_n,                             //Cpu1 socket occupy (active low)
    input iPchPrsnt_n,                               //PCH Present signal (active low)

    input iForcePwrOn,                               //taken from jumper J1C1 (FM_FORCE_PWRON_LVC3_R) active HIGH : 1-2 LOW (default), 2-3 HIGH


   
    input iP2v5BmcAuxPwrGdBP,                        //PWRGD signals of BMC related VRs
    input iP1v2BmcAuxPwrGdBP,
    input iP1v0BmcAuxPwrGdBP,

    input iP1v8PchAuxPwrGdBP,                        //PWRGD signals of PCH related VRs
    input iP1v05PchAuxPwrGdBP,
    input iPvnnPchAuxPwrGdBP,
   
    input iPsuPwrOKBP,
    input iP3v3MainPwrGdBP,
   
    input iPvppHbmCpu0PwrGdBP,                       //PWRGD signals of CPU0 related VRs
    input iPvccfaEhvCpu0PwrGdBP,
    input iPvccfaEhvfivraCpu0PwrGdBP,
    input iPvccinfaonCpu0PwrGdBP,
    input iPvnnMainCpu0PwrGdBP,
    input iPvccdHvCpu0PwrGdBP,                       //PWRGD signals of MEM CPU0 related VRs
    input iPvccinCpu0PwrGdBP,

    input iPvppHbmCpu1PwrGdBP,                       //PWRGD signals of CPU1 related VRs
    input iPvccfaEhvCpu1PwrGdBP,
    input iPvccfaEhvfivraCpu1PwrGdBP,
    input iPvccinfaonCpu1PwrGdBP,
    input iPvnnMainCpu1PwrGdBP,
    input iPvccdHvCpu1PwrGdBP,                       //PWRGD signals of CPU1 related VRs
    input iPvccinCpu1PwrGdBP,


    output [4:0] oLedStatus,                           //ByPass FSM states to LEDs for status
   
    output reg oP2v5BmcAuxEnaBP,                         //BMC related VR enables
    output reg oP1v2BmcAuxEnaBP,
    output reg oP1v0BmcAuxEnaBP,

                                                     //PCH related VR enables
    output reg oP1v8PchAuxEnaBP,
    output reg oP1v05PchAuxEnaBP,
    output reg oPvnnPchAuxEnaBP,

    output reg  oFmPldClk25MEna,                      //25MHz clock enable to PCH
    output reg  oFmPldClksDevEna,                     //CKMNG enable for 100MHz clocks to CPUs, NVMEs, etc

                                                     //PSU related enables 
    output reg oPsEnaBP,
    output reg oP5vEnaBP,

                                                     //S3 SW related enables
    output reg oAuxSwEnaBP,                              //select source for 12V AUX from STBY or MAIN
    output reg oSxSwP12vStbyEnaBP,                       //Switch ctrl to select 12V source for MEM VRs
    output reg oSxSwP12vEnaBP,


                                                     //CPU0 related VR enables   
    output reg oPvppHbmCpu0EnaBP,
    output reg oPvccfaEhvCpu0EnaBP,
    output reg oPvccfaEhvfivraCpu0EnaBP,
    output reg oPvccinfaonCpu0EnaBP,
    output reg oPvnnMainCpu0EnaBP,

                                                     //MEM CPU0 related enables
    output reg oPvccdHvCpu0EnaBP,
    output reg oPvccinCpu0EnaBP,
   
                                                     //CPU1 related VR enables
    output reg oPvppHbmCpu1EnaBP,
    output reg oPvccfaEhvCpu1EnaBP,
    output reg oPvccfaEhvfivraCpu1EnaBP,
    output reg oPvccinfaonCpu1EnaBP,
    output reg oPvnnMainCpu1EnaBP,

                                                     //MEM CPU1 related enables
    output reg oPvccdHvCpu1EnaBP,
    output reg oPvccinCpu1EnaBP
   );


   //////////////////////////////////
   //local params declarations
   //////////////////////////////////////////////////////////////////
   
   localparam INIT = 0;
   localparam BMC2V5 = 1;
   localparam BMC1V0 = 2;
   localparam BMC1V2 = 3;
   localparam PCH1V8 = 4;
   localparam PCHVNN = 5;
   localparam PCH1V05 = 6;
   localparam PSUENA = 7;
   localparam S3_SW = 8;
   localparam P3V3 = 9;
   localparam CPU0_PVPPHBM = 10;
   localparam CPU0_PVCCFAEHV = 11;
   localparam CPU0_PVCCFAEHVFIVRA = 12;
   localparam CPU0_PVCCINFAON = 13;
   localparam CPU0_PVNN = 14;
   localparam CPU1_PVPPHBM = 15;
   localparam CPU1_PVCCFAEHV = 16;
   localparam CPU1_PVCCFAEHVFIVRA = 17;
   localparam CPU1_PVCCINFAON = 18;
   localparam CPU1_PVNN = 19;
   localparam CPU0_MEM_PVCCDHV = 20;
   localparam CPU0_MEM_PVCCIN = 21;
   localparam CPU1_MEM_PVCCDHV = 22;
   localparam CPU1_MEM_PVCCIN = 23;
   localparam PLATFORM_ON = 24;
   
   
   //////////////////////////////////
   //internal signals
   //////////////////////////////////////////////////////////////////
   
   reg [4:0] rVrBypassFsm;                        //FSM to turn on all VRs in sequence
   reg       rVrTimerStart;                       //VR timer start signal (will define timer in the module instantiation)
   
   wire      wVrTimerDone;                        //VR timer expired

   wire      wBypassEna;                          //check that neither CPU0/CPU1, or PCH are present in platform AND Force Power ON jumper is at 2-3 (HIGH) to do Force Pwr VRs on (BYPASS)


   assign    wBypassEna = iForcePwrOn; //iCpu0SktOcc_n && iCpu1SktOcc_n && iPchPrsnt_n && iForcePwrOn;
   

   //////////////////////////////////
   //sub modules instantiation
   //////////////////////////////////////////////////////////////////
   
   //1-sec Timer: Started when enabling each VR to wait 1 sec for VR
   //             to turn on, or move to next VR
   //////////////////////////////////////////////////////////////////
   
   delay #(.COUNT(2000000)) 
     Timer20us(
               .iClk(iClk),
               .iRst(iRst_n),
               .iStart(rVrTimerStart),
               .iClrCnt(1'b0),
               .oDone(wVrTimerDone)
               );

   

   //////////////////////////////////
   //VR BYPASS FSM
   //////////////////////////////////////////////////////////////////
   
   always@(posedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)                         //syncrhonous reset
          begin
             //BMC related VR enables
             oP2v5BmcAuxEnaBP <= 1'b0;                         
             oP1v2BmcAuxEnaBP <= 1'b0;
             oP1v0BmcAuxEnaBP <= 1'b0;
                    
             //PCH related VR enables
             oP1v8PchAuxEnaBP <= 1'b0;
             oP1v05PchAuxEnaBP <= 1'b0;
             oPvnnPchAuxEnaBP <= 1'b0;

             //25MHz Clock Enabe to PCH
             oFmPldClk25MEna <= 1'b1;           //handling  negative-logic to test a rework in the board (need to go back to positive-logic once fixed)

             //CKMNG enable for 100MHz clocks
             oFmPldClksDevEna <= 1'b0;
                    
             //PSU related enables 
             oPsEnaBP <= 1'b0;
             oP5vEnaBP <= 1'b0;
                    
             //S3 SW related enables
             oAuxSwEnaBP <= 1'b0;                              //select source for 12V AUX from STBY or MAIN
             oSxSwP12vStbyEnaBP <= 1'b0;                       //Switch ctrl to select 12V source for MEM VRs
             oSxSwP12vEnaBP <= 1'b0;
                    
                    
             //CPU0 related VR enables   
             oPvppHbmCpu0EnaBP <= 1'b0;
             oPvccfaEhvCpu0EnaBP <= 1'b0;
             oPvccfaEhvfivraCpu0EnaBP <= 1'b0;
             oPvccinfaonCpu0EnaBP <= 1'b0;
             oPvnnMainCpu0EnaBP <= 1'b0;
                    
             //MEM CPU0 related enables
             oPvccdHvCpu0EnaBP <= 1'b0;
             oPvccinCpu0EnaBP <= 1'b0;
                    
             //CPU1 related VR enables
             oPvppHbmCpu1EnaBP <= 1'b0;
             oPvccfaEhvCpu1EnaBP <= 1'b0;
             oPvccfaEhvfivraCpu1EnaBP <= 1'b0;
             oPvccinfaonCpu1EnaBP <= 1'b0;
             oPvnnMainCpu1EnaBP <= 1'b0;
                    
             //MEM CPU1 related enables
             oPvccdHvCpu1EnaBP <= 1'b0;
             oPvccinCpu1EnaBP<= 1'b0;

             rVrBypassFsm <= INIT;
             rVrTimerStart <= 1'b0;
             
          end // if (!iRst_n)
        else //iClk rising edge
          begin
             if (wBypassEna)
               begin
                  case(rVrBypassFsm)
                    default: begin                        //INIT
                       rVrBypassFsm <= BMC2V5;
                       oP2v5BmcAuxEnaBP <= 1'b1;          //enable next VR
                       rVrTimerStart <= 1'b1;             //start timer
                    end
                    BMC2V5: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP2v5BmcAuxPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= BMC1V0;
                            oP1v0BmcAuxEnaBP <= 1'b1;     //enable next VR                            
                            rVrTimerStart <= 1'b0;        //clear timer
                         end
                       
                    end // case: BMC2V5
                    
                    
                    
                    BMC1V0: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP1v0BmcAuxPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= BMC1V2;
                            oP1v2BmcAuxEnaBP <= 1'b1;     //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer
                         end
                    end // case: BMC1V0


                    BMC1V2: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP1v2BmcAuxPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= PCH1V8;                            
                            oP1v8PchAuxEnaBP <= 1'b1;     //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer
                         end
                    end // case: BMC1V2
                    
                    
                    PCH1V8: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP1v8PchAuxPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= PCHVNN;
                            oPvnnPchAuxEnaBP <= 1'b1;     //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer
                         end
                    end // case: PCH1V8

                    PCHVNN: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iPvnnPchAuxPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= PCH1V05;
                            oP1v05PchAuxEnaBP <= 1'b1;     //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer
                         end
                    end // case: PCHVNN
                    
                    PCH1V05: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP1v05PchAuxPwrGdBP || wVrTimerDone)
                         begin

                            rVrBypassFsm <= PSUENA;
                            oPsEnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer

                            oFmPldClk25MEna <= 1'b0;      //Enable 25MHz clk (inverted logic, check reset comment about it)
                            
                            oSxSwP12vStbyEnaBP <= 1'b1;
                            oSxSwP12vEnaBP <= 1'b0;
                         end
                    end // case: PCH1V05
                    
                    
                    
                    PSUENA: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iPsuPwrOKBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= S3_SW;
                            oAuxSwEnaBP <= 1'b1;          //switching to 12V Main (assuming it PSU worked just fine)
                            oSxSwP12vStbyEnaBP <= 1'b1;   //starting switching from stby to main on mem 12v source
                            oSxSwP12vEnaBP <= 1'b1;
                            
                         end
                    end // case: PSUENA
                    
                    
                    S3_SW:begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if (wVrTimerDone)
                         begin
                            oP5vEnaBP <= 1'b1;            //enable next VR
                            rVrTimerStart <= 1'b0;        //clear timer
                            oSxSwP12vStbyEnaBP <= 1'b0;   //finishing switching from stby to main in 12v mem source
                            oSxSwP12vEnaBP <= 1'b1;
                            rVrBypassFsm <= P3V3;
                         end
                    end // case: S3_SW
                    
                    P3V3: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iP3v3MainPwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= CPU0_PVPPHBM;
                            oPvppHbmCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer

                            oFmPldClksDevEna <= 1'b1;
                            
                         end
                    end // case: P3V3
                    
                    CPU0_PVPPHBM: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iPvppHbmCpu0PwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= CPU0_PVCCFAEHV;
                            oPvccfaEhvCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         end
                    end // case: CPU0_PVPPHBM
                    
                    CPU0_PVCCFAEHV: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       if(iPvccfaEhvCpu0PwrGdBP || wVrTimerDone)
                         begin
                            rVrBypassFsm <= CPU0_PVCCFAEHVFIVRA;
                            oPvccfaEhvfivraCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         end
                    end // case: CPU0_PVCCFAEHV
                    
                    CPU0_PVCCFAEHVFIVRA: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccfaEhvfivraCpu0PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU0_PVCCINFAON;
                            oPvccinfaonCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU0_PVCCFAEHVFIVRA
                    
                    CPU0_PVCCINFAON: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccinfaonCpu0PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU0_PVNN;
                            oPvnnMainCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU0_PVCCINFAON
                    
                    CPU0_PVNN: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvnnMainCpu0PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_PVPPHBM;
                            oPvppHbmCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU0_PVNN
                    
                    CPU1_PVPPHBM: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvppHbmCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_PVCCFAEHV;
                            oPvccfaEhvCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_PVPPHBM
                    
                    CPU1_PVCCFAEHV: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccfaEhvCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_PVCCFAEHVFIVRA;
                            oPvccfaEhvfivraCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_PVCCFAEHV
                    
                    CPU1_PVCCFAEHVFIVRA: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccfaEhvfivraCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_PVCCINFAON;
                            oPvccinfaonCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_PVCCFAEHVFIVRA
                    
                    CPU1_PVCCINFAON: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccinfaonCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_PVNN;
                            oPvnnMainCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_PVCCINFAON
                    
                    CPU1_PVNN: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvnnMainCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU0_MEM_PVCCDHV;
                            oPvccdHvCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_PVNN
                    
                    CPU0_MEM_PVCCDHV: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccdHvCpu0PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU0_MEM_PVCCIN;
                            oPvccinCpu0EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU0_MEM_PVCCDHV
                    
                    CPU0_MEM_PVCCIN: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccinCpu0PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_MEM_PVCCDHV;
                            oPvccdHvCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU0_MEM_PVCCIN
                    
                    CPU1_MEM_PVCCDHV: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccdHvCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= CPU1_MEM_PVCCIN;
                            oPvccinCpu1EnaBP <= 1'b1;             //enable next VR
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_MEM_PVCCDHV
                    
                    CPU1_MEM_PVCCIN: begin
                       rVrTimerStart <= 1'b1;            //keep timer running until VR on or timer expires
                       //if(iPvccinCpu1PwrGdBP || wVrTimerDone)
                         //begin
                            rVrBypassFsm <= PLATFORM_ON;
                            rVrTimerStart <= 1'b0;                 //clear timer
                         //end
                    end // case: CPU1_MEM_PVCCIN
                    
                    PLATFORM_ON: begin
                       rVrBypassFsm <= PLATFORM_ON;
                    end
                    
                  endcase // case (rVrBypassFsm)
               end // if (wBypassEna)
             else
               begin
                  //BMC related VR enables
                  oP2v5BmcAuxEnaBP <= 1'b0;                         
                  oP1v2BmcAuxEnaBP <= 1'b0;
                  oP1v0BmcAuxEnaBP <= 1'b0;
                  
                  //PCH related VR enables
                  oP1v8PchAuxEnaBP <= 1'b0;
                  oP1v05PchAuxEnaBP <= 1'b0;
                  oPvnnPchAuxEnaBP <= 1'b0;
                  
                  //PSU related enables 
                  oPsEnaBP <= 1'b0;
                  oP5vEnaBP <= 1'b0;
                    
                  //S3 SW related enables
                  oAuxSwEnaBP <= 1'b0;                              //select source for 12V AUX from STBY or MAIN
                  oSxSwP12vStbyEnaBP <= 1'b0;                       //Switch ctrl to select 12V source for MEM VRs
                  oSxSwP12vEnaBP <= 1'b0;
                  
                  
                  //CPU0 related VR enables   
                  oPvppHbmCpu0EnaBP <= 1'b0;
                  oPvccfaEhvCpu0EnaBP <= 1'b0;
                  oPvccfaEhvfivraCpu0EnaBP <= 1'b0;
                  oPvccinfaonCpu0EnaBP <= 1'b0;
                  oPvnnMainCpu0EnaBP <= 1'b0;
                  
                  //MEM CPU0 related enables
                  oPvccdHvCpu0EnaBP <= 1'b0;
                  oPvccinCpu0EnaBP <= 1'b0;
                  
                  //CPU1 related VR enables
                  oPvppHbmCpu1EnaBP <= 1'b0;
                  oPvccfaEhvCpu1EnaBP <= 1'b0;
                  oPvccfaEhvfivraCpu1EnaBP <= 1'b0;
                  oPvccinfaonCpu1EnaBP <= 1'b0;
                  oPvnnMainCpu1EnaBP <= 1'b0;
                  
                  //MEM CPU1 related enables
                  oPvccdHvCpu1EnaBP <= 1'b0;
                  oPvccinCpu1EnaBP<= 1'b0;
                  
                  rVrBypassFsm <= INIT;
                  rVrTimerStart <= 1'b0;
               end // else: !if(wBypassEna)

          end // else: !if(!iRst_n)
        
     end // always@ (posedge iClk)



   //output assignments
   assign oLedStatus = rVrBypassFsm;
   
   
endmodule // vr_bypass




