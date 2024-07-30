#define CONFIGURATION_H
#include <stdint.h>
#include <stdbool.h>
#ifdef CONFIGURATION_H

// Cache parameters
#define L1_CACHE_SIZE 16384      // 16KB
#define L2_CACHE_SIZE 524288     // 512KB
#define L3_CACHE_SIZE 2097152    // 2MB
#define CACHE_LINE_SIZE 4       // 4 bytes

#define L1_ASSOCIATIVITY 1       // Direct mapped
#define L2_ASSOCIATIVITY 1       // Direct mapped
#define L3_ASSOCIATIVITY 1       // Direct mapped

#define L1_HIT_TIME 1            // 1 cycle
#define L2_HIT_TIME 6            // 6 cycles
#define L3_HIT_TIME 30           // 30 cycles

#define WRITE_ALLOCATE 1         // Write allocate
#define WRITE_BACK 1             // Write back


// DRAM parameters
#define BUS_WIDTH 4              // 4 bytes
#define CHANNELS 1               // 1 channel
#define DIMMS 1                  // 1 DIMM
#define BANKS 4                  // 4 banks
#define RAS_TIME 100             // 100 cycles
#define CAS_TIME 50              // 50 cycles

// initialized addresses
typedef struct {
    char name[10];
    unsigned int address;
} Address;

#define NUM_ADDRESSES 32

Address addressList[NUM_ADDRESSES] = {{"a0", 0},{"a1", 0},{"a2", 0},{"a3", 0},{"a4", 0},
{"a5", 0},{"a6", 0},{"a7", 0},{"gp", 0},{"ra", 0},{"s0", 0},{"s1", 0},{"s2", 0},{"s3", 0},
{"s4", 0},{"s5", 0},{"s6", 0},{"s7", 0},{"s8", 0},{"s9", 0},{"s10",0},{"s11",0},{"sp", 0},
{"t0", 0},{"t1", 0},{"t2", 0},{"t3", 0},{"t4", 0},{"t5", 0},{"t6", 0},{"tp", 0},{"zero",0}
};

// structs define
typedef struct {
    uint32_t tag;
    bool valid;
    bool dirty;
} CacheLine;

typedef struct {
    CacheLine* lines;
    size_t size;
    size_t num_lines;
    // access times and duration
    int access;
    int hits;
    int miss;
    // total access time = miss + hit
    // cache variables : 
    int tag_bits;
    int index_bits;
    int offset_bits;
} Cache;



#endif // CONFIGURATION_H