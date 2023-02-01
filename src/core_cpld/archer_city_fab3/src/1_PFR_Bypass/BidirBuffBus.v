module BidirBuffBus #(
parameter BUS_SIZE  = 6
)(
input iClk,
input iRst_n,
inout [BUS_SIZE-1:0] IOAbus,
inout [BUS_SIZE-1:0] IOBbus
);

genvar i;

generate//Generate OE buffers for all IOA and IOB signals
	for (i = 0; i < BUS_SIZE; i = i + 1) begin :OE
		BidirBuff BidirBuff_inst(
		.iClk(iClk),
		.iRst_n(iRst_n),
		.IOA(IOAbus[i]),
		.IOB(IOBbus[i])
		);
	end
endgenerate

endmodule
