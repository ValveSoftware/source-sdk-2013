//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEXTSTATSMGR_H
#define TEXTSTATSMGR_H
#ifdef _WIN32
#pragma once
#endif


#include "filesystem.h"
#include "utllinkedlist.h"


// Text stats get to print their stuff by implementing this type of function.
typedef void (*TextStatPrintFn)( IFileSystem *pFileSys, FileHandle_t hFile, void *pUserData );
typedef void (*TextStatFileFn)();


// The CTextStatsMgr is just a collection of CTextStat's that go into the same file.
class CTextStatsMgr
{
public:
	CTextStatsMgr( void );

	// Write a file with all the registered stats.
	bool				WriteFile( IFileSystem *pFileSystem, const char *pFilename = NULL );

	// Get the preset filename to write stats to, if none is specified when writing
	char				*GetStatsFilename( void );

	// Set the filename to write stats to, if none is specified when writing
	void				SetStatsFilename( char *sFilename );

private:
	char	m_szStatFilename[ MAX_PATH ];
};


// This is the default CTextStatsMgr, but there can be any number of them.
extern CTextStatsMgr g_TextStatsMgr;



// Make these to register text stat functions.
class CTextStat
{
friend class CTextStatsMgr;

public:
				CTextStat();
				CTextStat( TextStatPrintFn fn, void *pUserData, CTextStatsMgr *pMgr=&g_TextStatsMgr );
				~CTextStat();

	// This can be called if you don't want to pass parameters into the constructor.
	void		Init( TextStatPrintFn printFn, void *pUserData, CTextStatsMgr *pMgr=&g_TextStatsMgr );
	void		Term();


private:

				// Special constructor to just tie off the linked list.
						CTextStat( bool bGlobalListHead );

	// The global list of CTextStats.
	static CTextStat*	GetTextStatsList();
	
	static void	RemoveFn( void *pUserData );
	
	// Link it into the global list.
	CTextStat	*m_pPrev;
	CTextStat	*m_pNext;
	
	CTextStatsMgr	*m_pMgr;
	
	TextStatPrintFn	m_PrintFn;
	void			*m_pUserData;
};


// This class registers like a ConVar and acts like an int. When the game is shutdown,
// its value will be saved in the stats file along with its name.s
class CTextStatInt
{
public:
				CTextStatInt( const char *pName, int initialValue=0, CTextStatsMgr *pMgr=&g_TextStatsMgr );

				operator int() const	{ return m_Value; }
	int			operator=( int val )	{ m_Value = val; return m_Value; }

	int			operator++()			{ m_Value++; return m_Value; }
	int			operator--()			{ m_Value--; return m_Value; }
	int			operator+=( int val )	{ m_Value += val; return m_Value; }
	int			operator-=( int val )	{ m_Value -= val; return m_Value; }
	int			operator*=( int val )	{ m_Value *= val; return m_Value; }
	int			operator/=( int val )	{ m_Value /= val; return m_Value; }


private:

	static void	PrintFn( IFileSystem *pFileSys, FileHandle_t hFile, void *pUserData );
	

private:
	
	const char		*m_pName;
	int				m_Value;

	CTextStat	m_Reg;	// Use to register ourselves.
};


// This can be registered to get a callback when the text stats mgr is saving its files.
// You can write data out to your own file in here.
class CTextStatFile
{
public:
	CTextStatFile( TextStatFileFn fn );

private:
	friend class CTextStatsMgr;

	static CTextStatFile *s_pHead;
	CTextStatFile *m_pNext;
	TextStatFileFn m_pFn;
};


#endif // TEXTSTATSMGR_H
