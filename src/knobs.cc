#include <iostream>
#include <string>
#include <string.h>
#include <math.h>
#include "knobs.h"
#include "ini.h"
using namespace std;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

namespace knob
{
	uint64_t warmup_instructions = 1000000;
	uint64_t simulation_instructions = 1000000;
	bool  	 knob_cloudsuite = false;
	bool     knob_low_bandwidth = false;
	vector<string> 	 l2c_prefetcher_types;
	vector<string> 	 l1d_prefetcher_types;
	bool     l1d_perfect = false;
	bool     l2c_perfect = false;
	bool     llc_perfect = false;
	bool     l1d_semi_perfect = false;
	bool     l2c_semi_perfect = false;
	bool     llc_semi_perfect = false;
	uint32_t semi_perfect_cache_page_buffer_size = 64;
	bool     measure_ipc = false;
	uint64_t measure_ipc_epoch = 1000;
	uint32_t dram_io_freq = 2400;
	bool     measure_dram_bw = true;
	uint64_t measure_dram_bw_epoch = 256;
	bool     measure_cache_acc = true;
	uint64_t measure_cache_acc_epoch = 1024;

	/* next-line */
	vector<int32_t>  next_line_deltas;
	vector<float>  next_line_delta_prob;
	uint32_t next_line_seed = 255;
	uint32_t next_line_pt_size = 256;
	bool     next_line_enable_prefetch_tracking = true;
	bool     next_line_enable_trace = false;
	uint32_t next_line_trace_interval = 5;
	string   next_line_trace_name = string("next_line_trace.csv");
	uint32_t next_line_pref_degree = 1;

	/* SMS */
	uint32_t sms_at_size = 32;
	uint32_t sms_ft_size = 64;
	uint32_t sms_pht_size = 16384;
	uint32_t sms_pht_assoc = 16;
	uint32_t sms_pref_degree = 4;
	uint32_t sms_region_size = 2048;
	uint32_t sms_region_size_log = 11;
	bool     sms_enable_pref_buffer = true;
	uint32_t sms_pref_buffer_size = 256;

	/* SPP */
	uint32_t spp_st_size = 256;
	uint32_t spp_pt_size = 512;
	uint32_t spp_max_outcomes = 4;
	double   spp_max_confidence = 25.0;
	uint32_t spp_max_depth = 64;
	uint32_t spp_max_prefetch_per_level = 1;
	uint32_t spp_max_confidence_counter_value = 16;
	uint32_t spp_max_global_counter_value = 1024;
	uint32_t spp_pf_size = 1024;
	bool     spp_enable_alpha = true;
	bool     spp_enable_pref_buffer = true;
	uint32_t spp_pref_buffer_size = 256;
	uint32_t spp_pref_degree = 4;
	bool     spp_enable_ghr = true;
	uint32_t spp_ghr_size = 8;
	uint32_t spp_signature_bits = 12;
	uint32_t spp_alpha_epoch = 1024;

	/* SPP_dev2 */
	uint32_t spp_dev2_fill_threshold = 90;
	uint32_t spp_dev2_pf_threshold = 25;

	/* BOP */
	vector<int32_t> bop_candidates;
	uint32_t bop_max_rounds = 100;
	uint32_t bop_max_score = 31;
	uint32_t bop_top_n = 1;
	bool     bop_enable_pref_buffer = false;
	uint32_t bop_pref_buffer_size = 256;
	uint32_t bop_pref_degree = 4;
	uint32_t bop_rr_size = 256;

	/* Sandbox */
	uint32_t sandbox_pref_degree = 4;
	bool     sandbox_enable_stream_detect = false;
	uint32_t sandbox_stream_detect_length = 4;
	uint32_t sandbox_num_access_in_phase = 256;
	uint32_t sandbox_num_cycle_offsets = 4;
	uint32_t sandbox_bloom_filter_size = 2048;
	uint32_t sandbox_seed = 200;

	/* DSPatch */
	uint32_t dspatch_log2_region_size;
	uint32_t dspatch_num_cachelines_in_region;
	uint32_t dspatch_pb_size;
	uint32_t dspatch_num_spt_entries;
	uint32_t dspatch_compression_granularity;
	uint32_t dspatch_pred_throttle_bw_thr;
	uint32_t dspatch_bitmap_selection_policy;
	uint32_t dspatch_sig_type;
	uint32_t dspatch_sig_hash_type;
	uint32_t dspatch_or_count_max;
	uint32_t dspatch_measure_covP_max;
	uint32_t dspatch_measure_accP_max;
	uint32_t dspatch_acc_thr;
	uint32_t dspatch_cov_thr;
	bool     dspatch_enable_pref_buffer;
	uint32_t dspatch_pref_buffer_size;
	uint32_t dspatch_pref_degree;

	/* PPF */
	int32_t ppf_perc_threshold_hi = -5;
	int32_t ppf_perc_threshold_lo = -15;

	/* MLOP */
	uint32_t mlop_pref_degree;
	uint32_t mlop_num_updates;
	float 	mlop_l1d_thresh;
	float 	mlop_l2c_thresh;
	float 	mlop_llc_thresh;
	uint32_t	mlop_debug_level;

	/* Bingo */
	uint32_t bingo_region_size = 2048;
	uint32_t bingo_pattern_len = 32;
	uint32_t bingo_pc_width = 16;
	uint32_t bingo_min_addr_width = 5;
	uint32_t bingo_max_addr_width = 16;
	uint32_t bingo_ft_size = 64;
	uint32_t bingo_at_size = 128;
	uint32_t bingo_pht_size = 8192;
	uint32_t bingo_pht_ways = 16;
	uint32_t bingo_pf_streamer_size = 128;
	uint32_t bingo_debug_level = 0;
	float    bingo_l1d_thresh;
	float    bingo_l2c_thresh;
	float    bingo_llc_thresh;
	string   bingo_pc_address_fill_level;

	/* Stride */
	uint32_t stride_num_trackers = 64;
   	uint32_t stride_pref_degree = 2;

	/* GnnPrefetcher */
	uint32_t RegTableEntry_num = 512;
	uint32_t reg_pref_degree = 2;

	/* Streamer */
	uint32_t streamer_num_trackers = 64;
	uint32_t streamer_pref_degree = 5; /* models IBM POWER7 */

	/* AMPM */
	uint32_t ampm_pb_size = 64;
	uint32_t ampm_pred_degree = 4;
	uint32_t ampm_pref_degree = 4;
	uint32_t ampm_pref_buffer_size = 256;
	bool	 ampm_enable_pref_buffer = false;
	uint32_t ampm_max_delta = 16;

	/* Context Prefetcher */
	uint32_t cp_cst_size = 2048;
	uint32_t cp_cst_assoc = 16;
	uint32_t cp_max_response_per_cst = 4;
	int32_t  cp_init_reward = 0;
	uint32_t cp_prefetch_queue_size = 128;

