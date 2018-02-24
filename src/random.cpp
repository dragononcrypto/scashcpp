// Copyright (c) 2017-2018 Scash developers
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "random.h"

#include <limits>

DeterministicRandomGenerator::DeterministicRandomGenerator()
    : nRz(11),
      nRw(11)
{
}

