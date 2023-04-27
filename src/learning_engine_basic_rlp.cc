#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <sstream>
#include "learning_engine_basic_rlp.h"
#include "rlp.h"
// #include "velma.h"
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
	extern uint32_t rlp_le_ir_way;
	extern bool     rlp_le_ir_enable_trace;
	extern uint32_t rlp_le_ir_trace_interval;
	extern std::string   rlp_le_ir_trace_file_name;
	extern uint32_t rlp_le_ir_trace_state;
	extern bool     rlp_le_ir_enable_score_plot;
	//extern std::vector<int32_t> rlp_le_ir_plot_actions;
	extern std::string   rlp_le_ir_plot_file_name;
	extern bool     rlp_le_ir_enable_action_trace;
	extern uint32_t rlp_le_ir_action_trace_interval;
	extern std::string rlp_le_ir_action_trace_name;
	extern bool     rlp_le_ir_enable_action_plot;
}

LearningEngineBasic_rlp::LearningEngineBasic_rlp(Prefetcher *parent, float alpha, float gamma, uint32_t actions, uint32_t states, std::string policy, std::string learning_type, bool zero_init, uint64_t early_exploration_window)
	: LearningEngineBase(parent, alpha, gamma, 0, actions, states, 0, policy, learning_type)
{
	qtable = (rlp_ir_qtable_entry**)calloc(m_states, sizeof(rlp_ir_qtable_entry*));
	assert(qtable);
	for(uint32_t index = 0; index < m_states; ++index)
	{
		qtable[index] = (rlp_ir_qtable_entry*)calloc(m_actions, sizeof(rlp_ir_qtable_entry));
		assert(qtable);
	}

	assert(m_actions % knob::rlp_le_ir_way == 0 );
	m_qtable_ways = m_actions / knob::rlp_le_ir_way;
	assert(m_qtable_ways + 1 <= MAX_ACTION_WAYS);

	/* init Q-table */
	if(zero_init)
	{
		init_value = 0;
	}
	else
	{

		if(abs(gamma - 1.0) < 1e-7) 
			init_value = 0.0;
		else
			init_value = 1.0 / (1.0 - gamma);
	}

	for(uint32_t row = 0; row < m_states; ++row)
	{
		for(uint32_t col = 0; col < m_actions; ++col)
		{
			qtable[row][col].qvalue = init_value;
		}
	}

	if(knob::rlp_le_ir_enable_trace)
	{
		trace_interval = 0;
		trace_timestamp = 0;
		trace = fopen(knob::rlp_le_ir_trace_file_name.c_str(), "w");
		assert(trace);
	}

	if(knob::rlp_le_ir_enable_action_trace)
	{
		action_trace_interval = 0;
		action_trace_timestamp = 0;
		action_trace = fopen(knob::rlp_le_ir_action_trace_name.c_str(), "w");
		assert(action_trace);
	}

	m_early_exploration_window = early_exploration_window;
	m_action_counter = 0;

	init_stats();
}

LearningEngineBasic_rlp::~LearningEngineBasic_rlp()
{
	
	for(uint32_t row = 0; row < m_states; ++row)
	{
		free(qtable[row]);
	}
	free(qtable);
	
	if(knob::rlp_le_ir_enable_trace && trace)
	{
		fclose(trace);
	}

	if(knob::rlp_le_ir_enable_action_trace && action_trace){
		fclose(action_trace);
	}
}

