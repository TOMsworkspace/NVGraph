#include "gnnp.h"
#include "champsim.h"
#include<cmath>
#include <algorithm>

namespace knob{
    extern uint32_t RegTableEntry_num;
    extern uint32_t reg_pref_degree;
}

void RegularPrefetcher::init_knobs()
{

}

void RegularPrefetcher::init_stats()
{
   bzero(&stats, sizeof(stats));//set the stats' first sizeof(stats) bytes to 0
}

RegularPrefetcher::RegularPrefetcher(string type) : Prefetcher(type)
{
   last_cl_addr = 0;
   last_cl_addr_delta = 0;
   min_count = 0;
   middle_count = 0;
   learn_granuliraty = min_gran;
}

RegularPrefetcher::~RegularPrefetcher()
{
   
}

void RegularPrefetcher::print_config()
{
   cout << "RegTableEntry_num " << knob::RegTableEntry_num << endl
      << "reg_pref_degree " << knob::reg_pref_degree << endl
      ;
}

void RegularPrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
   // uint64_t page = address >> LOG2_PAGE_SIZE;
   uint64_t cl_addr = address >> LOG2_BLOCK_SIZE; //cacheline address
	// uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

   stats.tracker.lookup++;

   //update learn granularity
   uint64_t cl_addr_delta = abs((int64_t)cl_addr - (int64_t)last_cl_addr);
   if(cl_addr_delta < MIN_GRANULARITY_LINE && last_cl_addr_delta < MIN_GRANULARITY_LINE){
      ++min_count;
      if(min_count > GRANULARITY_COUNT){
         learn_granuliraty = min_gran;
      }
   }else if(cl_addr_delta < MIDDLE_GRANULARITY_LINE && last_cl_addr_delta < MIDDLE_GRANULARITY_LINE){
      ++middle_count;
      if(middle_count > GRANULARITY_COUNT){
         learn_granuliraty = middle_gran;
      }
   }else{
      learn_granuliraty = max_gran;
   }
   last_cl_addr_delta = cl_addr_delta;
   last_cl_addr = cl_addr;

   RegTableEntry *tracker = NULL;
   uint64_t tag = (address >> 8) & 0xfff;
   int pos;
   auto it = find_if(reg_table.begin(), reg_table.end(), [tag](RegTableEntry *t){return t->tag == tag;});
   if(it == reg_table.end()) //not in table
   {
      if(reg_table.size() >= knob::RegTableEntry_num)
      {
         /* evict */
         RegTableEntry *victim = reg_table.back(); //remove the last one
         reg_table.pop_back();
         delete victim;
         stats.tracker.evict++;
      }

      tracker = new RegTableEntry();
      tracker->tag = tag;
      tracker->delta = 0;
      tracker->count ++;

      if(learn_granuliraty == min_gran){  //1 cacheline
         tracker->last_offset = (address >> LOG2_BLOCK_SIZE) & 0x3f;
         stats.granularity.min_granularity_count++;
      }
      else if(learn_granuliraty == middle_gran){  //16 cacheline
         tracker->last_offset = (address >> 10) & 0x3f;
         pos = (address >> LOG2_BLOCK_SIZE) & 0xf;
         tracker->bitmap[pos] = 1;
         stats.granularity.middle_granularity_count++;
      }else{  //256 cacheline
         tracker->last_offset = (address >> 14) & 0x3f;
         pos = (address >> 10) & 0xf;
         tracker->bitmap[pos] = 1;
         stats.granularity.max_granularity_count++;
      }

      reg_table.push_front(tracker);
      stats.tracker.insert++;
      return;
   }

   // in table
   stats.tracker.hit++;
   int32_t stride = 0;
   tracker = (*it);
   uint32_t offset;
   int bit_map[16] = {0};
   if(learn_granuliraty == min_gran){
      offset = (address >> LOG2_BLOCK_SIZE) & 0x3f;
      stats.granularity.min_granularity_count++;
   }else if(learn_granuliraty == middle_gran){
      offset = (address >> 10) & 0x3f;
      pos = (address >> LOG2_BLOCK_SIZE) & 0xf;
      tracker->bitmap[pos] = 1;
      stats.granularity.middle_granularity_count++;
   }else{
      offset = (address >> 14) & 0x3f;
      pos = (address >> 10) & 0xf;
      tracker->bitmap[pos] = 1;
      stats.granularity.max_granularity_count++;
   }

   stride = (int32_t)offset - (int32_t)tracker->last_offset;
   tracker->last_offset = offset;
   for(int i = 0; i < 16; i ++){
      bit_map[i] = tracker->bitmap[i];
   }
   //bit_map.assign(tracker->bitmap.begin(),tracker->bitmap.end());
   if(stride == tracker->delta && tracker->count > PREFETCH_THRESHOLD){
      stats.pref.stride_match++;
      uint32_t count = generate_prefetch(address, stride, bit_map, pref_addr);
      stats.pref.generated += count;
   }else if(stride == tracker->delta && tracker->count <= PREFETCH_THRESHOLD){
      tracker->count++;
   }else{
      tracker->delta = stride;
      tracker->count = 1;
   }

   /* update tracker */
   reg_table.erase(it);
   reg_table.push_front(tracker);
}

