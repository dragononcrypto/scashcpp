// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockexplorer.h"
#include "util.h"
#include "sync.h"
#include "blockexplorerstyle.h"
#include "main.h"
#include "base58.h"
#include "db.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>

namespace BlockExplorer
{

bool fBlockExplorerEnabled = false;

static CCriticalSection g_cs_blocks;
static unsigned int g_lastUpdateTime = 0;

static CCriticalSection g_cs_ids;
static CCriticalSection g_cs_be_fatlock;

enum ObjectTypes {
    TYPE_NONE,
    TYPE_TX,
    TYPE_BLOCK,
    TYPE_ADDRESS,
    TYPE_ANY
};

std::map<std::string, ObjectTypes> g_ids;

struct BlockDataInfo
{
    std::string id;
    int height;
    unsigned int unixTs;
    bool isPoS;
};

static std::vector<BlockDataInfo> g_latestBlocksAdded;

std::string safeEncodeFileNameWithoutExtension(const std::string& name)
{
    std::string result = "";

    for (size_t u = 0; u < name.length(); u++)
    {
        if (isalnum(name[u]))
            result += name[u];
        else
            result += "_";
    }

    return result;
}


std::string filterURLRequest(const std::string& name)
{
    std::string result = "";

    for (size_t u = 0; u < name.length(); u++)
    {
        if (isalnum(name[u]) || name[u] == '?' || name[u] == '.' || name[u] == '/')
            result += name[u];
    }

    return result;
}

void reloadKnownObjects()
{
    LOCK(g_cs_ids);

    boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
    boost::filesystem::directory_iterator end;

    for (boost::filesystem::directory_iterator i(pathBe); i != end; ++i)
    {
        const boost::filesystem::path cp = (*i);
        if (fDebug && fDumpAll)
        {
            printf("Enumerated file %s \n", cp.stem().string().c_str());
        }
        g_ids[cp.stem().string()] = TYPE_ANY;
    }
}

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

        reloadKnownObjects();
    }
    catch (std::exception& ex)
    {
        printf("Block explorer init failed: %s\n", ex.what());
        return false;
    }
    return true;
}

std::string fixupKnownObjects(const std::string& src)
{
    std::string result = "";

    std::vector<std::string> tokenized;

    std::string buffer = "";
    for (size_t u = 0; u < src.length(); u++)
    {
        if (isalnum(src[u]))
        {
            buffer += src[u];
        }
        else
        {
            tokenized.push_back(buffer);
            buffer = "";
            tokenized.push_back(std::string("") + src[u]);
        }
    }

    if (buffer != "")
        tokenized.push_back(buffer);

    {
        LOCK(g_cs_ids);
        for (size_t u = 0; u < tokenized.size(); u++)
        {
            if ((tokenized[u].length() == 64 || tokenized[u].length() == 34)
                    && g_ids.find(tokenized[u]) != g_ids.end()
                    && ((u+1 >= tokenized.size()) ||
                            (tokenized[u+1] != ".")))
            {
                result += "<a href=\"" + tokenized[u] + ".html\">"
                        + tokenized[u] + "</a>";
            }
            else
                result += tokenized[u];
        }
    }

    return result;
}

unsigned int getNowTime()
{
    return time(NULL);
}

