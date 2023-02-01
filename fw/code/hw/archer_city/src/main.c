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

/**
 * @file main.c
 *
 * @brief Mainline function
 *
 */

// Includes
#include "pfr_main.h"

/**
 * @brief Main function
 *
 * 	 Prior to main being called, the following has happened:
 *      In crt0.S:
 *	        * Stack pointer and global pointer set
 *	        * BSS section is cleared
 *      alt_load() is called to load RAM variables from flash/ROM. IS THIS NEEDED?
 *      alt_main()
 *	       alt_irq_init() IS THIS NEEDED?
 *      main() <==== HERE
 *
 *	 In crt.S there is while loop to catch any exit from main.
 *
 * @param[in] None
 * @param[out] None
 * @return None
 */
int main()
{
    // Just call the pfr_main. This inlined so there there is no call
    pfr_main();

    return 0;
}
