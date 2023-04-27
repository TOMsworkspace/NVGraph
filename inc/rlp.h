#ifndef RLP_H
#define RLP_H

#include <random>
#include <vector>
#include <unordered_map>
#include "champsim.h"
#include "cache.h"
#include "prefetcher.h"
#include "rlp_helper.h"
#include "learning_engine_basic_rlp.h"
#include "learning_engine_featurewise_rlp.h"


using namespace std;

#define RLP_RP_MAX_ACTIONS 128
#define RLP_MAX_REWARDS 32
#define RLP_MAX_DEGREE 16
#define RLP_MAX_IPC_LEVEL 4

/* forward declaration */
class LearningEngineBasic_rlp;

class RLP : public Prefetcher
{
private:
	deque<RLP_STEntry*> signature_table;

	/*irregular access pattern learn engine*/
	LearningEngineBasic_rlp *ir_brain;

	/*regular access pattern learn engine*/
	LearningEngineFeaturewise_rlp *rp_brain;

	/*e-greedy select action between ir_bain and rp_brain*/
	float m_rlp_epsilon;

	uint32_t m_seed;
	std::default_random_engine generator;
    std::bernoulli_distribution *access_pattern_explore;

	deque<RLP_PQEntry*> prefetch_queue;

	deque<uint64_t> history_state_table;

	RLP_PQEntry *ir_last_evicted_tracker;
	RLP_PQEntry *rp_last_evicted_tracker;

	uint8_t bw_level;
	uint8_t core_ipc;
	uint32_t acc_level;

	/* for workload insights only
	 * has nothing to do with prefetching */
	RLPRecorder *recorder;

	/* Data structures for debugging */
	unordered_map<string, uint64_t> ir_target_action_state;
	unordered_map<string, uint64_t> rp_target_action_state;


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
			uint64_t ir_called;
			uint64_t rp_called;
			uint64_t rp_out_of_bounds;
			
			std::unordered_map<uint64_t, uint32_t> ir_action_dist;
			std::unordered_map<uint64_t, uint32_t> ir_issue_dist;
			std::unordered_map<uint64_t, uint32_t> ir_pred_hit;
			vector<uint64_t> rp_action_dist;
			vector<uint64_t> rp_issue_dist;
			vector<uint64_t> rp_pred_hit;
			vector<uint64_t> rp_out_of_bounds_dist;

			uint64_t predicted;
			uint64_t ir_predicted;
			uint64_t rp_predicted;
			uint64_t multi_deg;
			uint64_t ir_multi_deg;
			uint64_t rp_multi_deg;
			uint64_t ir_multi_deg_called;
			uint64_t rp_multi_deg_called;
			uint64_t ir_multi_deg_histogram[RLP_MAX_DEGREE + 1];
			uint64_t rp_multi_deg_histogram[RLP_MAX_DEGREE + 1];
			uint64_t ir_deg_histogram[RLP_MAX_DEGREE + 1];
			uint64_t rp_deg_histogram[RLP_MAX_DEGREE + 1];

		} predict;

		struct
		{
			uint64_t ir_called;
			uint64_t rp_called;
			uint64_t ir_same_address;
			uint64_t rp_same_address;
			uint64_t ir_evict;
			uint64_t rp_evict;
		} track;

		struct
		{
			struct
			{
				uint64_t called;
				uint64_t pq_not_found;
				uint64_t pq_found;
				uint64_t pq_found_total;
				uint64_t has_reward;
			} demand;

			struct
			{
				uint64_t ir_called;
				uint64_t rp_called;
			} train;

			struct
			{
				uint64_t called;
				uint64_t ir_called;
				uint64_t rp_called;
			} assign_reward;

			struct
			{
				/**0：low bandwidth, 1: high bandwidth*/
				uint64_t dist[RLP_MAX_REWARDS][2];
			} compute_reward;
			
			
			uint64_t ir_no_pref;
			uint64_t ir_incorrect;
			uint64_t ir_correct_timely;
			uint64_t ir_correct_untimely;
			uint64_t ir_prefetch_queue_hit;
			uint64_t rp_no_pref;
			uint64_t rp_incorrect;
			uint64_t rp_correct_timely;
			uint64_t rp_correct_untimely;
			uint64_t rp_prefetch_queue_hit;
			uint64_t rp_out_of_bounds;

			std::unordered_map<uint64_t, uint32_t> ir_dist[RLP_MAX_REWARDS];
			uint64_t rp_dist[RLP_RP_MAX_ACTIONS][RLP_MAX_REWARDS];

		} reward;

		struct
		{	
			uint64_t ir_called;
			uint64_t rp_called;
			uint64_t ir_compute_reward;
			uint64_t rp_compute_reward;
		} train;

		struct
		{
			uint64_t called;
			uint64_t set;
			uint64_t ir_set_total;
			uint64_t rp_set_total;
			uint64_t set_total;

		} register_fill;

		struct
		{
			uint64_t called;
			uint64_t set;
			uint64_t ir_set_total;
			uint64_t rp_set_total;
			uint64_t set_total;
		} register_prefetch_hit;

		struct
		{
			uint64_t rlp;
			uint64_t rlp_ir;
			uint64_t rlp_rp;
		} pref_issue;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[DRAM_BW_LEVELS];
		} bandwidth;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[RLP_MAX_IPC_LEVEL];
		} ipc;

		struct 
		{
			uint64_t epochs;
			uint64_t histogram[CACHE_ACC_LEVELS];
		} cache_acc;
	} stats;

	unordered_map<uint32_t, unordered_map<uint64_t, uint64_t> > ir_state_action_dist;
	unordered_map<std::string, unordered_map<uint64_t, uint64_t> > ir_state_action_dist2;
	unordered_map<uint64_t, vector<uint64_t> > ir_action_deg_dist;
	unordered_map<uint32_t, vector<uint64_t> > rp_state_action_dist;
	unordered_map<std::string, vector<uint64_t> > rp_state_action_dist2;
	unordered_map<int32_t, vector<uint64_t> > rp_action_deg_dist;

