#include <assert.h>
#include <algorithm>
#include <queue>
#include <iomanip>
#include "champsim.h"
#include "cache.h"
#include "memory_class.h"
#include "cbp.h"
#include "util.h"

#if 0
#	define LOCKED(...) {fflush(stdout); __VA_ARGS__; fflush(stdout);}
#	define LOGID() fprintf(stdout, "[%25s@%3u] ", \
							__FUNCTION__, __LINE__ \
							);
#	define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#	define MYLOG(...) {}
#endif


namespace knob
{

	/*init*/
	extern float           cbp_alpha;
	extern float           cbp_gamma;
	extern uint32_t        cbp_max_states;
	extern string          cbp_policy;
	extern string          cbp_learning_type;
	extern uint64_t        cbp_early_exploration_window;

	extern uint32_t        cbp_pt_size;
	extern uint32_t        cbp_st_size;
	extern uint32_t        cbp_hst_size;
	extern uint32_t        cbp_action_tracker_size;
	extern bool		       cbp_brain_zero_init;

	/*reward*/
	extern bool            cbp_enable_reward_all;
	extern bool            cbp_enable_hbw_reward;
	extern int32_t         cbp_reward_none;
	extern int32_t         cbp_reward_hbw_none;
	extern int32_t         cbp_reward_incorrect;
	extern int32_t         cbp_reward_hbw_incorrect;

	/*module*/
	extern uint32_t        cbp_state_type; /*state*/
	extern uint32_t        cbp_state_hash_type;

	/*feature*/
	extern uint32_t        cbp_pref_degree; /*dynamic pref degree*/
	extern uint32_t        cbp_high_bw_thresh;

	/*debug*/
	extern uint32_t        cbp_max_pcs;
	extern uint32_t        cbp_max_offsets;
	extern uint32_t        cbp_max_deltas;
	extern bool            cbp_enable_state_action_stats;
	extern uint32_t        cbp_state_action_stats_topK;
	extern bool            cbp_enable_track_multiple;

	extern bool            cbp_access_debug;
	extern bool            cbp_print_access_debug;
	extern uint64_t        cbp_print_access_debug_pc;
	extern uint32_t        cbp_print_access_debug_pc_count;
	extern bool            cbp_print_trace;

	/* CBP Learning Engine knobs */
	extern bool            cbp_le_enable_trace;
	extern uint32_t        cbp_le_trace_interval;
	extern std::string     cbp_le_trace_file_name;
	extern uint32_t        cbp_le_trace_state;
	extern bool            cbp_le_enable_score_plot;
	//extern std::vector<int32_t> cbp_le_plot_actions;
	extern std::string     cbp_le_plot_file_name;
	extern bool            cbp_le_enable_action_trace;
	extern uint32_t        cbp_le_action_trace_interval;
	extern std::string     cbp_le_action_trace_name;
	extern bool            cbp_le_enable_action_plot;
}

void CBP::init_knobs()
{

	if(knob::cbp_access_debug)
	{
		cout << "***WARNING*** setting knob::cbp_max_pcs, knob::cbp_max_offsets, and knob::cbp_max_deltas to large value as knob::cbp_access_debug is true" << endl;
		knob::cbp_max_pcs = 1024;
		knob::cbp_max_offsets = 1024;
		knob::cbp_max_deltas = 1024;
	}
	//assert(knob::cbp_pref_degree >= 1 && (knob::cbp_pref_degree == 1 || !knob::cbp_enable_dyn_degree));
	//assert(knob::cbp_max_to_avg_q_thresholds.size() == knob::cbp_dyn_degrees.size()-1);
	//assert(knob::cbp_last_pref_offset_conf_thresholds.size() == knob::cbp_dyn_degrees_type2.size()-1);
}

