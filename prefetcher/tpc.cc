#include "tpc.h"
#include "champsim.h"

namespace knob
{
    /**t2*/
    extern uint32_t t2_non_loop_table_size;
    extern uint32_t t2_min_seg_size;
    extern uint32_t t2_loop_branch_register_size;

    extern uint32_t t2_stride_identifier_table_size;
    extern uint32_t t2_perf_degree;

    /**tpc*/
	extern uint32_t tpc_t2_non_loop_table_size;
	extern uint32_t tpc_t2_min_seg_size;
	extern uint32_t tpc_t2_loop_branch_register_size;
    extern uint32_t tpc_t2_perf_degree; /* default is set 1*/
	
    extern uint32_t tpc_t2p1_stride_identifier_table_size;
	extern uint32_t tpc_p1_tain_reg_range;
	extern uint32_t tpc_p1_perf_degree; /* default is set 1*/
	
    extern uint32_t tpc_c1_instruction_monitor_size;
	extern uint32_t tpc_c1_region_monitor_size;
    extern uint32_t tpc_c1_total_region_threshold;
	extern uint32_t tpc_c1_dense_region_threshold;
	extern uint32_t tpc_c1_perf_degree; /* default is set 1*/

}

void stride_identifier_table::prefetch_state_transition(uint64_t modified_pc, bool is_matched)
{
    if (!is_tracked(modified_pc))
    {
        return;
    }

    struct stride_identifier_table_entry &entry = sit_table[modified_pc];

    // prefetch state transition

    switch (entry.state)
    {
    case sit_state::New:
        if (is_matched)
            entry.state = sit_state::Ready;
        break;

    case sit_state::Ready:
        if (is_matched)
            entry.state = sit_state::S1;
        else
            entry.state = sit_state::U1;
        break;

    case sit_state::S1:
        if (is_matched)
            entry.state = sit_state::S2;
        else
            entry.state = sit_state::U1;
        break;

    case sit_state::S2:
        if (is_matched)
            entry.state = sit_state::S3;
        else
            entry.state = sit_state::U1;
        break;

    case sit_state::S3:
        if (is_matched)
            entry.state = sit_state::S4;
        else
            entry.state = sit_state::U1;
        break;

    case sit_state::U1:
        if (is_matched)
            entry.state = sit_state::S1;
        else
            entry.state = sit_state::U2;
        break;

    case sit_state::U2:
        if (is_matched)
            entry.state = sit_state::S1;
        else
            entry.state = sit_state::U3;
        break;

    case sit_state::U3:
        if (is_matched)
            entry.state = sit_state::U4;
        else
            entry.state = sit_state::S1;
        break;

    default:
        // state == U4 OR S4, nothing to do.
        break;
    }
}

void T2Prefetcher::init_knobs()
{
}

void T2Prefetcher::init_stats()
{
    bzero(&stats, sizeof(stats)); // set the stats' first sizeof(stats) bytes to 0
}

bool T2Prefetcher::loop_switch(bool backward_branch, uint64_t pc, uint64_t target_pc)
{

    if (!backward_branch)
        return false;

    stats.lp.lookup++;

    for (auto loop_pc : non_loop_pc_table)
    {
        if (loop_pc == pc)
        {
            stats.lp.non_loop_branch++;
            return false;
        }
    }

    if (loop_branch_register.loop_pc == pc)
    {
        loop_branch_register.state = true;
        loop_branch_register.cnt++;

        if (loop_branch_register.cnt > knob::t2_min_seg_size)
        {
            non_loop_pc_table.push_back(pc);

            if (non_loop_pc_table.size() > knob::t2_non_loop_table_size)
            {
                non_loop_pc_table.pop_front();
            }
        }

        stats.lp.pc_matched++;
        stats.lp.non_loop_branch++;

        return false;
    }
    else
    {
        loop_branch_register.loop_pc = pc;
        loop_branch_register.target_pc = target_pc;
        loop_branch_register.state = false;
        loop_branch_register.cnt = 0;
    }

    stats.lp.loop_branch++;
    return true;
}

uint64_t T2Prefetcher::generate_modified_pc(uint64_t pc, uint64_t ras = 0)
{
    return pc ^ ras;
}

uint32_t T2Prefetcher::generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr)
{
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
    uint32_t count = 0;

    for (uint32_t deg = 0; deg < knob::t2_perf_degree; ++deg)
    {
        int32_t pref_offset = offset + stride * deg;
        if (pref_offset >= 0 && pref_offset < 64) /* bounds check */
        {
            uint64_t addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            pref_addr.push_back(addr);
            count++;
        }
        else
        {
            break;
        }
    }

    assert(count <= knob::t2_perf_degree);
    return count;
}

