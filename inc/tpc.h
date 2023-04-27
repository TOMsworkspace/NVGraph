#ifndef TPC_H
#define TPC_H

#include "prefetcher.h"
#include "champsim.h"
#include "ooo_cpu.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <bitset>

using namespace std;

class TPCPrefetcher;

bool p1_inst_tain(TPCPrefetcher * tpc_prefetcher, uint64_t pc, uint64_t address, uint32_t src_reg, uint32_t dec_reg);

namespace knob
{
   extern uint32_t tpc_c1_instruction_monitor_size;
	extern uint32_t tpc_c1_region_monitor_size;
   extern uint32_t tpc_c1_total_region_threshold;
	extern uint32_t tpc_c1_dense_region_threshold;

   extern uint32_t tpc_t2p1_stride_identifier_table_size;
}

enum sit_state
{
   New,
   Ready,
   U1,
   U2,
   U3,
   U4,
   S1,
   S2,
   S3,
   S4
};

enum sit_entry_type{
   None,
   T2,
   P1
};

enum tpc_mode{
   l1_mode,
   l2_mode
};

struct t2_lb_register
{
   uint64_t loop_pc;
   bool state;
   uint32_t cnt;
   uint64_t target_pc;

   t2_lb_register() : loop_pc(0), state(false), cnt(0), target_pc(0) {}
};

struct stride_identifier_table_entry
{
   enum sit_state state;
   enum sit_entry_type type;
   int32_t delta;
   uint64_t modified_pc;
   uint64_t last_addr;
   uint64_t last_pref_addr;

   //stride_identifier_table_entry() : state(sit_state::New), type(sit_entry_type::None), delta(0), modified_pc(UINT64_MAX), last_addr(UINT64_MAX), last_pref_addr(UINT64_MAX) {}
   stride_identifier_table_entry(sit_state state = sit_state::New, sit_entry_type type = sit_entry_type::None, int32_t delta = 0, uint64_t modified_pc = UINT64_MAX, uint64_t last_addr = UINT64_MAX, uint64_t last_pref_addr = UINT64_MAX) 
   : state(state), type(type), delta(delta), modified_pc(modified_pc), last_addr(last_addr), last_pref_addr(last_pref_addr) 
   {}
};


class stride_identifier_table{

private:

   unordered_map<uint64_t, struct stride_identifier_table_entry> sit_table;

public:

   stride_identifier_table() {};
   ~stride_identifier_table() {};

   void add_entry(sit_state state, sit_entry_type type, int32_t delta, uint64_t modified_pc, uint64_t last_addr) {
      if(!sit_table.count(modified_pc) && !is_full()){
         sit_table[modified_pc] = stride_identifier_table_entry(state, type, delta, modified_pc, last_addr);
      }
   }

   void update_entry(uint64_t modified_pc, int32_t delta, uint64_t last_addr){
      if(sit_table.count(modified_pc)){
         sit_table[modified_pc].delta = delta;
         sit_table[modified_pc].last_addr = last_addr;
      }
   }

   void update_last_perf_addr(uint64_t modified_pc, uint64_t last_perf_addr){
      if(sit_table.count(modified_pc)){
         sit_table[modified_pc].last_pref_addr = last_perf_addr;
      }
   }

   void evict_entry(uint64_t modified_pc) {
      if(sit_table.count(modified_pc))
         sit_table.erase(modified_pc);
   }

   inline uint32_t caculate_delta(uint64_t modified_pc, uint64_t addr){
      if(sit_table.count(modified_pc)){
         return addr - sit_table[modified_pc].last_addr;
      }

      return INT32_MAX;
   }

   bool is_stable(uint64_t modified_pc, sit_state stable_state = sit_state::S4) const {
      return sit_table.count(modified_pc) && (
         sit_table.at(modified_pc).state == stable_state 
         || sit_table.at(modified_pc).state == sit_state::S2
         || sit_table.at(modified_pc).state == sit_state::S3
         || sit_table.at(modified_pc).state == sit_state::S4);
   }

