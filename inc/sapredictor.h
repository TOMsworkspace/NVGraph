//
// Created by tom on 2020/10/16.
//

#ifndef SAPREDICTOR_H
#define SAPREDICTOR_H

#include "predictor.h"
#include <bitset>
#include "sapredictor_helper.h"
#include <iostream>

class SAPredictor : virtual public Hit_Predictor
{

private:
    uint32_t cache_line_size;
    uint32_t cache_line_shift;

    uint32_t length_shift;
    uint32_t length;

    Bimodal *stateBloomfilter;

public:
    SAPredictor(const uint32_t cache_line_size = 64, const uint32_t &length = 256, enum stateTransfer transfer = fast);

    ~SAPredictor();

    virtual uint32_t latency() const;
    bool update(const uint64_t &addr, const Cache_base &cache);
    bool predict(const uint64_t &addr, const operation &op);
    void adaptive_transfer1(const uint64_t &history_acc, const uint64_t &addr);
    void adaptive_transfer2(const uint64_t &history_acc, const uint64_t &addr);
    void adaptive_transfer3(const uint64_t &history_acc, const uint64_t &addr);
    void adaptive_transfer4(const bitset<1024> history_acc, const uint64_t &addr, uint32_t his_length);
    bool setTransfer(const uint64_t &addr, stateTransfer transfer);

};

#endif // SAPREDICTOR_H