void CBP::init_stats()
{
	//bzero(&stats, sizeof(stats));

	bzero(&stats.st, sizeof(stats.st));

	stats.predict.called = 0;
	stats.predict.out_of_bounds = 0;
	stats.predict.action_dist.clear();
	stats.predict.issue_dist.clear();
	stats.predict.pred_hit.clear();
	stats.predict.predicted = 0;
	stats.predict.multi_deg = 0;
	stats.predict.multi_deg_called = 0;
	bzero(&stats.predict.multi_deg_histogram, sizeof(stats.predict.multi_deg_histogram));
	bzero(&stats.predict.deg_histogram, sizeof(stats.predict.deg_histogram));

	bzero(&stats.track, sizeof(&stats.track));

	bzero(&stats.reward.demand, sizeof(stats.reward.demand));
	bzero(&stats.reward.train, sizeof(stats.reward.train));
	bzero(&stats.reward.assign_reward, sizeof(stats.reward.assign_reward)); 
	bzero(&stats.reward.compute_reward, sizeof(stats.reward.compute_reward));
	stats.reward.prefetch_queue_hit = 0;
	stats.reward.no_pref = 0;
	stats.reward.prefetch_queue_unhit = 0;
	for(uint32_t idx = 0; idx < MAX_REWARDS; idx++)
		stats.reward.dist[idx].clear();

	bzero(&stats.train, sizeof(stats.train));

	bzero(&stats.register_fill, sizeof(stats.register_fill));

	bzero(&stats.register_prefetch_hit, sizeof(stats.register_prefetch_hit));

	bzero(&stats.pref_issue, sizeof(stats.pref_issue));

	bzero(&stats.bandwidth, sizeof(stats.bandwidth));

	bzero(&stats.ipc, sizeof(stats.ipc));

	bzero(&stats.cache_acc, sizeof(stats.cache_acc));

	state_action_dist.clear();
	state_action_dist2.clear();
	action_deg_dist.clear();

}

CBP::CBP(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	recorder = new CBPRecorder();

	last_evicted_tracker = NULL;

	/* init learning engine */
	brain = NULL;

	brain = new LearningEngineBasic_cbp( this,
								knob::cbp_alpha, 
								knob::cbp_gamma, 
								knob::cbp_max_states,
								knob::cbp_policy,
								knob::cbp_learning_type,
								knob::cbp_brain_zero_init,
								knob::cbp_early_exploration_window);
	

	bw_level = 0;
	core_ipc = 0;
}

CBP::~CBP()
{
	if(brain) 		delete brain;

	delete recorder;

	for(uint32_t index = 0; index < signature_table.size(); index++) 
		delete signature_table[index];

	for(uint32_t index = 0; index < prefetch_queue.size(); index++) 
		delete prefetch_queue[index];

	if(last_evicted_tracker) delete last_evicted_tracker;
}

void CBP::print_config()
{

	cout << "cbp_alpha " << knob::cbp_alpha << endl
	 	<< "cbp_gamma " << knob::cbp_gamma << endl
	 	<< "cbp_max_states " << knob::cbp_max_states << endl
	 	<< "cbp_policy " << knob::cbp_policy << endl
	 	<< "cbp_learning_type " << knob::cbp_learning_type << endl
		<< "cbp_early_exploration_window " << knob::cbp_early_exploration_window << endl
		<< "cbp_pt_size " << knob::cbp_pt_size << endl
		<< "cbp_st_size " << knob::cbp_st_size << endl
		<< "cbp_hst_size " << knob::cbp_hst_size << endl
		<< "cbp_action_tracker_size " << knob::cbp_action_tracker_size << endl
		<< "cbp_brain_zero_init " << knob::cbp_brain_zero_init << endl
		<< "cbp_enable_reward_all " << knob::cbp_enable_reward_all << endl
		<< "cbp_enable_hbw_reward " << knob::cbp_enable_hbw_reward << endl
		<< "cbp_reward_none " << knob::cbp_reward_none << endl
		<< "cbp_reward_hbw_none " << knob::cbp_reward_hbw_none << endl
		<< "cbp_reward_incorrect " << knob::cbp_reward_incorrect << endl
		<< "cbp_reward_hbw_incorrect " << knob::cbp_reward_hbw_incorrect << endl
		<< "cbp_state_type " << knob::cbp_state_type << endl
		<< "cbp_state_hash_type " << knob::cbp_state_hash_type << endl
		<< "cbp_pref_degree " << knob::cbp_pref_degree << endl
		<< "cbp_high_bw_thresh " << knob::cbp_high_bw_thresh << endl
		<< "cbp_max_pcs " << knob::cbp_max_pcs << endl
		<< "cbp_max_offsets " << knob::cbp_max_offsets << endl
		<< "cbp_max_deltas " << knob::cbp_max_deltas << endl
		<< "cbp_enable_state_action_stats " << knob::cbp_enable_state_action_stats << endl
		<< "cbp_enable_track_multiple " << knob::cbp_enable_track_multiple << endl
		<< "cbp_access_debug " << knob::cbp_access_debug << endl
		<< "cbp_print_access_debug " << knob::cbp_print_access_debug << endl
		<< "cbp_print_access_debug_pc " << knob::cbp_print_access_debug_pc << endl
		<< "cbp_print_access_debug_pc_count " << knob::cbp_print_access_debug_pc << endl
		<< "cbp_print_trace " << knob::cbp_print_trace << endl
		<< endl
		<< "cbp learning engine config" << endl
		<< "cbp_le_enable_trace " << knob::cbp_le_enable_trace << endl
		<< "cbp_le_trace_interval " << knob::cbp_le_trace_interval << endl
		<< "cbp_le_trace_file_name " << knob::cbp_le_trace_file_name << endl
		<< "cbp_le_trace_state " << knob::cbp_le_trace_state << endl
		<< "cbp_le_enable_score_plot " << knob::cbp_le_enable_score_plot << endl
		<< "cbp_le_plot_file_name " << knob::cbp_le_plot_file_name << endl
		<< "cbp_le_enable_action_trace " << knob::cbp_le_enable_action_trace << endl
		<< "cbp_le_action_trace_interval " << knob::cbp_le_action_trace_interval << endl
		<< "cbp_le_action_trace_name " << knob::cbp_le_action_trace_name << endl
		<< "cbp_le_enable_action_plot " << knob::cbp_le_enable_action_plot << endl
		<< endl;

}

