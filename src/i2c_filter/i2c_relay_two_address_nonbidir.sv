// (C) 2019 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.

`timescale 1 ps / 1 ps
`default_nettype none

module i2c_relay_two_address_nonbidir #(
  parameter SMBUS_ALLOW_ALL        = 1,
  parameter SMBUS_FILTER_COMMAND   = 0,
  parameter SMBUS_SLAVE_ADDRESS_1  = 0,
  parameter USE_ADDRESS_2          = 0,
  parameter SMBUS_SLAVE_ADDRESS_2  = 0,
  parameter USE_ADDRESS_3          = 0,
  parameter SMBUS_SLAVE_ADDRESS_3  = 0,
  parameter USE_ADDRESS_4          = 0,
  parameter SMBUS_SLAVE_ADDRESS_4  = 0,
  parameter USE_ADDRESS_5          = 0,
  parameter SMBUS_SLAVE_ADDRESS_5  = 0,
  parameter USE_ADDRESS_6          = 0,
  parameter SMBUS_SLAVE_ADDRESS_6  = 0,
  parameter USE_ADDRESS_7          = 0,
  parameter SMBUS_SLAVE_ADDRESS_7  = 0,
  parameter SCL_FORCE_STRETCH      = 1
) (
   input logic       vln_soc_reset_n,
   input logic       vln_core_clk,
   input wire        i2c_scl,
   input wire        i2c_sda,
   output wire       i2c_scl_oe,
   output wire       i2c_sda_oe,
   inout wire        i2c_slave_scl,
   inout wire        i2c_slave_sda,
   input logic       smbus_cmd_whitelist[255:0],
   input logic       i2cm_override
);

   logic  stop_condition, start_condition, stop_condition_slv, error_condition; 
   logic [2:0] i2c_scl_cclk,i2c_slave_scl_cclk;
   logic [2:0] i2c_sda_cclk,i2c_slave_sda_cclk;
   logic [7:0] i2c_byte_regsiter, bit_shift_register;
   logic [2:0] vln_cpld_i2c_ack_b;
   logic [7:0] read_data_reg;
   logic vld_i2c_clk_l2h, vld_i2c_clk_h2l, write_operation, read_operation, read_phase,
         write_txn_sticky, read_txn_sticky, byte_phase_end, log_byte_enable ;
   logic [4:0] [7:0] log_bytes;
   logic my_device, generate_ack, terminate_txn, read_enable_slave, vln_pointer_device;
   logic not_my_transaction, busy, busy_l, slvscl, slvsda;
   logic mysda, myscl, last_sda;
   logic myscl_posedge_enable, myscl_negedge_enable;
   logic slvscl_posedge_enable, slvscl_negedge_enable;
   logic [1:0] myscl_posedge_dly;

   logic send_read_data, send_passthro_data;
   logic [6:0] device_addr_1, device_addr_2, device_addr_3, device_addr_4, device_addr_5, device_addr_6, device_addr_7;
   logic [3:0] stream_txn_total_bytes;
   logic reset_txn_count, capture_mailbox;
   logic read_cpldreg_load, read_buffer_load;
   logic pull_master_clock_low,pull_slave_clock_low;
   logic ack_expected_combo;
   logic pull_master_sda, pull_slave_sda;
   logic pull_master_scl, pull_slave_scl;
   logic slvscl_stretch, slvscl_stretch_ff;
   logic hold_slvscl, hold_mstscl;
   logic [1:0] stretch_timeout;


   enum logic [2:0] {IDLE=3'd0, 
                     DEV_ADDR_OPTYPE= 3'd1, 
                     WRITE_BYTES    = 3'd2,
                     READ_BYTES     = 3'd3,
                     PHASEPARK      = 3'd4,
                     DEV_COMMAND    = 3'd5
                    } i2c_phase;

   assign device_addr_1 = SMBUS_SLAVE_ADDRESS_1[7:1];
   assign device_addr_2 = SMBUS_SLAVE_ADDRESS_2[7:1];
   assign device_addr_3 = SMBUS_SLAVE_ADDRESS_3[7:1];
   assign device_addr_4 = SMBUS_SLAVE_ADDRESS_4[7:1];
   assign device_addr_5 = SMBUS_SLAVE_ADDRESS_5[7:1];
   assign device_addr_6 = SMBUS_SLAVE_ADDRESS_6[7:1];
   assign device_addr_7 = SMBUS_SLAVE_ADDRESS_7[7:1];

   logic       last_byte;
   logic [3:0] reset_count;

   typedef struct packed {
      logic [7:0] high;
      logic [7:0] low;
   } t_address_pointer;

   typedef enum logic[1:0]{
      NOOP            = 2'b0,
      READ_OPERATION  = 2'b1,
      WRITE_OPERATION = 2'd2
   } t_i2c_optype;

   t_i2c_optype i2c_operation, operation;
   logic reset_start_count;
   
  logic abort_transaction, abort_scl, abort_sda, hold_master_on_abort;
  logic smbus_cmd_valid;
  logic smbus_cmd_block_candidate;
  logic smbus_write_op, smbus_read_op;

  assign smbus_write_op = i2c_operation == WRITE_OPERATION;
  assign smbus_read_op = i2c_operation == READ_OPERATION;

   enum logic [3:0] {BIT7=4'd0, BIT6=4'd1, BIT5=4'd2, BIT4=4'd3, BIT3=4'd4, BIT2=4'd5, BIT1=4'd6, BIT0=4'd7, ACK=4'd8, BITPARK=4'd9} i2c_bit_tracker, count;

