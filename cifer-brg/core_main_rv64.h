/* Typedef: secs_ret
        For machines that have floating point support, get number of seconds as
   a double. Otherwise an unsigned int.
*/

#ifndef CORE_MAIN_RV64_H
#define CORE_MAIN_RV64_H

#ifndef COMPILER_VERSION
#if defined(__clang__)
#define COMPILER_VERSION __VERSION__
#elif defined(__GNUC__)
#define COMPILER_VERSION "GCC"__VERSION__
#else
#define COMPILER_VERSION "riscv64-unknown-elf-gcc"
#endif
#endif
#ifndef COMPILER_FLAGS
#define COMPILER_FLAGS \
    FLAGS_STR /* "Please put compiler flags here (e.g. -o3)" */
#endif
#ifndef MEM_LOCATION
#define MEM_LOCATION "static"
#define MEM_LOCATION_UNSPEC 1
#endif

typedef signed short   ee_s16;
typedef unsigned short ee_u16;
typedef signed int     ee_s32;
typedef double         ee_f32;
typedef unsigned char  ee_u8;
typedef unsigned int   ee_u32;
typedef uintptr_t      ee_ptr_int;
typedef uint64_t       ee_size_t;

typedef struct CORE_PORTABLE_S
{
    ee_u8 portable_id;
} core_portable;

typedef double secs_ret;

typedef size_t CORE_TICKS;

/*matrix benchmark related stuff */
typedef ee_s16 MATDAT;
typedef ee_s32 MATRES;

typedef struct MAT_PARAMS_S
{
    int     N;
    //MATDAT *A;
    //MATDAT *B;
    //MATRES *C;
    uint32_t A;
    uint32_t B;
    uint32_t C;
} mat_params;

/* state machine related stuff */
/* List of all the possible states for the FSM */
typedef enum CORE_STATE
{
    CORE_START = 0,
    CORE_INVALID,
    CORE_S1,
    CORE_S2,
    CORE_INT,
    CORE_FLOAT,
    CORE_EXPONENT,
    CORE_SCIENTIFIC,
    NUM_CORE_STATES
} core_state_e;

/* Helper structure to hold results */
typedef struct RESULTS_S
{
    /* inputs */
    ee_s16              seed1;       /* Initializing seed */
    ee_s16              seed2;       /* Initializing seed */
    ee_s16              seed3;       /* Initializing seed */
    //void *              memblock[4]; /* Pointer to safe memory location */
    uint32_t            memblock[4]; /* Pointer to safe memory location */
    ee_u32              size;        /* Size of the data */
    ee_u32              iterations;  /* Number of iterations to execute */
    ee_u32              execs;       /* Bitmask of operations to execute */
    //struct list_head_s *list;
    uint32_t            list;
    mat_params          mat;
    /* outputs */
    ee_u16 crc;
    ee_u16 crclist;
    ee_u16 crcmatrix;
    ee_u16 crcstate;
    ee_s16 err;
    /* ultithread specific */
    core_portable port;
} core_results;

/* Algorithm IDS */
#define ID_LIST             (1 << 0)
#define ID_MATRIX           (1 << 1)
#define ID_STATE            (1 << 2)
#define ALL_ALGORITHMS_MASK (ID_LIST | ID_MATRIX | ID_STATE)
#define NUM_ALGORITHMS      3

/* Misc useful functions */
ee_u16 crcu8(ee_u8 data, ee_u16 crc);
ee_u16 crc16(ee_s16 newval, ee_u16 crc);
ee_u16 crcu16(ee_u16 newval, ee_u16 crc);
ee_u16 crcu32(ee_u32 newval, ee_u16 crc);
ee_u8  check_data_types(void);

#define EE_TICKS_PER_SEC 1000000000 // 1GHz
static CORE_TICKS start_time_val, stop_time_val;

/* Function : start_time
        This function will be called right before starting the timed portion of
   the benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or zeroing some system parameters - e.g. setting the cpu clocks
   cycles to 0.
*/
void
start_time(void)
{
    start_time_val = read_csr(mcycle);
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the
   benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or other system parameters - e.g. reading the current value of
   cpu cycles counter.
*/
void
stop_time(void)
{
    stop_time_val = read_csr(mcycle);
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.

        Actual value returned may be cpu cycles, milliseconds or any other
   value, as long as it can be converted to seconds by <time_in_secs>. This
   methodology is taken to accommodate any hardware or simulated platform. The
   sample implementation returns millisecs by default, and the resolution is
   controlled by <TIMER_RES_DIVIDER>
*/
CORE_TICKS
get_time(void)
{
    return stop_time_val - start_time_val;
}
/* Function : time_in_secs
        Convert the value returned by get_time to seconds.

        The <secs_ret> type is used to accommodate systems with no support for
   floating point. Default implementation implemented by the EE_TICKS_PER_SEC
   macro above.
*/
secs_ret
time_in_secs(CORE_TICKS ticks)
{
    return ((secs_ret)ticks) / EE_TICKS_PER_SEC;
}

#endif // CORE_MAIN_RV64_H
