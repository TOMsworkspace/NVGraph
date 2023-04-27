#ifndef RL_HELPER_H
#define RL_HELPER_H

#define NO_PREFETCH_ADDR 0
#define NO_PREFETCH_ACTION 0

typedef enum
{
	PC = 0,
	Offset,
	Delta,
	PC_path,
	Offset_path,
	Delta_path,
	Address,
	Page,

	NumFeatures
} Feature;

const char* getFeatureString(Feature feature);

class State
{
public:
	uint64_t pc;
	uint64_t address;
    uint64_t cacheline;
	uint64_t page;
	uint32_t offset;
	int32_t	 delta;
	uint32_t local_delta_sig;
	uint32_t local_delta_sig2;
	uint32_t local_pc_sig;
	uint32_t local_offset_sig;
	uint8_t  bw_level;
	bool     is_high_bw;
	uint32_t acc_level;

	/* 
	 * Add more states here
	 */

	void reset()
	{
		pc = 0xdeadbeef;
		address = 0xdeadbeef;
		page = 0xdeadbeef;
		offset = 0;
		delta = 0;
		local_delta_sig = 0;
		local_delta_sig2 = 0;
		local_pc_sig = 0;
		local_offset_sig = 0;
		bw_level = 0;
		is_high_bw = false;
		acc_level = 0;
	}
	State(){reset();}
	~State(){}
	uint32_t value(uint32_t state_type, uint32_t state_hash_type, uint32_t max_states); /* apply as many state types as you want */
	uint32_t get_hash(uint64_t value, uint32_t state_hash_type, uint32_t max_states); /* play wild with hashes */
	std::string to_string();
};

bool no_prefetch(uint64_t addr);
bool no_prefetch_action(uint64_t action);

#endif