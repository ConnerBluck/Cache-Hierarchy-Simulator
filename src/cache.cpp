/**
 * @file cache.c
 * @brief CS 4290 Summer 2021 Project 1 cache simulator
 *
 * Fill in the functions to complete the cache simulator
 *
 * @author Conner Bluck
 */

#include <cstdarg>
#include <cinttypes>
#include <cstdio>
#include <cstdbool>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

#include "cache.hpp"


using std::vector;

// Use this for printing errors while debugging your code
// Most compilers support the __LINE__ argument with a %d argument type
// Example: print_error_exit("Error: Could not allocate memory %d", __LINE__);
static inline void print_error_exit(const char *msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    fprintf(stderr, msg, argptr);
    va_end(argptr);
    exit(EXIT_FAILURE);
}

// Define data structures and globals you might need for simulating the cache hierarchy below

//info for cache
struct cache_info {
    uint64_t total_bytes;
    uint64_t bytes_per_block;
    uint64_t blocks_per_set;
    uint64_t num_sets;
    uint64_t tag_bits;
    uint64_t index_bits;
    uint64_t offset_bits;
};

//block of L1
struct cache_block_t {
    uint64_t tag;
    uint64_t VCtag;
    uint64_t address;
    unsigned int num;
    bool dirty;
    bool valid;
    unsigned int freq;
    bool mru;
    char test;
};
//block of VC
struct victim_block {
    uint64_t tag;
    uint64_t VCtag;
    uint64_t address;
    uint64_t fifo;
    uint64_t idx;
    char test;
    bool dirty;
    bool valid;
};
//sets for L1
struct cache_set_t {
    vector <cache_block_t> blocks;
};

struct cache_t {
    vector <cache_set_t> sets;
};

struct victim_cache {
    vector <victim_block> Vblocks;
};

cache_info info;
static const int ADDRSIZE = 64;
vector <cache_t> cache;
vector <victim_cache> Vcache;
unsigned int count = 0;

/**
 * Function to initialize any data structures you might need for simulating the cache hierarchy. Use
 * the sim_conf structure for initializing dynamically allocated memory.
 *
 * @param sim_conf Pointer to simulation configuration structure
 *
 */
void sim_init(struct sim_config_t const *sim_conf)
{
    info.total_bytes = (uint64_t) ((uint64_t)1 << sim_conf->l1data.c);
    info.bytes_per_block = (uint64_t) ((uint64_t)1 << sim_conf->l1data.b);
    info.blocks_per_set = (uint64_t) ((uint64_t)1 << sim_conf->l1data.s);
    info.num_sets = info.total_bytes / (info.blocks_per_set * info.bytes_per_block);
    info.index_bits = sim_conf->l1data.c - sim_conf->l1data.s - sim_conf->l1data.b;
    info.offset_bits = sim_conf->l1data.b;
    info.tag_bits = ADDRSIZE - info.offset_bits - info.index_bits;

    cache_t l1_cache;
    victim_cache vic;

    /* Create the cache set_t data structure */
    vector <cache_set_t> sets(info.num_sets);
    for (unsigned int i = 0; i < info.num_sets; i++) {
        vector <cache_block_t> blocks(info.blocks_per_set);
        sets[i].blocks = blocks;
        for (unsigned int j = 0; j < info.blocks_per_set; j++) {
            sets[i].blocks[j].tag = (uint64_t) -1;
            sets[i].blocks[j].VCtag = (uint64_t) -1;
            sets[i].blocks[j].address = (uint64_t) -1;
            sets[i].blocks[j].num = 0;
            sets[i].blocks[j].dirty = false;
            sets[i].blocks[j].valid = false;
            sets[i].blocks[j].freq = 0;
            sets[i].blocks[j].mru = false;
        }
    }
    l1_cache.sets = sets;
    cache.push_back(l1_cache); 

    vector <victim_block> Vblocks(sim_conf->V);
    for (unsigned int j = 0; j < sim_conf->V; j++) {
        Vblocks[j].tag = (uint64_t) -1;
        Vblocks[j].VCtag = (uint64_t) -1;
        Vblocks[j].address = (uint64_t) -1;
        Vblocks[j].fifo = 0;
        Vblocks[j].idx = 0;
        Vblocks[j].dirty = false;
        Vblocks[j].valid = false;
    }
    vic.Vblocks = Vblocks;
    Vcache.push_back(vic);

}

