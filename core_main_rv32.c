#include <stdio.h>

#include "util.h"
#include "cifer-common.h"
#include "coremark.h"

// @Tuan: generated file containing all static variables
#include "core_main_rv32.h"

void*
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

int main(int argc, char ** argv) {
  int cid             = core_id();
  int ncores_per_tile = num_cores();
  int x               = tile_x();
  int y               = tile_y();
  int tile_id         = brg_tile_id(x, y);
  int nthreads        = ncores_per_tile * NUM_BRG_TILES;
  int rv32_thread_id  = tile_id * ncores_per_tile + cid;

  if (tile_id == -1)
    return BRG_STATUS_FAILURE;

  // only core 0 runs
  if (cid == 0)
    iterate(&results[0]);

  return BRG_STATUS_SUCCESS;
}
