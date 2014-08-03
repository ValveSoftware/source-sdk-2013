//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Utility to interrogate and modify the data in the OSX IPC Server
//
// $NoKeywords: $
//=============================================================================
// README:README
//
// This file implements the --wrap for ld on linux that lets file i/o api's
// behave as if it were running on a case insensitive file system.  Unfortunately,
// this is needed by both steam2 and steam3.  It was decided to check the source
// into both locations, otherwise someone would find the .o and have no idea
// where to go for the source if it was in the 'other' tree.  Also, because this
// needs to be linked into every elf binary, the .o is checked in for Steam3 so that it is
// always available.  In Steam2 it sits with the PosixWin32.cpp implementation and gets
// compiled along side of it through the make system.  If you are reading this in Steam3,
// you will probably want to actually make your changes in steam2 and do a baseless merge
// to the steam3 copy.
//
// HOWTO: Add a new function.  Add the function with _WRAP to the makefiles as noted below.
// Add the implementation to pathmatch.cpp - probably mimicking the existing functions.
// Build steam2 and copy to matching steam3/client.  Take the pathmatch.o from steam 2
// and check it in to steam3 (in the location noted below).  Full rebuild (re-link really)
// of steam3.  Test steam and check in.
//
// If you are looking at updating this file, please update the following as needed:
//
// STEAM2.../Projects/GazelleProto/Client/Engine/obj/RELEASE_NORMAL/libsteam_linux/Common/Misc/pathmatch.o
//          This is where steam2 builds the pathmatch.o out to.
//
// STEAM2.../Projects/GazelleProto/Makefile.shlib.base - contains _WRAP references
// STEAM2.../Projects/Common/Misc/pathmatch.cpp - Where the source is checked in, keep in sync with:
// STEAM3.../src/common/pathmatch.cpp - should be identical to previous file, but discoverable in steam3.
// STEAM3.../src/lib/linux32/release/pathmatch.o - steam3 checked in version
// STEAM3.../src/devtools/makefile_base_posix.mak - look for the _WRAP references

#ifdef LINUX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <utime.h>
#include <map>
#include <string>
#include <time.h>

// Enable to do pathmatch caching. Beware: this code isn't threadsafe.
// #define DO_PATHMATCH_CACHE

#ifdef UTF8_PATHMATCH
#define strcasecmp utf8casecmp
#endif

static bool s_bShowDiag;
#define DEBUG_MSG( ... ) if ( s_bShowDiag ) fprintf( stderr, ##__VA_ARGS__ )
#define DEBUG_BREAK() __asm__ __volatile__ ( "int $3" )
#define _COMPILE_TIME_ASSERT(pred) switch(0){case 0:case pred:;}

#define WRAP( fn, ret, ... ) \
	ret __real_##fn(__VA_ARGS__); \
	ret __wrap_##fn(__VA_ARGS__)

#define CALL( fn ) __real_##fn

// Needed by pathmatch code
extern "C" int __real_access(const char *pathname, int mode);
extern "C" DIR *__real_opendir(const char *name);


// UTF-8 work from PhysicsFS: http://icculus.org/physfs/
//  Even if it wasn't under the zlib license, Ryan wrote all this code originally.

#define UNICODE_BOGUS_CHAR_VALUE 0xFFFFFFFF
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

