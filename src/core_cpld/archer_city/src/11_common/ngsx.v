`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Serial GPIO (parameterized) module</b> 
    \details    This is the slave implementation of the GSX (SGPIO) module\n
                Receives a clk signal and an array of bytes (n*8 bits, where n is the\n
                number of bytes/registers to be handled).\n
                Each register is serialized and transmitted using the provided clk\n
                Serialized data is parellelized (8 bits) and accomodated in the \n
                n*8 output array to the internal logic \n
 
    \file       NGSX.v
    \author     amr/a.larios@intel.com
    \date       Aug 9, 2019
    \brief      $RCSfile: NGSX.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: NeonCity.v.rca $
                
                <b>Project:</b> Common Core\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   
                <b>Block Diagram:</b>
    \verbatim
        +-------------------------------------------------------------+
 -----> |> iClk       .     .               oPdataOut[(REGS*8 - 1):0] |=====>
 -----> |  iRst_n     .     .               oSdataOut                 |----->
 -----> |  iSdataIn  .      .      .    .     .     .     .     .     |
 =====> |  iPData[(REGS*8 - 1):0]     .     .     .     .     .     |
        +-------------------------------------------------------------+
                           GSX
    \endverbatim
    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/



module ngsx #(parameter IN_BYTE_REGS = 1,                                     //to define the amount of byte registers that are received by this slave
              parameter OUT_BYTE_REGS = 1)                                    //to define the amount of byte registers that are transmitted by this slave
(
 //% active low reset signal (driven from internal FPGA logic)
    input               iRst_n,
 //% serial clock (driven from SGPIO master)
    input               iClk,
 //% Load signal (driven from SGPIO master to capture serial data input in parallel register)
    input               iLoad,
 //% Serial data input (driven from SGPIO master)
    input               iSData,
 //% Parallel data from internal logic to master
    input [(OUT_BYTE_REGS*8)-1:0] iPData,
 //% Serial data output to SGPIO master
    output                       oSData,
 //% Parallel data to internal registers (slave)
    output reg [(IN_BYTE_REGS*8)-1:0] oPData
);
      

// Internal Signals


   reg [(IN_BYTE_REGS*8)-1:0] rSToPAcc;   //Serial to Parallel Accumulator (for serial data from SGPIO master). Goes to internal registers
   reg [(OUT_BYTE_REGS*8)-1:0] rPDataIn;   //parallel data input register (to latch data before serializing), goes to SGPIO master
   

//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
   
   assign oSData = rPDataIn[(OUT_BYTE_REGS*8) - 1];        //serial output is the MSb of the shifted register
      

//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////


always @(posedge iClk)
  begin
     if (iClk)
       begin
          if (!iRst_n)   //synchronous reset condition for outputs and internal signals
            begin
               rSToPAcc <= {IN_BYTE_REGS*8{1'b0}};
               oPData <= {IN_BYTE_REGS*8{1'b0}};
               rPDataIn <= {OUT_BYTE_REGS*8{1'b0}};
            end //if (!rst_n)
          else //not(if (!rst_n))
            begin

               //logic for serial data coming from SGPIO Master that goes to internal logic
               rSToPAcc = {rSToPAcc[(IN_BYTE_REGS*8)-2:0], iSData};
               
               if (!iLoad)  //parallel data is captured to start serialization for data that goes to SGPIO Master
                 begin
                    rPDataIn <= iPData;    //parallel data is captured in shift register to be serialized when Load==0
                    oPData = rSToPAcc;    //parallel output is driven when Load==0 
                 end //if (!iLoad)
               else 
                 begin
                    rPDataIn[OUT_BYTE_REGS*8 - 1:0] <= {rPDataIn[OUT_BYTE_REGS*8 - 2:0], 1'b0};   //shifting register to serialize parallel input
                 end //not(if (!iLoad))

            end //else (not(if (!rst_n)))
       end // if (clk)
     
  end //always

endmodule //module NGSX
