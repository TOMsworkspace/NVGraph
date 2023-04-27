#include <assert.h>
#include <algorithm>
#include <queue>
#include <iomanip>
#include "champsim.h"
#include "cache.h"
#include "memory_class.h"
#include "rlp.h"
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

/* Action array
 * Basically a set of deltas to evaluate
 * Similar to the concept of BOP */
static std::vector<int32_t> RLP_RP_Actions;

namespace knob
{

	/*init*/
	extern float            rlp_ir_alpha;
	extern float            rlp_rp_alpha;
	extern float            rlp_ir_gamma;
	extern float            rlp_rp_gamma;
	extern float            rlp_epsilon;
	extern float            rlp_rp_epsilon;
	extern uint32_t         rlp_ir_max_states;
	extern uint32_t         rlp_rp_max_states;
	extern vector<int32_t>  rlp_rp_actions;
	extern uint32_t         rlp_ir_max_actions;
	extern uint32_t         rlp_rp_max_actions;
	extern uint32_t         rlp_seed;
	extern uint32_t         rlp_rp_seed;
	extern string           rlp_ir_policy;
	extern string           rlp_rp_policy;
	extern string           rlp_ir_learning_type;
	extern string           rlp_rp_learning_type;
	extern bool		        rlp_ir_brain_zero_init;
	extern bool		        rlp_rp_brain_zero_init;
	extern uint64_t         rlp_ir_early_exploration_window;
	//extern uint64_t         rlp_rp_early_exploration_window;

	/*reward*/
	extern bool             rlp_enable_reward_all;
	extern bool             rlp_enable_hbw_reward;
	extern bool             rlp_enable_reward_ir_prefetch_queue_hit;
	extern bool             rlp_enable_reward_rp_prefetch_queue_hit;
	extern bool             rlp_enable_reward_rp_out_of_bounds;
	extern int32_t          rlp_reward_ir_none;
	extern int32_t          rlp_reward_rp_none;
	extern int32_t          rlp_reward_ir_incorrect;
	extern int32_t          rlp_reward_rp_incorrect;
	extern int32_t          rlp_reward_ir_correct_timely;
	extern int32_t          rlp_reward_rp_correct_timely;
	extern int32_t          rlp_reward_ir_correct_untimely;
	extern int32_t          rlp_reward_rp_correct_untimely;
	extern int32_t          rlp_reward_ir_prefetch_queue_hit;
	extern int32_t          rlp_reward_rp_prefetch_queue_hit;
	extern int32_t          rlp_reward_rp_out_of_bounds;
	extern int32_t          rlp_reward_ir_hbw_none;
	extern int32_t          rlp_reward_rp_hbw_none;
	extern int32_t          rlp_reward_ir_hbw_incorrect;
	extern int32_t          rlp_reward_rp_hbw_incorrect;
	extern int32_t          rlp_reward_ir_hbw_correct_timely;
	extern int32_t          rlp_reward_rp_hbw_correct_timely;
	extern int32_t          rlp_reward_ir_hbw_correct_untimely;
	extern int32_t          rlp_reward_rp_hbw_correct_untimely;
	extern int32_t          rlp_reward_ir_hbw_prefetch_queue_hit;
	extern int32_t          rlp_reward_rp_hbw_prefetch_queue_hit;
	extern int32_t          rlp_reward_rp_hbw_out_of_bounds;

	/*module*/
	extern uint32_t         rlp_prefetch_queue_size;
	extern uint32_t         rlp_signature_table_size;
	extern uint32_t         rlp_history_state_queue_size;
	extern uint32_t         rlp_action_tracker_size;
	extern uint32_t         rlp_ir_state_type; /*state*/
	extern uint32_t         rlp_rp_state_type;
	extern uint32_t         rlp_ir_state_hash_type;
	extern uint32_t         rlp_rp_state_hash_type;
	extern uint32_t         rlp_max_pcs;
	extern uint32_t         rlp_max_offsets;
	extern uint32_t         rlp_max_deltas;

	/*feature*/
	extern uint32_t         rlp_ir_pref_degree; 
	extern uint32_t         rlp_rp_pref_degree; 
	extern uint32_t         rlp_rp_enable_dyn_degree;/*dynamic pref degree*/
	extern uint32_t         rlp_rp_multi_deg_select_type;
	extern vector<float>    rlp_rp_max_to_avg_q_thresholds;
	extern vector<int32_t>  rlp_rp_dyn_degrees;
	extern vector<int32_t>  rlp_rp_last_pref_offset_conf_thresholds;
	extern vector<int32_t>  rlp_rp_dyn_degrees_type2;
	extern vector<int32_t>  rlp_rp_last_pref_offset_conf_thresholds_hbw;
	extern vector<int32_t>  rlp_rp_dyn_degrees_type2_hbw;
	extern bool             rlp_enable_track_multiple; /**prefetch queue multi-track*/
	extern uint32_t         rlp_high_bw_thresh;        /*high bandwidth threshold*/

	/*debug*/
	extern bool             rlp_debug_enable_state_action_stats;
	extern uint32_t         rlp_debug_state_action_stats_topK;
	extern bool             rlp_debug_access;
	extern bool             rlp_debug_print_access;
	extern uint64_t         rlp_debug_print_access_pc;
	extern uint32_t         rlp_debug_print_access_pc_count;
	extern bool             rlp_debug_print_trace;

	/* RLP Irregular Pattern Learning Engine knobs */
	extern uint32_t         rlp_le_ir_way;
	extern bool             rlp_le_ir_enable_trace;
	extern uint32_t         rlp_le_ir_trace_interval;
	extern std::string      rlp_le_ir_trace_file_name;
	extern uint32_t         rlp_le_ir_trace_state;
	extern bool             rlp_le_ir_enable_score_plot;
	//extern std::vector<int32_t> rlp_le_ir_plot_actions;
	extern std::string      rlp_le_ir_plot_file_name;
	extern bool             rlp_le_ir_enable_action_trace;
	extern uint32_t         rlp_le_ir_action_trace_interval;
	extern std::string      rlp_le_ir_action_trace_name;
	extern bool             rlp_le_ir_enable_action_plot;

	/* RLP Regular Pattern Learning Engine knobs */
	extern vector<int32_t> 	rlp_le_rp_featurewise_active_features;
	extern vector<int32_t> 	rlp_le_rp_featurewise_num_tilings;
	extern vector<int32_t> 	rlp_le_rp_featurewise_num_tiles;
	extern vector<int32_t> 	rlp_le_rp_featurewise_hash_types;
	extern vector<int32_t> 	rlp_le_rp_featurewise_enable_tiling_offset;
	extern float			rlp_le_rp_featurewise_max_q_thresh;
	extern bool				rlp_le_rp_featurewise_enable_action_fallback;
	extern vector<float> 	rlp_le_rp_featurewise_feature_weights;
	extern bool				rlp_le_rp_featurewise_enable_dynamic_weight;
	extern float			rlp_le_rp_featurewise_weight_gradient;
	extern bool				rlp_le_rp_featurewise_disable_adjust_weight_all_features_align;
	extern bool				rlp_le_rp_featurewise_selective_update;
	extern uint32_t 		rlp_le_rp_featurewise_pooling_type;
	extern bool             rlp_le_rp_featurewise_enable_dyn_action_fallback;
	extern uint32_t 		rlp_le_rp_featurewise_bw_acc_check_level;
	extern uint32_t 		rlp_le_rp_featurewise_acc_thresh;
	extern bool 			rlp_le_rp_featurewise_enable_trace;
	extern uint32_t		    rlp_le_rp_featurewise_trace_feature_type;
	extern string 			rlp_le_rp_featurewise_trace_feature;
	extern uint32_t 		rlp_le_rp_featurewise_trace_interval;
	extern uint32_t 		rlp_le_rp_featurewise_trace_record_count;
	extern std::string 	    rlp_le_rp_featurewise_trace_file_name;
	extern bool 			rlp_le_rp_featurewise_enable_score_plot;
	extern vector<int32_t>  rlp_le_rp_featurewise_plot_actions;
	extern std::string 	    rlp_le_rp_featurewise_plot_file_name;
	extern bool 			rlp_le_rp_featurewise_remove_plot_script;
}

void RLP::init_knobs()
{
	if(knob::rlp_rp_max_actions != 0 && knob::rlp_rp_actions.size() != 0){
		RLP_RP_Actions.resize(knob::rlp_rp_max_actions, 0);
		std::copy(knob::rlp_rp_actions.begin(), knob::rlp_rp_actions.end(), RLP_RP_Actions.begin());
		assert(RLP_RP_Actions.size() == knob::rlp_rp_max_actions);
	}
	else{
		knob::rlp_rp_actions.clear();
		for(int idx = -64; idx < 64; ++idx){
			RLP_RP_Actions.push_back(idx);
			knob::rlp_rp_actions.push_back(idx);
		}
		knob::rlp_rp_max_actions = 128;
	}
	assert(RLP_RP_Actions.size() <= RLP_RP_MAX_ACTIONS);

	if(knob::rlp_debug_access)
	{
		cout << "***WARNING*** setting knob::rlp_max_pcs, knob::rlp_max_offsets, and knob::rlp_max_deltas to large value as knob::rlp_debug_access is true" << endl;
		knob::rlp_max_pcs = 1024;
		knob::rlp_max_offsets = 1024;
		knob::rlp_max_deltas = 1024;
	}

	assert(knob::rlp_rp_pref_degree >= 1 && (knob::rlp_rp_pref_degree == 1 || !knob::rlp_rp_enable_dyn_degree));
	assert(knob::rlp_rp_max_to_avg_q_thresholds.size() == knob::rlp_rp_dyn_degrees.size()-1);
	assert(knob::rlp_rp_last_pref_offset_conf_thresholds.size() == knob::rlp_rp_dyn_degrees_type2.size()-1);
}

