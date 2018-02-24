// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLOCKEXPLORER_MAIN_H
#define BLOCKEXPLORER_MAIN_H

#include <string>
#include <vector>
#include "main.h"

namespace BlockExplorer
{
    extern bool fBlockExplorerEnabled;

    // Singleton
    class BlocksContainer
    {
    public:
        static bool BlockExplorerInit();

        static bool WriteBlockInfo(int height, CBlock& block);

        static bool UpdateIndex(bool force = false);

        static std::string GetFileDataByURL(const std::string& url);

    private:
        static const int MaxLatestBlocks = 30;

        static const int AutoUpdateTimeMs = 3 * 1000;
    };
}

#endif // BLOCKEXPLORER_MAIN_H