unsigned int get_L1_block(struct sim_config_t const *sim_conf, uint64_t index) {
    unsigned int blockIdx = 0;
    if (sim_conf->rp == 1 || sim_conf->rp == 3) {
        unsigned int min = 10000000;
        for (unsigned int i = 0; i < info.blocks_per_set; i++) {
            if (min > cache[0].sets[index].blocks[i].num) {
                min = cache[0].sets[index].blocks[i].num;
                blockIdx = i; //index of L1 block getting evicted
            }
        }
    } else {
        //find block index via LFU
        unsigned int min = 10000000;
        for (unsigned int i = 0; i < info.blocks_per_set; i++) {
            if (min >= cache[0].sets[index].blocks[i].freq) {
                if (cache[0].sets[index].blocks[i].mru == false) {
                    if (min == cache[0].sets[index].blocks[i].freq) {
                        if (cache[0].sets[index].blocks[i].tag > cache[0].sets[index].blocks[blockIdx].tag) {
                            min = cache[0].sets[index].blocks[i].freq;
                            blockIdx = i;
                        }
                    } else {
                        min = cache[0].sets[index].blocks[i].freq;
                        blockIdx = i;
                    }
                }
            }
        }
        for (unsigned int i = 0; i < info.blocks_per_set; i++) {
            cache[0].sets[index].blocks[i].mru = false;
        }
    }
    return blockIdx;
}

unsigned int get_VC_block(struct sim_config_t const *sim_conf) {
    unsigned int VCIdx = 0;
    uint64_t min = 100000000;
    for(unsigned i = 0; i < sim_conf->V; i++) {
        if (Vcache[0].Vblocks[i].fifo == 0) {
            VCIdx = i;
            return VCIdx;
        }
        if (min > Vcache[0].Vblocks[i].fifo) {
            min = Vcache[0].Vblocks[i].fifo;
            VCIdx = i;
        }
    }
    return VCIdx;
}


/**
 * Function to perform cache accesses, one access at a time. The print_debug function should be called
 * if the debug flag is true
 *
 * @param addr The address being accessed by the processor
 * @param type The type of access - LOAD or STORE
 * @param sim_stats Pointer to simulation statistics structure - Should be populated here
 * @param sim_conf Pointer to the simulation configuration structure - Don't modify it in this function
 */
