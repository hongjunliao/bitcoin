/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "util.h"

#include <openssl/rand.h>

#include "uint256.h"
#include <cstdarg>
#include <set>

#include "sds/sds.h"	//sds

using std::set;

std::string strprintf(const char* format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    sds s = sdscatvprintf(sdsempty(), format, arg_ptr);
    va_end(arg_ptr);

    std::string ret = std::string(s);
    sdsfree(s);

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////


bool error(const char* format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    sds s = sdscatvprintf(sdsempty(), format, arg_ptr);
    va_end(arg_ptr);

    printf("ERROR: %s\n", s);
    sdsfree(s);
    return false;
}

void RandAddSeed(bool fPerfmon) {}

uint64 GetRand(uint64 nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64 nRange = (SIZE_MAX / nMax) * nMax;
    uint64 nRand = 0;
    do
        RAND_bytes((unsigned char*)&nRand, sizeof(nRand));
    while (nRand >= nRange);
    return (nRand % nMax);
}

/////////////////////////////////////////////////////////////////////////////////////////////

//
// "Never go to sea with two chronometers; take one or three."
// Our three chronometers are:
//  - System clock
//  - Median of other server's clocks
//  - NTP servers
//
// note: NTP isn't implemented yet, so until then we just use the median
//  of other nodes clocks to correct ours.
//

int64 GetTime()
{
    return time(NULL);
}

static int64 nTimeOffset = 0;

int64 GetAdjustedTime()
{
    return GetTime() + nTimeOffset;
}


string FormatMoney(int64 n, bool fPlus)
{
    n /= CENT;
    string str = strprintf("%I64d.%02I64d", (n > 0 ? n : -n)/100, (n > 0 ? n : -n)%100);
    for (int i = 6; i < str.size(); i += 4)
        if (isdigit(str[str.size() - i - 1]))
            str.insert(str.size() - i, 1, ',');
    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    else if (fPlus && n > 0)
        str.insert((unsigned int)0, 1, '+');
    return str;
}

void AddTimeData(unsigned int ip, int64 nTime)
{
    int64 nOffsetSample = nTime - GetTime();

    // Ignore duplicates
    static set<unsigned int> setKnown;
    if (!setKnown.insert(ip).second)
        return;

    // Add data
    static vector<int64> vTimeOffsets;
    if (vTimeOffsets.empty())
        vTimeOffsets.push_back(0);
    vTimeOffsets.push_back(nOffsetSample);
    printf("Added time data, samples %d, ip %08x, offset %+I64d (%+I64d minutes)\n", vTimeOffsets.size(), ip, vTimeOffsets.back(), vTimeOffsets.back()/60);
    if (vTimeOffsets.size() >= 5 && vTimeOffsets.size() % 2 == 1)
    {
        sort(vTimeOffsets.begin(), vTimeOffsets.end());
        int64 nMedian = vTimeOffsets[vTimeOffsets.size()/2];
        nTimeOffset = nMedian;
        if ((nMedian > 0 ? nMedian : -nMedian) > 5 * 60)
        {
            // Only let other nodes change our clock so far before we
            // go to the NTP servers
            /// todo: Get time from NTP servers, then set a flag
            ///    to make sure it doesn't get changed again
        }
        foreach(int64 n, vTimeOffsets)
            printf("%+I64d  ", n);
        printf("|  nTimeOffset = %+I64d  (%+I64d minutes)\n", nTimeOffset, nTimeOffset/60);
    }
}
