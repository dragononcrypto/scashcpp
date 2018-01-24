// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "chartdata.h"
#include <ctime>

static const int CHART_MAX_REMEMBER = 15 * 60; // in seconds
static const int CHART_HOUR_POINTS = CHART_MAX_REMEMBER / CHARTDATA_DEFAULT_SECONDS_PER_POINT;

static ChartData cChartNetworkInBytes(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);
static ChartData cChartNetworkOutBytes(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);
static ChartData cChartNetworkInConnections(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);
static ChartData cChartNetworkOutConnections(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);
static ChartData cChartNetworkBannedConnections(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);

static ChartData cChartMemoryUtilization(CHART_HOUR_POINTS, ChartData::ChartAddMode_Maximum);
static ChartData cDatabaseQueries(CHART_HOUR_POINTS, ChartData::ChartAddMode_Maximum);
static ChartData cDatabageAvgTime(CHART_HOUR_POINTS, ChartData::ChartAddMode_Average);
static ChartData cBlocksAdded(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);
static ChartData cBlocksRejected(CHART_HOUR_POINTS, ChartData::ChartAddMode_SumLocal);

bool fChartsEnabled = true;

ChartData& Charts::NetworkInBytes()
{
    return cChartNetworkInBytes;
}

ChartData& Charts::NetworkOutBytes()
{
    return cChartNetworkOutBytes;
}

ChartData& Charts::NetworkInConnections()
{
    return cChartNetworkInConnections;
}

ChartData& Charts::NetworkOutConnections()
{
    return cChartNetworkOutConnections;
}

ChartData& Charts::NetworkBannedConnections()
{
    return cChartNetworkBannedConnections;
}

ChartData& Charts::MemoryUtilization()
{
    return cChartMemoryUtilization;
}

ChartData& Charts::DatabaseQueries()
{
    return cDatabaseQueries;
}

ChartData& Charts::DatabaseAvgTime()
{
    return cDatabageAvgTime;
}

ChartData& Charts::BlocksAdded()
{
    return cBlocksAdded;
}

ChartData& Charts::BlocksRejected()
{
    return cBlocksRejected;
}

static int UnixTimeNowToPoint()
{
    std::time_t tm = std::time(nullptr);
    // On most POSIX-compilant systems and on Windows it return time in seconds as Int
    // However, this is not guaranteed by the language standard and
    // so should be used with care.
    unsigned long long res = static_cast<unsigned long long>(tm);
    return res / CHARTDATA_DEFAULT_SECONDS_PER_POINT;
}

void ChartData::AddData(int data, ChartAddMode mode)
{
    LOCK(cs);

    if (mode == ChartAddMode_Default)
        mode = mDefaultMode;

    int index = UnixTimeNowToPoint();

    index %= mData.size();

    if (mFirstChangePos < 0)
    {
        mFirstChangePos = index;
    }

    if (mLastChangePos != index)
    {
        mData[index] = 0;
    }

    switch (mode)
    {
    case ChartAddMode_Replace:
        mData[index] = data;
        break;
    case ChartAddMode_Maximum:
        if (data > mData[index]) mData[index] = data;
        break;
    case ChartAddMode_SumLocal:
        mData[index] += data;
        break;
    case ChartAddMode_Average:
        // This is not a corect average implementation, but its fast and for our goals lets say its suitable
        mData[index] = (int)(((float)mData[index] + (float)data) / 2.0f);
        break;
    default:
        // TODO: shout some error
        break;
    }

    mLastChangePos = index;
}

int ChartData::GetCount() const
{
    return mData.size();
}

int ChartData::GetData(int index) // index should be < GetCount
{
    LOCK(cs);
    if (index < 0 || index > GetCount()) return 0;

    // now 0 is last change point, 1 is previous, 2 is prev prev and so on
    int pos = (mLastChangePos - index + GetCount()) % GetCount();
    return mData[pos];
}
