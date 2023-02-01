# Need resources vcsmx,vcs-vcsmx-lic

# VCS environment vars
export VCS_ARCH_OVERRIDE=linux
export VCS_LIC_EXPIRE_WARNING=0

DIR_QUARTUS_SIMLIB = $(QUARTUS_ROOTDIR)/eda/sim_lib

all: compile run_gui
simple: compile_all run_simple

RTL_FILES = \
	crypto_mem2_be.v    \
	crypto_mem3.v       \
	crypto_mem4_be.v    \
	csl_add_sub_393.v   \
	ecdsa384_top.v      \
	mult_384x9.v        \
	multr_all_384x384.v \
	ocs_ecp384_ad_jpc.v \
		

TB_FILES = \
	ecdsa384_top_tb.v   \
    
ALTERA_LIB_FILES = \
	$(DIR_QUARTUS_SIMLIB)/altera_mf.v \
	$(DIR_QUARTUS_SIMLIB)/altera_lnsim.sv



TOOL_COMMAND = vcs -l vcslog.txt -full64
TOOL_OPTIONS_RTL = +v2k +vcs+lic+wait
TOOL_OPTIONS_SV  = -sverilog +incdir+.
TOOL_OPTIONS = +vcs+lic+wait -t ps -timescale=1ps/1ps
COMP_LINT_OPTIONS = 

COMP_COMMAND = vlogan -full64
COMP_OPTIONS = $(TOOL_OPTIONS_RTL) $(TOOL_OPTIONS_SV)

COMP_COMMAND += -lca -kdb
TOOL_COMMAND += -lca -kdb
TOOL_OPTIONS_SV += +define+ENABLE_VCS_DEBUG 
TOOL_OPTIONS += -debug_access+all

ifneq ($(ENABLE_MEM_DEBUG),)
	TOOL_OPTIONS_SV += +define+ENABLE_VCS_MEM_DEBUG
endif


TOPLEVEL_NAME = ecdsa384_top_tb

.PHONY : compile
compile: simv

simv: $(RTL_FILES) $(TB_FILES) $(ALTERA_LIB_FILES)
	$(COMP_COMMAND) $(COMP_OPTIONS) $(TB_FILES)
	$(COMP_COMMAND) $(COMP_OPTIONS) $(RTL_FILES)
	$(COMP_COMMAND) $(COMP_OPTIONS) $(ALTERA_LIB_FILES)
	$(TOOL_COMMAND) $(TOOL_OPTIONS) $(TOPLEVEL_NAME) | tee vcslog.txt

.PHONY: run_gui
run_gui:
	./simv -gui 

.PHONY: dvedebug
dvedebug:
	dve -full64 -vpd vcdplus.vpd -session testbench_session.tcl

.PHONY: clean
clean:
	rm -rf AN.DB csrc simv* *.log *.vpd *.key ./.*.old ./.*.tcl vcslog.txt ./64 ./DVEfiles ./work*
