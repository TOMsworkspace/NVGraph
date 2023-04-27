//
// Created by tom on 2020/10/16.
//
#include "predictor.h"
#include <bitset>
#include "util.h"
#include <iostream>
#include "sapredictor.h"

SAPredictor::SAPredictor(const uint32_t cache_line_size, const uint32_t &length, enum stateTransfer transfer) : cache_line_size(cache_line_size), length(length)
{
    cache_line_shift = log2(cache_line_size);
    length_shift = log2(length);

    stateBloomfilter = new Bimodal[length];

    for (uint32_t i = 0; i < length; i++)
    {
        stateBloomfilter[i] = Bimodal(transfer, strongmiss);
    }
}

SAPredictor::~SAPredictor() { delete[] stateBloomfilter; }

uint32_t SAPredictor::latency() const
{
    return SBF_latency;
}

bool SAPredictor::update(const uint64_t &addr, const Cache_base &cache)
{

    uint32_t index = PredictorHashZoo::a1anda0(addr >> cache_line_shift, cache_line_shift, length_shift);

    if (cache.check_cache_hit(addr))
    {
        stateBloomfilter[index].stateTransform(Hit);
    }
    else
    {
        stateBloomfilter[index].stateTransform(Miss);

        uint64_t victimtag = cache.get_victim_tag();

        if (victimtag < ULLONG_MAX)
        {
            uint32_t victimindex = PredictorHashZoo::a1anda0(victimtag, cache_line_shift, length_shift);

            stateBloomfilter[victimindex].stateTransform(Miss);
        }
    }

    return true;
}

bool SAPredictor::predict(const uint64_t &addr, const operation &op)
{
    //   if(op==OPERATION_WRITE)
    //       return false;

    uint32_t index = PredictorHashZoo::a1anda0(addr >> cache_line_shift, cache_line_shift, length_shift);

    return stateBloomfilter[index].predict(op);
}

void SAPredictor::adaptive_transfer1(const uint64_t &history_acc, const uint64_t &addr)
{

    for (int i = 1; i <= 3; ++i)
    {
        int temp2 = history_acc & 7;
        int temp3 = history_acc & 15;

        if (temp2 == 0 || temp2 == 7)
        {
            this->setTransfer(addr, fastA);
            break;
        }

        if (temp2 == 4 || temp2 == 1)
        {
            this->setTransfer(addr, true_unfairA);
            break;
        }

        if (temp2 == 6 || temp2 == 3 || temp3 == 10)
        {
            this->setTransfer(addr, false_unfairA);
            break;
        }

        if (temp2 == 2 || temp2 == 5)
        {
            this->setTransfer(addr, rand_transferB);
            break;
        }
    }
}

void SAPredictor::adaptive_transfer2(const uint64_t &history_acc, const uint64_t &addr)
{

    // while(1) {
        
    uint64_t temp4 = history_acc & 31;

    uint64_t temp = temp4;

    if (__builtin_popcount(temp) == 1)
    {
        this->setTransfer(addr, true_unfairA);
        return;
    }

    if (__builtin_popcount(temp) == 5)
    {
        this->setTransfer(addr, false_unfairA);
        return;
    }

    if (temp == 42 || temp == 21)
    {
        this->setTransfer(addr, rand_transferA);
        return;
    }
    else
    {
        this->setTransfer(addr, fastA);
        return;
    }
    //}
}

void SAPredictor::adaptive_transfer3(const uint64_t &history_acc, const uint64_t &addr)
{

    Trie root;
    root.insert("HMHM", rand_transferA);
    root.insert("MHMH", rand_transferA);
    root.insert("HHMM", fastA);
    root.insert("MMHH", fastA);
    root.insert("HHMMM", fastA);
    root.insert("MMMHH", fastA);
    root.insert("HMM", true_unfairA);
    root.insert("MMH", true_unfairA);
    root.insert("HHM", false_unfairA);
    root.insert("MHH", false_unfairA);
    root.insert("HMMM", true_unfairA);
    root.insert("MMMH", true_unfairA);
    root.insert("HHHM", false_unfairA);
    root.insert("MHHH", false_unfairA);

    string key;

    for (int i = 0; i < 32; i++)
    {
        key += (history_acc & (1 << i)) ? 'H' : 'M';
    }

    for (int i = 0; i < 10; i++)
    {
        Trienode *node1 = root.search(key.substr(0, 5));
        Trienode *node2 = root.search(key.substr(5, 5));
        if (node1 && node2 && node1->transfer == node2->transfer)
        {
            this->setTransfer(addr, node1->transfer);
            break;
        }

        node1 = root.search(key.substr(0, 4));
        node2 = root.search(key.substr(4, 4));
        if (node1 && node2 && node1->transfer == node2->transfer)
        {
            this->setTransfer(addr, node1->transfer);
            break;
        }

        node1 = root.search(key.substr(0, 3));
        node2 = root.search(key.substr(3, 3));
        if (node1 && node2 && node1->transfer == node2->transfer)
        {
            this->setTransfer(addr, node1->transfer);
            break;
        }
    }
}

void SAPredictor::adaptive_transfer4(const bitset<1024> history_acc, const uint64_t &addr, uint32_t his_length)
{

    uint32_t predict_r = 0;
    uint32_t max_p = -1;

    int max_id = 0;

    vector<enum stateTransfer> replayTransfer = {fastA, rand_transferA, true_unfairA, false_unfairA};

    // replay every transfer
    for (uint32_t i = 0; i < replayTransfer.size(); ++i)
    {

        predict_r = 0;

        Bimodal b = Bimodal(replayTransfer[i], strongmiss);

        for (int j = his_length - 1; j >= 0; --j)
        {

            if (b.predict(READ) == (bool)history_acc[j])
                predict_r++;

            enum cacheAction action = history_acc[j] ? Hit : Miss;

            b.stateTransform(action);
        }

        // choose the highest transfer
        if (predict_r > max_p)
        {
            max_id = i;
            max_p = predict_r;
        }
    }

    for (uint32_t i = 0; i < length; ++i)
        stateBloomfilter[i].setTransfer(replayTransfer[max_id]);
}

bool SAPredictor::setTransfer(const uint64_t &addr, stateTransfer transfer)
{
    uint32_t index = PredictorHashZoo::a1anda0(addr >> cache_line_shift, cache_line_shift, length_shift);
    stateBloomfilter[index].setTransfer(transfer);

    return true;
}