   bool is_unstable(uint64_t modified_pc, sit_state unstable_state = sit_state::U4) const {
      return sit_table.count(modified_pc) && (
         sit_table.at(modified_pc).state == unstable_state
         || sit_table.at(modified_pc).state == sit_state::U4);
   }

   bool is_tracked(uint64_t modified_pc) const {
      return sit_table.count(modified_pc);
   }

   bool is_delta_matched(uint64_t modified_pc, int32_t delta){
      if(sit_table.count(modified_pc)){
         return sit_table[modified_pc].delta == delta;
      }

      return false;
   }

   bool is_type_matched(uint64_t modified_pc, enum sit_entry_type type) const {
      return sit_table.count(modified_pc) && sit_table.at(modified_pc).type == type;
   }

   bool is_full() const {
      return sit_table.size() > knob::tpc_t2p1_stride_identifier_table_size;
   }

   void prefetch_state_transition(uint64_t pc, bool is_matched);

};



/**
 * @brief tpu: taint propagation unit
 * 
 */
class taint_propagation_unit{

private:
   bitset<4096> reg_taint_bit_vector;
   bool is_busy;

public:
   taint_propagation_unit():reg_taint_bit_vector(0), is_busy(false){}
   ~taint_propagation_unit(){}

   bool is_taint(int reg_index){
      return reg_taint_bit_vector[reg_index];
   }

   void set_taint(int reg_index) {
      reg_taint_bit_vector.set(reg_index, true);
   }

   void reset_tain(int reg_index) {
      reg_taint_bit_vector.reset(reg_index);
   }

   void set_busy(){
      is_busy = true;
   }

   void reset_busy(){
      is_busy = false;
   }

   bool busy() const {return is_busy;}

   void free_tpu(){
      is_busy = false;
      reg_taint_bit_vector = 0;
   }
};


/**
 * @brief instruction monitor for c1 prefetcher
 * 
 */
class instruction_monitor{

   struct instruction_monitor_entry{
      bool valid;
      uint8_t id;
      uint8_t total_region;
      uint8_t densor_region;
      uint64_t pc;

      instruction_monitor_entry(bool valid = false, uint8_t id = UINT8_MAX, uint8_t total_region = 0, uint8_t densor_region = 0, uint64_t pc = UINT64_MAX)
      :valid(valid), id(id), total_region(total_region), densor_region(densor_region), pc(pc)
      {}
   };

   private:
      vector<struct instruction_monitor_entry> im;

      struct
      {
         uint64_t lookup;
         uint64_t evict;
         uint64_t insert;
         uint64_t hit;
      } stats;
   