void CBP::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint64_t cacheline = address >> LOG2_BLOCK_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	MYLOG("---------------------------------------------------------------------");
	MYLOG("%s %lx cacheline %lx pc %lx page %lx off %u", GetAccessType(type), address, cacheline, pc, page, offset);

	/* compute reward on demand */
	reward(address);

	/* record the access: just to gain some insights from the workload
	 * defined in cbp_helper.h(cc) */
	recorder->record_access(pc, address, page, offset, bw_level);

	/* global state tracking */
	update_global_state(pc, page, offset, address);
	/* per page state tracking */
	CBP_STEntry *stentry = update_local_state(pc, page, offset, address);

	/* Measure state.
	 * state can contain per page local information like delta signature, pc signature etc.
	 * it can also contain global signatures like last three branch PCs etc.
	 */
	State *state = new State();
	state->pc = pc;
	state->address = address;
	state->cacheline = cacheline;
	state->page = page;
	state->offset = offset;
	state->delta = !stentry->deltas.empty() ? stentry->deltas.back() : 0;
	state->local_delta_sig = stentry->get_delta_sig();
	state->local_delta_sig2 = stentry->get_delta_sig2();
	state->local_pc_sig = stentry->get_pc_sig();
	state->local_offset_sig = stentry->get_offset_sig();
	state->bw_level = bw_level;
	state->is_high_bw = is_high_bw();
	state->acc_level = acc_level;

	uint32_t count = pref_addr.size();
	predict(address, page, offset, state, pref_addr);
	stats.pref_issue.cbp += (pref_addr.size() - count);
}

void CBP::update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	/* @rbera TODO: implement */
}

CBP_STEntry* CBP::update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	stats.st.lookup++;
	CBP_STEntry *stentry = NULL;
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](CBP_STEntry *stentry){return stentry->page == page;});
	if(st_index != signature_table.end())
	{
		stats.st.hit++;
		stentry = (*st_index);
		stentry->update(page, pc, offset, address);
		signature_table.erase(st_index);
		signature_table.push_back(stentry);
		return stentry;
	}
	else
	{
		if(signature_table.size() >= knob::cbp_st_size)
		{
			stats.st.evict++;
			stentry = signature_table.front();
			signature_table.pop_front();
			if(knob::cbp_access_debug)
			{
				recorder->record_access_knowledge(stentry);
				if(knob::cbp_print_access_debug)
				{
					print_access_debug(stentry);
				}
			}
			delete stentry;
		}

		stats.st.insert++;
		stentry = new CBP_STEntry(page, pc, offset);
		recorder->record_trigger_access(page, pc, offset);
		signature_table.push_back(stentry);
		return stentry;
	}
}

