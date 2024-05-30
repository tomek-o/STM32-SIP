/**
 * @file rand.c  Random generator
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <stdlib.h>
#ifdef USE_OPENSSL
#include <openssl/rand.h>
#include <openssl/err.h>
#endif
#include <re_types.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sys.h>


#define DEBUG_MODULE "rand"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifndef RELEASE
#define RAND_DEBUG 0  /**< Enable random debugging */
#endif


#if RAND_DEBUG
static bool inited = false;
/** Check random state */
#define RAND_CHECK							\
	if (!inited) {							\
		DEBUG_WARNING("%s: random not inited\n", __FUNCTION__);	\
	}
#else
#define RAND_CHECK if (0) {}
#endif


#ifdef STM32F4
#define SEED 12345
static uint32_t z1 = SEED, z2 = SEED, z3 = SEED, z4 = SEED;
static unsigned int lfsr113 (void)
{
   unsigned int b;
   b  = ((z1 << 6) ^ z1) >> 13;
   z1 = ((z1 & 4294967294UL) << 18) ^ b;
   b  = ((z2 << 2) ^ z2) >> 27;
   z2 = ((z2 & 4294967288UL) << 2) ^ b;
   b  = ((z3 << 13) ^ z3) >> 21;
   z3 = ((z3 & 4294967280UL) << 7) ^ b;
   b  = ((z4 << 3) ^ z4) >> 12;
   z4 = ((z4 & 4294967168UL) << 13) ^ b;
   return (z1 ^ z2 ^ z3 ^ z4);
}

#include "rng.h"
#endif


/**
 * Initialise random number generator
 */
int rand_init(void)
{
#if defined(STM32F4)
    printf("rand_init\n");
    HAL_StatusTypeDef status;
    status = HAL_RNG_GenerateRandomNumber(&hrng, &z1);
    if (status != HAL_OK) {
        printf("Failed to generate random seed\n");
        return status;
    }
    status = HAL_RNG_GenerateRandomNumber(&hrng, &z2);
    if (status != HAL_OK) {
        printf("Failed to generate random seed\n");
        return status;
    }
    status = HAL_RNG_GenerateRandomNumber(&hrng, &z3);
    if (status != HAL_OK) {
        printf("Failed to generate random seed\n");
        return status;
    }
    status = HAL_RNG_GenerateRandomNumber(&hrng, &z4);
    if (status != HAL_OK) {
        printf("Failed to generate random seed\n");
        return status;
    }
    /**** VERY IMPORTANT **** :
    The initial seeds z1, z2, z3, z4  MUST be larger than
    1, 7, 15, and 127 respectively.
    */
    if (z1 < 2)
        z1 = 2;
    if (z2 < 8)
        z2 = 8;
    if (z3 < 16)
        z3 = 16;
    if (z4 < 128)
        z4 = 128;

    printf("RNG seed: z1 = %lu, z2 = %lu, z3 = %lu, z4 = %lu\n", z1, z2, z3, z4);
    return status;
#else
	srand((uint32_t) tmr_jiffies());
#endif
#if RAND_DEBUG
	inited = true;
#endif
    return 0;
}


/**
 * Generate an unsigned 16-bit random value
 *
 * @return 16-bit random value
 */
uint16_t rand_u16(void)
{
#ifdef __BORLANDC__
#pragma warn -8008	// disable "Condition is always false" warning
#endif
	RAND_CHECK;
#ifdef __BORLANDC__
#pragma warn .8008	// restore default warning setting
#endif

#if defined(WIN32) || defined(__WIN32__)
	return rand();
#elif defined(STM32F4)
    return lfsr113();
#else
	/* Use higher-order bits (see man 3 rand) */
	return rand_u32() >> 16;
#endif
}


/**
 * Generate an unsigned 32-bit random value
 *
 * @return 32-bit random value
 */
uint32_t rand_u32(void)
{
	uint32_t v;
#ifdef __BORLANDC__
#pragma warn -8008	// disable "Condition is always false" warning
#endif
	RAND_CHECK;
#ifdef __BORLANDC__
#pragma warn .8008	// restore default warning setting
#endif

#ifdef USE_OPENSSL
	v = 0;
	if (RAND_bytes((unsigned char *)&v, sizeof(v)) <= 0) {
		DEBUG_WARNING("RAND_bytes() error: %u\n", ERR_get_error());
	}
#elif defined(WIN32) || defined(__WIN32__)
	v = (rand() << 16) + rand(); /* note: 16-bit rand */
#elif defined(STM32F4)
    v = lfsr113();
#else
	v = rand();
#endif

	return v;
}


/**
 * Generate an unsigned 64-bit random value
 *
 * @return 64-bit random value
 */
uint64_t rand_u64(void)
{
#ifdef __BORLANDC__
#pragma warn -8008	// disable "Condition is always false" warning
#endif
	RAND_CHECK;
#ifdef __BORLANDC__
#pragma warn .8008	// restore default warning setting
#endif
	return (uint64_t)rand_u32()<<32 | rand_u32();
}


/**
 * Generate a random printable character
 *
 * @return Random printable character
 */
char rand_char(void)
{
	char c;
#ifdef __BORLANDC__
#pragma warn -8008	// disable "Condition is always false" warning
#endif
	RAND_CHECK;
#ifdef __BORLANDC__
#pragma warn .8008	// restore default warning setting
#endif
	do {
		c = 0x30 + (rand_u16() % 0x4f);
	} while (!isalpha(c) && !isdigit(c));

	return c;
}


/**
 * Generate a string of random characters
 *
 * @param str  Pointer to string
 * @param size Size of string
 */
void rand_str(char *str, size_t size)
{
	if (!str || !size)
		return;
#ifdef __BORLANDC__
#pragma warn -8008	// disable "Condition is always false" warning
#endif
	RAND_CHECK;
#ifdef __BORLANDC__
#pragma warn .8008	// restore default warning setting
#endif

	str[--size] = '\0';
	while (size--) {
		str[size] = rand_char();
	}
}


/**
 * Generate a set of random bytes
 *
 * @param p    Pointer to buffer
 * @param size Size of buffer
 */
void rand_bytes(uint8_t *p, size_t size)
{
#ifdef USE_OPENSSL
	if (RAND_bytes(p, (int)size) <= 0) {
		DEBUG_WARNING("RAND_bytes() error: %u\n", ERR_get_error());
	}
#else
	while (size--) {
		p[size] = rand_u32();
	}
#endif
}