void RLP::init_stats()
{
	//bzero(&stats, sizeof(stats));

	//stats
	bzero(&stats.st, sizeof(stats.st));
	stats.predict.called = 0;
	stats.predict.ir_called = 0;
	stats.predict.rp_called = 0;
	stats.predict.rp_out_of_bounds = 0;
	stats.predict.ir_action_dist.clear();
	stats.predict.ir_issue_dist.clear();
	stats.predict.ir_pred_hit.clear();
	stats.predict.rp_action_dist.resize(knob::rlp_rp_max_actions, 0);
	stats.predict.rp_issue_dist.resize(knob::rlp_rp_max_actions, 0);
	stats.predict.rp_pred_hit.resize(knob::rlp_rp_max_actions, 0);
	stats.predict.rp_out_of_bounds_dist.resize(knob::rlp_rp_max_actions, 0);
	stats.predict.predicted = 0;
	stats.predict.ir_predicted = 0;
	stats.predict.rp_predicted = 0;
	stats.predict.multi_deg = 0;
	stats.predict.ir_multi_deg = 0;
	stats.predict.rp_multi_deg = 0;
	stats.predict.ir_multi_deg_called = 0;
	stats.predict.rp_multi_deg_called = 0;
	bzero(&stats.predict.ir_multi_deg_histogram, sizeof(stats.predict.ir_multi_deg_histogram));
	bzero(&stats.predict.rp_multi_deg_histogram, sizeof(stats.predict.rp_multi_deg_histogram));
	bzero(&stats.predict.ir_deg_histogram, sizeof(stats.predict.ir_deg_histogram));
	bzero(&stats.predict.rp_deg_histogram, sizeof(stats.predict.rp_deg_histogram));
	bzero(&stats.track, sizeof(&stats.track));
	bzero(&stats.reward.demand, sizeof(stats.reward.demand));
	bzero(&stats.reward.train, sizeof(stats.reward.train));
	bzero(&stats.reward.assign_reward, sizeof(stats.reward.assign_reward)); 
	bzero(&stats.reward.compute_reward, sizeof(stats.reward.compute_reward));
	stats.reward.ir_no_pref = 0;
	stats.reward.ir_incorrect = 0;
	stats.reward.ir_correct_timely = 0;
	stats.reward.ir_correct_untimely = 0;
	stats.reward.ir_prefetch_queue_hit = 0;
	stats.reward.rp_no_pref = 0;
	stats.reward.rp_incorrect = 0;
	stats.reward.rp_correct_timely = 0;
	stats.reward.rp_correct_untimely = 0;
	stats.reward.rp_prefetch_queue_hit = 0;
	stats.reward.rp_out_of_bounds = 0;
	for(uint32_t idx = 0; idx < RLP_MAX_REWARDS; idx++)
		stats.reward.ir_dist[idx].clear();
	bzero(&stats.reward.rp_dist, sizeof(stats.reward.rp_dist));
	bzero(&stats.train, sizeof(stats.train));
	bzero(&stats.register_fill, sizeof(stats.register_fill));
	bzero(&stats.register_prefetch_hit, sizeof(stats.register_prefetch_hit));
	bzero(&stats.pref_issue, sizeof(stats.pref_issue));
	bzero(&stats.bandwidth, sizeof(stats.bandwidth));
	bzero(&stats.ipc, sizeof(stats.ipc));
	bzero(&stats.cache_acc, sizeof(stats.cache_acc));

	//dist
	ir_state_action_dist.clear();
	ir_state_action_dist2.clear();
	ir_action_deg_dist.clear();
	rp_state_action_dist.clear();
	rp_state_action_dist2.clear();
	rp_action_deg_dist.clear();
}

RLP::RLP(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	m_rlp_epsilon = knob::rlp_epsilon;
	m_seed = knob::rlp_seed;
	generator.seed(m_seed);
	access_pattern_explore = new std::bernoulli_distribution(m_rlp_epsilon);

	recorder = new RLPRecorder();
	prefetch_queue.clear();
	history_state_table.clear();

	ir_last_evicted_tracker = NULL;
	rp_last_evicted_tracker = NULL;

	/* init learning engine */
	ir_brain = NULL;
	rp_brain = NULL;

	ir_brain = new LearningEngineBasic_rlp( this,
								knob::rlp_ir_alpha, 
								knob::rlp_ir_gamma, 
								knob::rlp_ir_max_actions,
								knob::rlp_ir_max_states,
								knob::rlp_ir_policy,
								knob::rlp_ir_learning_type,
								knob::rlp_ir_brain_zero_init,
								knob::rlp_ir_early_exploration_window);
	
	rp_brain = new LearningEngineFeaturewise_rlp(this,
								knob::rlp_rp_alpha, 
								knob::rlp_rp_gamma, 
								knob::rlp_rp_epsilon,
								knob::rlp_rp_max_actions,
								knob::rlp_rp_seed,
								knob::rlp_rp_policy,
								knob::rlp_rp_learning_type,
								knob::rlp_rp_brain_zero_init);

	bw_level = 0;
	core_ipc = 0;
}

RLP::~RLP()
{
	if(ir_brain) 		delete ir_brain;
	if(rp_brain) 		delete rp_brain;

	delete recorder;

	for(uint32_t index = 0; index < signature_table.size(); index++) 
		delete signature_table[index];
	
	for(uint32_t index = 0; index < prefetch_queue.size(); index++) 
		delete prefetch_queue[index];

	prefetch_queue.clear();
	history_state_table.clear();
	
	delete access_pattern_explore;

	if(ir_last_evicted_tracker) delete ir_last_evicted_tracker;
	if(rp_last_evicted_tracker) delete rp_last_evicted_tracker;
}

