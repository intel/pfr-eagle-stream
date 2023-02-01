/* ==========================================================================================================
   = Project     :  Archer City
   = Title       :  Thermal Trip Delay
   = File        :  ac_thermtrip_dly.v
   ==========================================================================================================
   = Author      :  Pedro Saldana
   = Division    :  BD GDC
   = E-mail      :  <pedro.a.saldana.zepeda@intel.com>
                    
   ==========================================================================================================
   = Updated by  :  
   = Division    :  
   = E-mail      :  
   ==========================================================================================================
   = Description :  

   = Constraints :  
   
   = Notes       :  
   
   ==========================================================================================================
   = Revisions   :  
     July 15, 2019;    0.1; Initial release
   ========================================================================================================== */


module ac_thermtrip_dly
(
    input  iClk_2M,                   //2MHz Clock Input
    input  iRst_n,
    input  iTherm_Trip_Dly_En,
    input  iFM_CPU0_THERMTRIP_LVT3_N,
    input  iFM_CPU1_THERMTRIP_LVT3_N,
    input  iFM_MEM_THERM_EVENT_CPU0_LVT3_N,
    input  iFM_MEM_THERM_EVENT_CPU1_LVT3_N,
    input  iFM_CPU1_SKTOCC_LVT3_N,

    output oFM_THERMTRIP_DLY_N
);
    parameter  T_100US_2M  =  8'd200;
    wire wThermtripDly_100us;
    wire wCpuThermtrip_n;

    
    assign wCpuThermtrip_n = iFM_CPU0_THERMTRIP_LVT3_N       && iFM_MEM_THERM_EVENT_CPU0_LVT3_N && 
                            (iFM_CPU1_THERMTRIP_LVT3_N       || iFM_CPU1_SKTOCC_LVT3_N) && // Mask logic to mask CPU0 thermtrip event if CPU1 
                            (iFM_MEM_THERM_EVENT_CPU1_LVT3_N || iFM_CPU1_SKTOCC_LVT3_N);   // is not present

    assign oFM_THERMTRIP_DLY_N = iTherm_Trip_Dly_En? (~wThermtripDly_100us): wCpuThermtrip_n ; 
	


    genCntr #( .MAX_COUNT(T_100US_2M)) 
    thermtripDelayCounter    
    (
        .oCntDone     (wThermtripDly_100us),     // It is high when cnt == MAX_COUNT.   
       
        .iClk         (iClk_2M),  
        .iRst_n       (iRst_n ),		               
        .iCntEn       (~wCpuThermtrip_n),	  
        .iCntRst_n    ((~wCpuThermtrip_n) || (~wThermtripDly_100us)),
	    .oCntr        ( /*empty*/   )
    ); 	 


endmodule