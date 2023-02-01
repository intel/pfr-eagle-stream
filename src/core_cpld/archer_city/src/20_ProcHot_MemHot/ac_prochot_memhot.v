/* ==========================================================================================================
   = Project     :  Archer City Power Sequencing
   = Title       :  PROCHOT and MEMHOT Control
   = File        :  ac_prochot_memhot.v
   ==========================================================================================================
   = Author      :  Pedro Saldana
   = Division    :  BD GDC
   = E-mail      :  pedro.a.saldana.zepeda@intel.com
   ==========================================================================================================
   = Updated by  :  
   = Division    :  
   = E-mail      :  
   ==========================================================================================================
   = Description :  This modules controls the prochot and memohot signals to CPUs


   = Constraints :  
   
   = Notes       :  
   
   ==========================================================================================================
   = Revisions   :  
     July 15, 2019;    0.1; Initial release
   ========================================================================================================== */
   module ac_prochot_memhot
(
    //Inputs
    input      iIRQ_CPU_MEM_VRHOT_N,
    input      iIRQ_PSYS_CRIT_N,
    input      iIRQ_CPU_VRHOT_LVC3_N,
    input      iFM_SYS_THROTTLE_LVC3_N, 
    //Outputs
    output     oFM_PROCHOT_LVC3_N,
    output     oFM_H_CPU_MEMHOT_N
);


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////

  parameter  LOW =1'b0;
  parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////

assign oFM_PROCHOT_LVC3_N  = (iFM_SYS_THROTTLE_LVC3_N && iIRQ_CPU_VRHOT_LVC3_N && iIRQ_PSYS_CRIT_N)      ? HIGH : LOW;
assign oFM_H_CPU_MEMHOT_N  = (iFM_SYS_THROTTLE_LVC3_N && iIRQ_PSYS_CRIT_N      && iIRQ_CPU_MEM_VRHOT_N)  ? HIGH : LOW;

endmodule