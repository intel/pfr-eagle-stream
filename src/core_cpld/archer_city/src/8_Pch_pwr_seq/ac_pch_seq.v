/* ==========================================================================================================
   = Project     :  Archer City Power Sequencing
   = Title       :  BMC Power Sequence State Machine
   = File        :  ac_bmc_pwrseq_fsm.v
   ==========================================================================================================
   = Author      :  Pedro Saldana
   = Division    :  BD GDC
   = E-mail      :  pedro.a.saldana.zepeda@intel.com
   ==========================================================================================================
   = Updated by  :  
   = Division    :  
   = E-mail      :  
   ==========================================================================================================
   = Description :  This module controls the power sequence for EBG PCH VR's, fault condition and RSMRST#. 
                    It also support LBG interposer.
                    This module does not depend on Master Sequence to enable the VR's.
                    The PCH VR's are enabled on S5 state.


   = Constraints :  
   
   = Notes       :  
   
   ==========================================================================================================
   = Revisions   :  
     July 15, 2019;    0.1; Initial release
   ========================================================================================================== */

module ac_pch_seq
(
//------------------------
//Clock and Reset signals
//------------------------

    input           iClk,                         //clock for sequential logic 
    input           iRst_n,                       //reset signal, resets state machine to initial state

// ----------------------------
// Power-up sequence inputs and outputs
// ---------------------------- 
     input           iFM_PCH_PRSNT_N,             //Detect if the PCH is present 
     input           iRST_SRST_BMC_N,             //RST BMC
     input           iPWRGD_P1V8_BMC_AUX,         //PCH VR PWRGD P1V8
     input           iPWRGD_PCH_PVNN_AUX,         //PCH VR PVNN
     input           iPWRGD_PCH_P1V05_AUX,        //PCH VR PWRGD P1V05
     input           iRST_DEDI_BUSY_PLD_N,        //Dediprog Detection Support
     input           iPCH_PWR_EN,                 //Enable PCH 

     output  reg     oFM_PVNN_PCH_EN,             //PVVN VR EN
     output  reg     oFM_P1V05_PCH_EN,            //PVVN VR EN
     output  reg     oRST_RSMRST_N,               //RSMRST# 
     output  reg     oPCH_PWRGD,                  //PWRGD of all PCH VR's 
     output  reg     oPCH_PWR_FLT_PVNN,           //Fault PCH PVNN
     output  reg     oPCH_PWR_FLT_P1V05,          //Fault PCH P1V05
     output  reg     oPCH_PWR_FLT,                //Fault PCH VR's 
     output  reg     oCLK_INJ_25MHz_EN            //Enable 25MHz CLOCK_INJ
);

//////////////////////////////////////////////////////////////////////////////////
// States for root state machine
//////////////////////////////////////////////////////////////////////////////////
     localparam ST_INIT                    = 4'd0;
     localparam ST_PVNN_ON                 = 4'd1;
     localparam ST_P1V05_ON                = 4'd2;
     localparam ST_PCH_PWR_OK              = 4'd3;
     localparam ST_P1V05_OFF               = 4'd4;
     localparam ST_PVNN_OFF                = 4'd5;
//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
     localparam  LOW =1'b0;
     localparam  HIGH=1'b1;
     localparam  T_10uS_2M  =  5'd20;
     localparam  T_10mS_2M  =  15'd20000;
     localparam  T_50mS_2M  =  17'd100000;

     reg [3:0] root_state;                        //main sequencing state machine
     reg [3:0]  rPREVIOUS_STATE = ST_INIT;
     reg       rPWR_FAULT;
     reg       rPWRGD_PVNN_PCH_FF1;
     reg       rPWRGD_P1V05_PCH_FF1;
     reg       rPWRGD_PVNN_PCH_FAIL;
     reg       rPWRGD_P1V05_PCH_FAIL;

     wire      wPWRGD_ALL_PCH_VRs_DLY_10mS;
     wire      wRST_SRST_BMC_DLY_50mS;
     wire      wDLY_10uS;
     wire      wTIME_OUT_10uS;
     wire      wCNT_RST_N;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments    /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