T2Prefetcher::T2Prefetcher(string type) : Prefetcher(type)
{
    is_stride_pc = false;
}

T2Prefetcher::~T2Prefetcher()
{
}

void T2Prefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{

    // uint64_t page = address >> LOG2_PAGE_SIZE;
    uint64_t cl_addr = address >> LOG2_BLOCK_SIZE;
    // uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    uint64_t modified_pc = generate_modified_pc(pc, 0);

    // filter non-loop switch code
    if (!loop_switch(true, modified_pc, cl_addr))
    {
        is_stride_pc = false;
        return;
    }

    bool is_tracked = stride_identifier_table.is_tracked(modified_pc);

    bool is_matched = false;

    int32_t delta = 0;

    stats.sit.lookup++;

    if (is_tracked)
    {
        delta = stride_identifier_table.caculate_delta(modified_pc, cl_addr);
        is_matched = stride_identifier_table.is_delta_matched(modified_pc, delta);

        if (is_matched)
            stats.pref.stride_match++;

        stats.sit.hit++;

        if (delta == 0)
        {
            stats.stride.zero++;
        }
        else if (delta > 0)
        {
            stats.stride.pos++;
        }
        else
        {
            stats.stride.neg++;
        }

        stride_identifier_table.prefetch_state_transition(modified_pc, is_matched);

        if (stride_identifier_table.is_unstable(modified_pc, sit_state::U4))
        {

            // invalid SIT entry
            stats.sit.evict++;
            stride_identifier_table.evict_entry(modified_pc);
            is_stride_pc = false;
            return;
        }
        else if (stride_identifier_table.is_stable(modified_pc, sit_state::S4))
        {
            // prefecth;

            is_stride_pc = true;

            uint32_t count = generate_prefetch(address, delta, pref_addr);

            stats.pref.generated += count;

            if (count > 0)
            {
                //stride_identifier_table[modified_pc].last_pref_addr = pref_addr[pref_addr.size() - count];
                stride_identifier_table.update_last_perf_addr(modified_pc, pref_addr[pref_addr.size() - count]);
            }
        }

        stride_identifier_table.update_entry(modified_pc, delta, cl_addr);
        // stride_identifier_table[modified_pc].delta = delta;
        // stride_identifier_table[modified_pc].last_addr = cl_addr;
    }
    else
    {
        is_stride_pc = false;
        stats.sit.insert++;
        if (stride_identifier_table.is_full())
            return;

        stride_identifier_table.add_entry(sit_state::New, sit_entry_type::T2, delta, modified_pc, cl_addr);
        //stride_identifier_table[modified_pc] = t2_stride_identifier_table_entry(sit_state::New, delta, modified_pc, cl_addr);
    }
}

void T2Prefetcher::dump_stats()
{

    cout << "t2_lp_lookup " << stats.lp.lookup << endl
         << "t2_lp_loop_branch " << stats.lp.loop_branch << endl
         << "t2_lp_non_loop_branch " << stats.lp.non_loop_branch << endl
         << "t2_lp_pc_matched " << stats.lp.pc_matched << endl
         << "t2_sit_lookup " << stats.sit.lookup << endl
         << "t2_sit_evict " << stats.sit.evict << endl
         << "t2_sit_insert " << stats.sit.insert << endl
         << "t2_sit_hit " << stats.sit.hit << endl
         << "t2_stride_pos " << stats.stride.pos << endl
         << "t2_stride_neg " << stats.stride.neg << endl
         << "t2_stride_zero " << stats.stride.zero << endl
         << "t2_pref_stride_match " << stats.pref.stride_match << endl
         << "t2_pref_generated " << stats.pref.generated << endl;
}

void T2Prefetcher::print_config()
{

    cout << "t2_non_loop_table_size " << knob::t2_non_loop_table_size << endl
         << "t2_min_seg_size " << knob::t2_min_seg_size << endl
         << "t2_loop_branch_register_size " << knob::t2_loop_branch_register_size << endl
         << "t2_stride_identifier_table_size " << knob::t2_stride_identifier_table_size << endl
         << "t2_perf_degree " << knob::t2_perf_degree << endl;
}