private:
	void init_knobs();
	void init_stats();

	void update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	RLP_STEntry* update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	uint32_t predict(uint64_t address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr);
	void ir_predict(uint64_t address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr, bool prefetch = false);
	void rp_predict(uint64_t address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr, bool prefetch = false);
	bool ir_track(uint64_t address, State *state, uint64_t action_index, RLP_PQEntry **tracker, bool prefetch = false);
	bool rp_track(uint64_t address, State *state, uint32_t action_index, RLP_PQEntry **tracker, bool prefetch = false);
	void reward(uint64_t address);
	void reward(RLP_PQEntry *pqentry);
	void assign_reward(RLP_PQEntry *pqentry, RLPRewardType type);
	int32_t compute_reward(RLP_PQEntry *pqentry, RLPRewardType type);
	void ir_train(RLP_PQEntry *ir_curr_evicted, RLP_PQEntry *ir_last_evicted);
	void rp_train(RLP_PQEntry *rp_curr_evicted, RLP_PQEntry *rp_last_evicted);
	std::vector<RLP_PQEntry*> search_pq(uint64_t address, bool search_all = false);
	std::vector<RLP_PQEntry*> ir_search_pq(uint64_t address, bool search_all = false);
	std::vector<RLP_PQEntry*> rp_search_pq(uint64_t address, bool search_all = false);
	void ir_update_stats(uint32_t state, uint64_t action_index, uint32_t pref_degree = 1);
	void ir_update_stats(State *state, uint64_t action_index, uint32_t degree = 1);
	void rp_update_stats(uint32_t state, uint32_t action_index, uint32_t pref_degree = 1);
	void rp_update_stats(State *state, uint32_t action_index, uint32_t degree = 1);
	void track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset);
	void track_in_hsq(State * state);
	void ir_gen_multi_degree_pref(uint64_t page, uint32_t offset, uint64_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr);
	void rp_gen_multi_degree_pref(uint64_t page, uint32_t offset, int32_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr);
	uint32_t rp_get_dyn_pref_degree(float max_to_avg_q_ratio, uint64_t page = 0xdeadbeef, int32_t action = 0);
	bool is_high_bw();

public:
	RLP(string type);
	~RLP();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
	void register_fill(uint64_t address);
	void register_prefetch_hit(uint64_t address);
	void dump_stats();
	void print_config();
	uint64_t getIrAction(uint64_t action_index);
	int32_t getRpAction(uint32_t action_index);
	void update_bw(uint8_t bw_level);
	void update_ipc(uint8_t ipc);
	void update_acc(uint32_t acc_level);
};

#endif /* RLP_H */