uint32_t CBP::predict(uint64_t base_address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr)
{
	MYLOG("addr@%lx page %lx off %u state %x", base_address, page, offset, state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states));

	stats.predict.called++;

	/* query learning engine to get the next prediction */
	uint64_t action_index = 0;
	uint32_t pref_degree = knob::cbp_pref_degree;

	
	uint32_t state_index = state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states);
	assert(state_index < knob::cbp_max_states);
	action_index = brain->chooseAction(state_index);
	if(knob::cbp_enable_state_action_stats)
	{
		update_stats(state_index, action_index, pref_degree);
	}
	

	MYLOG("act_idx %lu act %lu", action_index, action_index);

	uint64_t addr = NO_PREFETCH_ADDR;
	CBP_PQEntry *pqentry = NULL;
	int32_t predicted_offset = action_index & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
	
	if(!no_prefetch_action(action_index))
	{

		addr = action_index << LOG2_BLOCK_SIZE;

		/* track prefetch */
		bool new_addr = track(addr, state, action_index, &pqentry);
		if(new_addr)
		{
			pref_addr.push_back(addr);
			track_in_st(page, predicted_offset, action_index);
			stats.predict.issue_dist[action_index]++;
			if(pref_degree > 1)
			{
				gen_multi_degree_pref(page, offset, action_index, pref_degree, pref_addr);
			}
			stats.predict.deg_histogram[pref_degree]++;
			//pqentry->consensus_vec = consensus_vec;
		}
		else
		{
			MYLOG("pred_off %d tracker_hit", predicted_offset);
			stats.predict.pred_hit[action_index]++;
			
			addr = NO_PREFETCH_ADDR;
			track(addr, state, action_index, &pqentry);
			assert(pqentry);
			assign_reward(pqentry, CBPRewardType::cbp_prefetch_queue_hit);
		}
		stats.predict.action_dist[action_index]++;
	}
	else
	{
		MYLOG("no prefecth");
		/* agent decided not to prefetch */
		addr = NO_PREFETCH_ADDR;
		/* track no prefetch */
		track(addr, state, action_index, &pqentry);
		stats.predict.action_dist[action_index]++;
		//pqentry->consensus_vec = consensus_vec;
	}

	stats.predict.predicted += pref_addr.size();
	MYLOG("end@%lx", base_address);

	return pref_addr.size();
}

/* Returns true if the address is not already present in prefetch_queue
 * false otherwise */
bool CBP::track(uint64_t address, State *state, uint64_t action_index, CBP_PQEntry **tracker)
{
	MYLOG("addr@%lx state %x act_idx %lu act %lu", address, state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), action_index, action_index);
	stats.track.called++;

	bool new_addr = true;
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> ptentries = search_pt(address, false);
	if(ptentries.empty())
	{
		new_addr = true;
	}
	else
	{
		new_addr = false;
	}

	if(!new_addr && !no_prefetch(address) && !knob::cbp_enable_track_multiple)
	{
		stats.track.same_address++;
		tracker = NULL;
		return new_addr;
	}

	/* new prefetched address that hasn't been seen before */
	CBP_PQEntry *pqentry = NULL;

	/*build history state -> cur addr action*/
	for(auto state_index : history_state_table){
		brain->insertAction(state_index, state->cacheline);
	}
	if(history_state_table.size() >= knob::cbp_hst_size){
		history_state_table.pop_front();
	}
	history_state_table.push_back(state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states));

	/*evict entry from prefetch queue*/
	if(prefetch_queue.size() >= knob::cbp_pt_size)
	{
		stats.track.evict++;
		pqentry = prefetch_queue.front();
		prefetch_queue.pop_front();
		MYLOG("victim_state %x victim_act_idx %lu victim_act %lu", pqentry->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), pqentry->action_index, pqentry->action_index);
		if(last_evicted_tracker)
		{
			MYLOG("last_victim_state %x last_victim_act_idx %lu last_victim_act %lu", last_evicted_tracker->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), last_evicted_tracker->action_index, last_evicted_tracker->action_index);
			/* train the agent */
			train(pqentry, last_evicted_tracker);
			delete last_evicted_tracker->state;
			delete last_evicted_tracker;
		}
		last_evicted_tracker = pqentry;
	}

	pqentry = new CBP_PQEntry(address, state, action_index);
	prefetch_queue.push_back(pqentry);
	assert(prefetch_queue.size() <= knob::cbp_pt_size);

	(*tracker) = pqentry;
	MYLOG("end@%lx", address);

	return new_addr;
}

void CBP::gen_multi_degree_pref(uint64_t page, uint32_t offset, uint64_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr)
{
	stats.predict.multi_deg_called++;
	uint64_t addr = NO_PREFETCH_ADDR;
	int32_t predicted_offset = 0;
	if(!no_prefetch_action(action))
	{
		for(uint32_t degree = 1; degree < pref_degree; ++degree)
		{
			predicted_offset = (int32_t)(action & ((1 << 6) - 1)) + degree;
			if(predicted_offset >=0 && predicted_offset < 64)
			{
				addr = (action + degree) << LOG2_BLOCK_SIZE;
				pref_addr.push_back(addr);
				MYLOG("degree %u pred_off %d pred_addr %lx", degree, predicted_offset, addr);
				stats.predict.multi_deg++;
				stats.predict.multi_deg_histogram[degree]++;
			}
		}
	}
}

/* This reward fucntion is called after seeing a demand access to the address */
/* TODO: what if multiple prefetch request generated the same address?
 * Currently, it just rewards the oldest prefetch request to the address
 * Should we reward all? */