void cache_access(uint64_t addr, char type, struct sim_stats_t *sim_stats, struct sim_config_t const *sim_conf)
{
    count += 1;
    unsigned int blockIdx = 0;
    unsigned int VCIdx = 0;
    bool hit = false;
    uint64_t tag;
    uint64_t VCtag;
    uint64_t index;
    sim_stats->l1data_num_accesses++;

    if (type == 'S') {
        sim_stats->l1data_num_accesses_stores++;
    } else {
        sim_stats->l1data_num_accesses_loads++;
    }

    tag = addr >> (info.offset_bits + info.index_bits);
    VCtag = addr >> info.offset_bits;
    index = VCtag & (info.num_sets - 1);

    //check L1 for address
    for(unsigned int i = 0; i < info.blocks_per_set; i++) {
        if (cache[0].sets[index].blocks[i].valid == true) {
            if (tag == cache[0].sets[index].blocks[i].tag) {
                hit = true;
                //if LRU, update global count variable so we know it was recently used
                if (sim_conf->rp == 1) {
                    cache[0].sets[index].blocks[i].num = count;
                }
                if (sim_conf->rp == 2) {
                    for(unsigned int i = 0; i <info.blocks_per_set; i++) {
                        cache[0].sets[index].blocks[i].mru = false;
                    }
                    cache[0].sets[index].blocks[i].freq++;
                    cache[0].sets[index].blocks[i].mru = true;
                }
                //if store instruction, set dirty bit
                if (type == 'S') {
                    cache[0].sets[index].blocks[i].dirty = true;
                }
                break;
            } else {
                hit = false;
            }
        } else {
            hit = false;
        }
    }

    //if block was not in L1
    if (hit == false) {
        sim_stats->l1data_num_misses++;
        if (type == 'S') {
            sim_stats->l1data_num_misses_stores++;
        } else {
            sim_stats->l1data_num_misses_loads++;
        }
        //If configuration has VC
        if (sim_conf->has_victim_cache == true) {
            sim_stats->victim_cache_accesses++;

            for (unsigned int i = 0; i < sim_conf->V; i++) {
                if (Vcache[0].Vblocks[i].valid == true) {
                   if (Vcache[0].Vblocks[i].VCtag == VCtag) {
                        //if (Vcache[0].Vblocks[i].idx == index) {
                        hit = true;
                        VCIdx = i;
                        sim_stats->victim_cache_hits++;
                        break;
                       //}
                    } 
                }
            }
            //block is in VC
            if (hit == true) {
                sim_stats->l1data_num_evictions++;
                blockIdx = get_L1_block(sim_conf, index);   //get l1 block index

                //if store instruction, set vc block to dirty
                if (type == 'S') {
                    Vcache[0].Vblocks[VCIdx].dirty = true;
                }

                //swap l1 block and vc block

                //swap dirty bits
                bool temp = cache[0].sets[index].blocks[blockIdx].dirty;
                cache[0].sets[index].blocks[blockIdx].dirty = Vcache[0].Vblocks[VCIdx].dirty;
                Vcache[0].Vblocks[VCIdx].dirty = temp;

                //Vcache[0].Vblocks[VCIdx].address = addr;
                cache[0].sets[index].blocks[blockIdx].num = count;
                Vcache[0].Vblocks[VCIdx].fifo = count;
                Vcache[0].Vblocks[VCIdx].idx = index;
                cache[0].sets[index].blocks[blockIdx].mru = true;
                cache[0].sets[index].blocks[blockIdx].freq = 0;

                uint64_t temp2 = cache[0].sets[index].blocks[blockIdx].tag;
                cache[0].sets[index].blocks[blockIdx].tag = Vcache[0].Vblocks[VCIdx].tag;
                Vcache[0].Vblocks[VCIdx].tag = temp2;

                uint64_t temp3 = cache[0].sets[index].blocks[blockIdx].VCtag;
                cache[0].sets[index].blocks[blockIdx].VCtag = Vcache[0].Vblocks[VCIdx].VCtag;
                Vcache[0].Vblocks[VCIdx].VCtag = temp3;

                uint64_t temp4 = cache[0].sets[index].blocks[blockIdx].address;
                cache[0].sets[index].blocks[blockIdx].address = Vcache[0].Vblocks[VCIdx].address;
                Vcache[0].Vblocks[VCIdx].address = temp4;
                        
            //block not in L1 or in VC
            } else {
                sim_stats->victim_cache_misses++;
                blockIdx = get_L1_block(sim_conf, index);   //get l1 block index 
                VCIdx = get_VC_block(sim_conf);             //get VC block index 

             
                
                //if evicted VC block is dirty, writeback
                if (Vcache[0].Vblocks[VCIdx].valid == true) {
                    if (Vcache[0].Vblocks[VCIdx].dirty == true) {
                        sim_stats->num_writebacks++;
                        sim_stats->bytes_transferred_to_mem += info.bytes_per_block;
                        Vcache[0].Vblocks[VCIdx].dirty = false;
                        //printf("%lld + %lld\n", Vcache[0].Vblocks[VCIdx].address, sim_stats->l1data_num_accesses);
                    }
                }

                //put old l1 block in VC
                if (cache[0].sets[index].blocks[blockIdx].valid == true) {
                    Vcache[0].Vblocks[VCIdx].dirty = cache[0].sets[index].blocks[blockIdx].dirty; //vc dirty bit = l1 dirty bit
                    Vcache[0].Vblocks[VCIdx].valid = true;  //vc data is valid
                    Vcache[0].Vblocks[VCIdx].fifo = count;  //vc fifo = count
                    Vcache[0].Vblocks[VCIdx].idx = index;
                    Vcache[0].Vblocks[VCIdx].address = cache[0].sets[index].blocks[blockIdx].address;
                    Vcache[0].Vblocks[VCIdx].tag = cache[0].sets[index].blocks[blockIdx].tag;   //vc tag = l1 tag
                    Vcache[0].Vblocks[VCIdx].VCtag = cache[0].sets[index].blocks[blockIdx].VCtag;   //vc vctag = l1 vctag
                    sim_stats->l1data_num_evictions++;
                }

                //put block from main memory into l1
                cache[0].sets[index].blocks[blockIdx].freq = 0;
                cache[0].sets[index].blocks[blockIdx].address = addr;
                cache[0].sets[index].blocks[blockIdx].mru = true;
                cache[0].sets[index].blocks[blockIdx].num = count;
                cache[0].sets[index].blocks[blockIdx].tag = tag;
                cache[0].sets[index].blocks[blockIdx].valid = true;
                cache[0].sets[index].blocks[blockIdx].VCtag = VCtag;
                cache[0].sets[index].blocks[blockIdx].dirty = false;
                if (type == 'S') {
                    cache[0].sets[index].blocks[blockIdx].dirty = true;  //block is dirty if store instruction
                }
                sim_stats->bytes_transferred_from_mem += info.bytes_per_block;
            }

            
            
        //If configuration does not have VC
         } else {

            //find block to get evicted
            blockIdx = get_L1_block(sim_conf, index);

            //if block is dirty, writeback
            if (cache[0].sets[index].blocks[blockIdx].valid == true) {
                if (cache[0].sets[index].blocks[blockIdx].dirty == true) {
                    sim_stats->num_writebacks++;
                    sim_stats->bytes_transferred_to_mem += info.bytes_per_block;
                }
                sim_stats->l1data_num_evictions++;
            }
                
            cache[0].sets[index].blocks[blockIdx].tag = tag;
            cache[0].sets[index].blocks[blockIdx].VCtag = VCtag;
            cache[0].sets[index].blocks[blockIdx].valid = true;
            cache[0].sets[index].blocks[blockIdx].num = count;
            cache[0].sets[index].blocks[blockIdx].freq = 0;
            cache[0].sets[index].blocks[blockIdx].mru = true;
            cache[0].sets[index].blocks[blockIdx].dirty = false;
            if (type == 'S') {
                cache[0].sets[index].blocks[blockIdx].dirty = true;
            }
            sim_stats->bytes_transferred_from_mem += info.bytes_per_block;
        }
    }

}