   public:
      instruction_monitor() {

         bzero(&stats, sizeof(stats));

         for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
            im.emplace_back(instruction_monitor_entry());
            im[i].id = i;
         }
      }
      ~instruction_monitor() {}

      void init_entry(int im_id, uint64_t pc){
         assert(im_id < (int)knob::tpc_c1_instruction_monitor_size);
         im[im_id].valid = true;
         im[im_id].total_region = 0;
         im[im_id].densor_region = 0;
         im[im_id].pc = pc;
         stats.insert++;
      }

      void evict_entry(int im_id){
         assert(im_id < (int)knob::tpc_c1_instruction_monitor_size);
         im[im_id].valid = false;
         im[im_id].total_region = 0;
         im[im_id].densor_region = 0;
         im[im_id].pc = UINT64_MAX;
      }

      uint32_t find_pc(uint64_t pc){

         stats.lookup++;

         uint32_t pc_id = knob::tpc_c1_instruction_monitor_size;
         for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
            if(im[i].valid && im[i].pc == pc) pc_id = im[i].id;
         }

         if(pc_id < knob::tpc_c1_instruction_monitor_size)
            stats.hit++;

         return pc_id;
      }

      void update_entry(uint64_t pc_bit_vector, bool is_densor_region){
         for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
            if((pc_bit_vector & (1 << i)) && im[i].valid){
               im[i].total_region++;
               if(is_densor_region) im[i].densor_region++;
            }
         }
      }

      uint32_t find_victim(){

         uint32_t victim_id = knob::tpc_c1_instruction_monitor_size;

         for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
            if(victim_id == knob::tpc_c1_instruction_monitor_size && !im[i].valid){
               victim_id = i;
               break;
            }
         }

         if(victim_id  != knob::tpc_c1_instruction_monitor_size) return victim_id;

         for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
            if(im[i].total_region > knob::tpc_c1_total_region_threshold){
               victim_id = i;
               stats.evict++;
               break;
            }
         }

         //assert(victim_id >= 0 && victim_id <= knob::tpc_c1_instruction_monitor_size);

         return victim_id;
      }

      bool trained_by_instruction_monitor(uint64_t pc){
         uint32_t id = find_pc(pc);
         
         if(id == knob::tpc_c1_instruction_monitor_size) return false;

         return im[id].valid && im[id].total_region > knob::tpc_c1_total_region_threshold;

      }

      bool dense_pc(uint64_t pc){
         uint32_t id = find_pc(pc);
         
         if(id == knob::tpc_c1_instruction_monitor_size) return false;

         return im[id].valid && im[id].total_region > knob::tpc_c1_total_region_threshold 
         && im[id].densor_region > knob::tpc_c1_dense_region_threshold;

      }

      void dump_stats() const {
         cout << "tpc_c1_im_lookup " << stats.lookup << endl
         << "tpc_c1_im_evict " << stats.evict << endl
         << "tpc_c1_im_insert " << stats.insert << endl
         << "tpc_c1_im_hit " << stats.hit << endl;
      }
};

/**
 * @brief region monitor for c1 prefetcher
 * 
 */

class region_monitor{
   struct region_monitor_entry{
      bool valid;
      uint32_t lru;
      uint64_t region_tag;
      uint64_t cache_line_bit_vector;
      uint64_t pc_bit_vector;

      region_monitor_entry(bool valid = false, uint32_t lru = UINT32_MAX, uint64_t region_tag = UINT64_MAX, uint64_t cache_line_bit_vector = 0, uint64_t pc_bit_vector = 0)
      :valid(valid), lru(lru), region_tag(region_tag), cache_line_bit_vector(cache_line_bit_vector), pc_bit_vector(pc_bit_vector)
      {}
   };

private:

   vector<struct region_monitor_entry> rm;


   struct
   {
      uint64_t lookup;
      uint64_t evict;
      uint64_t insert;
      uint64_t hit;
   } stats;

   void lru_increase_cnt(){
      for(uint32_t i = 0; i < knob::tpc_c1_region_monitor_size; i++){
         if(rm[i].valid){
            rm[i].lru++;
         }
      }
   }

   bool set_cache_line_bit(int id, uint64_t region_tag, uint64_t cache_line_id){
      assert(id < (int)knob::tpc_c1_region_monitor_size);
      assert(cache_line_id < 64);

      if(rm[id].valid && rm[id].region_tag == region_tag){
         rm[id].cache_line_bit_vector &= (1 << (cache_line_id));
      }

      return true;
   }

   bool set_pc_bit(int id, uint64_t pc_id){
      assert(id < (int)knob::tpc_c1_region_monitor_size);
      assert(pc_id < knob::tpc_c1_instruction_monitor_size);

      if(rm[id].valid){
         rm[id].pc_bit_vector &= (1 << (pc_id));
      }

      return true;
   }

   bool reset_pc_bit(int id, uint64_t pc_id){
      assert(id < (int)knob::tpc_c1_region_monitor_size);
      assert(pc_id < knob::tpc_c1_instruction_monitor_size);

      if(rm[id].valid){
         rm[id].pc_bit_vector &= ~(1 << (pc_id));
      }

      return true;
   }

   bool is_dense_region(int id){
      assert(id < (int)knob::tpc_c1_region_monitor_size);
      uint32_t cnt = 0;

      uint64_t bit_vector = rm[id].cache_line_bit_vector;

      while(bit_vector > 0){
         cnt += (bit_vector & 1);
         bit_vector >>= 1;
      }
      
      return cnt >= knob::tpc_c1_dense_region_threshold;
   }