bool T2Prefetcher::trained_by_t2(uint64_t pc)
{
    uint64_t modified_pc = generate_modified_pc(pc);
    return stride_identifier_table.is_tracked(modified_pc) && stride_identifier_table.is_stable(modified_pc);
}

bool T2Prefetcher::stride_pc() const
{
    return is_stride_pc;
}


TPCPrefetcher::TPCPrefetcher(string type, enum tpc_mode mode) : Prefetcher(type), mode(mode){
}

TPCPrefetcher::~TPCPrefetcher()
{
}


void TPCPrefetcher::init_knobs()
{
}

void TPCPrefetcher::init_stats()
{
    bzero(&stats, sizeof(stats)); // set the stats' first sizeof(stats) bytes to 0
}

void TPCPrefetcher::dump_stats()
{   
    if(mode == tpc_mode::l1_mode){
        cout << "tpc_t2_lp_lookup " << stats.lp.lookup << endl
        << "tpc_t2_lp_loop_branch " << stats.lp.loop_branch << endl
        << "tpc_t2_lp_non_loop_branch " << stats.lp.non_loop_branch << endl
        << "tpc_t2_lp_pc_matched " << stats.lp.pc_matched << endl
        << "tpc_sit_lookup " << stats.sit.lookup << endl
        << "tpc_t2_sit_lookup " << stats.sit.t2_lookup << endl
        << "tpc_p1_sit_lookup " << stats.sit.p1_lookup << endl
        << "tpc_sit_evict " << stats.sit.evict << endl
        << "tpc_t2_sit_evict " << stats.sit.t2_evict << endl
        << "tpc_p1_sit_evict " << stats.sit.p1_evict << endl
        << "tpc_sit_insert " << stats.sit.insert << endl
        << "tpc_t2_sit_insert " << stats.sit.t2_insert << endl
        << "tpc_p1_sit_insert " << stats.sit.p1_insert << endl
        << "tpc_sit_hit " << stats.sit.hit << endl
        << "tpc_t2_sit_hit " << stats.sit.t2_hit << endl
        << "tpc_p1_sit_hit " << stats.sit.p1_hit << endl
        << "tpc_t2_stride_pos " << stats.stride.pos << endl
        << "tpc_t2_stride_neg " << stats.stride.neg << endl
        << "tpc_t2_stride_zero " << stats.stride.zero << endl
        << "tpc_t2_pref_stride_match " << stats.t2_pref.stride_match << endl
        << "tpc_t2_pref_generated " << stats.t2_pref.generated << endl
        << "tpc_p1_pref_ptr_match " << stats.p1_pref.ptr_match << endl
        << "tpc_p1_pref_ptr_generated " << stats.p1_pref.generated << endl;
    }
    else{
        instruction_monitor.dump_stats();
        region_monitor.dump_stats();
        
        cout << "tpc_c1_pref_dense_match " << stats.c1_pref.dense_match << endl
        << "tpc_c1_pref_generated " << stats.c1_pref.generated << endl;  
    }


}

void TPCPrefetcher::print_config()
{   
    if(mode == tpc_mode::l1_mode){
        cout << "tpc config of t2: " << endl
            << "tpc_t2_non_loop_table_size " << knob::tpc_t2_non_loop_table_size << endl
            << "tpc_t2_min_seg_size " << knob::tpc_t2_min_seg_size << endl
            << "tpc_t2_loop_branch_register_size " << knob::tpc_t2_loop_branch_register_size << endl
            << "tcp_t2_stride_identifier_table_size " << knob::tpc_t2p1_stride_identifier_table_size << endl
            << "tpc_t2_perf_degree " << knob::tpc_t2_perf_degree << endl
            << "tpc config of p1: " << endl
            << "tpc_p1_instruction_tain_reg_range " << knob::tpc_p1_tain_reg_range << endl
            << "tpc_p1_perf_degree " << knob::tpc_p1_perf_degree << endl;
    }
    else{
        cout << "tpc config of c1: " << endl
            << "tpc_c1_total_region_threshold " << knob::tpc_c1_total_region_threshold << endl
            << "tpc_c1_dense_region_threshold " << knob::tpc_c1_dense_region_threshold << endl
            << "tpc_c1_instruction_monitor_size " << knob::tpc_c1_instruction_monitor_size << endl
            << "tpc_c1_region_monitor_size " << knob::tpc_c1_region_monitor_size << endl
            << "tpc_c1_perf_degree " << knob::tpc_c1_perf_degree << endl; 
    }
    
}



