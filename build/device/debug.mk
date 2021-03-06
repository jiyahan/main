# Copyright (c) 2015, Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# This file describes development debug interface for all programs and
# projects

# FIXME: project+core specific
GDB_QRK_PORT_NUMBER ?= 3333
GDB_ARC_PORT_NUMBER ?= 3334

.PHONY:
gdb-auto-load-safe-path-enable:
	$(shell \
		if [ ! -f $(HOME)/.gdbinit ]; then \
			touch $(HOME)/.gdbinit; \
		fi )
	$(shell \
		if [ `grep -c add-auto-load-safe-path "$(HOME)/.gdbinit"` -eq 0 ]; then \
		$(AT)echo "add-auto-load-safe-path $(PWD)/.gdbinit" >> $(HOME)/.gdbinit ;   \
		fi                                                                     \
		)

debug_start: debug_stop gdb-auto-load-safe-path-enable _tmux-exists flash_files
	TMUX="" $(TMUX_BINARY) new-session -d -n openocd -s openocd "$(OPENOCD) -s $(OUT)/firmware -f $(subst ",,$(OPTION_JTAG_INTERFACE_CFG_FILE)) -f $(OUT)/firmware/init_dbg.cfg"

debug_console_qrk: ELF := $(OUT)/firmware/quark.elf
debug_console_qrk: debug_start
	$(AT)echo "path to ELF: $(ELF)"
	$(AT)echo "target remote localhost:$(GDB_QRK_PORT_NUMBER)" > .gdbinit
	$(AT)echo "set architecture i386" >> .gdbinit
	$(AT)echo "monitor version" >> .gdbinit
	$(AT)echo "monitor halt" >> .gdbinit
	$(AT)echo "symbol-file $(ELF)" >> .gdbinit
	@# Make sure that we re-resume the target after quitting gdb
	$(AT)echo "monitor resume" >> gdbdeinit
	$(AT)echo "quit" >> gdbdeinit
	-bash -c "PYTHONHOME=$(TOOLCHAIN_DIR)/tools/python $(GDB_QRK) $(ELF); PYTHONHOME=$(TOOLCHAIN_DIR)/tools/python $(GDB_QRK) $(ELF) -x gdbdeinit > /dev/null; rm gdbdeinit; $(MAKE) debug_stop"
	@# All make command after this line will not be executed when the user quit gdb using CTRL + C

debug_console_arc: ELF := $(OUT)/firmware/arc.elf
debug_console_arc: debug_start
	$(AT)echo "path to ELF: $(ELF)"
	$(AT)echo "target remote localhost:$(GDB_ARC_PORT_NUMBER)" > .gdbinit
	$(AT)echo "monitor version" >> .gdbinit
	$(AT)echo "monitor halt" >> .gdbinit
	$(AT)echo "symbol-file $(ELF)" >> .gdbinit
	$(AT)echo "source $(T)/projects/curie_common/build/config/debug/gdb_arc_ext.gdb" >> .gdbinit
	@$(AT)echo $(ANSI_CYAN)Breakpoint was set in main, please wait...$(ANSI_OFF)
	$(AT)echo "restart" >> .gdbinit
	@# Make sure that we re-resume the target after quitting gdb
	$(AT)echo "monitor resume" >> gdbdeinit
	$(AT)echo "quit" >> gdbdeinit
	-bash -c "PYTHONHOME=$(TOOLCHAIN_DIR)/tools/python $(GDB_ARC) $(ELF); PYTHONHOME=$(TOOLCHAIN_DIR)/tools/python $(GDB_ARC) $(ELF) -x gdbdeinit > /dev/null; rm gdbdeinit; $(MAKE) debug_stop"
	@# All make command after this line will not be executed when the user quit gdb using CTRL + C

debug_stop:
	rm .gdbinit 2> /dev/null || exit 0
	killall openocd 2> /dev/null || exit 0
	$(TMUX_BINARY) kill-window -t openocd 2> /dev/null || exit 0

debug_help:
	@$(AT)echo "Type \"make debug_console_qrk ELF=path/to/elf_file_to_debug\" or \
	\"make debug_console_arc ELF=path/to/elf_file_to_debug\" to start the specific debug"
	@$(AT)echo " environment, where:"
	@$(AT)echo " ELF=[path to .elf]: load symbol from ELF when you start console."

help::
	@$(AT)echo 'Debug:'
	@$(AT)echo ' debug_help	- detailed help on debug'
	@$(AT)echo ' debug_console	- start console gdb client after calling debug_start target'
	@$(AT)echo ' debug_start	- start debug environment'
	@$(AT)echo ' debug_stop	- stop debug environment'
	@$(AT)echo
