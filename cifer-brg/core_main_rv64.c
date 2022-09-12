//=========================================================================
// core_main_rv64.c
//=========================================================================
// Host code

#include <stdint.h>
#include <stdio.h>
#include "util.h"
#include "cifer_common.h"

#include "core_main_rv64.h"

core_results* results = (core_results*) 0x40008000U;
ee_u16* seedcrc_ptr   = (ee_u16*)       0x40007000U;


static ee_u16 list_known_crc[]   = { (ee_u16)0xd4b0,
                                     (ee_u16)0x3340,
                                     (ee_u16)0x6a79,
                                     (ee_u16)0xe714,
                                     (ee_u16)0xe3c1 };
static ee_u16 matrix_known_crc[] = { (ee_u16)0xbe52,
                                     (ee_u16)0x1199,
                                     (ee_u16)0x5608,
                                     (ee_u16)0x1fd7,
                                     (ee_u16)0x0747 };
static ee_u16 state_known_crc[]  = { (ee_u16)0x5e47,
                                     (ee_u16)0x39bf,
                                     (ee_u16)0xe5a4,
                                     (ee_u16)0x8e3a,
                                     (ee_u16)0x8d84 };

volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
volatile ee_s32 seed4_volatile = 1;
volatile ee_s32 seed5_volatile = 0;

