//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef P4HELPERS_H
#define P4HELPERS_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlstring.h"
#include "tier1/smartptr.h"


//
// Class representing file operations
//
class CP4File
{
public:
	explicit CP4File( char const *szFilename );
	virtual ~CP4File();

public:
	// Opens the file for edit
	virtual bool Edit( void );
	
	// Opens the file for add
	virtual bool Add( void );

	// Reverts the file
	virtual bool Revert( void );

	// Is the file in perforce?
	virtual bool IsFileInPerforce();

	// Changes the file to the specified filetype.
	virtual bool SetFileType( const CUtlString& desiredFileType );

protected:
	// The filename that this class instance represents
	CUtlString m_sFilename;
};

//
// An override of CP4File performing no Perforce interaction
//
class CP4File_Dummy : public CP4File
{
public:
	explicit CP4File_Dummy( char const *szFilename ) : CP4File( szFilename ) {}

public:
	virtual bool Edit( void ) { return true; }
	virtual bool Add( void ) { return true; }
	virtual bool IsFileInPerforce() { return false; }
	virtual bool SetFileType(const CUtlString& desiredFileType) { return true; }
};


//
// Class representing a factory for creating other helper objects
//
class CP4Factory
{
public:
	CP4Factory();
	~CP4Factory();

public:
	// Sets whether dummy objects are created by the factory.
	// Returns the old state of the dummy mode.
	bool SetDummyMode( bool bDummyMode );

public:
	// Sets the name of the changelist to open files under,
	// NULL for "Default" changelist.
	void SetOpenFileChangeList( const char *szChangeListName );

public:
	// Creates a file access object for the given filename.
	CP4File *AccessFile( char const *szFilename ) const;

protected:
	// Whether the factory is in the "dummy mode" and is creating dummy objects
	bool m_bDummyMode;
};

// Default p4 factory
extern CP4Factory *g_p4factory;


//
// CP4AutoEditFile - edits the file upon construction
//
class CP4AutoEditFile
{
public:
	explicit CP4AutoEditFile( char const *szFilename ) : m_spImpl( g_p4factory->AccessFile( szFilename ) ) { m_spImpl->Edit(); }

	CP4File * File() const { return m_spImpl.Get(); }

protected:
	CPlainAutoPtr< CP4File > m_spImpl;
};

//
// CP4AutoAddFile - adds the file upon construction
//
class CP4AutoAddFile
{
public:
	explicit CP4AutoAddFile( char const *szFilename ) : m_spImpl( g_p4factory->AccessFile( szFilename ) ) { m_spImpl->Add(); }

	CP4File * File() const { return m_spImpl.Get(); }

protected:
	CPlainAutoPtr< CP4File > m_spImpl;
};

//
// CP4AutoEditAddFile - edits the file upon construction / adds upon destruction
//
class CP4AutoEditAddFile
{
public:
	explicit CP4AutoEditAddFile( char const *szFilename ) 
	: m_spImpl( g_p4factory->AccessFile( szFilename ) )
	, m_bHasDesiredFileType( false )
	{ 
		m_spImpl->Edit(); 
	}

	explicit CP4AutoEditAddFile( char const *szFilename, const char *szFiletype ) 
	: m_spImpl( g_p4factory->AccessFile( szFilename ) )
	, m_sFileType(szFiletype)
	, m_bHasDesiredFileType( true )
	{ 
		m_spImpl->Edit(); 
		m_spImpl->SetFileType( m_sFileType );
	}

	~CP4AutoEditAddFile( void ) 
	{ 
		m_spImpl->Add(); 
		if ( m_bHasDesiredFileType )
			m_spImpl->SetFileType( m_sFileType );
	}

	CP4File * File() const { return m_spImpl.Get(); }

protected:
	CPlainAutoPtr< CP4File > m_spImpl;
	CUtlString m_sFileType;
	bool m_bHasDesiredFileType;
};


//
// CP4AutoRevert - reverts the file upon construction
//
class CP4AutoRevertFile
{
public:
	explicit CP4AutoRevertFile( char const *szFilename ) : m_spImpl( g_p4factory->AccessFile( szFilename ) ) { m_spImpl->Revert(); }

	CP4File * File() const { return m_spImpl.Get(); }

protected:
	CPlainAutoPtr< CP4File > m_spImpl;
};


#endif // #ifndef P4HELPERS_H
