/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/

/* File: core_main.c
        This file contains the framework to acquire a block of memory, seed
   initial parameters, tun t he benchmark and report the results.
*/
#include "coremark.h"

/* Function: iterate
        Run the benchmark for a specified number of iterations.

        Operation:
        For each type of benchmarked algorithm:
                a - Initialize the data block for the algorithm.
                b - Execute the algorithm N times.

        Returns:
        NULL.
*/

#include "cifer_common.h"

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

#define MULTITHREAD 18

// results
core_results results[MULTITHREAD];

#if (SEED_METHOD == SEED_ARG)
ee_s32 get_seed_args(int i, int argc, char *argv[]);
#define get_seed(x)    (ee_s16) get_seed_args(x, argc, argv)
#define get_seed_32(x) get_seed_args(x, argc, argv)
#else /* via function or volatile */
ee_s32 get_seed_32(int i);
#define get_seed(x) (ee_s16) get_seed_32(x)
#endif

// static_memblk
ee_u8 static_memblk[TOTAL_DATA_SIZE * MULTITHREAD];

char *mem_name[3] = { "Static", "Heap", "Stack" };

// other variables
//=========================================================================
// init_data()
//=========================================================================
// Initialize data

void
init_data(int cid, int argc, char *argv[])
{
    ee_u16 i, j = 0;
    ee_u16 num_algorithms = 0;

    ///* first call any initializations needed */
    //portable_init(&(results[0].port), &argc, argv);

    ///* First some checks to make sure benchmark will run ok */
    //if (sizeof(struct list_head_s) > 128)
    //{
    //    ee_printf("list_head structure too big for comparable data!\n");
    //    return MAIN_RETURN_VAL;
    //}

    results[cid].seed1      = get_seed(1);
    results[cid].seed2      = get_seed(2);
    results[cid].seed3      = get_seed(3);
    results[cid].iterations = get_seed_32(4);
#if CORE_DEBUG
    results[cid].iterations = 1;
#endif
    results[cid].execs = get_seed_32(5);

    if (results[cid].execs == 0)
    { /* if not supplied, execute all algorithms */
        results[cid].execs = ALL_ALGORITHMS_MASK;
    }

    /* put in some default values based on one seed only for easy testing */
    if ((results[cid].seed1 == 0) && (results[cid].seed2 == 0)
        && (results[cid].seed3 == 0))
    { /* performance run */
        results[cid].seed1 = 0;
        results[cid].seed2 = 0;
        results[cid].seed3 = 0x66;
    }

    if ((results[cid].seed1 == 1) && (results[cid].seed2 == 0)
        && (results[cid].seed3 == 0))
    { /* validation run */
        results[cid].seed1 = 0x3415;
        results[cid].seed2 = 0x3415;
        results[cid].seed3 = 0x66;
    }

    results[cid].memblock[0] = (void *)static_memblk + cid * TOTAL_DATA_SIZE;
    results[cid].size        = TOTAL_DATA_SIZE;
    results[cid].err         = 0;

    /* Data init */
    /* Find out how space much we have based on number of algorithms */
    for (i = 0; i < NUM_ALGORITHMS; i++)
    {
        if ((1 << (ee_u32)i) & results[cid].execs)
            num_algorithms++;
    }

    results[cid].size = results[cid].size / num_algorithms;

    /* Assign pointers */
    for (i = 0; i < NUM_ALGORITHMS; i++)
    {
        if ((1 << (ee_u32)i) & results[cid].execs)
        {
            results[cid].memblock[i + 1]
                = (char *)(results[cid].memblock[0]) + results[cid].size * j;
            j++;
        }
    }

    /* call inits */
    if (results[cid].execs & ID_LIST)
    {
        results[cid].list = core_list_init(
            results[cid].size, results[cid].memblock[1], results[cid].seed1);
    }
    if (results[cid].execs & ID_MATRIX)
    {
        core_init_matrix(results[cid].size,
                         results[cid].memblock[2],
                         (ee_s32)results[cid].seed1
                             | (((ee_s32)results[cid].seed2) << 16),
                         &(results[cid].mat));
    }
    if (results[cid].execs & ID_STATE)
    {
        core_init_state(
            results[cid].size, results[cid].seed1, results[cid].memblock[3]);
    }
}

//=========================================================================
// iterate()
//=========================================================================
// ROI kernel

void *
iterate(void *pres)
{
    ee_u32        i;
    ee_u16        crc;
    core_results *res        = (core_results *)pres;
    ee_u32        iterations = res->iterations;
    res->crc                 = 0;
    res->crclist             = 0;
    res->crcmatrix           = 0;
    res->crcstate            = 0;

    for (i = 0; i < iterations; i++)
    {
        crc      = core_bench_list(res, 1);
        res->crc = crcu16(crc, res->crc);
        crc      = core_bench_list(res, -1);
        res->crc = crcu16(crc, res->crc);
        if (i == 0)
            res->crclist = res->crc;
    }

    return NULL;
}

