//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%SysCheck </b>
    \file     SysCheck.v
    \details    <b>Image of the Block:</b>
                \image html SysCheck.png

                 <b>Description:</b> \n
                The module contain all logic needed for SysCheck logic\n\n 

                Checks if the CPU ID and Package IDs match the correct way in the platform.\n

                The true table was provide by PAS 0.6v

                \image html truetable.png
                
    \brief  <b>Last modified</b> 
            $Date:   Jan 19, 2018 $
            $Author:  David.bolanos@intel.com $         
            Project         : Wilson City RP  
            Group           : BD
    \version    
             20160609 \b  David.bolanos@intel.com - File creation\n
             20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Neon city\n 
			 20180223 \b  jorge.juarez.campos@intel.com - Added support for STP Test Card.\n
             20181129 \b  jorge.juarez.campos@intel.com - Modified to match with latest PAS docuemnt
               
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module SysCheck
(
    //% Clock input
    input           iClk,
    //% Asynchronous reset
    input           iRst,
    //% Socket occupied
    input   [1:0]   invCPUSktOcc,
    //% CPU0 Processor ID
    input   [1:0]   ivProcIDCPU1,
    //% CPU1 Processor ID    
    input   [1:0]   ivProcIDCPU2,    
    //% CPU0 Package ID
    input   [2:0]   ivPkgIDCPU1,
    //% CPU1 Package ID    
    input   [2:0]   ivPkgIDCPU2,
    //% interposer present
    input   [1:0]   ivIntr,
    //% Auxiliar Power Ready
    input   		iAuxPwrDone,
    //% System validation Ok
    output          oSysOk,
    //% CPU Mismatch - priority 
    output          oCPUMismatch,  
    //% MCP Clock Control
    output          oMCPSilicon,
    //% Socket Removed
    output          oSocketRemoved
);
`define ICX     2'b00
`define CPX     2'b01
`define STP     2'b11


`define NON_MCP     3'b000
`define XCC_CPX4    3'b001
`define CPX6_CPU    3'b010
`define STP_CPU     3'b111

reg         rSysOk_d;
reg         rSysOk_q;

reg         rProcIDErr_d;
reg         rProcIDErr_q;

reg         rProcPkgErr_d;
reg         rProcPkgErr_q;

reg         rXCC_CPX4_d;
reg         rXCC_CPX4_q;

reg         rSocketRemoved_d;
reg         rSocketRemoved_q;

wire 		wSocket1Removed;
wire 		wSocket2Removed;

wire 		wSocket1RemovedLatch;
wire 		wSocket2RemovedLatch;

assign  oSysOk          =   rSysOk_q;
assign  oCPUMismatch    =   rProcIDErr_q || rProcPkgErr_q;
assign  oMCPSilicon     =   rXCC_CPX4_q;
assign  oSocketRemoved  =   rSocketRemoved_q;


always @(posedge iClk or posedge iRst )
begin
    if ( iRst )
    begin      
        rSysOk_q            <=  1'b0;
        rProcIDErr_q        <=  1'b0;
        rProcPkgErr_q       <=  1'b0;
        rXCC_CPX4_q         <=  1'b0;
        rSocketRemoved_q    <=  1'b0;
    end
    else
    begin
        rSysOk_q            <=  rSysOk_d;
        rProcIDErr_q        <=  rProcIDErr_d;
        rProcPkgErr_q       <=  rProcPkgErr_d;
        rXCC_CPX4_q         <=  rXCC_CPX4_d;
        rSocketRemoved_q    <=  rSocketRemoved_d;
    end
end


always @*
begin
    rSysOk_d        =   1'b0;
    rProcIDErr_d    =   1'b0;
    rProcPkgErr_d   =   1'b0;
    rXCC_CPX4_d     =   1'b0;
    if ( invCPUSktOcc[0] == 1'b0 )
    begin
        casex ( {invCPUSktOcc[1], ivIntr} )
            3'b1_x1: begin  rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0; end   //With interposer. Only CPU1 Present
            3'b0_11: begin  rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0; end   //With interposer on both CPUs present

            3'b1_x0:  //No interposer. Only CPU1 Present
            begin
                // PROC ID + PKG ID valid
                case ( {ivProcIDCPU1, ivPkgIDCPU1})
                    {`ICX, `NON_MCP}:   begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b0; end     //ICX-HCC/ICX-LCC (Non-MCP)
                    {`ICX, `XCC_CPX4}:  begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //ICX-XCC (MCP required)
                    {`CPX, `XCC_CPX4}:  begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //ICX-4 (MCP enabled)
                    {`STP, `STP_CPU}:   begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //STP (MCP enabled)
                    default:            begin   rProcIDErr_d = 1'b1;    rProcPkgErr_d = 1'b1;   rXCC_CPX4_d = 1'b0; end     //All other are invalid
                endcase
            end

            3'b0_00:  //No interposer
            begin
                // PROC ID + PKG ID valid
                case ({ivProcIDCPU2,ivProcIDCPU1,ivPkgIDCPU2,ivPkgIDCPU1})
                    {`ICX, `ICX, `NON_MCP,  `NON_MCP}:      begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b0; end     //ICX-XCC/ICX-HCC/ICX-LCC (Non-MCP)
                    {`ICX, `ICX, `NON_MCP,  `XCC_CPX4}:     begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)
                    {`ICX, `ICX, `XCC_CPX4, `NON_MCP}:      begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)
                    {`ICX, `ICX, `XCC_CPX4, `XCC_CPX4}:     begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)

                    {`CPX, `CPX, `XCC_CPX4, `XCC_CPX4}:     begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //CPX-4 (MCP Enabled)

                    {`STP, `STP, `STP_CPU,  `STP_CPU}:      begin   rProcIDErr_d = 1'b0;    rProcPkgErr_d = 1'b0;   rXCC_CPX4_d = 1'b1; end     //STP Test Card (MCP Enabled)

                    default:                                begin   rProcIDErr_d = 1'b1;    rProcPkgErr_d = 1'b1;   rXCC_CPX4_d = 1'b0; end     //All other are invalid
                endcase
            end
            default:
            begin
                rProcIDErr_d    =   1'b1;
                rProcPkgErr_d   =   1'b1;
                rXCC_CPX4_d     =   1'b0;
            end
        endcase
    end
    rSysOk_d            =   ~rProcIDErr_q & ~rProcPkgErr_q;		//No PKG and PROC ID error and at least CPU0 present
    rSocketRemoved_d    =   (wSocket1Removed || wSocket2Removed) && (!iRst);
end


EdgeDetector #
(
    .EDGE                   ( 1'b1 )
) mSocket1Removed
(
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iSignal                ( invCPUSktOcc[0] ),
    .oEdgeDetected          ( wSocket1RemovedLatch )
);

FF_SR mLatchOut1
(
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iSet                   ( wSocket1RemovedLatch && iAuxPwrDone),
    .oQ                     ( wSocket1Removed )
);

EdgeDetector #
(
    .EDGE                   ( 1'b1 )
) mSocket2Removed
(
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iSignal                ( invCPUSktOcc[1] ),
    .oEdgeDetected          ( wSocket2RemovedLatch )
);

FF_SR mLatchOut2
(
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iSet                   ( wSocket2RemovedLatch && iAuxPwrDone),
    .oQ                     ( wSocket2Removed )
);

endmodule