   void evict_entry(int id){
      assert(id < (int)knob::tpc_c1_region_monitor_size);

      rm[id].valid = false;
      rm[id].lru = UINT32_MAX;
      rm[id].region_tag = UINT64_MAX;
      rm[id].cache_line_bit_vector = 0;
      rm[id].pc_bit_vector = 0;
   }

   void init_entry(int id, uint64_t region_tag){
      assert(id < (int)knob::tpc_c1_region_monitor_size);
      rm[id].valid = true;
      rm[id].lru = 0;
      rm[id].region_tag = region_tag;
      rm[id].cache_line_bit_vector = 0;
      rm[id].pc_bit_vector = 0;
   }

   uint32_t lru_find_victim_rm_entry(){
      uint32_t victim_id = knob::tpc_c1_region_monitor_size;
      uint32_t maxcnt = 0;

      for(uint32_t i = 0; i < knob::tpc_c1_region_monitor_size; i++){
         if(!rm[i].valid && victim_id == knob::tpc_c1_region_monitor_size){
            victim_id = i;
            stats.evict++;
            break;
         }
      }

      if(victim_id < knob::tpc_c1_region_monitor_size) return victim_id;

      for(uint32_t i = 0; i < knob::tpc_c1_region_monitor_size; i++){
         if(rm[i].valid && rm[i].lru > maxcnt ){
               victim_id = i;
               maxcnt = rm[i].lru;
         }
      }

      assert(victim_id >= 0 && victim_id < knob::tpc_c1_region_monitor_size);

      return victim_id;
   }

public:
   region_monitor(){

      bzero(&stats, sizeof(stats));

      for(uint32_t i = 0; i < knob::tpc_c1_region_monitor_size; i++){
        rm.emplace_back(region_monitor_entry());
      }
   }

   ~region_monitor() {}

   uint64_t region_monitor_operate(uint64_t address, int pc_id, uint64_t & evict_pc_bit_vector, bool & densor_range){
      assert((uint32_t)pc_id < knob::tpc_c1_region_monitor_size);

      //uint64_t evict_pc_bit_vector = 0;
      //bool densor_range = false;
      
      uint64_t region_tag = address >> LOG2_PAGE_SIZE;

      uint32_t cache_line_id = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

      uint32_t rm_id = knob::tpc_c1_region_monitor_size;

      stats.lookup++;

      for(uint32_t i = 0; i < knob::tpc_c1_region_monitor_size; i++){
         if(rm[i].valid && region_tag == rm[i].region_tag){
            rm_id = i;
            stats.hit++;
            break;
         }
      }

      if(rm_id == knob::tpc_c1_region_monitor_size){
         rm_id = lru_find_victim_rm_entry();
         evict_pc_bit_vector = rm[rm_id].pc_bit_vector;
         densor_range = is_dense_region(rm_id);
         init_entry(rm_id, region_tag);

         stats.insert++;
      }

      set_cache_line_bit(rm_id, region_tag, cache_line_id);
      set_pc_bit(rm_id, pc_id);
      lru_increase_cnt();
      rm[rm_id].lru = 0;


      return evict_pc_bit_vector;

   }

   void update_pc_bit_vector(uint32_t pc_id){
      for(uint32_t i = 0; i < knob::tpc_c1_instruction_monitor_size; i++){
         reset_pc_bit(i, pc_id);
      }
   }

   void dump_stats() const {
      cout << "tpc_c1_rm_lookup " << stats.lookup << endl
      << "tpc_c1_rm_evict " << stats.evict << endl
      << "tpc_c1_rm_insert " << stats.insert << endl
      << "tpc_c1_rm_hit " << stats.hit << endl;
   }

};

class T2Prefetcher : public Prefetcher
{
private:
   deque<uint64_t> non_loop_pc_table;
   struct t2_lb_register loop_branch_register;
   class stride_identifier_table stride_identifier_table;

   bool is_stride_pc;

   /* stats */
   struct
   {

