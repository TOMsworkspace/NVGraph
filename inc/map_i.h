#ifndef MAP_I_H
#define MAP_I_H

#include "predictor.h"
#include "map_g.h"

class Map_I : public Hit_Predictor
{
    public:
        Map_I(const uint32_t & cache_line_size=64,const uint32_t &  counternum=256, const uint32_t & counterbit=3);
        virtual ~Map_I();

        virtual uint32_t latency() const {
            return Map_I_latency;
        }

        virtual bool update(const uint64_t& addr, const Cache_base & cache);  //更新
        virtual bool predict(const uint64_t &addr,const operation& op);   //预测


    private:

        uint32_t cache_line_size;
        uint32_t length;
        uint32_t counterbit;
        
        saturate_count * MACT;

        uint32_t length_shift;
        uint32_t cache_line_shift;

};

#endif // MAP-I_H
