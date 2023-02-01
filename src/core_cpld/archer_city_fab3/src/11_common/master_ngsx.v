`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Serial GPIO (parameterized) module</b> 
    \details    This is the master implementation of the GSX (SGPIO) module\n
                
 
                Receives a clk signal from internal PLL dedicated for SGPIO I/F 
                 (the logic then should be clocked by this clk signal and it will be used
                 also as the SGPIO clk signal fwded to SGPIO slave logic)
                Also receives an array of bytes (n*8 bits, where n is the
                 number of bytes/registers to be output
                Each register is serialized and transmitted using the mentioned clk signal
 
                The module also receives serial data (from an SGPIO Slave)
                 this data is parellelized (8 bits) and accomodated in the
                 n*8 output array to the internal logic
 
                Finally a load pulse (active-low) is generated. When LOW, the next clock cycle 
                 will transmit the 1st bit of the serial data. 
                It also helps the slave to capture its parallel data and start serializing 
                 to the SGPIO master block
 
    \file       MASTER_NGSX.v
    \author     amr/a.larios@intel.com
    \date       Dec 13, 2019
                

    \copyright  Intel Proprietary -- Copyright 2019 Intel -- All rights reserved
*/



module master_ngsx #(parameter BYTE_REGS = 1)                                     //to define the amount of byte registers that are Tx/Rx by this master module
   (
    //% active low reset signal (driven from internal FPGA logic)
    input                          iRst_n,
    //% serial clock (driven from internal PLL). SGPIO logic is clocked with this and used as SGPIO output as well
    input                          iClk,
    //% Load signal (to SGPIO slave signals the 1st Tx bit and slave uses to latch parallel data as well)
    output reg                     oLoad_n,
    //% Parallel data from internal logic to SGPIO slave
    input [(BYTE_REGS*8)-1:0]      iPData,
    //% Serial data output to SGPIO slave
    output                         oSData,
    //% Serial data input (driven from SGPIO slave)
    input                          iSData,

    //% output clock signal (same as input, assigned here to make it in a modular way)
    output                         oClk,
    //% Parallel data to internal registers (master internal logic, clocked with SGPIO clk
    output reg [(BYTE_REGS*8)-1:0] oPData
    );

   ////////////////////////////////////////////////////////////////
   // local function to calculate the log-base2 of a number
   ///////////////////////////
   
   function integer clog2;
      input integer                   value;
      begin
         value = value-1;
         for (clog2=0; value>0; clog2=clog2+1)
           value = value>>1;
      end
   endfunction
 

   ///////////////////////////////////////////////////////////////
   // Local Param definitions
   ///////////////////////////
   localparam CNT_SIZE = clog2(BYTE_REGS*8);     //determines the bits required for the LDCNT 
   localparam MAX_LDCNT = BYTE_REGS*8 - 1;     //determines the MAX cNT value for the LDCNT

   
   // Internal Signals
   
   
   reg [(BYTE_REGS*8)-1:0]         rSToPAcc;   //Serial to Parallel Accumulator (for serial data from SGPIO slave). Goes to internal registers
   reg [(BYTE_REGS*8)-1:0]         rPDataIn;   //parallel data input register (to latch data before serializing), goes to SGPIO slave

   reg [CNT_SIZE: 0]               ld_cnt;      //helps to generate the load pulse
         

   //////////////////////////////////////////////////////////////////////////////////
   // Continous assigments
   ////////////////////////
   
   assign oSData = rPDataIn[(BYTE_REGS*8) - 1];        //serial output is the MSb of the shifted register
      

   //////////////////////////////////////////////////////////////////////////////////
   // Sequential logic
   ///////////////////////
   
   
   always @(negedge iClk, negedge iRst_n)
     begin
        if (!iRst_n)   //Asynchronous reset condition for outputs and internal signals
          begin
             ld_cnt <= MAX_LDCNT;
             oLoad_n <= 1'b1;
             oPData <= {BYTE_REGS*8{1'b0}};                   
             rPDataIn <= {BYTE_REGS*8{1'b0}};
             rSToPAcc <= {BYTE_REGS*8{1'b0}};
          end
        else //not(if (!rst_n))
          begin
             
             if (ld_cnt == 0)                       //load counter (decremental), wrap-around if = 0 
               begin
                  ld_cnt <= MAX_LDCNT;
                  oLoad_n <= 1'b1;                  //load output should be LOW at this moment, making it HIGH for the next clk cycle
               end
             else
               begin
                  ld_cnt <= ld_cnt - 1;             //we need to count
                  if (ld_cnt == 1)                  
                    begin                           //means next cycle, we are an MIN COUNT value and load has to be asserted then
                       oLoad_n <= 1'b0;               
                    end
                  else
                    begin                           //load has to be de-asserted (HIGH)
                       oLoad_n <= 1'b1;             
                    end
               end // else: !if(ld_cnt == 0)

             //logic for serial data coming from SGPIO Master that goes to internal logic
             rSToPAcc <= {rSToPAcc[(BYTE_REGS*8)-2:0], iSData};
             
             if (!oLoad_n)  //parallel data is captured to start serialization for data that goes to SGPIO Master
               begin
                  rPDataIn <= iPData;    //parallel data is captured in shift register to be serialized when Load==0
                  oPData <= rSToPAcc;    //parallel output is driven when Load==0 
               end //if (!oLoad_n)
             else 
               begin
                  rPDataIn[BYTE_REGS*8 - 1:0] <= {rPDataIn[BYTE_REGS*8 - 2:0], 1'b0};   //shifting register to serialize parallel input
               end //not(if (!oLoad_n))
          end // else: !if(iRst_n)
        
     end // always @ (negedge iClk, negedge iRst_n)


   //output assignments

   assign oClk = iClk;
   
endmodule // master_ngsx


