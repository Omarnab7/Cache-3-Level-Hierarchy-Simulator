#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configuration.h"
#include <math.h>

unsigned int search_address_by_name(Address addressList[], const char* name);
void change_address_by_name(Address addressList[], const char* name, unsigned int val);
Cache* init_cache(size_t size, int access);
uint32_t getTAG(Cache* cache, uint32_t address) { return address >> (cache->index_bits + cache->offset_bits); }
uint32_t getINDEX(Cache* cache, uint32_t address);
void free_cache(Cache* cache);
int check_access_cache(Cache* cache, uint32_t address, int is_write);
void access_cache(Cache* cache, uint32_t address,bool dirty);
void programProccess(const char* filename, Cache* L1, Cache* L2, Cache* L3);
long int DRAMcost(uint32_t* openRow, uint32_t address, int numOfBanks, int* hits, int* misses);



void main() {
    char filesnames[4][512] = { "C:\\Users\\GilMa\\PycharmProjects\\Memory-Hierarchy-Performance\\coremark_val_filtered.trc" , "C:\\Users\\GilMa\\PycharmProjects\\Memory-Hierarchy-Performance\\dhrystone_val_filtered.trc","C:\\Users\\GilMa\\PycharmProjects\\Memory-Hierarchy-Performance\\fibonacci_val_filtered.trc", "C:\\Users\\GilMa\\PycharmProjects\\Memory-Hierarchy-Performance\\linpack_val_filtered.trc" };
    char names[4][512] = { "coremark_val_filtered.trc", "dhrystone_val_filtered.trc" , "fibonacci_val_filtered.trc", "linpack_val_filtered.trc" };

    Cache* L1;
    Cache* L2;
    Cache* L3;
    for (int i = 0; i < 4; i++)
    {
        printf("\n %s :\n",names[i]);
        L1 = init_cache(L1_CACHE_SIZE, L1_HIT_TIME);
        L2 = init_cache(L2_CACHE_SIZE, L2_HIT_TIME);
        L3 = init_cache(L3_CACHE_SIZE, L3_HIT_TIME);
        programProccess(filesnames[i], L1, L2, L3);
    }
}


// Function to search for the address by name
unsigned int search_address_by_name(Address addressList[], const char* name) {
    for (int i = 0; i < NUM_ADDRESSES; i++) {
        if (strcmp(addressList[i].name, name) == 0) {
            return addressList[i].address;
        }
    }
    // Return a special value (e.g., 0xFFFFFFFF) if the name is not found
    printf("search_address_by_name() adrees not found"); // error incase of not found
    return 0xFFFFFFFF;
}

void change_address_by_name(Address addressList[], const char* name, unsigned int val) {
    for (int i = 0; i < NUM_ADDRESSES; i++) {
        if (strcmp(addressList[i].name, name) == 0) {
            addressList[i].address = val;
            return;
        }
    }
    printf("change_address_by_name adress not found");
    return;
}

//initilize the cash size and lines number
Cache* init_cache(size_t size, int access) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    if (!cache) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    cache->size = size;
    cache->num_lines = size / CACHE_LINE_SIZE;
    cache->access = access;
    cache->offset_bits = 2;
    cache->index_bits = (int)log2(cache->num_lines);
    cache->tag_bits = 32 - cache->index_bits - cache->offset_bits;
    cache->hits = 0;
    cache->miss = 0;

    cache->lines = (CacheLine*)malloc(sizeof(CacheLine) * cache->num_lines);
    if (cache->lines == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    for (size_t i = 0; i < cache->num_lines; i++) {
        cache->lines[i].tag = 0;
        cache->lines[i].valid = false;
        cache->lines[i].dirty = false;
    }

    return cache;
}

void free_cache(Cache* cache) {
    free(cache->lines);
    free(cache);
}

uint32_t getINDEX(Cache* cache, uint32_t address)
{
    uint32_t INDEX = address << cache->tag_bits;
    INDEX = INDEX >> (cache->tag_bits + cache->offset_bits);
    return INDEX;
}