assign  wCNT_RST_N       = (root_state == rPREVIOUS_STATE);
assign  wTIME_OUT_10uS    = wDLY_10uS && wCNT_RST_N;

     //state machine for PCH
     always @ (posedge iClk, negedge iRst_n)
     begin
          if(!iRst_n)                             //asynchronous, negative-edge reset
          begin
               oFM_PVNN_PCH_EN   <= LOW;          //disable all VR's, initial state
               oFM_P1V05_PCH_EN  <= LOW; 
               root_state        <= ST_INIT;      //reset state machines to initial states
               oPCH_PWRGD        <= LOW;          //reset BMC PWR OK signal
               oCLK_INJ_25MHz_EN <= HIGH;         //Requested by Humberto need to be chnged to LOW
          end
          else
          begin
               case(root_state)
               ST_INIT: 
               begin
                    oPCH_PWRGD        <= LOW;
                    oCLK_INJ_25MHz_EN <= HIGH;                                    //reset BMC PWR OK signal
                    if(iPCH_PWR_EN && iPWRGD_P1V8_BMC_AUX && !iFM_PCH_PRSNT_N && !rPWR_FAULT)  //Condition to start PCH power up sequence
                         begin
                              root_state <= ST_PVNN_ON;                                       
                         end      
               end
               
               ST_PVNN_ON:
               begin
                    oFM_PVNN_PCH_EN <= HIGH;                           //power up the VR 
                    if(rPWR_FAULT || !iPCH_PWR_EN)                     //if power down flag set, skip to power down sequence
                         root_state <= ST_P1V05_OFF;
                    else if(iPWRGD_PCH_PVNN_AUX && wTIME_OUT_10uS)     //if the power good signal for P2V5 is high, implement next state now   
                    begin
                         root_state <= ST_P1V05_ON;                    //go to next state
                    end
               end
               
               ST_P1V05_ON: 
               begin
                    oFM_P1V05_PCH_EN <= HIGH;                          //power up the VR  
                    if(rPWR_FAULT || !iPCH_PWR_EN)                     //if power down flag set, skip to power down sequence
                         root_state <= ST_P1V05_OFF;
                    else if(iPWRGD_PCH_P1V05_AUX && wTIME_OUT_10uS)    //if the power good signal for P1V0 is high, implement next state now
                    begin
                         root_state <= ST_PCH_PWR_OK;                  //go to next state
                    end
               end

               ST_PCH_PWR_OK: 
               begin
                    oPCH_PWRGD        <= HIGH;                         //All BMC VR's Up and Running 
                    oCLK_INJ_25MHz_EN <= LOW;                          //Enable CLK_INJ  
                    if(rPWR_FAULT || !iPCH_PWR_EN) 
                         root_state <= ST_P1V05_OFF; 
               end

               ST_P1V05_OFF: 
               begin
                    oFM_P1V05_PCH_EN <= LOW;                        //deassert enable to power down 

                    if(!iPWRGD_PCH_P1V05_AUX && wTIME_OUT_10uS)     //if current stage is off
                    begin
                         root_state <= ST_PVNN_OFF;                 //go to next state
                    end
               end
               
               ST_PVNN_OFF: 
               begin
                    oFM_PVNN_PCH_EN <= LOW;                        //deassert enable to power down
                    oPCH_PWRGD      <= LOW; 
                    if(!iPWRGD_PCH_PVNN_AUX && wTIME_OUT_10uS)     //if current stage is off
                    begin
                         root_state <= ST_INIT;                    //now go to state of waiting for power-up
                    end
               end
               
               default: 
               begin
                    root_state <= ST_INIT;
               end
               endcase
               rPREVIOUS_STATE <= root_state;     
          end
     end

//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////

