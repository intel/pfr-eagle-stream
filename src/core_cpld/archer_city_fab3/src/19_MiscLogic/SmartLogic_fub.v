//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Smart-LogicFunctional Unit Block</b> 
    \details    
 
    \file       SmartLogic_fub.v
    \author     amr/a.larios@intel.com
    \date       Sept 7, 2019
    \brief      $RCSfile: SmartLogic_fub.v.rca $
                $Date:  $
                $Author:  Alejandro Larios$
                $Revision:  $
                $Aliases: $
                
                <b>Project:</b> Archer City (EagleStream)\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   
                <b>Block Diagram:</b>
 
    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/


module SmartLogic_fub
  
  (
   input iFmThrottle_n,                //from PCH
   input iIrqSml1PmbusAlert_n,         //from PDB
   input iFmPmbusAlertBEna,            //from PCH
   
   output oFmSysThrottle_n             //to PCIe Slots (B,D & E) and Risers 1 & 2, as well as internally in the FPGA to MISC (for MEMHOT, PROCHOT of both CPUs)
   );

   wire   wPmbusAlert_n;
   

   //////////////////////////////////
   //output assignments
   //////////////////////////////////////////////////////////////////

   assign wPmbusAlert_n = iIrqSml1PmbusAlert_n || ~iFmPmbusAlertBEna;
   assign oFmSysThrottle_n = iFmThrottle_n && wPmbusAlert_n;     //asserting FM_SYS_THROTTLE_N with any of inputs beign asserted
   

   
   

endmodule // SmartLogic_fub


   