int check_access_cache(Cache* cache, uint32_t address, int is_write) {
    uint32_t TAG = getTAG(cache, address);
    uint32_t INDEX = getINDEX(cache, address);
    CacheLine* line = &cache->lines[INDEX];

    // CACHE HIT/MISS LOGIC
    if (!line->valid) {
        // case the index is empty
        line->valid = true;
        line->tag = TAG;
        cache->miss += 1;
        //printf("miss -> line is empty ");
        return -2;
    }

    if (line->valid && line->tag != TAG && line->dirty) {
        // line is dirty and need to save it in the lower level
        cache->miss += 1;
        //printf("miss -> line is diffrent and dirty ");
        return 0;
    }

    if (line->valid && line->tag != TAG) {
        // case is valid and the tag is wrong miss and go to lower level
        cache->miss += 1;
        //printf("miss - line is diffrent ");
        return -1;
    }

    if (line->valid && line->tag == TAG ) {
        // case the line is valid and the same tag
        cache->hits += 1;
        //printf("hit same line ");
        return 1;
    }
    printf("something is not good");
}

void access_cache(Cache* cache, uint32_t address,bool dirty) {
    uint32_t TAG = getTAG(cache, address);
    uint32_t INDEX = getINDEX(cache, address);
    cache->lines[INDEX].valid = true;
    cache->lines[INDEX].tag = TAG;
    cache->lines[INDEX].dirty = dirty;
    //if(dirty) printf("set dirty");
}

