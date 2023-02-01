module BidirBuff(
input iClk,
input iRst_n,
inout IOA,
inout IOB
);
localparam LOW = 1'b0;
localparam HIGH = 1'b1;

localparam IDLE = 2'h0;
localparam IOA_DRIVES = 2'h1;
localparam IOB_DRIVES = 2'h2;

localparam IOA_LOW_IOB_HIGH = 2'b01;
localparam IOA_HIGH_IOB_LOW = 2'b10;

reg [1:0] rCurrState;
reg rIOA;
reg rIOB;

//Enclose state machine in for generate to make this module parametrizable

always @(posedge iClk, negedge iRst_n) begin//Only syncronous SET therefore, reset removed from sensitive list
	 if(!iRst_n) begin
		rCurrState <= IDLE;
	 	rIOA <= LOW;
	 	rIOB <= LOW;
	 end
	 else begin
		case (rCurrState)
			IDLE: begin//Idle state, both signals are HIGH Z so any external agent can drive them to 0
				rIOA <= HIGH;
				rIOB <= HIGH;

				case ({IOA,IOB})
					IOA_LOW_IOB_HIGH: rCurrState <= IOA_DRIVES;//IOA lowers fiRst_n
					IOA_HIGH_IOB_LOW: rCurrState <= IOB_DRIVES;//IOB lowers fiRst_n
					default: rCurrState <= IDLE;//If both lower simultaniously or both high
				endcase
			end

			IOA_DRIVES: begin//IOA lowered before IOB, IOA drives IOB
				rIOA <= HIGH;//IOA stays High Z
				rIOB <= IOA;

				if (IOA)begin
					rCurrState <= IDLE;
				end

				else begin
					rCurrState <= IOA_DRIVES;
				end
			end

			IOB_DRIVES: begin//IOB lowered before IOA, IOB drives IOA
				rIOA <= IOB;
				rIOB <= HIGH;//IOB stays High Z

				if (IOB) begin
					rCurrState <= IDLE;
				end

				else begin
					rCurrState <= IOB_DRIVES;
				end
			end

			default: begin
				rIOA <= HIGH;
				rIOB <= HIGH;
				rCurrState <= IDLE;
			end
		endcase
	 end
end

assign IOA = rIOA ? 1'bz : 1'b0;
assign IOB = rIOB ? 1'bz : 1'b0;

endmodule
