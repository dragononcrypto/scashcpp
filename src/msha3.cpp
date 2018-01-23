// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "msha3.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <fstream>

static const uint64 PAGE_GRANULARITY = 1048576UL; // each page is 8Mb
static uint64 PAGES_COUNT = ((8UL * 1UL) / 8UL);

#define SHA3_ASSERT( x )
#if defined(_MSC_VER)
#define SHA3_TRACE( format, ...)
#define SHA3_TRACE_BUF( format, buf, l, ...)
#else
#define SHA3_TRACE(format, args...)
#define SHA3_TRACE_BUF(format, buf, l, args...)
#endif


#if defined(_MSC_VER)
#define SHA3_CONST(x) x
#else
#define SHA3_CONST(x) x##L
#endif

/* The following state definition should normally be in a separate 
 * header file 
 */

/* 'Words' here refers to uint64 */
#define SHA3_KECCAK_SPONGE_WORDS \
    (((1600)/8/*bits to byte*/)/sizeof(uint64))
typedef struct sha3_context_ {
    uint64 saved;             /* the portion of the input message that we
                                 * didn't consume yet */
    union {                     /* Keccak's state */
        uint64 s[SHA3_KECCAK_SPONGE_WORDS];
        uint8_t sb[SHA3_KECCAK_SPONGE_WORDS * 8];
    };
    unsigned byteIndex;         /* 0..7--the next byte after the set one
                                 * (starts from 0; 0--none are buffered) */
    unsigned wordIndex;         /* 0..24--the next word to integrate input
                                 * (starts from 0) */
    unsigned capacityWords;     /* the double size of the hash output in
                                 * words (e.g. 16 for Keccak 512) */
} sha3_context;

#ifndef SHA3_ROTL64
#define SHA3_ROTL64(x, y) \
    (((x) << (y)) | ((x) >> ((sizeof(uint64)*8) - (y))))
#endif

static const uint64 keccakf_rndc[24] = {
    SHA3_CONST(0x0000000000000001UL), SHA3_CONST(0x0000000000008082UL),
    SHA3_CONST(0x800000000000808aUL), SHA3_CONST(0x8000000080008000UL),
    SHA3_CONST(0x000000000000808bUL), SHA3_CONST(0x0000000080000001UL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008009UL),
    SHA3_CONST(0x000000000000008aUL), SHA3_CONST(0x0000000000000088UL),
    SHA3_CONST(0x0000000080008009UL), SHA3_CONST(0x000000008000000aUL),
    SHA3_CONST(0x000000008000808bUL), SHA3_CONST(0x800000000000008bUL),
    SHA3_CONST(0x8000000000008089UL), SHA3_CONST(0x8000000000008003UL),
    SHA3_CONST(0x8000000000008002UL), SHA3_CONST(0x8000000000000080UL),
    SHA3_CONST(0x000000000000800aUL), SHA3_CONST(0x800000008000000aUL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008080UL),
    SHA3_CONST(0x0000000080000001UL), SHA3_CONST(0x8000000080008008UL)
};

static const unsigned keccakf_rotc[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
    18, 39, 61, 20, 44
};

static const unsigned keccakf_piln[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
    14, 22, 9, 6, 1
};

#define INT64_LIM 0xFFFFFFFFFFFFFFFF
#define RED_LIMIT_START (INT64_LIM / 32) 
#define RED_PART_START (RED_LIMIT_START / 2)
#define RED_SPACE_START 65536
#define RED_PART_ADDPART 16
#define RED_SPACE_MUL 2

