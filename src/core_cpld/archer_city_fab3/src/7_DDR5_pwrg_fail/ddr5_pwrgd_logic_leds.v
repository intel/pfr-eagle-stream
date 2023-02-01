/* ==========================================================================================================
   = Project     :  Archer City Power Sequencing
   = Title       :  DDR5 PWRGD_FAIL LEDs
   = File        :  DDR5_pwrgd_logic_LEDs.v
   ==========================================================================================================
   = Author      :  Pedro Saldana
   = Division    :  BD GDC
   = E-mail      :  pedro.a.saldana.zepeda@intel.com
   ==========================================================================================================
   = Updated by  :
   = Division    :
   = E-mail      :
   ==========================================================================================================
   = Description :  This module controls the behavior of PWRGD and FAIL LEDs for each DDR5 Memory controller


   = Constraints :

   = Notes       :

   ==========================================================================================================
   = Revisions   :
     July 15, 2019;    0.1; Initial release
   ========================================================================================================== */
module ddr5_pwrgd_logic_leds
(
	input 		iClk,			  //Clock input.
	input 	    iRst_n,			  //Reset

    input      [3:0]  iCpuMemFlt,     //Input LED errors
	output reg [3:0]  oCpuMemFlt 	  //Latched LEDs error
);

//////////////////////////////////////////////////////////////////////////////////
// States for root state machine
//////////////////////////////////////////////////////////////////////////////////
     localparam ST_INIT              = 4'd0;
     localparam ST_LATCH_OK          = 4'd1;
//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
     localparam  LOW  = 1'b0;
     localparam  HIGH = 1'b1;

     reg [1:0] root_state;        //main sequencing state machine

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments    /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
     //State machine for DDR5 MEM PWRDG FAIL
     always @ (posedge iClk, negedge iRst_n)
     begin
          if(!iRst_n)                             //asynchronous, negative-edge reset
          begin
      		 oCpuMemFlt  <= 4'b0000;
             root_state <= ST_INIT;               //added by alarios 04/22/20
          end
          else
          begin
               case(root_state)
                 ST_INIT: begin
                    oCpuMemFlt <= iCpuMemFlt;       //alarios 04/22/20      
                    if(iCpuMemFlt) 
                      begin 
                         //oCpuMemFlt <= iCpuMemFlt;  //alarios 04/22/20
                         root_state <= ST_LATCH_OK;
                      end
                 end
                 
                 ST_LATCH_OK: begin
					oCpuMemFlt <= oCpuMemFlt;		    
                 end
                 
                 default:
                   begin
                      root_state <= ST_INIT;
                   end
               endcase // case (root_state)
          end // else: !if(!iRst_n)
     end // always @ (posedge iClk, negedge iRst_n)

endmodule