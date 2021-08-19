/**
 * @file cache.hpp
 * @brief Header for the CS 4290 Summer 2021 Project 1 framework
 *
 * Header file for the cache simulator containing a bunch of struct definitions,
 * constants, defaults, etc. Do not modify this file!
 *
 * @author Anirudh Jain
 */

#ifndef CACHE_HPP
#define CACHE_HPP


#include <cstdarg>
#include <cinttypes>
#include <cstdio>
#include <cstdbool>
#include <cstdlib>
#include <cstring>

// Constants
enum write_policy {WBWA = 1, WTWNA = 2};
enum replacement_policy {LRU = 1, LFU = 2, FIFO = 3};

static const char *const write_policy_map[] = {"NA", "WBWA", "WTWNA"};
static const char *const replacement_policy_map[] = {"NA", "LRU", "LFU", "FIFO"};

static const char LOAD = 'L';
static const char STORE = 'S';
static const char INST = 'I';
static const uint8_t TRUE = 1;
static const uint8_t FALSE = 0;

// Access time constants
static const uint16_t MAX_S = 3;
static const uint64_t MIN_L1_C = 9;
static const uint64_t MAX_L1_C = 15;
static const uint64_t MAX_VC_SIZE = 16;

// A map of L1 cache access times
static const double L1_ACCESS_TIME[5][7] = {
    {1, 1, 2, 2, 2, 2, 2},  // DM
    {1, 2, 2, 2, 2, 3, 3},  // 2W
    {2, 2, 2, 3, 3, 3, 4},  // 4W
    {2, 3, 3, 3, 3 ,3, 4},  // 8W
    {4, 4, 4, 5, 5, 6, 6}   // FA
};

// Struct for storing per Cache parameters
struct cache_config_t {
    uint64_t c;
    uint64_t b; // We assume that both the caches have the exact same block size
    uint64_t s;
};

// Struct for tracking the simulation parameters
struct sim_config_t {
    struct cache_config_t l1data;

    bool has_victim_cache;          // Hierarchy has victim cache if true
    uint64_t V;                     // number of blocks in the victim cache

    enum replacement_policy rp;     // replacement policy
};

// Struct for keeping track of simulation statistics
struct sim_stats_t {

    // L1 Data Cache statistics
    uint64_t l1data_num_accesses;           // Total L1 Data Accesses
    uint64_t l1data_num_accesses_loads;     // L1 Data Accesses which are Loads
    uint64_t l1data_num_accesses_stores;    // L1 Data Accesses which are Stores
    uint64_t l1data_num_misses;             // Total L1 Data Misses
    uint64_t l1data_num_misses_loads;       // L1 Data Misses which are Loads
    uint64_t l1data_num_misses_stores;      // L1 Data Misses which are Stores
    uint64_t l1data_num_evictions;          // Total blocks evicted from L1-Data cache

    // Victim Cache statistics
    uint64_t victim_cache_accesses;         // Number of accesses to the victim cache
    uint64_t victim_cache_misses;           // Accesses that are misses in the victim cache
    uint64_t victim_cache_hits;             // hits in the victim cache

    // Memory bus statistics
    uint64_t num_writebacks;                // Number of writebacks
    uint64_t bytes_transferred_from_mem;    // number of bytes transferred from main memory
    uint64_t bytes_transferred_to_mem;      // Number of bytes transferred to main memory

    // Performance Statistics
    double l1data_hit_time;                 // L1 Data Hit Time
    double l1data_miss_penalty;             // L1 Data Miss Penalty
    double l1data_miss_rate;                // L1 Data Miss Rate
    double victim_cache_miss_rate;          // Miss rate of the victim cache

    double avg_access_time;                 // Average Access Time per access
};

// Visible functions
void sim_init(struct sim_config_t const *sim_conf);
void cache_access(uint64_t addr, char type, struct sim_stats_t *sim_stats, struct sim_config_t const *sim_conf);
void sim_cleanup(struct sim_stats_t *sim_stats, struct sim_config_t const *sim_conf);

#endif // CACHE_HPP