void RLP::print_config()
{	

	cout << "rlp_ir_alpha " << knob::rlp_ir_alpha << endl
		<< "rlp_rp_alpha " << knob::rlp_rp_alpha << endl
		<< "rlp_ir_gamma " << knob::rlp_ir_gamma << endl
		<< "rlp_rp_gamma " << knob::rlp_rp_gamma << endl
		<< "rlp_epsilon " << knob::rlp_epsilon << endl
		<< "rlp_rp_epsilon " << knob::rlp_rp_epsilon << endl
		<< "rlp_ir_max_states " << knob::rlp_ir_max_states << endl
		<< "rlp_rp_max_states " << knob::rlp_rp_max_states << endl
		<< "rlp_rp_actions " << array_to_string(RLP_RP_Actions) << endl
		<< "rlp_ir_max_actions " << knob::rlp_ir_max_actions << endl
		<< "rlp_rp_max_actions " << knob::rlp_rp_max_actions << endl
		<< "rlp_seed " << knob::rlp_seed << endl
		<< "rlp_rp_seed " << knob::rlp_rp_seed << endl
		<< "rlp_ir_policy " << knob::rlp_ir_policy << endl
		<< "rlp_rp_policy " << knob::rlp_rp_policy << endl
		<< "rlp_ir_learning_type " << knob::rlp_ir_learning_type << endl
		<< "rlp_rp_learning_type " << knob::rlp_rp_learning_type << endl
		<< "rlp_ir_brain_zero_type " << knob::rlp_ir_brain_zero_init << endl
		<< "rlp_rp_brain_zero_type " << knob::rlp_rp_brain_zero_init << endl
		<< "rlp_ir_early_exploration_window " << knob::rlp_ir_early_exploration_window << endl
		<< "rlp reward" << endl
		<< "rlp_enable_reward_all " << knob::rlp_enable_reward_all << endl
		<< "rlp_enable_hbw_reward " << knob::rlp_enable_hbw_reward << endl
		<< "rlp_enable_reward_ir_prefetch_queue_hit " << knob::rlp_enable_reward_ir_prefetch_queue_hit << endl
		<< "rlp_enable_reward_rp_prefetch_queue_hit " << knob::rlp_enable_reward_rp_prefetch_queue_hit << endl
		<< "rlp_enbale_reward_rp_out_of_bounds " << knob::rlp_enable_reward_rp_out_of_bounds << endl
		<< "rlp_reward_ir_none " << knob::rlp_reward_ir_none << endl
		<< "rlp_reward_rp_none " << knob::rlp_reward_rp_none << endl
		<< "rlp_reward_ir_incorrect " << knob::rlp_reward_ir_incorrect << endl
		<< "rlp_reward_rp_incorrect " << knob::rlp_reward_rp_incorrect << endl
		<< "rlp_reward_ir_correct_timely " << knob::rlp_reward_ir_correct_timely << endl
		<< "rlp_reward_rp_correct_timely " << knob::rlp_reward_rp_correct_timely << endl
		<< "rlp_reward_ir_correct_untimely " << knob::rlp_reward_ir_correct_untimely << endl
		<< "rlp_reward_rp_correct_untimely " << knob::rlp_reward_rp_correct_untimely << endl
		<< "rlp_reward_ir_prefetch_queue_hit " << knob::rlp_reward_ir_prefetch_queue_hit << endl
		<< "rlp_reward_rp_prefetch_queue_hit " << knob::rlp_reward_rp_prefetch_queue_hit << endl
		<< "rlp_reward_rp_out_of_bounds " << knob::rlp_reward_rp_out_of_bounds << endl
		<< "rlp_reward_ir_hbw_none " << knob::rlp_reward_ir_none << endl
		<< "rlp_reward_rp_hbw_none " << knob::rlp_reward_rp_none << endl
		<< "rlp_reward_ir_hbw_incorrect " << knob::rlp_reward_ir_incorrect << endl
		<< "rlp_reward_rp_hbw_incorrect " << knob::rlp_reward_rp_incorrect << endl
		<< "rlp_reward_ir_hbw_correct_timely " << knob::rlp_reward_ir_correct_timely << endl
		<< "rlp_reward_rp_hbw_correct_timely " << knob::rlp_reward_rp_correct_timely << endl
		<< "rlp_reward_ir_hbw_correct_untimely " << knob::rlp_reward_ir_correct_untimely << endl
		<< "rlp_reward_rp_hbw_correct_untimely " << knob::rlp_reward_rp_correct_untimely << endl
		<< "rlp_reward_ir_hbw_prefetch_queue_hit " << knob::rlp_reward_ir_prefetch_queue_hit << endl
		<< "rlp_reward_rp_hbw_prefetch_queue_hit " << knob::rlp_reward_rp_prefetch_queue_hit << endl
		<< "rlp_reward_rp_hbw_out_of_bounds " << knob::rlp_reward_rp_out_of_bounds << endl
		<< "rlp module" << endl
		<< "rlp_prefetch_queue_size " << knob::rlp_prefetch_queue_size << endl
		<< "rlp_signature_table_size " << knob::rlp_signature_table_size << endl
		<< "rlp_history_state_queue_size " << knob::rlp_history_state_queue_size << endl
		<< "rlp_action_tracker_size " << knob::rlp_action_tracker_size << endl
		<< "rlp_ir_state_type " << knob::rlp_ir_state_type << endl
		<< "rlp_rp_state_type " << knob::rlp_rp_state_type << endl
		<< "rlp_ir_state_hash_type " << knob::rlp_ir_state_hash_type << endl
		<< "rlp_rp_state_hash_type " << knob::rlp_rp_state_hash_type << endl
		<< "rlp_max_pcs " << knob::rlp_max_pcs << endl
		<< "rlp_max_offsets " << knob::rlp_max_offsets << endl
		<< "rlp_max_deltas " << knob::rlp_max_deltas << endl
		<< "rlp feature" << endl
		<< "rlp_ir_pref_degree " << knob::rlp_ir_pref_degree << endl
		<< "rlp_rp_pref_degree " << knob::rlp_rp_pref_degree << endl
		<< "rlp_rp_enable_dyn_degree " << knob::rlp_rp_enable_dyn_degree << endl
		<< "rlp_rp_multi_deg_select_type " << knob::rlp_rp_multi_deg_select_type << endl
		<< "rlp_rp_max_to_avg_q_thresholds " << array_to_string(knob::rlp_rp_max_to_avg_q_thresholds) << endl
		<< "rlp_rp_dyn_degrees " << array_to_string(knob::rlp_rp_dyn_degrees) << endl
		<< "rlp_rp_last_pref_offset_conf_thresholds " << array_to_string(knob::rlp_rp_last_pref_offset_conf_thresholds) << endl
		<< "rlp_rp_dyn_degrees_type2 " << array_to_string(knob::rlp_rp_dyn_degrees_type2) << endl
		<< "rlp_rp_last_pref_offset_conf_thresholds_hbw " << array_to_string(knob::rlp_rp_last_pref_offset_conf_thresholds_hbw) << endl
		<< "rlp_rp_dyn_degrees_type2_hbw " << array_to_string(knob::rlp_rp_dyn_degrees_type2_hbw) << endl
		<< "rlp_enable_track_multiple " << knob::rlp_enable_track_multiple << endl
		<< "rlp_high_bw_threshold " << knob::rlp_high_bw_thresh << endl
		<< "rlp debug" << endl
		<< "rlp_debug_enable_state_action_stats " << knob::rlp_debug_enable_state_action_stats << endl
		<< "rlp_debug_state_action_stats_topK " << knob::rlp_debug_state_action_stats_topK << endl
		<< "rlp_debug_access " << knob::rlp_debug_access << endl
		<< "rlp_debug_print_access " << knob::rlp_debug_print_access << endl
		<< "rlp_debug_print_access_pc " << knob::rlp_debug_print_access_pc_count << endl
		<< "rlp_debug_print_trace " << knob::rlp_debug_print_trace << endl
		<< "rlp ir learn engine knobs" << endl
		<< "rlp_le_ir_way " << knob::rlp_le_ir_way << endl
		<< "rlp_le_ir_enable_trace " << knob::rlp_le_ir_enable_trace << endl
		<< "rlp_le_ir_trace_interval " << knob::rlp_le_ir_trace_interval << endl
		<< "rlp_le_ir_trace_file_name " << knob::rlp_le_ir_trace_file_name << endl
		<< "rlp_le_ir_trace_state " << knob::rlp_le_ir_trace_state << endl
		<< "rlp_le_ir_enable_score_plot " << knob::rlp_le_ir_enable_score_plot << endl
		<< "rlp_le_ir_plot_file_name " << knob::rlp_le_ir_plot_file_name << endl
		<< "rlp_le_ir_enable_action_trace " << knob::rlp_le_ir_enable_action_trace << endl
		<< "rlp_le_ir_action_trace_interval " << knob::rlp_le_ir_action_trace_interval << endl
		<< "rlp_le_ir_action_trace_name " << knob::rlp_le_ir_action_trace_name << endl
		<< "rlp_le_ir_enable_action_plot " << knob::rlp_le_ir_enable_action_plot << endl 
		<< "rlp rp learn engine knobs" << endl
		<< "rlp_le_rp_featurewise_active_features " << print_active_features2(knob::rlp_le_rp_featurewise_active_features) << endl
		<< "rlp_le_rp_featurewise_num_tilings " << array_to_string(knob::rlp_le_rp_featurewise_num_tilings) << endl
		<< "rlp_le_rp_featurewise_num_tiles " << array_to_string(knob::rlp_le_rp_featurewise_num_tiles) << endl
		<< "rlp_le_rp_featurewise_hash_types " << array_to_string(knob::rlp_le_rp_featurewise_hash_types) << endl
		<< "rlp_le_rp_featurewise_enabrle_tiling_offset " << array_to_string(knob::rlp_le_rp_featurewise_enable_tiling_offset) << endl
		<< "rlp_le_rp_featurewise_max_q_thresh " << knob::rlp_le_rp_featurewise_max_q_thresh << endl
		<< "rlp_le_rp_featurewise_enable_action_fallback " << knob::rlp_le_rp_featurewise_enable_action_fallback << endl
		<< "rlp_le_rp_featurewise_feature_weights " << array_to_string(knob::rlp_le_rp_featurewise_feature_weights) << endl
		<< "rlp_le_rp_featurewise_enable_dynamic_weight " << knob::rlp_le_rp_featurewise_enable_dynamic_weight << endl
		<< "rlp_le_rp_featurewise_weight_gradient " << knob::rlp_le_rp_featurewise_weight_gradient << endl
		<< "rlp_le_rp_featurewise_disable_adjust_weight_all_features_align " << knob::rlp_le_rp_featurewise_disable_adjust_weight_all_features_align << endl
		<< "rlp_le_rp_featurewise_selective_update " << knob::rlp_le_rp_featurewise_selective_update << endl
		<< "rlp_le_rp_featurewise_pooling_type " << knob::rlp_le_rp_featurewise_pooling_type << endl
		<< "rlp_le_rp_featurewise_enable_dyn_action_fallback " << knob::rlp_le_rp_featurewise_enable_dyn_action_fallback << endl
		<< "rlp_le_rp_featurewise_bw_acc_check_level " << knob::rlp_le_rp_featurewise_bw_acc_check_level << endl
		<< "rlp_le_rp_featurewise_acc_thresh " << knob::rlp_le_rp_featurewise_acc_thresh << endl
		<< "rlp_le_rp_featurewise_enable_trace " << knob::rlp_le_rp_featurewise_enable_trace << endl
		<< "rlp_le_rp_featurewise_trace_feature_type " << knob::rlp_le_rp_featurewise_trace_feature_type << endl
		<< "rlp_le_rp_featurewise_trace_feature " << knob::rlp_le_rp_featurewise_trace_feature << endl
		<< "rlp_le_rp_featurewise_trace_interval " << knob::rlp_le_rp_featurewise_trace_interval << endl
		<< "rlp_le_rp_featurewise_trace_record_count " << knob::rlp_le_rp_featurewise_trace_record_count << endl
		<< "rlp_le_rp_featurewise_trace_file_name " << knob::rlp_le_rp_featurewise_trace_file_name << endl
		<< "rlp_le_rp_featurewise_enable_score_plot " << knob::rlp_le_rp_featurewise_enable_score_plot << endl
		<< "rlp_le_rp_featurewise_plot_actions " << array_to_string(knob::rlp_le_rp_featurewise_plot_actions) << endl
		<< "rlp_le_rp_featurewise_plot_file_name " << knob::rlp_le_rp_featurewise_plot_file_name << endl
		<< "rlp_le_rp_featurewise_remove_plot_script " << knob::rlp_le_rp_featurewise_remove_plot_script << endl
		<< endl;

}

void RLP::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint64_t cacheline = address >> LOG2_BLOCK_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	MYLOG("---------------------------------------------------------------------");
	MYLOG("%s %lx cacheline %lx pc %lx page %lx off %u", GetAccessType(type), address, cacheline, pc, page, offset);

	/* compute reward on demand */
	reward(address);

	/* record the access: just to gain some insights from the workload
	 * defined in rlp_helper.h(cc) */
	recorder->record_access(pc, address, page, offset, bw_level);

	/* global state tracking */
	update_global_state(pc, page, offset, address);
	/* per page state tracking */
	RLP_STEntry *stentry = update_local_state(pc, page, offset, address);

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
	stats.pref_issue.rlp += (pref_addr.size() - count);
}

void RLP::update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	/* @rbera TODO: implement */
}