always @ ( posedge iClk) 
     begin 
          if (  !iRst_n  )   
               begin
                    oPCH_PWR_FLT                    <= LOW;

                    oRST_RSMRST_N                   <= LOW;
                    rPWRGD_PVNN_PCH_FF1             <= LOW;
                    rPWRGD_P1V05_PCH_FF1            <= LOW;
                    rPWRGD_PVNN_PCH_FAIL            <= LOW;
                    rPWRGD_P1V05_PCH_FAIL           <= LOW;
                    oPCH_PWR_FLT_PVNN               <= LOW;
                    oPCH_PWR_FLT_P1V05              <= LOW;

                    rPWR_FAULT                      <= LOW;
               end 
          else 
               begin
                    rPWRGD_PVNN_PCH_FF1             <= iPWRGD_PCH_PVNN_AUX;
                    rPWRGD_P1V05_PCH_FF1            <= iPWRGD_PCH_P1V05_AUX;

                    rPWRGD_PVNN_PCH_FAIL            <= (oFM_PVNN_PCH_EN  && rPWRGD_PVNN_PCH_FF1  && !iPWRGD_PCH_PVNN_AUX)   ?  HIGH: rPWRGD_PVNN_PCH_FAIL;
                    rPWRGD_P1V05_PCH_FAIL           <= (oFM_P1V05_PCH_EN && rPWRGD_P1V05_PCH_FF1 && !iPWRGD_PCH_P1V05_AUX)  ?  HIGH: rPWRGD_P1V05_PCH_FAIL;
                    rPWR_FAULT                      <= (rPWRGD_PVNN_PCH_FAIL || rPWRGD_P1V05_PCH_FAIL) ? HIGH: rPWR_FAULT; 
                    oPCH_PWR_FLT                    <= (rPWRGD_PVNN_PCH_FAIL || rPWRGD_P1V05_PCH_FAIL) ? HIGH: oPCH_PWR_FLT; 
                    oPCH_PWR_FLT_PVNN               <= rPWRGD_PVNN_PCH_FAIL;
                    oPCH_PWR_FLT_P1V05              <= rPWRGD_P1V05_PCH_FAIL;

                    oRST_RSMRST_N                   <= wPWRGD_ALL_PCH_VRs_DLY_10mS && wRST_SRST_BMC_DLY_50mS /*&& iRST_DEDI_BUSY_PLD_N*/;                                     
               end
     end  

//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// 10ms minimun Delay for power UP (requiered for PCH-t31 power up sequence spec)
//
counter2 #( .MAX_COUNT(T_10mS_2M) ) PWR_OK_10mS_DLY
( .iClk        (iClk), 
.iRst_n        (iRst_n),

.iCntRst_n     (oPCH_PWRGD),
.iCntEn        (oPCH_PWRGD),
.iLoad         (!(oPCH_PWRGD)),
.iSetCnt       (T_10mS_2M[14:0]),                   // set the time out time as 10ms
.oDone         (wPWRGD_ALL_PCH_VRs_DLY_10mS),
.oCntr         ( ) 
);
//////////////////////////////////////////////////////////////////////
// 50ms minimun Delay for power UP (requiered for AST2600 power up sequence spec)
//
counter2 #( .MAX_COUNT(T_50mS_2M) ) PWR_OK_50mS_DLY
( .iClk        (iClk), 
.iRst_n        (iRst_n),

.iCntRst_n     (iRST_SRST_BMC_N),
.iCntEn        (iRST_SRST_BMC_N),
.iLoad         (!(iRST_SRST_BMC_N)),
.iSetCnt       (T_50mS_2M[16:0]),                   // set the time out time as 50ms
.oDone         (wRST_SRST_BMC_DLY_50mS),
.oCntr         ( ) 
);

counter2 #( .MAX_COUNT(T_10uS_2M) ) PWR_UP_DOWN_10uS_DLY
( .iClk         (iClk), 
.iRst_n        (iRst_n),

.iCntRst_n     (wCNT_RST_N),
.iCntEn        (wCNT_RST_N),
.iLoad         (!(wCNT_RST_N)),
.iSetCnt       (T_10uS_2M[4:0]),  // set the time out time as 1ms
.oDone         (wDLY_10uS),
.oCntr         ( ) 
);

endmodule
