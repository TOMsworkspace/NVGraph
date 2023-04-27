//
// Created by tom on 2020/12/30.
//

#ifndef MURMURHASH3_H
#define MURMURHASH3_H


//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else	// defined(_MSC_VER)

#include "stdint.h"

#endif // !defined(_MSC_VER)

#define SEED 0x97c29b3a

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed, void * out ); //生成32位的hash值,为32位平台设计

void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );  //生成128位的hash值,为32位平台设计

void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );   //生成128位的hash值,为64位平台设计


//-----------------------------------------------------------------------------

#endif //MURMURHASH3_H
