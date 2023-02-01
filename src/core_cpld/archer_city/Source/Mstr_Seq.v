
//////////////////////////////////////////////////////////////////////////////////
/*!
 
    \brief    <b>%Master Sequence Control </b>          
    \file     Mstr_Seq.v 
    \details    <b>Image of the Block:</b>
                \image html Module-Mstr_Seq.PNG

                 <b>Description:</b> \n 
                Master FSM (Finite State Machine) block is the KEY Module that controls system power state transition, 
                synchronizing the VRs on/off and verifying that the PCH sends the appropriate signals to check that the sequence is correct.
                 The FSM is designed to comply with the Wilson City RP, Architecture diagrams <a href="https://sharepoint.amr.ith.intel.com/sites/WhitleyPlatform/commoncore_arch_wg/SitePages/Home.aspx" target="_blank"><b>Here</b></a>.\n

                The Mstr_Seq.v module has control of PSU_Seq.v, Mem_Seq.v, Cpu_Seq.v and MainVR_Seq.v for turn ON/OFF the VR's and also receive fault condition from each module.
                 The standby VR are controlled by Bmc_Seq.v and Pch_Seq.v and the Mstr_Seq.v receive the PWRGD for change Machine state from STBY to OFF. \n
                 The System verification related to valid CPU mix configurations this module use iSysOk to guarantee the system can turn on.\n
                
                This table contains each state of the FSM S5 -> S0 ->S5.
                
                State Hex  | State Name | Next State | Condition | Details
                -----------|------------|------------|-----------|-----------
                A          | ST_STBY    | ST_OFF     | RST_RSMRST_PCH_N & RST_SRST_BMC_N & wPwrgd_All_Aux =1 | Default State during AC-On. The system is waiting for all the STBY rails ready to trigger PCH RSMRST# as well as SRST to BMC.     
                9          | ST_OFF (S5/S4)     | ST_PS      | wPwrEn=1  | Maps to S4/S5 system Status. All the STBY rails are ready. Waiting for Wake-up event to wait up to S0.
                7          | ST_PS      | ST_MAIN     | iPSUPwrgd=1 | PSU DC Enable signal has been asserted by PLD and waiting for PSU PS_PWROK.
                6          | ST_MAIN    | ST_MEM     | iMainVRPwrgd=1 | Main VR has been enable by PLD and wait iMainVRPwrgd=1 from MainVR_Seq.v 
                5          | ST_MEM     | ST_CPU     | iMemPwrgd=1 | Memory VR has been enabled by PLD and PLD is waiting for Memory rails ready.
                4          | ST_CPU     | ST_SYSPWROK| iCpuPwrgd =1 | PLD has triggered the CPU Main rails enable sequence and waiting for the indication of all the CPU Main rail ready. 
                3          | ST_SYSPWROK| ST_CPUPWRGD| PWRGD_SYS_PWROK=1 | An intermediate state to delay 100ms before asserting the SYS_PWROK and wait for Nevo Ready.
                2          | ST_CPUPWRGD| ST_RESET   | PWRGD_CPUPWRGD =1 | Waiting for CPU_PWRGD from PCH. 
                1          | ST_RESET   | ST_DONE    | RST_PLTRST_N =1 | System is in Reset Status, Waiting for PLTRST de-assertion.
                0          | ST_DONE (S0)   | ST_SHUTDOWN| wPwrEn=0 | All the bring-up power sequence has been completed, and the BIOS shall start to execute since st_done.
                B          | ST_SHUTDOWN| ST_MAIN_OFF  | iCpuPwrgd=0 | System is in the shutdown process. Waiting for the indication of all CPU Power rails has been turned off.
				C          | ST_MAIN_OFF | ST_PS_OFF  | iMainVRPwrgd=0 | Waiting for the indication of all Main VR OFF.
                D          | ST_PS_OFF  | ST_S3      | FM_SLPS3_N=0| PSU is instructed to turn off and PLD is waiting for the assertion of SLPS3
                8          | ST_S3      | ST_OFF     | !FM_SLPS4_N && !iMemPwrgd = 1 | System in S3 Status with STBY and Memory rails On.
                F          | ST_FAULT   | ST_STBY    | oGoOutFltSt=1 | System in fault State due to various power failures and stay at fault status until the next power on command.
                
                \n\n
                For the complete flow including the fault condition, S3, shutdown, please review the next diagram:
                \image html Master-Seq.png    

                 
                
    \brief  <b>Last modified</b> 
            $Date:   Jan 19, 2018 $
            $Author:  David.bolanos@intel.com $            
            Project            : Wilson City  
            Group            : BD
    \version    
             20160709 \b  David.bolanos@intel.com - File creation\n 
             20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Mehlow\n
             20182803 \b  jorge.juarez.campos@intel.com - Added support for S3 State\n
               
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module Mstr_Seq
(
	input		        iClk,//%Clock input 2 Mhz
	input		        iRst_n,//%Reset enable on low
	input               iForcePwrOn,//% Force the system to turn on (by only PCH Signals)
	input               iEnableGoOutFltSt,//% This signal enable the system go out Fault
	input               iEnableTimeOut,//% Enable the Timeout

    input               iSysOk, //% System validation Ok
    input               iDebugPLD_Valid, //% PLD validation Ok

	input               iMemPwrgd,//% Pwrgd of all Mem VR's 
	input               iMemPwrFlt,//% Pwrgd fault of all Mem VR's 

	input               iCpuPwrgd,//% Pwrgd of all Cpu VR's 
	input               iCpuPwrFlt,//% Pwrgd fault of all Cpu VR's

	input               iBmcPwrgd,//% Pwrgd of all Bmc VR's 
	input               iBmcPwrFlt,//% Pwrgd fault of all Bmc VR's

	input               iPchPwrgd,//% Pwrgd of all Pch VR's 
	input               iPchPwrFlt,//% Pwrgd fault of all Pch VR's

	input               iPSUPwrgd,//% Pwrgd of Power Supply 
	input               iPsuPwrFlt,//% Pwrgd fault of Power Supply 

	input               iMainVRPwrgd,//% Pwrgd of Main Vr's 
	input               iMainPwrFlt,//% Pwrgd fault of Main Vr's 

	input 				iSocketRemoved,//% Go to Fault State when Socked is removed

	input               PWRGD_P3V3_AUX,//% Pwrgd from P3V3 Aux VR
	input               PWRGD_SYS_PWROK,//% Pwrgd SYS generate by PLD
	input               PWRGD_CPUPWRGD,//% Pwrgd CPU from PCH

	input               FM_BMC_ONCTL_N,//% ONCTL  coming from BMC

	input               FM_SLPS4_N,//% SLP4# from PCH
	input               FM_SLPS3_N,//% SLP3# from PCH
	input               RST_PLTRST_N,//% PLTRST# from PCH
	input               RST_RSMRST_PCH_N,//% RSMRST# from PLD
	input               RST_SRST_BMC_N,//% SRST RST# from BMC

	output reg          oMemPwrEn,//% Memory VR's enable (For module Mem_Seq).
	output reg          oCpuPwrEn,//% Cpu VR's enable (For module Cpu_Seq).
	output reg          oPsuPwrEn,//% Psu enable (For module PSU_Seq).
	output reg          oMainVRPwrEn,//% Main VR enable (For module MainVR_Seq).

	output              oGoOutFltSt,//% Go out fault state.
	output              oTimeOut,//% Time-out if the Turn ON/OFF take more of 2s the system will force shutdown

	output              oFault,//% Fault Condition 
	output 		        oPwrEn, //% PWREN 	

	output reg [3:0]	oDbgMstSt7Seg,//Code according RPAS 3.7.9 Code Guideline 
	output     [3:0]    oDbgMstSt     //Normal Code from 0x0 to 0xC (Done state is 0xA)
);


`ifdef FAST_SIM_MODE
localparam SIMULATION_MODE =  1'b1;
`else
localparam SIMULATION_MODE =  1'b0;
`endif


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter   ST_FAULT  		= 4'h0;
parameter   ST_STBY   		= 4'h1;  
parameter   ST_OFF        	= 4'h2;    
parameter   ST_S3        	= 4'h3;    				   
parameter	ST_PS         	= 4'h4;   
parameter   ST_MAIN         = 4'h5;
parameter	ST_MEM       	= 4'h6;      
parameter	ST_CPU       	= 4'h7;
parameter	ST_SYSPWROK   	= 4'h8;
parameter	ST_CPUPWRGD   	= 4'h9;                                             
parameter	ST_RESET      	= 4'hA; 
parameter	ST_DONE       	= 4'hB;
parameter	ST_SHUTDOWN   	= 4'hC;
parameter   ST_MAIN_OFF     = 4'hD;
parameter   ST_PS_OFF       = 4'hE;

parameter  LOW =1'b0;
parameter  HIGH=1'b1;

parameter  T_2S_2M     		=  SIMULATION_MODE ? 24'd100 : 24'd4000000;
parameter  T_5S_2M     		=  SIMULATION_MODE ? 24'd100 : 24'd10000000;
parameter  T_100MS_2M  		=  SIMULATION_MODE ? 24'd100 : 24'd200000; 
parameter  T_10MS_2M  		=  SIMULATION_MODE ? 24'd100 : 24'd100000; 
parameter  T_5MS_2M    		=  SIMULATION_MODE ? 24'd100 : 24'd10000;

parameter bitH     			= 5'd23;	



//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////									
reg [3:0] rCurrSt, rNextSt, rCurrSt_dly;
reg [bitH:0] setCount;
reg rST_S3_d,rST_S3_q;
reg FM_BMC_ONCTL_N_ff;
wire wPwrgd_All_Aux;
wire wRst_Rsmrst_Srst;
wire wPwrEn;
wire wWatchDogEn;
wire wCntRst_n,wCntDone,wCntEn,wTimeOutX;
wire wPsuTimeOut;
wire wCntDoneX, wPsuWatchDogEn, wPsuTimeOutX;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////
assign oDbgMstSt 		= rCurrSt;
assign wPwrgd_All_Aux	= iBmcPwrgd && iPchPwrgd && PWRGD_P3V3_AUX;
assign wRst_Rsmrst_Srst	= RST_RSMRST_PCH_N && RST_SRST_BMC_N;
assign wPwrEn 			= (!FM_BMC_ONCTL_N     	&&
							FM_SLPS3_N         	&& 
							iSysOk             	&&				   
							!oFault) || iForcePwrOn;
assign oPwrEn 			= wPwrEn;
assign oGoOutFltSt  	= ( (rCurrSt == ST_FAULT) && wCntDone  && iEnableGoOutFltSt && (!FM_BMC_ONCTL_N && FM_BMC_ONCTL_N_ff) ) ; //stay at fault status until the next power on command.

assign oFault 			= (	iMainPwrFlt 		|  
							iPsuPwrFlt  		|
							iBmcPwrFlt  		|
							iPchPwrFlt  		|
							iMemPwrFlt  		|
							iCpuPwrFlt 			|
							iSocketRemoved 		) && 	(!iForcePwrOn); 


assign wWatchDogEn		= (	(rCurrSt == ST_STBY)    ||//watch dog timer, set watch dog timer for PSU and main VRs
						(	rCurrSt == ST_PS)      ||	
						(	rCurrSt == ST_MAIN)||				
						(	rCurrSt == ST_MEM)     ||								
						(	rCurrSt == ST_SHUTDOWN) );
assign wPsuWatchDogEn	= (rCurrSt == ST_PS_OFF);//watch dog timer for PSU shut down, in some systems, the PSU is always on, and need to support the P12V always on system

assign wCntRst_n 		= !(rCurrSt != rCurrSt_dly);
assign wCntDone 		= wCntRst_n & wCntDoneX;	
assign wCntEn      		= ((rCurrSt == ST_MEM ) || (rCurrSt == ST_FAULT)); 
assign wPsuTimeOut 		= wPsuTimeOutX & wCntRst_n & iEnableTimeOut;
assign oTimeOut 		= (wTimeOutX && wCntRst_n && iEnableTimeOut); 

						


//////////////////////////////////////////////////////////////////////////////////
// FSM CurrentState generation
//////////////////////////////////////////////////////////////////////////////////
///
always @ ( posedge iClk) 
begin 
	if (  !iRst_n  )   
	begin
		rCurrSt  <= ST_STBY;
		rCurrSt_dly  <= ST_STBY;// the dly is used to compare to the next state to reset the counter.				
	end 
	else 	
	begin	
		rCurrSt  <= rNextSt;
		rCurrSt_dly  <= rCurrSt;
	end
end 

//////////////////////////////////////////////////////////////////////////////////
// State Machine logic
//////////////////////////////////////////////////////////////////////////////////

	always @ (*)
		begin
		rST_S3_d = rST_S3_q;
			case( rCurrSt )   
				ST_FAULT      :   if (oGoOutFltSt  )                        rNextSt = ST_STBY;  	//0  //F   						
								  else 									    rNextSt = ST_FAULT;
									  
				ST_STBY       :   if( oFault || oTimeOut )                	rNextSt = ST_FAULT; 	//1 //A
								  else if(wPwrgd_All_Aux && 
								  		 wRst_Rsmrst_Srst || iForcePwrOn) 
																		    rNextSt = ST_OFF;  
								  else                                      rNextSt = ST_STBY;   
							
				ST_OFF        :   if( oFault )                              rNextSt = ST_FAULT; 	//2 //9
								  else if( wPwrEn  )                        rNextSt = ST_PS;        
				                  else                                      rNextSt = ST_OFF;                  
				
				ST_S3         :   if( oFault  )                             rNextSt = ST_FAULT; 	//3 //8
								  else if (!FM_SLPS4_N && !iMemPwrgd )		rNextSt = ST_OFF;
								  else if (wPwrEn)	                        rNextSt = ST_PS; 								
								  else
									begin
										rNextSt = ST_S3;
										rST_S3_d = HIGH;
									end
									  
				ST_PS         :   if( iPsuPwrFlt )               			rNextSt = ST_PS_OFF;
								  else if( iPSUPwrgd )                      rNextSt = ST_MAIN;            
				                  else                                      rNextSt = ST_PS;   		//4 //7

				ST_MAIN   :       if( !wPwrEn || oTimeOut   )               rNextSt = ST_MAIN_OFF; 	//5 //6
					              else if((iMainVRPwrgd))                   rNextSt = ST_MEM;            
				                  else                                      rNextSt = ST_MAIN;             
									  
				ST_MEM    :       if( !wPwrEn || oTimeOut )                 rNextSt = ST_SHUTDOWN;  //6 	//5		
								  else if( iMemPwrgd && wCntDone)			rNextSt = ST_CPU;    	// wCntDone is used as a watchdog timer when a power fault is encountered. 
								  else                                      rNextSt = ST_MEM;		//With rst signal this will be held on 1, until a power fault 
								  																	//occurs and clears this up to wait for the 5s delay
			   	
			   	ST_CPU   :        if( !wPwrEn || oTimeOut )                 rNextSt = ST_SHUTDOWN; 	//7    //4 
					              else if( iCpuPwrgd )   	                rNextSt = ST_SYSPWROK;    				
								  else                                      rNextSt = ST_CPU;               
				
				ST_SYSPWROK   :   if( !wPwrEn )                             rNextSt = ST_SHUTDOWN;	//8 //3
					              else if(PWRGD_SYS_PWROK || iForcePwrOn)   rNextSt = ST_CPUPWRGD;    
				                  else                                      rNextSt = ST_SYSPWROK;               
				
				ST_CPUPWRGD   :   if( !wPwrEn )                             rNextSt = ST_SHUTDOWN;	//9 //2
					              else if(PWRGD_CPUPWRGD || iForcePwrOn )   rNextSt = ST_RESET;       				
					              else                                      rNextSt = ST_CPUPWRGD;               
				
				ST_RESET      :   if( !wPwrEn)                              rNextSt = ST_SHUTDOWN;	//A //1
					              else if(RST_PLTRST_N || iForcePwrOn )     rNextSt = ST_DONE;                      		
				                  else                                      rNextSt = ST_RESET;               
				
				ST_DONE       :   if ( !wPwrEn )                            rNextSt = ST_SHUTDOWN; 	//B  //0
					              else if( !RST_PLTRST_N )                  rNextSt = ST_RESET;                                
				                  else
										begin
											rNextSt = ST_DONE;
											rST_S3_d = LOW;
										end
				
				
				ST_SHUTDOWN   :   if((!iCpuPwrgd )  ||
									    oTimeOut )                          rNextSt = ST_MAIN_OFF;	//C    //B                      
					              else            					        rNextSt = ST_SHUTDOWN;                                
				
				ST_MAIN_OFF :     if((!iMainVRPwrgd)  ||
									    oTimeOut )                          rNextSt = ST_PS_OFF;  	//D // C                 
				                 	else                                    rNextSt = ST_MAIN_OFF; 

				ST_PS_OFF     :   if(!FM_SLPS3_N)                           rNextSt = ST_S3;   		//E     //D
								  else if (oFault)                          rNextSt = ST_FAULT; 	
								  else if (wPsuTimeOut)                     rNextSt = ST_S3; 					                 
				                  else                                      rNextSt = ST_PS_OFF; 
				
				default      :                                              rNextSt = ST_STBY;                          
			endcase 
		end   

//////////////////////////////////////////////////////////////////////////////////
// Output generation  
//////////////////////////////////////////////////////////////////////////////////

	always @( posedge iClk) begin 
			
			if (  !iRst_n  ) 
				begin
					oMemPwrEn                 <= LOW;
					oCpuPwrEn                 <= LOW; 
					oPsuPwrEn                 <= LOW;
					oMainVRPwrEn              <= LOW;

					FM_BMC_ONCTL_N_ff		  <= HIGH;				
               		rST_S3_q 				  <= LOW;
				end


			else begin	
					
					oPsuPwrEn     <= (rCurrSt >= ST_PS)    && (rCurrSt < ST_PS_OFF);	

					oMainVRPwrEn  <= (rCurrSt >= ST_MAIN)  && (rCurrSt < ST_MAIN_OFF);			

					oMemPwrEn     <= ((rCurrSt >= ST_MEM ||  rCurrSt == ST_S3 || (rST_S3_q && FM_SLPS4_N) ) && (FM_SLPS4_N || (rST_S3_q && FM_SLPS4_N) )) ? HIGH : LOW ;   

					oCpuPwrEn     <= ((rCurrSt >= ST_CPU)  && (rCurrSt < ST_SHUTDOWN)) ?  HIGH : LOW;  		

					FM_BMC_ONCTL_N_ff		<= FM_BMC_ONCTL_N;			
					
					rST_S3_q 				<= rST_S3_d;
				end
	end



//////////////////////////////////////////////////////////////////////////////////
// Case for modify the counter depending of state 	
//////////////////////////////////////////////////////////////////////////////////

	always @ (*) begin         
		case( rCurrSt )	
		ST_MEM           :   setCount = T_10MS_2M [bitH:0];
		ST_FAULT         :   setCount = T_5S_2M [bitH : 0];	
		default          :   setCount = T_5S_2M [bitH : 0];			
		endcase 
	end    

//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////////////////

	// Counters 

	counter2 #( .MAX_COUNT(T_5S_2M) ) 
			counter  
			
	(       .iClk         	(iClk), 
			.iRst_n       	(iRst_n),		
			.iCntRst_n  	(wCntRst_n),
			.iCntEn      	(wCntRst_n & wCntEn),
			.iLoad        	(!(wCntRst_n & wCntEn)),
			.iSetCnt    	(setCount),		
			.oDone        	(wCntDoneX),
			.oCntr         	(             )
	);

	counter2 #( .MAX_COUNT(T_2S_2M) ) 
		WatchDogTimer 
	(       .iClk         	(iClk), 
			.iRst_n       	(iRst_n),
			.iCntRst_n  	(wCntRst_n),
			.iCntEn      	(wCntRst_n & wWatchDogEn),
			.iLoad        	(!(wCntRst_n & wWatchDogEn)),
			.iSetCnt    	(T_2S_2M [23 : 0]),  // set the time out time as 2s
			.oDone        	(wTimeOutX),
			.oCntr         	(             )
	);

	counter2 #( .MAX_COUNT(T_2S_2M) ) 
		WatchDogTimer_PSU 
	(       .iClk         	(iClk), 
			.iRst_n       	(iRst_n),
			.iCntRst_n  	(wCntRst_n),
			.iCntEn      	(wCntRst_n & wPsuWatchDogEn),
			.iLoad        	(!(wCntRst_n & wPsuWatchDogEn)),
			.iSetCnt    	(T_2S_2M [23 : 0]),  // set the time out time as 2s
			.oDone        	(wPsuTimeOutX),
			.oCntr         	(             )
	);	

	
//////////////////////////////////////////////////////////////////////
// Local functions 
//////////////////////////////////////////////////////////////////////

// This converter is used to show the PAS states in the 7-segment

	always @ (*)
		begin
			case (rCurrSt)
				ST_FAULT: 			oDbgMstSt7Seg = 4'hF;
				ST_STBY: 			oDbgMstSt7Seg = 4'hA;
				ST_OFF: 			oDbgMstSt7Seg = 4'h9;
				ST_S3:  			oDbgMstSt7Seg = 4'h8;
				ST_PS:  			oDbgMstSt7Seg = 4'h7;
				ST_MAIN:            oDbgMstSt7Seg = 4'h6;
				ST_MEM: 	        oDbgMstSt7Seg = 4'h5;
				ST_CPU:     		oDbgMstSt7Seg = 4'h4;
				ST_SYSPWROK: 		oDbgMstSt7Seg = 4'h3;
				ST_CPUPWRGD: 		oDbgMstSt7Seg = 4'h2;
				ST_RESET: 			oDbgMstSt7Seg = 4'h1; 
				ST_DONE: 			oDbgMstSt7Seg = 4'h0;
				ST_SHUTDOWN: 		oDbgMstSt7Seg = 4'hB;
				ST_MAIN_OFF:        oDbgMstSt7Seg = 4'hC;
				ST_PS_OFF: 			oDbgMstSt7Seg = 4'hD;
				default: 			oDbgMstSt7Seg = 4'hE;      
			endcase
		end

endmodule 