//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: Tools for grabbing/dumping the stack at runtime
//
// $NoKeywords: $
//=============================================================================//

#ifndef TIER0_STACKTOOLS_H
#define TIER0_STACKTOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

#if (defined( PLATFORM_WINDOWS ) || defined( PLATFORM_X360 )) && !defined( STEAM ) && !defined( _CERT ) && defined( TCHAR_IS_CHAR ) //designed for windows/x360, not built/tested with wide characters, not intended for release builds (but probably wouldn't damage anything)
#	define ENABLE_RUNTIME_STACK_TRANSLATION //uncomment to enable runtime stack translation tools. All of which use on-demand loading of necessary dll's and pdb's
#endif

#if defined( ENABLE_RUNTIME_STACK_TRANSLATION )
//#define ENABLE_THREAD_PARENT_STACK_TRACING 1 //uncomment to actually enable tracking stack traces from threads and jobs to their parent thread. Must also define THREAD_PARENT_STACK_TRACE_SUPPORTED in threadtools.h
#	if defined( ENABLE_THREAD_PARENT_STACK_TRACING )
#		define THREAD_PARENT_STACK_TRACE_LENGTH 32
#	endif
#endif




PLATFORM_INTERFACE int GetCallStack( void **pReturnAddressesOut, int iArrayCount, int iSkipCount );

//ONLY WORKS IF THE CRAWLED PORTION OF THE STACK DISABLES FRAME POINTER OMISSION (/Oy-)  "vpc /nofpo"
PLATFORM_INTERFACE int GetCallStack_Fast( void **pReturnAddressesOut, int iArrayCount, int iSkipCount );

typedef int (*FN_GetCallStack)( void **pReturnAddressesOut, int iArrayCount, int iSkipCount );

//where we'll find our PDB's for win32.
PLATFORM_INTERFACE void SetStackTranslationSymbolSearchPath( const char *szSemicolonSeparatedList = NULL );
PLATFORM_INTERFACE void StackToolsNotify_LoadedLibrary( const char *szLibName );

//maximum output sample "tier0.dll!TranslateStackInfo - u:\Dev\L4D\src\tier0\stacktools.cpp(162) + 4 bytes"
enum TranslateStackInfo_StyleFlags_t
{
	TSISTYLEFLAG_NONE = 0,
	TSISTYLEFLAG_MODULENAME = (1<<0), //start with module			Sample: "tier0.dll!"
	TSISTYLEFLAG_SYMBOLNAME = (1<<1), //include the symbol name		Sample: "TranslateStackInfo"
	TSISTYLEFLAG_FULLPATH = (1<<2), //include full path				Sample: "u:\Dev\L4D\src\tier0\stacktools.cpp"
	TSISTYLEFLAG_SHORTPATH = (1<<3), //only include 2 directories	Sample: "\src\tier0\stacktools.cpp"
	TSISTYLEFLAG_LINE = (1<<4), //file line number					Sample: "(162)"
	TSISTYLEFLAG_LINEANDOFFSET = (1<<5), //file line + offset		Sample: "(162) + 4 bytes"
	TSISTYLEFLAG_LAST = TSISTYLEFLAG_LINEANDOFFSET,
	TSISTYLEFLAG_DEFAULT = (TSISTYLEFLAG_MODULENAME | TSISTYLEFLAG_SYMBOLNAME | TSISTYLEFLAG_FULLPATH | TSISTYLEFLAG_LINEANDOFFSET), //produces sample above
};

//Generates a formatted list of function information, returns number of translated entries
//On 360 this generates a string that can be decoded by VXConsole in print functions. Optimal path for translation because it's one way. Other paths require multiple transactions.
PLATFORM_INTERFACE int TranslateStackInfo( const void * const *pCallStack, int iCallStackCount, tchar *szOutput, int iOutBufferSize, const tchar *szEntrySeparator, TranslateStackInfo_StyleFlags_t style = TSISTYLEFLAG_DEFAULT );