namespace  mSHA3 {


static uint64 reductionF(uint64 s0, uint64 s) {
	// 50% of data will fit in first page:
	if (s0 >= INT64_LIM / 2) return s % PAGE_GRANULARITY;
	// Now s0 is 0..INT64_LIM/2

	// Now distribute another 50%
	/*
	(s % SPACE)   (s0 < LIM[n])

	1GB x
	2GB xxx
	4GB xxxxxxx
	8GB xxxxxxxxxxxxxx
	16G xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	32G xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	64G xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	etc
	SPACE[0] = LIM[0] = 0

	if (s0 < LIM[n]) return s % SPACE[n];

	SPACE[n] = SPACE[n-1]*2
	LIM[n] = LIM[n-1]*2 + 1
	*/

    uint64 limit = RED_LIMIT_START;
    uint64 part = RED_PART_START;
    uint64 space = RED_SPACE_START;

	while (limit < INT64_LIM - part) {
		limit += part;

		if (s0 < limit) return (s % space);
		
		part = part + part / RED_PART_ADDPART;
		space *= RED_SPACE_MUL;
	}
	return s;
}

static uint64 sha3Mem64(uint64 s);

#define KECCAK_ROUNDS 24

/* generally called after SHA3_KECCAK_SPONGE_WORDS-ctx->capacityWords words 
 * are XORed into the state s 
 */
static void keccakfExt(uint64 s[25], bool extendedVersion)
{
    int i, j, round;
    uint64 t, bc[5];

    for(round = 0; round < KECCAK_ROUNDS; round++) {

        /* Theta */
		for (i = 0; i < 5; i++) {
			bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];
		}

        for(i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ SHA3_ROTL64(bc[(i + 1) % 5], 1);
            for(j = 0; j < 25; j += 5)
                s[j + i] ^= t;
        }

        /* Rho Pi */
        t = s[1];
        for(i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = s[j];
            s[j] = SHA3_ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        /* Chi */
        for(j = 0; j < 25; j += 5) {
            for(i = 0; i < 5; i++)
                bc[i] = s[j + i];
            for(i = 0; i < 5; i++)
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota */
        s[0] ^= keccakf_rndc[round];

		/* Zetta */
		if (extendedVersion) {
			s[0] ^= sha3Mem64(reductionF(s[1], s[0]));
		}
    }
}

/* *************************** Public Inteface ************************ */

/* For Init or Reset call these: */

void msha3_Init512(void *priv)
{
    sha3_context *ctx = (sha3_context *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 512 / (8 * sizeof(uint64));
}

void msha3_Update(void *priv, void const *bufIn, size_t len, bool extendedVersion)
{
    sha3_context *ctx = (sha3_context *) priv;

    /* 0...7 -- how much is needed to have a word */
    unsigned old_tail = (8 - ctx->byteIndex) & 7;

    size_t words;
    unsigned tail;
    size_t i;

    const uint8_t *buf = (const uint8_t*)bufIn;

    SHA3_TRACE_BUF("called to update with:", buf, len);

    SHA3_ASSERT(ctx->byteIndex < 8);
    SHA3_ASSERT(ctx->wordIndex < sizeof(ctx->s) / sizeof(ctx->s[0]));

    if(len < old_tail) {        /* have no complete word or haven't started 
                                 * the word yet */
        SHA3_TRACE("because %d<%d, store it and return", (unsigned)len,
                (unsigned)old_tail);
        /* endian-independent code follows: */
        while (len--)
            ctx->saved |= (uint64) (*(buf++)) << ((ctx->byteIndex++) * 8);
        SHA3_ASSERT(ctx->byteIndex < 8);
        return;
    }

    if(old_tail) {              /* will have one word to process */
        SHA3_TRACE("completing one word with %d bytes", (unsigned)old_tail);
        /* endian-independent code follows: */
        len -= old_tail;
        while (old_tail--)
            ctx->saved |= (uint64) (*(buf++)) << ((ctx->byteIndex++) * 8);

        /* now ready to add saved to the sponge */
        ctx->s[ctx->wordIndex] ^= ctx->saved;
        SHA3_ASSERT(ctx->byteIndex == 8);
        ctx->byteIndex = 0;
        ctx->saved = 0;
        if(++ctx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords)) {
			keccakfExt(ctx->s, extendedVersion);
            ctx->wordIndex = 0;
        }
    }

    /* now work in full words directly from input */

    SHA3_ASSERT(ctx->byteIndex == 0);

    words = len / sizeof(uint64);
    tail = (unsigned int)(len - words * sizeof(uint64));

    SHA3_TRACE("have %d full words to process", (unsigned)words);

    for(i = 0; i < words; i++, buf += sizeof(uint64)) {
        const uint64 t = (uint64) (buf[0]) |
                ((uint64) (buf[1]) << 8 * 1) |
                ((uint64) (buf[2]) << 8 * 2) |
                ((uint64) (buf[3]) << 8 * 3) |
                ((uint64) (buf[4]) << 8 * 4) |
                ((uint64) (buf[5]) << 8 * 5) |
                ((uint64) (buf[6]) << 8 * 6) |
                ((uint64) (buf[7]) << 8 * 7);
#if defined(__x86_64__ ) || defined(__i386__)
        SHA3_ASSERT(memcmp(&t, buf, 8) == 0);
#endif
        ctx->s[ctx->wordIndex] ^= t;
        if(++ctx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords)) {
			keccakfExt(ctx->s, extendedVersion);
            ctx->wordIndex = 0;
        }
    }

    SHA3_TRACE("have %d bytes left to process, save them", (unsigned)tail);

    /* finally, save the partial word */
    SHA3_ASSERT(ctx->byteIndex == 0 && tail < 8);
    while (tail--) {
        SHA3_TRACE("Store byte %02x '%c'", *buf, *buf);
        ctx->saved |= (uint64) (*(buf++)) << ((ctx->byteIndex++) * 8);
    }
    SHA3_ASSERT(ctx->byteIndex < 8);
    SHA3_TRACE("Have saved=0x%016" PRIx64 " at the end", ctx->saved);
}

void const *msha3_Finalize(void *priv, bool extendedVersion)
{
    sha3_context *ctx = (sha3_context *) priv;

    SHA3_TRACE("called with %d bytes in the buffer", ctx->byteIndex);

    /* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
     * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
     * Overall, we feed 0, then 1, and finally 1 to start padding. Without
     * M || 01, we would simply use 1 to start padding. */

    /* SHA3 version */
    ctx->s[ctx->wordIndex] ^=
            (ctx->saved ^ ((uint64) ((uint64) (0x02 | (1 << 2)) <<
                            ((ctx->byteIndex) * 8))));

    ctx->s[SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords - 1] ^=
            SHA3_CONST(0x8000000000000000UL);
	keccakfExt(ctx->s, extendedVersion);

    /* Return first bytes of the ctx->s. This conversion is not needed for
     * little-endian platforms e.g. wrap with #if !defined(__BYTE_ORDER__)
     * || !defined(__ORDER_LITTLE_ENDIAN__) || \
     * __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__ ... the conversion below ...
     * #endif */
    {
        unsigned i;
        for(i = 0; i < SHA3_KECCAK_SPONGE_WORDS; i++) {
            const unsigned t1 = (uint32_t) ctx->s[i];
            const unsigned t2 = (uint32_t) ((ctx->s[i] >> 16) >> 16);
            ctx->sb[i * 8 + 0] = (uint8_t) (t1);
            ctx->sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
            ctx->sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
            ctx->sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
            ctx->sb[i * 8 + 4] = (uint8_t) (t2);
            ctx->sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
            ctx->sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
            ctx->sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
        }
    }

    SHA3_TRACE_BUF("Hash: (first 32 bytes)", ctx->sb, 256 / 8);

    return (ctx->sb);
}

uint64** PrecomputedTable = 0;

int PrecomputeHit = 0;
int PrecomputeMiss = 0;
int Sha3MemCalls = 0;

static char sha3m_buf[256] = { 0 };
static const int UnitSize = 8; // assert == sizeof(uint64)
static const int HashSizeInUints = 8; // 512bits / 8 / sizeof(uint64)

static uint64 sha3UnMem64(uint64 s) {
	sha3_context c;
    const uint64 *hash;

    msha3_Init512(&c);
    for (size_t i = 0; i < sizeof(sha3m_buf); i += UnitSize) {
        memcpy(&sha3m_buf[i], &s, sizeof(s));
	}
    msha3_Update(&c, &sha3m_buf, sizeof(sha3m_buf), false);
    hash = (const uint64*)msha3_Finalize(&c, false);

	// Compress result into one 64bit int
    uint64 v = 0;
	for (int i = 0; i < HashSizeInUints; i++)
		v ^= hash[i];

	// Should never be zero
	if (v == 0) v = 1;

	return v;
}

static uint64 sha3Mem64(uint64 s) {
	Sha3MemCalls++;

	if (s < PAGES_COUNT * PAGE_GRANULARITY) {
		PrecomputeHit++;
		return PrecomputedTable[s / PAGE_GRANULARITY][s % PAGE_GRANULARITY];
	}

	PrecomputeMiss++;

    return sha3UnMem64(s);
}

std::string toHex(const uint8_t *buffer, int n)
{
	std::string result = "";
	for (int i = 0; i < n; i++)
	{
		char buf[4] = { 0 };
        sprintf(buf, "%02X", buffer[i]);
		result += buf[0];
		result += buf[1];
	}
	return result;
}

inline bool exists_test0(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}

static void precompute()
{
    for (size_t p = 0; p < PAGES_COUNT; p++) {

        std::string fileName = "mSHA3precomp/" + std::to_string(p) + ".bin";

        if (exists_test0(fileName))
        {
            printf("Loading hash table page %lu of %llu...\n", p, PAGES_COUNT);
            std::fstream filePrecompPage(fileName, std::ios::in | std::ios::binary);
            filePrecompPage.read((char*)PrecomputedTable[p], PAGE_GRANULARITY * sizeof(uint64));
            filePrecompPage.close();
        }
        else
        {
            printf("Filling hash table page %lu of %llu...\n", p, PAGES_COUNT);
            for (size_t j = 0; j < PAGE_GRANULARITY; j++) {
				PrecomputedTable[p][j] = sha3UnMem64(p * PAGE_GRANULARITY + j);
			}
		}

        std::fstream filePrecompPage(fileName, std::ios::out | std::ios::binary);
        filePrecompPage.write((const char*)PrecomputedTable[p], PAGE_GRANULARITY * sizeof(uint64));
        filePrecompPage.close();
	}	
}


void MeasureStart() {
    //
}

void MeasureEnd(uint64 calls) {
    //
}

void InitPrecomputedTable(uint64 pagesCount)
{
    PAGES_COUNT = pagesCount;
    PrecomputedTable = new uint64*[PAGES_COUNT];
    for (size_t i = 0; i < PAGES_COUNT; i++)
        PrecomputedTable[i] = new uint64[PAGE_GRANULARITY];
    precompute();
}

#ifdef MSHA3_TESTING
namespace  testing {


static void reductionF_verboseTest() {
    uint64 limit = RED_LIMIT_START;
    uint64 part = RED_PART_START;
    uint64 space = RED_SPACE_START;

    while (limit < INT64_LIM - part) {
        limit += part;

        printf("%llu [%llu]: %llu\n", limit, part, space);
        printf("%f%%: %lluMb\n\n", (double)limit / (double)(INT64_LIM)*100.0, (space / (1024 / 8) / 1024));

        part = part + part / RED_PART_ADDPART;
        space *= RED_SPACE_MUL;
    }
}

bool testAlgoOnTestVectors()
{
    uint8_t buf[200];
    sha3_context c;
    const uint8_t *hash;
    unsigned i;
    const uint8_t c1 = 0xa3;
	const uint8_t c2 = 0xa2;
	
	reductionF_verboseTest();
	
	MeasureStart();
    InitPrecomputedTable(PAGES_COUNT);
	MeasureEnd(PAGES_COUNT * PAGE_GRANULARITY);

    memset(buf, c1, sizeof(buf));

	/* MSHA3-512 as a empty buffer. */
	MeasureStart();

    hash = (const uint8_t*)msha3_Finalize(&c, true);
	if (toHex(hash, 64) != "4D995D6A94A433A24B7CEEF851946DC07DFC3DB6D6136355A7B43203E5FAA2FA6B758D6931DE45522307D89DF6C3139D0A52E61C8B634507E25BFFA91F7CC9C0") {
		printf("Failed to pass hash check 0 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	MeasureEnd(1);
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

    /* MSHA3-512 as a single buffer. */
    msha3_Init512(&c);
    msha3_Update(&c, buf, sizeof(buf), true);
    hash = (const uint8_t*)msha3_Finalize(&c, true);
	if (toHex(hash, 64) != "8FF228D737DB31D472A94F6AD85A508402DC21E05113274FCCC1E2EC9727B3BBE478D1032A9F2D4A807C24C698FBB401B47254E8AEDD22D8D2EE1C2EB9950E56") {
		printf("Failed to pass hash check 1 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

    /* MSHA3-512 in two steps. */
    msha3_Init512(&c);
    msha3_Update(&c, buf, sizeof(buf) / 2, true);
    msha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2, true);
    hash = (const uint8_t*)msha3_Finalize(&c, 1);
	if (toHex(hash, 64) != "8FF228D737DB31D472A94F6AD85A508402DC21E05113274FCCC1E2EC9727B3BBE478D1032A9F2D4A807C24C698FBB401B47254E8AEDD22D8D2EE1C2EB9950E56") {
		printf("Failed to pass hash check 2 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

    /* MSHA3-512 byte-by-byte: 200 steps. */
    i = 200;
    msha3_Init512(&c);
    while (i--) {
        msha3_Update(&c, &c1, 1, true);
    }
    hash = (const uint8_t*)msha3_Finalize(&c, true);
	if (toHex(hash, 64) != "8FF228D737DB31D472A94F6AD85A508402DC21E05113274FCCC1E2EC9727B3BBE478D1032A9F2D4A807C24C698FBB401B47254E8AEDD22D8D2EE1C2EB9950E56") {
		printf("Failed to pass hash check 3 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

	/* MSHA3-512 byte-by-byte: 199 steps.  */
	i = 199;
    msha3_Init512(&c);
	while (i--) {
        msha3_Update(&c, &c1, 1, true);
	}
    hash = (const uint8_t*)msha3_Finalize(&c, true);
	if (toHex(hash, 64) != "9BAACE0FEFFDC25EBA7F2C00C61493E1D08D65D28067A353F3443F6924ABCDCC619CAF2A03EDC9C50880819EEB071AD3F5A1AF8AA1EC272D056108836F85DB64") {
		printf("Failed to pass hash check 4 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

	/* MSHA3-512 byte-by-byte: 200 steps. */
	i = 200;
    msha3_Init512(&c);
	while (i--) {
        msha3_Update(&c, &c2, 1, true);
	}
    hash = (const uint8_t*)msha3_Finalize(&c, true);
	if (toHex(hash, 64) != "F2270DAEAAFEA502E134429B5FAEAE398EE75F2EA87ED214F27C775D5931F4177581C0FFF7FE8F92A305A3B593D10C4F0555211917CF829FB36001E40AC232AE") {
		printf("Failed to pass hash check 5 (%s)!\n", toHex(hash, 64).c_str());
        return false;
	}
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

	MeasureStart();
	for (unsigned int u = 0; u < 1024; u++) {
        msha3_Init512(&c);
        sprintf((char*)buf, "%u", u);
        msha3_Update(&c, buf, sizeof(buf), true);
        hash = (const uint8_t*)msha3_Finalize(&c, true);
	}
	MeasureEnd(1024);
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

	MeasureStart();
	for (unsigned int u = 1024; u < 16 * 1024; u++) {
        msha3_Init512(&c);
        sprintf((char*)buf, "%u", u);
        msha3_Update(&c, buf, sizeof(buf), true);
        hash = (const uint8_t*)msha3_Finalize(&c, true);
	}
	MeasureEnd(15 * 1024);
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);


	MeasureStart();
	for (unsigned int u = 1024; u < 16 * 1024; u++) {
        msha3_Init512(&c);
        sprintf((char*)buf, "%u%u%u%u%u%u%u%u", u, u, u, u, u, u, u, u);
        msha3_Update(&c, buf, sizeof(buf), true);
        hash = (const uint8_t*)msha3_Finalize(&c, true);
	}
	MeasureEnd(15 * 1024);
	printf("Sha3MemCalls: %i [%i]; PrecomputeHit: %i (%f %%), PrecomputeMiss: %i\n", Sha3MemCalls, Sha3MemCalls % KECCAK_ROUNDS, PrecomputeHit, (double)(PrecomputeHit) / Sha3MemCalls * 100.0, PrecomputeMiss);

    return true;
}

}
#endif

}