RLP_STEntry* RLP::update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	stats.st.lookup++;
	RLP_STEntry *stentry = NULL;
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](RLP_STEntry *stentry){return stentry->page == page;});
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
		if(signature_table.size() >= knob::rlp_signature_table_size)
		{
			stats.st.evict++;
			stentry = signature_table.front();
			signature_table.pop_front();
			if(knob::rlp_debug_access)
			{
				recorder->record_access_knowledge(stentry);
				if(knob::rlp_debug_print_access)
				{
					print_access_debug(stentry);
				}
			}
			delete stentry;
		}

		stats.st.insert++;
		stentry = new RLP_STEntry(page, pc, offset);
		recorder->record_trigger_access(page, pc, offset);
		signature_table.push_back(stentry);
		return stentry;
	}
}

uint32_t RLP::predict(uint64_t base_address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr)
{

	stats.predict.called++;

	/* query learning engine to get the next prediction */

	bool ir_prefetch = (*access_pattern_explore)(generator);

	vector<uint64_t> ir_pref, rp_pref;
	ir_pref.clear();
	rp_pref.clear();

	ir_predict(base_address, page, offset, state, ir_pref, ir_prefetch);
	stats.predict.ir_predicted += ir_pref.size();

	rp_predict(base_address, page, offset, state, rp_pref, !ir_prefetch);
	stats.predict.rp_predicted += rp_pref.size();

	if(ir_prefetch){
		//use ir_engine to get the next prefetch
		stats.pref_issue.rlp_ir += ir_pref.size();
		for(uint32_t idx = 0; idx < ir_pref.size(); ++idx){
			pref_addr.push_back(ir_pref[idx]);
		}
	}
	else{
		//use rp_engine to get the next prefetch
		stats.pref_issue.rlp_rp += rp_pref.size();
		for(uint32_t idx = 0; idx < rp_pref.size(); ++idx){
			pref_addr.push_back(rp_pref[idx]);
		}
	}
	
	stats.predict.predicted += ir_pref.size() + rp_pref.size();
	MYLOG("end@%lx", base_address);

	return pref_addr.size();
}

void RLP::ir_predict(uint64_t base_address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr, bool prefetch)
{

	stats.predict.ir_called++;
	uint64_t action_index = 0;
	uint32_t state_index = 0;
	uint32_t pref_degree = 1;

	MYLOG("addr@%lx page %lx off %u ir state %x", base_address, page, offset, state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states));
	
	pref_degree = knob::rlp_ir_pref_degree;
	state_index = state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states);
	assert(state_index < knob::rlp_ir_max_states);

	//query ir_engine to get the next prediction
	action_index = ir_brain->chooseAction(state_index);

	if(knob::rlp_debug_enable_state_action_stats && prefetch)
	{
		ir_update_stats(state_index, action_index, pref_degree);
	}
	
	assert(action_index < knob::rlp_ir_max_actions);
	
	MYLOG("ir act_idx %lu act %lu", action_index, getIrAction(action_index));

	uint64_t addr = NO_PREFETCH_ADDR;
	RLP_PQEntry *pqentry = NULL;

	if(!no_prefetch_action(action_index))
	{

		addr = action_index << LOG2_BLOCK_SIZE;

		/* track prefetch */
		bool new_addr = ir_track(addr, state, action_index, &pqentry, prefetch);
		if(new_addr)
		{
			pref_addr.push_back(addr);

			if(prefetch)
				stats.predict.ir_issue_dist[action_index]++;
			if(pref_degree > 1)
			{
				ir_gen_multi_degree_pref(page, offset, action_index, pref_degree, pref_addr);
			}
			stats.predict.ir_deg_histogram[pref_degree]++;
		}
		else
		{
			MYLOG("ir pred_addr %lx prefetch_queue_hit", action_index << LOG2_BLOCK_SIZE);
			stats.predict.ir_pred_hit[action_index]++;
			
			if(knob::rlp_enable_reward_ir_prefetch_queue_hit && prefetch){
				assert(pqentry);
				assign_reward(pqentry, RLPRewardType::rlp_ir_prefetch_queue_hit);
			}
		}
		stats.predict.ir_action_dist[action_index]++;
	}
	else
	{

		MYLOG("ir no prefecth");
		
		/* agent decided not to prefetch */
		addr = NO_PREFETCH_ADDR;
		/* track no prefetch */
		ir_track(addr, state, action_index, &pqentry, prefetch);
		stats.predict.ir_action_dist[action_index]++;

	}
}

void RLP::rp_predict(uint64_t base_address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr, bool prefetch)
{
	
	//query rp_engine to get the next prediction

	stats.predict.rp_called++;
	uint32_t action_index = 0;
	uint32_t pref_degree = 1;
	
	MYLOG("addr@%lx page %lx off %u rp state %x", base_address, page, offset, state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states));

	vector<bool> consensus_vec;

	float max_to_avg_q_ratio = 1.0;
	action_index = rp_brain->chooseAction(state, max_to_avg_q_ratio, consensus_vec);
	if(knob::rlp_rp_enable_dyn_degree)
	{
		pref_degree = rp_get_dyn_pref_degree(max_to_avg_q_ratio, page, getRpAction(action_index));
	}
	if(knob::rlp_debug_enable_state_action_stats && prefetch)
	{
		rp_update_stats(state, action_index, pref_degree);
	}

	assert(action_index < knob::rlp_rp_max_actions);

	MYLOG("rp act_idx %u rp act %d", action_index, getRpAction(action_index));

	uint64_t addr = NO_PREFETCH_ADDR;
	RLP_PQEntry *pqentry = NULL;
	int32_t predicted_offset = 0;
	if(!no_prefetch_action(getRpAction(action_index)))
	{
		predicted_offset = (int32_t)offset + getRpAction(action_index);
		if(predicted_offset >=0 && predicted_offset < 64) /* falls within the page */
		{
			addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
			MYLOG("rp pred_off %d pred_addr %lx", predicted_offset, addr);
			/* track prefetch */
			bool new_addr = rp_track(addr, state, action_index, &pqentry, prefetch);
			if(new_addr)
			{
				pref_addr.push_back(addr);
				track_in_st(page, predicted_offset, getRpAction(action_index));

				if(prefetch)
					stats.predict.rp_issue_dist[action_index]++;
				if(pref_degree > 1)
				{
					rp_gen_multi_degree_pref(page, offset, getRpAction(action_index), pref_degree, pref_addr);
				}
				stats.predict.rp_deg_histogram[pref_degree]++;
				if(prefetch)
					pqentry->consensus_vec = consensus_vec;
				
			}
			else
			{
				MYLOG("rp pred_off %d prefetch_queue_hit", predicted_offset);
				stats.predict.rp_pred_hit[action_index]++;
				if(knob::rlp_enable_reward_rp_prefetch_queue_hit && prefetch)
				{
					assert(pqentry);
					assign_reward(pqentry, RLPRewardType::rlp_rp_prefetch_queue_hit);
					pqentry->consensus_vec = consensus_vec;
				}
			}
			stats.predict.rp_action_dist[action_index]++;
		}
		else
		{
			MYLOG("rp pred_off %d out_of_bounds", predicted_offset);
			stats.predict.rp_out_of_bounds++;
			stats.predict.rp_out_of_bounds_dist[action_index]++;
			if(knob::rlp_enable_reward_rp_out_of_bounds && prefetch)
			{
				assert(pqentry);
				assign_reward(pqentry, RLPRewardType::rlp_rp_out_of_bounds);
				pqentry->consensus_vec = consensus_vec;
			}
		}
	}
	else
	{
		MYLOG("rp no prefecth");
		/* agent decided not to prefetch */
		addr = NO_PREFETCH_ADDR;
		/* track no prefetch */
		rp_track(addr, state, action_index, &pqentry, prefetch);
		stats.predict.rp_action_dist[action_index]++;

		if(prefetch)
			pqentry->consensus_vec = consensus_vec;
	}
}

/* Returns true if the address is not already present in prefetch_queue
 * false otherwise */
bool RLP::ir_track(uint64_t address, State *state, uint64_t action_index, RLP_PQEntry **ir_tracker, bool prefetch)
{	

	if(no_prefetch(address)){
		ir_tracker = NULL;
		return false;
	}

	MYLOG("addr@%lx state %x ir act_idx %lu act %lu", 
			address, 
			state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
			action_index, 
			getIrAction(action_index)
		);

	stats.track.ir_called++;

	bool new_addr = true;
	std::vector<RLP_PQEntry*> pqentries = ir_search_pq(address, false);
	if(pqentries.empty())
	{
		new_addr = true;
	}
	else
	{
		new_addr = false;
	}

	if(!new_addr && !no_prefetch(address) && !knob::rlp_enable_track_multiple)
	{
		stats.track.ir_same_address++;
		ir_tracker = NULL;
		return new_addr;
	}

	/*build history state -> cur addr action*/
	track_in_hsq(state);

	if(!prefetch){
		ir_tracker = NULL;
		return new_addr;
	}

	/* new prefetched address that hasn't been seen before */
	RLP_PQEntry *pqentry = NULL;

	/*evict entry from prefetch queue*/
	if(prefetch_queue.size() >= knob::rlp_prefetch_queue_size)
	{
		stats.track.ir_evict++;
		pqentry = prefetch_queue.front();
		prefetch_queue.pop_front();
		//evict ir entry from prefetch queue;
		MYLOG("victim_ir_state %x victim_ir_act_idx %lu victim_ir_act %lu", 
		       pqentry->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
			   pqentry->action_index, 
			   getIrAction(pqentry->action_index)
			);

		if(ir_last_evicted_tracker){
			MYLOG("last_victim_ir_state %x last_victim_act_idx %u last_victim_act %d", 
			       ir_last_evicted_tracker->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
				   ir_last_evicted_tracker->action_index, 
				   getIrAction(ir_last_evicted_tracker->action_index)
			);
			//train the agent
			ir_train(pqentry, ir_last_evicted_tracker);
			delete ir_last_evicted_tracker->state;
			delete ir_last_evicted_tracker;
		}
		
		ir_last_evicted_tracker = pqentry;
	}

	pqentry = new RLP_PQEntry(address, state, action_index, true);
	prefetch_queue.push_back(pqentry);
	assert(prefetch_queue.size() <= knob::rlp_prefetch_queue_size);

	(*ir_tracker) = pqentry;
	MYLOG("end@%lx", address);

	return new_addr;
}