//#############################################################
// CLock Synchronization:
// Get the SMBUS clock and data synchronized to the local clock
//#############################################################
   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n)	
      begin
         if (~vln_soc_reset_n)
            begin
               i2c_scl_cclk[2:0] <= '1;
               i2c_sda_cclk[2:0] <= '1;
            end
         else 
            begin
               i2c_scl_cclk[2:0] <= {i2c_scl_cclk[1:0],i2c_scl};
               i2c_sda_cclk[2:0] <= {i2c_sda_cclk[1:0],i2c_sda};
            end
      end

//#############################################################
// CLock Synchronization for SMBUS return path:
// Get the SMBUS clock and data synchronized to the local clock
//#############################################################
   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n)	
      begin
         if (~vln_soc_reset_n)
            begin
               i2c_slave_scl_cclk[2:0] <= '1;
               i2c_slave_sda_cclk[2:0] <= '1;
            end
         else 
            begin
               i2c_slave_scl_cclk[2:0] <= {i2c_slave_scl_cclk[1:0],i2c_slave_scl};
               i2c_slave_sda_cclk[2:0] <= {i2c_slave_sda_cclk[1:0],i2c_slave_sda};
            end
      end

//#############################################################
// SMBUS start and stop condition detection
//############################################################

   always_comb// @(posedge vln_core_clk)
      begin
      
      // Incoming SMBUS from master
      //
         myscl  = i2c_scl_cclk[2];
         mysda  = i2c_sda_cclk[2];
      
      // Return SMBUS from master
      //
         slvscl = i2c_slave_scl_cclk[2];
         slvsda = i2c_slave_sda_cclk[2];
      
      // Clock edge detection
      //
         myscl_posedge_enable = i2c_scl_cclk[0] &  i2c_scl_cclk[1] & ~i2c_scl_cclk[2];
         myscl_negedge_enable = i2c_scl_cclk[2] & ~i2c_scl_cclk[1] & ~i2c_scl_cclk[1];

         slvscl_posedge_enable = i2c_slave_scl_cclk[0] &  i2c_slave_scl_cclk[1] & ~i2c_slave_scl_cclk[2];
         slvscl_negedge_enable = i2c_slave_scl_cclk[2] & ~i2c_slave_scl_cclk[1] & ~i2c_slave_scl_cclk[0];
      end

   always_comb
      begin
         
         start_condition = ((&i2c_scl_cclk[2:1]) & i2c_sda_cclk[2] & ~i2c_sda_cclk[1]);
         
         stop_condition  = ((&i2c_scl_cclk[2:1]) & ~i2c_sda_cclk[2] & i2c_sda_cclk[1] & busy);

         stop_condition_slv  = ((&i2c_slave_scl_cclk[2:1]) & ~i2c_slave_sda_cclk[2] & i2c_slave_sda_cclk[1] & busy);

         error_condition = ((i2c_slave_scl_cclk == 3'b111) && (i2c_scl_cclk == 3'b000) && (i2c_slave_sda_cclk == 3'b111) && (i2c_sda_cclk[1] & ~i2c_sda_cclk[0])) && ~abort_transaction && my_device;
                           
      end

//   logic nack;
//   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n) begin
//     if(~vln_soc_reset_n) begin
//       nack <= 1'b0;
//     end else begin
//       if(start_condition || stop_condition) begin
//         nack <= 1'b0;
//       end else if((i2c_bit_tracker == ACK) && myscl_negedge_enable) begin
//         nack <= slvsda;
//       end
//     end  
//   end

//###############################################################################
// SMBUS Transaction busy & start count (used for repeat start) detection logic
//###############################################################################
   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n)
      if (~vln_soc_reset_n)
         begin
            busy               <= 1'b0;
            i2c_bit_tracker  <= BITPARK;
            bit_shift_register <= 'hff;
            i2c_operation    <= NOOP;
            i2c_phase        <= PHASEPARK;
            my_device          <= 0;
            generate_ack       <= 1'b0;
            not_my_transaction <= 0;
            last_byte          <= '0;
            send_read_data     <= '0;
            send_passthro_data <= '1;
         end
      else if (stop_condition)
         begin
            busy <= 1'b0;
            i2c_bit_tracker  <= BITPARK;
            i2c_operation    <= NOOP;
            bit_shift_register <= 'hff;
            i2c_phase        <= PHASEPARK;
            my_device          <= 0;
            not_my_transaction <= 0;
            generate_ack       <= 1'b0;
            last_byte          <= '0;
            send_read_data     <= '0;
            send_passthro_data <= '1;
         end
      else if (start_condition)
         begin
            
            busy               <= 1'b1;
            
         // Reset all the critical FSM state elements again at start condition to make sure that FSM is properly initialized for the first
         // transaction
            i2c_bit_tracker  <= BITPARK;
            i2c_operation      <= NOOP;
            bit_shift_register <= 'hff;
            i2c_phase          <= PHASEPARK;
            my_device          <= 0;
            not_my_transaction <= 0;
            generate_ack       <= 1'b0;
            last_byte          <= '0;
            send_read_data     <= '0;
            send_passthro_data <= '1;
         end
      
      else if (busy & myscl_posedge_enable)
         begin
         // Bit shift regsiter. Track and Shift incoming sda bit pattern
         //
            bit_shift_register <= {bit_shift_register[6:0],mysda};

            i2c_bit_tracker  <= ((i2c_bit_tracker == BITPARK) || (i2c_bit_tracker == ACK)) ? BIT7 : (i2c_bit_tracker.next());
            
            
            if (i2c_phase == PHASEPARK)
               begin
                  i2c_phase <= DEV_ADDR_OPTYPE;
               end

            else if (i2c_phase == DEV_ADDR_OPTYPE)
               begin
                  if (i2c_bit_tracker == BIT1)
                     begin
                        if (SMBUS_ALLOW_ALL ||  (
                                                  (bit_shift_register[6:3] == device_addr_1[6:3]) || 
                                                  (bit_shift_register[6:3] == device_addr_2[6:3] && USE_ADDRESS_2) ||
                                                  (bit_shift_register[6:3] == device_addr_3[6:3] && USE_ADDRESS_3) ||
                                                  (bit_shift_register[6:3] == device_addr_4[6:3] && USE_ADDRESS_4) ||
												  (bit_shift_register[6:3] == device_addr_5[6:3] && USE_ADDRESS_5) ||
												  (bit_shift_register[6:3] == device_addr_6[6:3] && USE_ADDRESS_6) ||
												  (bit_shift_register[6:3] == device_addr_7[6:3] && USE_ADDRESS_7) 
												 
                                                ))
                           begin
                              my_device          <= 1'b1;
                              not_my_transaction <= 0;
                           end
                        else
                           begin
                              my_device          <= 1'b0;
                              not_my_transaction <= 1;
                           end
                     end
                  else if (i2c_bit_tracker == BIT0)
                     begin
                     // SET the read/write operation 
                        if (bit_shift_register[0])
                           i2c_operation <= READ_OPERATION;
                        else
                           i2c_operation <= WRITE_OPERATION;
                     end
                  else if (i2c_bit_tracker == ACK)
                     begin
                        if (i2c_operation == READ_OPERATION)
                           i2c_phase <= READ_BYTES;
                        else if (i2c_operation == WRITE_OPERATION)
                          if(SMBUS_FILTER_COMMAND)
                            i2c_phase <= DEV_COMMAND;
                          else  
                            i2c_phase <= WRITE_BYTES;
                     end
               end

            else if (i2c_phase == DEV_COMMAND && SMBUS_FILTER_COMMAND) begin
              if(i2c_bit_tracker == ACK) begin
                // if we're here than there was no "repeated start" for a read command so we're actually processing a write command
                i2c_phase <= WRITE_BYTES;
              end
            end

            else if (i2c_phase == READ_BYTES)
               begin

                  i2c_phase <= READ_BYTES; 
                  
                  // Sample the SDA during the right phase to see if it is a NACK. if NACK, it is the last byte
                  //
                  last_byte   <= (my_device && (i2c_bit_tracker == BIT0) && i2c_sda); 

               end
            
            else if (i2c_phase == WRITE_BYTES)
               begin
                  i2c_phase <= WRITE_BYTES;
               end
            
         end
      else if (busy & myscl_negedge_enable)
         begin

            
            
            send_read_data <= ((i2c_operation==READ_OPERATION) && ~last_byte && my_device &&
                              (((i2c_bit_tracker >= BIT7) && (i2c_bit_tracker <= BIT1))|| (i2c_bit_tracker == ACK))) ;

            // Logic to generate acks for the current transaction bytes
            // Last byte on read transaction is communicated by a NACK from master
            generate_ack <= (((i2c_phase == WRITE_BYTES) || (i2c_phase == DEV_ADDR_OPTYPE) || (i2c_phase == DEV_COMMAND)) && (i2c_bit_tracker == BIT0) && my_device);

            // release during ack phase
     //     send_passthro_data <= (((i2c_phase == DEV_ADDR_OPTYPE) || (i2c_phase == WRITE_BYTES)) && (i2c_bit_tracker != BIT0))
     //                             || (i2c_phase == PHASEPARK) || last_byte  || ((i2c_phase == READ_BYTES) && (i2c_bit_tracker == BIT0));

            send_passthro_data <= ((i2c_phase == DEV_ADDR_OPTYPE) && (i2c_operation!=READ_OPERATION) && (i2c_bit_tracker != BIT0))
                                 || ((i2c_phase == READ_BYTES) && (i2c_bit_tracker == BIT0))
                                 || ((i2c_phase == DEV_COMMAND)&& (i2c_bit_tracker != BIT0))
                                 || ((i2c_phase == WRITE_BYTES)&& (i2c_bit_tracker != BIT0))
                                 || (i2c_phase == PHASEPARK) || last_byte ;
               
         end 

//###########################################################
// SMBUS clock stretching support logic
//###########################################################

   logic [9:0] target, timer;
   logic [8:0] stretch_timer;
   logic last_byte_negedge, my_device_negedge, freq_search, track_low_phase, timeout_low_phase;  

   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n)
      begin
         if (~vln_soc_reset_n)
            begin
               freq_search  <=0;
               target       <=0;
            end
         else if (start_condition)
            begin
               freq_search       <=0;
               target            <=0;
            end
         else if (freq_search)
            begin
               if (myscl_posedge_enable)
                  begin
                     freq_search <= 0;
                     target      <= target + 'h3;
                  end
               else
                  target <= target + 1;
            end
         else if ((i2c_phase == PHASEPARK) && myscl_negedge_enable)
            begin
               freq_search <= 1; 
            end
      end

   always_ff @(posedge vln_core_clk)
      if (~vln_soc_reset_n)
         my_device_negedge <= 0;
      else if (start_condition || stop_condition || abort_transaction)
         my_device_negedge <= 0;
      else if (myscl_negedge_enable)
         my_device_negedge <= my_device;

   always_ff @(posedge vln_core_clk)
      if (~vln_soc_reset_n)
         last_byte_negedge <= 0;
      else if (start_condition || stop_condition)
         last_byte_negedge <= 0;
      else if (myscl_negedge_enable)
         last_byte_negedge <= last_byte;

   always_ff @(posedge vln_core_clk or negedge vln_soc_reset_n)
      begin
         if (~vln_soc_reset_n)
            begin
               timeout_low_phase <=0;
               track_low_phase   <=0;
               timer             <=0;
               stretch_timeout <= 2'b0;
            end
         //   
         else if(error_condition) 
            begin
              track_low_phase <= 1'b1;
              timer             <= target;
              timeout_low_phase <= 1'b0;
              stretch_timeout <= 2'b0;
            end     
         else if(abort_transaction)
            begin
              track_low_phase <= 1'b1;
              //
              if(timer == 0) begin
                timer             <= target;
                timeout_low_phase <= 1'b1;
              end else begin
                timer             <= timer - 1;
                timeout_low_phase <= 1'b0;
              end  
              stretch_timeout <= 2'b0;
            end
         //
         else if (my_device)
            begin
               if (myscl_posedge_enable)
                  begin
                     timer             <= 0;
                     timeout_low_phase <= 0;
                  end
               else if (myscl_negedge_enable)
                  begin
                     track_low_phase<= 1;
                     timer          <= target;
                  end
               else if ( track_low_phase && (timer==0))
                  begin
                     timeout_low_phase<= 1;
                     track_low_phase  <= 0;
                  end
               else if (track_low_phase)
                  timer <= timer - 1;

              stretch_timeout <= 2'b0;
            end
         //   
         else if (slvscl_stretch_ff)
            begin
              if(slvscl_posedge_enable) track_low_phase <= 1'b1;
              //
              if(track_low_phase) begin
                if(timer == 0) begin
                  timer             <= target;
                  timeout_low_phase <= 1'b1;
                end else begin
                  timer             <= timer - 1;
                  timeout_low_phase <= 1'b0;
                end  
              end else begin
                timer             <= target;
                timeout_low_phase <= 1'b0;
              end  
              stretch_timeout <= 2'b0;
            end
         //
         else if (slvscl_stretch)
            begin
              track_low_phase   <= 0;
              timeout_low_phase <= 0;
              if(timer == 0) begin
                timer <= {1'b0, target[9:1]};
                stretch_timeout <= {stretch_timeout[0], 1'b1};
              end else begin 
                timer <= timer - 1;
                stretch_timeout <= 2'b0;
              end   
            end
         //
         else // if (start_condition || stop_condition)
            begin
               timeout_low_phase<=0;
               track_low_phase  <=0;
               timer            <=0;
               stretch_timeout <= 2'b0;
            end
      end
      
// SMBUS logic to drive the SDA
// Read data and ACK are passed from the slave device
//###########################################################

  generate 
    if(SMBUS_FILTER_COMMAND == 1'b1) begin
      //
      always @(posedge vln_core_clk or negedge vln_soc_reset_n) begin
        if(~vln_soc_reset_n)
          smbus_cmd_valid   <= 1'b1;
        else begin  
          if(busy == 1'b0)
            smbus_cmd_valid <= 1'b1;
          else if(myscl_negedge_enable && smbus_cmd_block_candidate && i2c_phase == WRITE_BYTES )
            smbus_cmd_valid <= 1'b0;
        end  
      end
      //
      always @(posedge vln_core_clk or negedge vln_soc_reset_n) begin
        if(~vln_soc_reset_n)
          smbus_cmd_block_candidate <= 1'b0;
        else begin
          if(busy == 1'b0)
            smbus_cmd_block_candidate <= 1'b0;
          else if(myscl_posedge_dly[1] && i2c_phase == DEV_COMMAND && i2c_bit_tracker == BIT0 && my_device)
            smbus_cmd_block_candidate <= ~smbus_cmd_whitelist[bit_shift_register];
        end
      end 
      //
    end else begin
      assign smbus_cmd_valid = 1'b1;
      assign smbus_cmd_block_candidate = 1'b0;
    end
  endgenerate  

  always @(posedge vln_core_clk or negedge vln_soc_reset_n) begin
    if(~vln_soc_reset_n) begin
      hold_master_on_abort  <= 1'b0;
    	abort_transaction <= 1'b0;
      abort_scl         <= 1'b0;
      abort_sda         <= 1'b0;
      myscl_posedge_dly <= 2'b0;
    end else begin
      myscl_posedge_dly <= {myscl_posedge_dly[0], myscl_posedge_enable};
      //
      if(abort_transaction) begin
        if(stop_condition) begin
          abort_transaction <= 1'b0;
        end  
        if(stop_condition_slv) begin
          hold_master_on_abort <= 1'b0;
        end  
        if(timeout_low_phase) begin
          abort_scl <= 1'b1;
          abort_sda <= slvscl;
        end
      end else begin
    	  abort_transaction <= ~smbus_cmd_valid || not_my_transaction || error_condition; 
    	  hold_master_on_abort <= ~smbus_cmd_valid || not_my_transaction; 
        abort_sda <= 1'b0;
        if(~smbus_cmd_valid || not_my_transaction)
          abort_scl <= i2c_slave_scl;
        else
          abort_scl <= 1'b0; 
      end
      //
    end
  end 

  logic send_passthro_data_ff;

  always @(posedge vln_core_clk or negedge vln_soc_reset_n) begin
    if(~vln_soc_reset_n) begin
      slvscl_stretch_ff     <= 1'b0;
      last_sda              <= 1'b0;
      hold_slvscl           <= 1'b0;
      send_passthro_data_ff <= 1'b1;
    end else begin
      send_passthro_data_ff <= send_passthro_data;
      //
      if(abort_transaction) slvscl_stretch_ff <= 1'b0;
      else begin
        if(~slvscl_stretch_ff) begin
          if(myscl_negedge_enable || (stretch_timeout[1] && i2c_phase == DEV_ADDR_OPTYPE)) slvscl_stretch_ff <= slvscl_stretch;
        end else begin
          if(timeout_low_phase && hold_slvscl) slvscl_stretch_ff <= 1'b0;
        end
      end  
      //
      if(myscl_posedge_enable || slvscl_negedge_enable) last_sda <= mysda;
      //
      if(slvscl_stretch_ff) begin
        if(timeout_low_phase) hold_slvscl <= 1'b1; 
      end else begin
        hold_slvscl <= 1'b0;
      end  
    end
  end

  logic post_ack_clock_control;
  logic [10:0] scl_cycle;
  assign scl_cycle = {target, 1'b0};

  logic [13:0] post_ack_hold_timer;
  always_ff @(posedge vln_core_clk) begin
    if (!vln_soc_reset_n) begin
      post_ack_hold_timer <= 0;
      post_ack_clock_control <= 1'b0;
    end
    else begin
  
  
    if(SCL_FORCE_STRETCH == 0) begin
      post_ack_clock_control <= 1'b0;
      post_ack_hold_timer    <= '0;
    end else begin  
      if(busy) begin
        if(post_ack_clock_control) begin
          post_ack_hold_timer <= post_ack_hold_timer - 1;
          if(post_ack_hold_timer == 0) begin
            post_ack_clock_control <= 1'b0;
          end else begin
            post_ack_clock_control <= 1'b1;
          end
        end else if(slvscl_negedge_enable) begin
          if(i2c_bit_tracker == ACK && smbus_write_op) begin
            post_ack_clock_control <= 1'b1;
            if(SCL_FORCE_STRETCH == 2) 
              post_ack_hold_timer <= {2'b0, scl_cycle, 1'b0};
            else if(SCL_FORCE_STRETCH == 3)
              post_ack_hold_timer <= {2'b0, scl_cycle, 1'b0} + {3'b0, scl_cycle};
            else if(SCL_FORCE_STRETCH == 4)
              post_ack_hold_timer <= {1'b0, scl_cycle, 2'b0};
            else if(SCL_FORCE_STRETCH == 5)
              post_ack_hold_timer <= {1'b0, scl_cycle, 2'b0} + {3'b0, scl_cycle};
            else if(SCL_FORCE_STRETCH == 6)
              post_ack_hold_timer <= {1'b0, scl_cycle, 2'b0} + {2'b0, scl_cycle, 1'b0};
            else if(SCL_FORCE_STRETCH == 8)
              post_ack_hold_timer <= {scl_cycle, 3'b0};
            else // SCL_FORCE_STRETCH == 1
              post_ack_hold_timer <= {3'b0, scl_cycle};
          end   
        end
      end  
    end      
  end
  end

  always_comb begin
    // master
    pull_master_scl = post_ack_clock_control  ? 1'b1 : 
                                                ((my_device_negedge && ~slvscl) || slvscl_stretch_ff) || hold_master_on_abort;
	  pull_master_sda = (~abort_transaction && ((generate_ack && ~slvsda) || (send_read_data && ~slvsda)));
	  // slave
    pull_slave_scl =  post_ack_clock_control  ? 1'b1        : 
                      abort_transaction       ? ~abort_scl  : 
                      slvscl_stretch_ff       ? hold_slvscl : 
                                                (~myscl && ~timeout_low_phase);

    pull_slave_sda = abort_transaction ? ~abort_sda : (slvscl_stretch_ff ? ~last_sda    : (~mysda && send_passthro_data_ff));
    // detect clock stretching
    slvscl_stretch  = ~pull_slave_scl && ~i2c_slave_scl;
  end

  // master
  assign i2c_scl_oe = (pull_master_scl || i2cm_override);
  assign i2c_sda_oe = pull_master_sda;

  //assign i2c_scl       = (pull_master_scl || i2cm_override) ? 1'b0 : 1'bz;
  //assign i2c_sda       = pull_master_sda                    ? 1'b0 : 1'bz;
  // slave
  assign i2c_slave_scl = (pull_slave_scl && ~i2cm_override)  ? 1'b0 : 1'bz;
  assign i2c_slave_sda = (pull_slave_sda && ~i2cm_override)  ? 1'b0 : 1'bz;
         
endmodule

