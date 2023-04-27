#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <sstream>
#include "learning_engine_basic_cbp.h"
#include "cbp.h"
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
	extern bool     cbp_le_enable_trace;
	extern uint32_t cbp_le_trace_interval;
	extern std::string   cbp_le_trace_file_name;
	extern uint32_t cbp_le_trace_state;
	extern bool     cbp_le_enable_score_plot;
	//extern std::vector<int32_t> cbp_le_plot_actions;
	extern std::string   cbp_le_plot_file_name;
	extern bool     cbp_le_enable_action_trace;
	extern uint32_t cbp_le_action_trace_interval;
	extern std::string cbp_le_action_trace_name;
	extern bool     cbp_le_enable_action_plot;
}

LearningEngineBasic_cbp::LearningEngineBasic_cbp(Prefetcher *parent, float alpha, float gamma, uint32_t states, std::string policy, std::string learning_type, bool zero_init, uint64_t early_exploration_window)
	: LearningEngineBase(parent, alpha, gamma, 0, 0, states, 0, policy, learning_type)
{
	qtable = new std::map<uint64_t, float>[m_states];
	assert(qtable);

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
		qtable[row][0] = init_value;
	}

	if(knob::cbp_le_enable_trace)
	{
		trace_interval = 0;
		trace_timestamp = 0;
		trace = fopen(knob::cbp_le_trace_file_name.c_str(), "w");
		assert(trace);
	}

	if(knob::cbp_le_enable_action_trace)
	{
		action_trace_interval = 0;
		action_trace_timestamp = 0;
		action_trace = fopen(knob::cbp_le_action_trace_name.c_str(), "w");
		assert(action_trace);
	}

	m_early_exploration_window = early_exploration_window;
	m_action_counter = 0;

	init_stats();
}

LearningEngineBasic_cbp::~LearningEngineBasic_cbp()
{
	
	delete [] qtable;
	qtable = nullptr;
	
	if(knob::cbp_le_enable_trace && trace)
	{
		fclose(trace);
	}

	if(knob::cbp_le_enable_action_trace && action_trace){
		fclose(action_trace);
	}
}

