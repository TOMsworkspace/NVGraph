#include "map_g.h"

bool Map_G::update(const uint64_t& addr, const Cache_base & cache)  //更新
{
    if(cache.check_cache_hit(addr))
    {
        counter.decrment();
    }
    else
    {
        counter.incrment();
    }
    return true;
}

bool Map_G::predict(const uint64_t &addr, const operation& op)   //预测
{
   // if(op==OPERATION_WRITE)
   //     return false;

    return !counter.MSB();
}