void CBP::reward(uint64_t address)
{
	MYLOG("addr @ %lx", address);

	stats.reward.demand.called++;
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> ptentries = search_pt(address, knob::cbp_enable_reward_all);

	if(ptentries.empty())
	{
		MYLOG("PT miss");
		stats.reward.demand.pt_not_found++;
		return;
	}
	else
	{
		stats.reward.demand.pt_found++;
	}

	for(uint32_t index = 0; index < ptentries.size(); ++index)
	{
		uint32_t depth = ptentries[index].first;
		CBP_PQEntry *pqentry = ptentries[index].second;

		stats.reward.demand.pt_found_total++;

		MYLOG("PT hit. state %x act_idx %lu act %lu", pqentry->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), pqentry->action_index, pqentry->action_index);
		/* Do not compute reward if already has a reward.
		 * This can happen when a prefetch access sees multiple demand reuse */
		if(pqentry->has_reward)
		{
			MYLOG("entry already has reward: %d", pqentry->reward);
			stats.reward.demand.has_reward++;
			return;
		}


		assign_reward(pqentry, CBPRewardType::cbp_prefetch_queue_hit, depth);
		MYLOG("assigned reward prefetch queue hit(%d)", pqentry->reward);

		pqentry->has_reward = true;
	}
}

/* This reward function is called during eviction from prefetch_queue */
void CBP::reward(CBP_PQEntry *pqentry)
{
	MYLOG("reward PT evict %lx state %x act_idx %u act %d", pqentry->address, pqentry->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), pqentry->action_index, pqentry->action_index);

	stats.reward.train.called++;
	assert(!pqentry->has_reward);
	/* this is called during eviction from prefetch tracker
	 * that means, this address doesn't see a demand reuse.
	 * hence it either can be incorrect, or no prefetch */
	if(no_prefetch(pqentry->address)) /* no prefetch */
	{
		assign_reward(pqentry, CBPRewardType::cbp_none);
		MYLOG("assigned reward no_pref(%d)", pqentry->reward);
	}
	else /* incorrect prefetch */
	{
		assign_reward(pqentry, CBPRewardType::cbp_prefetch_queue_unhit);
		MYLOG("assigned reward incorrect(%d)", pqentry->reward);
	}
	pqentry->has_reward = true;
}

void CBP::assign_reward(CBP_PQEntry *pqentry, CBPRewardType type, uint32_t pq_depth)
{
	MYLOG("assign_reward PT evict %lx state %x act_idx %lu act %lu", pqentry->address, pqentry->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), pqentry->action_index, pqentry->action_index);
	assert(!pqentry->has_reward);

	/* compute the reward */
	float reward = compute_reward(pqentry, type, pq_depth);

	/* assign */
	pqentry->reward = reward;
	pqentry->reward_type = type;
	pqentry->has_reward = true;

	/* maintain stats */
	stats.reward.assign_reward.called++;
	switch(type)
	{
		case CBPRewardType::cbp_none: 				stats.reward.no_pref++; break;
		case CBPRewardType::cbp_prefetch_queue_hit: 		stats.reward.prefetch_queue_hit++; break;
		case CBPRewardType::cbp_prefetch_queue_unhit: 		stats.reward.prefetch_queue_unhit++; break;
		default:							assert(false);
	}
	stats.reward.dist[type][pqentry->action_index]++;
}

float CBP::compute_reward(CBP_PQEntry *pqentry, CBPRewardType type, uint32_t pq_depth)
{
	bool high_bw = (knob::cbp_enable_hbw_reward && is_high_bw()) ? true : false;
	float reward = 0;

	stats.reward.compute_reward.dist[type][high_bw]++;

	if(type == CBPRewardType::cbp_none)
	{
		reward = high_bw ? knob::cbp_reward_hbw_none : knob::cbp_reward_none;
	}
	else if(type == CBPRewardType::cbp_prefetch_queue_hit)
	{
		if(pq_depth < 5){
			reward = -1;
		}
		else if(pq_depth >= 5 && pq_depth < 35){
			reward = -1 + (pq_depth - 5) * (5.0 / 30);
		}
		else if(pq_depth >= 35 && pq_depth < 70){
			reward = 4 + (pq_depth - 35) * (-7.0 / 35); 
		}
		else if(pq_depth >= 70){
			reward = -3 + (pq_depth - 70) * (-1.0 / 50);
		}

		reward = high_bw ? reward - 0.2 : reward;  
	}
	else if(type == CBPRewardType::cbp_prefetch_queue_unhit){
		reward = high_bw ? knob::cbp_reward_hbw_incorrect : knob::cbp_reward_incorrect;
	}
	else
	{
		cout << "Invalid reward type found " << type << endl;
		assert(false);
	}

	return reward;
}