bool TPCPrefetcher::t2_loop_switch(bool backward_branch, uint64_t pc, uint64_t target_pc){
    if (!backward_branch)
        return false;

    stats.lp.lookup++;

    for (auto loop_pc : non_loop_pc_table)
    {
        if (loop_pc == pc)
        {
            stats.lp.non_loop_branch++;
            
            return true;
            //return false;
        }
    }

    if (loop_branch_register.loop_pc == pc)
    {
        loop_branch_register.state = true;
        loop_branch_register.cnt++;

        if (loop_branch_register.cnt > knob::tpc_t2_min_seg_size)
        {
            non_loop_pc_table.push_back(pc);

            if (non_loop_pc_table.size() > knob::tpc_t2_non_loop_table_size)
            {
                non_loop_pc_table.pop_front();
            }
        }

        stats.lp.pc_matched++;
        stats.lp.non_loop_branch++;

        return true;
         //return false;
    }
    else
    {
        loop_branch_register.loop_pc = pc;
        loop_branch_register.target_pc = target_pc;
        loop_branch_register.state = false;
        loop_branch_register.cnt = 0;
    }

    stats.lp.loop_branch++;
    return true;
}

bool p1_inst_tain(TPCPrefetcher * tpc_prefetcher, uint64_t pc, uint64_t address, uint32_t src_reg, uint32_t dec_reg){
    uint64_t cl_addr = address >> LOG2_BLOCK_SIZE;
    // uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    uint64_t modified_pc = tpc_prefetcher->p1_generate_modified_pc(pc);

    if(tpc_prefetcher->stride_identifier_table.is_tracked(modified_pc) && tpc_prefetcher->stride_identifier_table.is_type_matched(modified_pc, sit_entry_type::P1)){
        tpc_prefetcher->taint_propagation_unit.free_tpu();
    }

    assert(dec_reg < knob::tpc_p1_tain_reg_range);

    tpc_prefetcher->taint_propagation_unit.set_taint(dec_reg);

    assert(src_reg < knob::tpc_p1_tain_reg_range);

    bool is_taint = tpc_prefetcher->taint_propagation_unit.is_taint(src_reg);

    if(is_taint){
        tpc_prefetcher->stride_identifier_table.add_entry(sit_state::New, sit_entry_type::P1, INT32_MAX, modified_pc, cl_addr);
    }

    return is_taint;
}

uint32_t TPCPrefetcher::t2_generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr){
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
    uint32_t count = 0;

    for (uint32_t deg = 0; deg < knob::tpc_t2_perf_degree; ++deg)
    {
        int32_t pref_offset = offset + stride * deg;
        if (pref_offset >= 0 && pref_offset < 64) /* bounds check */
        {
            uint64_t addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            pref_addr.push_back(addr);
            count++;
        }
        else
        {
            break;
        }
    }

    assert(count <= knob::tpc_t2_perf_degree);
    return count;
}

uint32_t TPCPrefetcher::p1_generate_prefetch(uint64_t address, int32_t stride, vector<uint64_t> &pref_addr){
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
    uint32_t count = 0;

    for (uint32_t deg = 0; deg < knob::tpc_p1_perf_degree; ++deg)
    {
        int32_t pref_offset = offset + stride * deg;
        if (pref_offset >= 0 && pref_offset < 64) /* bounds check */
        {
            uint64_t addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            pref_addr.push_back(addr);
            count++;
        }
        else
        {
            break;
        }
    }

    assert(count <= knob::tpc_p1_perf_degree);
    return count;
}

uint32_t TPCPrefetcher::c1_generate_prefetch(uint64_t address, vector<uint64_t> &pref_addr){
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
    uint32_t count = 0;

    for (uint32_t deg = 0; deg < knob::tpc_c1_perf_degree; ++deg)
    {
        int32_t pref_offset = offset + deg;
        if (pref_offset >= 0 && pref_offset < 64) /* bounds check */
        {
            uint64_t addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            pref_addr.push_back(addr);
            count++;
        }
        else
        {
            break;
        }
    }

    assert(count <= knob::tpc_c1_perf_degree);
    return count;
}

uint64_t TPCPrefetcher::t2_generate_modified_pc(uint64_t pc, uint64_t ras)
{
    return pc ^ ras;
}

uint64_t TPCPrefetcher::p1_generate_modified_pc(uint64_t pc, uint64_t ras)
{
    return pc ^ ras;
}