PLATFORM_INTERFACE void PreloadStackInformation( void * const *pAddresses, int iAddressCount ); //caches data and reduces communication with VXConsole to speed up 360 decoding when using any of the Get***FromAddress() functions. Nop on PC.
PLATFORM_INTERFACE bool GetFileAndLineFromAddress( const void *pAddress, tchar *pFileNameOut, int iMaxFileNameLength, uint32 &iLineNumberOut, uint32 *pDisplacementOut = NULL );
PLATFORM_INTERFACE bool GetSymbolNameFromAddress( const void *pAddress, tchar *pSymbolNameOut, int iMaxSymbolNameLength, uint64 *pDisplacementOut = NULL );
PLATFORM_INTERFACE bool GetModuleNameFromAddress( const void *pAddress, tchar *pModuleNameOut, int iMaxModuleNameLength );



class PLATFORM_CLASS CCallStackStorage //a helper class to grab a stack trace as close to the leaf code surface as possible, then pass it on to deeper functions intact with less unpredictable inlining pollution
{
public:
	CCallStackStorage( FN_GetCallStack GetStackFunction = GetCallStack, uint32 iSkipCalls = 0 );
	CCallStackStorage( const CCallStackStorage &copyFrom )
	{
		iValidEntries = copyFrom.iValidEntries;
		memcpy( pStack, copyFrom.pStack, sizeof( void * ) * copyFrom.iValidEntries );
	}

	void *pStack[128]; //probably too big, possibly too small for some applications. Don't want to spend the time figuring out how to generalize this without templatizing pollution or mallocs
	uint32 iValidEntries;
};


//Hold onto one of these to denote the top of a functional stack trace. Also allows us to string together threads to their parents
class PLATFORM_CLASS CStackTop_Base
{
protected:
#if defined( ENABLE_RUNTIME_STACK_TRANSLATION )
	CStackTop_Base *m_pPrevTop;
	void *m_pStackBase;
	void *m_pReplaceAddress;

	void * const *m_pParentStackTrace;
	int m_iParentStackTraceLength;
#endif
};

//makes a copy of the parent stack
class PLATFORM_CLASS CStackTop_CopyParentStack : public CStackTop_Base
{
public:
	CStackTop_CopyParentStack( void * const * pParentStackTrace, int iParentStackTraceLength );
	~CStackTop_CopyParentStack( void );
};

//just references the parent stack. Assuming that you'll keep that memory around as long as you're keeping this Stack Top marker.
class PLATFORM_CLASS CStackTop_ReferenceParentStack : public CStackTop_Base
{
public:
	CStackTop_ReferenceParentStack( void * const * pParentStackTrace = NULL, int iParentStackTraceLength = 0 );
	~CStackTop_ReferenceParentStack( void );
	void ReleaseParentStackReferences( void ); //in case you need to delete the parent stack trace before this class goes out of scope
};


//Encodes data so that every byte's most significant bit is a 1. Ensuring no null terminators.
//This puts the encoded data in the 128-255 value range. Leaving all standard ascii characters for control.
//Returns string length (not including the written null terminator as is standard). 
//Or if the buffer is too small. Returns negative of necessary buffer size (including room needed for null terminator)
PLATFORM_INTERFACE int EncodeBinaryToString( const void *pToEncode, int iDataLength, char *pEncodeOut, int iEncodeBufferSize );

//Decodes a string produced by EncodeBinaryToString(). Safe to decode in place if you don't mind trashing your string, binary byte count always less than string byte count.
//Returns:
//	>= 0 is the decoded data size
//	INT_MIN (most negative value possible) indicates an improperly formatted string (not our data)
//	all other negative values are the negative of how much dest buffer size is necessary.
PLATFORM_INTERFACE int DecodeBinaryFromString( const char *pString, void *pDestBuffer, int iDestBufferSize, char **ppParseFinishOut = NULL );




// 360<->VXConsole specific communication definitions
#define XBX_CALLSTACKDECODEPREFIX ":CSDECODE["

enum StackTranslation_BinaryHandler_Command_t
{
	ST_BHC_LOADEDLIBARY,
	ST_BHC_GETTRANSLATIONINFO,
};

#pragma pack(push)
#pragma pack(1)
struct FullStackInfo_t
{
	const void *pAddress;
	char szModuleName[24];
	char szFileName[MAX_PATH/2];
	char szSymbol[64];
	uint32 iLine;
	uint32 iSymbolOffset;

};
#pragma pack(pop)

#endif //#ifndef TIER0_STACKTOOLS_H
