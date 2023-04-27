#include "missmap.h"
#include <limits>

MissMap::MissMap(const uint32_t &cache_segement_size, const uint32_t &cache_line_size, const uint32_t length) : missmap(), cache_line_size(cache_line_size), cache_segement_size(cache_segement_size), Maxlength(length)
{
    cache_line_shift = log2(cache_line_size);

    cache_segement_shift = log2(cache_segement_size);

    posmask = (~(0xffffffffffffffff << (cache_segement_shift - cache_line_shift)));
    // ctor
}

MissMap::MissMap(const MissMap &&missmap)
{
    this->missmap = missmap.missmap;
    this->cache_line_size = missmap.cache_line_size;
    this->cache_segement_size = missmap.cache_segement_size;
    this->Maxlength = missmap.Maxlength;
    this->cache_line_shift = missmap.cache_line_shift;
    this->cache_segement_shift = missmap.cache_segement_shift;
    this->posmask = missmap.posmask;
}

bool MissMap::AddEntry(const uint64_t &SegementTag, const uint64_t &pos)
{
    if (Maxlength > missmap.size())
    {
        MissMapEntry e;
        e.set(pos);
        missmap.insert(std::pair<uint64_t, MissMapEntry>(SegementTag, e));
        return true;
    }
    return false;
}

bool MissMap::predict(const uint64_t &addr, const operation &op)
{
    // if(op==OPERATION_WRITE)
    //     return false;

    uint64_t tag = addr >> cache_segement_shift;

    uint32_t pos = posmask & (addr >> cache_line_shift);

    auto iter = missmap.find(tag);

    if (iter != missmap.end() && iter->second.get_bitmap()[pos])
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MissMap::update(const uint64_t &addr, const Cache_base &cache)
{
    uint64_t SegementTag = addr >> cache_segement_shift;
    uint32_t pos = posmask & (addr >> cache_line_shift);
    if (cache.check_cache_hit(addr))
    {
        // 缓存命中
        auto iter = missmap.find(SegementTag);

        if (iter != missmap.end())
        {
            iter->second.set(pos); // 有项,对应位置1
            return true;
        }
        else
        {
            AddEntry(SegementTag, pos); // 没有项，添加
        }

        return true;
    }
    else
    {
        auto iter = missmap.find(SegementTag);

        if (iter != missmap.end()) // 有项,对应位置1
        {
            iter->second.set(pos);
        }
        else
        {
            AddEntry(SegementTag, pos); // 没有项，添加
        }

        uint64_t victimLinetag = cache.get_victim_tag();

        if (victimLinetag < numeric_limits<uint64_t>::max())
        {
            uint64_t victimSegementTag = victimLinetag >> (cache_segement_shift - cache_line_shift);
            uint32_t victimpos = (posmask & victimLinetag);

            iter = missmap.find(victimSegementTag);

            if (iter != missmap.end() && victimLinetag < numeric_limits<uint64_t>::max())
            {
                iter->second.reset(victimpos);

                if (iter->second.check_bitmap_zero())
                {
                    missmap.erase(iter);
                }
            }
        }

        return true;
    }
}
