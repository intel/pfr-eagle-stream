/*! \mainpage Wilson City RP PLD Documentation


\section intro_sec Introduction


This document describes the PLD implementation for Wilson City RP. Target audiences of this document are board design engineers, platform architects,
RTL developers and hardware verification engineers. The goal is to help team understand the PLD internal blocks and the corresponding logics  with out  digging 
into PLD code. \n 

This document assumes that you have already read the PAS Document  omitting certain explanations that are the PAS and each code this 
document to thereby avoid duplicating information and to move more fluid, if you need more detail about some module please go to files
and select the module in which you have a question,the code is made using  modules.\n

Wilson city RP PAS Document <a href="https://sharepoint.amr.ith.intel.com/sites/WhitleyPlatform/commoncore_arch_wg/SitePages/Home.aspx?RootFolder=%2Fsites%2FWhitleyPlatform%2Fcommoncore_arch_wg%2FShared%20Documents%2FPAS&FolderCTID=0x012000DF5E0A7889222348B1C4A02878510BA0&View={A244E90F-FF7B-40A8-8BF1-235AFE71B76B}" target="_blank"><b>Open file</b></a>

Main PLD features:\n

<ul style="list-style-type:square">
  <li>Common Core Code</li>
  <li>Power Sequence controlling (PSU, BMC, PCH, Main VR's, Memory, CPU Vr's)</li>
  <li>CATERR delay</li>
  <li>mDP (XDP) Support BTN / SYSPWROK</li>
  <li>Thermal trip delay</li>
  <li>ADR Support</li>
  <li>System throttling</li>
  <li>PROCHOT</li>
  <li>MEMHOT</li>
  <li>SGPIO interface to replace shift registers</li>
  <li>Code version register through SGPIO interface and User Code register</li>
  <li>VR failure report to BMC through SGPIO interface</li>
  <li> <b>New:</b> Force power on to support VR debug</li>
  <li> <b>New:</b> Event Logger feature that stores changes in a set of signals and displays it graphically through SMBUS and SAT Tool</li>
  <li> <b>New:</b> SMBUS access to internal PLD registers, used for debug</li>
  <li> <b>New:</b> Fault Display on 7-Segments and PostCodes</li>
  <li> <b>New:</b> eSPI control logic for Mux</li>
  <li> <b>New:</b> Rst Perst logic</li>
</ul>

Debug PLD features:\n

<ul style="list-style-type:square">
  <li>eSPI latch</li>
  <li>SMBUS HOT-PLUG Support</li>  
  <li> <b>New:</b> MitiBug Support</li>
</ul>

\n

\image html High-level-Diagram.png
Figure 1. Illustrate the high-level diagram

\section selection-pld PLD Device Selection 

In order to have enough resources for the common core code , PFR and additional features, it was required select the MAX10 10M16 UBGA324 for Main PLD and  MAX10 10M02 UBGA169 for the 
debug PLD, the reason to not select some options of MAX II or MAX V is because not have RAM on die this will complicate the RP debug. \n

One negative impact of this PLD is not have the ability to enable internal Pull-downs and this maybe cause some problem, to prevent issues was added
some resistors in the board also to have a better precision a external clock of 25MHz is used.\n

The next video have a overview about MAX 10 device, \n

\htmlonly

<iframe width="560" height="315" src="https://www.youtube.com/embed/e7iL2XulpI8" frameborder="0" allowfullscreen></iframe>

\endhtmlonly
\n\n

\section resource1 Resources 

Wilson City RP PLD resource consumption is listed in the next image: \n

Main PLD :\n
\image html resourceTop.png

Debug PLD :\n
\image html resourceDebug.png

The resources by module are the next:

Main PLD :\n
\image html resourcebymoduleTop.png

Debug PLD :\n
\image html resourcebymoduleDebug.png

\section pinlist Pin List

Go to the next link for download the latest pin list <a href="Wilson_City_RP_Pinlist.xlsx" target="_blank"><b>HERE.</b></a>

\section clockset Clock

The PLD selected for Wilson City RP generation need take a external clock this clock is generate by CKMNG (25MHz) because 
the internal clock have 10% of tolerance.The PLD divide the clock to work at 2.08MHz. This Main Clock will be used to synchronize all FSMs.
Additionally, the PLD contains a clock divider tree that generates multiple frequencies to avoid gated clocks on the system, 
and to reduce the typical required logic used for delays. It generates 10uS, 50uS, 500uS, 1mS, 20ms, 250mS and 1S clock enables.
For more details on the implementation you can check the source file ClkDivTree.v on the Doxygen documentation.\n

\section  inputsync Input signal synchronization

As power goods and power control inputs to PLD are not synchronized with internal 2MHz clock, to avoid the set-up/hold time violations due to asynchronous inputs,
 a two level input flip-flops structure is added to almost all the inputs, as shown in Figure 2. Rest flip-flops are used to synchronize the input to FSM with 
 the internal 2MHz clock. Due to the two flip-flops structure, it’s expected to have 1us delay for each input signal, such as VR PWRGD and FM_SLP_S3# signals.
For more details on the implementation you can check the source file InputsSync.v on the Doxygen documentation.


\image html inpu2flipflop.png
Figure 2 Input Two Flip-Flop Structure 


\section Disclaimersmain  Disclaimers

Information in this document is provided in connection with Intel products. No license, express or implied, by estoppels or otherwise, to any intellectual property rights is granted by this document. Except as provided in Intel’s Terms and Conditions of Sale for such products, Intel assumes no liability whatsoever, and Intel disclaims any express or implied warranty, relating to sale and/or use of Intel products including liability or warranties relating to fitness for a particular purpose, merchantability, or infringement of any patent, copyright or other intellectual property right. Intel products are not intended for use in medical, life saving, or life sustaining applications.  Intel may make changes to specifications and product descriptions at any time, without notice.
Copyright © Intel Corporation \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly \n
*Third party brands and names are the property of their respective owners.\n

\section Powersec  Power Sequence
\subsection stanbyvr Standby Sequence (G3->S5)

The standby state is cover by the modules Bmc_Seq.v and  Pch_Seq.v, this modules are in charge of turn ON/OFF the BMC, PCH VR,
 ,RSMRST# and SRST#. The reason to control now the BMC VR's is because before to turn off the VR's the SRST# signal
 need be asserted to prevent damage in the silicon.The next diagram show how the system go to G3->S5. Figure 3.

\image html G3-S5diagram.png

\subsection mainstate Main Sequence (S5->S0)

The Main state (S0) is control by Mstr_Seq.v and this module is in change to request to turn on the VR/PSU but the sequence related with each area are
cover for each module (CPU VR's  are cover by Cpu_Seq.v so went all VR's are turn ON, The Cpu_Seq.v will sent CpuPwrgd=1 to Mstr_Seq.v and the FSM can go to the next state), the modules for this stages are the modules:

<ul style="list-style-type:square">
  <li>Mstr_Seq.v - Master Sequence </li>
  <li>Cpu_Seq.v - CPU VR's</li>
  <li>Mem_Seq.v - Mem VR's</li>
  <li>PSU_Seq.v - PSU</li>
  <li>MainVR_Seq.v  - Main VR's</li>
  <li>PwrgdLogic.v- PWRGD_PCH_PWROK, PWRGD_SYS_PWROK, PWRGD_DRAMPWRGD_CPU and PWRGD_CPU_LVC3 </li>
  <li>SysCheck.v - System Check Mix CPU configuration </li>
  <li>ClockLogic.v - Clock Logic to turn on the Main Clock generator </li>
</ul>

This table contains each state of the FSM S5 -> S0 ->S5.

State Hex  | State Name | Next State | Condition | Details
-----------|------------|------------|-----------|-----------
A          | ST_STBY    | ST_OFF     | RST_RSMRST_PCH_N & RST_SRST_BMC_N & wPwrgd_All_Aux =1 | Default State during AC-On. The system is waiting for all the STBY rails ready to trigger PCH RSMRST# as well as SRST to BMC.     
9          | ST_OFF (S5/S4)     | ST_PS      | wPwrEn=1  | Maps to S4/S5 system Status. All the STBY rails are ready. Waiting for Wake-up event to wait up to S0.
7          | ST_PS      | ST_MAIN     | iPSUPwrgd=1 | PSU DC Enable signal has been asserted by PLD and waiting for PSU PS_PWROK.
6          | ST_MAIN    | ST_MEM     | iMainVRPwrgd=1 | Main VR has been enable by PLD and wait iMainVRPwrgd=1 from MainVR_Seq.v 
5          | ST_MEM     | ST_CPU     | iMemPwrgd=1 | Memory VR has been enabled by PLD and PLD is waiting for Memory rails ready.
4          | ST_CPU     | ST_SYSPWROK| iCpuPwrgd =1 | PLD has triggered the CPU Main rails enable sequence and waiting for the indication of all the CPU Main rail ready. 
3          | ST_SYSPWROK| ST_CPUPWRGD| PWRGD_SYS_PWROK=1 | An intermediate state to delay 100ms before asserting the SYS_PWROK and wait for Nevo Ready.
2          | ST_CPUPWRGD| ST_RESET   | PWRGD_CPUPWRGD =1 | Waiting for CPU_PWRGD from PCH. 
1          | ST_RESET   | ST_DONE    | RST_PLTRST_N =1 | System is in Reset Status, Waiting for PLTRST de-assertion.
0          | ST_DONE (S0)   | ST_SHUTDOWN| wPwrEn=0 | All the bring-up power sequence has been completed, and the BIOS shall start to execute since st_done.
B          | ST_SHUTDOWN| ST_MAIN_OFF  | iCpuPwrgd=0 | System is in the shutdown process. Waiting for the indication of all CPU Power rails has been turned off.
C          | ST_MAIN_OFF | ST_PS_OFF  | iMainVRPwrgd=0 | Waiting for the indication of all Main VR OFF.
D          | ST_PS_OFF  | ST_S3      | FM_SLPS3_N=0| PSU is instructed to turn off and PLD is waiting for the assertion of SLPS3
8          | ST_S3      | ST_OFF     | !FM_SLPS4_N && !iMemPwrgd = 1 | System in S3 Status with STBY and Memory rails On.
F          | ST_FAULT   | ST_STBY    | oGoOutFltSt=1 | System in fault State due to various power failures and stay at fault status until the next power on command.
                

\image html Master-Seq.png 

For more details, please click in each module to get more info about it. 

\section ADR ADR Support 

ADR NVDIMM features is to allow the system to backup important information to its
non-volatile memory section when a power supply failure is detected with minimal baseboard level HW or FW support.\n\n 

The assertion of FM_ADR_TRIGGER from Main PLD would trigger the ADR (Asynchronous DIMM Self-Refresh) 
function of LBG which would instruct CPU to save required RAID information to system memory and put 
memory into Self-Refresh mode. \n\n

After ADR timer expired (maximum 100us), LBG would assert ADR_COMPLETE signal which is connected 
to SAVE# pin of each DDR4 slot after inversion. There is a reserved implemented in PLD to have additional 
delay on ADR_COMPLETE in case 100us was determined as not sufficient for CPU to complete all the ADR operation.\n

At the falling edge of SAVE# pin, NVDIMM would back up the content on DDR Chips onto the NVM (Typically NAND Flash) on NVDIMM. 
During this back-up operation, system power is not required to be valid, as the power will be sourced from its own battery.\n\n

<b>Signal function:</b>\n
The signal FM_ADR_TRIGGER_N /FM_ADR_SMI_GPIO_N is a copy of inverted of PSU Fault  also will be de-asserted if PWRGD_CPUPWRGD go to LOW\n

FM_PS_PWROK_DLY_SEL control the power down delay added for PWRGD_PS_PWROK:
    0 -> 15mS delay for eADR mode
    1 -> 600uS delay for Legacy ADR mode\n

PWRGD_PS_PWROK_DLY_ADR is a copy of PWRGD_PS_PWROK but will the ADR delay added. This will be used to delay the deassertion of PCH_PWROK\n
FM_AUX_SW_EN_DLY_ADR is a copy of FM_AUX_SW_EN but will the ADR delay added.\n

FM_DIS_PS_PWROK_DLY and FM_PLD_PCH_DATA will Disable delay from PWRGD_PS (0 DLY power-down condition) 

\image html ADR_Diagram.png


\section interr INTERRUPT ERROR

PROCHOT# is a bidirectional pin used to signal that a processor internal temperature has exceeded a predetermined limit
 and is self-throttling, or as a method of throttling the processor by an external source.  
PROCHOT# is asserted by the processor when the TCC is active. 
When any DTS temperature reaches the TCC activation temperature, the PROCHOT# signal will be asserted. For more details about the implementation go to the module Prochot.v\n\n

\image html Prochot.png

The SmaRT.v module is dedicated to controlling  SYS_THROTTLE  for more details about the implementation go to the module (SmaRT.v).\n\n

\image html thermlogic.PNG

The PLD toggles the MEMHOT_IN# to force the memory controller to go into throttling mode. 
This happens when the system reaches a certain power / thermal threshold. 
In the Whitley platform implementation, VRHOT# is an output from the memory subsystem VR13.0 controller,
 which is capable of throttling the ICX memory controller via MEMHOT_IN#.For more details about the implementation go to the module Memhot.v\n\n
 \image html Memhot.PNG             

\section pertlogic Platform and PCIe Reset

The Platform and PCIe Reset Function block are responsible for controlling the platform and PCIe reset signals.
 Many signals are generated by the CPLD; some only depend on of CPUPWRGD or the PLTRST produced by the PCH.
  The signals that are produced by the CPLD are defined on the Power Good / Reset block diagram. Below can see the table that contains the expected PLD output.\n

\image html Perstable.png

Only the PCIE CPU signals are capable of choosing between CPUPWRGD or PLTRST as their reset.
The selection can be made using the PCH GPIO or using Jumper (only available for RP). 
The RST_PLTRST_N is bypassing  to NICs, TPM, DediProg, LPC/eSPI, BMC. 

\image html Perstlogic.png

The implementation of these resets is found on the continuous assignments on Rst_Perst.v and Wilson_City_Main.v 

\section espilogic  eSPI Circuitry

The eSPI interface is new to this generation of servers. the PCH is provided with backward compatibility 
to LPC in case, it is needed by using a header to select between LPC/eSPI modes.

The CPLD generates one signal for this purpose:
<ul style="list-style-type:square">
  <li>FM_PCH_ESPI_MUX_SEL: This signal is connected to the ESPI_EN strap mux circuit of the PCH to select between strap functionality and normal functionality.</li>
 </ul>

For more info about the implementation go to eSPI_Ctl.v.

\section gsx  SGPIO Interface 

The PLD implements a logic that connects to the BMC’s SGPIO interface. Implemented via input/output serial shifters. 
In last generation server boards, the shifters are included on the board. However, the decision was made to implement them in the PLD for this generation because it reduces the board costs. The figure  contains the interface signals to connect the SGPIO section on the CPLD and the BMC for more information 
related with the module go to  GSX.v

The SGPIO pin-out registers is in the Common Core Architecture SharePoint  <a href="https://sharepoint.amr.ith.intel.com/sites/WhitleyPlatform/commoncore_arch_wg/SitePages/Home.aspx?RootFolder=%2Fsites%2FWhitleyPlatform%2Fcommoncore_arch_wg%2FShared%20Documents%2FGPIO_Maps%2FBMC_GPIOS&FolderCTID=0x012000DF5E0A7889222348B1C4A02878510BA0&View={A244E90F-FF7B-40A8-8BF1-235AFE71B76B}
" target="_blank"><b>Open file.</b></a>



\image html gsx.png

\section leds LED control

The CPLD also implements a LED Control logic that display LED information from the BMC Output Shifters to the respective LEDs in the board via a muxing circuitry. The reason is for save routing spaces, since this reduces the signals required to route.

The LEDs controlled by the CPLD are the following:

<ul style="list-style-type:square">
  <li>BIOS Binary POST code LEDs</li>
  <li>BIOS POST code 7-segment display</li>
  <li>DIMM's Fault LED</li>
  <li>Fan Fault LED</li>
  </ul>

For more info about the implementation go to led_control.v.

\section fault2 Fault Detection 

To be able to detect a fault condition easily, The code of what VR has a fault condition sent to Postcode acording to VR with the fault.

If the system detects a fault condition, the hyphen change to the corresponding fault. Example, the state of the machine, (ST_FAULT) -.F. and the postcode will show the number of VR.

So the code show in the 7-segment is -F, The fault detection code can show multiple fault code; The code change step by step.
Example the Fault codes detected are 0,8,9, A the code show in the postcode  0, 8, 9, A and the 7-segment -F. the reason to show the hyphen is restarted and show the numbers again. 

The next table fault codes:
Fault number | Name of the VR
-------------|--------------
0 | wBmcPwrFlt
1 | wPchPwrFltP1V8
2 | wPchPwrFltP1V05
3 | wPsuPwrFlt
4 | wMainPwrFlt
5 | wMemPwrFltVDDQ_CPU1_ABCD
6 | wMemPwrFltVDDQ_CPU1_EFHG
7 | wMemPwrFltVDDQ_CPU2_ABCD
8 | wMemPwrFltVDDQ_CPU2_EFHG
9 | wCpuPwrFltVCCSA_CPU1
A | wCpuPwrFltVCCIN_CPU1
B | wCpuPwrFltVCCANA_CPU1
C | wCpuPwrFltP1V8_PCIE_CPU1
D | wCpuPwrFltVCCIO_CPU1
E | wCpuPwrFltVCCSA_CPU2
F | wCpuPwrFltVCCIN_CPU2
10 | wCpuPwrFltVCCANA_CPU2
11 | wCpuPwrFltP1V8_PCIE_CPU2
12 | wCpuPwrFltVCCIO_CPU2


\section debug Debug 
\subsection force Force power on

A special mode has is implemented in the PLD code which allows turning on all the VRs in the system without PCH and CPU installed. 
The VRs are on in the same sequence as the normal sequence, but the PLD  does not wait for any power sequence related signals. 
If a VR does not turn on, the user can check the PLD status codes to determine the VR that is not working properly.
This feature is useful for factory tests and VR debug. The power sequence is still be guaranteed by PLD even in the force power on mode.
To enable this feature is necessary to bring High FM_FORCE_PWRON_LVC3, by populating J87 jumper 2-3.

\subsection smbus1 SMBUS Monitoring

This feature allows SMBus access to internal signals in the PLD for monitoring.
 It required an SMBUS slave module, and a set of reading only registers (Pwrgd signals, etc.) connected to the desired signals monitored. 
 From the hardware point of view, it only requires the connection of the PLD to SMBUS interface. 
 By adding this module, enables the monitor any power failures, and verify in general the sanity of the subsystems.

The proposed register set that supports read only (for monitoring), is the following:\n
<ul style="list-style-type:square">
  <li>Power state</li>
  <li>Power Oks & Power Enables</li>
  <li>CPU & Platform Controller Hub(PCH) Signals </li>
  <li>Basic Input/Output System (BIOS) Post Code</li>
  <li>System Inventory</li>
  <li>System Errors </li>
  </ul>

  The next table show the internal registers:\n

Device address: 17h (7 bits)

\image html table0-5.png
\image html table6-10.png


\subsection eventlo Event Logger

This module allows the system to record the signals state and a timestamp every time one of the monitored signals change its value from the previous sample.
This feature requires the instantiation of a set of Block Random Access Memory (RAM) (already included in selected PLD), an event recording engine,
 a change detector and it reuses the SMBUS module as shown in Figure. 
\image html eventlog.png


Since the implemented sample frequency was 100 kHz, and the used timestamp is 32 bits, it allows around 12 hours of monitoring (if the 512 event’s limit is not reached first).
After running the test, the content of the logs can be extracted from the PLD through SMBUS, using the System Administration Tool (SAT - a compilation of Python Scripts) the PLD 
data can be translated to a user-friendly graphic. An example of the output that can be obtained with the Event Logger can be found on Figure.

\image html eventloggui.png



\section utili Utilities 
\subsection Altera Programer 


The next pdf show how to program the Altera devices. <strong> Remember alway check the box of back-programing to prevent damages in the board.

\image html program.png

This is the steps for the program the device.

\htmlonly


<embed src="program.pdf" width="900" height="500" alt="pdf" pluginspage="http://www.adobe.com/products/acrobat/readstep2.html">
\endhtmlonly
\n\n

\n\n
\section contact2 Contact or Questions
    For any concern please feel free to contact me :D \n\n
    Name: David Bolaños or Jorge Juarez \n
    Lync: Bolanos, David or Juarez Campos, Jorge\n
    Email:david.bolanos@intel.com or jorge.juarez.campos@intel.com\n
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
 */