bool RLP::rp_track(uint64_t address, State *state, uint32_t action_index, RLP_PQEntry **rp_tracker, bool prefetch)
{	

	MYLOG("addr@%lx rp state %x rp act_idx %u rp act %d", 
			address, 
			state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states), 
			action_index, 
			getRpAction(action_index)
		);
	

	stats.track.rp_called++;

	bool new_addr = true;
	std::vector<RLP_PQEntry*> pqentries = rp_search_pq(address, false);
	if(pqentries.empty())
	{
		new_addr = true;
	}
	else
	{
		new_addr = false;
	}

	if(!new_addr && !no_prefetch(address) && !knob::rlp_enable_track_multiple)
	{
		stats.track.rp_same_address++;
		rp_tracker = NULL;
		return new_addr;
	}

	if(!prefetch){
		rp_tracker = NULL;
		return new_addr;
	}

	/* new prefetched address that hasn't been seen before */
	RLP_PQEntry *pqentry = NULL;

	/*evict entry from prefetch queue*/
	if(prefetch_queue.size() >= knob::rlp_prefetch_queue_size)
	{
		stats.track.rp_evict++;
		pqentry = prefetch_queue.front();
		prefetch_queue.pop_front();

		//evict rp entry from prefetch queue;
		MYLOG("victim_rp_state %x victim_rp_act_idx %lu victim_rp_act %lu", 
		       pqentry->state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states),
			   pqentry->action_index, 
			   getRpAction(pqentry->action_index)
			);
	
		if(rp_last_evicted_tracker){
			MYLOG("last_victim_rp_state %x last_victim_rp_act_idx %u last_victim_rp_act %d", 
			      rp_last_evicted_tracker->state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states), 
				  rp_last_evicted_tracker->action_index, 
				  getRpAction(rp_last_evicted_tracker->action_index)
				);

			ir_train(pqentry, rp_last_evicted_tracker);
			delete rp_last_evicted_tracker->state;
			delete rp_last_evicted_tracker;
		}
		
		rp_last_evicted_tracker = pqentry;
	}

	pqentry = new RLP_PQEntry(address, state, action_index, false);
	prefetch_queue.push_back(pqentry);
	assert(prefetch_queue.size() <= knob::rlp_prefetch_queue_size);

	(*rp_tracker) = pqentry;
	MYLOG("end@%lx", address);

	return new_addr;
}

void RLP::ir_gen_multi_degree_pref(uint64_t page, uint32_t offset, uint64_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr)
{
	stats.predict.ir_multi_deg_called++;
	uint64_t addr = NO_PREFETCH_ADDR;
	int32_t predicted_offset = 0;
	if(!no_prefetch_action(action))
	{
		for(uint32_t degree = 1; degree < pref_degree; ++degree)
		{
			predicted_offset = (int32_t)(action & ((1 << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1)) + degree;
			if(predicted_offset >=0 && predicted_offset < 64)
			{
				addr = (action + degree) << LOG2_BLOCK_SIZE;
				pref_addr.push_back(addr);
				MYLOG("ir degree %u pred_off %d pred_addr %lx", degree, predicted_offset, addr);
				stats.predict.multi_deg++;
				stats.predict.ir_multi_deg++;
				stats.predict.ir_multi_deg_histogram[degree]++;
			}
		}
	}
}

void RLP::rp_gen_multi_degree_pref(uint64_t page, uint32_t offset, int32_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr)
{
	stats.predict.rp_multi_deg_called++;
	uint64_t addr = NO_PREFETCH_ADDR;
	int32_t predicted_offset = 0;
	if(action != 0)
	{
		for(uint32_t degree = 2; degree <= pref_degree; ++degree)
		{
			predicted_offset = (int32_t)offset + degree * action;
			if(predicted_offset >= 0 && predicted_offset < 64)
			{
				addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
				pref_addr.push_back(addr);
				MYLOG("rp degree %u pred_off %d pred_addr %lx", degree, predicted_offset, addr);
				stats.predict.multi_deg++;
				stats.predict.rp_multi_deg++;
				stats.predict.rp_multi_deg_histogram[degree]++;
			}
		}
	}
}

uint32_t RLP::rp_get_dyn_pref_degree(float max_to_avg_q_ratio, uint64_t page, int32_t action)
{
	uint32_t counted = false;
	uint32_t degree = 1;

	if(knob::rlp_rp_multi_deg_select_type == 2)
	{
		auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](RLP_STEntry *stentry){return stentry->page == page;});
		if(st_index != signature_table.end())
		{
			int32_t conf = 0;
			bool found = (*st_index)->search_action_tracker(action, conf);
			vector<int32_t> conf_thresholds, deg_afterburning, deg_normal;

			conf_thresholds = is_high_bw() ? knob::rlp_rp_last_pref_offset_conf_thresholds_hbw : knob::rlp_rp_last_pref_offset_conf_thresholds;
			deg_normal = is_high_bw() ? knob::rlp_rp_dyn_degrees_type2_hbw : knob::rlp_rp_dyn_degrees_type2;

			if(found)
			{
				for(uint32_t index = 0; index < conf_thresholds.size(); ++index)
				{
					/* rlp_last_pref_offset_conf_thresholds is a sorted list in ascending order of values */
					if(conf <= conf_thresholds[index])
					{
						degree = deg_normal[index];
						counted = true;
						break;
					}
				}
				if(!counted)
				{
					degree = deg_normal.back();
				}
			}
			else
			{
				degree = 1;
			}
		}
	}
	return degree;
}

/* This reward fucntion is called after seeing a demand access to the address */
/* TODO: what if multiple prefetch request generated the same address?
 * Currently, it just rewards the oldest prefetch request to the address
 * Should we reward all? */
void RLP::reward(uint64_t address)
{
	MYLOG("addr @ %lx", address);

	stats.reward.demand.called++;
	std::vector<RLP_PQEntry*> pqentries = search_pq(address, knob::rlp_enable_reward_all);

	if(pqentries.empty())
	{
		MYLOG("PQ miss");
		stats.reward.demand.pq_not_found++;
		return;
	}
	else
	{
		stats.reward.demand.pq_found++;
	}

	for(uint32_t index = 0; index < pqentries.size(); ++index)
	{
		
		RLP_PQEntry *pqentry = pqentries[index];

		stats.reward.demand.pq_found_total++;

		if(pqentry->ir_entry){
			MYLOG("PQ hit. ir state %x ir act_idx %lu ir act %lu", 
				  pqentry->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
				  pqentry->action_index, 
				  getIrAction(pqentry->action_index)
			);
				
		}
		else{
			MYLOG("PQ hit. rp state %x rp act_idx %u rp act %d", 
			      pqentry->state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states), 
				  pqentry->action_index, 
				  getRpAction(pqentry->action_index)
			);
		}
		
		/* Do not compute reward if already has a reward.
		 * This can happen when a prefetch access sees multiple demand reuse */
		if(pqentry->has_reward)
		{
			MYLOG("entry already has reward: %d", pqentry->reward);
			stats.reward.demand.has_reward++;
			return;
		}


		if(pqentry->ir_entry){
			if(pqentry->is_filled){
				assign_reward(pqentry, RLPRewardType::rlp_ir_correct_timely);
				MYLOG("assigned ir entry reward correct timely(%d)", pqentry->reward);
			}
			else{
				assign_reward(pqentry, RLPRewardType::rlp_ir_correct_untimely);
				MYLOG("assigned ir entry reward correct untimely(%d)", pqentry->reward);
			}
		}
		else{
			if(pqentry->is_filled){
				assign_reward(pqentry, RLPRewardType::rlp_rp_correct_timely);
				MYLOG("assigned rp entry reward correct timely(%d)", pqentry->reward);
			}
			else{
				assign_reward(pqentry, RLPRewardType::rlp_rp_correct_untimely);
				MYLOG("assigned rp entry reward correct untimely(%d)", pqentry->reward);
			}
		}

		pqentry->has_reward = true;
	}
}

/* This reward function is called during eviction from prefetch_queue */
void RLP::reward(RLP_PQEntry *pqentry)
{

	if(pqentry->ir_entry){
		stats.reward.train.ir_called++;
		MYLOG("reward PQ evict %lx ir state %x ir act_idx %lu ir act %lu", 
		      pqentry->address, 
			  pqentry->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
			  pqentry->action_index, 
			  getIrAction(pqentry->action_index)
		);
	}
	else{
		stats.reward.train.rp_called++;
		MYLOG("reward PQ evict %lx rp state %x rp act_idx %u rp act %d", 
		      pqentry->address, 
			  pqentry->state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states), 
			  pqentry->action_index, 
			  getRpAction(pqentry->action_index)
		);
	}

	assert(!pqentry->has_reward);
	/* this is called during eviction from prefetch tracker
	 * that means, this address doesn't see a demand reuse.
	 * hence it either can be incorrect, or no prefetch */
	if(no_prefetch(pqentry->address)) /* no prefetch */
	{
		if(pqentry->ir_entry){
			assign_reward(pqentry, RLPRewardType::rlp_ir_none);
			MYLOG("assigned ir entry reward no_pref(%d)", pqentry->reward);
		}
		else{
			assign_reward(pqentry, RLPRewardType::rlp_rp_none);
			MYLOG("assigned rp entry reward no_pref(%d)", pqentry->reward);
		}
	}
	else /* incorrect prefetch */
	{	
		if(pqentry->ir_entry){
			assign_reward(pqentry, RLPRewardType::rlp_ir_incorrect);
			MYLOG("assigned ir entry reward incorrect(%d)", pqentry->reward);
		}
		else{
			assign_reward(pqentry, RLPRewardType::rlp_rp_incorrect);
			MYLOG("assigned rp entry reward incorrect(%d)", pqentry->reward);	
		}
	}
	pqentry->has_reward = true;
}

