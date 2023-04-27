#ifndef MISSMAP_H
#define MISSMAP_H

#include <list>
#include <map>
#include <bitset>
#include "predictor.h"

using namespace std;

class MissMapEntry
{
private:
    std::bitset<64> bitmap;

public:
    MissMapEntry(const std::bitset<64> &b = 0) : bitmap(b) {}
    virtual ~MissMapEntry() {}

    bitset<64> get_bitmap() const { return bitmap; }
    void set(uint64_t pos) { bitmap[pos] = true; }

    void reset(uint64_t pos) { bitmap[pos] = false; }
    bool check_bitmap_zero() const { return bitmap.any(); } // return 1 if there is a 1 in the bitmap
};

class MissMap : public Hit_Predictor
{
private:
    std::map<uint64_t, MissMapEntry> missmap;

    uint32_t cache_line_size;
    uint32_t cache_line_shift;

    uint32_t cache_segement_size;
    uint32_t cache_segement_shift;

    uint32_t Maxlength; // entry max length

    uint64_t posmask;

protected:
    bool AddEntry(const uint64_t &SegementTag, const uint64_t &pos);

public:
    MissMap(const uint32_t &cache_segement_size = 4 * 1024, const uint32_t &cache_line_size = 64, const uint32_t length = 128 * 1024); // 64bit+ 48bit tag Line vs 2M (128*1024 entry)

    MissMap(const MissMap &&);
    virtual ~MissMap() {}
    /*insert to dram
    the tag is in MissMap? yes-set the bit to 1?
                           no-add one entry with this tag, set the bit to 1*/

    /*evict from dram
    set the bit to 0
    all bits in bitmap are 0?yes-delete the entry
                             no-let it go*/

    virtual uint32_t latency() const
    {
        return MissMap_latency;
    }

    virtual bool update(const uint64_t &addr, const Cache_base &cache);

    virtual bool predict(const uint64_t &addr, const operation &op);
};

#endif // MISSMAP_H