std::string unixTimeToString(unsigned int ts)
{
    struct tm epoch_time;
    long int tsLI = ts;
    memcpy(&epoch_time, gmtime(&tsLI), sizeof (struct tm));
    char res[64];
    strftime(res, sizeof(res), "%Y-%m-%d %H:%M:%S UTC", &epoch_time);
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

std::string getHead(std::string titleAdd = "", bool addRefreshTag = false)
{
    std::string result =  "<html><head><title>Scash Block Explorer";
    if (titleAdd != "") result += " - " + titleAdd;
    result += "</title>"
         + Style::getStyleCssLink()
         + searchScript
         + (addRefreshTag ? "<meta http-equiv=\"refresh\" content=\"10\" />" : "")
         + "</head><body>"
         + searchForm;
    return result;
}

std::string getTail()
{
    return "<br><br><i>Copyright &copy; 2017-2018 by Scash developers.</i></p></body></html>";
}

enum FormattingType {
    FORMAT_TYPE_PLAIN,
    FORMAT_TYPE_CSS,
    FORMAT_TYPE_HTML,
    FORMAT_TYPE_NICE_HTML,
};

void makeObjectKnown(const std::string& id, ObjectTypes t)
{
    LOCK(g_cs_ids);
    g_ids[id] = t;
}

static const std::string Address_NoAddress = "no address";

bool addAddressTx(const std::string& fileAddress,
                  const std::string& sourceAddress, const std::string& destAddress,
                  int64 amount,
                  const std::string& txId, const std::string& blockId,
                  const std::string& txDate,
                  std::string message = "")
{
    if (Address_NoAddress != sourceAddress) makeObjectKnown(sourceAddress, TYPE_ADDRESS);
    if (Address_NoAddress != destAddress) makeObjectKnown(destAddress, TYPE_ADDRESS);

    makeObjectKnown(txId, TYPE_TX);

    if (Address_NoAddress == fileAddress) return true;

    try
    {
        if (message.length() > 140)
        {
            message = message.substr(0,137) + "...[TRIMMED]";
        }
        if (message.length() > 0)
        {
            message = "<font color=darkblue>" + message + "<font>";
        }

        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        std::string addressFileName = safeEncodeFileNameWithoutExtension(fileAddress) + ".html";
        boost::filesystem::path pathAddressFile = pathBe / addressFileName;

        bool fileIsAlreadyCreated = boost::filesystem::exists(pathAddressFile);

        std::ostringstream temp;

        if (!fileIsAlreadyCreated)
        {
            temp << "<h3 align=center><a href='" + txId + ".html'>&lt;&lt;&lt;</a>&nbsp;Details for address " << fileAddress << "</h3>";
            temp << "<table><tr><th>Param</th><th>Value</th></tr>"
                   << "<tr><td>Balance</td><td>" << "<!--dynamic:balance:0x"+fileAddress+"-->" << "</td></tr>"
                   << "<tr class=\"even\"><td>Balance confirmed</td><td>" << "<!--dynamic:balanceconfirmed:0x"+fileAddress+"-->" << "</td></tr>"
                   << "</table>";

            temp << "<p><h3 align=center>Transactions:</h3>";
            temp << "<table><tr><th>TX id</th><th>Date</th><th>From</th><th>To</th><th>Amount</th><th>State</th><th>Message</th></tr>";
        }

        std::string amountStr = std::to_string((double)amount / (double)COIN) + " SCS";
        if (amount < 0) amountStr = "<font color=darkred>" + amountStr + "</font>";
        if (amount > 0) amountStr = "<font color=darkgreen>" + amountStr + "</font>";

        std::string amountTechnical = "";
        if (amount < 0) amountTechnical = "<!--amount:minus:" + std::to_string(amount) + "-->";
        if (amount > 0) amountTechnical = "<!--amount:plus:" + std::to_string(amount) + "-->";

        temp << "<tr>"
                << "<td>" << txId << " </td>"
                << "<td>" << txDate << " </td>"
                << "<td>" << sourceAddress << "</td>"
                << "<td>" << destAddress << "</td>"
                << "<td>" << (amount ? amountStr : "-") << " " << amountTechnical << "</td>"
                << "<td>" << "<!--dynamic:blockstate:0x"+blockId+"-->" << "</td>"
                << "<td>" << message << "</td>"
                << "<tr>\n";

        std::string addressContent = temp.str();

        if (!fileIsAlreadyCreated)
        {
            std::fstream fileOut(pathAddressFile.c_str(), std::ios::out);
            fileOut << getHead("Address " + fileAddress);

            fileOut << fixupKnownObjects(addressContent);

            fileOut.close();
        }
        else
        {
            std::fstream fileOut(pathAddressFile.c_str(), std::ofstream::out | std::ofstream::app);

            fileOut << fixupKnownObjects(addressContent);

            fileOut.close();
        }
    }
    catch (std::exception& ex)
    {
        printf("Write address info failed: %s\n", ex.what());
        return false;
    }

    return true;
}

void printTxToStream(CTransaction& t, std::ostringstream& stream,
                     int height,
                     const std::string& blockId,
                     FormattingType formattingType)
{
    std::string txDate = unixTimeToString(t.nTime);

    stream << "<h3 align=center><a href='" + blockId + ".html'>&lt;&lt;&lt;</a>&nbsp;Details for transaction " << t.GetHash().ToString() << "</h3>";
    stream << "<table><tr><th>Param</th><th>Value</th></tr>"
           << "<tr><td>Status</td><td>" << "<!--dynamic:blockstate:0x"+blockId+"-->" << "</td></tr>"
           << "<tr class=\"even\"><td>Included in block</td><td>" << blockId << "</td></tr>"
           << "<tr><td>Included in block at height</td><td>" << height << "</td></tr>"
           << "<tr class=\"even\"><td>Version</td><td>" << t.nVersion << "</td></tr>"
           << "<tr><td>Time</td><td>" << txDate << "</td></tr>"
           << "<tr class=\"even\"><td>LockTime</td><td>" << t.nLockTime << "</td></tr>"
           << "<tr><td>DoS flag</td><td>" << t.nDoS << "</td></tr>"
           << "<tr class=\"even\"><td>Inputs</td><td>" << t.vin.size() << "</td></tr>"
           << "<tr><td>Outputs</td><td>" << t.vout.size() << "</td></tr>"

           << "<tr class=\"even\"><td>Value</td><td>" << (t.GetValueOut() / (double)COIN) << " SCS</td></tr>"
           << "<tr><td>CoinBase</td><td>" << (t.IsCoinBase() ? "&#10004;" : "") << "</td></tr>"

           << "<tr class=\"even\"><td>CoinStake</td><td>" << (t.IsCoinStake() ? "&#10004;" : "") << "</td></tr>"
           << "<tr><td>IsStandard</td><td>" << (t.IsStandard() ? "&#10004;" : "") << "</td></tr>"

           << "<tr class=\"even\"><td>LegacySigOpCount</td><td>" << t.GetLegacySigOpCount() << "</td></tr>"

           << "</table>";

    std::string nonZeroInputAddr = "";
    std::vector<std::string> nonZeroOutputAddrs;
    std::vector<int64> nonZeroAmountOuts;
    bool hasPoSOutputs = false;
    std::string messageSafe = "";

    if (t.HasMessage())
    {
        stream << "<br><p style=\"margin-left: auto; margin-right: auto; width: 780px\">";
        stream << "<h3 align=center>Message:</h3><table><tr><td><font color=darkblue>";
        try
        {
            messageSafe = simpleHTMLSafeDisplayFilter(t.message);
            stream << messageSafe;
        }
        catch (std::exception &ex)
        {
            printf("error while processing message: %s", ex.what());
        }

        stream << "</font></td></tr></table></p><br>";
    }

    CTxDB txdb("r");


    stream << "<p><h3 align=center>Inputs:</h3>";
    stream << "<table><tr><th>Amount</th><th>prevOut</th><th>scriptSig</th><th>nSequence</th><th>Source address</th></tr>";
    for (unsigned int i = 0; i < t.vin.size(); i++)
    {
        CTxDestination address;
        int64 amount = 0;
        bool ok = false;

        CTransaction prev;
        if (txdb.ReadDiskTx(t.vin[i].prevout.hash, prev))
        {
            if (t.vin[i].prevout.n < prev.vout.size())
            {
                const CTxOut &vout = prev.vout[t.vin[i].prevout.n];
                ok = ExtractDestination(vout.scriptPubKey, address);
                amount = vout.nValue;
            }
        }

        if (ok && (amount > 0))
        {
            nonZeroInputAddr = CBitcoinAddress(address).ToString();
        }

        stream << ((i % 2 != 0) ? "<tr>" : "<tr class=\"even\">")
                << "<td>" << (amount ? (std::to_string((double)t.vout[i].nValue / (double)COIN) + " SCS") : "") << " </td>"
                << "<td>" << t.vin[i].prevout.ToString() << "</td>"
                << "<td>" << t.vin[i].scriptSig.ToString(true) << "</td>"
                << "<td>" << t.vin[i].nSequence << "</td>"
                << "<td>" << (ok ? CBitcoinAddress(address).ToString() : "-") << "</td>"
                << "<tr>";
    }
    stream << "</table>";

    stream << "<p><h3 align=center>Outputs:</h3>";
    stream << "<table><tr><th>Amount</th><th>scriptPubKey</th><th>Destination address</th></tr>";
    for (unsigned int i = 0; i < t.vout.size(); i++)
    {
        CTxDestination address;
        bool ok = ExtractDestination(t.vout[i].scriptPubKey, address);

        if (t.vout[i].nValue && ok)
        {
            nonZeroAmountOuts.push_back(t.vout[i].nValue);
            nonZeroOutputAddrs.push_back(CBitcoinAddress(address).ToString());
        }

        if (t.vout[i].nValue == 0)
        {
            hasPoSOutputs = true;
        }

        stream << ((i % 2 != 0) ? "<tr>" : "<tr class=\"even\">")
                << "<td>" << (t.vout[i].nValue / (double)COIN) << " SCS</td>"
                << "<td>" << t.vout[i].scriptPubKey.ToString(true) << "</td>"
                << "<td>" << (ok ? CBitcoinAddress(address).ToString() : "-") << "</td>"
                << "<tr>";
    }
    stream << "</table>";

    if (nonZeroInputAddr.empty()) nonZeroInputAddr = Address_NoAddress;

    if (hasPoSOutputs)
    {
        // This is PoS Transaction
        // TODO: decide to show or not
    }
    else
    {
        for (size_t u = 0; u < nonZeroAmountOuts.size() && u < nonZeroOutputAddrs.size(); u++)
        {
            addAddressTx(nonZeroOutputAddrs[u],
                         nonZeroInputAddr, nonZeroOutputAddrs[u], nonZeroAmountOuts[u],
                         t.GetHash().ToString(), blockId, txDate, messageSafe);

            addAddressTx(nonZeroInputAddr,
                         nonZeroInputAddr, nonZeroOutputAddrs[u], -nonZeroAmountOuts[u],
                         t.GetHash().ToString(), blockId, txDate, messageSafe);
        }
    }
}

void printBlockToStream(CBlock& b, std::ostringstream& stream, int height, FormattingType formattingType)
{
    std::string skip = "\n  ";
    std::string bigskip = "\n\n";

    if (formattingType == FORMAT_TYPE_CSS)
    {
            skip = ",";
            bigskip = ",";
    }
    else if (formattingType == FORMAT_TYPE_HTML)
    {
        skip = "\n<br>&nbsp;";
        bigskip = "\n<p>";
    }

    if (formattingType == FORMAT_TYPE_NICE_HTML)
    {
        stream << "<h3 align=center><a href='index.html'>&lt;&lt;&lt;</a>&nbsp;Details for block " << b.GetHash().ToString() << "</h3>";
        stream << "<table><tr><th>Param</th><th>Value</th></tr>"
              << "<tr><td>Status</td><td>" << "<!--dynamic:blockstate:0x"+b.GetHash().ToString()+"-->" << "</td></tr>"
               << "<tr><td>Height</td><td>" << height << "</td></tr>"
               << "<tr class=\"even\"><td>Version</td><td>" << b.nVersion << "</td></tr>"
               << "<tr><td>Prev block hash</td><td>" << b.hashPrevBlock.ToString() << "</td></tr>"
               << "<tr class=\"even\"><td>Merkle root hash</td><td>" << b.hashMerkleRoot.ToString() << "</td></tr>"
               << "<tr><td>Time</td><td>" << unixTimeToString(b.nTime) << "</td></tr>"
               << "<tr class=\"even\"><td>nBits</td><td>" << b.nBits << "</td></tr>"
               << "<tr><td>Nonce</td><td>" << b.nNonce << "</td></tr>"
               << "<tr class=\"even\"><td>Transactions count</td><td>" << b.vtx.size() << "</td></tr>"
               << "<tr><td>Block signature</td><td>" << HexStr(b.vchBlockSig.begin(), b.vchBlockSig.end()) << "</td></tr>"
               << "</table>";

        stream << "<p><h3 align=center>Transactions list:</h3>";
        stream << "<table><tr><th>Transaction Id</th><th>Version</th><th>Time</th><th>Lock Time</th><th>Ins</th><th>Outs</th><th>CoinBase?</th><th>CoinStake?</th><th>Amount</th></tr>";
        for (unsigned int i = 0; i < b.vtx.size(); i++)
        {
            stream << ((i % 2 != 0) ? "<tr>" : "<tr class=\"even\">")
                    << "<td>" << b.vtx[i].GetHash().ToString() << "</td>"
                    << "<td>" << b.vtx[i].nVersion << "</td>"
                    << "<td>" << b.vtx[i].nTime << "</td>"
                    << "<td>" << b.vtx[i].nLockTime << "</td>"
                    << "<td>" << b.vtx[i].vin.size() << "</td>"
                    << "<td>" << b.vtx[i].vout.size() << "</td>"
                    << "<td>" << (b.vtx[i].IsCoinBase() ? "&#10004;" : "") << "</td>"
                    << "<td>" << (b.vtx[i].IsCoinStake() ? "&#10004;" : "") << "</td>"
                    << "<td>" << ((double)b.vtx[i].GetValueOut() / (double)COIN) << " SCS</td>"
                    << "</tr>";
        }
        stream << "</table>";

        stream << "<p><h3 align=center>Merkle tree:</h3>";
        stream << "<table><tr><th>#</th><th>Merkle Tree Hash</th></tr>";
        for (unsigned int i = 0; i < b.vMerkleTree.size(); i++)
        {
            stream << ((i % 2 != 0) ? "<tr>" : "<tr class=\"even\">")
                    << "<td>" << i << "</td>"
                    << "<td>" << b.vMerkleTree[(b.vMerkleTree.size()-1) - i].ToString() << "</td>"
                    << "<tr>";
        }
        stream << "</table>";
    }
    else
    {
        stream << "Block hash: " + b.GetHash().ToString()
               << skip << "height: " << height
               << skip << "version: " << b.nVersion
               << skip << "hashPrevBlock: " << b.hashPrevBlock.ToString()
               << skip << "hashMerkleRoot: " << b.hashMerkleRoot.ToString()
               << skip << "nTime: " << b.nTime
               << skip << "nBits: " << b.nBits
               << skip << "nNonce: " << b.nNonce
               << skip << "Transactions count: " << b.vtx.size()
               << skip << "Block signature: " << HexStr(b.vchBlockSig.begin(), b.vchBlockSig.end());

        stream << bigskip << "Transactions list:";
        for (unsigned int i = 0; i < b.vtx.size(); i++)
        {
            stream << skip << b.vtx[i].GetHash().ToString();
        }
        stream  << bigskip << "Merkle Tree:";
        for (unsigned int i = 0; i < b.vMerkleTree.size(); i++)
            stream << skip << b.vMerkleTree[i].ToString();
    }
}

bool writeBlockTransactions(int height, CBlock& block)
{
    bool result = true;

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        std::string txId = block.vtx[i].GetHash().ToString();

        try
        {
            std::ostringstream temp;
            printTxToStream(block.vtx[i], temp, height, block.GetHash().ToString(), FORMAT_TYPE_NICE_HTML);
            std::string txContent = temp.str();

            boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
            std::string txFileName = safeEncodeFileNameWithoutExtension(txId) + ".html";
            boost::filesystem::path pathTxFile = pathBe / txFileName;

            std::fstream fileBlock(pathTxFile.c_str(), std::ios::out);
            fileBlock << getHead("Transaction " + txId);

            fileBlock << fixupKnownObjects(txContent);

            fileBlock << getTail();
            fileBlock.close();
        }
        catch (std::exception& ex)
        {
            printf("Write tx info failed: %s\n", ex.what());
            result = false;
        }
    }

    return result;
}

