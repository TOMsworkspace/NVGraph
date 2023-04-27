//
// Created by tom on 2020/9/7.
//

#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "memory_class.h"
#include "cache.h"
#include "base_cache.h"

const float sys_freq = 2200000000; //hz
const float cycle_to_time = 1e9 / sys_freq; // ns / cycles; 

const uint32_t DRAM_READ_RAND = 101; //ns
const uint32_t DRAM_READ_SEQ = 81; //
const uint32_t DRAM_WRITE_SEQ = 86;
const uint32_t DRAM_WRITE_RAND = 101; 
const uint32_t NVM_READ_RAND = 165;
const uint32_t NVM_READ_SEQ = 169;
const uint32_t NVM_WRITE_RAND = 90;
const uint32_t NVM_WRITE_SEQ = 62; 

const uint32_t L1_latency=3; //3 cycles
const uint32_t L2_latency=10;
const uint32_t L3_latency=30;
const uint32_t DRAM_CACHE_latency=80;
const uint32_t NVMM_latency_read=90;
const uint32_t NVMM_latency_write=200;


const uint32_t MissMap_latency=20;
const uint32_t Map_G_latency=5;
const uint32_t Map_I_latency=10;
const uint32_t dualgrain_latency=20;
const uint32_t CBF_latency=5;
const uint32_t BF_latency=1;
const uint32_t bimodal_latency=0;
const uint32_t SBF_latency=1;
const uint32_t BCBF_latency=5;
const uint32_t CBBF_latency=5;
const uint32_t SBBF_latency=3;
const uint32_t BSBF_latency=3;
const uint32_t SBF_fusion_latency=1;

const double L1_hit_ratio=0.98; //98%
const double L2_hit_ratio=0.97;
const double L3_hit_ratio=0.96;

class Hit_Predictor
{
protected:
    uint64_t op_num;
    uint64_t hit_right;
    uint64_t miss_right;

public:

    virtual uint32_t latency() const {
        return Map_G_latency;
    }

    virtual bool update(const uint64_t& addr, const CACHE & cache)=0;  //更新

    virtual bool predict(const uint64_t &addr,const operation& op)=0;   //预测

    double predict_ratio() const{
        return 100.0*(hit_right+miss_right)/op_num;
    }
};

const double L3_average_latency=L1_hit_ratio*L1_latency
                                +(1-L1_hit_ratio)*L2_hit_ratio*(L2_latency+L1_latency)
                                +(1-L1_hit_ratio)*(1-L2_hit_ratio)*(L1_latency+L2_latency+L3_latency);

/*
static double just_DRAM_latency(const Cache & cache){
    uint64_t rcount = cache.get_r_count();
    uint64_t wcount = cache.get_w_count();

    double r_rate = rcount * 1.0 / (rcount + wcount);
    double w_rate = 1 - r_rate;

    uint32_t dram_cache_latency = DRAM_CACHE_latency + cache.get_cache_size()-1;

    double NVMM_average_latency=r_rate*(dram_cache_latency+NVMM_latency_read)+w_rate*(dram_cache_latency+NVMM_latency_write);
    double just_dram_cache_latency=L3_average_latency+dram_cache_latency*cache.get_hit_rate()+cache.get_miss_rate()*NVMM_average_latency;

    return just_dram_cache_latency;
}


static double with_Predictor_latency(const Cache & cache, const uint32_t & predictor_latency ,const double & predictor_r_ratio){

    uint64_t rcount = cache.get_r_count();
    uint64_t wcount = cache.get_w_count();

    double r_rate = rcount * 1.0 / (rcount + wcount);
    double w_rate = 1 - r_rate;

    double hit_r = cache.get_hit_rate();
    double miss_r = cache.get_miss_rate();

    uint32_t dram_cache_latency = DRAM_CACHE_latency + cache.get_cache_size()-1;

    double average_NVMM_read_latency=predictor_r_ratio*(NVMM_latency_read+predictor_latency)
                                +(1-predictor_r_ratio)*(NVMM_latency_read+DRAM_CACHE_latency+predictor_latency);


    double with_predictor_latency=L3_average_latency
            + r_rate * (
                predictor_latency
                +predictor_r_ratio * cache.get_hit_r_rate() * dram_cache_latency
                + predictor_r_ratio * (1-cache.get_hit_r_rate())* NVMM_latency_read
                + (1-predictor_r_ratio) * cache.get_hit_r_rate()* NVMM_latency_read
                +(1-predictor_r_ratio) * (1-cache.get_hit_r_rate()) * (NVMM_latency_read + dram_cache_latency))
            + w_rate * (
                cache.get_hit_w_rate() * dram_cache_latency
                + (1-cache.get_hit_w_rate())*(NVMM_latency_write + dram_cache_latency));


    return with_predictor_latency;
}
*/

#endif //CACHESIM_MASTER_PREDICTOR_H
