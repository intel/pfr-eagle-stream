//////////////////////////////////////////////////////////////////////////////////
// Description: The PSU, Main VR rails and S3 switch logic control logic 
//////////////////////////////////////////////////////////////////////////////////

module psu_s3_sw_ctrl
  (
   input            iClk,
   input            iRst_n,
   input            iEnaSw,                        //Sw enable to operate
   input            iEnaMain,                      //Main Rails (PSU, P5V & P3V enabled now)
   input            iSlpS3,
   input            iSlpS5,
   input            iDimm12VCpsS5_n,

   input            iPsPwrOk,
   input            iPwrGdP3V3,

   input            iAdrEna,

   input            iAcRemoval,    //helps pwrdwn DIMMs switch (if transitions from Main to STBY or not), instead of previosly named iAdrEvent
   //input            iAdrEvent,    //This is used for when J83 is populated (iDimm12VCpsS5_n == 0). On an ADR shutdown, system needs to jump to
                                  // S5 (not to transition 12V to the DIMM from Main to STBY to avoid overloading STBY)
   
   input            iFailId, //comes from master to signal a failure detected by FPGA logic on a different Sequencer



   output reg       oPsuEna,
   output reg       oP5VEna,
   output reg       oClkDevEna, //100MHz external clock gen enable (Si5332 or CK440)
   output reg       oFault, //to signal the master fub this sequencer has detected a failure
   output reg [2:0] oFaultCode, //to know what is the failure signaled by oFault
   output reg       oDone, //indicates when this sequencer is Done with its sequence

   output reg       oAuxSwEna,
   output reg       oS3SwP12vStbyEna,
   output reg       oS3SwP12vMainEna
   
   );
   
    

   parameter INIT = 0;
   parameter S5 = 1;
   parameter S5p = 2;
   parameter S3 = 3;
   parameter PSU = 4;
   parameter P5V = 5;
   parameter S0hold = 6;
   parameter S3hold = 7;
   parameter S0 = 8;
   parameter S0down = 9;
   //parameter ERROR = 10;


   reg [3:0]        rPsuSwFsm;         //FSM for PSU, MAIN VRs and S3 SW control  (PwrUp & PwrDwn)

   reg              rStartTimerT1;     //start timer to pwrup (transition from Stby to Main on 12V to DIMMs SW)
   reg              rStartTimerT2;     //start timer to pwrdwn (transition from Main to Stby on 12V to DIMMs SW)

   //reg              rFromPwrUp;
   reg              rAdrEna;          //to latch the AdrEna indication so it is kept fixed in a pwr dwn

   reg              rS3Flag;          //on the power up sequence, we need to know if we come from S3 or S5 to avoid 12V to dimms to be OFF when needs to be ON
   reg              rS5pFlag;         //on the power up sequence, we need to know if we come from S5p or S5 to avoid 12V to dimms to be ON when needs to be OFF
   

   wire             wDoneT1;          //T1 timer is done (transition from STBY to MAIN on S3 SW)
   wire             wDoneT2;          //T2 timer is done (transition from MAIN to STBY on S3 SW)
   


   //Instance of counter to generate a delay of 1ms for PwrUp (transition from STBY to MAIn for 12V Dimms SW)
   counterSW #(.UnitTime(1000))
   T1 (
       .iClk(iClk),
       .iRst_n(iRst_n),
       .enable(rStartTimerT1),
       .done(wDoneT1)
       );
   
   //Instance of counter to generate a delay of 200ms for PwrDwn (transition from Main to STBY for 12V Dimms SW)
   counterSW #(.UnitTime(400000))
   T2 (
       .iClk(iClk),
       .iRst_n(iRst_n),
       .enable(rStartTimerT2),
       .done(wDoneT2)
       );
   

   always @(posedge iClk, negedge iRst_n)
     begin
        if (~iRst_n)
          begin
             oPsuEna <= 1'b0;
             oP5VEna <= 1'b0;
             oClkDevEna <= 1'b0;
             oFault <= 1'b0;
             oFaultCode <= 3'b000;
             oDone <= 1'b0;

             rS3Flag <= 1'b0;
             rS5pFlag <= 1'b0;

             rStartTimerT1 = 1'b0;
             rStartTimerT2 = 1'b0;

             //rFromPwrUp <= 1'b0;
             oAuxSwEna <= 1'b0;
             oS3SwP12vStbyEna <= 1'b0;
             oS3SwP12vMainEna <= 1'b0;

             rAdrEna <= 1'b0;

             rPsuSwFsm <= INIT;
             
             
          end // if (~iRst_n)
        else                                    //no iRst_n low, so posedge of iClk
          begin
             case (rPsuSwFsm)

               default:  begin                 //Init state
                
                  oPsuEna <= 1'b0;
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  //rFromPwrUp <= 1'b0;
                  
                  oAuxSwEna <= 1'b0;
                  oS3SwP12vStbyEna <= 1'b0;
                  oS3SwP12vMainEna <= 1'b0;

                  rS3Flag <= 1'b0;
                  rS5pFlag <= 1'b0;

                  rAdrEna <= 1'b0;
                  
                  //if (iEnaSw && iDimm12VCpsS5_n)
                  if (iEnaSw)
                    rPsuSwFsm <= S5;
                  //else if (iEnaSw && !iDimm12VCpsS5_n)
                  //  rPsuSwFsm <= S5p;
                  else
                    rPsuSwFsm <= INIT;
               end // case: default
               
               S5: begin
                  
                  oPsuEna <= 1'b0;
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  rS3Flag <= 1'b0;
                  rS5pFlag <= 1'b0;
                  
                  oAuxSwEna <= 1'b0;
                  oS3SwP12vStbyEna <= 1'b0;
                  oS3SwP12vMainEna <= 1'b0;

                  rAdrEna <= 1'b0;
                  
                  if (~iSlpS5 && iEnaMain)
                    rPsuSwFsm <= PSU; //rPsuSwFsm <= S3;  //alarios 27-Nov-2020 won't pass thru S3, we go directly to PSU so 12V to DIMMs is not active until MAIN is ON
                  else
                    rPsuSwFsm <= S5;
                  
               end // case: S5


               S5p: begin
                  
                  oPsuEna <= 1'b0;
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  rS3Flag <= 1'b0;
                  rS5pFlag <= 1'b1;
                  oAuxSwEna <= 1'b0;
                  oS3SwP12vStbyEna <= 1'b1;
                  oS3SwP12vMainEna <= 1'b0;

                  rAdrEna <= 1'b0;
                  
                  if (~iSlpS5 && iEnaMain)
                    rPsuSwFsm <= PSU; //rPsuSwFsm <= S3;  //alarios 27-Nov-2020 won't pass thru S3, we go directly to PSU so 12V to DIMMs is not active until MAIN is ON
                  else
                    rPsuSwFsm <= S5p;
                  
               end // case: S5p
               
               S3: begin
                  
                  oPsuEna <= 1'b0;
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  rS3Flag <= 1'b1;                          //aserting flag so at pwr-up, we know we come from S3
                  
                  oAuxSwEna <= 1'b0;
                  oS3SwP12vStbyEna <= 1'b1;
                  oS3SwP12vMainEna <= 1'b0;
                  
                  if (~iSlpS5 && ~iSlpS3 && iEnaMain)             //sleeps de-asserted and Master enabling Main rails
                    begin
                       rPsuSwFsm <= PSU;
                       //rFromPwrUp <= 1'b1;
                    end
                  else if (iSlpS5 && iDimm12VCpsS5_n && ~iEnaMain)            //SplS5 asserted and system has to power down DIMMs 12V
                    begin
                       rPsuSwFsm <= S5;
                       //rFromPwrUp <= 1'b0;
                    end
                  else if (iSlpS5 && !iDimm12VCpsS5_n && ~iEnaMain)             //SlpS5 asserted and system has to keep 12V power to DIMMs
                    begin
                       rPsuSwFsm <= S5p;
                       //rFromPwrUp <= 1'b0;
                    end
                  else                                            //We Keep system in S3 until ready to move on any of prior conditions are met
                    begin
                       rPsuSwFsm <= S3;
                       //rFromPwrUp <= 1'b0;
                    end
                  
                  
               end // case: S3

               S3hold: begin

                  oPsuEna <= 1'b1;
                  oP5VEna <= 1'b0;  
                  oClkDevEna <= 1'b0;                             //P5V & P3V3 are ON, enabling clocks for SI5332 / CK440
                  oDone <= 1'b1;
                  rStartTimerT1 = 1'b0;                           //clear T1 timer
                  rStartTimerT2 = 1'b1;                           //clear T1 timer
                  
                  oAuxSwEna <= 1'b1;
                  oS3SwP12vStbyEna <= 1'b1;                       //Disable STBY to S3 SW 
                  oS3SwP12vMainEna <= 1'b1;                       //Keeping MAIN to S3 SW

                  rS3Flag <= 1'b1;                                //aserting flag so at pwr-up, we know we come from S3

                  if (iPsPwrOk == 1'b0 && rAdrEna == 1'b0)        //Psu PwrOk dropped unexpectedly and ADR is not enabled, then sense as failure
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b001;                      //fault code for PSU dropping PSU PWROK (when ADR not enabled)
                       //rPsuSwFsm <= ERROR;                      //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                  else if (~iEnaMain && iSlpS3 && wDoneT2)                                    //If going to S3 as well as enable from master is removed...
                    rPsuSwFsm <= S3;
                  
                  else if (!iEnaMain && iSlpS5 && iDimm12VCpsS5_n)                 //SplS5 asserted and system has to power down DIMMs 12V
                    rPsuSwFsm <= S5;
                  else if (!iEnaMain && iSlpS5 && !iDimm12VCpsS5_n && wDoneT2)       //SlpS5 asserted and system has to keep 12V power to DIMMs
                    rPsuSwFsm <= S5p;
                  else                                                              //We Keep system in S3 until ready to move on any of prior conditions are met
                    rPsuSwFsm <= S3hold;
                  

               end // case: S3hold
               
               

               PSU: begin

                  oPsuEna <= 1'b1;                               //Enable PSU (12V Main)
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  oAuxSwEna <= 1'b0;                                            //if coming from pwr up sequence, keep it in STBY
                  if ((!iDimm12VCpsS5_n && rS5pFlag)  || rS3Flag)               //if jumper set and we come from S5p, then we have 12V DIMMs alive since S5p, so we keep it, or if coming from S3 too
                    oS3SwP12vStbyEna <= 1'b1;                                   
                  else
                    oS3SwP12vStbyEna <= 1'b0;                                   //if coming from S5 with no 12V on DIMMs, we don't enable 12V to DIMMs from STBY
                  
                  oS3SwP12vMainEna <= 1'b0;

                  if (iEnaMain && iPsPwrOk)                       //master enabing Main rails and PsPwrOk received
                    rPsuSwFsm <= P5V;
                  else if (!iEnaMain && iSlpS5 && iDimm12VCpsS5_n)
                    rPsuSwFsm <= S5;
                  else if (!iEnaMain && iSlpS5 && !iDimm12VCpsS5_n && iAcRemoval)        //SlpS5 asserted and system has jumper to keep 12V power to DIMMs but there was an ADR event
                    rPsuSwFsm <= S5;
                  else if (!iEnaMain && iSlpS5 && !iDimm12VCpsS5_n && !iAcRemoval)       //SlpS5 asserted and system has to keep 12V power to DIMMs (no ADR event)
                    rPsuSwFsm <= S5p;
                  else if (!iEnaMain)                                                     //even if no SlpS5 is asserted, if the enable to this module is de-asserted, it needs to go to S5
                    rPsuSwFsm <= S5;
                  else                                            //We Keep system in S3 until ready to move on any of prior conditions are met
                    rPsuSwFsm <= PSU;
                  
               end // case: PSU

               P5V: begin
                  oPsuEna <= 1'b1;
                  oP5VEna <= 1'b1;                                //Enable P5V MAIN VR
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b0;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;
                  

                  //rFromPwrUp <= 1'b0;
                  oAuxSwEna <= 1'b1;

                  if (iPsPwrOk == 1'b0 && rAdrEna == 1'b0)        //Psu PwrOk dropped unexpectedly and ADR is not enabled, then sense as failure
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b001;                      //fault code for PSU dropping PSU PWROK (when ADR not enabled)
                       //rPsuSwFsm <= ERROR;                      //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                                  
                  if ((!iDimm12VCpsS5_n && rS5pFlag)|| rS3Flag)               //if jumper set and coming from S5p, or coming from S3, then we have 12V DIMMs alive since, so we keep it,
                    begin
                       //rStartTimerT1 = 1'b1;                                  //Start T1 Timer for PwrUp transition from STBY to MAIN
                       oS3SwP12vStbyEna <= 1'b1;                
                    end
                  else
                    begin
                       rStartTimerT1 = 1'b0;                                  //if coming from S5 with no 12V on DIMMs, we don't enable 12V to DIMMs from STBY
                       oS3SwP12vStbyEna <= 1'b0;
                    end

                  oS3SwP12vMainEna <= 1'b0;
                  //oS3SwP12vMainEna <= 1'b1;                      //as Main is ON, we enable 12V Main to DIMMs and start transition (if required)

                  rAdrEna <= iAdrEna;
                  
                  if (iEnaMain && iPwrGdP3V3)                     //master enabing Main rails and P3V Main PowerGood received
                    begin
                       if ((!iDimm12VCpsS5_n && rS5pFlag) || rS3Flag)            //we only need to go to S0hold if we come from S3 or 12VCpsS5 jumper (J83) is populated and come from S5p
                         rPsuSwFsm <= S0hold;
                       else
                         rPsuSwFsm <= S0;                         //otherwise, 12V to Dimms was already enabled from MAIN and no need to transition                        
                    end
                  else                                            //We Keep system in S3 until ready to move on any of prior conditions are met
                    rPsuSwFsm <= P5V;

               end // case: P5V

               S0hold: begin
                  
                  oPsuEna <= 1'b1;
                  oP5VEna <= 1'b1;  
                  oClkDevEna <= 1'b1;                             //P5V & P3V3 are ON, enabling clocks for SI5332 / CK440
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b1;
                  
                  rStartTimerT1 = 1'b1;                           //Start T1 Timer for PwrUp transition from STBY to MAIN
                  rStartTimerT2 = 1'b0;

                  //rFromPwrUp <= 1'b0;

                  rAdrEna <= iAdrEna;
                  
                  oAuxSwEna <= 1'b1;
                  oS3SwP12vStbyEna <= 1'b1;
                  oS3SwP12vMainEna <= 1'b1;

                  rS3Flag <= 1'b0;                                //We can de-asert flag here it only will be asserted again if we go to S3
                  rS5pFlag <= 1'b0;                               //We can de-asert flag here it only will be asserted again if we go to S5p

                  if (iPsPwrOk == 1'b0 && rAdrEna == 1'b0)        //Psu PwrOk dropped unexpectedly and ADR is not enabled, then sense as failure
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b001;                      //fault code for PSU dropping PSU PWROK (when ADR not enabled)
                       //rPsuSwFsm <= ERROR;                      //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                  else if (~iPwrGdP3V3 && iPsPwrOk)
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b010;                                //either P5V or P3V3 MAIN domain VRs has failed
                       //rPsuSwFsm <= ERROR;                                //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                  
                  else if (!iEnaMain && iSlpS3)                  //If going to S3 as well as enable from master is removed...
                    rPsuSwFsm <= S3;

                  else if (!iEnaMain && iSlpS5 && (iDimm12VCpsS5_n || iAcRemoval))         //SplS5 asserted and system has to power down DIMMs 12V (no jumper or AC removal happened)
                    rPsuSwFsm <= S5;
                  else if (!iEnaMain && iSlpS5 && !iDimm12VCpsS5_n && !iAcRemoval)       //SlpS5 asserted and system has to keep 12V power to DIMMs (jumper set and no AC removal event)
                    rPsuSwFsm <= S5p;

                  else if (iEnaMain && wDoneT1 && !iSlpS3 && !iSlpS5)       //if we are in power up sequence and T1 has expired, safe to move to S0 and turn down STBY to keep only MAIN feeding 12V to DIMMs thru s3 switch
                      rPsuSwFsm <= S0;
                  
                  else                                                      //We Keep system in S3 until ready to move on any of prior conditions are met
                    rPsuSwFsm <= S0hold;

               end // case: S0hold
               

               S0: begin

                  oPsuEna <= 1'b1;
                  oP5VEna <= 1'b1;  
                  oClkDevEna <= 1'b1;                             //P5V & P3V3 are ON, enabling clocks for SI5332 / CK440
                  //oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b1;
                  
                  rStartTimerT1 = 1'b0;                           //clear T1 timer
                  
                  oAuxSwEna <= 1'b1;

                  //rFromPwrUp <= 1'b0;

                  rStartTimerT1 = 1'b0;                           //clear timers
                  rStartTimerT2 = 1'b0;
                  
                  oS3SwP12vStbyEna <= 1'b0;                       //Disable STBY to S3 SW 
                  oS3SwP12vMainEna <= 1'b1;                       //Keeping MAIN to S3 SW

                  rAdrEna <= iAdrEna;

                  rS3Flag <= 1'b0;                                //We can de-asert flag here it only will be asserted again if we go to S3

                  if (iPsPwrOk == 1'b0 && rAdrEna == 1'b0)        //Psu PwrOk dropped unexpectedly and ADR is not enabled, then sense as failure
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b001;                      //fault code for PSU dropping PSU PWROK (when ADR not enabled)
                       //rPsuSwFsm <= ERROR;                      //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                  else if (~iPwrGdP3V3 && iPsPwrOk)
                    begin
                       oFault <= 1'b1;
                       oFaultCode <= 3'b010;                      //either P5V or P3V3 MAIN domain VRs has failed
                       //rPsuSwFsm <= ERROR;                      //alarios Jun-30-21: if we jump into ERROR,we will shut down clock to CPU, which is still consuming it for CPS, etc
                    end
                  else if (~iEnaMain)                             //Master Sequencer signals a power down. We are waiting for the Master to command the pwrdwn instead of following the SLP signals
                    begin
                       rPsuSwFsm <= S0down;                       //In order to do it in an orderly manner
                    end
                  else
                    rPsuSwFsm <= S0;

               end // case: S0

               S0down: begin

                  oPsuEna <= 1'b1;                               //Enable PSU (12V Main)
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oFault <= 1'b0;
                  oFaultCode <= 3'b000;
                  oDone <= 1'b1;

                  rStartTimerT1 = 1'b0;
                  rStartTimerT2 = 1'b0;

                  oAuxSwEna <= 1'b1;                    //if coming from pwr up sequence, keep it in STBY
                  oS3SwP12vStbyEna <= 1'b0;
                  oS3SwP12vMainEna <= 1'b1;

                  if (!iEnaMain && !iPwrGdP3V3 && iSlpS3 && !iSlpS5)
                    rPsuSwFsm <= S3hold;
                  else if (!iEnaMain && !iPwrGdP3V3 && iSlpS5 && (iDimm12VCpsS5_n || iAcRemoval))       //no need to keep 12V to dimms when going to S5 because either no jumper 83 is set, or there was an AC removal event
                    rPsuSwFsm <= S5;
                  else if (!iEnaMain && !iPwrGdP3V3 && iSlpS5 && !iDimm12VCpsS5_n && !iAcRemoval)       //SlpS5 asserted and we need to keep 12V (from STBY) to DIMMs (jumper J83 is set and no AC removal event) so we jump to a transition state
                    rPsuSwFsm <= S3hold;
                  else if (!iEnaMain && !iSlpS5 && !iSlpS3)                                              //if enable to module is removed, we need to power down even if SlpSignals are not asserted
                    rPsuSwFsm <= S5;
                  else                                            //We Keep system in S0down until ready to move-on to any of prior conditions are met
                    rPsuSwFsm <= S0down;
                  
               end // case: S0down
               

               /*ERROR: begin
                  oPsuEna <= 1'b0;
                  oP5VEna <= 1'b0;
                  oClkDevEna <= 1'b0;
                  oAuxSwEna <= 1'b0;
                  oS3SwP12vStbyEna <= 1'b0;
                  oS3SwP12vMainEna <= 1'b0;

                  oDone <= 1'b0;
                  
                  rPsuSwFsm <= ERROR;
               end*/
               
               

             endcase
          end
     end
   
    
   
    
endmodule // psu_s3_sw_ctrl