bool BlocksContainer::WriteBlockInfo(int height, CBlock& block)
{
    LOCK(g_cs_be_fatlock);

    std::string blockId = block.GetHash().ToString();

    makeObjectKnown(blockId, TYPE_BLOCK);

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        makeObjectKnown(block.vtx[i].GetHash().ToString(), TYPE_TX);
    }

    try
    {
        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        writeBlockTransactions(height, block);

        std::string blockFileName = safeEncodeFileNameWithoutExtension(blockId) + ".html";

        std::ostringstream temp;
        printBlockToStream(block, temp, height, FORMAT_TYPE_NICE_HTML);
        std::string blockContent = temp.str();

        boost::filesystem::path pathBlockFile = pathBe / blockFileName;

        std::fstream fileBlock(pathBlockFile.c_str(), std::ios::out);
        fileBlock << getHead("Block " + blockId);

        fileBlock << fixupKnownObjects(blockContent);

        fileBlock << getTail();
        fileBlock.close();
    }
    catch (std::exception& ex)
    {
        printf("Write block info failed: %s\n", ex.what());
        return false;
    }

    // Update data for index.html
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
            bs.unixTs = block.nTime;
            bs.isPoS = block.IsProofOfStake();
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

bool  BlocksContainer::UpdateIndex(bool force)
{
    if (!force && (getTicksCountToMeasure() - g_lastUpdateTime < AutoUpdateTimeMs))
        return true;

    try
    {
        LOCK(g_cs_be_fatlock);

        unsigned long nowTime = getNowTime();

        boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
        boost::filesystem::create_directory(pathBe);

        std::string blockFileName = "index.html";

        boost::filesystem::path pathIndexFile = pathBe / blockFileName;

        std::fstream fileIndex(pathIndexFile.c_str(), std::ios::out);
        fileIndex << getHead("", true);

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
                          << "<td>" << fixupKnownObjects(g_latestBlocksAdded[u].id) << "</td><td>"
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
                 <<"Updated at " << unixTimeToString(nowTime) << " up to block " << upToBlock << ".";

        fileIndex << getTail();
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

std::string fixupDynamicStatuses(const std::string& data)
{
    // TODO
    return data;
}

// we don't wanna this thing will be DoSed right after start, so here's small hack to cutoff suspicious stuff
const int largeRequestCutoff = 100; // ms

// should be operated under g_cs_be_fatlock
std::map<std::string, int> largeRequests;

std::string BlocksContainer::GetFileDataByURL(const std::string& urlUnsafe)
{
    try
    {
        LOCK(g_cs_be_fatlock);

        std::string reqSafe = urlUnsafe;
        std::string reqPrint;

        // eat all data before /
        size_t pos = reqSafe.find('/');
        if (pos != std::string::npos) reqSafe = reqSafe.substr(pos+1);

        bool searchRequest = false;
        const std::string searchPrefix = "search?q=";
        if (reqSafe.length() > searchPrefix.length()
                && reqSafe.substr(0, searchPrefix.length()) == searchPrefix)
        {
            // this is search request
            reqSafe = reqSafe.substr(searchPrefix.length());
            searchRequest = true;
            reqPrint = reqSafe;
        }

        reqSafe = filterURLRequest(reqSafe);

        bool malformedRequest = false;

        if (reqSafe == "mystyle.css")
        {
            return Style::getStyleCssFileContent();
        }

        if (!searchRequest)
        {
            pos = reqSafe.find('.');
            if (pos > 0 && pos != std::string::npos)
            {
                reqSafe = reqSafe.substr(0, pos);
                reqPrint = reqSafe;
            }
            else
            {
                malformedRequest = true;
                if (fDebug)
                {
                    printf("Some malformed request got: %s\n", urlUnsafe.c_str());
                }
            }
        }

        if (reqSafe.length() == 0)
        {
            malformedRequest = false;
            reqSafe = "index";
        }

        if (!malformedRequest)
        {
            // try to load and parse file with base name of reqSafe
            reqSafe = safeEncodeFileNameWithoutExtension(reqSafe);
            reqSafe += ".html";

            try
            {
                unsigned int startTime = getTicksCountToMeasure();

                boost::filesystem::path pathBe = GetDataDir() / "blockexplorer";
                boost::filesystem::create_directory(pathBe);

                boost::filesystem::path pathTarget = pathBe / reqSafe;

                std::ifstream fileStream(pathTarget.string());
                std::string strData;

                fileStream.seekg(0, std::ios::end);
                strData.reserve(fileStream.tellg());
                fileStream.seekg(0, std::ios::beg);

                strData.assign((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

                std::string resultData;

                bool foundInLargeRequests = largeRequests.find(reqSafe) != largeRequests.end();
                if (foundInLargeRequests && largeRequest[reqSafe] > largeRequestCutoff)
                {
                     resultData = strData;
                }
                else
                {
                     resultData = fixupDynamicStatuses(strData);
                }

                unsigned int elapsedTime = getTicksCountToMeasure();
                if (elapsedTime > largeRequestCutoff || foundInLargeRequests)
                {
                    largeRequests[reqSafe] = elapsedTime;
                }

                return resultData;
            }
            catch (std::exception& ex)
            {
                printf("Requested file [%s] not found or some error appears: %s\n", reqSafe.c_str(), ex.what());
            }
        }

        // return generic not found page
        std::string result = getHead("Not found: " + reqSafe)
                + "<br><h3>Sorry, the requested information is not found: " + reqPrint + "</h3><br>"
                + "Return to the <a href='index.html'>index</a><br>"
                + getTail();
        return result;
    }
    catch (std::exception& ex)
    {
        printf("Request processing error: %s\n", ex.what());
        return "500 ERROR";
    }
}

}

