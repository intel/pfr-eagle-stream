
//////////////////////////////////////////////////////////////////////////////////
/* Description: The PSU switch logic is considering PCH states and DIMM strap to determine the output for S3 logic
The switch logic is going to be designed considering the following truth table: */


//////////////////////////////////////////////////////////////////////////////////

module psu_sw_logic
(
    input             iClk,   					 //System clock
    input             iRst_n, 					 //synchronous Reset 
	 input            ienable,					 //internal input signal coming from master to start the FSM
	 input			  iPWRGD_PS_PWROK_PLD_R, //Power Good from Power Supply
	 input            iFM_SLP3_N,				 //Sleep 3 signal
	 input            iFM_SLP4_N,				 //Sleep 4 signal
	 input            iFM_DIMM_12V_CPS_SX_N, //2 Pin header option .. Deofault 3.3V
	 //input            iPWRGD_P3V3, 				//Indicates if Vr 3.3V Main is power OK
	 
    output reg        oFM_AUX_SW_EN,         // Signal to enable P12V_AUX Switch
	 //output reg       oFM_P5V_EN,  			 // Enable signal for P5V VR on System VRs
	 output reg       oFM_S3_SW_P12V_STBY_EN,// Output to select in switch 12V_STBY 
	 output reg       oFM_S3_SW_P12V_EN      // Output to select in switch 12V
	 //output reg       ofault,	 
	 //output reg 		oFM_PLD_CLK_DEV_EN    //Output to enable 100MHz external generator from SI5332 or CK440
);

//////////////////////////////////////////////////////////////////////////////////
// wires
//////////////////////////////////////////////////////////////////////////////////

//wire tr1, tr2, tr3, tr4, tr5, tr6, tr7, tr8, tr9, tr10, tr11, tr12;
//wire tr13, tr14, tr15, tr16, tr17, tr24;//, tr18, tr19, tr20, tr21, tr22, tr23, tr24, tr25;
wire trigger;
wire done;

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
	 //parameter  faultSt = 3'b111;   //State of FSM


//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
reg [2:0] state, // Current state
 nxtState; // Next state

assign trigger = (state == S0hold)  ? 1'b1  : 1'b0;


//Instance of counter to generate a delay of 1ms
counterSW u1 (.iClk(iClk),.iRst_n(iRst_n),.enable(trigger),.done(done));


//////////////////////////////////////////////////////////////////////////////////
// Combinational Logic
//////////////////////////////////////////////////////////////////////////////////

always @(*) begin
	nxtState = state;
	oFM_AUX_SW_EN = LOW;
	oFM_S3_SW_P12V_STBY_EN = LOW;
	oFM_S3_SW_P12V_EN = LOW;
	//ofault = LOW;
	case (state)
		init : 	begin           //init state... after powering up FGPA, the FSM should be in this state. 
					//#############################################################################################
					//OUTPUTS of INIT STATE//
					oFM_AUX_SW_EN = LOW;										//enable to switch 12V Stby to 12Main											
					oFM_S3_SW_P12V_STBY_EN = LOW;							// enable to power VR woth 12v Stby
					oFM_S3_SW_P12V_EN = LOW;								// enable to power VR woth 12v Main
					//#############################################################################################
					
					
					//#############################################################################################
					//Conditions to move to next state					
					if (!ienable) begin                   //Condition to freeze FMS and stay on init
						nxtState = init;
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && iFM_DIMM_12V_CPS_SX_N) begin //if jumper(DIMM_12V_CPS) is set to "1", move S4 state
						nxtState = S4;                                     
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && !iFM_DIMM_12V_CPS_SX_N) begin//if jumper(DIMM_12V_CPS) is set to "0", move S4p state
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
					if ((!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && iFM_DIMM_12V_CPS_SX_N) || !ienable) begin//if jumper(DIMM_12V_CPS) is set to "1", stay S4 state, or enable from master is 0
						nxtState = S4;    
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && iFM_SLP4_N) begin //if transition to S3 is detected, move to S3 state
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
					if ((!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && !iFM_DIMM_12V_CPS_SX_N) || !ienable) begin//if jumper(DIMM_12V_CPS) is set to "0", stay S4p state, or enable from master is 0
						nxtState = S4p;        
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && iFM_SLP4_N) begin				//if transition to S3 is detected, move to S3 state
						nxtState = S3; 
						end
					else begin
						nxtState = init;						//if any other invalid condition, move to init
						end
					//#############################################################################################

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
					if ((!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && iFM_SLP4_N) || !ienable) begin	//if S3 condition, but enable from master is "0", stay in s3 state
						nxtState = S3;        
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "1" change to S4 state
						nxtState = S4; 
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && !iFM_SLP3_N && !iFM_SLP4_N && !iFM_DIMM_12V_CPS_SX_N) begin //if PCH moves to S4/5, and jumper is set to "0" change to S4p state
						nxtState = S4p; 
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N &  iFM_SLP4_N) begin			//is PCH moves to S0, but PSU pwrd good is not on yet, move to S0 state
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
					if ((!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N) || !ienable) begin  //is state is S0, or enable is set to "0", stay in S0
						nxtState = S0;         
						end
					else if (iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N) begin	//if PCH is in S0, anf PSU power good is "1", move to S0hold state
						nxtState = S0hold;    
						end
					else if (!iFM_SLP3_N && iFM_SLP4_N) begin				//if PCH is in S3, move to S3 state
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
					oFM_AUX_SW_EN = LOW;                                   //enable to switch 12V Stby to 12Main
					oFM_S3_SW_P12V_STBY_EN = HIGH;                          // enable to power VR woth 12v Stby
					oFM_S3_SW_P12V_EN = HIGH;                               // enable to power VR woth 12v Main
					//#############################################################################################					
					
					//#############################################################################################
					//Conditions to move to next state							
					if (iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N && done) begin			//if PCH is S0, PSU pwrdgd and timer is done.. move to S0on
						nxtState = S0on;        
						end
					else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N && done) begin		//if PCH is S0, PSU pwrdgd is "0" and timer is done.. move to S0
						nxtState = S0;     				
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
					if ((iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N) || !ienable) begin	 //if PCH in S0 and PSU power good, but enable is 0, stay on SOon
						nxtState = S0on;        
						end
					else if (!iPWRGD_PS_PWROK_PLD_R/* && iFM_SLP3_N && iFM_SLP4_N*/) begin	//if PCH in S0 and PSU power good =0, move back to SOhold
						nxtState = S0;   					
						end
//					else if (!iPWRGD_PS_PWROK_PLD_R && iFM_SLP3_N && iFM_SLP4_N) begin	//if PCH in S0, but PSU power good is "0", move to init
//						nxtState = init;                                                                                        
//						end
					//#############################################################################################
					end						
		default : begin
			oFM_AUX_SW_EN = LOW;
			oFM_S3_SW_P12V_STBY_EN = LOW;
			oFM_S3_SW_P12V_EN = LOW;	
			nxtState = init;
			end
	endcase
end



//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////



always @(posedge iClk) begin
	if (!iRst_n) begin
		state <= init; // Initial state
	end 
	else begin
		state <= nxtState;
	end
end 







endmodule 