void CBP::train(CBP_PQEntry *curr_evicted, CBP_PQEntry *last_evicted)
{
	MYLOG("victim %s %lu %lu last_victim %s %lu %lu", curr_evicted->state->to_string().c_str(), curr_evicted->action_index, curr_evicted->action_index,
												last_evicted->state->to_string().c_str(), last_evicted->action_index, last_evicted->action_index);

	stats.train.called++;
	if(!last_evicted->has_reward)
	{
		stats.train.compute_reward++;
		reward(last_evicted);
	}
	assert(last_evicted->has_reward);

	/* train */
	MYLOG("===SARSA=== S1: %s A1: %lu R1: %d S2: %s A2: %lu", last_evicted->state->to_string().c_str(), last_evicted->action_index,
															last_evicted->reward,
															curr_evicted->state->to_string().c_str(), curr_evicted->action_index);
	brain->learn(last_evicted->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), last_evicted->action_index, last_evicted->reward, curr_evicted->state->value(knob::cbp_state_type, knob::cbp_state_hash_type, knob::cbp_max_states), curr_evicted->action_index);
	
	MYLOG("train done");
}

/* TODO: what if multiple prefetch request generated the same address?
 * Currently it just sets the fill bit of the oldest prefetch request.
 * Do we need to set it for everyone? */
void CBP::register_fill(uint64_t address)
{
	MYLOG("fill @ %lx", address);

	stats.register_fill.called++;
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> ptentries = search_pt(address, knob::cbp_enable_reward_all);
	if(!ptentries.empty())
	{
		stats.register_fill.set++;
		for(uint32_t index = 0; index < ptentries.size(); ++index)
		{
			stats.register_fill.set_total++;
			ptentries[index].second->is_filled = true;
			MYLOG("fill PT hit. pref with act_idx %lu act %lu", ptentries[index].second->action_index, ptentries[index].second->action_index);
		}
	}
}

void CBP::register_prefetch_hit(uint64_t address)
{
	MYLOG("pref_hit @ %lx", address);

	stats.register_prefetch_hit.called++;
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> ptentries = search_pt(address, knob::cbp_enable_reward_all);
	if(!ptentries.empty())
	{
		stats.register_prefetch_hit.set++;
		for(uint32_t index = 0; index < ptentries.size(); ++index)
		{
			stats.register_prefetch_hit.set_total++;
			ptentries[index].second->pf_cache_hit = true;
			MYLOG("pref_hit PT hit. pref with act_idx %lu act %lu", ptentries[index].second->action_index, ptentries[index].second->action_index);
		}
	}
}

std::vector<std::pair<uint32_t, CBP_PQEntry*>> CBP::search_pt(uint64_t address, bool search_all)
{
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> entries;
	for(uint32_t index = 0; index < prefetch_queue.size(); ++index)
	{
		if(prefetch_queue[index]->address == address)
		{
			entries.push_back({index, prefetch_queue[index]});
			if(!search_all) break;
		}
	}
	return entries;
}

void CBP::update_stats(uint32_t state, uint64_t action_index, uint32_t pref_degree)
{
	
	state_action_dist[state][action_index]++;

	auto it2 = action_deg_dist.find(getAction(action_index));
	if(it2 != action_deg_dist.end())
	{
		it2->second[pref_degree]++;
	}
	else
	{
		action_deg_dist[action_index].resize(MAX_CBP_DEGREE, 0);
		action_deg_dist[action_index][pref_degree]++;
	}
	
}

void CBP::update_stats(State *state, uint64_t action_index, uint32_t degree)
{
	string state_str = state->to_string();
	auto it = state_action_dist2.find(state_str);
	if(it != state_action_dist2.end())
	{
		it->second[action_index]++;
		//it->second[knob::cbp_max_actions]++; /* counts total occurences of this state */
	}
	else
	{
		state_action_dist2[state_str][action_index]++;
		// vector<uint64_t> act_dist;
		// act_dist.resize(knob::cbp_max_actions+1, 0);
		// act_dist[action_index]++;
		// act_dist[knob::cbp_max_actions]++; /* counts total occurences of this state */
		// state_action_dist2.insert(std::pair<string, vector<uint64_t> >(state_str, act_dist));
	}

	auto it2 = action_deg_dist.find(getAction(action_index));
	if(it2 != action_deg_dist.end())
	{
		it2->second[degree]++;
	}
	else
	{
		//action_deg_dist[action_index][degree]++;
		 //vector<uint64_t> deg_dist;
		 action_deg_dist[action_index].resize(MAX_CBP_DEGREE, 0);
		 //deg_dist[degree]++;
		 action_deg_dist[action_index][degree]++;
		// action_deg_dist.insert(std::pair<int32_t, vector<uint64_t> >(getAction(action_index), deg_dist));
	}
}

