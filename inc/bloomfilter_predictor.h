//
// Created by tom on 2020/9/7.
//

#ifndef BLOOMFILTER_PREDICT_H
#define BLOOMFILTER_PREDICT_H

#include "predictor.h"

class BloomFilter
{
private:
    uint32_t length_shift;
    uint32_t length;

    bool *bloomfilter;

public:
    BloomFilter(const uint32_t &length); 

    ~BloomFilter();

    bool set(const uint32_t &index);

    bool reset(const uint32_t &index);
    
    bool check(const uint32_t &index);

    uint64_t size() const;

};

class BloomFilterPredictor : public Hit_Predictor
{

private:
    uint32_t cache_line_size;

    // uint32_t cache_index; //组相连组数
    // uint32_t cache_index_shifts;
    uint32_t length;

    class BloomFilter bloomfilter;

    uint32_t length_shift;
    uint32_t cache_line_shift;

public:
    BloomFilterPredictor(const uint32_t cache_line_size = 64, const uint32_t &length = 128 * 8 * 1024);

    ~BloomFilterPredictor();

    bool update(const uint64_t &addr, const Cache_base &cache);

    bool predict(const uint64_t &addr, const operation &op);
    
    virtual uint32_t latency() const;

};

#endif // CACHESIM_MASTER_BLOOMFILTER_PREDICT_H