void RLP::assign_reward(RLP_PQEntry *pqentry, RLPRewardType type)
{
	if(pqentry->ir_entry){
		stats.reward.assign_reward.ir_called++;
		MYLOG("assign reward PQ evict %lx ir state %x ir act_idx %lu ir act %lu", 
		      pqentry->address, 
			  pqentry->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
			  pqentry->action_index, 
			  getIrAction(pqentry->action_index)
		);
	}
	else{
		stats.reward.assign_reward.rp_called++;
		MYLOG("assign reward PQ evict %lx rp state %x rp act_idx %u rp act %d", 
		      pqentry->address, 
			  pqentry->state->value(knob::rlp_rp_state_type, knob::rlp_rp_state_hash_type, knob::rlp_rp_max_states), 
			  pqentry->action_index, 
			  getRpAction(pqentry->action_index)
		);
	}
	
	assert(!pqentry->has_reward);

	/* compute the reward */
	int32_t reward = compute_reward(pqentry, type);

	/* assign */
	pqentry->reward = reward;
	pqentry->reward_type = type;
	pqentry->has_reward = true;

	/* maintain stats */
	stats.reward.assign_reward.called++;
	switch(type)
	{
		case RLPRewardType::rlp_none: 						break;
		case RLPRewardType::rlp_ir_none:			     	stats.reward.ir_no_pref++; break;
		case RLPRewardType::rlp_ir_incorrect:				stats.reward.ir_incorrect++; break;
		case RLPRewardType::rlp_ir_correct_timely:			stats.reward.ir_correct_timely++; break;
		case RLPRewardType::rlp_ir_correct_untimely:		stats.reward.ir_correct_untimely++; break;
		case RLPRewardType::rlp_ir_prefetch_queue_hit:		stats.reward.ir_prefetch_queue_hit++; break;
		case RLPRewardType::rlp_rp_none:					stats.reward.rp_no_pref++; break;
		case RLPRewardType::rlp_rp_incorrect:				stats.reward.rp_incorrect++; break;
		case RLPRewardType::rlp_rp_correct_timely:			stats.reward.ir_correct_timely++; break;
		case RLPRewardType::rlp_rp_correct_untimely:		stats.reward.ir_correct_untimely++; break;
		case RLPRewardType::rlp_rp_prefetch_queue_hit: 		stats.reward.rp_prefetch_queue_hit++; break;
		case RLPRewardType::rlp_rp_out_of_bounds: 			stats.reward.rp_out_of_bounds++; break;
		default:							assert(false);
	}

	if(pqentry->ir_entry){
		stats.reward.ir_dist[type][pqentry->action_index]++;
	}
	else{
		stats.reward.rp_dist[pqentry->action_index][type]++;
	}
}

int32_t RLP::compute_reward(RLP_PQEntry *pqentry, RLPRewardType type)
{
	bool high_bw = (knob::rlp_enable_hbw_reward && is_high_bw()) ? true : false;
	float reward = 0;

	stats.reward.compute_reward.dist[type][high_bw]++;

	if(type == RLPRewardType::rlp_none){
		reward =  0;
	}
	else if(type == RLPRewardType::rlp_ir_none)
	{
		reward = high_bw ? knob::rlp_reward_ir_hbw_none : knob::rlp_reward_ir_none;
	}
	else if(type == RLPRewardType::rlp_ir_incorrect){
		reward = high_bw ? knob::rlp_reward_ir_hbw_incorrect : knob::rlp_reward_ir_incorrect;
	}
	else if(type == RLPRewardType::rlp_ir_correct_timely){
		reward = high_bw ? knob::rlp_reward_ir_hbw_correct_timely : knob::rlp_reward_ir_correct_timely;
	}
	else if(type == RLPRewardType::rlp_ir_correct_untimely){
		reward = high_bw ? knob::rlp_reward_ir_hbw_correct_untimely : knob::rlp_reward_ir_correct_untimely;
	}
	else if(type == RLPRewardType::rlp_ir_prefetch_queue_hit){
		reward = high_bw ? knob::rlp_reward_ir_hbw_prefetch_queue_hit : knob::rlp_reward_ir_prefetch_queue_hit; 
	}
	else if(type == RLPRewardType::rlp_rp_none){
		reward = high_bw ? knob::rlp_reward_rp_hbw_none : knob::rlp_reward_rp_none;
	}
	else if(type == RLPRewardType::rlp_rp_incorrect){
		reward = high_bw ? knob::rlp_reward_rp_hbw_incorrect : knob::rlp_reward_rp_incorrect;
	}
	else if(type == RLPRewardType::rlp_rp_correct_timely){
		reward = high_bw ? knob::rlp_reward_rp_hbw_correct_timely : knob::rlp_reward_rp_correct_timely;
	}
	else if(type == RLPRewardType::rlp_rp_correct_untimely){
		reward = high_bw ? knob::rlp_reward_rp_hbw_correct_untimely : knob::rlp_reward_rp_correct_untimely;
	}
	else if(type == RLPRewardType::rlp_rp_prefetch_queue_hit){
		reward = high_bw ? knob::rlp_reward_rp_hbw_prefetch_queue_hit : knob::rlp_reward_rp_prefetch_queue_hit; 
	}
	else if(type == RLPRewardType::rlp_rp_out_of_bounds){
		reward = high_bw ? knob::rlp_reward_rp_hbw_out_of_bounds : knob::rlp_reward_rp_out_of_bounds;
	}
	else
	{
		cout << "Invalid reward type found " << type << endl;
		assert(false);
	}

	return reward;
}

void RLP::ir_train(RLP_PQEntry *ir_curr_evicted, RLP_PQEntry *ir_last_evicted)
{
	MYLOG("ir victim %s %lu %lu ir last_victim %s %lu %lu", 
	        ir_curr_evicted->state->to_string().c_str(), 
			ir_curr_evicted->action_index, 
			getIrAction(ir_curr_evicted->action_index),								
			ir_last_evicted->state->to_string().c_str(), 
			ir_last_evicted->action_index, 
			getIrAction(ir_last_evicted->action_index)
		);

	stats.train.ir_called++;
	if(!ir_last_evicted->has_reward)
	{
		stats.train.ir_compute_reward++;
		reward(ir_last_evicted);
	}
	assert(ir_last_evicted->has_reward);

	/* train */
	MYLOG("===SARSA=== S1: %s A1: %lu R1: %d S2: %s A2: %lu", 
			last_evicted->state->to_string().c_str(), 
			getIrAction(last_evicted->action_index),
			last_evicted->reward,
			curr_evicted->state->to_string().c_str(), 
			getIrAction(curr_evicted->action_index)
		);
	
	ir_brain->learn(ir_last_evicted->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
	                ir_last_evicted->action_index, 
					ir_last_evicted->reward, 
					ir_curr_evicted->state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states), 
					ir_curr_evicted->action_index);
	
	MYLOG("ir engine train done");
}

void RLP::rp_train(RLP_PQEntry *rp_curr_evicted, RLP_PQEntry *rp_last_evicted)
{
	MYLOG("rp victim %s %u %d rp last_victim %s %u %d", 
			rp_curr_evicted->state->to_string().c_str(), 
			rp_curr_evicted->action_index, 
			getRpAction(rp_curr_evicted->action_index),
			RP_last_evicted->state->to_string().c_str(), 
			rp_last_evicted->action_index, 
			getRpAction(rp_last_evicted->action_index)
		);

	stats.train.rp_called++;
	if(!rp_last_evicted->has_reward)
	{
		stats.train.rp_compute_reward++;
		reward(rp_last_evicted);
	}
	assert(rp_last_evicted->has_reward);

	/* train */
	MYLOG("===SARSA=== S1: %s A1: %lu R1: %d S2: %s A2: %lu", 
				rp_last_evicted->state->to_string().c_str(), 
				rp_last_evicted->action_index,
				rp_last_evicted->reward,
				rp_curr_evicted->state->to_string().c_str(), 
				rp_curr_evicted->action_index
			);

	rp_brain->learn(rp_last_evicted->state, 
					rp_last_evicted->action_index, 
					rp_last_evicted->reward, 
					rp_curr_evicted->state, 
					rp_curr_evicted->action_index,
					rp_last_evicted->consensus_vec, 
					rp_last_evicted->reward_type
				);

	MYLOG("rp engine train done");
}

/* TODO: what if multiple prefetch request generated the same address?
 * Currently it just sets the fill bit of the oldest prefetch request.
 * Do we need to set it for everyone? */
void RLP::register_fill(uint64_t address)
{
	MYLOG("fill @ %lx", address);

	stats.register_fill.called++;
	std::vector<RLP_PQEntry*> pqentries = search_pq(address, knob::rlp_enable_reward_all);
	if(!pqentries.empty())
	{
		stats.register_fill.set++;
		for(uint32_t index = 0; index < pqentries.size(); ++index)
		{
			stats.register_fill.set_total++;
			pqentries[index]->is_filled = true;

			if(pqentries[index]->ir_entry){
				stats.register_fill.ir_set_total++;
				MYLOG("fill PT hit. pref with ir act_idx %lu ir act %lu", 
						pqentries[index]->action_index, 
						getIrAction(pqentries[index]->action_index)
					);
			}
			else{
				stats.register_fill.rp_set_total++;
				MYLOG("fill PT hit. pref with rp act_idx %u rp act %d", 
						pqentries[index]->action_index, 
						getRpAction(pqentries[index]->action_index)
					);
			}
		}
	}
}