inline __attribute__ ((always_inline)) static uint32_t utf8codepoint(const char **_str)
{
	const char *str = *_str;
	uint32_t retval = 0;
	uint32_t octet = (uint32_t) ((uint8_t) *str);
	uint32_t octet2, octet3, octet4;

	if (octet == 0)  // null terminator, end of string.
		return 0;

	else if (octet < 128)  // one octet char: 0 to 127
	{
		(*_str)++;  // skip to next possible start of codepoint.
		return octet;
	}

	else if ((octet > 127) && (octet < 192))  // bad (starts with 10xxxxxx).
	{
		// Apparently each of these is supposed to be flagged as a bogus
		//  char, instead of just resyncing to the next valid codepoint.
		(*_str)++;  // skip to next possible start of codepoint.
		return UNICODE_BOGUS_CHAR_VALUE;
	}

	else if (octet < 224)  // two octets
	{
		octet -= (128+64);
		octet2 = (uint32_t) ((uint8_t) *(++str));
		if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		*_str += 2;  // skip to next possible start of codepoint.
		retval = ((octet << 6) | (octet2 - 128));
		if ((retval >= 0x80) && (retval <= 0x7FF))
			return retval;
	}

	else if (octet < 240)  // three octets
	{
		octet -= (128+64+32);
		octet2 = (uint32_t) ((uint8_t) *(++str));
		if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet3 = (uint32_t) ((uint8_t) *(++str));
		if ((octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		*_str += 3;  // skip to next possible start of codepoint.
		retval = ( ((octet << 12)) | ((octet2-128) << 6) | ((octet3-128)) );

		// There are seven "UTF-16 surrogates" that are illegal in UTF-8.
		switch (retval)
		{
			case 0xD800:
			case 0xDB7F:
			case 0xDB80:
			case 0xDBFF:
			case 0xDC00:
			case 0xDF80:
			case 0xDFFF:
				return UNICODE_BOGUS_CHAR_VALUE;
		}

		// 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
		if ((retval >= 0x800) && (retval <= 0xFFFD))
			return retval;
	}

	else if (octet < 248)  // four octets
	{
		octet -= (128+64+32+16);
		octet2 = (uint32_t) ((uint8_t) *(++str));
		if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet3 = (uint32_t) ((uint8_t) *(++str));
		if ((octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet4 = (uint32_t) ((uint8_t) *(++str));
		if ((octet4 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		*_str += 4;  // skip to next possible start of codepoint.
		retval = ( ((octet << 18)) | ((octet2 - 128) << 12) |
		           ((octet3 - 128) << 6) | ((octet4 - 128)) );
		if ((retval >= 0x10000) && (retval <= 0x10FFFF))
			return retval;
	}

	// Five and six octet sequences became illegal in rfc3629.
	//  We throw the codepoint away, but parse them to make sure we move
	//  ahead the right number of bytes and don't overflow the buffer.

	else if (octet < 252)  // five octets
	{
		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		*_str += 5;  // skip to next possible start of codepoint.
		return UNICODE_BOGUS_CHAR_VALUE;
	}

	else  // six octets
	{
		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		octet = (uint32_t) ((uint8_t) *(++str));
		if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			return UNICODE_BOGUS_CHAR_VALUE;

		*_str += 6;  // skip to next possible start of codepoint.
		return UNICODE_BOGUS_CHAR_VALUE;
	}

	return UNICODE_BOGUS_CHAR_VALUE;
}

typedef struct CaseFoldMapping
{
	uint32_t from;
	uint32_t to0;
	uint32_t to1;
	uint32_t to2;
} CaseFoldMapping;

typedef struct CaseFoldHashBucket
{
	const uint8_t count;
	const CaseFoldMapping *list;
} CaseFoldHashBucket;

#include "pathmatch_casefolding.h"

inline __attribute__ ((always_inline)) static void locate_case_fold_mapping(const uint32_t from, uint32_t *to)
{
	const uint8_t hashed = ((from ^ (from >> 8)) & 0xFF);
	const CaseFoldHashBucket *bucket = &case_fold_hash[hashed];
	const CaseFoldMapping *mapping = bucket->list;
	uint32_t i;

	for (i = 0; i < bucket->count; i++, mapping++)
	{
		if (mapping->from == from)
		{
			to[0] = mapping->to0;
			to[1] = mapping->to1;
			to[2] = mapping->to2;
			return;
		}
	}

	// Not found...there's no remapping for this codepoint.
	to[0] = from;
	to[1] = 0;
	to[2] = 0;
}

inline __attribute__ ((always_inline)) static uint32_t *fold_utf8(const char *str)
{
	uint32_t *retval = new uint32_t[(strlen(str) * 3) + 1];
	uint32_t *dst = retval;
	while (*str)
	{
		const char ch = *str;
		if (ch & 0x80)  // high bit set? UTF-8 sequence!
		{
			uint32_t fold[3];
			locate_case_fold_mapping(utf8codepoint(&str), fold);
			*(dst++) = fold[0];
			if (fold[1])
			{
				*(dst++) = fold[1];
				if (fold[2])
					*(dst++) = fold[2];
			}
		}
		else  // simple ASCII test.
		{
			*(dst++) = (uint32_t) (((ch >= 'A') && (ch <= 'Z')) ? ch + 32 : ch);
			str++;
		}
	}
	*dst = 0;
	return retval;
}

inline __attribute__ ((always_inline)) static int utf8casecmp_loop(const uint32_t *folded1, const uint32_t *folded2)
{
	while (true)
	{
		const uint32_t ch1 = *(folded1++);
		const uint32_t ch2 = *(folded2++);
		if (ch1 < ch2)
			return -1;
		else if (ch1 > ch2)
			return 1;
		else if (ch1 == 0)
			return 0;  // complete match.
	}
}

static int utf8casecmp(const char *str1, const char *str2)
{
	uint32_t *folded1 = fold_utf8(str1);
	uint32_t *folded2 = fold_utf8(str2);
	const int retval = utf8casecmp_loop(folded1, folded2);
	delete[] folded1;
	delete[] folded2;
	return retval;
}

// Simple object to help make sure a DIR* from opendir
// gets closed when it goes out of scope.
class CDirPtr
{
public:
    CDirPtr() { m_pDir = NULL; }
    CDirPtr( DIR *pDir ) : m_pDir(pDir) {}
    ~CDirPtr() { Close(); }

    void operator=(DIR *pDir) { Close(); m_pDir = pDir; }

    operator DIR *() { return m_pDir; }
    operator bool() { return m_pDir != NULL; }
private:

    void Close() { if ( m_pDir ) closedir( m_pDir ); }

    DIR *m_pDir;
};

// Object used to temporarily slice a path into a smaller componentent
// and then repair it when going out of scope. Typically used as an unnamed
// temp object that is a parameter to a function.
class CDirTrimmer
{
public:
    CDirTrimmer( char * pPath, size_t nTrimIdx )
    {
        m_pPath = pPath;
        m_idx = nTrimIdx;
        m_c = m_pPath[nTrimIdx];
        m_pPath[nTrimIdx] = '\0';
    }
    ~CDirTrimmer() { m_pPath[m_idx] = m_c; }

    operator const char *() { return m_pPath; }

private:
    size_t m_idx;
    char *m_pPath;
    char m_c;
};


enum PathMod_t
{
	kPathUnchanged,
	kPathLowered,
	kPathChanged,
	kPathFailed,
};

static bool Descend( char *pPath, size_t nStartIdx, bool bAllowBasenameMismatch, size_t nLevel = 0 )
{
	DEBUG_MSG( "(%zu) Descend: %s, (%s), %s\n", nLevel, pPath, pPath+nStartIdx, bAllowBasenameMismatch ? "true" : "false " );
	// We assume up through nStartIdx is valid and matching
	size_t nNextSlash = nStartIdx+1;

	// path might be a dir
	if ( pPath[nNextSlash] == '\0' )
	{
		return true;
	}

	bool bIsDir = false; // is the new component a directory for certain?
	while ( pPath[nNextSlash] != '\0' && pPath[nNextSlash] != '/' )
	{
		nNextSlash++;
	}

	// Modify the pPath string
	if ( pPath[nNextSlash] == '/' )
		bIsDir = true;

	// See if we have an immediate match
	if ( __real_access( CDirTrimmer(pPath, nNextSlash), F_OK ) == 0 )
	{
		if ( !bIsDir )
			return true;

		bool bRet = Descend( pPath, nNextSlash, bAllowBasenameMismatch, nLevel+1 );
		if ( bRet )
			return true;
	}

	// Start enumerating dirents
	CDirPtr spDir;
	if ( nStartIdx )
	{
		// we have a path
		spDir = __real_opendir( CDirTrimmer( pPath, nStartIdx ) );
		nStartIdx++;
	}
	else
	{
		// we either start at root or cwd
		const char *pRoot = ".";
		if ( *pPath == '/' )
		{
		    pRoot = "/";
		    nStartIdx++;
		}
		spDir = __real_opendir( pRoot );
	}

    errno = 0;
    struct dirent *pEntry = spDir ? readdir( spDir ) : NULL;
    char *pszComponent = pPath + nStartIdx;
    size_t cbComponent = nNextSlash - nStartIdx;
    while ( pEntry )
    {
        DEBUG_MSG( "\t(%zu) comparing %s with %s\n", nLevel, pEntry->d_name, (const char *)CDirTrimmer(pszComponent, cbComponent) );

        // the candidate must match the target, but not be a case-identical match (we would
        // have looked there in the short-circuit code above, so don't look again)
        bool bMatches = ( strcasecmp( CDirTrimmer(pszComponent, cbComponent), pEntry->d_name ) == 0 &&
                          strcmp( CDirTrimmer(pszComponent, cbComponent), pEntry->d_name ) != 0 );

        if ( bMatches )
        {
            char *pSrc = pEntry->d_name;
            char *pDst = &pPath[nStartIdx];
            // found a match; copy it in.
            while ( *pSrc && (*pSrc != '/') )
            {
                *pDst++ = *pSrc++;
            }

            if ( !bIsDir )
                return true;

            if ( Descend( pPath, nNextSlash, bAllowBasenameMismatch, nLevel+1 ) )
                return true;

            // If descend fails, try more directories
        }
        pEntry = readdir( spDir );
    }

    if ( bIsDir )
    {
        DEBUG_MSG( "(%zu) readdir failed to find '%s' in '%s'\n", nLevel, (const char *)CDirTrimmer(pszComponent, cbComponent), (const char *)CDirTrimmer( pPath, nStartIdx ) );
    }

	// Sometimes it's ok for the filename portion to not match
	// since we might be opening for write.  Note that if
	// the filename matches case insensitive, that will be
	// preferred over preserving the input name
	if ( !bIsDir && bAllowBasenameMismatch )
		return true;

	return false;
}

#ifdef DO_PATHMATCH_CACHE
typedef std::map<std::string, std::pair<std::string, time_t> > resultCache_t;
typedef std::map<std::string, std::pair<std::string, time_t> >::iterator resultCacheItr_t;
static resultCache_t resultCache;
static const int k_cMaxCacheLifetimeSeconds = 2;
#endif // DO_PATHMATCH_CACHE

PathMod_t pathmatch( const char *pszIn, char **ppszOut, bool bAllowBasenameMismatch, char *pszOutBuf, size_t OutBufLen )
{
	// Path matching can be very expensive, and the cost is unpredictable because it
	// depends on how many files are in directories on a user's machine. Therefore
	// it should be disabled whenever possible, and only enabled in environments (such
	// as running with loose files such as out of Perforce) where it is needed.
	static const char *s_pszPathMatchEnabled = getenv("ENABLE_PATHMATCH");
	if ( !s_pszPathMatchEnabled )
		return kPathUnchanged;

	static const char *s_pszDbgPathMatch = getenv("DBG_PATHMATCH");

	s_bShowDiag = ( s_pszDbgPathMatch != NULL );

	*ppszOut = NULL;

	if ( __real_access( pszIn, F_OK ) == 0 )
		return kPathUnchanged;

#ifdef DO_PATHMATCH_CACHE
	resultCacheItr_t cachedResult = resultCache.find( pszIn );
	if ( cachedResult != resultCache.end() )
	{
		unsigned int age = time( NULL ) - cachedResult->second.second;
		const char *pszResult = cachedResult->second.first.c_str(); 
		if ( pszResult[0] != '\0' || age <= k_cMaxCacheLifetimeSeconds )
		{
			if ( pszResult[0] != '\0' )
			{
				*ppszOut = strdup( pszResult );
				DEBUG_MSG( "Cached '%s' -> '%s'\n", pszIn, *ppszOut );
				return kPathChanged;
			}
			else
			{
				DEBUG_MSG( "Cached '%s' -> kPathFailed\n", pszIn );
				return kPathFailed;
			}
		}
		else if ( age <= k_cMaxCacheLifetimeSeconds )
		{
			DEBUG_MSG( "Rechecking '%s' - cache is %u seconds old\n", pszIn, age );			
		}
	}
#endif // DO_PATHMATCH_CACHE

	char *pPath;
	if( strlen( pszIn ) >= OutBufLen )
	{
		pPath = strdup( pszIn );
	}
	else
	{
		strncpy( pszOutBuf, pszIn, OutBufLen );
		pPath = pszOutBuf;
	}

	if ( pPath )
	{
		// I believe this code is broken. I'm guessing someone wanted to avoid lowercasing
		//	the path before the steam directory - but it's actually skipping lowercasing
		//	whenever steam is found anywhere - including the filename. For example, 
		//	  /home/mikesart/valvesrc/console/l4d2/game/left4dead2_dlc1/particles/steam_fx.pcf
		//	winds up only having the "steam_fx.pcf" portion lowercased.
#ifdef NEVER
		// optimization, if the path contained steam somewhere
		// assume the path up through the component with 'steam' in
		// is valid (because we almost certainly obtained it
		// progamatically
		char *p = strcasestr( pPath, "steam" );
		if ( p )
		{
			while ( p > pPath )
			{
				if ( p[-1] == '/' )
					break;
				p--;
			}

			if ( ( p == pPath+1 ) && ( *pPath != '/' ) )
				p = pPath;
		}
		else
		{
			p = pPath;
		}
#else
		char *p = pPath;
#endif

		// Try the lower casing of the remaining path
		char *pBasename = p;
		while ( *p )
		{
			if ( *p == '/' )
				pBasename = p+1;

			*p = tolower(*p);
			p++;
		}
		if ( __real_access( pPath, F_OK ) == 0 )
		{
			*ppszOut = pPath;
			DEBUG_MSG( "Lowered '%s' -> '%s'\n", pszIn, pPath );
			return kPathLowered;
		}

		// path didn't match lowered successfully, restore the basename
		// if bAllowBasenameMismatch was true
		if ( bAllowBasenameMismatch )
		{
			const char *pSrc = pszIn + (pBasename - pPath);
			while ( *pBasename )
			{
				*pBasename++ = *pSrc++;
			}
		}

		if ( s_pszDbgPathMatch && strcasestr( s_pszDbgPathMatch, pszIn ) )
		{
			DEBUG_MSG( "Breaking '%s' in '%s'\n", pszIn, s_pszDbgPathMatch );
			DEBUG_BREAK();
		}

		bool bSuccess = Descend( pPath, 0, bAllowBasenameMismatch );
		if ( bSuccess )
		{
			*ppszOut = pPath;
			DEBUG_MSG( "Matched '%s' -> '%s'\n", pszIn, pPath );
		}
		else
		{
			DEBUG_MSG( "Unmatched %s\n", pszIn );
		}

#ifndef DO_PATHMATCH_CACHE
		return bSuccess ? kPathChanged : kPathFailed;
#else
		time_t now = time(NULL);
		if ( bSuccess )
		{
			resultCache[ pszIn ] = std::make_pair( *ppszOut, now ); 
			return kPathChanged;
		}
		else
		{
			resultCache[ pszIn ] = std::make_pair( "", now ); 
			return kPathFailed;			
		}
#endif
	}
	return kPathFailed;
}

// Wrapper object that manages the 'typical' usage cases of pathmatch()
class CWrap
{
public:
	CWrap( const char *pSuppliedPath, bool bAllowMismatchedBasename )
		: m_pSuppliedPath( pSuppliedPath ), m_pBestMatch( NULL )
	{
	    m_eResult = pathmatch( m_pSuppliedPath, &m_pBestMatch, bAllowMismatchedBasename, m_BestMatchBuf, sizeof( m_BestMatchBuf ) );
		if ( m_pBestMatch == NULL )
		{
			m_pBestMatch = const_cast<char*>( m_pSuppliedPath );
		}
	}

	~CWrap()
	{
		if ( ( m_pBestMatch != m_pSuppliedPath ) && ( m_pBestMatch != m_BestMatchBuf ) )
			free( m_pBestMatch );
	}

	const char *GetBest() const { return m_pBestMatch; }
	const char *GetOriginal() const { return m_pSuppliedPath; }
	PathMod_t GetMatchResult() const { return m_eResult; }

	operator const char*() { return GetBest(); }

private:
	const char *m_pSuppliedPath;
	char *m_pBestMatch;
	char m_BestMatchBuf[ 512 ];
	PathMod_t m_eResult;
};

#ifdef MAIN_TEST
void usage()
{
    puts("pathmatch [options] <path>");
    //puts("options:");
    //puts("\t");

    exit(-1);
}

void test( const char *pszFile, bool bAllowBasenameMismatch )
{
    char *pNewPath;
	char NewPathBuf[ 512 ];
    PathMod_t nStat = pathmatch( pszFile, &pNewPath, bAllowBasenameMismatch, NewPathBuf, sizeof( NewPathBuf ) );

    printf("AllowMismatchedBasename: %s\n", bAllowBasenameMismatch ? "true" : "false" );
    printf("Path Was: ");
    switch ( nStat )
    {
    case kPathUnchanged:
        puts("kPathUnchanged");
        break;
    case kPathLowered:
        puts("kPathLowered");
        break;
    case kPathChanged:
        puts("kPathChanged");
        break;
    case kPathFailed:
        puts("kPathFailed");
        break;
    }

    printf(" Path In: %s\n", pszFile );
    printf("Path Out: %s\n",  nStat == kPathUnchanged ? pszFile : pNewPath );

    if ( pNewPath )
        free( pNewPath );
}

int
main(int argc, char **argv)
{
    if ( argc <= 1 || argc > 2 )
        usage();

    test( argv[1], false );
    test( argv[1], true );

    return 0;
}
#endif

extern "C" {

	WRAP(freopen, FILE *, const char *path, const char *mode, FILE *stream)
	{
		// if mode does not have w, a, or +, it's open for read.
		bool bAllowBasenameMismatch = strpbrk( mode, "wa+" ) != NULL;
		CWrap mpath( path, bAllowBasenameMismatch );

		return CALL(freopen)( mpath, mode, stream );
	}

	WRAP(fopen, FILE *, const char *path, const char *mode)
	{
		// if mode does not have w, a, or +, it's open for read.
		bool bAllowBasenameMismatch = strpbrk( mode, "wa+" ) != NULL;
		CWrap mpath( path, bAllowBasenameMismatch );

		return CALL(fopen)( mpath, mode );
	}


	WRAP(fopen64, FILE *, const char *path, const char *mode)
	{
		// if mode does not have w, a, or +, it's open for read.
		bool bAllowBasenameMismatch = strpbrk( mode, "wa+" ) != NULL;
		CWrap mpath( path, bAllowBasenameMismatch );

		return CALL(fopen64)( mpath, mode );
	}

	WRAP(open, int, const char *pathname, int flags, mode_t mode)
	{
		bool bAllowBasenameMismatch = ((flags & (O_WRONLY | O_RDWR)) != 0);
		CWrap mpath( pathname, bAllowBasenameMismatch );
		return CALL(open)( mpath, flags, mode );
	}

	WRAP(open64, int, const char *pathname, int flags, mode_t mode)
	{
		bool bAllowBasenameMismatch = ((flags & (O_WRONLY | O_RDWR)) != 0);
		CWrap mpath( pathname, bAllowBasenameMismatch );
		return CALL(open64)( mpath, flags, mode );
	}

	int __wrap_creat(const char *pathname, mode_t mode)
	{
		return __wrap_open( pathname, O_CREAT|O_WRONLY|O_TRUNC, mode );
	}

	int __wrap_access(const char *pathname, int mode)
	{
		return __real_access( CWrap( pathname, false ), mode );
	}

	WRAP(stat, int, const char *path, struct stat *buf)
	{
		return CALL(stat)( CWrap( path, false ), buf );
	}

	WRAP(lstat, int, const char *path, struct stat *buf)
	{
		return CALL(lstat)( CWrap( path, false ), buf );
	}

	WRAP(scandir, int, const char *dirp, struct dirent ***namelist,
		 int (*filter)(const struct dirent *),
		 int (*compar)(const struct dirent **, const struct dirent **))
	{
		return CALL(scandir)( CWrap( dirp, false ), namelist, filter, compar );
	}

	WRAP(opendir, DIR*, const char *name)
	{
		return CALL(opendir)( CWrap( name, false ) );
	}

    WRAP(__xstat, int, int __ver, __const char *__filename, struct stat *__stat_buf)
    {
        return CALL(__xstat)( __ver, CWrap( __filename, false), __stat_buf );
    }

    WRAP(__lxstat, int, int __ver, __const char *__filename, struct stat *__stat_buf)
    {
        return CALL(__lxstat)( __ver, CWrap( __filename, false), __stat_buf );
    }

    WRAP(__xstat64, int, int __ver, __const char *__filename, struct stat *__stat_buf)
    {
        return CALL(__xstat64)( __ver, CWrap( __filename, false), __stat_buf );
    }

    WRAP(__lxstat64, int, int __ver, __const char *__filename, struct stat *__stat_buf)
    {
        return CALL(__lxstat64)( __ver, CWrap( __filename, false), __stat_buf );
    }

	WRAP(chmod, int, const char *path, mode_t mode)
	{
        return CALL(chmod)( CWrap( path, false), mode );
	}

	WRAP(chown, int, const char *path, uid_t owner, gid_t group)
	{
        return CALL(chown)( CWrap( path, false), owner, group );
	}

	WRAP(lchown, int, const char *path, uid_t owner, gid_t group)
	{
        return CALL(lchown)( CWrap( path, false), owner, group );
	}

	WRAP(symlink, int, const char *oldpath, const char *newpath)
	{
        return CALL(symlink)( CWrap( oldpath, false), CWrap( newpath, true ) );
	}

	WRAP(link, int, const char *oldpath, const char *newpath)
	{
        return CALL(link)( CWrap( oldpath, false), CWrap( newpath, true ) );
	}

	WRAP(mknod, int, const char *pathname, mode_t mode, dev_t dev)
	{
        return CALL(mknod)( CWrap( pathname, true), mode, dev );
	}

	WRAP(mount, int, const char *source, const char *target,
		 const char *filesystemtype, unsigned long mountflags,
		 const void *data)
	{
		return CALL(mount)( CWrap( source, false ), CWrap( target, false ), filesystemtype, mountflags, data );
	}

	WRAP(unlink, int, const char *pathname)
	{
        return CALL(unlink)( CWrap( pathname, false ) );
	}

	WRAP(mkfifo, int, const char *pathname, mode_t mode)
	{
        return CALL(mkfifo)( CWrap( pathname, true ), mode );
	}

	WRAP(rename, int, const char *oldpath, const char *newpath)
	{
        return CALL(rename)( CWrap( oldpath, false), CWrap( newpath, true ) );
	}

	WRAP(utime, int, const char *filename, const struct utimbuf *times)
	{
        return CALL(utime)( CWrap( filename, false), times );
	}

	WRAP(utimes, int, const char *filename, const struct timeval times[2])
	{
        return CALL(utimes)( CWrap( filename, false), times );
	}

	WRAP(realpath, char *, const char *path, char *resolved_path)
	{
		return CALL(realpath)( CWrap( path, true ), resolved_path );
	}

	WRAP(mkdir, int, const char *pathname, mode_t mode)
	{
		return CALL(mkdir)( CWrap( pathname, true ), mode );
	}

	WRAP(rmdir, char *, const char *pathname)
	{
		return CALL(rmdir)( CWrap( pathname, false ) );
	}

};

#endif
