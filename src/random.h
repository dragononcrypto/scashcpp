// Copyright (c) 2017-2018 Scash developers
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RANDOM_H
#define BITCOIN_RANDOM_H

#include <stdint.h>

/**
 * PRNG initialized from secure entropy based RNG
 */
class DeterministicRandomGenerator
{
private:
    uint32_t nRz;
    uint32_t nRw;

public:
    DeterministicRandomGenerator();

   /**
    * MWC RNG of George Marsaglia
    * This is intended to be fast. It has a period of 2^59.3, though the
    * least significant 16 bits only have a period of about 2^30.1.
    *
    * @return random value
    */
    int64_t Next()
    {
        nRz = 36969 * (nRz & 65535) + (nRz >> 16);
        nRw = 18000 * (nRw & 65535) + (nRw >> 16);
        return ((nRw << 16) + nRz);
    }
};

#endif // BITCOIN_RANDOM_H
