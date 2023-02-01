
//////////////////////////////////////////////////////////////////////////////////
/* Description: The PSU switch logic is considering PCH states and DIMM strap to determine the output for S3 logic
 The switch logic is going to be designed considering the following truth table: */


//////////////////////////////////////////////////////////////////////////////////

module psu_sw_logic
  (
   input      iClk, //System clock
   input      iRst_n, //synchronous Reset 
   input      ienable, //internal input signal coming from master to start the FSM
   input      iPWRGD_PS_PWROK_PLD_R, //Power Good from Power Supply
   input      iFM_SLP3_ID, //Sleep 3 signal //alarios: changed, now active high, comming from Master FUB
   input      iFM_SLP4_ID, //Sleep 4 signal //alarios: changed, now active high, comming from Master FUB
   input      iFM_DIMM_12V_CPS_SX_N, //2 Pin header option .. Deofault 3.3V
   input      iPsuMainRdy,           //when psu_pwr_on_check module is ready (and waiting) to turn PSU off

   input      iFailId,              //when in fail condition, switch should not enable power switches back on even if pch de-asserts SLP signals
   
   //input            iPWRGD_P3V3, 				//Indicates if Vr 3.3V Main is power OK
   
   output reg oFM_AUX_SW_EN, // Signal to enable P12V_AUX Switch
   //output reg       oFM_P5V_EN,  			 // Enable signal for P5V VR on System VRs
   output reg oFM_S3_SW_P12V_STBY_EN,// Output to select in switch 12V_STBY 
   output reg oFM_S3_SW_P12V_EN      // Output to select in switch 12V
   //output reg       ofault,	 
   //output reg 		oFM_PLD_CLK_DEV_EN    //Output to enable 100MHz external generator from SI5332 or CK440
   );
   
   //////////////////////////////////////////////////////////////////////////////////
  // wires
   //////////////////////////////////////////////////////////////////////////////////
   
   //wire tr1, tr2, tr3, tr4, tr5, tr6, tr7, tr8, tr9, tr10, tr11, tr12;
   //wire tr13, tr14, tr15, tr16, tr17, tr24;//, tr18, tr19, tr20, tr21, tr22, tr23, tr24, tr25;
   wire       trigger;
   wire       done1ms;
   wire       done3ms;
   
   //////////////////////////////////////////////////////////////////////////////////
   // Parameters
   //////////////////////////////////////////////////////////////////////////////////
   
   parameter  LOW     = 1'b0;
   parameter  HIGH    = 1'b1;
   parameter  init    = 3'b000;   //State of FSM
   parameter  S4      = 3'b001;   //State of FSM
   parameter  S4p     = 3'b010;   //State of FSM
   parameter  S3      = 3'b011;   //State of FSM
   parameter  S0      = 3'b100;   //State of FSM
   parameter  S0hold  = 3'b101;   //State of FSM
   parameter  S0on    = 3'b110;   //State of FSM
   parameter  S4hold  = 3'b111;   //State of FSM 
   //parameter  faultSt = 3'b111;   //State of FSM
   
   
   //////////////////////////////////////////////////////////////////////////////////
   // Internal Signals
   //////////////////////////////////////////////////////////////////////////////////
   reg [2:0]  state, nxtState; // Current state

   wire       wDoneMemSwTimerDone;
   
   assign trigger = (state == S0hold)  ? 1'b1  : 1'b0;
   
   
   //Instance of counter to generate a delay of 1ms
   counterSW u1 (.iClk(iClk),.iRst_n(iRst_n),.enable(trigger),.done(done1ms));

   //Instance of counter to generate a delay of 1ms
   counterSW #(.UnitTime(400000))
     u2 (
         .iClk(iClk),
         .iRst_n(iRst_n),
         .enable(trigger),
         .done(done3ms)
         );

   //Instance of counter to generate a delay of 1ms
   counterSW #(.UnitTime(3000))
     t1 (
         .iClk(iClk),
         .iRst_n(iRst_n),
         .enable(state == S4hold),
         .done(wDoneMemSwTimerDone)
         );
   
   //////////////////////////////////////////////////////////////////////////////////
   // Combinational Logic
   //////////////////////////////////////////////////////////////////////////////////
   
   always @(*) begin

           //nxtState = state;
           //oFM_AUX_SW_EN = LOW;              //enable to switch 12V Stby to 12Main	(selected STBY at rst)
		   //oFM_S3_SW_P12V_STBY_EN = LOW;     // enable to power VR woth 12v Stby   
		   //oFM_S3_SW_P12V_EN = LOW;          // enable to power VR woth 12v Main
	       case (state)
		     init : 	begin           //init state... after powering up FGPA, the FSM should be in this state. 
		        //#############################################################################################
		        //OUTPUTS of INIT STATE//
		        oFM_AUX_SW_EN = LOW;									//enable to switch 12V Stby to 12Main											
		        oFM_S3_SW_P12V_STBY_EN = LOW;							// enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;								// enable to power VR woth 12v Main
		        //#############################################################################################
		   
		   
		        //#############################################################################################
		        //Conditions to move to next state					
		        if (!ienable) begin                   //Condition to freeze FMS and stay on init
			       nxtState = init;
		        end
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
                else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP4_ID && iFM_DIMM_12V_CPS_SX_N) begin //if jumper(DIMM_12V_CPS) is set to "1", move S4 state
			       nxtState = S4;                                     
		        end
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
		        else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP4_ID && !iFM_DIMM_12V_CPS_SX_N) begin//if jumper(DIMM_12V_CPS) is set to "0", move S4p state
			       nxtState = S4p;                              
		        end
		        else begin									  //if any other invalid condition, move to init
			       nxtState = init;                                                                                                                 
		        end
		     end
		     //#############################################################################################
		     
		
		     S4 : 		begin
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//
		        oFM_AUX_SW_EN = LOW;                           //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = LOW;                  // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;                       // enable to power VR woth 12v Main
		        //#############################################################################################
		        
		        //#############################################################################################
		        //Conditions to move to next state			
                if (iFailId)
                  nxtState = S4;
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
		        else if ((!iPWRGD_PS_PWROK_PLD_R && iFM_SLP4_ID && iFM_DIMM_12V_CPS_SX_N) || !ienable) begin//if jumper(DIMM_12V_CPS) is set to "1", stay S4 state, or enable from master is 0
			       nxtState = S4;    
		        end
                //alarios: iFM_SLP3_ID active high has been detected by Master FUB
		        else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP4_ID) begin //if transition to S3 is detected, move to S3 state
			       nxtState = S3;  	 
		        end
		        else begin                              //if any other invalid condition, move to init
			       nxtState = init;                                                                             
		        end
		        //#############################################################################################
		     end
		     S4p : 	begin
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//	
		        oFM_AUX_SW_EN = LOW;                               //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = HIGH;                     // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;	                        // enable to power VR woth 12v Main
		        //#############################################################################################
		        
		        //#############################################################################################
		        //Conditions to move to next state
                if (iFailId)
                  nxtState = S4;
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
		        if ((!iPWRGD_PS_PWROK_PLD_R && iFM_SLP4_ID && !iFM_DIMM_12V_CPS_SX_N) || !ienable) begin//if jumper(DIMM_12V_CPS) is set to "0", stay S4p state, or enable from master is 0
			       nxtState = S4p;        
		        end
                //alarios: iFM_SLP3_ID active high has been detected by Master FUB
		        else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP4_ID) begin				//if transition to S3 is detected, move to S3 state
			       nxtState = S3; 
		        end
		        else begin
			       nxtState = init;						//if any other invalid condition, move to init
		        end
		        //#############################################################################################
             end // case: S4p

             //alarios: when going to S5 directly, turn off memory and wait some time before P12V AUX is switched to STBY
             //also sync with PSU so P12V Main remains alive until this happens
             S4hold : begin
                oFM_AUX_SW_EN = LOW;                               //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = HIGH;                     // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;	                        // enable to power VR woth 12v Main

                if (wDoneMemSwTimerDone)
                  nxtState = S3;                //going to
                else
                  nxtState = S4hold;
		     end
		     S3 : 		begin
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//			
		        oFM_AUX_SW_EN = LOW;                                //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = HIGH;                      // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;	                         // enable to power VR woth 12v Main
		        //#############################################################################################
		        
		        //#############################################################################################
		        //Conditions to move to next state
		        //if ((!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && iFM_SLP4_ID) || !ienable) begin	//if S3 condition, but enable from master is "0", stay in s3 state
                //alarios: iFM_SLP3_ID active high has been detected by Master FUB
                if ((iFM_SLP3_ID && !iFM_SLP4_ID)) begin	//above condition will cause this if not to enter if psupwrOK is still HIGH, then it could cause to go to INIT state as none of the
                   nxtState = S3;                        //conditions will be met either if just arrived from S0on
		        end
		        //else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID && iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "1" change to S4 state
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
                else if (iFM_SLP4_ID && iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "1" change to S4 state
			       nxtState = S4; 
		        end
		        //else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID && !iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "0" change to S4p state
                //alarios: iFM_SLP4_ID active high has been detected by Master FUB
                else if (iFM_SLP4_ID && !iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "0" change to S4p state
			       nxtState = S4p; 
		        end
		        //else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_ID &  iFM_SLP4_ID) begin			//is PCH moves to S0, but PSU pwrd good is not on yet, move to S0 state
                //alarios: iFM_SLP3_ID & iFM_SLP4_ID active high have not been detected by Master FUB
                else if (!iFM_SLP3_ID &  !iFM_SLP4_ID) begin			//alarios: removed checking for PSUPWROK here, it may be asserted or not here, is PCH moves to S0
			       nxtState = S0;        
		        end
		        else begin										//if any other invalid condition, move to init
			       nxtState = init;					                                                                        
		        end
		        //#############################################################################################
		     end
		     S0 : 		begin
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//									
		        oFM_AUX_SW_EN = LOW;                               //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = HIGH;                     // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = LOW;                           // enable to power VR woth 12v Main
		        //#############################################################################################					
		        
		        //#############################################################################################
		        //Conditions to move to next state				
                //alarios: no SLP ID from master fub
		        if ((!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID) || !ienable) begin  //is state is S0, or enable is set to "0", stay in S0
			       nxtState = S0;         
		        end
                //alarios: no SLP ID from master fub
		        else if (iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID) begin	//if PCH is in S0, anf PSU power good is "1", move to S0hold state
			       nxtState = S0hold;    
		        end
		        //else if (!iFM_SLP3_ID && iFM_SLP4_ID) begin				//if PCH is in S3, move to S3 state
                //alarios: SLPS3 ID from master fub
                else if (iFM_SLP3_ID || iFM_SLP4_ID) begin        //alarios: if S3 or S4 ID signal, move to S3 (we will check for S4 again in S3 state)
			       nxtState = S3;        
		        end
		        else begin
			       nxtState = init;						//if any other invalid condition, move to init
		        end
		        //#############################################################################################					
		     end
		     S0hold: 	begin      //This state triggers a counter in order to create a delay of X-us
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//
                oFM_AUX_SW_EN = HIGH;                                   //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = HIGH;                          // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = HIGH;                               // enable to power VR woth 12v Main
		        //#############################################################################################					
		        
		        //#############################################################################################
		        //Conditions to move to next state
                //alarios: no SLP ID from master fub
		        if (iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID && done1ms) begin			//if PCH is S0, PSU pwrdgd and timer is done.. move to S0on
			       nxtState = S0on;        
		        end
		        //else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_ID && iFM_SLP4_ID && done1ms) begin		//if PCH is S0, PSU pwrdgd is "0" and timer is done.. move to S0
		        //  state <= S0;
                //end
                //alarios: SLPS3 or SLPS4 ID from master fub
                else if ((iFM_SLP3_ID || iFM_SLP4_ID) && done3ms) begin
                   nxtState = S4hold;   //alarios: go to S3
                   //oFM_AUX_SW_EN = LOW;                                   // switch 12V to STBY
		        end
		        else begin										//if timer is not expired, wait on this state
			       nxtState = S0hold;   		                                                                                     
		        end
		        //#############################################################################################					
		     end
		     S0on : 	begin
		        //#############################################################################################
		        //OUTPUTS of S4 STATE//		
		        oFM_AUX_SW_EN = HIGH;                               //enable to switch 12V Stby to 12Main
		        oFM_S3_SW_P12V_STBY_EN = LOW;                       // enable to power VR woth 12v Stby
		        oFM_S3_SW_P12V_EN = HIGH;                           // enable to power VR woth 12v Main
		        //#############################################################################################										
		        
		        //#############################################################################################
		        //Conditions to move to next state
                //alarios: no SLP ID from master fub
		        if ((iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_ID && !iFM_SLP4_ID) || !ienable) begin	 //if PCH in S0 and PSU power good, but enable is 0, stay on SOon
			       nxtState = S0on;        
		        end
		        //else if (!iPWRGD_PS_PWROK_PLD_R/* && iFM_SLP3_ID && iFM_SLP4_ID*/) begin	//if PCH in S0 and PSU power good =0, move back to SOhold
                //alarios: SLPS3 or SLPS4 ID from master fub
                else if ((iFM_SLP3_ID || iFM_SLP4_ID) && iPsuMainRdy) begin	         //alarios: above condition is wrong! we need to move following S3, and we need to do it to S0hold
                   nxtState = S0hold;
                end
                else
                  nxtState = S0on;
                
                             
                //else if ((iFM_SLP3_ID) && iPsuMainRdy) begin	         //alarios: above condition is wrong! we need to move following S3, and we need to do it to S0hold
			       //state = S0;
                //   nxtState = S0hold;
                //end
                //else if (iFM_SLP4_ID && iPsuMainRdy  && iFM_DIMM_12V_CPS_SX_N) begin
                //   nxtState = S4hold;
                //end
                //else if (iFM_SLP4_ID && iPsuMainRdy && !iFM_DIMM_12V_CPS_SX_N) begin
                //   nxtState = S4p;
		        //end
                
		        //#############################################################################################
		     end
	         default : begin
                oFM_AUX_SW_EN = LOW;
		        oFM_S3_SW_P12V_STBY_EN = LOW;
		        oFM_S3_SW_P12V_EN = LOW;	
		        nxtState = init;
		     end
	       endcase // case (state)
      
   end // always @ (posedge iClk, negedge iRst_n)

   //////////////////////////////////////////////////////////////////////////////////
   // Secuencial Logic
   //////////////////////////////////////////////////////////////////////////////////
   
   
   
   always @(posedge iClk, negedge iRst_n) begin
	  if (!iRst_n) 
        begin
		   state <= init; // Initial state
	    end 
	  else 
        begin
		   state <= nxtState;
	    end
   end //always @(posedge iClk, negedge iRst_n)
   
   
endmodule // psu_sw_logic