void programProccess(const char* filename, Cache* L1, Cache* L2, Cache* L3) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(1);
    }
    uint32_t address;
    uint32_t openRow;
    int L1_state;
    int L2_state;
    int L3_state;
    int openRowHit = 0;
    int openRowMiss = 0;
    double L1_missrate;
    double L2_missrate;
    double L3_missrate;
    double DRAM_missrate;
    double L1_AMAT;
    double RAS = RAS_TIME;
    double CAS = CAS_TIME;
    long int DRAMtime = 0;
    char line[256];
    char instruction[10];
    char var1[10], var2[10], var3[10];
    unsigned int num1, num2;

    while (fgets(line, sizeof(line), file)) {
        // Check for load or store instructions

        if (strstr(line, "->")) { // if the line has -> in it, then update the values of each register
            sscanf(line, "%s %x -> %x", var1, &num1, &num2);
            //printf("%s %8.x -> ", var1, num1);
            change_address_by_name(addressList, var1, num2); //there the registers values (Address) stored.
            //printf("%x\n", search_address_by_name(addressList, var1));
            continue;
        }


        sscanf(line, "%s %[^,],%d(%[^)])", instruction, var1, &num1, var2);
        //printf("Parsed lw/sw line: instruction=%s, var1=%s, num1=%d, var2=%s\n", instruction, var1, num1, var2);

        int isWrite = 0;
        if (strstr(line, "lw")) {
            isWrite = 0;
        }
        else if (strstr(line, "sw")) {
            isWrite = 1;
        }
        else {
            continue;
        }

        address = search_address_by_name(addressList, var2) + num1;
        //printf("proccess %s %x --> tag = %x , index = %x \n",instruction,address,getTAG(L1,address), getINDEX(L1, address));
        uint32_t dirtyAddress = 0;
        // Simulate cache access
        L1_state = check_access_cache(L1, address, isWrite);
        switch (L1_state) {
        case  -2: //evry thing is empty L1 miss
            DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
            access_cache(L1, address, isWrite);
            access_cache(L2, address, false);
            L2->miss += 1;
            access_cache(L3, address, false);
            L3->miss += 1;
            break;
        case -1: //L1 miss
            L2_state = check_access_cache(L2, address, isWrite);
            switch (L2_state) {
            case -2: // L2 miss and line is empty
                DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                L3->miss += 1;
                access_cache(L1, address, isWrite);
                access_cache(L2, address, false);
                access_cache(L3, address, false);
                break;
            case -1: //miss check in L3
                L3_state = check_access_cache(L3, address, isWrite);
                switch (L3_state) {
                case -2: // L3 miss line is empty
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case -1: // every cache got miss
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 0: //every cache got miss and L3 line is dirty
                    DRAMtime += 2 * DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 1: //line found got hit
                    // NO PAY
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                } break;
            case 0: // L2 got miss and the line is dirty
                L3_state = check_access_cache(L3, address, isWrite);
                dirtyAddress = (L2->lines[getINDEX(L2, address)].tag << (L2->offset_bits + L2->index_bits)) | (getINDEX(L2, address) << L2->offset_bits);
                access_cache(L3, dirtyAddress, true);
                dirtyAddress = 0;
                switch (L3_state) {
                case -2: // L3 line is empty
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case -1: // // every cache got miss
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 0: //L3 line is the same as L2
                    DRAMtime += 2 * DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 1: // L3 hit
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                } break;
            case 1: // L2 hit
                access_cache(L1, address, isWrite);
                access_cache(L2, address, false);
                break;
            }
            break;
        case 0: // L1 line is dirty
            dirtyAddress = (L1->lines[getINDEX(L1, address)].tag << (L1->offset_bits + L1->index_bits)) | (getINDEX(L1, address) << L1->offset_bits);
            access_cache(L2, dirtyAddress, true);
            L2_state = check_access_cache(L2, address, isWrite);
            switch (L2_state) {
            case -2: // L2 miss and line is empty
                DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                L3->miss += 1;
                access_cache(L1, address, isWrite);
                access_cache(L2, address, false);
                access_cache(L3, address, false);
                break;
            case -1: //miss check in L3
                L3_state = check_access_cache(L3, address, isWrite);
                switch (L3_state) {
                case -2: // L3 miss line is empty
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case -1: // every cache got miss
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 0: //every cache got miss and L3 line is dirty
                    DRAMtime += 2 * DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 1: //line found got hit
                    // NO PAY
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                } break;
            case 0: // L2 got miss and the line is dirty
                L3_state = check_access_cache(L3, address, isWrite);
                dirtyAddress = (L2->lines[getINDEX(L2, address)].tag << (L2->offset_bits + L2->index_bits)) | (getINDEX(L2, address) << L2->offset_bits);
                access_cache(L3, dirtyAddress, true);
                dirtyAddress = 0;
                switch (L3_state) {
                case -2: // L3 line is empty
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case -1: // // every cache got miss
                    DRAMtime += DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 0: //L3 line is the same as L2
                    DRAMtime += 2 * DRAMcost(&openRow, address, BANKS, &openRowHit, &openRowMiss);
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                case 1: // L3 hit
                    access_cache(L1, address, isWrite);
                    access_cache(L2, address, false);
                    access_cache(L3, address, false);
                    break;
                } break;
            case 1: // L2 hit
                access_cache(L1, address, isWrite);
                L3->hits += 1;
                break;
            }
            break;
        case 1: // L1 hit
            access_cache(L1, address, isWrite);
            L2->hits += 1;
            L3->hits += 1;
            break;
        }
    }

    fclose(file);

    //printf("Total memory accesses: %d\n", L1->miss + L1->hits);
    //printf("L1 cache hits: %d\n", L1->hits);
    //printf("L1 cache misses: %d\n", L1->miss);
    L1_missrate = (double)(L1->miss) / (L1->miss + L1->hits);
    L2_missrate = (double)(L2->miss) / (L2->miss + L2->hits);
    L3_missrate = (double)(L3->miss) / (L3->miss + L3->hits);
    DRAM_missrate = (double)openRowMiss / (openRowMiss + openRowHit);
    //printf("L1 miss rate : %f\n", L1_missrate);
    L1_AMAT = L1->access + L1_missrate * (L2->access + L2_missrate * (L3->access + L3_missrate * (CAS + DRAM_missrate * (RAS + CAS))));
    printf("L1 Avarage Memory Access Time is : %f\n", L1_AMAT);
    //printf("L2 cache hits: %d\n", L2->hits);
    //printf("L2 cache misses: %d\n", L2->miss);
    //
    ////L1_AMAT = L3->access + L3_missrate * (CAS + DRAM_missrate * (RAS + CAS));
    //printf("L3 cache hits: %d\n", L3->hits);
    //printf("L3 cache misses: %d\n", L3->miss);
    ////printf("L3 miss rate: %f\n", L3_missrate);
    ////printf("L3 Avarage Memory Access Time: %f\n", L1_AMAT);

    //printf("DRAM time spend: %d\n", DRAMtime);

    //printf("Average memory access time: %.2f\n", (float)(total_time + write_back_cost) /(L1_hits + L2_hits + L3_hits + L3_misses + L1_misses + L2_misses));


    free_cache(L1);
    free_cache(L2);
    free_cache(L3);

}

long int DRAMcost(uint32_t *openRow,uint32_t address, int numOfBanks, int  *hits, int * misses) {
    long int RAS = RAS_TIME;
    long int CAS = CAS_TIME;
    // calculate the row
    uint32_t row = *openRow / (numOfBanks); // 4 bytes
    uint32_t addressRow = address / (numOfBanks);
    if (row == addressRow) {
        *openRow = row * numOfBanks;
        *hits += 1;
        return  CAS;
    }
    *openRow = addressRow * numOfBanks;
    *misses += 1;
    return RAS + CAS;
}