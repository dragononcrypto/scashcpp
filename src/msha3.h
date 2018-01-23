// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _MSHA3_H_
#define _MSHA3_H_ 1

#include "util.h"

// #define MSHA3_TESTING

namespace mSHA3 {
    void InitPrecomputedTable(uint64 pagesCount);

    /* Init specified buffer with defaults
     */
    static void msha3_Init512(void *priv);

    /* Calculate hashing round of priv buffer for bufIn data
     */
    static void msha3_Update(void *priv, void const *bufIn, size_t len, bool extendedVersion = false);

    /* This is simply the 'update' with the padding block.
     * The padding block is 0x01 || 0x00* || 0x80. First 0x01 and last 0x80
     * bytes are always present, but they can be the same byte.
     * Returns resulting hash buffer.
     */
    static void const *msha3_Finalize(void *priv, bool extendedVersion = false);

#ifdef MSHA3_TESTING
    namespace testing {
        static bool testAlgoOnTestVectors();
    }
#endif
}

#endif // _MSHA3_H_