int main(int argc, char** argv) {
  int rv64_thread_id = argv[0][0];

  // only core 0 runs
  if (rv64_thread_id != 0)
    return 0;

  printf("Hello from Ariane core %d\n", rv64_thread_id);

  ///** Init data */
  //printf("Initializing data using BRG tile (0,2) ...\n");

  //// wake up the tile
  //wakeup_brg_tile(0, 2);

  //// wait for BRG tile to finish
  //int ret = 0;
  //ret = wait_brg_tile(0, 2);
  //if (ret != 0) {
  //  printf("[FAILED] tile (0, 2)\n");
  //  return ret;
  //}

  /** ROI */
  printf("Executing kernel using all BRG tiles ...\n");

  // start measuring time
  start_time();

  // wake up the tile
  wakeup_brg_tile(0, 2);
  wakeup_brg_tile(0, 3);
  wakeup_brg_tile(1, 3);

  // wait for BRG tile to finish
  int ret = wait_brg_tile(0, 2);
  if (ret != 0) {
    printf("[FAILED] tile (0, 2)\n");
    return ret;
  }

  ret = wait_brg_tile(0, 3);
  if (ret != 0) {
    printf("[FAILED] tile (0, 3)\n");
    return ret;
  }

  ret = wait_brg_tile(1, 3);
  if (ret != 0) {
    printf("[FAILED] tile (1, 3)\n");
    return ret;
  }

  // stop measuring time
  stop_time();
  CORE_TICKS total_time = get_time();

  printf("Finished execution\n");

  /** Verify output */

  /* get a function of the input to report */
  ee_u16 seedcrc = *seedcrc_ptr;
  ee_s16 known_id = -1, total_errors = 0;
  ee_u16 i, j = 0;
  ee_u32 default_num_contexts = 18;

  for (int i = 0; i < default_num_contexts; ++i) {
    printf("results[%d].seed1=%x\n", i, results[i].seed1);
    printf("results[%d].seed2=%x\n", i, results[i].seed2);
    printf("results[%d].seed3=%x\n", i, results[i].seed3);
    printf("results[%d].memblock=%x\n", i, ((uint32_t*)results[i].memblock)[0]);
    printf("results[%d].size=%u\n", i, results[i].size);
    printf("results[%d].iterations=%u\n", i, results[i].iterations);
  }

  seedcrc = crc16(results[0].seed1, seedcrc);
  seedcrc = crc16(results[0].seed2, seedcrc);
  seedcrc = crc16(results[0].seed3, seedcrc);
  seedcrc = crc16(results[0].size, seedcrc);

  switch (seedcrc)
  {                /* test known output for common seeds */
      case 0x8a02: /* seed1=0, seed2=0, seed3=0x66, size 2000 per algorithm */
          known_id = 0;
          printf("6k performance run parameters for coremark.\n");
          break;
      case 0x7b05: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 2000 per
                      algorithm */
          known_id = 1;
          printf("6k validation run parameters for coremark.\n");
          break;
      case 0x4eaf: /* seed1=0x8, seed2=0x8, seed3=0x8, size 400 per algorithm
                    */
          known_id = 2;
          printf("Profile generation run parameters for coremark.\n");
          break;
      case 0xe9f5: /* seed1=0, seed2=0, seed3=0x66, size 666 per algorithm */
          known_id = 3;
          printf("2K performance run parameters for coremark.\n");
          break;
      case 0x18f2: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 666 per
                      algorithm */
          known_id = 4;
          printf("2K validation run parameters for coremark.\n");
          break;
      default:
          total_errors = -1;
          break;
  }

  if (known_id >= 0)
  {
      for (i = 0; i < default_num_contexts; i++)
      {
          results[i].err = 0;
          if ((results[i].execs & ID_LIST)
              && (results[i].crclist != list_known_crc[known_id]))
          {
              printf("[%u]ERROR! list crc 0x%04x - should be 0x%04x\n",
                        i,
                        results[i].crclist,
                        list_known_crc[known_id]);
              results[i].err++;
          }
          if ((results[i].execs & ID_MATRIX)
              && (results[i].crcmatrix != matrix_known_crc[known_id]))
          {
              printf("[%u]ERROR! matrix crc 0x%04x - should be 0x%04x\n",
                        i,
                        results[i].crcmatrix,
                        matrix_known_crc[known_id]);
              results[i].err++;
          }
          if ((results[i].execs & ID_STATE)
              && (results[i].crcstate != state_known_crc[known_id]))
          {
              printf("[%u]ERROR! state crc 0x%04x - should be 0x%04x\n",
                        i,
                        results[i].crcstate,
                        state_known_crc[known_id]);
              results[i].err++;
          }
          total_errors += results[i].err;
      }
  }

  total_errors += check_data_types();
  /* and report results */
  printf("CoreMark Size    : %lu\n", (long unsigned)results[0].size);
  printf("Total ticks      : %lu\n", (long unsigned)total_time);

  double secs = time_in_secs (total_time);
  printf("Total time (secs): fp(0x%016llx)\n", *((uint64_t*)(&secs)));
  if (time_in_secs(total_time) > 0) {
      double ips = default_num_contexts * results[0].iterations
                    / time_in_secs(total_time);
      printf("Iterations/Sec   : fp(0x%016llx)\n",
              *((uint64_t*)(&ips)));
  }
  if (time_in_secs(total_time) < 10)
  {
      printf(
          "ERROR! Must execute for at least 10 secs for a valid result!\n");
      total_errors++;
  }

  printf("Iterations       : %lu\n",
              (long unsigned)default_num_contexts * results[0].iterations);
  printf("Compiler version : %s\n", COMPILER_VERSION);
  printf("Compiler flags   : %s\n", COMPILER_FLAGS);
  printf("Memory location  : %s\n", MEM_LOCATION);

  /* output for verification */
  printf("seedcrc          : 0x%04x\n", seedcrc);
  if (results[0].execs & ID_LIST)
      for (i = 0; i < default_num_contexts; i++)
          printf("[%d]crclist       : 0x%04x\n", i, results[i].crclist);
  if (results[0].execs & ID_MATRIX)
      for (i = 0; i < default_num_contexts; i++)
          printf("[%d]crcmatrix     : 0x%04x\n", i, results[i].crcmatrix);
  if (results[0].execs & ID_STATE)
      for (i = 0; i < default_num_contexts; i++)
          printf("[%d]crcstate      : 0x%04x\n", i, results[i].crcstate);
  for (i = 0; i < default_num_contexts; i++)
      printf("[%d]crcfinal      : 0x%04x\n", i, results[i].crc);
  if (total_errors == 0)
  {
      printf("Correct operation validated. See README.md for run and reporting "
             "rules.\n");
      if (known_id == 3)
      {
          double score = default_num_contexts * results[0].iterations
              / time_in_secs(total_time);
          printf("CoreMark 1.0 : fp(0x%016llx)", *((uint64_t*)(&score)));
          printf("\n");
      }
  }

    if (total_errors > 0)
        printf("Errors detected\n");
    if (total_errors < 0)
        printf("Cannot validate operation for these seed values, please compare "
               "with results on a known platform.\n");

  return 0;
}
