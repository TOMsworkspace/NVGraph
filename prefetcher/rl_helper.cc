#include <assert.h>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "champsim.h"
#include "rl_helper.h"
#include "util.h"

namespace knob
{
	extern uint32_t cbp_max_pcs;
}


const char* MapFeatureString[] = {"PC", "Offset", "Delta", "PC_path", "Offset_path", "Delta_path", "Address", "Page"};
const char* getFeatureString(Feature feature)
{
	assert(feature < Feature::NumFeatures);
	return MapFeatureString[(uint32_t)feature];
}

uint32_t State::value(uint32_t state_type, uint32_t state_hash_type, uint32_t max_states)
{
	uint64_t value = 0;
	switch(state_type)
	{
		case 1: /* Only PC */
			// return (uint32_t)(pc % knob::cbp_max_states);
			value = pc;
			break;

		case 2: /* PC+ offset */
			value = pc;
			value = value << 6;
			value = value + offset;
			// value = value % knob::cbp_max_states;
			// return value;
			break;

		case 3: /* Only offset */
			value = offset;
			// value = value % knob::cbp_max_states;
			// return value;
			break;

		case 4: /* SPP like delta-path signature */
			value = local_delta_sig;
			// value = value % knob::cbp_max_states;
			// return value;
			break;

		case 5: /* SPP like path signature, but made with shifted PC */
			value = local_pc_sig;
			// value = value % knob::cbp_max_states;
			// return value;
			break;

		case 6: /* SPP's delta-path signature */
			value = local_delta_sig2;
			// value = value % knob::cbp_max_states;
			// return value;
			break;
		case 7: /*PC and address*/
			value = pc ^ cacheline;
			break;

		default:
			assert(false);
	}

	uint32_t hashed_value = get_hash(value, state_hash_type, max_states);
	assert(hashed_value < max_states);

	return hashed_value;
}

uint32_t State::get_hash(uint64_t key, uint32_t state_hash_type, uint32_t max_states)
{
	uint32_t value = 0;
	switch(state_hash_type)
	{
		case 1: /* simple modulo */
			value = (uint32_t)(key % max_states);
			break;

		case 2:
			value = HashZoo::jenkins((uint32_t)key);
			value = (uint32_t)(value % max_states);
			break;

		case 3:
			value = HashZoo::knuth((uint32_t)key);
			value = (uint32_t)(value % max_states);
			break;

		case 4:
			value = HashZoo::murmur3(key);
			value = (uint32_t)(value % max_states);
			break;

		case 5:
			value = HashZoo::jenkins32(key);
			value = (uint32_t)(value % max_states);
			break;

		case 6:
			value = HashZoo::hash32shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 7:
			value = HashZoo::hash32shiftmult(key);
			value = (uint32_t)(value % max_states);
			break;

		case 8:
			value = HashZoo::hash64shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 9:
			value = HashZoo::hash5shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 10:
			value = HashZoo::hash7shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 11:
			value = HashZoo::Wang6shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 12:
			value = HashZoo::Wang5shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 13:
			value = HashZoo::Wang4shift(key);
			value = (uint32_t)(value % max_states);
			break;

		case 14:
			value = HashZoo::Wang3shift(key);
			value = (uint32_t)(value % max_states);
			break;

		default:
			assert(false);
	}

	return value;
}

std::string State::to_string()
{
	std::stringstream ss;

	ss << std::hex << pc << std::dec << "|"
		<< offset << "|"
		<< delta;

	return ss.str();
}

bool no_prefetch(uint64_t addr){
	return addr == NO_PREFETCH_ADDR;
}


bool no_prefetch_action(uint64_t action){
	return action == NO_PREFETCH_ACTION;
}