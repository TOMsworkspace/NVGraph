#ifndef HYBRID_MEMORY_H
#define HYBRID_MEMORY_H

#include "memory_class.h"
#include "nvm_controller.h"
#include "predictor.h"

// NVM configuration
#define HMM_NVM_CHANNEL_WIDTH 8 // 8B
#define HMM_NVM_WQ_SIZE 64
#define HMM_NVM_RQ_SIZE 64

#define tRP_HMM_NVM_NANOSECONDS  30
#define tRCD_HMM_NVM_NANOSECONDS 30
#define tCAS_HMM_NVM_NANOSECONDS 25

// the data bus must wait this amount of time when switching between reads and writes, and vice versa
#define HMM_NVM_DBUS_TURN_AROUND_TIME ((30*CPU_FREQ)/2000) // 15 ns 
extern uint32_t HMM_NVM_MTPS, HMM_NVM_DBUS_RETURN_TIME, HMM_NVM_DBUS_MAX_CAS;

// these values control when to send out a burst of writes
#define HMM_NVM_WRITE_HIGH_WM    ((HMM_NVM_WQ_SIZE*7)>>3) // 7/8th
#define HMM_NVM_WRITE_LOW_WM     ((HMM_NVM_WQ_SIZE*3)>>2) // 6/8th
#define MIN_HMM_NVM_WRITES_PER_SWITCH (HMM_NVM_WQ_SIZE * 1 / 4)

void print_hybrid_memory_config();


class HYBRID_MEMORY_CONTROLLER : public MEMORY {
  public:
    const string NAME;

    NVM_ARRAY nvm_array[HMM_NVM_CHANNELS][HMM_NVM_RANKS][HMM_NVM_BANKS];
    uint64_t dbus_cycle_available[HMM_NVM_CHANNELS], dbus_cycle_congested[HMM_NVM_CHANNELS], dbus_congested[HMM_NVM_CHANNELS][NUM_TYPES+1][NUM_TYPES+1];
    uint64_t bank_cycle_available[HMM_NVM_CHANNELS][HMM_NVM_RANKS][HMM_NVM_BANKS];
    uint8_t  do_write, write_mode[HMM_NVM_CHANNELS]; 
    uint32_t processed_writes, scheduled_reads[HMM_NVM_CHANNELS], scheduled_writes[HMM_NVM_CHANNELS];
    int fill_level;

    BANK_REQUEST bank_request[HMM_NVM_CHANNELS][HMM_NVM_RANKS][HMM_NVM_BANKS];

    // queues
    PACKET_QUEUE WQ[HMM_NVM_CHANNELS], RQ[HMM_NVM_CHANNELS];
    
    // to measure bandwidth
    uint64_t rq_enqueue_count, last_enqueue_count, epoch_enqueue_count, next_bw_measure_cycle;
    uint8_t bw;
    uint64_t total_bw_epochs;
    uint64_t bw_level_hist[HMM_NVM_BW_LEVELS];

    // constructor
    HYBRID_MEMORY_CONTROLLER(string v1) : NAME (v1) {
	for(uint32_t channel = 0; channel < HMM_NVM_CHANNELS; ++channel){    
            for (uint32_t i=0; i<NUM_TYPES+1; i++) {
                for (uint32_t j=0; j<NUM_TYPES+1; j++) {
                    dbus_congested[channel][i][j] = 0;
                }
            }
	}
        do_write = 0;
        processed_writes = 0;
        for (uint32_t i=0; i<HMM_NVM_CHANNELS; i++) {
            dbus_cycle_available[i] = 0;
            dbus_cycle_congested[i] = 0;
            write_mode[i] = 0;
            scheduled_reads[i] = 0;
            scheduled_writes[i] = 0;

            for (uint32_t j=0; j < HMM_NVM_RANKS; j++) {
                for (uint32_t k=0; k< HMM_NVM_BANKS; k++)
                    bank_cycle_available[i][j][k] = 0;
            }

            WQ[i].NAME = "NVM_WQ" + to_string(i);
            WQ[i].SIZE = HMM_NVM_WQ_SIZE;
            WQ[i].entry = new PACKET [HMM_NVM_WQ_SIZE];

            RQ[i].NAME = "NVM_RQ" + to_string(i);
            RQ[i].SIZE = HMM_NVM_RQ_SIZE;
            RQ[i].entry = new PACKET [HMM_NVM_RQ_SIZE];
        }

        fill_level = FILL_NVM;

        rq_enqueue_count = 0;
        last_enqueue_count = 0;
        epoch_enqueue_count = 0;
        next_bw_measure_cycle = 1;
        bw = 0;
    };

    // destructor
    ~HYBRID_MEMORY_CONTROLLER() {

    };

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    void schedule(PACKET_QUEUE *queue), process(PACKET_QUEUE *queue),
         update_schedule_cycle(PACKET_QUEUE *queue),
         update_process_cycle(PACKET_QUEUE *queue),
         reset_remain_requests(PACKET_QUEUE *queue, uint32_t channel);

    uint32_t nvm_get_channel(uint64_t address),
             nvm_get_rank   (uint64_t address),
             nvm_get_bank   (uint64_t address),
             nvm_get_row    (uint64_t address),
             nvm_get_column (uint64_t address),
             drc_check_hit (uint64_t address, uint32_t cpu, uint32_t channel, uint32_t rank, uint32_t bank, uint32_t row);

    uint64_t get_bank_earliest_cycle();

    int check_nvm_queue(PACKET_QUEUE *queue, PACKET *packet);
};

#endif