	/* IBM POWER7 Prefetcher */
	uint32_t power7_explore_epoch = 1000;
	uint32_t power7_exploit_epoch = 100000;
	uint32_t power7_default_streamer_degree = 4;

	/*T2 Prefetcher */

	uint32_t t2_non_loop_table_size = 20;
	uint32_t t2_min_seg_size = 2;
	uint32_t t2_loop_branch_register_size = 20;

	uint32_t t2_stride_identifier_table_size = 32;
	uint32_t t2_perf_degree = 1; /* default is set 1*/

	/*TPC Prefetcher */

	uint32_t tpc_t2_non_loop_table_size = 32;
	uint32_t tpc_t2_min_seg_size = 2;
	uint32_t tpc_t2_loop_branch_register_size = 512;
	uint32_t tpc_t2p1_stride_identifier_table_size = 512;
	uint32_t tpc_p1_tain_reg_range = 4096;
	uint32_t tpc_t2_perf_degree = 2; /* default is set 2*/
	uint32_t tpc_p1_perf_degree = 2; /* default is set 2*/
	
	uint32_t tpc_c1_total_region_threshold = 4;
	uint32_t tpc_c1_dense_region_threshold = 6;
	uint32_t tpc_c1_instruction_monitor_size = 512;
	uint32_t tpc_c1_region_monitor_size = 512;
	uint32_t tpc_c1_perf_degree = 4; /* default is set 4*/

	bool tpc_enable = false;	

	/* Scooby */
	float    scooby_alpha = 0.0065;
	float    scooby_gamma = 0.55;
	float    scooby_epsilon = 0.002;
	uint32_t scooby_state_num_bits = 10;
	uint32_t scooby_max_states = 1024;
	uint32_t scooby_seed = 200;
	string   scooby_policy = std::string("EGreedy");
	string   scooby_learning_type = std::string("SARSA");
	vector<int32_t> scooby_actions;
	uint32_t scooby_max_actions = 64;
	uint32_t scooby_pt_size = 256;
	uint32_t scooby_st_size = 64;
	int32_t  scooby_reward_none = -4;
	int32_t  scooby_reward_incorrect = -8;
	int32_t  scooby_reward_correct_untimely = 12;
	int32_t  scooby_reward_correct_timely = 20;
	uint32_t scooby_max_pcs = 5;
	uint32_t scooby_max_offsets = 5;
	uint32_t scooby_max_deltas = 5;
	bool     scooby_brain_zero_init = false;
	bool     scooby_enable_reward_all = false;
	bool     scooby_enable_track_multiple = false;
	bool     scooby_enable_reward_out_of_bounds = true;
	int32_t  scooby_reward_out_of_bounds = -12;
	uint32_t scooby_state_type = 1;
	bool     scooby_access_debug = false;
	bool     scooby_print_access_debug = false;
	uint64_t scooby_print_access_debug_pc = 0;
	uint32_t scooby_print_access_debug_pc_count = 0;
	bool     scooby_print_trace = false;
	bool     scooby_enable_state_action_stats = false;
	bool     scooby_enable_reward_tracker_hit = false;
	int32_t  scooby_reward_tracker_hit = -2;
	bool     scooby_enable_shaggy = false;
	uint32_t scooby_state_hash_type = 11;
	bool     scooby_prefetch_with_shaggy = false;
	bool     scooby_enable_featurewise_engine = true;
	uint32_t scooby_pref_degree = 1; /* default is set to 1 */
	bool     scooby_enable_dyn_degree = true;
	vector<float> scooby_max_to_avg_q_thresholds; /* depricated */
	vector<int32_t> scooby_dyn_degrees;
	uint64_t scooby_early_exploration_window = 0;
	uint32_t scooby_multi_deg_select_type = 2; /* type 1 is already depricated */
	vector<int32_t> scooby_last_pref_offset_conf_thresholds;
	vector<int32_t> scooby_dyn_degrees_type2;
	uint32_t scooby_action_tracker_size = 2;
	uint32_t scooby_high_bw_thresh = 4;
	bool     scooby_enable_hbw_reward = true;
	int32_t  scooby_reward_hbw_correct_timely = 20;
	int32_t  scooby_reward_hbw_correct_untimely = 12;
	int32_t  scooby_reward_hbw_incorrect = -14;
	int32_t  scooby_reward_hbw_none = -2;
	int32_t  scooby_reward_hbw_out_of_bounds = -12;
	int32_t  scooby_reward_hbw_tracker_hit = -2;
	vector<int32_t> scooby_last_pref_offset_conf_thresholds_hbw;
	vector<int32_t> scooby_dyn_degrees_type2_hbw;

	/* Learning Engine */
	bool     le_enable_trace;
	uint32_t le_trace_interval;
	string   le_trace_file_name;
	uint32_t le_trace_state;
	bool     le_enable_score_plot;
	vector<int32_t> le_plot_actions;
	string   le_plot_file_name;
	bool     le_enable_action_trace;
	uint32_t le_action_trace_interval;
	std::string le_action_trace_name;
	bool     le_enable_action_plot;

	/* Featurewise Learning Engine */
	vector<int32_t> le_featurewise_active_features;
	vector<int32_t> le_featurewise_num_tilings;
	vector<int32_t> le_featurewise_num_tiles;
	vector<int32_t> le_featurewise_hash_types;
	vector<int32_t> le_featurewise_enable_tiling_offset;
	float    le_featurewise_max_q_thresh = 0.5;
	bool     le_featurewise_enable_action_fallback = true;
	vector<float> le_featurewise_feature_weights;
	bool     le_featurewise_enable_dynamic_weight = false;
	float    le_featurewise_weight_gradient = 0.001;
	bool     le_featurewise_disable_adjust_weight_all_features_align = false;
	bool     le_featurewise_selective_update = false;
	uint32_t le_featurewise_pooling_type = 2; /* max pooling */
	bool     le_featurewise_enable_dyn_action_fallback = true;
	uint32_t le_featurewise_bw_acc_check_level = 1;
	uint32_t le_featurewise_acc_thresh = 2;

	bool 			le_featurewise_enable_trace = false;
	uint32_t		le_featurewise_trace_feature_type;
	string 			le_featurewise_trace_feature;
	uint32_t 		le_featurewise_trace_interval;
	uint32_t 		le_featurewise_trace_record_count;
	std::string 	le_featurewise_trace_file_name;
	bool 			le_featurewise_enable_score_plot;
	vector<int32_t> le_featurewise_plot_actions;
	std::string 	le_featurewise_plot_file_name;
	bool 			le_featurewise_remove_plot_script;

	/*Cbp*/
	/*init*/
	float           cbp_alpha = 0.0065;
	float           cbp_gamma = 0.55;
	uint32_t        cbp_max_states = 1024;
	std::string     cbp_policy = std::string("Directly");
	std::string     cbp_learning_type = std::string("SARSA");
	uint64_t        cbp_early_exploration_window = 0;