bool TPCPrefetcher::trained_by_t2(uint64_t pc){
    uint64_t modified_pc = t2_generate_modified_pc(pc);
    return stride_identifier_table.is_tracked(modified_pc) && stride_identifier_table.is_type_matched(modified_pc,sit_entry_type::T2);// && stride_identifier_table.is_stable(modified_pc);
}

bool TPCPrefetcher::stride_pc(uint64_t pc) {
    //return is_stride_pc;
    uint64_t modified_pc = t2_generate_modified_pc(pc);
    return stride_identifier_table.is_tracked(modified_pc) && stride_identifier_table.is_type_matched(modified_pc,sit_entry_type::T2)
    && stride_identifier_table.is_stable(modified_pc);
}

bool TPCPrefetcher::trained_by_p1(uint64_t pc){
    uint64_t modified_pc = p1_generate_modified_pc(pc);
    return stride_identifier_table.is_tracked(modified_pc) && stride_identifier_table.is_type_matched(modified_pc,sit_entry_type::P1);// && stride_identifier_table.is_stable(modified_pc);
}

bool TPCPrefetcher::ptr_pc(uint64_t pc) {
    //return is_stride_pc;
    uint64_t modified_pc = p1_generate_modified_pc(pc);
    return stride_identifier_table.is_tracked(modified_pc) && stride_identifier_table.is_type_matched(modified_pc,sit_entry_type::P1)
    && stride_identifier_table.is_stable(modified_pc);
}

bool TPCPrefetcher::trained_by_c1(uint64_t pc){
    return instruction_monitor.trained_by_instruction_monitor(pc);
}

bool TPCPrefetcher::dense_pc(uint64_t pc){
    return instruction_monitor.dense_pc(pc);
}


void TPCPrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){

    switch (mode)
    {
        case l1_mode:
            l1_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
            break;
        case l2_mode:
            l2_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
            break;
        default:
            break;
    }

}

void TPCPrefetcher::l1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){

    if(trained_by_t2(pc)){
        if(stride_pc(pc)){
            t2_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
        }

        if(trained_by_p1(pc)){
            if(ptr_pc(pc)){
                p1_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
            }
        }
        else{
            p1_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
        }
    }
    else{
        t2_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
    }

}

void TPCPrefetcher::l2_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){

    if(trained_by_c1(pc)){
        if(dense_pc(pc)){
            c1_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
        }
    }
    else{
        c1_invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
    }

}


void TPCPrefetcher::t2_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){
    // uint64_t page = address >> LOG2_PAGE_SIZE;
    uint64_t cl_addr = address >> LOG2_BLOCK_SIZE;
    // uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    uint64_t modified_pc = t2_generate_modified_pc(pc);

    // filter non-loop switch code
    if (!t2_loop_switch(true, modified_pc, cl_addr))
    {
        return;
    }

    bool is_tracked = stride_identifier_table.is_tracked(modified_pc);

    bool is_matched = false;

    int32_t delta = 0;

    stats.sit.lookup++;
    stats.sit.t2_lookup++;

    if (is_tracked)
    {
        delta = stride_identifier_table.caculate_delta(modified_pc, cl_addr);
        is_matched = stride_identifier_table.is_delta_matched(modified_pc, delta);

        if (is_matched)
            stats.t2_pref.stride_match++;

        stats.sit.hit++;
        stats.sit.t2_hit++;

        if (delta == 0)
        {
            stats.stride.zero++;
        }
        else if (delta > 0)
        {
            stats.stride.pos++;
        }
        else
        {
            stats.stride.neg++;
        }

        stride_identifier_table.prefetch_state_transition(modified_pc, is_matched);

        if (stride_identifier_table.is_unstable(modified_pc))
        {

            // invalid SIT entry
            stats.sit.evict++;
            stats.sit.t2_evict++;
            stride_identifier_table.evict_entry(modified_pc);
            return;
        }
        else if (stride_identifier_table.is_stable(modified_pc))
        {
            // prefecth;

            uint32_t count = t2_generate_prefetch(address, delta, pref_addr);

            stats.t2_pref.generated += count;

            if (count > 0)
            {
                stride_identifier_table.update_last_perf_addr(modified_pc, pref_addr[pref_addr.size() - count]);
            }
        }

        stride_identifier_table.update_entry(modified_pc, delta, cl_addr);

    }
    else
    {
        stats.sit.insert++;
        stats.sit.t2_insert++;
        if (stride_identifier_table.is_full())
            return;

        stride_identifier_table.add_entry(sit_state::New, sit_entry_type::T2, delta, modified_pc, cl_addr);
        //stride_identifier_table[modified_pc] = t2_stride_identifier_table_entry(sit_state::New, delta, modified_pc, cl_addr);
    }

}

