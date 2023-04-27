#ifndef GNNP_H
#define GNNP_H

#include "prefetcher.h"
#include "champsim.h"
#include "ooo_cpu.h"
#include <vector>
#include <queue>

#define MIN_GRANULARITY_LINE 10000
#define MIDDLE_GRANULARITY_LINE 600000
#define GRANULARITY_COUNT 4
#define PREFETCH_THRESHOLD 3

using namespace std;

class GNNPrefetcher;
enum Granuliraty {min_gran, middle_gran, max_gran};

class RegTableEntry   //history table of regular mode
{
  public:
    uint32_t tag;
    uint32_t last_offset;
    int32_t delta;
    int32_t count;
    int32_t bitmap[16];

    RegTableEntry()
    {
        tag = 0;
        last_offset = 0;
        delta = 0;
        count = 0;
        for(int i = 0; i < 16; i++)    bitmap[i] = 0;
    };
};

class RegularPrefetcher : public Prefetcher
{
public:
   uint64_t last_cl_addr,
            last_cl_addr_delta;
   uint32_t min_count,
            middle_count;
   Granuliraty learn_granuliraty; 

private:
   deque<RegTableEntry*> reg_table;  //similar to vector, can insert by front

   /* stats */
   struct
   {
      struct
      {
         uint64_t lookup;
         uint64_t evict;
         uint64_t insert;
         uint64_t hit;
      } tracker;

      struct
      {
         uint64_t stride_match;
         uint64_t generated;
      } pref;

      struct 
      {
         uint64_t min_granularity_count;
         uint64_t middle_granularity_count;
         uint64_t max_granularity_count;
      } granularity;
      

   } stats;

private:
   void init_knobs();
   void init_stats();
   uint32_t generate_prefetch(uint64_t address, int32_t stride, int* bitmap, vector<uint64_t> &pref_addr);

public:
   RegularPrefetcher(string type);
   ~RegularPrefetcher();
   void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
   void dump_stats();
   void print_config();
};

#endif