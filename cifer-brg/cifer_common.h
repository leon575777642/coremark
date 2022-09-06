#define CTRL_REG_GOBIT        0b000000
#define CTRL_REG_RESET_PC     0b000100
#define CTRL_REG_PROC2MNGR    0b001000
#define CTRL_REG_MNGR2PROC    0b001100
#define CTRL_REG_ADDR_SEXT_EN 0b010000
#define CTRL_REG_GO_IFC       0b010100
#define CTRL_REG_DONE_IFC     0b011000

#define BRG_TEST_RESETPC 0x40000000U

#define BRG_STATUS_RUNNING 0
#define BRG_STATUS_SUCCESS 1
#define BRG_STATUS_FAILURE 2

#define NUM_BRG_TILES 3

#define brg_ctrlregs_base(x, y) (0xe100a00000ULL | ((x & 0xF) << 28) | ((y & 0xF) << 24))

#define mark( x ) \
    asm (   "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
            "addi zero,zero," #x ";\n"  \
        );

#include <stdint.h>
#include <stdio.h>
#include "util.h"

/** RV64 */

#ifdef RV64_ARCH
__attribute__ ((noinline))
static void send_wakeup_interrupt(uint8_t xpos, uint8_t ypos) {
  volatile uint64_t* int_addr = (uint64_t*)0x9800000800ULL;
  uint64_t tmp = (1UL << 63) | (ypos << 26) | (xpos << 18) | (0b010 << 15) | 0b1;
  (*int_addr) = swap_uint64(tmp);
}

__attribute__ ((noinline))
static void* brg_ctrlregs_addr(uint8_t tile_x, uint8_t tile_y,
                               uint8_t coreid, uint16_t regnum) {
  uint64_t addr = brg_ctrlregs_base(tile_x, tile_y);
  addr |= ((coreid & 0xF) << 16);
  addr |= (regnum & 0xFFFF);
  return (void*)addr;
}

__attribute__ ((noinline))
static void write_brg_ctrlregs(uint8_t tile_x, uint8_t tile_y,
                               uint8_t coreid, uint16_t regnum,
                               unsigned int data) {
  volatile unsigned int* addr = brg_ctrlregs_addr(tile_x, tile_y, coreid, regnum);
  (*addr) = data;
}

__attribute__ ((noinline))
static unsigned int read_brg_ctrlregs(uint8_t tile_x, uint8_t tile_y,
                                      uint8_t coreid, uint16_t regnum) {
  volatile unsigned int* addr = brg_ctrlregs_addr(tile_x, tile_y, coreid, regnum);
  return *addr;
}

inline static void wakeup_brg_tile(uint8_t tile_x, uint8_t tile_y) {
#ifdef DEBUG_PRINT
  printf("Starting BRG tile (%u, %u) ...\n", (uint32_t) tile_x, (uint32_t) tile_y);
#endif

  write_brg_ctrlregs(tile_x, tile_y, 0, CTRL_REG_GO_IFC, 0x0);       // set to NORMAL mode
  write_brg_ctrlregs(tile_x, tile_y, 0, CTRL_REG_ADDR_SEXT_EN, 0x0); // disable sext

  // start all 6 cores
  for (int i = 0; i < 6; i++) {
    write_brg_ctrlregs(tile_x, tile_y, i, CTRL_REG_GOBIT, 0x1); // enable go_bit for core i
  }

  // wake up the brg tile
  send_wakeup_interrupt(tile_x, tile_y);
}

inline static int wait_brg_tile(uint8_t tile_x, uint8_t tile_y) {
#ifdef DEBUG_PRINT
  printf("Waiting BRG tile (%u, %u) ...\n", (uint32_t) tile_x, (uint32_t) tile_y);
#endif

  int brg_status = BRG_STATUS_RUNNING;
  volatile uint64_t N = 2;
  volatile uint64_t x = 0;
  while (brg_status == BRG_STATUS_RUNNING) {
    brg_status = read_brg_ctrlregs(tile_x, tile_y, 0, CTRL_REG_DONE_IFC);
    // exponential back-off
    for (uint64_t i = 0; i < N; i++)
      x++;
    if (N < 256) N *= 2;
  }

  if (brg_status != BRG_STATUS_SUCCESS) {
#ifdef DEBUG_PRINT
    printf("brg_status=%u\n", (uint32_t) brg_status);
#endif
    return 1;
  } else {
#ifdef DEBUG_PRINT
    printf("BRG_STATUS_SUCCESS\n");
#endif
    return 0;
  }
}
#endif

/** RV32 */

#ifdef RV32_ARCH
inline static int brg_tile_id(int x, int y) {
  if (x == 0 && y == 2)
    return 0;
  else if (x == 0 && y == 3)
    return 1;
  else if (x == 1 && y == 3)
    return 2;
  else
    return -1;
}
#endif
