#ifndef MAP_G_H
#define MAP_G_H

#include "predictor.h"
#include <bitset>

class saturate_count
{
private:
    uint32_t countbit;
    uint32_t countmax;

    uint32_t val;

public:
    saturate_count(const unsigned int &countbit = 8) : countbit(countbit)
    {
        countmax = (1 << countbit) - 1;

        val = 0;
    }

    void incrment()
    {

        if (val < countmax)
            val++;
    }

    void decrment()
    {
        if (val > 0)
            val--;
    }

    bool resize(const unsigned int &countbit = 8)
    {
        this->countbit = countbit;
        this->countmax = (1 << countbit) - 1;
        return true;
    }

    bool MSB() const
    {
        return val >> (countbit - 1);
    }

    uint32_t value() const
    {
        return val;
    }

    uint32_t counterbit() const
    {
        return countbit;
    }
};

class Map_G : public Hit_Predictor
{
public:
    Map_G(const uint32_t &countbit = 8) : counter(countbit) {}
    virtual ~Map_G() {}

    virtual uint32_t latency() const
    {
        return Map_G_latency;
    }

    virtual bool update(const uint64_t &addr, const Cache_base &cache);   // 更新
    virtual bool predict(const uint64_t &addr, const operation &op); // 预测

protected:
private:
    saturate_count counter;
};

#endif // MAP-G_H