void RLP::register_prefetch_hit(uint64_t address)
{
	MYLOG("pref_hit @ %lx", address);

	stats.register_prefetch_hit.called++;
	std::vector<RLP_PQEntry*> pqentries = search_pq(address, knob::rlp_enable_reward_all);
	if(!pqentries.empty())
	{
		stats.register_prefetch_hit.set++;
		for(uint32_t index = 0; index < pqentries.size(); ++index)
		{
			stats.register_prefetch_hit.set_total++;
			pqentries[index]->pf_cache_hit = true;

			if(pqentries[index]->ir_entry){
				stats.register_prefetch_hit.ir_set_total++;
				MYLOG("fill PT hit. pref with ir act_idx %lu ir act %lu", 
						pqentries[index]->action_index, 
						getIrAction(pqentries[index]->action_index)
					);
			}
			else{
				stats.register_prefetch_hit.rp_set_total++;
				MYLOG("fill PT hit. pref with rp act_idx %u rp act %d", 
						pqentries[index]->action_index, 
						getRpAction(pqentries[index]->action_index)
					);
			}
			
		}
	}
}

std::vector<RLP_PQEntry*> RLP::search_pq(uint64_t address, bool search_all)
{
	std::vector<RLP_PQEntry*> entries;
	for(uint32_t index = 0; index < prefetch_queue.size(); ++index)
	{
		if(prefetch_queue[index]->address == address)
		{
			entries.emplace_back(prefetch_queue[index]);
			if(!search_all) break;
		}
	}
	return entries;
}

std::vector<RLP_PQEntry*> RLP::ir_search_pq(uint64_t address, bool search_all)
{
	std::vector<RLP_PQEntry*> entries;
	for(uint32_t index = 0; index < prefetch_queue.size(); ++index)
	{
		if(prefetch_queue[index]->address == address && prefetch_queue[index]->ir_entry)
		{
			entries.emplace_back(prefetch_queue[index]);
			if(!search_all) break;
		}
	}
	return entries;
}

std::vector<RLP_PQEntry*> RLP::rp_search_pq(uint64_t address, bool search_all)
{
	std::vector<RLP_PQEntry*> entries;
	for(uint32_t index = 0; index < prefetch_queue.size(); ++index)
	{
		if(prefetch_queue[index]->address == address && !prefetch_queue[index]->ir_entry)
		{
			entries.emplace_back(prefetch_queue[index]);
			if(!search_all) break;
		}
	}
	return entries;
}

void RLP::ir_update_stats(uint32_t state, uint64_t action_index, uint32_t pref_degree)
{
	
	ir_state_action_dist[state][action_index]++;

	auto it2 = ir_action_deg_dist.find(getIrAction(action_index));
	if(it2 != ir_action_deg_dist.end())
	{
		it2->second[pref_degree]++;
	}
	else
	{
		ir_action_deg_dist[action_index].resize(RLP_MAX_DEGREE, 0);
		ir_action_deg_dist[action_index][pref_degree]++;
	}
	
}

void RLP::ir_update_stats(State *state, uint64_t action_index, uint32_t degree)
{
	string state_str = state->to_string();
	ir_state_action_dist2[state_str][action_index]++;

	auto it2 = ir_action_deg_dist.find(getIrAction(action_index));
	if(it2 != ir_action_deg_dist.end())
	{
		it2->second[degree]++;
	}
	else
	{
		ir_action_deg_dist[action_index].resize(RLP_MAX_DEGREE, 0);
		ir_action_deg_dist[action_index][degree]++;
	}
}

void RLP::rp_update_stats(uint32_t state, uint32_t action_index, uint32_t pref_degree)
{
	
	auto it = rp_state_action_dist.find(state);
	if(it != rp_state_action_dist.end())
	{
		it->second[action_index]++;
	}
	else
	{
		rp_state_action_dist[state].resize(knob::rlp_rp_max_actions, 0);
		rp_state_action_dist[state][action_index]++;
	}
	
}

void RLP::rp_update_stats(State *state, uint32_t action_index, uint32_t degree)
{
	string state_str = state->to_string();
	auto it = rp_state_action_dist2.find(state_str);
	if(it != rp_state_action_dist2.end())
	{
		it->second[action_index]++;
		it->second[knob::rlp_rp_max_actions]++; /* counts total occurences of this state */
	}
	else
	{
		rp_state_action_dist2[state_str].resize(knob::rlp_rp_max_actions + 1, 0);
		rp_state_action_dist2[state_str][action_index]++;
		rp_state_action_dist2[state_str][knob::rlp_rp_max_actions]++; /* counts total occurences of this state */
	}

	auto it2 = rp_action_deg_dist.find(getRpAction(action_index));
	if(it2 != rp_action_deg_dist.end())
	{
		it2->second[degree]++;
	}
	else
	{
		rp_action_deg_dist[action_index].resize(RLP_MAX_DEGREE, 0);
		rp_action_deg_dist[action_index][degree]++;
	}
}

uint64_t RLP::getIrAction(uint64_t action_index)
{
	return action_index;
}

int32_t RLP::getRpAction(uint32_t action_index)
{
	assert(action_index < RLP_RP_Actions.size());
	return RLP_RP_Actions[action_index];
}

void RLP::track_in_hsq(State * state){
	/*build history state -> cur addr action*/
	for(auto state_index : history_state_table){
		ir_brain->insertAction(state_index, state->cacheline);
	}
	if(history_state_table.size() >= knob::rlp_history_state_queue_size){
		history_state_table.pop_front();
	}
	history_state_table.push_back(state->value(knob::rlp_ir_state_type, knob::rlp_ir_state_hash_type, knob::rlp_ir_max_states));
}

void RLP::track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset)
{
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](RLP_STEntry *stentry){return stentry->page == page;});
	if(st_index != signature_table.end())
	{
		(*st_index)->track_prefetch(pred_offset, pref_offset);
	}
}

void RLP::update_bw(uint8_t bw)
{
	assert(bw < DRAM_BW_LEVELS);
	bw_level = bw;
	stats.bandwidth.epochs++;
	stats.bandwidth.histogram[bw_level]++;
}

void RLP::update_ipc(uint8_t ipc)
{
	assert(ipc < RLP_MAX_IPC_LEVEL);
	core_ipc = ipc;
	stats.ipc.epochs++;
	stats.ipc.histogram[ipc]++;
}

void RLP::update_acc(uint32_t acc)
{
	assert(acc < CACHE_ACC_LEVELS);
	acc_level = acc;
	stats.cache_acc.epochs++;
	stats.cache_acc.histogram[acc]++;
}

bool RLP::is_high_bw()
{
	return bw_level >= knob::rlp_high_bw_thresh ? true : false;
}

