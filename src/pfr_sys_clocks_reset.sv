
`timescale 1 ps / 1 ps
`default_nettype none

module pfr_sys_clocks_reset (
    input wire refclk,
    input wire pll_reset,
    output wire pll_locked,
    
    output wire clk2M,
    output wire clk50M,
    output wire sys_clk,
    output wire spi_clk,

    output wire clk2M_reset_sync_n,
    output wire clk50M_reset_sync_n,
    output wire sys_clk_reset_sync_n,
    output wire spi_clk_reset_sync_n
);

wire global_reset;
sys_pll_ip u_sys_pll_ip (
   .areset(pll_reset),
   .inclk0(refclk),
   .c0(clk50M),
   .c1(sys_clk),
   .c2(clk2M),
   .c3(spi_clk),
   .locked(pll_locked)
);


assign global_reset = !pll_locked;

// Reset sync to clk50M
synchronizer #(
    .USE_INIT_0(1),
    .STAGES(3),
    .WIDTH(1)
) u_reset_sync_clk50M (
   .clk_in(clk50M),
   .arst_in(global_reset),
   .clk_out(clk50M),
   .arst_out(global_reset),
   
   .dat_in(1'b1),
   .dat_out(clk50M_reset_sync_n)
);


// Reset sync to clk2M
synchronizer #(
    .USE_INIT_0(1),
    .STAGES(3),
    .WIDTH(1)
) u_reset_sync_clk2M (
   .clk_in(clk2M),
   .arst_in(global_reset),
   .clk_out(clk2M),
   .arst_out(global_reset),
   
   .dat_in(1'b1),
   .dat_out(clk2M_reset_sync_n)
);

// Reset sync to sys_clk
synchronizer #(
    .USE_INIT_0(1),
    .STAGES(3),
    .WIDTH(1)
) u_reset_sync_sys_clk (
   .clk_in(sys_clk),
   .arst_in(global_reset),
   .clk_out(sys_clk),
   .arst_out(global_reset),
   
   .dat_in(1'b1),
   .dat_out(sys_clk_reset_sync_n)
);

// Reset sync to spi_clk
synchronizer #(
    .USE_INIT_0(1),
    .STAGES(3),
    .WIDTH(1)
) u_reset_sync_spi_clk (
   .clk_in(spi_clk),
   .arst_in(global_reset),
   .clk_out(spi_clk),
   .arst_out(global_reset),
   
   .dat_in(1'b1),
   .dat_out(spi_clk_reset_sync_n)
);

endmodule
