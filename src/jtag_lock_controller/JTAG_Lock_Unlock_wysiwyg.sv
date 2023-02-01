//***Design Example for volatile key clear and security mode verification****//
//***in MAX 10 through internal JTAG interface (JTAG access from core)**//
		
module JTAG_Lock_Unlock_wysiwyg (
	jtag_core_en, 
	tck_core,
	tdi_core,
	tdo_ignore,
	tdo_core,
	tms_core,
	tck_ignore,
	tdi_ignore,
	tms_ignore
);

input 	jtag_core_en, tck_core, tdi_core, tms_core, tck_ignore, tdi_ignore, tms_ignore;
output	tdo_ignore, tdo_core;

// Define WYSIWYG atoms
//
// WYSIWYG atoms to access JTAG from core

fiftyfivenm_jtag JTAG
(
      .clkdruser(),
		.corectl(jtag_core_en),
		.runidleuser(),
		.shiftuser(),
		.tck(tck_ignore),
		.tckcore(tck_core),
		.tckutap(),
		.tdi(tdi_ignore),
		.tdicore(tdi_core),
		.tdiutap(),
		.tdo(tdo_ignore),
		.tdocore(tdo_core),
		.tdouser(),
		.tdoutap(),
		.tms(tms_ignore),
		.tmscore(tms_core),
		.tmsutap(),
		.updateuser(),
		.usr1user(),
		.ntdopinena()
);


endmodule
