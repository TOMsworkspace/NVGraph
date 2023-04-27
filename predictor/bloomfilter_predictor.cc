//
// Created by tom on 2020/9/7.
//
#include <bitset>
#include <iostream>
#include <cmath>
#include "predictor.h"
#include "util.h"
#include "bloomfilter_predictor.h"


BloomFilter::BloomFilter(const uint32_t &length) : length(length), bloomfilter(new bool[length])
{
    length_shift = log2(length);

    for (uint64_t i = 0; i < length; i++)
        bloomfilter[i] = false;
}

BloomFilter::~BloomFilter() { delete[] bloomfilter; }

bool BloomFilter::set(const uint32_t &index)
{

    if (index >= length)
        return false;

    bloomfilter[index] = true;

    return true;
}

bool BloomFilter::reset(const uint32_t &index)
{

    if (index >= length)

        bloomfilter[index] = false;
    return true;
}

bool BloomFilter::check(const uint32_t &index)
{
    if (index >= length)
        return false;
    return bloomfilter[index];
}

uint64_t BloomFilter::size() const
{
    return length;
}



BloomFilterPredictor::BloomFilterPredictor(const uint32_t cache_line_size, const uint32_t &length )
    : cache_line_size(cache_line_size), length(length), bloomfilter(length)
{
    cache_line_shift = log2(cache_line_size);

    length_shift = log2(length);
}

BloomFilterPredictor::~BloomFilterPredictor() {}

bool BloomFilterPredictor::update(const uint64_t &addr, const Cache_base &cache)
{

    uint32_t index = PredictorHashZoo::a1anda0(addr >> cache_line_shift, cache_line_shift, length_shift);
    if (cache.check_cache_hit(addr))
    {
        bloomfilter.set(index);

        return true;
    }
    else
    {
        bloomfilter.set(index);

        uint64_t victimTag = cache.get_victim_tag();

        if (victimTag < ULLONG_MAX)
        {
            uint32_t victimindex = PredictorHashZoo::a1anda0(victimTag, cache_line_shift, length_shift);

            bloomfilter.reset(victimindex);
        }

        return true;
    }
}

bool BloomFilterPredictor::predict(const uint64_t &addr, const operation &op)
{
    //  if(op==OPERATION_WRITE)
    //      return false;
    uint32_t index = PredictorHashZoo::a1anda0(addr >> cache_line_shift, cache_line_shift, length_shift);

    return bloomfilter.check(index);
}

uint32_t BloomFilterPredictor::latency() const
{
    return BF_latency;
}