	uint32_t        cbp_pt_size = 256;
	uint32_t        cbp_st_size = 64;
	uint32_t        cbp_hst_size = 256;
	uint32_t        cbp_action_tracker_size = 2;
	bool		    cbp_brain_zero_init = false;
	/*reward*/
	bool            cbp_enable_reward_all = false;
	bool            cbp_enable_hbw_reward = true;
	int32_t         cbp_reward_none = -1;
	int32_t         cbp_reward_hbw_none = -2;
	int32_t         cbp_reward_incorrect = -4;
	int32_t         cbp_reward_hbw_incorrect = -7;
	/*module*/
	uint32_t        cbp_state_type = 7; /*state*/
	uint32_t        cbp_state_hash_type = 11;
	/*feature*/
	uint32_t        cbp_pref_degree = 1; /*dynamic pref degree*/
	uint32_t        cbp_high_bw_thresh = 4;
	/*debug*/
	uint32_t        cbp_max_pcs = 5;
	uint32_t        cbp_max_offsets = 5;
	uint32_t        cbp_max_deltas = 5;
	bool            cbp_enable_state_action_stats = true;
	uint32_t        cbp_state_action_stats_topK = 10;
	bool            cbp_enable_track_multiple = false;

	bool            cbp_access_debug = false;
	bool            cbp_print_access_debug = false;
	uint64_t        cbp_print_access_debug_pc = 0;
	uint32_t        cbp_print_access_debug_pc_count = 0;
	bool            cbp_print_trace = false;

	/*Cbp Learning Engine*/
	bool            cbp_le_enable_trace = false;
	uint32_t        cbp_le_trace_interval = 5;
	string          cbp_le_trace_file_name = std::string("cbp_trace_file.txt");
	uint32_t        cbp_le_trace_state;
	bool            cbp_le_enable_score_plot = false;
	//vector<int32_t> cbp_le_plot_actions;
	string          cbp_le_plot_file_name = std::string("cbp_score_plot.png");
	bool            cbp_le_enable_action_trace = false;
	uint32_t        cbp_le_action_trace_interval = 5;
	std::string     cbp_le_action_trace_name = std::string("cbp_action_trace_file.txt");
	bool            cbp_le_enable_action_plot = false;



	/*RLPrefetcher: RLP*/
	/*init*/
	float            rlp_ir_alpha = 0.0065;
	float            rlp_rp_alpha = 0.0065;
	float            rlp_ir_gamma = 0.55;
	float            rlp_rp_gamma = 0.55;
	float            rlp_epsilon = 0.5;
	float            rlp_rp_epsilon = 0.002;
	uint32_t         rlp_ir_max_states = 1024;
	uint32_t         rlp_rp_max_states = 1024;
	vector<int32_t>  rlp_rp_actions;
	uint32_t         rlp_ir_max_actions = 1024;
	uint32_t         rlp_rp_max_actions = 128; //delta in [-64, 63]
	uint32_t         rlp_seed = 123;
	uint32_t         rlp_rp_seed = 199;
	string           rlp_ir_policy = std::string("Directly");
	string           rlp_rp_policy = std::string("EGreedy");
	string           rlp_ir_learning_type = std::string("SARSA");
	string           rlp_rp_learning_type = std::string("SARSA");
	bool		     rlp_ir_brain_zero_init = false;
	bool		     rlp_rp_brain_zero_init = false;
	uint64_t         rlp_ir_early_exploration_window = 64;
	//uint64_t         rlp_rp_early_exploration_window;

	/*reward*/
	bool             rlp_enable_hbw_reward = true;
	bool             rlp_enable_reward_all = false;
	bool             rlp_enable_reward_ir_prefetch_queue_hit = false;
	bool             rlp_enable_reward_rp_prefetch_queue_hit = false;
	bool             rlp_enable_reward_rp_out_of_bounds = false;
	int32_t          rlp_reward_ir_none = -8;
	int32_t          rlp_reward_rp_none = -4;
	int32_t          rlp_reward_ir_incorrect = -12;
	int32_t          rlp_reward_rp_incorrect = -8;
	int32_t          rlp_reward_ir_correct_timely = 16;
	int32_t          rlp_reward_rp_correct_timely = 20;
	int32_t          rlp_reward_ir_correct_untimely = 8;
	int32_t          rlp_reward_rp_correct_untimely = 12;
	int32_t          rlp_reward_ir_prefetch_queue_hit = -4;
	int32_t          rlp_reward_rp_prefetch_queue_hit = -2;
	int32_t          rlp_reward_rp_out_of_bounds;
	int32_t          rlp_reward_ir_hbw_none = -4;
	int32_t          rlp_reward_rp_hbw_none = -2;
	int32_t          rlp_reward_ir_hbw_incorrect = -18;
	int32_t          rlp_reward_rp_hbw_incorrect = -14;
	int32_t          rlp_reward_ir_hbw_correct_timely = 16;
	int32_t          rlp_reward_rp_hbw_correct_timely = 20;
	int32_t          rlp_reward_ir_hbw_correct_untimely = 8;
	int32_t          rlp_reward_rp_hbw_correct_untimely = 12;
	int32_t          rlp_reward_ir_hbw_prefetch_queue_hit = -4;
	int32_t          rlp_reward_rp_hbw_prefetch_queue_hit = -2;
	int32_t          rlp_reward_rp_hbw_out_of_bounds = -12;

	/*module*/
	uint32_t         rlp_prefetch_queue_size = 256;
	uint32_t         rlp_signature_table_size = 64;
	uint32_t         rlp_history_state_queue_size = 256;
	uint32_t         rlp_action_tracker_size = 2;
	uint32_t         rlp_ir_state_type = 7; /*state*/
	uint32_t         rlp_rp_state_type = 1;
	uint32_t         rlp_ir_state_hash_type = 11;
	uint32_t         rlp_rp_state_hash_type = 11;
	uint32_t         rlp_max_pcs = 5;
	uint32_t         rlp_max_offsets = 5;
	uint32_t         rlp_max_deltas = 5;

	/*feature*/
	uint32_t         rlp_ir_pref_degree = 1; 
	uint32_t         rlp_rp_pref_degree = 1; 
	uint32_t         rlp_rp_enable_dyn_degree = true;/*dynamic pref degree*/
	uint32_t         rlp_rp_multi_deg_select_type = 2;
	vector<float>    rlp_rp_max_to_avg_q_thresholds;
	vector<int32_t>  rlp_rp_dyn_degrees;
	vector<int32_t>  rlp_rp_last_pref_offset_conf_thresholds;
	vector<int32_t>  rlp_rp_dyn_degrees_type2;
	vector<int32_t>  rlp_rp_last_pref_offset_conf_thresholds_hbw;
	vector<int32_t>  rlp_rp_dyn_degrees_type2_hbw;
	bool             rlp_enable_track_multiple = false; /**prefetch queue multi-track*/
	uint32_t         rlp_high_bw_thresh = 4;        /*high bandwidth threshold*/

