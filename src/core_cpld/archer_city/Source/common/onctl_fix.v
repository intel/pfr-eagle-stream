////////////////////////////////////////////////////////////////////////////////// 
/*! 
 \brief    <b>%ONCTL logic latch for fix ONCTL_N logic issue during power button override </b> 
 \file     onctl_fix.v 
 \details    <b>Image of the Block:</b> 
    \image html  
     <b>Description:</b> \n 
    ONCTL logic latch for fix ONCTL_N logic issue during power button override\n\n     
 \brief  <b>Last modified</b>  
   $Date:   Nov 22, 2016 $ 
   $Author:  paul.xu@intel.com $    
   Project   : BNP  
   Group   : BD 
 \version     
    0.9 \b  paul.xu@intel.com - File creation\n 
 \copyright Intel Proprietary -- Copyright 2016 Intel -- All rights reserved 
*/ 
////////////////////////////////////////////////////////////////////////////////// 

//`include "pkg-test.v"


module onctl_fix
    (
    input iClk_2M,
    input iRst_n,
	
    input FM_BMC_ONCTL_N,
    input FM_SLPS3_N,
    input FM_BMC_PWRBTN_OUT_N,
    
    output reg FM_BMC_ONCTL_N_LATCH
    );

reg latch_on, FM_BMC_ONCTL_N_LATCH_ff;
reg [31:0] counter;

parameter   T_20S_2M     =  32'd40000000;   

always @ (negedge iRst_n or posedge iClk_2M) begin
    if (!iRst_n) begin
        latch_on <= 1'b0;
        FM_BMC_ONCTL_N_LATCH <= 1'b1;
        FM_BMC_ONCTL_N_LATCH_ff <= 1'b1;
        counter <= 32'd0;
    end
    else begin
        FM_BMC_ONCTL_N_LATCH_ff <= FM_BMC_ONCTL_N;
        if ((!FM_BMC_ONCTL_N_LATCH) & (FM_BMC_ONCTL_N) & (!FM_BMC_PWRBTN_OUT_N))    //Latch is on when PWRBTN_N assertion is detected at pos-edge of ONCTL_N
            latch_on <= 1'b1;
        else if ((!FM_SLPS3_N)|(counter == T_20S_2M))       //Latch is released whenever SLP_S3_N is asserted or watchdog timer reached 20s
            latch_on <= 1'b0;
        else
            latch_on <= latch_on;
        
        if (latch_on) begin
            FM_BMC_ONCTL_N_LATCH <= 1'b0;
            counter <= counter + 1;
        end
        else begin
            FM_BMC_ONCTL_N_LATCH <= FM_BMC_ONCTL_N_LATCH_ff;
            counter <= 32'd0;
        end
    end
end
	
endmodule