uint32_t RegularPrefetcher::generate_prefetch(uint64_t address, int32_t stride, int* bitmap, vector<uint64_t> &pref_addr)
{
   uint64_t addr0; // base address
   uint64_t addr; //prefetch address
	uint32_t offset;
   uint32_t count = 0;

   for(uint32_t deg = 0; deg < knob::reg_pref_degree; ++deg)
   {
      if(learn_granuliraty == min_gran){ //1 cacheline
         offset = (address >> LOG2_BLOCK_SIZE) & 0x3f;
         addr0 = address >> 12;
         int32_t pref_offset = offset + stride*deg;
         if(pref_offset >= 0 && pref_offset < 64) /* bounds check */{
            addr = (addr0 << 12) + (pref_offset << LOG2_BLOCK_SIZE);
            pref_addr.push_back(addr);
         }
         else{
            break;
         }
      }else if(learn_granuliraty == middle_gran){ //prefetch 1 from 16 cacheline
         offset = (address >> 10) & 0x3f;
         addr0 = address >> 16;
         int32_t pref_offset = offset + stride*deg;
         if(pref_offset >= 0 && pref_offset < 64){
            for(int i = 0; i < 16; i++){
               if(bitmap[i] == 1){
                  addr = (addr0 << 16) + (pref_offset << 10) + (i << 6);
                  pref_addr.push_back(addr);
               }
            }
         }else{
            break;
         }
      }else{ //prefetch 16 from 256 cacheline
         offset = (address >> 14) & 0x3f;
         addr0 = address >> 20;
         int32_t pref_offset = offset + stride*deg;
         if(pref_offset >= 0 && pref_offset < 64){
            for(int i = 0; i < 16; i++){
               if(bitmap[i] == 1){
                  addr = (addr0 << 20) + (pref_offset << 14) + (i << 10);
                  pref_addr.push_back(addr);  
               }
            }
         }else{
            break;
         }
      }
      count++;
   }

   assert(count <= knob::reg_pref_degree);
   return count;
}

void RegularPrefetcher::dump_stats()
{
   cout << "reg_tracker_lookup " << stats.tracker.lookup << endl
      << "reg_tracker_evict " << stats.tracker.evict << endl
      << "reg_tracker_insert " << stats.tracker.insert << endl
      << "reg_tracker_hit " << stats.tracker.hit << endl
      << "reg_min_granularity_count " << stats.granularity.min_granularity_count << endl
      << "reg_middle_granularity_count " << stats.granularity.middle_granularity_count << endl
      << "reg_max_granularity_count "<< stats.granularity.max_granularity_count << endl
      << "reg_pref_stride_match " << stats.pref.stride_match << endl
      << "reg_pref_generated " << stats.pref.generated << endl
      << endl;
}