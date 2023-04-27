#ifndef CBP_H
#define CBP_H

#include <vector>
#include <unordered_map>
#include "champsim.h"
#include "cache.h"
#include "prefetcher.h"
#include "cbp_helper.h"
#include "learning_engine_basic_cbp.h"


using namespace std;

#define MAX_REWARDS 16
#define MAX_CBP_DEGREE 16
#define CBP_MAX_IPC_LEVEL 4

/* forward declaration */
class LearningEngineBasic_cbp;

class CBP : public Prefetcher
{
private:
	deque<CBP_STEntry*> signature_table;
	LearningEngineBasic_cbp *brain;

	deque<CBP_PQEntry*> prefetch_queue;

	deque<uint64_t> history_state_table;

	CBP_PQEntry *last_evicted_tracker;
	uint8_t bw_level;
	uint8_t core_ipc;
	uint32_t acc_level;

	/* for workload insights only
	 * has nothing to do with prefetching */
	CBPRecorder *recorder;

	/* Data structures for debugging */
	unordered_map<string, uint64_t> target_action_state;

	struct
	{
		struct
		{
			uint64_t lookup;
			uint64_t hit;
			uint64_t evict;
			uint64_t insert;
			uint64_t streaming;
		} st;

		struct
		{
			uint64_t called;
			uint64_t out_of_bounds;
			std::unordered_map<uint64_t, uint32_t> action_dist;
			std::unordered_map<uint64_t, uint32_t> issue_dist;
			std::unordered_map<uint64_t, uint32_t> pred_hit;

			uint64_t predicted;
			uint64_t multi_deg;
			uint64_t multi_deg_called;
			uint64_t multi_deg_histogram[MAX_CBP_DEGREE+1];
			uint64_t deg_histogram[MAX_CBP_DEGREE+1];
		} predict;

		struct
		{
			uint64_t called;
			uint64_t same_address;
			uint64_t evict;
		} track;

		struct
		{
			struct
			{
				uint64_t called;
				uint64_t pt_not_found;
				uint64_t pt_found;
				uint64_t pt_found_total;
				uint64_t has_reward;
			} demand;

			struct
			{
				uint64_t called;
			} train;

			struct
			{
				uint64_t called;
			} assign_reward;

			struct
			{
				uint64_t dist[MAX_REWARDS][2];
			} compute_reward;
			
			uint64_t prefetch_queue_hit;
			uint64_t no_pref;
			uint64_t prefetch_queue_unhit;

			std::unordered_map<uint64_t, uint32_t> dist[MAX_REWARDS];

		} reward;

		struct
		{
			uint64_t called;
			uint64_t compute_reward;
		} train;

		struct
		{
			uint64_t called;
			uint64_t set;
			uint64_t set_total;
		} register_fill;

		struct
		{
			uint64_t called;
			uint64_t set;
			uint64_t set_total;
		} register_prefetch_hit;

		struct
		{
			uint64_t cbp;
		} pref_issue;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[DRAM_BW_LEVELS];
		} bandwidth;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[CBP_MAX_IPC_LEVEL];
		} ipc;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[CACHE_ACC_LEVELS];
		} cache_acc;
	} stats;

	unordered_map<uint32_t, unordered_map<uint64_t, uint32_t> > state_action_dist;
	unordered_map<std::string, unordered_map<uint64_t, uint32_t> > state_action_dist2;
	unordered_map<uint64_t, vector<uint32_t> > action_deg_dist;

private:
	void init_knobs();
	void init_stats();

	void update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	CBP_STEntry* update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	uint32_t predict(uint64_t address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr);
	bool track(uint64_t address, State *state, uint64_t action_index, CBP_PQEntry **tracker);
	void reward(uint64_t address);
	void reward(CBP_PQEntry *pqentry);
	void assign_reward(CBP_PQEntry *pqentry, CBPRewardType type, uint32_t pq_depth = 0);
	float compute_reward(CBP_PQEntry *pqentry, CBPRewardType type, uint32_t pq_depth = 0);
	void train(CBP_PQEntry *curr_evicted, CBP_PQEntry *last_evicted);
	std::vector<std::pair<uint32_t, CBP_PQEntry*>> search_pt(uint64_t address, bool search_all = false);
	void update_stats(uint32_t state, uint64_t action_index, uint32_t pref_degree = 1);
	void update_stats(State *state, uint64_t action_index, uint32_t degree = 1);
	void track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset);
	void gen_multi_degree_pref(uint64_t page, uint32_t offset, uint64_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr);
	bool is_high_bw();

public:
	CBP(string type);
	~CBP();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
	void register_fill(uint64_t address);
	void register_prefetch_hit(uint64_t address);
	void dump_stats();
	void print_config();
	uint64_t getAction(uint64_t action_index);
	void update_bw(uint8_t bw_level);
	void update_ipc(uint8_t ipc);
	void update_acc(uint32_t acc_level);
};

#endif /* CBP_H */

