/**
 * @file magic.h  Interface to magic macros
 *
 * Copyright (C) 2010 Creytiv.com
 */


//#ifndef RELEASE
#if 0
#ifndef MAGIC
#error "macro MAGIC must be defined"
#endif

/** Check magic number */
#define MAGIC_DECL uint32_t magic;
#define MAGIC_INIT(s) (s)->magic = MAGIC
#define MAGIC_CHECK(s) \
	if (MAGIC != s->magic) {					\
		DEBUG_WARNING("%s: wrong magic struct=%p (magic=0x%08x)\n", \
			      __FUNCTION__, s, s->magic);		\
		BREAKPOINT;						\
	}
#else
#define MAGIC_DECL
#define MAGIC_INIT(s)
#define MAGIC_CHECK(s) do {(void)(s);} while (0);
#endif