void RLP::dump_stats()
{
	cout << "rlp_st_lookup " << stats.st.lookup << endl
		<< "rlp_st_hit " << stats.st.hit << endl
		<< "rlp_st_evict " << stats.st.evict << endl
		<< "rlp_st_insert " << stats.st.insert << endl
		<< "rlp_st_streaming " << stats.st.streaming << endl
		<< endl

		<< "rlp_predict_called " << stats.predict.called << endl
		<< "rlp_predict_ir_called " << stats.predict.ir_called << endl
		<< "rlp_predict_rp_called " << stats.predict.rp_called << endl		
		<< "rlp_predict_rp_out_of_bounds " << stats.predict.rp_out_of_bounds << endl;

	for(uint32_t index = 0; index < RLP_RP_Actions.size(); ++index)
	{
		cout << "rlp_predict_rp_action_" << RLP_RP_Actions[index] << " " << stats.predict.rp_action_dist[index] << endl;
		cout << "rlp_predict_rp_issue_action_" << RLP_RP_Actions[index] << " " << stats.predict.rp_issue_dist[index] << endl;
		cout << "rlp_predict_rp_hit_action_" << RLP_RP_Actions[index] << " " << stats.predict.rp_pred_hit[index] << endl;
		cout << "rlp_predict_rp_out_of_bounds_action_" << RLP_RP_Actions[index] << " " << stats.predict.rp_out_of_bounds_dist[index] << endl;
	}

	cout << "rlp_predict_predicted " << stats.predict.predicted << endl
		<< "rlp_predict_ir_predicted " << stats.predict.ir_predicted << endl
		<< "rlp_predict_rp_predicted " << stats.predict.rp_predicted << endl
		<< "rlp_predict_ir_multi_deg_called " << stats.predict.ir_multi_deg_called << endl
		<< "rlp_predict_rp_multi_deg_called " << stats.predict.rp_multi_deg_called << endl
		<< "rlp_predict_multi_deg " << stats.predict.multi_deg << endl
		<< "rlp_predict_ir_multi_deg " << stats.predict.ir_multi_deg << endl
		<< "rlp_predict_rp_multi_deg " << stats.predict.rp_multi_deg << endl;

	for(uint32_t index = 2; index <= RLP_MAX_DEGREE; ++index)
	{
		cout << "rlp_predict_ir_multi_deg_" << index << " " << stats.predict.ir_multi_deg_histogram[index] << endl;
	}
	cout << endl;
	for(uint32_t index = 1; index <= RLP_MAX_DEGREE; ++index)
	{
		cout << "rlp_ir_selected_deg_" << index << " " << stats.predict.ir_deg_histogram[index] << endl;
	}
	cout << endl;

	for(uint32_t index = 2; index <= RLP_MAX_DEGREE; ++index)
	{
		cout << "rlp_predict_rp_multi_deg_" << index << " " << stats.predict.rp_multi_deg_histogram[index] << endl;
	}
	cout << endl;
	for(uint32_t index = 1; index <= RLP_MAX_DEGREE; ++index)
	{
		cout << "rlp_rp_selected_deg_" << index << " " << stats.predict.rp_deg_histogram[index] << endl;
	}
	cout << endl;

	if(knob::rlp_debug_enable_state_action_stats)
	{

		//print top-K state-action pair
		cout << "print top-K ir state-action pair" << endl;
		priority_queue<pair<uint32_t, uint32_t>, vector<pair<uint32_t, uint32_t>>, 
		greater<pair<uint32_t, uint32_t>>> topk_state_action_q;

		for(auto [state, action_dist] : ir_state_action_dist){
			uint32_t st = state;
			uint32_t cnt = 0;
			for(auto [action, action_cnt] : action_dist){
				cnt += action_cnt;
			}

			topk_state_action_q.emplace(cnt, st);
			if(topk_state_action_q.size() > knob::rlp_debug_state_action_stats_topK)
				topk_state_action_q.pop();
		}

		while(!topk_state_action_q.empty()){
			uint32_t cnt = topk_state_action_q.top().first;
			uint32_t state = topk_state_action_q.top().second;
			topk_state_action_q.pop();

			priority_queue<pair<uint32_t, uint64_t>, vector<pair<uint32_t, uint64_t>>, 
			greater<pair<uint32_t, uint64_t>>> topk_action_q;

			for(auto [action, action_cnt] : ir_state_action_dist[state]){
				topk_action_q.emplace(action_cnt, action);
				if(topk_action_q.size() > knob::rlp_debug_state_action_stats_topK)
					topk_action_q.pop();
			}

			cout << "rlp_ir_state_" << hex << state << dec << " " << cnt << ", top-K action: ";
			while(!topk_action_q.empty()){
				uint32_t action_cnt = topk_action_q.top().first;
				uint64_t action = topk_action_q.top().second;
				cout << hex << action << dec << ":" << action_cnt << ",";
			}
			cout  << endl;
		}

		cout << "print rp state-action pair" << endl;
		std::vector<std::pair<string, vector<uint64_t> > > pairs;
		for (auto itr = rp_state_action_dist2.begin(); itr != rp_state_action_dist2.end(); ++itr)
			pairs.push_back(*itr);
		sort(pairs.begin(), pairs.end(), [](std::pair<string, vector<uint64_t>>& a, std::pair<string, vector<uint64_t>>& b){
			return a.second[knob::rlp_rp_max_actions] > b.second[knob::rlp_rp_max_actions];
			});
		for(auto it = pairs.begin(); it != pairs.end(); ++it)
		{
			cout << "rlp_rp_state_" << hex << it->first << dec << " ";
			for(uint32_t index = 0; index < it->second.size(); ++index)
			{
				cout << it->second[index] << ",";
			}
			cout << endl;
		}

		for(auto it = ir_action_deg_dist.begin(); it != ir_action_deg_dist.end(); ++it)
		{
			cout << "rlp_ir_action_" << it->first << "_deg_dist ";
			for(uint32_t index = 0; index < RLP_MAX_DEGREE; ++index)
			{
				cout << it->second[index] << ",";
			}
			cout << endl;
		}
		cout << endl;
		for(auto it = rp_action_deg_dist.begin(); it != rp_action_deg_dist.end(); ++it)
		{
			cout << "rlp_rp_action_" << it->first << "_deg_dist ";
			for(uint32_t index = 0; index < RLP_MAX_DEGREE; ++index)
			{
				cout << it->second[index] << ",";
			}
			cout << endl;
		}
	}
	cout << endl;

	cout << "rlp_ir_track_called " << stats.track.ir_called << endl
		<< "rlp_rp_track_called " << stats.track.rp_called << endl
		<< "rlp_ir_track_same_address " << stats.track.ir_same_address << endl
		<< "rlp_rp_track_same_address " << stats.track.rp_same_address << endl
		<< "rlp_ir_track_evict " << stats.track.ir_evict << endl
		<< "rlp_rp_track_evict " << stats.track.rp_evict << endl
		<< endl
		<< "rlp_reward_demand_called " << stats.reward.demand.called << endl
		<< "rlp_reward_demand_pq_not_found " << stats.reward.demand.pq_not_found << endl
		<< "rlp_reward_demand_pq_found " << stats.reward.demand.pq_found << endl
		<< "rlp_reward_demand_pq_found_total " << stats.reward.demand.pq_found_total << endl
		<< "rlp_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
		<< "rlp_reward_train_ir_called " << stats.reward.train.ir_called << endl
		<< "rlp_reward_train_rp_called " << stats.reward.train.rp_called << endl
		<< "rlp_reward_assign_reward_called " << stats.reward.assign_reward.called << endl
		<< "rlp_reward_assign_reward_ir_called " << stats.reward.assign_reward.ir_called << endl
		<< "rlp_reward_assign_reward_rp_called " << stats.reward.assign_reward.rp_called << endl
		<< "rlp_reward_ir_no_pref " << stats.reward.ir_no_pref << endl
		<< "rlp_reward_ir_incorrect " << stats.reward.ir_incorrect << endl
		<< "rlp_reward_ir_correct_timely " << stats.reward.ir_correct_timely << endl
		<< "rlp_reward_ir_correct_untimely " << stats.reward.ir_correct_untimely << endl
		<< "rlp_reward_ir_prefetch_queue_hit " << stats.reward.ir_prefetch_queue_hit << endl
		<< "rlp_reward_rp_no_pref " << stats.reward.rp_no_pref << endl
		<< "rlp_reward_rp_incorrect " << stats.reward.rp_incorrect << endl
		<< "rlp_reward_rp_correct_timely " << stats.reward.rp_correct_timely << endl
		<< "rlp_reward_rp_correct_untimely " << stats.reward.rp_correct_untimely << endl
		<< "rlp_reward_rp_prefetch_queue_hit " << stats.reward.rp_prefetch_queue_hit << endl
		<< "rlp_reward_rp_out_of_bounds " << stats.reward.rp_out_of_bounds << endl
		<< endl;


	for(uint32_t reward = 1; reward < RLPRewardType::rlp_num_rewards; ++reward)
	{
		cout << "rlp_reward_" << RLPgetRewardTypeString((RLPRewardType)reward) << "_low_bw " << stats.reward.compute_reward.dist[reward - 1][0] << endl
			<< "rlp_reward_" << RLPgetRewardTypeString((RLPRewardType)reward) << "_high_bw " << stats.reward.compute_reward.dist[reward - 1][1] << endl;
	}
	cout << endl;

	for(uint32_t action = 0; action < RLP_RP_Actions.size(); ++action)
	{
		cout << "rlp_rp_reward_" << RLP_RP_Actions[action] << " ";
		for(uint32_t reward = 0; reward < RLPRewardType::rlp_num_rewards; ++reward)
		{
			cout << stats.reward.rp_dist[action][reward] << ",";
		}
		cout << endl;
	}

	cout << endl
		<< "rlp_train_ir_called " << stats.train.ir_called << endl
		<< "rlp_train_rp_called " << stats.train.rp_called << endl
		<< "rlp_train_ir_compute_reward " << stats.train.ir_compute_reward << endl
		<< "rlp_train_rp_compute_reward " << stats.train.rp_compute_reward << endl
		<< endl
		<< "rlp_register_fill_called " << stats.register_fill.called << endl
		<< "rlp_register_fill_set " << stats.register_fill.set << endl
		<< "rlp_register_fill_ir_set_total " << stats.register_fill.ir_set_total << endl
		<< "rlp_register_fill_rp_set_total " << stats.register_fill.rp_set_total << endl
		<< "rlp_register_fill_set_total " << stats.register_fill.set_total << endl
		<< endl
		<< "rlp_register_prefetch_hit_called " << stats.register_prefetch_hit.called << endl
		<< "rlp_register_prefetch_hit_set " << stats.register_prefetch_hit.set << endl		
		<< "rlp_register_prefetch_hit_ir_set_total " << stats.register_prefetch_hit.ir_set_total << endl
		<< "rlp_register_prefetch_hit_rp_set_total " << stats.register_prefetch_hit.rp_set_total << endl
		<< "rlp_register_prefetch_hit_set_total " << stats.register_prefetch_hit.set_total << endl
		<< endl
		<< "rlp_pref_issue_rlp " << stats.pref_issue.rlp << endl
		<< "rlp_pref_issue_rlp_ir " << stats.pref_issue.rlp_ir << endl
		<< "rlp_pref_issue_rlp_rp " << stats.pref_issue.rlp_rp << endl
		<< endl;

	cout << "rlp_bw_epochs " << stats.bandwidth.epochs << endl;
	for(uint32_t index = 0; index < DRAM_BW_LEVELS; ++index)
	{
		cout << "rlp_bw_level_" << index << " " << stats.bandwidth.histogram[index] << endl;
	}
	cout << endl;

	cout << "rlp_ipc_epochs " << stats.ipc.epochs << endl;
	for(uint32_t index = 0; index < RLP_MAX_IPC_LEVEL; ++index)
	{
		cout << "rlp_ipc_level_" << index << " " << stats.ipc.histogram[index] << endl;
	}
	cout << endl;

	cout << "rlp_cache_acc_epochs " << stats.cache_acc.epochs << endl;
	for(uint32_t index = 0; index < CACHE_ACC_LEVELS; ++index)
	{
		cout << "rlp_cache_acc_level_" << index << " " << stats.cache_acc.histogram[index] << endl;
	}
	cout << endl;

	ir_brain->dump_stats();
	rp_brain->dump_stats();
	recorder->dump_stats();
}