/**
 * Function to cleanup dynamically allocated simulation memory, and perform any calculations
 * that might be required
 *
 * @param stats Pointer to the simulation structure - Final calculations should be performed here
 */
void sim_cleanup(struct sim_stats_t *sim_stats, struct sim_config_t const *sim_conf)
{
    for (unsigned int i = 0; i < info.num_sets; i++) {
        cache[0].sets[i].blocks.clear();

    }
    Vcache[0].Vblocks.clear();
    cache.clear();
    Vcache.clear();

    sim_stats->l1data_miss_rate = (double) sim_stats->l1data_num_misses / (double) sim_stats->l1data_num_accesses;
    if (sim_conf->has_victim_cache == true) {
        sim_stats->victim_cache_miss_rate = (double) sim_stats->victim_cache_misses / (double) sim_stats->victim_cache_accesses;
        sim_stats->avg_access_time = (double) sim_stats->l1data_miss_penalty * (double) sim_stats->l1data_miss_rate * (double) sim_stats->victim_cache_miss_rate + (double) sim_stats->l1data_hit_time;
    } else {
        sim_stats->avg_access_time = (double) sim_stats->l1data_miss_penalty * (double) sim_stats->l1data_miss_rate + (double) sim_stats->l1data_hit_time;
    }
}
