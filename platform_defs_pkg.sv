package platform_defs_pkg;

    // Period of the system clock in ps
    // TODO: extract this from some PLL generated file somehow?
    // localparam SYS_CLOCK_PERIOD_PS          = 33333;        // period of the input 'clock', in picoseconds (33333 ps => 30 MHz)
    localparam SYS_CLOCK_PERIOD_PS          = 50000;        // period of the input 'clock', in picoseconds (50000 ps => 20 MHz)

    //////////////////////////////////////////
    // SMBus relay
    //////////////////////////////////////////

    // there are 3 instances of the Relay block in this platform
    localparam RELAY1_BUS_SPEED_KHZ         = 100;          // bus speed of the slowest device on the slave SMBus 1 interface, in kHz (must be 100, 400, or 1000)
    localparam RELAY2_BUS_SPEED_KHZ         = 100;          // bus speed of the slowest device on the slave SMBus 2 interface, in kHz (must be 100, 400, or 1000)
    localparam RELAY3_BUS_SPEED_KHZ         = 100;          // bus speed of the slowest device on the slave SMBus 3 interface, in kHz (must be 100, 400, or 1000)

    // relay master SCL timeout period in milliseconds. Default value is 36ms (35ms + 1ms buffer). 
    // Valid values ranging from 25ms to 351ms. For TNP platform, SCL timeout value is 350ms, PFR recommends to add 1ms buffer = 351ms. 
    localparam SCL_LOW_TIMEOUT_PERIOD_MS    = 35;
    
    //////////////////////////////////////////
    // SMBus mailbox
    //////////////////////////////////////////

    // SMBus mailbox address
    // can support 7 and 10 bit addresses
    localparam PCH_SMBUS_MAILBOX_ADDR           = 7'h70;
    localparam BMC_SMBUS_MAILBOX_ADDR           = 7'h38;
    localparam PCIE_SMBUS_MAILBOX_ADDR          = 7'h70;
    localparam SMBUS_MAILBOX_FIFO_DEPTH     = 96;
    localparam SMBUS_BIOS_FIFO_DEPTH     = 64;
    localparam SMBUS_MAILBOX_MCTP_FIFO_DEPTH= 1024;

    // definitions of special register file addresses
    // the RTL doesn't do anything special for some addresses so they are not enumerated here
    localparam PROVISION_COMMAND_ADDR       = 8'h0B;
    localparam COMMAND_TRIGGER_ADDR         = 8'h0C;
    localparam WRITE_FIFO_ADDR              = 8'h0D;
    localparam READ_FIFO_ADDR               = 8'h0E;
    localparam BMC_CHECKPOINT_ADDR          = 8'h60;
    localparam ACM_CHECKPOINT_ADDR          = 8'h10;
    localparam BIOS_CHECKPOINT_ADDR         = 8'h11;
    localparam PCH_UPDATE_INTENT_ADDR       = 8'h12;
    localparam BMC_UPDATE_INTENT_ADDR       = 8'h13;
    
    localparam MCTP_WRITE_FIFO_ADDR         = 8'h0F;
    
    localparam PCH_UPDATE_INTENT_PART2_ADDR = 8'h61;  
    localparam BMC_UPDATE_INTENT_PART2_ADDR = 8'h62;    
                                                      
    localparam PCH_MCTP_FIFO_AVAIL_ADDR     = 8'h64;  
    localparam BMC_MCTP_FIFO_AVAIL_ADDR     = 8'h65;  
    localparam PCIE_MCTP_FIFO_AVAIL_ADDR    = 8'h66;  
                                                      
    localparam PCH_MCTP_BYTECOUNT_ADDR      = 8'h67;  
    localparam BMC_MCTP_BYTECOUNT_ADDR      = 8'h68;  
    localparam PCIE_MCTP_BYTECOUNT_ADDR     = 8'h69;  
    
    localparam BIOS_CONTROL_STATUS_REG      = 8'h6A;  
    localparam BIOS_READ_FIFO_ADDR          = 8'h6B;  

    localparam PCH_MCTP_READ_FIFO_ADDR      = 8'h71;  
    localparam BMC_MCTP_READ_FIFO_ADDR      = 8'h72;  
    localparam PCIE_MCTP_READ_FIFO_ADDR     = 8'h73; 
    
    // writable addresses for the PCH/CPU connection to the SMBus mailbox
    function logic pch_mailbox_writable_address (input logic [7:0] address);
        pch_mailbox_writable_address =  address == PROVISION_COMMAND_ADDR  ||
                                        address == COMMAND_TRIGGER_ADDR    ||
                                        address == WRITE_FIFO_ADDR         ||
                                        address == MCTP_WRITE_FIFO_ADDR    ||
                                        address == ACM_CHECKPOINT_ADDR     ||
                                        address == BIOS_CHECKPOINT_ADDR    ||
                                        address == PCH_UPDATE_INTENT_ADDR  ||
                                        address == PCH_UPDATE_INTENT_PART2_ADDR  ||
                                        address[7:6] == 2'b10; // (address >= 8'h80 && address <= 8'hBF)
    endfunction

    // writable address for the BMC connection to the SMBus mailbox
    function logic bmc_mailbox_writable_address (input logic [7:0] address);
        bmc_mailbox_writable_address =  address == PROVISION_COMMAND_ADDR  ||
                                        address == COMMAND_TRIGGER_ADDR    ||
                                        address == WRITE_FIFO_ADDR         ||
                                        address == MCTP_WRITE_FIFO_ADDR    ||
                                        address == BMC_CHECKPOINT_ADDR     ||
                                        address == BMC_UPDATE_INTENT_ADDR  ||
                                        address == BMC_UPDATE_INTENT_PART2_ADDR  ||
                                        address[7:6] == 2'b11; // (address >= 8'hC0 && address <= 8'hFF)
    endfunction
    
    // writable address for the PCIe connection to the SMBus mailbox
    function logic pcie_mailbox_writable_address (input logic [7:0] address);
        pcie_mailbox_writable_address = address == MCTP_WRITE_FIFO_ADDR;
    endfunction
    
    //////////////////////////////////////////
    // SPI interfaces
    //////////////////////////////////////////
    
    localparam [31:16] BMC_IBB_ADDRESS_MSBS = 16'h0000;     // any time the BMC accesses a FLASH address where the top 16 bits match this value, it indicates the BMC is accessing the 'Initial Boot Block'
    localparam BMC_FLASH_ADDRESS_BITS       = 27;           // number of BYTE-based address bits supported by the BMC FLASH device (27 bits = 128 MBytes = 1 Gbit)
    localparam PCH_FLASH_ADDRESS_BITS       = 26;           // number of BYTE-based address bits supported by the PCH FLASH device (26 bits = 64 MBytes = 512 Mbit)
    
    localparam BMC_WE_AVMM_ADDRESS_BITS    = BMC_FLASH_ADDRESS_BITS-12-5;          // -12 because we divide the FLASH into 16kB chunks, -5 because each 32 bit word provides info for 32 16kB chunks
    localparam PCH_WE_AVMM_ADDRESS_BITS    = PCH_FLASH_ADDRESS_BITS-12-5;          // -12 because we divide the FLASH into 16kB chunks, -5 because each 32 bit word provides info for 32 16kB chunks
    
    localparam SPI_FILTER_AVMM_CSR_WIDTH   = 1;             // This is set to 1 as default for spi filter CSR avmm width. Here, we solely use the extra AVMM width for NIOS avmm read access to filter logging signal o_filtered_command_info
    //////////////////////////////////////////
    // RFNVRAM Master
    //////////////////////////////////////////

    localparam RFNVRAM_FIFO_SIZE = 512;
    
    //////////////////////////////////////////
    // CRYPTO TOP
    //////////////////////////////////////////
    
    // Valid use-case: enable at least one of the crypto mode
    localparam ENABLE_CRYPTO_256 = 0;
    localparam ENABLE_CRYPTO_384 = 1;
    
    //////////////////////////////////////////
    // CRYPTO DMA
    //////////////////////////////////////////
    localparam CRYPTO_DMA_SPI_ADDRESS_WIDTH = 25;   // word addressing based on largest supported flash device 
    localparam CRYPTO_DMA_UFM_ADDRESS_WIDTH = 21;   // word addressing based on address memory map setting in pfr_sys

endpackage
