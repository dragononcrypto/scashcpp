// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Lightweigh charts data class

#ifndef CHART_DATA_H
#define CHART_DATA_H

#include <vector>
#include "sync.h"

extern bool fChartsEnabled;

#define CHARTDATA_DEFAULT_SECONDS_PER_POINT 1

class ChartData
{
public:
    enum ChartAddMode {
       ChartAddMode_Replace,
       ChartAddMode_Average,
       ChartAddMode_Maximum,
       ChartAddMode_SumLocal,
       ChartAddMode_Default,
    };

   ChartData(int sizeInPoints, ChartAddMode mode = ChartAddMode_Replace)
       : mData(sizeInPoints), mPos(0)
       , mFirstChangePos(-1)
       , mLastChangePos(-1)
       , mDefaultMode(mode) { }

   void AddData(int data, ChartAddMode mode = ChartAddMode_Default);

   int GetCount() const;
   int GetData(int index); // index should be < GetCount

private:
   std::vector<int> mData;
   int mPos;
   int mFirstChangePos;
   int mLastChangePos;
   ChartAddMode mDefaultMode;
   CCriticalSection cs;
};

class Charts
{
public:
    static ChartData& NetworkInBytes();
    static ChartData& NetworkOutBytes();
    static ChartData& NetworkInConnections();
    static ChartData& NetworkOutConnections();
    static ChartData& NetworkBannedConnections();
    static ChartData& MemoryUtilization();
    static ChartData& DiskOps();
};

#endif