//=========================================================================
// main()
//=========================================================================

/* Function: main
        Main entry routine for the benchmark.
        This function is responsible for the following steps:

        1 - Initialize input seeds from a source that cannot be determined at
   compile time. 2 - Initialize memory block for use. 3 - Run and time the
   benchmark. 4 - Report results, testing the validity of the output if the
   seeds are known.

        Arguments:
        1 - first seed  : Any value
        2 - second seed : Must be identical to first for iterations to be
   identical 3 - third seed  : Any value, should be at least an order of
   magnitude less then the input size, but bigger then 32. 4 - Iterations  :
   Special, if set to 0, iterations will be automatically determined such that
   the benchmark will run between 10 to 100 secs

*/

MAIN_RETURN_TYPE
main(int argc, char *argv[])
{
  int cid             = core_id();
  int ncores_per_tile = num_cores();
  int x               = tile_x();
  int y               = tile_y();
  int tile_id         = brg_tile_id(x, y);
  int nthreads        = ncores_per_tile * NUM_BRG_TILES;
  int rv32_thread_id  = tile_id * ncores_per_tile + cid;

  // cache invalidate
  cache_invalidate();

  // compute
  init_data(rv32_thread_id, argc, argv);

  // compute
  iterate(&results[rv32_thread_id]);

  // cache flush
  cache_flush();

  //// Running on a single core
  //switch (tile_id) {
  //  case 0:
  //    // only core 0 does does the work
  //    if (cid == 0) {
  //      // cache invalidate
  //      cache_invalidate();

  //      // compute
  //      init_data(argc, argv);

  //      // cache flush
  //      cache_flush();
  //    }
  //    return BRG_STATUS_SUCCESS;
  //  case 1:
  //    // only core 0 does does the work
  //    if (cid == 0) {
  //      // cache invalidate
  //      cache_invalidate();

  //      // compute
  //      iterate(&results[0]);

  //      // cache flush
  //      cache_flush();
  //    }
  //    return BRG_STATUS_SUCCESS;
  //  case 2:
  //    return BRG_STATUS_SUCCESS;
  //  default:
  //    return BRG_STATUS_FAILURE;
  //}






//    ee_u16       i, j = 0, num_algorithms = 0;
//    ee_s16       known_id = -1, total_errors = 0;
//    ee_u16       seedcrc = 0;
//    CORE_TICKS   total_time;
//
//
//
//    /* perform actual benchmark */
//    start_time();
//
//#if (MULTITHREAD > 1)
//    if (default_num_contexts > MULTITHREAD)
//    {
//        default_num_contexts = MULTITHREAD;
//    }
//    for (i = 0; i < default_num_contexts; i++)
//    {
//        results[i].iterations = results[0].iterations;
//        results[i].execs      = results[0].execs;
//        core_start_parallel(&results[i]);
//    }
//    for (i = 0; i < default_num_contexts; i++)
//    {
//        core_stop_parallel(&results[i]);
//    }
//#else
//    iterate(&results[0]);
//#endif
//
//    stop_time();
//    total_time = get_time();
//
//    /* get a function of the input to report */
//    seedcrc = crc16(results[0].seed1, seedcrc);
//    seedcrc = crc16(results[0].seed2, seedcrc);
//    seedcrc = crc16(results[0].seed3, seedcrc);
//    seedcrc = crc16(results[0].size, seedcrc);
//
//    switch (seedcrc)
//    {                /* test known output for common seeds */
//        case 0x8a02: /* seed1=0, seed2=0, seed3=0x66, size 2000 per algorithm */
//            known_id = 0;
//            ee_printf("6k performance run parameters for coremark.\n");
//            break;
//        case 0x7b05: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 2000 per
//                        algorithm */
//            known_id = 1;
//            ee_printf("6k validation run parameters for coremark.\n");
//            break;
//        case 0x4eaf: /* seed1=0x8, seed2=0x8, seed3=0x8, size 400 per algorithm
//                      */
//            known_id = 2;
//            ee_printf("Profile generation run parameters for coremark.\n");
//            break;
//        case 0xe9f5: /* seed1=0, seed2=0, seed3=0x66, size 666 per algorithm */
//            known_id = 3;
//            ee_printf("2K performance run parameters for coremark.\n");
//            break;
//        case 0x18f2: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 666 per
//                        algorithm */
//            known_id = 4;
//            ee_printf("2K validation run parameters for coremark.\n");
//            break;
//        default:
//            total_errors = -1;
//            break;
//    }
//    if (known_id >= 0)
//    {
//        for (i = 0; i < default_num_contexts; i++)
//        {
//            results[i].err = 0;
//            if ((results[i].execs & ID_LIST)
//                && (results[i].crclist != list_known_crc[known_id]))
//            {
//                ee_printf("[%u]ERROR! list crc 0x%04x - should be 0x%04x\n",
//                          i,
//                          results[i].crclist,
//                          list_known_crc[known_id]);
//                results[i].err++;
//            }
//            if ((results[i].execs & ID_MATRIX)
//                && (results[i].crcmatrix != matrix_known_crc[known_id]))
//            {
//                ee_printf("[%u]ERROR! matrix crc 0x%04x - should be 0x%04x\n",
//                          i,
//                          results[i].crcmatrix,
//                          matrix_known_crc[known_id]);
//                results[i].err++;
//            }
//            if ((results[i].execs & ID_STATE)
//                && (results[i].crcstate != state_known_crc[known_id]))
//            {
//                ee_printf("[%u]ERROR! state crc 0x%04x - should be 0x%04x\n",
//                          i,
//                          results[i].crcstate,
//                          state_known_crc[known_id]);
//                results[i].err++;
//            }
//            total_errors += results[i].err;
//        }
//    }
//    total_errors += check_data_types();
//    /* and report results */
//    ee_printf("CoreMark Size    : %lu\n", (long unsigned)results[0].size);
//    ee_printf("Total ticks      : %lu\n", (long unsigned)total_time);
//#if HAS_FLOAT
//    double secs = time_in_secs (total_time);
//    ee_printf("Total time (secs): fp(0x%016llx)\n", *((uint64_t*)(&secs)));
//    if (time_in_secs(total_time) > 0) {
//        double ips = default_num_contexts * results[0].iterations
//                      / time_in_secs(total_time);
//        ee_printf("Iterations/Sec   : fp(0x%016llx)\n",
//                *((uint64_t*)(&ips)));
//    }
//#else
//    ee_printf("Total time (secs): %d\n", time_in_secs(total_time));
//    if (time_in_secs(total_time) > 0)
//        ee_printf("Iterations/Sec   : %d\n",
//                  default_num_contexts * results[0].iterations
//                      / time_in_secs(total_time));
//#endif
//    if (time_in_secs(total_time) < 10)
//    {
//        ee_printf(
//            "ERROR! Must execute for at least 10 secs for a valid result!\n");
//        total_errors++;
//    }
//
//    ee_printf("Iterations       : %lu\n",
//              (long unsigned)default_num_contexts * results[0].iterations);
//    ee_printf("Compiler version : %s\n", COMPILER_VERSION);
//    ee_printf("Compiler flags   : %s\n", COMPILER_FLAGS);
//#if (MULTITHREAD > 1)
//    ee_printf("Parallel %s : %d\n", PARALLEL_METHOD, default_num_contexts);
//#endif
//    ee_printf("Memory location  : %s\n", MEM_LOCATION);
//    /* output for verification */
//    ee_printf("seedcrc          : 0x%04x\n", seedcrc);
//    if (results[0].execs & ID_LIST)
//        for (i = 0; i < default_num_contexts; i++)
//            ee_printf("[%d]crclist       : 0x%04x\n", i, results[i].crclist);
//    if (results[0].execs & ID_MATRIX)
//        for (i = 0; i < default_num_contexts; i++)
//            ee_printf("[%d]crcmatrix     : 0x%04x\n", i, results[i].crcmatrix);
//    if (results[0].execs & ID_STATE)
//        for (i = 0; i < default_num_contexts; i++)
//            ee_printf("[%d]crcstate      : 0x%04x\n", i, results[i].crcstate);
//    for (i = 0; i < default_num_contexts; i++)
//        ee_printf("[%d]crcfinal      : 0x%04x\n", i, results[i].crc);
//    if (total_errors == 0)
//    {
//        ee_printf(
//            "Correct operation validated. See README.md for run and reporting "
//            "rules.\n");
//#if HAS_FLOAT
//        if (known_id == 3)
//        {
//            double score = default_num_contexts * results[0].iterations
//                / time_in_secs(total_time);
//            ee_printf("CoreMark 1.0 : fp(0x%016llx)", *((uint64_t*)(&score)));
//#if defined(MEM_LOCATION) && !defined(MEM_LOCATION_UNSPEC)
//            ee_printf(" / %s", MEM_LOCATION);
//#else
//            ee_printf(" / %s", mem_name[MEM_METHOD]);
//#endif
//
//#if (MULTITHREAD > 1)
//            ee_printf(" / %d:%s", default_num_contexts, PARALLEL_METHOD);
//#endif
//            ee_printf("\n");
//        }
//#endif
//    }
//    if (total_errors > 0)
//        ee_printf("Errors detected\n");
//    if (total_errors < 0)
//        ee_printf(
//            "Cannot validate operation for these seed values, please compare "
//            "with results on a known platform.\n");
//
//#if (MEM_METHOD == MEM_MALLOC)
//    for (i = 0; i < MULTITHREAD; i++)
//        portable_free(results[i].memblock[0]);
//#endif
//    /* And last call any target specific code for finalizing */
//    portable_fini(&(results[0].port));
//
//    return MAIN_RETURN_VAL;
}