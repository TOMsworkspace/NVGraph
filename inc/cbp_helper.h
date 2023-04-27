#ifndef CBP_HELPER_H
#define CBP_HELPER_H

#include <string>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "bitmap.h"
#include "rl_helper.h"

using namespace std;

#define MAX_HOP_COUNT 16

typedef enum
{
	cbp_none = 0,
	cbp_prefetch_queue_hit,
	cbp_prefetch_queue_unhit,
	cbp_num_rewards
} CBPRewardType;

const char* CBPgetRewardTypeString(CBPRewardType type);

// inline bool isRewardCorrect(RewardType type) { return (type == correct_timely || type == correct_untimely); }
// inline bool isRewardIncorrect(RewardType type) { return type == incorrect; }

class CBPActionTracker
{
public:
	int64_t action;
	int32_t conf;
	CBPActionTracker(int64_t act, int32_t c) : action(act), conf(c) {}
	~CBPActionTracker() {}
};

class CBP_STEntry
{
public:
	uint64_t page;
	deque<uint64_t> pcs;
	deque<uint32_t> offsets;
	deque<int32_t> deltas;
	Bitmap bmp_real;
	Bitmap bmp_pred;
	unordered_set <uint64_t> unique_pcs;
	unordered_set <int32_t> unique_deltas;
	uint64_t trigger_pc;
	uint32_t trigger_offset;
	bool streaming;

	/* tracks last n actions on a page to determine degree */
	deque<CBPActionTracker*> action_tracker;
	unordered_set<int32_t> action_with_max_degree;
	unordered_set<int32_t> afterburning_actions;

	uint32_t total_prefetches;

public:
	CBP_STEntry(uint64_t p, uint64_t pc, uint32_t offset) : page(p)
	{
		pcs.clear();
		offsets.clear();
		deltas.clear();
		bmp_real.reset();
		unique_pcs.clear();
		unique_deltas.clear();
		trigger_pc = pc;
		trigger_offset = offset;
		streaming = false;

		pcs.push_back(pc);
		offsets.push_back(offset);
		unique_pcs.insert(pc);
		bmp_real[offset] = 1;
	}
	~CBP_STEntry(){}
	uint32_t get_delta_sig();
	uint32_t get_delta_sig2();
	uint32_t get_pc_sig();
	uint32_t get_offset_sig();
	void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
	void track_prefetch(uint32_t offset, int32_t pref_offset);
	void insert_action_tracker(int32_t pref_offset);
	bool search_action_tracker(uint64_t action, int32_t &conf);
};

class CBP_PQEntry
{
public:
	uint64_t address;
	State *state;
	uint64_t action_index;
	/* set when prefetched line is filled into cache 
	 * check during reward to measure timeliness */
	bool is_filled;
	/* set when prefetched line is alredy found in cache
	 * donotes extreme untimely prefetch */
	bool pf_cache_hit;
	float reward;
	CBPRewardType reward_type;
	bool has_reward;
	vector<bool> consensus_vec; // only used in featurewise engine
	
	CBP_PQEntry(uint64_t ad, State *st, uint64_t ac) : address(ad), state(st), action_index(ac)
	{
		is_filled = false;
		pf_cache_hit = false;
		reward = 0.0;
		reward_type = CBPRewardType::cbp_none;
		has_reward = false;
	}
	~CBP_PQEntry(){}
};

/* some data structures to mine information from workloads */
class CBPRecorder
{
public:
	unordered_set<uint64_t> unique_pcs;
	unordered_set<uint64_t> unique_trigger_pcs;
	unordered_set<uint64_t> unique_pages;
	unordered_map<uint64_t, uint64_t> access_bitmap_dist;
	// vector<unordered_map<int32_t, uint64_t>> hop_delta_dist;
	uint64_t hop_delta_dist[MAX_HOP_COUNT+1][127];
	uint64_t total_bitmaps_seen;
	uint64_t unique_bitmaps_seen;
	unordered_map<uint64_t, vector<uint64_t> > pc_bw_dist;

	CBPRecorder(){}
	~CBPRecorder(){}	
	void record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset, uint8_t bw_level);
	void record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset);
	void record_access_knowledge(CBP_STEntry *stentry);
	void dump_stats();
};

/* auxiliary functions */
void print_access_debug(CBP_STEntry *stentry);

#endif /* CBP_HELPER_H */