uint64_t LearningEngineBasic_rlp::chooseAction(uint32_t state)
{
	stats.action.called++;
	assert(state < m_states);
	uint64_t action = 0;
	if(m_type == LearningType::SARSA && m_policy == Policy::Directly)
	{
		if(m_action_counter < m_early_exploration_window)
		{
			action = NO_PREFETCH_ACTION; // take no prefetch action
			uint32_t action_way = get_qtable_way(action);
			stats.action.explore++;
			stats.action.dist[action_way][0]++;
			MYLOG("action taken %lu explore, state %x, scores %s", action, state, getStringQ(state).c_str());
		}
		else
		{
			action = getMaxAction(state);
			uint32_t action_way = get_qtable_way(action);
			if(!no_prefetch_action(action)) action_way += 1;
			stats.action.exploit++;
			stats.action.dist[action_way][1]++;
			MYLOG("action taken %lu exploit, state %x, scores %s", action, state, getStringQ(state).c_str());
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

	m_action_counter++;


	if(knob::rlp_le_ir_enable_action_trace && action_trace_interval++ == knob::rlp_le_ir_action_trace_interval)
	{
		dump_action_trace(action);
		action_trace_interval = 0;
	}

	return action;
}

void LearningEngineBasic_rlp::learn(uint32_t state1, uint64_t action1, int32_t reward, uint32_t state2, uint64_t action2)
{
	stats.learn.called++;
	if(m_type == LearningType::SARSA && m_policy == Policy::Directly)
	{	
		float Qsa1, Qsa2, Qsa1_old;
		Qsa1 = consultQ(state1, action1);
		Qsa2 = consultQ(state2, action2);
		Qsa1_old = Qsa1;
		/* SARSA */
		Qsa1 = Qsa1 + m_alpha * ((float)reward + m_gamma * Qsa2 - Qsa1);
		updateQ(state1, action1, Qsa1);
		MYLOG("Q(%x,%lu) = %.2f, R = %d, Q(%x,%lu) = %.2f, Q(%x,%lu) = %.2f", state1, action1, Qsa1_old, reward, state2, action2, Qsa2, state1, action1, Qsa1);
		MYLOG("state %x, scores %s", state1, getStringQ(state1).c_str());

		if(knob::rlp_le_ir_enable_trace && state1 == knob::rlp_le_ir_trace_state && trace_interval++ == knob::rlp_le_ir_trace_interval)
		{
			dump_state_trace(state1);
			trace_interval = 0;
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

float LearningEngineBasic_rlp::consultQ(uint32_t state, uint64_t action)
{
	assert(state < m_states );

	float value = init_value;

	uint32_t way = get_qtable_way(action);
	way *= knob::rlp_le_ir_way;

	for(uint32_t idx = 0; idx < knob::rlp_le_ir_way; idx++){
		if(qtable[state][way + idx].valid && qtable[state][way + idx].action == action){
			value = qtable[state][way + idx].qvalue;
			break;
		}
	}

	return value;
}

void LearningEngineBasic_rlp::updateQ(uint32_t state, uint64_t action, float value)
{
	assert(state < m_states);

	if(no_prefetch_action(action)) 
		return;

	uint32_t way = get_qtable_way(action);
	way *= knob::rlp_le_ir_way;

	for(uint32_t idx = 0; idx < knob::rlp_le_ir_way; idx++){
		if(qtable[state][way + idx].valid && qtable[state][way + idx].action == action){
			qtable[state][way + idx].qvalue = value;
			qtable[state][way + idx].update_lru();
			break;
		}
	}	
}

uint64_t LearningEngineBasic_rlp::getMaxAction(uint32_t state)
{
	assert(state < m_states);
	float maxScore = qtable[state][0].qvalue;
	uint64_t action = 0;
	for(uint32_t index = 1; index < m_actions; ++index)
	{
		if(qtable[state][index].qvalue && qtable[state][index].qvalue > maxScore)
		{
			maxScore = qtable[state][index].qvalue;
			action = qtable[state][index].action;
		}
	}
	return action;
}

bool LearningEngineBasic_rlp::insertAction(uint32_t state, uint64_t action){

	assert(state < m_states);

	if(no_prefetch_action(action)) 
		return false;

	uint32_t way = get_qtable_way(action);
	way *= knob::rlp_le_ir_way;

	uint32_t victim_idx = knob::rlp_le_ir_way;
	uint64_t victim_lru_cnt = cur_lru_cnt;

	for(uint32_t idx = 0; idx < knob::rlp_le_ir_way; idx++){
		if(!qtable[state][way + idx].valid)
		{
			victim_idx = idx;
			break;
		}

		if(qtable[state][way + idx].action == action) 
			return false;
	}

	if(victim_idx == knob::rlp_le_ir_way){
		for(uint32_t idx = 0; idx < knob::rlp_le_ir_way; idx++){
		 	if(qtable[state][way + idx].valid && qtable[state][way + idx].lru < victim_lru_cnt){
				victim_idx = idx;
				victim_lru_cnt = qtable[state][way + idx].lru;
			}
		}
	}

	qtable[state][way + victim_idx].action = action;
	qtable[state][way + victim_idx].qvalue = init_value;
	qtable[state][way + victim_idx].valid = true;
	qtable[state][way + victim_idx].update_lru();
	
	return true;
}

std::string LearningEngineBasic_rlp::getStringQ(uint32_t state)
{
	
	assert(state < m_states);
	std::stringstream ss;
	for(uint32_t index = 0; index < m_actions; ++index)
	{
		ss << qtable[state][index].qvalue << ",";
	}
	return ss.str();
}

void LearningEngineBasic_rlp::print_aux_stats()
{
	/* compute state-action table usage
	 * how mane state entries are actually used?
	 * heat-map dump, etc. etc. */
	uint32_t state_used = 0;
	for(uint32_t state = 0; state < m_states; ++state)
	{
		for(uint32_t action = 0; action < m_actions; ++action)
		{
			if(abs(qtable[state][action].qvalue - init_value) < 1e-7)
			{
				state_used++;
				break;
			}
		}
	}

	fprintf(stdout, "rlp.ir.learning_engine.state_used %u\n", state_used);
	fprintf(stdout, "\n");
}

void LearningEngineBasic_rlp::dump_stats()
{
	
	
	//RLP *rlp = (RLP*)m_parent;
	fprintf(stdout, "rlp.ir.learning_engine.action.called %lu\n", stats.action.called);
	fprintf(stdout, "rlp.ir.learning_engine.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "rlp.ir.learning_engine.action.exploit %lu\n", stats.action.exploit);

	for(uint32_t action_way = 0; action_way <= m_actions / knob::rlp_le_ir_way; ++action_way)
	{
		fprintf(stdout, "rlp.ir.learning_engine.action_way.index_%u_explored %lu\n", action_way, stats.action.dist[action_way][0]);
		fprintf(stdout, "rlp.ir.learning_engine.action_way.index_%u_exploited %lu\n", action_way, stats.action.dist[action_way][1]);
	}
	fprintf(stdout, "rlp.ir.learning_engine.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");

	print_aux_stats();

	if(knob::rlp_le_ir_enable_trace && knob::rlp_le_ir_enable_score_plot)
	{
		plot_scores();
	}
}

void LearningEngineBasic_rlp::dump_state_trace(uint32_t state)
{
	trace_timestamp++;
	fprintf(trace, "%lu,", trace_timestamp);
	for(uint32_t index = 0; index < m_actions; ++index)
	{
		fprintf(trace, "%lu, %.2f", qtable[state][index].action, qtable[state][index].qvalue);
	}

	fprintf(trace, "\n");
	fflush(trace);
}

void LearningEngineBasic_rlp::plot_scores()
{
	/*
	char *script_file = (char*)malloc(16*sizeof(char));
	assert(script_file);
	gen_random(script_file, 16);
	FILE *script = fopen(script_file, "w");
	assert(script);

	fprintf(script, "set term png size 960,720 font 'Helvetica,12'\n");
	fprintf(script, "set datafile sep ','\n");
	fprintf(script, "set output '%s'\n", knob::rlp_le_ir_plot_file_name.c_str());
	fprintf(script, "set title \"Reward over time\"\n");
	fprintf(script, "set xlabel \"Time\"\n");
	fprintf(script, "set ylabel \"Score\"\n");
	fprintf(script, "set grid y\n");
	fprintf(script, "set key right bottom Left box 3\n");
	fprintf(script, "plot ");
	fprintf(script, ", ");

	for(auto )
	for(uint32_t index = 0; index < knob::rlp_le_ir_plot_actions.size(); ++index)
	{
		if(index) fprintf(script, ", ");
		fprintf(script, "'%s' using 1:%lu with lines title \"action_index[%lu]\"", 
		knob::rlp_le_ir_trace_file_name.c_str(), (knob::rlp_le_ir_plot_actions[index]+2), knob::rlp_le_ir_plot_actions[index]);
	}
	fprintf(script, "\n");
	fclose(script);

	std::string cmd = "gnuplot " + std::string(script_file);
	system(cmd.c_str());

	std::string cmd2 = "rm " + std::string(script_file);
	system(cmd2.c_str());
	*/
}

void LearningEngineBasic_rlp::dump_action_trace(uint64_t action)
{
	action_trace_timestamp++;
	fprintf(action_trace, "%lu, %lu\n", action_trace_timestamp, action);
}

void LearningEngineBasic_rlp::init_stats() {

	bzero(&stats, sizeof(stats));

}

uint32_t LearningEngineBasic_rlp::get_qtable_way(uint64_t action){
	return action % m_qtable_ways;
}