uint64_t LearningEngineBasic_cbp::chooseAction(uint32_t state)
{
	stats.action.called++;
	assert(state < m_states);
	uint64_t action = 0;
	if(m_type == LearningType::SARSA && m_policy == Policy::Directly)
	{
		if(m_action_counter < m_early_exploration_window)
		{
			action = NO_PREFETCH_ACTION; // take no prefetch action
			stats.action.explore++;
			stats.action.dist[0][action]++;
			MYLOG("action taken %lu explore, state %x, scores %s", action, state, getStringQ(state).c_str());
		}
		else
		{
			action = getMaxAction(state);
			stats.action.exploit++;
			stats.action.dist[1][action]++;
			MYLOG("action taken %lu exploit, state %x, scores %s", action, state, getStringQ(state).c_str());
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

	m_action_counter++;


	if(knob::cbp_le_enable_action_trace && action_trace_interval++ == knob::cbp_le_action_trace_interval)
	{
		dump_action_trace(action);
		action_trace_interval = 0;
	}

	return action;
}

void LearningEngineBasic_cbp::learn(uint32_t state1, uint64_t action1, int32_t reward, uint32_t state2, uint64_t action2)
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

		if(knob::cbp_le_enable_trace && state1 == knob::cbp_le_trace_state && trace_interval++ == knob::cbp_le_trace_interval)
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

float LearningEngineBasic_cbp::consultQ(uint32_t state, uint64_t action)
{
	assert(state < m_states );

	if(!qtable[state].count(action)) return init_value;

	float value = qtable[state][action];
	return value;
}

void LearningEngineBasic_cbp::updateQ(uint32_t state, uint64_t action, float value)
{
	assert(state < m_states);
	qtable[state][action] = value;	
}

uint64_t LearningEngineBasic_cbp::getMaxAction(uint32_t state)
{
	assert(state < m_states);
	float maxScore = qtable[state][0];
	uint64_t action = 0;
	for(auto [curAction, curScore] : qtable[state])
	{
		if(curScore >= maxScore)
		{
			maxScore = curScore;
			action = curAction;
		}
	}
	return action;
}

bool LearningEngineBasic_cbp::insertAction(uint32_t state, uint64_t action){

	assert(state < m_states);

	if(!qtable[state].count(action)){
		qtable[state][action] = init_value;
		return true;
	}

	return false;
}

std::string LearningEngineBasic_cbp::getStringQ(uint32_t state)
{
	
	assert(state < m_states);
	std::stringstream ss;
	for(auto [curAction, curScore] : qtable[state])
	{
		ss << curScore << ",";
	}
	return ss.str();
}

void LearningEngineBasic_cbp::print_aux_stats()
{
	/* compute state-action table usage
	 * how mane state entries are actually used?
	 * heat-map dump, etc. etc. */
	uint32_t state_used = 0;
	for(uint32_t state = 0; state < m_states; ++state)
	{

		for(auto [curAction, curScore] : qtable[state])
		{
			if(abs(curScore - init_value) < 1e-7)
			{
				state_used++;
				break;
			}
		}
	}

	fprintf(stdout, "learning_engine.state_used %u\n", state_used);
	fprintf(stdout, "\n");
}

void LearningEngineBasic_cbp::dump_stats()
{
	
	
	CBP *cbp = (CBP*)m_parent;
	fprintf(stdout, "learning_engine.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine.action.exploit %lu\n", stats.action.exploit);

	for(auto [action, cnt] : stats.action.dist[0])
	{
		fprintf(stdout, "learning_engine.action.index_%lu_explored %u\n", cbp->getAction(action), stats.action.dist[0][action]);
	}

	for(auto [action, cnt] : stats.action.dist[1])
	{
		fprintf(stdout, "learning_engine.action.index_%lu_exploited %u\n", cbp->getAction(action), stats.action.dist[1][action]);
	}
	fprintf(stdout, "learning_engine.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");

	print_aux_stats();

	if(knob::cbp_le_enable_trace && knob::cbp_le_enable_score_plot)
	{
		plot_scores();
	}
}

void LearningEngineBasic_cbp::dump_state_trace(uint32_t state)
{
	trace_timestamp++;
	fprintf(trace, "%lu,", trace_timestamp);
	for(auto [curAction, curScore] : qtable[state])
	{
		fprintf(trace, "%lu, %.2f", curAction, curScore);
	}
	fprintf(trace, "\n");
	fflush(trace);
}

void LearningEngineBasic_cbp::plot_scores()
{
	/*
	char *script_file = (char*)malloc(16*sizeof(char));
	assert(script_file);
	gen_random(script_file, 16);
	FILE *script = fopen(script_file, "w");
	assert(script);

	fprintf(script, "set term png size 960,720 font 'Helvetica,12'\n");
	fprintf(script, "set datafile sep ','\n");
	fprintf(script, "set output '%s'\n", knob::cbp_le_plot_file_name.c_str());
	fprintf(script, "set title \"Reward over time\"\n");
	fprintf(script, "set xlabel \"Time\"\n");
	fprintf(script, "set ylabel \"Score\"\n");
	fprintf(script, "set grid y\n");
	fprintf(script, "set key right bottom Left box 3\n");
	fprintf(script, "plot ");
	fprintf(script, ", ");

	for(auto )
	for(uint32_t index = 0; index < knob::cbp_le_plot_actions.size(); ++index)
	{
		if(index) fprintf(script, ", ");
		fprintf(script, "'%s' using 1:%lu with lines title \"action_index[%lu]\"", knob::cbp_le_trace_file_name.c_str(), (knob::cbp_le_plot_actions[index]+2), knob::cbp_le_plot_actions[index]);
	}
	fprintf(script, "\n");
	fclose(script);

	std::string cmd = "gnuplot " + std::string(script_file);
	system(cmd.c_str());

	std::string cmd2 = "rm " + std::string(script_file);
	system(cmd2.c_str());
	*/
}

void LearningEngineBasic_cbp::dump_action_trace(uint64_t action)
{
	action_trace_timestamp++;
	fprintf(action_trace, "%lu, %lu\n", action_trace_timestamp, action);
}

void LearningEngineBasic_cbp::init_stats() {

	/**带容器结构体，初始化错误*/
	//bzero(&stats, sizeof(stats));

	stats.action.called = 0;
	stats.action.explore = 0;
	stats.action.exploit = 0;
	stats.action.dist[0].clear();
	stats.action.dist[1].clear();

	stats.learn.called = 0;
	
}