// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockexplorer.h"
#include "util.h"
#include "sync.h"
#include "blockexplorerstyle.h"
#include "main.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>

namespace BlockExplorer
{

bool fBlockExplorerEnabled = false;

static CCriticalSection g_cs_blocks;
static unsigned int g_lastUpdateTime = 0;

struct BlockDataInfo
{
    std::string id;
    int height;
    unsigned int unixTs;
    bool isPoS;
};

static std::vector<BlockDataInfo> g_latestBlocksAdded;

bool BlocksContainer::BlockExplorerInit()
{
    try
    {
        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        boost::filesystem::path pathStyleFile = pathBe / Style::getStyleCssFileName();

        std::fstream fileStyle(pathStyleFile.c_str(), std::ios::out);
        fileStyle << Style::getStyleCssFileContent();
        fileStyle.close();
    }
    catch (std::exception& ex)
    {
        printf("Block explorer init failed: %s\n", ex.what());
        return false;
    }
    return true;
}

bool BlocksContainer::WriteBlockInfo(
        bool isPos,
        int height, unsigned int unixTs,
        const std::string& blockId,
        const std::string& blockContent)
{

    try
    {
        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        std::string blockFileName = "b_" + blockId + ".html";

        boost::filesystem::path pathStyleFile = pathBe / blockFileName;

        std::fstream fileBlock(pathStyleFile.c_str(), std::ios::out);
        fileBlock << blockContent;
        fileBlock.close();
    }
    catch (std::exception& ex)
    {
        printf("Write block info failed: %s\n", ex.what());
        return false;
    }

    {
        LOCK(g_cs_blocks);

        bool blockFound = false;
        for (size_t u = 0; u < g_latestBlocksAdded.size(); u++)
        {
            if (g_latestBlocksAdded[u].id == blockId)
            {
                blockFound = true;
                break;
            }
        }

        if (!blockFound)
        {
            BlockDataInfo bs;
            bs.id = blockId;
            bs.height = height;
            bs.unixTs = unixTs;
            bs.isPoS = isPos;
            g_latestBlocksAdded.insert(g_latestBlocksAdded.begin(), bs);

            while (g_latestBlocksAdded.size() > MaxLatestBlocks)
            {
                g_latestBlocksAdded.pop_back();
            }
        }
        else
        {
            printf("Duplicate of block %s tried to add to block explorer\n", blockId.c_str());
        }
    }

    return true;
}

bool BlocksContainer::WriteTransactionInfo(const std::string& blockId, const std::string& txId,
                                 const std::string& txContent)
{
    return false;
}

bool BlocksContainer::UpdateWalletInfo(const std::string walletId, const std::string txId)
{
    return false;
}

unsigned int getNowTime()
{
    return time(NULL);
}

std::string unixTimeToString(unsigned int ts)
{
    struct tm epoch_time;
    long int tsLI = ts;
    memcpy(&epoch_time, localtime(&tsLI), sizeof (struct tm));
    char res[64];
    strftime(res, sizeof(res), "%Y-%m-%d %H:%M:%S", &epoch_time);
    return res;
}


std::string unixTimeToAgeFromNow(unsigned int ts, unsigned int from)
{
    if (from <= ts) return "now";
    unsigned int diff = from - ts;
    if (diff < 60) return std::to_string(diff) + "s";
    if (diff < 60*60) return std::to_string(diff/60) + "m";
    return std::to_string(diff/60/60) + "h";
}

static const std::string searchScript = + "<script>function nav() { window.location.href=\"search?q=\" + window.document.getElementById(\"search\").value; return false; }</script>";
static const std::string searchForm = "<form id='searchForm' onSubmit='return nav();' class='form-wrapper' > "
     " <input type='text' id='search' placeholder='Search address, block, transaction, tag...' value='' width=\"588px\" required> "
     " <input style='margin-top: -1px' type='button' value='find' id='submit' onclick='return nav();'></form>";

bool  BlocksContainer::UpdateIndex(bool force)
{
    if (!force && (getTicksCountToMeasure() - g_lastUpdateTime < AutoUpdateTimeMs))
        return true;

    try
    {
        unsigned long nowTime = getNowTime();

        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        std::string blockFileName = "index.html";

        boost::filesystem::path pathStyleFile = pathBe / blockFileName;

        std::fstream fileIndex(pathStyleFile.c_str(), std::ios::out);
        fileIndex << "<html><head><title>Scash Block Explorer</title>"
                     + Style::getStyleCssLink()
                     + searchScript
                     + "</head><body>";

        fileIndex << searchForm;

        fileIndex << "<br>"; // TODO: block generation graph
                // if (fChartsEnabled || BlockExplorer::fBlockExplorerEnabled) Charts::BlocksAdded().AddData(1);

        fileIndex << "<table width=\"788px\">";
        fileIndex << "<thead><tr><th>Block hash</th><th>PoS</th><th>Height</th><th>Time</th><th>Age</th></tr></thead>";
        int upToBlock = 0;

        {
            LOCK(g_cs_blocks);
            for (size_t u = 0; u < g_latestBlocksAdded.size(); u++)
            {
                fileIndex << ((u % 2 == 0) ? "<tr class=\"even\">" : "<tr>")
                          << "<td><a href=\"b_" << g_latestBlocksAdded[u].id << ".html\">"
                            << g_latestBlocksAdded[u].id << "</a></td><td>"
                          << (g_latestBlocksAdded[u].isPoS ? "&#10004;" : "") << "</td><td>"
                          << g_latestBlocksAdded[u].height << "</td><td>"
                          << unixTimeToString(g_latestBlocksAdded[u].unixTs) << "</td><td>"
                          << unixTimeToAgeFromNow(g_latestBlocksAdded[u].unixTs, nowTime)
                          << "</td></tr>\n";
                if (u == 0) upToBlock = g_latestBlocksAdded[u].height;
            }
        }

        fileIndex << "</table>";
        fileIndex << "<br><p style=\"margin-left: auto; margin-right: auto; width: 780px\">"
                 <<"Updated at " << unixTimeToString(nowTime) << " up to block " << upToBlock << "."
                 <<"<br><br><i>Copyright &copy; 2017-2018 by Scash developers.</i></p>";

        fileIndex << "</body></html>";

        fileIndex.close();
    }
    catch (std::exception& ex)
    {
        printf("Write block index failed: %s\n", ex.what());
        return false;
    }

    g_lastUpdateTime = getTicksCountToMeasure();
    return true;
}

}
