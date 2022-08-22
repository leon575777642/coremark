#!/bin/env python3
#=========================================================================
# build_all
#=========================================================================
# Build host (RV64) and device (RV32) code
#
# Author: Tuan Ta
# Date:   01/21/2022

import os
import sys
import subprocess
import ast
import argparse
import csv

file_path  = os.path.realpath(__file__)
dir_path   = os.path.abspath(os.path.join(file_path, '..'))
build_path = os.path.join(dir_path, 'build')

# coremark-related paths
coremark_path   = os.path.abspath(os.path.join(dir_path, '..'))
main_src_file   = os.path.join(coremark_path, 'core_main.c')
util_src_file   = os.path.join(coremark_path, 'core_util.c')
list_src_file   = os.path.join(coremark_path, 'core_list_join.c')
state_src_file  = os.path.join(coremark_path, 'core_state.c')
matrix_src_file = os.path.join(coremark_path, 'core_matrix.c')
portme_file     = os.path.join(coremark_path, 'cifer-brg', 'core_portme.c')

# cifer-related paths
dv_root = '/work/global/qtt2/cifer/cifer-chip-brg/cifer/piton'

# binary files
rv32_exe_file = os.path.join(build_path, 'coremark_rv32.exe')
rv64_exe_file = os.path.join(build_path, 'coremark_rv64.exe')

#-------------------------------------------------------------------------
# execute bash commands
#-------------------------------------------------------------------------

def exec_bash(command):
  print('$ {}'.format(command))
  print('...')
  process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
  output, error = process.communicate()
  return process.returncode, output, error

#-------------------------------------------------------------------------
# Targeting RV64
#-------------------------------------------------------------------------

def compile_rv64(use_brg, iterations):

  rv64_syscall_file = f"{dv_root}/verif/diag/assembly/include/riscv/ariane/syscalls.c"
  rv64_include_dir  = f"{dv_root}/verif/diag/assembly/include/riscv/ariane"
  rv64_crt_file     = f"{dv_root}/verif/diag/assembly/include/riscv/ariane/crt.S"
  rv64_linker_file  = f"{dv_root}/verif/diag/assembly/include/riscv/ariane/link.ld"

  rv64_march    = "rv64imafdc"
  rv64_mabi     = "lp64d"
  rv64_numtiles = 4
  rv64_x_tiles  = 2
  rv64_y_tiles  = 2

  # build syscalls.c
  bashCommand = f"""riscv64-unknown-elf-gcc -c -O2 \
-march={rv64_march} -mabi={rv64_mabi} \
-static -std=gnu99 -mcmodel=medany \
-nostdlib -nostartfiles -ffreestanding \
-I{rv64_include_dir} \
{rv64_syscall_file} -o {build_path}/rv64_syscalls.o"""
  ret, _, _ = exec_bash(bashCommand)
  assert ret == 0

  # build src files
  bashCommand = f"""riscv64-unknown-elf-gcc -O2 \
-march={rv64_march} -mabi={rv64_mabi} \
-static -std=gnu99 -mcmodel=medany \
-fno-common -nostdlib -nostartfiles -ffreestanding -fno-builtin-printf \
-ffp-contract=off \
-I{rv64_include_dir} -I{dir_path} -I{coremark_path} \
{rv64_crt_file} {build_path}/rv64_syscalls.o \
{main_src_file} {util_src_file} {portme_file} \
{list_src_file} {state_src_file} {matrix_src_file} \
-o {rv64_exe_file} \
-lm -lgcc -T {rv64_linker_file} \
-DRV64_ARCH=1 \
-DPITON_RV64_TILES={rv64_numtiles} \
-DPITON_RV64_X_TILES={rv64_x_tiles} \
-DPITON_RV64_Y_TILES={rv64_y_tiles} \
-DPREALLOCATE=1 \
-DITERATIONS={iterations} \
-DFLAGS_STR=\\"-O2\\" \
"""
  ret, _, _ = exec_bash(bashCommand)
  assert ret == 0

#-------------------------------------------------------------------------
# Targeting RV32
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# Combine RV64 binary and RV32 binary into memory image
#-------------------------------------------------------------------------

