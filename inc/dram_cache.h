//
// Created by tom on 2020/9/4.
//

#ifndef DRAM_CACHE_H
#define DRAM_CACHE_H

#include "base_cache.h"

enum cache_replacement_algo {
    CACHE_FIFO,
    CACHE_LRU,
    CACHE_RAND
};


class DRAM_Cache : public Cache_base
{
private:
    /**每个set有多少way*/
    uint32_t cache_ways;
    /**整个cache有多少组*/
    uint32_t cache_set_num;

    /**2的多少次方是set的数量，用于匹配地址时，进行位移比较*/
    uint32_t cache_set_shifts;

    /**缓存替换算法*/
    int replacement_algo;



public:

    DRAM_Cache():Cache_base(),cache_ways(4),cache_set_num(512),cache_set_shifts(2),replacement_algo(cache_replacement_algo::CACHE_LRU) {} //default LRU

    DRAM_Cache(const uint64_t & cache_size,const uint32_t & cache_line_size, enum cache_replacement_algo algo,const uint32_t& way);

    ~DRAM_Cache() {}


    /**对一个指令进行读写处理*/
    virtual void do_cache_op(const uint64_t & addr,const enum operation & op) override;

    /*
      * 检查是否命中
      * @args:
      * addr: 要判断的内存地址
      * @return:
     */
    virtual bool check_cache_hit(const uint64_t & addr) const override;


    /**获取cache当前set中空余的line*/
    virtual uint64_t get_cache_free_line(const uint64_t &addr) const;

    /**
     * 根据替换算法查找替换行
     * addr: 请求的地址
     * return: 返回受害者line号
     */

    virtual uint64_t find_victim(const uint64_t & addr) override;

    //  template<uint64_t n>
  //  friend uint64_t BloomFilterPredictor<n>::get_victim_tag(const uint64_t & addr,const DRAM_Cache& cache);


    friend std::ostream & operator << (std::ostream & os, const DRAM_Cache & set_cache);

protected:

    virtual bool read_cache(const uint64_t & line) override;

    virtual bool write_cache(const uint64_t & line) override;

    /**lock a cache line*/
    virtual int lock_cache_line(uint64_t addr) override {return addr;}
    /**unlock a cache line*/
    virtual int unlock_cache_line(uint64_t addr) override {return addr;}
    /**@return 返回miss率 */


};
#endif //DRAM_CACHE_H
