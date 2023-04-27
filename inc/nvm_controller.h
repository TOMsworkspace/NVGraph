#ifndef NVM_H
#define NVM_H

#include "memory_class.h"

// NVM configuration
#define NVM_CHANNEL_WIDTH 8 // 8B
#define NVM_WQ_SIZE 64
#define NVM_RQ_SIZE 64

#define tRP_NVM_NANOSECONDS  30
#define tRCD_NVM_NANOSECONDS 30
#define tCAS_NVM_NANOSECONDS 25

// the data bus must wait this amount of time when switching between reads and writes, and vice versa
#define NVM_DBUS_TURN_AROUND_TIME ((30*CPU_FREQ)/2000) // 15 ns 
extern uint32_t NVM_MTPS, NVM_DBUS_RETURN_TIME, NVM_DBUS_MAX_CAS;

// these values control when to send out a burst of writes
#define NVM_WRITE_HIGH_WM    ((NVM_WQ_SIZE*7)>>3) // 7/8th
#define NVM_WRITE_LOW_WM     ((NVM_WQ_SIZE*3)>>2) // 6/8th
#define MIN_NVM_WRITES_PER_SWITCH (NVM_WQ_SIZE * 1 / 4)

void print_nvm_config();

// NVM
class NVM_MEMORY_CONTROLLER : public MEMORY {
  public:
    const string NAME;

    NVM_ARRAY nvm_array[NVM_CHANNELS][NVM_RANKS][NVM_BANKS];
    uint64_t dbus_cycle_available[NVM_CHANNELS], dbus_cycle_congested[NVM_CHANNELS], dbus_congested[NVM_CHANNELS][NUM_TYPES+1][NUM_TYPES+1];
    uint64_t bank_cycle_available[NVM_CHANNELS][NVM_RANKS][NVM_BANKS];
    uint8_t  do_write, write_mode[NVM_CHANNELS]; 
    uint32_t processed_writes, scheduled_reads[NVM_CHANNELS], scheduled_writes[NVM_CHANNELS];
    int fill_level;

    BANK_REQUEST bank_request[NVM_CHANNELS][NVM_RANKS][NVM_BANKS];

    // queues
    PACKET_QUEUE WQ[NVM_CHANNELS], RQ[NVM_CHANNELS];
    
    // to measure bandwidth
    uint64_t rq_enqueue_count, last_enqueue_count, epoch_enqueue_count, next_bw_measure_cycle;
    uint8_t bw;
    uint64_t total_bw_epochs;
    uint64_t bw_level_hist[NVM_BW_LEVELS];

    // constructor
    NVM_MEMORY_CONTROLLER(string v1) : NAME (v1) {
	for(uint32_t channel = 0; channel < NVM_CHANNELS; ++channel){    
            for (uint32_t i=0; i<NUM_TYPES+1; i++) {
                for (uint32_t j=0; j<NUM_TYPES+1; j++) {
                    dbus_congested[channel][i][j] = 0;
                }
            }
	}
        do_write = 0;
        processed_writes = 0;
        for (uint32_t i=0; i<NVM_CHANNELS; i++) {
            dbus_cycle_available[i] = 0;
            dbus_cycle_congested[i] = 0;
            write_mode[i] = 0;
            scheduled_reads[i] = 0;
            scheduled_writes[i] = 0;

            for (uint32_t j=0; j<NVM_RANKS; j++) {
                for (uint32_t k=0; k<NVM_BANKS; k++)
                    bank_cycle_available[i][j][k] = 0;
            }

            WQ[i].NAME = "NVM_WQ" + to_string(i);
            WQ[i].SIZE = NVM_WQ_SIZE;
            WQ[i].entry = new PACKET [NVM_WQ_SIZE];

            RQ[i].NAME = "NVM_RQ" + to_string(i);
            RQ[i].SIZE = NVM_RQ_SIZE;
            RQ[i].entry = new PACKET [NVM_RQ_SIZE];
        }

        fill_level = FILL_NVM;

        rq_enqueue_count = 0;
        last_enqueue_count = 0;
        epoch_enqueue_count = 0;
        next_bw_measure_cycle = 1;
        bw = 0;
    };

    // destructor
    ~NVM_MEMORY_CONTROLLER() {

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