void TPCPrefetcher::p1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){
    // uint64_t page = address >> LOG2_PAGE_SIZE;
    uint64_t cl_addr = address >> LOG2_BLOCK_SIZE;
    // uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    uint64_t modified_pc = p1_generate_modified_pc(pc);

    bool is_tracked = stride_identifier_table.is_tracked(modified_pc);

    bool is_matched = false;

    bool is_stride_pc = stride_pc(pc);

    int32_t delta = 0;

    stats.sit.lookup++;
    stats.sit.p1_lookup++;

    if(trained_by_p1(modified_pc)){

        stats.sit.hit++;
        stats.sit.p1_hit++;

        if(ptr_pc(pc)){
            if(is_stride_pc){

                if(is_tracked){
                    //prefetch
                }
                else{
                    //reset taints, set busy
                    taint_propagation_unit.free_tpu();
                    taint_propagation_unit.set_busy();
                    return;
                }
            }
            else{
                //prefetch
            }
        }
        else{
            return;
        }
    }
    else{
        if(is_stride_pc){
            //reset taints, set busy
            taint_propagation_unit.free_tpu();
            taint_propagation_unit.set_busy();
            return;
        }
        else{
            //test for self driving load, prefetch
        }

        // insert pc
        stats.sit.insert++;
        stats.sit.p1_insert++;
        if (stride_identifier_table.is_full())
            return;

        stride_identifier_table.add_entry(sit_state::New, sit_entry_type::P1, delta, modified_pc, cl_addr);
    }

    if(is_tracked && stride_identifier_table.is_type_matched(modified_pc, sit_entry_type::P1)){
        delta = stride_identifier_table.caculate_delta(modified_pc, cl_addr);
        is_matched = stride_identifier_table.is_delta_matched(modified_pc, delta);

        if(is_matched)
            stats.p1_pref.ptr_match++;

        stride_identifier_table.prefetch_state_transition(modified_pc, is_matched);

        if (stride_identifier_table.is_unstable(modified_pc))
        {
            stats.sit.evict++;
            stats.sit.p1_evict++;
            // invalid SIT entry
            stride_identifier_table.evict_entry(modified_pc);
            return;
        }
        else if (stride_identifier_table.is_stable(modified_pc))
        {
            // prefecth;

            uint32_t count = p1_generate_prefetch(address, delta, pref_addr);

            stats.p1_pref.generated += count;

            if (count > 0)
            {
                //stride_identifier_table[modified_pc].last_pref_addr = pref_addr[pref_addr.size() - count];
                stride_identifier_table.update_last_perf_addr(modified_pc, pref_addr[pref_addr.size() - count]);
            }
        }

        stride_identifier_table.update_entry(modified_pc, delta, cl_addr);
    }

}


void TPCPrefetcher::c1_invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr){
    // uint64_t page = address >> LOG2_PAGE_SIZE;
    //uint64_t cl_addr = address >> LOG2_BLOCK_SIZE;
    // uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    
    bool is_trained = trained_by_c1(pc);
    bool is_dense_pc = dense_pc(pc);

    if(is_trained){
        if(is_dense_pc){
            //prefetch

            stats.c1_pref.dense_match++;

            uint32_t count = c1_generate_prefetch(address, pref_addr);
            
            stats.c1_pref.generated += count;
        }
        else{
            return;
        }
    }
    else{
        uint32_t pc_id = instruction_monitor.find_pc(pc);

        bool is_tracked = pc_id < knob::tpc_c1_instruction_monitor_size;

        if(!is_tracked){
            pc_id = instruction_monitor.find_victim();
            if(pc_id < knob::tpc_c1_instruction_monitor_size){
                // evict entry in instruction monitor
                
                instruction_monitor.init_entry(pc_id, pc);
                region_monitor.update_pc_bit_vector(pc_id);
            }
        }
        else{
            bool is_dense_region = false;
            uint64_t pc_bit_vector = 0;

            region_monitor.region_monitor_operate(address, pc_id, pc_bit_vector, is_dense_region);

            instruction_monitor.update_entry(pc_bit_vector, is_dense_region);
        }

        
    }

    uint32_t count = c1_generate_prefetch(address, pref_addr);
            
    stats.c1_pref.generated += count;

}