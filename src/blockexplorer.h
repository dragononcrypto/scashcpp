// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLOCKEXPLORER_MAIN_H
#define BLOCKEXPLORER_MAIN_H

#include <string>
#include <vector>

namespace BlockExplorer
{
    extern bool fBlockExplorerEnabled;

    // Singleton
    class BlocksContainer
    {
    public:
        static bool BlockExplorerInit();

        static bool WriteBlockInfo(bool isPos,
                int height, unsigned int unixTs,
                const std::string& blockId,
                const std::string& blockContent);

        static bool WriteTransactionInfo(const std::string& blockId, const std::string& txId,
                                         const std::string& txContent);
        static bool UpdateWalletInfo(const std::string walletId, const std::string txId);
        static bool UpdateIndex(bool force = false);

    private:
        static const int MaxLatestBlocks = 24;
        static const int AutoUpdateTimeMs = 10 * 1000;
    };
}

#endif // BLOCKEXPLORER_MAIN_H
