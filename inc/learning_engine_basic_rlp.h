#ifndef LEARNING_ENGINE_BASIC_RLP_H
#define LEARNING_ENGINE_BASIC_RLP_H


#include <string.h>
#include <map>
#include <unordered_map>
#include "prefetcher.h"
#include "learning_engine_base.h"
#include "rl_helper.h"

#define MAX_ACTION_WAYS 1024

/*
 * table format
 *      |action 0| action 1| action 2|...| action n
state 0 |
state 1 |
		|	  ____         _        _     _      
		|	 / __ \       | |      | |   | |     
		|	| |  | |______| |_ __ _| |__ | | ___ 
		|	| |  | |______| __/ _` | '_ \| |/ _ \
		|	| |__| |      | || (_| | |_) | |  __/
		|	 \___\_\       \__\__,_|_.__/|_|\___|
		|                             
state m |
*/

/**
 * @brief rlp prefetcher irregular learning engine qtable entry
 * 
 */

static uint64_t cur_lru_cnt = 0;
class rlp_ir_qtable_entry{
public:
	uint64_t action;
	float qvalue;
	bool valid;
	uint64_t lru;

public:
	rlp_ir_qtable_entry(uint64_t action = NO_PREFETCH_ACTION, float qvalue = 0.0, bool valid = false)
		:action(action),qvalue(qvalue),valid(valid){
		lru = cur_lru_cnt;
	}
	~rlp_ir_qtable_entry(){}

	void update_lru(){
		lru = cur_lru_cnt++;
	}
};

class LearningEngineBasic_rlp : public LearningEngineBase
{
private:
	float init_value;

	class rlp_ir_qtable_entry** qtable;

	uint32_t m_qtable_ways;

	/* tracing related knobs */
	uint32_t trace_interval;
	uint64_t trace_timestamp;
	FILE *trace;
	uint32_t action_trace_interval;
	uint64_t action_trace_timestamp;
	FILE *action_trace;
	uint64_t m_action_counter;
	uint64_t m_early_exploration_window;

	struct
	{
		struct
		{
			uint64_t called;
			uint64_t explore;
			uint64_t exploit;
			uint64_t dist[MAX_ACTION_WAYS][2]; /* 0:explored, 1:exploited */
		} action;

		struct
		{
			uint64_t called;
		} learn;
	} stats;

	void init_stats();

	float consultQ(uint32_t state, uint64_t action);
	void updateQ(uint32_t state, uint64_t action, float value);
	std::string getStringQ(uint32_t state);
	uint64_t getMaxAction(uint32_t state);
	void print_aux_stats();
	void dump_state_trace(uint32_t state);
	void plot_scores();
	void dump_action_trace(uint64_t action);
	uint32_t get_qtable_way(uint64_t action);

public:
	LearningEngineBasic_rlp(Prefetcher *p,  float alpha, float gamma, uint32_t actions, uint32_t states, std::string policy, std::string learning_type, bool zero_init, uint64_t early_exploration_window);
	~LearningEngineBasic_rlp();

	uint64_t chooseAction(uint32_t state);
	bool insertAction(uint32_t state, uint64_t action);
	void learn(uint32_t state1, uint64_t action1, int32_t reward, uint32_t state2, uint64_t action2);
	void dump_stats();
};

#endif /* LEARNING_ENGINE_BASIC_RLP_H */