	/*debug*/
	bool             rlp_debug_enable_state_action_stats = false;
	uint32_t         rlp_debug_state_action_stats_topK = false;
	bool             rlp_debug_access = false;
	bool             rlp_debug_print_access = false;
	uint64_t         rlp_debug_print_access_pc = false;
	uint32_t         rlp_debug_print_access_pc_count = false;
	bool             rlp_debug_print_trace = false;

	/* RLP Irregular Pattern Learning Engine knobs */
	uint32_t         rlp_le_ir_way = 4;
	bool             rlp_le_ir_enable_trace = false;
	uint32_t         rlp_le_ir_trace_interval = 5;
	std::string      rlp_le_ir_trace_file_name = std::string("rlp_le_ir_trace_file.txt");
	uint32_t         rlp_le_ir_trace_state;
	bool             rlp_le_ir_enable_score_plot = false;
	//std::vector<int32_t> rlp_le_ir_plot_actions;
	std::string      rlp_le_ir_plot_file_name = std::string("rlp_le_ir_score_plot.png");
	bool             rlp_le_ir_enable_action_trace = false;
	uint32_t         rlp_le_ir_action_trace_interval = 5;
	std::string      rlp_le_ir_action_trace_name = std::string("rlp_le_ir_action_trace_file.txt");
	bool             rlp_le_ir_enable_action_plot = false;

	/* RLP Regular Pattern Learning Engine knobs */
	vector<int32_t>  rlp_le_rp_featurewise_active_features;
	vector<int32_t>  rlp_le_rp_featurewise_num_tilings;
	vector<int32_t>  rlp_le_rp_featurewise_num_tiles;
	vector<int32_t>  rlp_le_rp_featurewise_hash_types;
	vector<int32_t>  rlp_le_rp_featurewise_enable_tiling_offset;
	float			 rlp_le_rp_featurewise_max_q_thresh = 0.5;
	bool			 rlp_le_rp_featurewise_enable_action_fallback = true;
	vector<float> 	 rlp_le_rp_featurewise_feature_weights;
	bool			 rlp_le_rp_featurewise_enable_dynamic_weight = true;
	float			 rlp_le_rp_featurewise_weight_gradient = 0.001;
	bool			 rlp_le_rp_featurewise_disable_adjust_weight_all_features_align = false;
	bool			 rlp_le_rp_featurewise_selective_update = false;
	uint32_t 		 rlp_le_rp_featurewise_pooling_type = 2; // max pooling */ 
	bool             rlp_le_rp_featurewise_enable_dyn_action_fallback = true;
	uint32_t 		 rlp_le_rp_featurewise_bw_acc_check_level = 1;
	uint32_t 		 rlp_le_rp_featurewise_acc_thresh = 2;
	bool 			 rlp_le_rp_featurewise_enable_trace = false;
	uint32_t		 rlp_le_rp_featurewise_trace_feature_type;
	string 			 rlp_le_rp_featurewise_trace_feature;
	uint32_t 		 rlp_le_rp_featurewise_trace_interval;
	uint32_t 		 rlp_le_rp_featurewise_trace_record_count;
	std::string		 rlp_le_rp_featurewise_trace_file_name;
	bool 			 rlp_le_rp_featurewise_enable_score_plot;
	vector<int32_t>  rlp_le_rp_featurewise_plot_actions;
	std::string 	 rlp_le_rp_featurewise_plot_file_name;
	bool 			 rlp_le_rp_featurewise_remove_plot_script;

}

void parse_args(int argc, char *argv[])
{
	for(int index = 0; index < argc; ++index)
	{
		string arg = string(argv[index]);
		if(arg.compare(0, 2, "--") == 0)
		{
			arg = arg.substr(2);
		}
		if(ini_parse_string(arg.c_str(), handler, NULL) < 0)
		{
			printf("error parsing commandline %s\n", argv[index]);
			exit(1);
		}
	}
}

int handler(void* user, const char* section, const char* name, const char* value)
{
	char config_file_name[MAX_LEN];

	if(MATCH("", "config"))
	{
		strcpy(config_file_name, value);
		parse_config(config_file_name);
	}
	else
	{
		parse_knobs(user, section, name, value);
	}
	return 1;
}

void parse_config(char *config_file_name)
{
	cout << "parsing config file: " << string(config_file_name) << endl;
	if (ini_parse(config_file_name, parse_knobs, NULL) < 0)
	{
        printf("Failed to load %s\n", config_file_name);
        exit(1);
    }
}