def create_mem_image(use_brg):
  assert os.path.isfile(rv64_exe_file), \
              f"RV64 binary ({rv64_exe_file}) doesn't exist"
  if use_brg:
    assert os.path.isfile(rv32_exe_file), \
                f"RV32 binary ({rv32_exe_file}) doesn't exist"

  if use_brg:
    outfile = f"coremark_brg"
  else:
    outfile = f"coremark_ariane"

  bashCommands = []

  # create image file
  bashCommands.append(f"cat /dev/null > {outfile}.image")

  #if use_brg:
  #  # convert rv32_exe_file to mem image
  #  bashCommands.append(f"riscv32-unknown-elf-objcopy -I elf32-littleriscv -O binary {rv32_exe_file} diag_32.o")
  #  bashCommands.append("du diag_32.o -b | awk '{print(128 - ($1 % 128));}' | xargs -t -ISIZE truncate diag_32.o -s +SIZE")
  #  bashCommands.append("printf \"\\n@0000000040000000\\t// RV32 test code\\n\" >> " + f"{outfile}.image")
  #  bashCommands.append("xxd -g 8 -c 32 diag_32.o | awk '{printf(\"%s %s %s %s\\n\", $2, $3, $4, $5);}' >> " + f"{outfile}.image")

  # convert rv64_exe_file to mem image
  bashCommands.append(f"riscv64-unknown-elf-objcopy -I elf64-littleriscv -O binary {rv64_exe_file} diag_64.o")
  bashCommands.append("du diag_64.o -b | awk '{print(128 - ($1 % 128));}' | xargs -t -ISIZE truncate diag_64.o -s +SIZE")
  bashCommands.append("printf \"\\n@0000000080000000\\t// RV64 host code\\n\" >> " + f"{outfile}.image")
  bashCommands.append("xxd -g 8 -c 32 diag_64.o | awk '{printf(\"%s %s %s %s\\n\", $2, $3, $4, $5);}' >> " + f"{outfile}.image")

  # create symbol.tbl
  bashCommands.append(f"riscv64-unknown-elf-objdump -d {rv64_exe_file}" + " | grep \"<pass>:\" | awk '{printf(\"good_trap %s X %s\\n\", $1, $1);}' > " + f"{outfile}.tbl")
  bashCommands.append(f"riscv64-unknown-elf-objdump -d {rv64_exe_file}" + " | grep \"<fail>:\" | awk '{printf(\"bad_trap %s X %s\\n\", $1, $1);}' >> " + f"{outfile}.tbl")

  ## create objdump
  #if use_brg:
  #  bashCommands.append(f"riscv32-unknown-elf-objdump -d {rv32_exe_file} --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data >  diag.dump")
  #  bashCommands.append(f"riscv32-unknown-elf-objdump -D {rv32_exe_file} > rv32_{outfile}.S")

  bashCommands.append(f"riscv64-unknown-elf-objdump -d {rv64_exe_file} --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data >> diag.dump")
  bashCommands.append(f"riscv64-unknown-elf-objdump -D {rv64_exe_file} > rv64_{outfile}.S")
  bashCommands.append(f"touch {outfile}.ev")

  for command in bashCommands:
    ret, _, _ = exec_bash(command)
    assert ret == 0

#-------------------------------------------------------------------------
# main
#-------------------------------------------------------------------------

if __name__ == "__main__":
  # input args
  parser = argparse.ArgumentParser(description='build coremark')
  parser.add_argument('--target-rv32', action='store_true', default=False)
  opts = parser.parse_args()

  # make a build directory
  cmd = "mkdir -p {}".format(build_path)
  exec_bash(cmd)

  # change working directory
  os.chdir(build_path)

  # compile
  if opts.target_rv32:
    # TODO: compile the host code to RV64 and the kernel to RV32
    # then combine both binaries into a single image
    print('TODO')
    exit(1)
  else:
    # compile both host and kernel code to RV64
    compile_rv64(use_brg=False, iterations=1)
    create_mem_image(use_brg=False)
