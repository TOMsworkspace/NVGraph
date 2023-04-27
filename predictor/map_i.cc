#include "map_i.h"
#include "util.h"

Map_I::Map_I(const uint32_t &cache_line_size, const uint32_t &counternum, const uint32_t &counterbit)
    : cache_line_size(cache_line_size), length(counternum), counterbit(counterbit)
{
    // ctor
    MACT = new saturate_count[length];

    length_shift = log2(length);

    for (uint32_t i = 0; i < length; i++)
    {
        MACT[i] = saturate_count(counterbit);
    }

    cache_line_shift = log2(cache_line_size);
}

Map_I::~Map_I()
{
    // dtor
    delete[] MACT;
}

bool Map_I::update(const uint64_t &addr, const Cache_base &cache)
{
    uint32_t pos = PredictorHashZoo::fold_xor(addr >> cache_line_shift, length_shift);

    if (cache.check_cache_hit(addr))
    {
        MACT[pos].decrment();
    }
    else
    {
        MACT[pos].incrment();

        uint64_t victimtag = cache.get_victim_tag();

        if (victimtag < ULLONG_MAX)
        {
            uint32_t victimpos = PredictorHashZoo::fold_xor(victimtag, length_shift);
            MACT[victimpos].incrment();
        }
    }

    return true;
}

bool Map_I::predict(const uint64_t &addr, const operation &op)
{
    // if(op==OPERATION_WRITE)
    //    return false;

    uint32_t pos = PredictorHashZoo::fold_xor(addr >> cache_line_shift, length_shift);

    // std::cout << MACT[pos].value() << std::endl;

    return !MACT[pos].MSB();
}