      struct
      {
         uint64_t lookup;
         uint64_t non_loop_branch;
         uint64_t loop_branch;
         uint64_t pc_matched;
      } lp;

      struct
      {
         uint64_t lookup;
         uint64_t evict;
         uint64_t insert;
         uint64_t hit;

      } sit;

      struct
      {
         uint64_t pos;
         uint64_t neg;
         uint64_t zero;
      } stride;

      struct
      {
         uint64_t stride_match;
         uint64_t generated;
      } pref;
   
   } stats;

private:
   void init_knobs();
   void init_stats();
   bool loop_switch(bool backward_branch, uint64_t pc, uint64_t target_pc);
   uint64_t generate_modified_pc(uint64_t pc, uint64_t ras);
   uint32_t generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr);

public:
   T2Prefetcher(string type);
   ~T2Prefetcher();
   void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
   void dump_stats();
   void print_config();
   bool trained_by_t2(uint64_t pc);

   //must called after invoke_prefetcher()
   bool stride_pc() const;
};


class TPCPrefetcher : public Prefetcher
{
private:

   // tpc mode, run as l1 or l2 prefetcher

   enum tpc_mode mode;

   /**t2*/
   deque<uint64_t> non_loop_pc_table;
   struct t2_lb_register loop_branch_register;

   class stride_identifier_table stride_identifier_table;
   
   /**p1*/
   class taint_propagation_unit taint_propagation_unit;

   /**c1*/
   class instruction_monitor instruction_monitor;
   class region_monitor region_monitor;

   /* stats */
   struct
   {
      struct
      {
         uint64_t lookup;
         uint64_t non_loop_branch;
         uint64_t loop_branch;
         uint64_t pc_matched;
      } lp;

      struct
      {
         uint64_t lookup;
         uint64_t t2_lookup;
         uint64_t p1_lookup;
         uint64_t evict;
         uint64_t t2_evict;
         uint64_t p1_evict;
         uint64_t insert;
         uint64_t t2_insert;
         uint64_t p1_insert;
         uint64_t hit;
         uint64_t t2_hit;
         uint64_t p1_hit;
      } sit;

      struct
      {
         uint64_t pos;
         uint64_t neg;
         uint64_t zero;
      } stride;

      struct
      {
         uint64_t stride_match;
         uint64_t generated;
      } t2_pref;

      struct
      {
         uint64_t ptr_match;
         uint64_t generated;
      } p1_pref;

      struct{
         uint64_t dense_match;
         uint64_t generated;
      } c1_pref;

   } stats;

private:
   void init_knobs();
   void init_stats();

   /** t2 */
   bool t2_loop_switch(bool backward_branch, uint64_t pc, uint64_t target_pc);
   uint64_t t2_generate_modified_pc(uint64_t pc, uint64_t ras = 0);
   uint32_t t2_generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr);

   /** p1*/
   
   uint64_t p1_generate_modified_pc(uint64_t pc, uint64_t ras = 0xa0a0a0a0a0a0a0a0);
   uint32_t p1_generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr);

   /** c1*/
   uint32_t c1_generate_prefetch(uint64_t address, vector<uint64_t> &pref_addr);

   
   
public:

   TPCPrefetcher(string type, enum tpc_mode mode = tpc_mode::l1_mode);
   ~TPCPrefetcher();
   friend bool p1_inst_tain(TPCPrefetcher * tpc_prefetcher, uint64_t pc, uint64_t address, uint32_t src_reg, uint32_t dec_reg);

   void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
   void l1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
   void l2_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
   void dump_stats();
   void print_config();
   

   /**t2*/
   bool trained_by_t2(uint64_t pc);
   bool stride_pc(uint64_t pc);
   void t2_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);

   /**p1*/
   bool trained_by_p1(uint64_t pc);
   bool ptr_pc(uint64_t pc);
   void p1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);

   /**c1*/
   bool trained_by_c1(uint64_t pc);
   bool dense_pc(uint64_t pc);
   void c1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
};

#endif /* TPC_H */
