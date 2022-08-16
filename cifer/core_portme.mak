# Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# Original Author: Shay Gal-on

#File: core_portme.mak

# Flag: OUTFLAG
#	Use this flag to define how to to get an executable (e.g -o)
OUTFLAG = -o
# Flag: CC
#	Use this flag to define compiler to use
CC = riscv64-unknown-elf-gcc
# Flag: CFLAGS
#	Use this flag to define compiler options. Note, you can add compiler options from the command line using XCFLAGS="other flags"
PORT_CFLAGS = -I${DV_ROOT}/verif/diag/assembly/include/riscv/ariane
PORT_CFLAGS += -march=rv64imafdc -mabi=lp64d -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -fno-common -fno-builtin-printf
PORT_CFLAGS += -DPITON_RV64_TILES=1 -DPITON_RV64_X_TILES=1 -DPITON_RV64_Y_TILES=1
LFLAGS_END = -static -nostdlib -nostartfiles -lm -lgcc -T ${DV_ROOT}/verif/diag/assembly/include/riscv/ariane/link.ld
FLAGS_STR = "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS = $(PORT_CFLAGS) -I$(PORT_DIR) -I. -DFLAGS_STR=\"$(FLAGS_STR)\"
# Flag: PORT_SRCS
#	Port specific source files can be added here
PORT_SRCS = cifer/core_portme.c
PORT_SRCS += ${DV_ROOT}/verif/diag/assembly/include/riscv/ariane/syscalls.c
PORT_SRCS += ${DV_ROOT}/verif/diag/assembly/include/riscv/ariane/crt.S
vpath %.c cifer
vpath %.h cifer
vpath %.mak cifer
# Flag: EXTRA_DEPENDS
#	Port specific extra build dependencies.
#	Some ports inherit from us, so ensure this Makefile is always a dependency.
EXTRA_DEPENDS += cifer/core_portme.mak
# Flag: LOAD
#	Define this flag if you need to load to a target, as in a cross compile environment.

# Flag: RUN
#	Define this flag if running does not consist of simple invocation of the binary.
#	In a cross compile environment, you need to define this.

#For flashing and using a tera term macro, you could use
#LOAD = flash ADDR 
#RUN =  ttpmacro coremark.ttl

#For copying to target and executing via SSH connection, you could use
#LOAD = scp $(OUTFILE)  user@target:~
#RUN = ssh user@target -c  

#For native compilation and execution
LOAD = echo Loading done
RUN = 

OEXT = .o
EXE =

# Target: port_prebuild
# Generate any files that are needed before actual build starts.
# E.g. generate profile guidance files. Sample PGO generation for gcc enabled with PGO=1
#  - First, check if PGO was defined on the command line, if so, need to add -fprofile-use to compile line.
#  - Second, if PGO reference has not yet been generated, add a step to the prebuild that will build a profile-generate version and run it.
#  Note - Using REBUILD=1 
#
# Use make PGO=1 to invoke this sample processing.

ifdef PGO
 ifeq (,$(findstring $(PGO),gen))
  PGO_STAGE=build_pgo_gcc
  CFLAGS+=-fprofile-use
 endif
 PORT_CLEAN+=*.gcda *.gcno gmon.out
endif

.PHONY: port_prebuild
port_prebuild: $(PGO_STAGE)

.PHONY: build_pgo_gcc
build_pgo_gcc:
	$(MAKE) PGO=gen XCFLAGS="$(XCFLAGS) -fprofile-generate -DTOTAL_DATA_SIZE=1200" ITERATIONS=10 gen_pgo_data REBUILD=1
	
# Target: port_postbuild
# Generate any files that are needed after actual build end.
# E.g. change format to srec, bin, zip in order to be able to load into flash
.PHONY: port_postbuild
port_postbuild:

# Target: port_postrun
# 	Do platform specific after run stuff. 
#	E.g. reset the board, backup the logfiles etc.
.PHONY: port_postrun
port_postrun:

# Target: port_prerun
# 	Do platform specific after run stuff. 
#	E.g. reset the board, backup the logfiles etc.
.PHONY: port_prerun
port_prerun:

# Target: port_postload
# 	Do platform specific after load stuff. 
#	E.g. reset the reset power to the flash eraser
.PHONY: port_postload
port_postload:

# Target: port_preload
# 	Do platform specific before load stuff. 
#	E.g. reset the reset power to the flash eraser
.PHONY: port_preload
port_preload:

# FLAG: OPATH
# Path to the output folder. Default - current folder.
OPATH = ./
MKDIR = mkdir -p

# FLAG: PERL
# Define perl executable to calculate the geomean if running separate.
PERL=/usr/bin/perl