int parse_knobs(void* user, const char* section, const char* name, const char* value)
{
	char config_file_name[MAX_LEN];

	if(MATCH("", "config"))
	{
		strcpy(config_file_name, value);
		parse_config(config_file_name);
	}
    else if (MATCH("", "warmup_instructions"))
    {
		knob::warmup_instructions = atol(value);
    }
    else if (MATCH("", "simulation_instructions"))
    {
		knob::simulation_instructions = atol(value);
    }
    else if (MATCH("", "knob_cloudsuite"))
    {
		knob::knob_cloudsuite = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "knob_low_bandwidth"))
    {
		knob::knob_low_bandwidth = atoi(value);
    }
    else if (MATCH("", "l2c_prefetcher_types"))
    {
		knob::l2c_prefetcher_types.push_back(string(value));
    }
    else if (MATCH("", "l1d_prefetcher_types"))
    {
		knob::l1d_prefetcher_types.push_back(string(value));
    }
    else if (MATCH("", "l1d_perfect"))
    {
		knob::l1d_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "l2c_perfect"))
    {
		knob::l2c_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "llc_perfect"))
    {
		knob::llc_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "l1d_semi_perfect"))
    {
		knob::l1d_semi_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "l2c_semi_perfect"))
    {
		knob::l2c_semi_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "llc_semi_perfect"))
    {
		knob::llc_semi_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "semi_perfect_cache_page_buffer_size"))
    {
		knob::semi_perfect_cache_page_buffer_size = atoi(value);
    }
    else if (MATCH("", "measure_ipc"))
    {
		knob::measure_ipc = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "measure_ipc_epoch"))
    {
		knob::measure_ipc_epoch = atoi(value);
    }
    else if (MATCH("", "dram_io_freq"))
    {
		knob::dram_io_freq = atoi(value);
    }
    else if (MATCH("", "measure_dram_bw"))
    {
		knob::measure_dram_bw = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "measure_dram_bw_epoch"))
    {
		knob::measure_dram_bw_epoch = atoi(value);
    }
    else if (MATCH("", "measure_cache_acc"))
    {
		knob::measure_cache_acc = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "measure_cache_acc_epoch"))
    {
		knob::measure_cache_acc_epoch = atoi(value);
    }

    /* next-line */
    else if (MATCH("", "next_line_deltas"))
    {
		knob::next_line_deltas = get_array_int(value);
    }
    else if (MATCH("", "next_line_delta_prob"))
    {
		knob::next_line_delta_prob = get_array_float(value);
    }
    else if (MATCH("", "next_line_seed"))
    {
		knob::next_line_seed = atoi(value);
    }
    else if (MATCH("", "next_line_pt_size"))
    {
		knob::next_line_pt_size = atoi(value);
    }
    else if (MATCH("", "next_line_enable_prefetch_tracking"))
    {
		knob::next_line_enable_prefetch_tracking = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "next_line_enable_trace"))
    {
		knob::next_line_enable_trace = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "next_line_trace_interval"))
    {
		knob::next_line_trace_interval = atoi(value);
    }
    else if (MATCH("", "next_line_trace_name"))
    {
		knob::next_line_trace_name = string(value);
    }
    else if (MATCH("", "next_line_pref_degree"))
    {
		knob::next_line_pref_degree = atoi(value);
    }

    /* SMS */
	else if(MATCH("", "sms_at_size"))
	{
		knob::sms_at_size = atoi(value);
	}
	else if(MATCH("", "sms_ft_size"))
	{
		knob::sms_ft_size = atoi(value);
	}
	else if(MATCH("", "sms_pht_size"))
	{
		knob::sms_pht_size = atoi(value);
	}
	else if(MATCH("", "sms_pht_assoc"))
	{
		knob::sms_pht_assoc = atoi(value);
	}
	else if(MATCH("", "sms_pref_degree"))
	{
		knob::sms_pref_degree = atoi(value);
	}
	else if(MATCH("", "sms_region_size"))
	{
		knob::sms_region_size = atoi(value);
		knob::sms_region_size_log = log2(knob::sms_region_size);
	}
	else if(MATCH("", "sms_enable_pref_buffer"))
	{
		knob::sms_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if(MATCH("", "sms_pref_buffer_size"))
	{
		knob::sms_pref_buffer_size = atoi(value);
	}

	/* SPP */
	else if (MATCH("", "spp_st_size"))
	{
		knob::spp_st_size = atoi(value);
	}
	else if (MATCH("", "spp_pt_size"))
	{
		knob::spp_pt_size = atoi(value);
	}
	else if (MATCH("", "spp_max_outcomes"))
	{
		knob::spp_max_outcomes = atoi(value);
	}
	else if (MATCH("", "spp_max_confidence"))
	{
		knob::spp_max_confidence = strtod(value, NULL);
	}
	else if (MATCH("", "spp_max_depth"))
	{
		knob::spp_max_depth = atoi(value);
	}
	else if (MATCH("", "spp_max_prefetch_per_level"))
	{
		knob::spp_max_prefetch_per_level = atoi(value);
	}
	else if (MATCH("", "spp_max_confidence_counter_value"))
	{
		knob::spp_max_confidence_counter_value = atoi(value);
	}
	else if (MATCH("", "spp_max_global_counter_value"))
	{
		knob::spp_max_global_counter_value = atoi(value);
	}
	else if (MATCH("", "spp_pf_size"))
	{
		knob::spp_pf_size = atoi(value);
	}
	else if (MATCH("", "spp_enable_alpha"))
	{
		knob::spp_enable_alpha = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_enable_pref_buffer"))
	{
		knob::spp_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_pref_buffer_size"))
	{
		knob::spp_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "spp_pref_degree"))
	{
		knob::spp_pref_degree = atoi(value);
	}
	else if (MATCH("", "spp_enable_ghr"))
	{
		knob::spp_enable_ghr = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_ghr_size"))
	{
		knob::spp_ghr_size = atoi(value);
	}
	else if (MATCH("", "spp_signature_bits"))
	{
		knob::spp_signature_bits = atoi(value);
	}
	else if (MATCH("", "spp_alpha_epoch"))
	{
		knob::spp_alpha_epoch = atoi(value);
	}

	/* SPP_dev2 */
	else if (MATCH("", "spp_dev2_fill_threshold"))
	{
		knob::spp_dev2_fill_threshold = atoi(value);
	}
	else if (MATCH("", "spp_dev2_pf_threshold"))
	{
		knob::spp_dev2_pf_threshold = atoi(value);
	}

	/* BOP */
	else if (MATCH("", "bop_candidates"))
	{
		knob::bop_candidates = get_array_int(value);
	}
	else if (MATCH("", "bop_max_rounds"))
	{
		knob::bop_max_rounds = atoi(value);
	}
	else if (MATCH("", "bop_max_score"))
	{
		knob::bop_max_score = atoi(value);
	}
	else if (MATCH("", "bop_top_n"))
	{
		knob::bop_top_n = atoi(value);
	}
	else if (MATCH("", "bop_enable_pref_buffer"))
	{
		knob::bop_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "bop_pref_buffer_size"))
	{
		knob::bop_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "bop_pref_degree"))
	{
		knob::bop_pref_degree = atoi(value);
	}
	else if (MATCH("", "bop_rr_size"))
	{
		knob::bop_rr_size = atoi(value);
	}

	/* Sandbox */
	else if (MATCH("", "sandbox_pref_degree"))
	{
		knob::sandbox_pref_degree = atoi(value);
	}
	else if (MATCH("", "sandbox_enable_stream_detect"))
	{
		knob::sandbox_enable_stream_detect = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "sandbox_stream_detect_length"))
	{
		knob::sandbox_stream_detect_length = atoi(value);
	}
	else if (MATCH("", "sandbox_num_access_in_phase"))
	{
		knob::sandbox_num_access_in_phase = atoi(value);
	}
	else if (MATCH("", "sandbox_num_cycle_offsets"))
	{
		knob::sandbox_num_cycle_offsets = atoi(value);
	}
	else if (MATCH("", "sandbox_bloom_filter_size"))
	{
		knob::sandbox_bloom_filter_size = atoi(value);
	}
	else if (MATCH("", "sandbox_seed"))
	{
		knob::sandbox_seed = atoi(value);
	}

	/* DSPatch */
	else if (MATCH("", "dspatch_log2_region_size"))
	{
		knob::dspatch_log2_region_size = atoi(value);
		knob::dspatch_num_cachelines_in_region = 1 << (knob::dspatch_log2_region_size - 6); /* considers traditional 64B cachelines */
	}
	else if (MATCH("", "dspatch_pb_size"))
	{
		knob::dspatch_pb_size = atoi(value);
	}
	else if (MATCH("", "dspatch_num_spt_entries"))
	{
		knob::dspatch_num_spt_entries = atoi(value);
	}
	else if (MATCH("", "dspatch_compression_granularity"))
	{
		knob::dspatch_compression_granularity = atoi(value);
	}
	else if (MATCH("", "dspatch_pred_throttle_bw_thr"))
	{
		knob::dspatch_pred_throttle_bw_thr = atoi(value);
	}
	else if (MATCH("", "dspatch_bitmap_selection_policy"))
	{
		knob::dspatch_bitmap_selection_policy = atoi(value);
	}
	else if (MATCH("", "dspatch_sig_type"))
	{
		knob::dspatch_sig_type = atoi(value);
	}
	else if (MATCH("", "dspatch_sig_hash_type"))
	{
		knob::dspatch_sig_hash_type = atoi(value);
	}
	else if (MATCH("", "dspatch_or_count_max"))
	{
		knob::dspatch_or_count_max = atoi(value);
	}
	else if (MATCH("", "dspatch_measure_covP_max"))
	{
		knob::dspatch_measure_covP_max = atoi(value);
	}
	else if (MATCH("", "dspatch_measure_accP_max"))
	{
		knob::dspatch_measure_accP_max = atoi(value);
	}
	else if (MATCH("", "dspatch_acc_thr"))
	{
		knob::dspatch_acc_thr = atoi(value);
	}
	else if (MATCH("", "dspatch_cov_thr"))
	{
		knob::dspatch_cov_thr = atoi(value);
	}
	else if (MATCH("", "dspatch_enable_pref_buffer"))
	{
		knob::dspatch_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "dspatch_pref_buffer_size"))
	{
		knob::dspatch_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "dspatch_pref_degree"))
	{
		knob::dspatch_pref_degree = atoi(value);
	}

	/* PPF */
	else if (MATCH("", "ppf_perc_threshold_hi"))
	{
		knob::ppf_perc_threshold_hi = atoi(value);
	}
	else if (MATCH("", "ppf_perc_threshold_lo"))
	{
		knob::ppf_perc_threshold_lo = atoi(value);
	}

	/* MLOP */
	else if (MATCH("", "mlop_pref_degree"))
	{
		knob::mlop_pref_degree = atoi(value);
	}
	else if (MATCH("", "mlop_num_updates"))
	{
		knob::mlop_num_updates = atoi(value);
	}
	else if (MATCH("", "mlop_l1d_thresh"))
	{
		knob::mlop_l1d_thresh = atof(value);
	}
	else if (MATCH("", "mlop_l2c_thresh"))
	{
		knob::mlop_l2c_thresh = atof(value);
	}
	else if (MATCH("", "mlop_llc_thresh"))
	{
		knob::mlop_llc_thresh = atof(value);
	}
	else if (MATCH("", "mlop_debug_level"))
	{
		knob::mlop_debug_level = atoi(value);
	}

	/* Bingo */
	else if (MATCH("", "bingo_region_size"))
	{
	   knob::bingo_region_size = atoi(value);
	}
	else if (MATCH("", "bingo_pattern_len"))
	{
	   knob::bingo_pattern_len = atoi(value);
	}
	else if (MATCH("", "bingo_pc_width"))
	{
	   knob::bingo_pc_width = atoi(value);
	}
	else if (MATCH("", "bingo_min_addr_width"))
	{
	   knob::bingo_min_addr_width = atoi(value);
	}
	else if (MATCH("", "bingo_max_addr_width"))
	{
	   knob::bingo_max_addr_width = atoi(value);
	}
	else if (MATCH("", "bingo_ft_size"))
	{
	   knob::bingo_ft_size = atoi(value);
	}
	else if (MATCH("", "bingo_at_size"))
	{
	   knob::bingo_at_size = atoi(value);
	}
	else if (MATCH("", "bingo_pht_size"))
	{
	   knob::bingo_pht_size = atoi(value);
	}
	else if (MATCH("", "bingo_pht_ways"))
	{
	   knob::bingo_pht_ways = atoi(value);
	}
	else if (MATCH("", "bingo_pf_streamer_size"))
	{
	   knob::bingo_pf_streamer_size = atoi(value);
	}
	else if (MATCH("", "bingo_debug_level"))
	{
	   knob::bingo_debug_level = atoi(value);
	}
	else if (MATCH("", "bingo_l1d_thresh"))
	{
	   knob::bingo_l1d_thresh = atof(value);
	}
	else if (MATCH("", "bingo_l2c_thresh"))
	{
	   knob::bingo_l2c_thresh = atof(value);
	}
	else if (MATCH("", "bingo_llc_thresh"))
	{
	   knob::bingo_llc_thresh = atof(value);
	}
	else if (MATCH("", "bingo_pc_address_fill_level"))
	{
	   knob::bingo_pc_address_fill_level = string(value);
	}

	/* Stride Prefetcher */
	else if (MATCH("", "stride_num_trackers"))
	{
		knob::stride_num_trackers = atoi(value);
	}
	else if (MATCH("", "stride_pref_degree"))
	{
		knob::stride_pref_degree = atoi(value);
	}

	else if (MATCH("", "streamer_num_trackers"))
	{
		knob::streamer_num_trackers = atoi(value);
	}
	else if (MATCH("", "streamer_pref_degree"))
	{
		knob::streamer_pref_degree = atoi(value);
	}

	/* GnnPrefetcher */
	else if (MATCH("", "RegTableEntry_num"))
	{
		knob::RegTableEntry_num = atoi(value);
	}
	else if (MATCH("", "reg_pref_degree"))
	{
		knob::reg_pref_degree = atoi(value);
	}

	/* AMPM */
	else if (MATCH("", "ampm_pb_size"))
	{
		knob::ampm_pb_size = atoi(value);
	}
	else if (MATCH("", "ampm_pred_degree"))
	{
		knob::ampm_pred_degree = atoi(value);
	}
	else if (MATCH("", "ampm_pref_degree"))
	{
		knob::ampm_pref_degree = atoi(value);
	}
	else if (MATCH("", "ampm_pref_buffer_size"))
	{
		knob::ampm_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "ampm_enable_pref_buffer"))
	{
		knob::ampm_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "ampm_max_delta"))
	{
		knob::ampm_max_delta = atoi(value);
	}

	/* Context Prefetcher */
	else if (MATCH("", "cp_cst_size"))
	{
		knob::cp_cst_size = atoi(value);
	}
	else if (MATCH("", "cp_cst_assoc"))
	{
		knob::cp_cst_assoc = atoi(value);
	}
	else if (MATCH("", "cp_max_response_per_cst"))
	{
		knob::cp_max_response_per_cst = atoi(value);
	}
	else if (MATCH("", "cp_init_reward"))
	{
		knob::cp_init_reward = atoi(value);
	}
	else if (MATCH("", "cp_prefetch_queue_size"))
	{
		knob::cp_prefetch_queue_size = atoi(value);
	}

	/* POWER7 Prefetcher */
	else if (MATCH("", "power7_explore_epoch"))
	{
		knob::power7_explore_epoch = atoi(value);
	}
	else if (MATCH("", "power7_exploit_epoch"))
	{
		knob::power7_exploit_epoch = atoi(value);
	}
	else if (MATCH("", "power7_default_streamer_degree"))
	{
		knob::power7_default_streamer_degree = atoi(value);
	}

	/* Scooby */
	else if (MATCH("", "scooby_alpha"))
	{
		knob::scooby_alpha = atof(value);
	}
	else if (MATCH("", "scooby_gamma"))
	{
		knob::scooby_gamma = atof(value);
	}
	else if (MATCH("", "scooby_epsilon"))
	{
		knob::scooby_epsilon = atof(value);
	}
	else if (MATCH("", "scooby_state_num_bits"))
	{
		knob::scooby_state_num_bits = atoi(value);
		knob::scooby_max_states = pow(2.0, knob::scooby_state_num_bits);
	}
	else if (MATCH("", "scooby_seed"))
	{
		knob::scooby_seed = atoi(value);
	}
	else if (MATCH("", "scooby_policy"))
	{
		knob::scooby_policy = string(value);
	}
	else if (MATCH("", "scooby_learning_type"))
	{
		knob::scooby_learning_type = string(value);
	}
	else if (MATCH("", "scooby_actions"))
	{
		knob::scooby_actions = get_array_int(value);
		knob::scooby_max_actions = knob::scooby_actions.size();
	}
	else if (MATCH("", "scooby_pt_size"))
	{
		knob::scooby_pt_size = atoi(value);
	}
	else if (MATCH("", "scooby_st_size"))
	{
		knob::scooby_st_size = atoi(value);
	}
	else if (MATCH("", "scooby_reward_none"))
	{
		knob::scooby_reward_none = atoi(value);
	}
	else if (MATCH("", "scooby_reward_incorrect"))
	{
		knob::scooby_reward_incorrect = atoi(value);
	}
	else if (MATCH("", "scooby_reward_correct_untimely"))
	{
		knob::scooby_reward_correct_untimely = atoi(value);
	}
	else if (MATCH("", "scooby_reward_correct_timely"))
	{
		knob::scooby_reward_correct_timely = atoi(value);
	}
	else if (MATCH("", "scooby_max_pcs"))
	{
		knob::scooby_max_pcs = atoi(value);
	}
	else if (MATCH("", "scooby_max_offsets"))
	{
		knob::scooby_max_offsets = atoi(value);
	}
	else if (MATCH("", "scooby_max_deltas"))
	{
		knob::scooby_max_deltas = atoi(value);
	}
	else if (MATCH("", "scooby_brain_zero_init"))
	{
		knob::scooby_brain_zero_init = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_all"))
	{
		knob::scooby_enable_reward_all = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_track_multiple"))
	{
		knob::scooby_enable_track_multiple = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_out_of_bounds"))
	{
		knob::scooby_enable_reward_out_of_bounds = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_reward_out_of_bounds"))
	{
		knob::scooby_reward_out_of_bounds = atoi(value);
	}
	else if (MATCH("", "scooby_state_type"))
	{
		knob::scooby_state_type = atoi(value);
	}
	else if (MATCH("", "scooby_access_debug"))
	{
		knob::scooby_access_debug = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_print_access_debug"))
	{
		knob::scooby_print_access_debug = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_print_access_debug_pc"))
	{
		knob::scooby_print_access_debug_pc = strtoul(value, NULL, 0);
	}
	else if (MATCH("", "scooby_print_access_debug_pc_count"))
	{
		knob::scooby_print_access_debug_pc_count = atoi(value);
	}
	else if (MATCH("", "scooby_print_trace"))
	{
		knob::scooby_print_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_state_action_stats"))
	{
		knob::scooby_enable_state_action_stats = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_tracker_hit"))
	{
		knob::scooby_enable_reward_tracker_hit = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_reward_tracker_hit"))
	{
		knob::scooby_reward_tracker_hit = atoi(value);
	}
	else if (MATCH("", "scooby_enable_shaggy"))
	{
		knob::scooby_enable_shaggy = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_state_hash_type"))
	{
		knob::scooby_state_hash_type = atoi(value);
	}
	else if (MATCH("", "scooby_prefetch_with_shaggy"))
	{
		knob::scooby_prefetch_with_shaggy = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_featurewise_engine"))
	{
		knob::scooby_enable_featurewise_engine = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_pref_degree"))
	{
		knob::scooby_pref_degree = atoi(value);
	}
	else if (MATCH("", "scooby_enable_dyn_degree"))
	{
		knob::scooby_enable_dyn_degree = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_max_to_avg_q_thresholds"))
	{
		knob::scooby_max_to_avg_q_thresholds = get_array_float(value);
	}
	else if (MATCH("", "scooby_early_exploration_window"))
	{
		knob::scooby_early_exploration_window = atoi(value);
	}
	else if (MATCH("", "scooby_dyn_degrees"))
	{
		knob::scooby_dyn_degrees = get_array_int(value);
	}
	else if (MATCH("", "scooby_multi_deg_select_type"))
	{
		knob::scooby_multi_deg_select_type = atoi(value);
	}
	else if (MATCH("", "scooby_last_pref_offset_conf_thresholds"))
	{
		knob::scooby_last_pref_offset_conf_thresholds = get_array_int(value);
	}
	else if (MATCH("", "scooby_dyn_degrees_type2"))
	{
		knob::scooby_dyn_degrees_type2 = get_array_int(value);
	}
	else if (MATCH("", "scooby_action_tracker_size"))
	{
		knob::scooby_action_tracker_size = atoi(value);
	}
	else if (MATCH("", "scooby_high_bw_thresh"))
	{
		knob::scooby_high_bw_thresh = atoi(value);
	}
	else if (MATCH("", "scooby_enable_hbw_reward"))
	{
		knob::scooby_enable_hbw_reward = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_reward_hbw_correct_timely"))
	{
		knob::scooby_reward_hbw_correct_timely = atoi(value);
	}
	else if (MATCH("", "scooby_reward_hbw_correct_untimely"))
	{
		knob::scooby_reward_hbw_correct_untimely = atoi(value);
	}
	else if (MATCH("", "scooby_reward_hbw_incorrect"))
	{
		knob::scooby_reward_hbw_incorrect = atoi(value);
	}
	else if (MATCH("", "scooby_reward_hbw_none"))
	{
		knob::scooby_reward_hbw_none = atoi(value);
	}
	else if (MATCH("", "scooby_reward_hbw_out_of_bounds"))
	{
		knob::scooby_reward_hbw_out_of_bounds = atoi(value);
	}
	else if (MATCH("", "scooby_reward_hbw_tracker_hit"))
	{
		knob::scooby_reward_hbw_tracker_hit = atoi(value);
	}
	else if (MATCH("", "scooby_last_pref_offset_conf_thresholds_hbw"))
	{
		knob::scooby_last_pref_offset_conf_thresholds_hbw = get_array_int(value);
	}
	else if (MATCH("", "scooby_dyn_degrees_type2_hbw"))
	{
		knob::scooby_dyn_degrees_type2_hbw = get_array_int(value);
	}

	/* Learning Engine */
	else if (MATCH("", "le_enable_trace"))
	{
		knob::le_enable_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_trace_interval"))
	{
		knob::le_trace_interval = atoi(value);
	}
	else if (MATCH("", "le_trace_file_name"))
	{
		knob::le_trace_file_name = string(value);
	}
	else if (MATCH("", "le_trace_state"))
	{
		knob::le_trace_state = strtoul(value, NULL, 0);
	}
	else if (MATCH("", "le_enable_score_plot"))
	{
		knob::le_enable_score_plot = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_plot_actions"))
	{
		knob::le_plot_actions = get_array_int(value);
	}
	else if (MATCH("", "le_plot_file_name"))
	{
		knob::le_plot_file_name = string(value);
	}
	else if (MATCH("", "le_enable_action_trace"))
	{
		knob::le_enable_action_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_action_trace_interval"))
	{
		knob::le_action_trace_interval = atoi(value);
	}
	else if (MATCH("", "le_action_trace_name"))
	{
		knob::le_action_trace_name = string(value);
	}
	else if (MATCH("", "le_enable_action_plot"))
	{
		knob::le_enable_action_plot = !strcmp(value, "true") ? true : false;
	}

	/* Featurewise Learning Engine */
	else if (MATCH("", "le_featurewise_active_features"))
	{
		knob::le_featurewise_active_features = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_num_tilings"))
	{
		knob::le_featurewise_num_tilings = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_num_tiles"))
	{
		knob::le_featurewise_num_tiles = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_hash_types"))
	{
		knob::le_featurewise_hash_types = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_enable_tiling_offset"))
	{
		knob::le_featurewise_enable_tiling_offset = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_max_q_thresh"))
	{
		knob::le_featurewise_max_q_thresh = atof(value);
	}
	else if (MATCH("", "le_featurewise_enable_action_fallback"))
	{
		knob::le_featurewise_enable_action_fallback = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_feature_weights"))
	{
		knob::le_featurewise_feature_weights = get_array_float(value);
	}
	else if (MATCH("", "le_featurewise_enable_dynamic_weight"))
	{
		knob::le_featurewise_enable_dynamic_weight = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_weight_gradient"))
	{
		knob::le_featurewise_weight_gradient = atof(value);
	}
	else if (MATCH("", "le_featurewise_disable_adjust_weight_all_features_align"))
	{
		knob::le_featurewise_disable_adjust_weight_all_features_align = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_selective_update"))
	{
		knob::le_featurewise_selective_update = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_pooling_type"))
	{
		knob::le_featurewise_pooling_type = atoi(value);
	}
	else if (MATCH("", "le_featurewise_enable_dyn_action_fallback"))
	{
		knob::le_featurewise_enable_dyn_action_fallback = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_bw_acc_check_level"))
	{
		knob::le_featurewise_bw_acc_check_level = atoi(value);
	}
	else if (MATCH("", "le_featurewise_acc_thresh"))
	{
		knob::le_featurewise_acc_thresh = atoi(value);
	}
	else if (MATCH("", "le_featurewise_enable_trace"))
	{
	   knob::le_featurewise_enable_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_trace_feature_type"))
	{
	   knob::le_featurewise_trace_feature_type = atoi(value);
	}
	else if (MATCH("", "le_featurewise_trace_feature"))
	{
	   knob::le_featurewise_trace_feature = string(value);
	}
	else if (MATCH("", "le_featurewise_trace_interval"))
	{
	   knob::le_featurewise_trace_interval = atoi(value);
	}
	else if (MATCH("", "le_featurewise_trace_record_count"))
	{
	   knob::le_featurewise_trace_record_count = atoi(value);
	}
	else if (MATCH("", "le_featurewise_trace_file_name"))
	{
	   knob::le_featurewise_trace_file_name = string(value);
	}
	else if (MATCH("", "le_featurewise_enable_score_plot"))
	{
	   knob::le_featurewise_enable_score_plot = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_featurewise_plot_actions"))
	{
	   knob::le_featurewise_plot_actions = get_array_int(value);
	}
	else if (MATCH("", "le_featurewise_plot_file_name"))
	{
	   knob::le_featurewise_plot_file_name = string(value);
	}
	else if (MATCH("", "le_featurewise_remove_plot_script"))
	{
	   knob::le_featurewise_remove_plot_script = !strcmp(value, "true") ? true : false;
	}

	/*rlp */
	else if (MATCH("", "rlp_rp_actions"))
	{
		knob::rlp_rp_actions = get_array_int(value);
		knob::rlp_rp_max_actions = knob::rlp_rp_actions.size();
	}
	else if (MATCH("", "rlp_rp_max_to_avg_q_thresholds"))
	{
		knob::rlp_rp_max_to_avg_q_thresholds = get_array_float(value);
	}
	else if (MATCH("", "rlp_rp_dyn_degrees"))
	{
		knob::rlp_rp_dyn_degrees = get_array_int(value);
	}
	else if (MATCH("", "rlp_rp_last_pref_offset_conf_thresholds"))
	{
		knob::rlp_rp_last_pref_offset_conf_thresholds = get_array_int(value);
	}
	else if (MATCH("", "rlp_rp_dyn_degrees_type2"))
	{
		knob::rlp_rp_dyn_degrees_type2 = get_array_int(value);
	}
	else if (MATCH("", "rlp_rp_last_pref_offset_conf_thresholds_hbw"))
	{
		knob::rlp_rp_last_pref_offset_conf_thresholds_hbw = get_array_int(value);
	}
	else if (MATCH("", "rlp_rp_dyn_degrees_type2_hbw"))
	{
		knob::rlp_rp_dyn_degrees_type2_hbw = get_array_int(value);
	}

	/*rlp regular learning engine*/
	else if (MATCH("", "rlp_le_rp_featurewise_active_features"))
	{
		knob::rlp_le_rp_featurewise_active_features = get_array_int(value);
	}
	else if (MATCH("", "rlp_le_rp_featurewise_num_tilings"))
	{
		knob::rlp_le_rp_featurewise_num_tilings = get_array_int(value);
	}
	else if (MATCH("", "rlp_le_rp_featurewise_num_tiles"))
	{
		knob::rlp_le_rp_featurewise_num_tiles = get_array_int(value);
	}
	else if (MATCH("", "rlp_le_rp_featurewise_hash_types"))
	{
		knob::rlp_le_rp_featurewise_hash_types = get_array_int(value);
	}
	else if (MATCH("", "rlp_le_rp_featurewise_enable_tiling_offset"))
	{
		knob::rlp_le_rp_featurewise_enable_tiling_offset = get_array_int(value);
	}
	else if (MATCH("", "rlp_le_rp_featurewise_feature_weights"))
	{
		knob::rlp_le_rp_featurewise_feature_weights = get_array_float(value);
	}

    else
    {
    	printf("unable to parse section: %s, name: %s, value: %s\n", section, name, value);
        return 0;
    }
    return 1;
}

std::vector<int32_t> get_array_int(const char *str)
{
	std::vector<int32_t> value;
	char *tmp_str = strdup(str); //use malloc to copy str to *tmp_str
	char *pch = strtok(tmp_str, ",");//split tmp_str when meet ,
	while(pch)
	{
		value.push_back(strtol(pch, NULL, 0));  //transfer string to long
		pch = strtok(NULL, ",");
	}
	free(tmp_str);
	return value;
}

std::vector<float> get_array_float(const char *str)
{
	std::vector<float> value;
	char *tmp_str = strdup(str);
	char *pch = strtok(tmp_str, ",");
	while(pch)
	{
		value.push_back(atof(pch));
		pch = strtok(NULL, ",");
	}
	free(tmp_str);
	return value;
}