uint64_t CBP::getAction(uint64_t action_index)
{
	//assert(action_index < Actions.size());
	return action_index;
}

void CBP::track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset)
{
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](CBP_STEntry *stentry){return stentry->page == page;});
	if(st_index != signature_table.end())
	{
		(*st_index)->track_prefetch(pred_offset, pref_offset);
	}
}

void CBP::update_bw(uint8_t bw)
{
	assert(bw < DRAM_BW_LEVELS);
	bw_level = bw;
	stats.bandwidth.epochs++;
	stats.bandwidth.histogram[bw_level]++;
}

void CBP::update_ipc(uint8_t ipc)
{
	assert(ipc < CBP_MAX_IPC_LEVEL);
	core_ipc = ipc;
	stats.ipc.epochs++;
	stats.ipc.histogram[ipc]++;
}

void CBP::update_acc(uint32_t acc)
{
	assert(acc < CACHE_ACC_LEVELS);
	acc_level = acc;
	stats.cache_acc.epochs++;
	stats.cache_acc.histogram[acc]++;
}

bool CBP::is_high_bw()
{
	return bw_level >= knob::cbp_high_bw_thresh ? true : false;
}

void CBP::dump_stats()
{
	cout << "cbp_st_lookup " << stats.st.lookup << endl
		<< "cbp_st_hit " << stats.st.hit << endl
		<< "cbp_st_evict " << stats.st.evict << endl
		<< "cbp_st_insert " << stats.st.insert << endl
		<< "cbp_st_streaming " << stats.st.streaming << endl
		<< endl

		<< "cbp_predict_called " << stats.predict.called << endl
		// << "cbp_predict_shaggy_called " << stats.predict.shaggy_called << endl
		<< "cbp_predict_out_of_bounds " << stats.predict.out_of_bounds << endl;

	// for(uint32_t index = 0; index < Actions.size(); ++index)
	// {
	// 	cout << "cbp_predict_action_" << Actions[index] << " " << stats.predict.action_dist[index] << endl;
	// 	cout << "cbp_predict_issue_action_" << Actions[index] << " " << stats.predict.issue_dist[index] << endl;
	// 	cout << "cbp_predict_hit_action_" << Actions[index] << " " << stats.predict.pred_hit[index] << endl;
	// 	cout << "cbp_predict_out_of_bounds_action_" << Actions[index] << " " << stats.predict.out_of_bounds_dist[index] << endl;
	// }

	cout << "cbp_predict_multi_deg_called " << stats.predict.multi_deg_called << endl
		<< "cbp_predict_predicted " << stats.predict.predicted << endl
		<< "cbp_predict_multi_deg " << stats.predict.multi_deg << endl;
	for(uint32_t index = 2; index <= MAX_CBP_DEGREE; ++index)
	{
		cout << "cbp_predict_multi_deg_" << index << " " << stats.predict.multi_deg_histogram[index] << endl;
	}
	cout << endl;
	for(uint32_t index = 1; index <= MAX_CBP_DEGREE; ++index)
	{
		cout << "cbp_selected_deg_" << index << " " << stats.predict.deg_histogram[index] << endl;
	}
	cout << endl;

	if(knob::cbp_enable_state_action_stats)
	{

		//print top-K state-action pair
		cout << "print top-K state-action pair" << endl;
		priority_queue<pair<uint32_t, uint32_t>, vector<pair<uint32_t, uint32_t>>, 
		greater<pair<uint32_t, uint32_t>>> topk_state_action_q;

		for(auto [state, action_dist] : state_action_dist){
			uint32_t st = state;
			uint32_t cnt = 0;
			for(auto [action, action_cnt] : action_dist){
				cnt += action_cnt;
			}

			topk_state_action_q.emplace(cnt, st);
			if(topk_state_action_q.size() > knob::cbp_state_action_stats_topK)
				topk_state_action_q.pop();
		}

		while(!topk_state_action_q.empty()){
			uint32_t cnt = topk_state_action_q.top().first;
			uint32_t state = topk_state_action_q.top().second;
			topk_state_action_q.pop();

			priority_queue<pair<uint32_t, uint64_t>, vector<pair<uint32_t, uint64_t>>, 
			greater<pair<uint32_t, uint64_t>>> topk_action_q;

			for(auto [action, action_cnt] : state_action_dist[state]){
				topk_action_q.emplace(action_cnt, action);
				if(topk_action_q.size() > knob::cbp_state_action_stats_topK)
					topk_action_q.pop();
			}

			cout << "cbp_state_" << hex << state << dec << " " << cnt << ", top-K action: ";
			while(!topk_action_q.empty()){
				uint32_t action_cnt = topk_action_q.top().first;
				uint64_t action = topk_action_q.top().second;
				cout << hex << action << dec << ":" << action_cnt << ",";
			}
			cout  << endl;
		}
	}
	cout << endl;

	for(auto it = action_deg_dist.begin(); it != action_deg_dist.end(); ++it)
	{
		cout << "cbp_action_" << it->first << "_deg_dist ";
		for(uint32_t index = 0; index < MAX_CBP_DEGREE; ++index)
		{
			cout << it->second[index] << ",";
		}
		cout << endl;
	}
	cout << endl;

	cout << "cbp_track_called " << stats.track.called << endl
		<< "cbp_track_same_address " << stats.track.same_address << endl
		<< "cbp_track_evict " << stats.track.evict << endl
		<< endl

		<< "cbp_reward_demand_called " << stats.reward.demand.called << endl
		<< "cbp_reward_demand_pt_not_found " << stats.reward.demand.pt_not_found << endl
		<< "cbp_reward_demand_pt_found " << stats.reward.demand.pt_found << endl
		<< "cbp_reward_demand_pt_found_total " << stats.reward.demand.pt_found_total << endl
		<< "cbp_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
		<< "cbp_reward_train_called " << stats.reward.train.called << endl
		<< "cbp_reward_assign_reward_called " << stats.reward.assign_reward.called << endl
		<< "cbp_reward_no_pref " << stats.reward.no_pref << endl
		<< "cbp_reward_prefetch_queue_hit " << stats.reward.prefetch_queue_hit << endl
		<< "cbp_reward_prefetch_queue_unhit " << stats.reward.prefetch_queue_unhit << endl
		<< endl;

	for(uint32_t reward = 0; reward < CBPRewardType::cbp_num_rewards; ++reward)
	{
		cout << "cbp_reward_" << CBPgetRewardTypeString((CBPRewardType)reward) << "_low_bw " << stats.reward.compute_reward.dist[reward][0] << endl
			<< "cbp_reward_" << CBPgetRewardTypeString((CBPRewardType)reward) << "_high_bw " << stats.reward.compute_reward.dist[reward][1] << endl;
	}
	cout << endl;

	for(uint32_t reward = 0; reward < CBPRewardType::cbp_num_rewards; ++reward){
		uint64_t cnt = 0;

		for(auto [action, action_cnt] : stats.reward.dist[reward]){
			cnt += action_cnt;
		}
		
		cout << "cbp_reward_" << reward << " " << cnt << endl;
	}

	cout << endl
		<< "cbp_train_called " << stats.train.called << endl
		<< "cbp_train_compute_reward " << stats.train.compute_reward << endl
		<< endl

		<< "cbp_register_fill_called " << stats.register_fill.called << endl
		<< "cbp_register_fill_set " << stats.register_fill.set << endl
		<< "cbp_register_fill_set_total " << stats.register_fill.set_total << endl
		<< endl

		<< "cbp_register_prefetch_hit_called " << stats.register_prefetch_hit.called << endl
		<< "cbp_register_prefetch_hit_set " << stats.register_prefetch_hit.set << endl
		<< "cbp_register_prefetch_hit_set_total " << stats.register_prefetch_hit.set_total << endl
		<< endl

		<< "cbp_pref_issue_cbp " << stats.pref_issue.cbp << endl
		// << "cbp_pref_issue_shaggy " << stats.pref_issue.shaggy << endl
		<< endl;


	brain->dump_stats();
	
	recorder->dump_stats();

	cout << "cbp_bw_epochs " << stats.bandwidth.epochs << endl;
	for(uint32_t index = 0; index < DRAM_BW_LEVELS; ++index)
	{
		cout << "cbp_bw_level_" << index << " " << stats.bandwidth.histogram[index] << endl;
	}
	cout << endl;

	cout << "cbp_ipc_epochs " << stats.ipc.epochs << endl;
	for(uint32_t index = 0; index < CBP_MAX_IPC_LEVEL; ++index)
	{
		cout << "cbp_ipc_level_" << index << " " << stats.ipc.histogram[index] << endl;
	}
	cout << endl;

	cout << "cbp_cache_acc_epochs " << stats.cache_acc.epochs << endl;
	for(uint32_t index = 0; index < CACHE_ACC_LEVELS; ++index)
	{
		cout << "cbp_cache_acc_level_" << index << " " << stats.cache_acc.histogram[index] << endl;
	}
	cout << endl;
}
