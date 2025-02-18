//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Describes the way to compile a MDL file (eventual replacement for qc)
//
//===========================================================================//

#ifndef DMEMAKEFILE_H
#define DMEMAKEFILE_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "datamodel/dmehandle.h"
#include "vstdlib/iprocessutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeMakefile;


//-----------------------------------------------------------------------------
// Returns a list of source types and whether there can be only 1 or not
//-----------------------------------------------------------------------------
struct DmeMakefileType_t
{
	const char *m_pTypeName;
	const char *m_pHumanReadableName;
	bool m_bIsSingleton;
	const char *m_pDefaultDirectoryID;	// NOTE: Use CDmeMakefile::GetDefaultDirectory, passing this in to crack it.
	const char *m_pFileFilter;
	const char *m_pFileFilterString;
};


//-----------------------------------------------------------------------------
// Describes an asset source; contains a source name + options
//-----------------------------------------------------------------------------
class CDmeSource : public CDmElement
{
	DEFINE_ELEMENT( CDmeSource, CDmElement );

public:
	// NOTE: Filenames are stored as relative file names in dmesource
	// To resolve them to full paths, use CDmeMakefile::GetSourceFullPath
	const char *GetRelativeFileName() const;
	void SetRelativeFileName( const char *pFileName );

	// If this source can be built by another makefile, return the type of makefile
	// NOTE: This can be a base class of a number of makefile types
	virtual const char **GetSourceMakefileTypes() { return NULL; }

	// Sets/gets the makefile that was used to build this source
	// NOTE: This information isn't saved and must be reconstructed each frame
	void SetDependentMakefile( CDmeMakefile *pMakeFile );
	CDmeMakefile *GetDependentMakefile();

	// Call this to open the source file in an editor
	void OpenEditor();

private:
	// The makefile that built this source
	CDmeHandle< CDmeMakefile > m_DependentMakefile;	
};


inline const char *CDmeSource::GetRelativeFileName() const
{
	return m_Name;
}

inline void CDmeSource::SetRelativeFileName( const char *pName )
{
	m_Name = pName;
}


//-----------------------------------------------------------------------------
// Describes an asset: something that is compiled from sources 
//-----------------------------------------------------------------------------
class CDmeMakefile : public CDmElement
{
	DEFINE_ELEMENT( CDmeMakefile, CDmElement );

public:
	// NOTE: Adding or removing sources of the specified type will invalidate the index
	// NOTE: This index is the same index used in GetSources()
	CDmeSource *AddSource( const char *pSourceType, const char *pFullPath );
	template< class T >
	T* AddSource( const char *pFullPath );
	CDmeSource *SetSingleSource( const char *pSourceType, const char *pFullPath );
	template< class T >
	T* SetSingleSource( const char *pFullPath );
	CDmeSource *FindSource( const char *pSourceType, const char *pFullPath );
	void RemoveSource( const char *pSourceType, const char *pFullPath );
	void RemoveSource( CDmeSource *pSource );
	void RemoveAllSources( const char *pSourceType );
	bool HasSourceOfType( const char *pSourceType );

	// Gets/sets paths associated with sources
	void SetSourceFullPath( CDmeSource *pSource, const char *pFullPath );
	void GetSourceFullPath( CDmeSource *pSource, char *pFullPath, int nBufLen );

	// Returns the output directory we expect to compile files into
	bool GetOutputDirectory( char *pFullPath, int nBufLen );

	// Returns the output name (output directory + filename, no extension)
	bool GetOutputName( char *pFullPath, int nBufLen );

	// Call this to change the file the makefile is stored in
	// Will make all sources be relative to this path
	bool SetFileName( const char *pFileName );
	const char *GetFileName() const;

	// Gets a list of all sources of a particular type
	void GetSources( const char *pSourceType, CUtlVector< CDmeHandle< CDmeSource > > &sources );
	template< class T >
	void GetSources( CUtlVector< CDmeHandle<T> > &sources );

	// Gets a list of all sources, regardless of type
	int GetSourceCount();
	CDmeSource *GetSource( int nIndex );

	virtual DmeMakefileType_t *GetMakefileType() { return NULL; }
	virtual DmeMakefileType_t* GetSourceTypes() { Assert(0); return NULL; }

	// FIXME: Should we have output types? Not sure...
	virtual void GetOutputs( CUtlVector<CUtlString> &fullPaths ) { Assert(0); }

	// Converts the m_pDefaultDirectoryID field of the DmeMakefileType_t to a full path
	bool GetDefaultDirectory( const char *pDefaultDirectoryID, char *pFullPath, int nBufLen );

	// These methods are used to help traverse a dependency graph.
	// They work with information that is not saved.
	void SetAssociation( CDmeSource *pSource, CDmeMakefile *pSourceMakefile );
	CDmeMakefile *FindDependentMakefile( CDmeSource *pSource );
	CDmeSource *FindAssociatedSource( CDmeMakefile *pChildMakefile );
	CDmElement *GetOutputElement( bool bCreateIfNecessary = false );

	// Performs compilation steps
	void PreCompile( );
	void PostCompile( );

	// Dirty, dirty!
	bool IsDirty() const;
	void SetDirty( bool bDirty );

protected:
	// Make all outputs writeable
	void MakeOutputsWriteable( );

	// Gets the path of the makefile
	void GetMakefilePath( char *pFullPath, int nBufLen );

private:
	// Inherited classes should re-implement these methods
	virtual CDmElement *CreateOutputElement( ) { return NULL; }
	virtual void DestroyOutputElement( CDmElement *pOutput ) { }
	virtual ProcessHandle_t PerformCompilation() { Assert(0); return PROCESS_HANDLE_INVALID; }
	virtual const char *GetOutputDirectoryID() { return "makefilegamedir:"; }

private:
	// Relative path to full path
	void RelativePathToFullPath( const char *pRelativePath, char *pFullPath, int nBufLen );

	// Fullpath to relative path
	void FullPathToRelativePath( const char *pFullPath, char *pRelativePath, int nBufLen );

	// Updates the source names to be relative to a particular path
	bool UpdateSourceNames( const char *pOldRootDir, const char *pNewRootDir, bool bApplyChanges );

	CDmaElementArray< CDmeSource > m_Sources;
	CDmeHandle< CDmElement > m_hOutput;
	ProcessHandle_t m_hCompileProcess;
	bool m_bIsDirty;
};


//-----------------------------------------------------------------------------
// Dirty, dirty!
//-----------------------------------------------------------------------------
inline bool CDmeMakefile::IsDirty() const
{
	return m_bIsDirty;
}

inline void CDmeMakefile::SetDirty( bool bDirty )
{
	m_bIsDirty = bDirty;
}


//-----------------------------------------------------------------------------
// Gets a list of all sources of a particular type
//-----------------------------------------------------------------------------
template< class T >
void CDmeMakefile::GetSources( CUtlVector< CDmeHandle<T> > &sources )
{
	int nCount = m_Sources.Count();
	sources.EnsureCapacity( nCount );
	for ( int i = 0; i < nCount; ++i )
	{
		if ( m_Sources[i]->IsA( T::GetStaticTypeSymbol() ) )
		{
			int j = sources.AddToTail();
			sources[j] = CastElement<T>( m_Sources[i] );
		}
	}
}


//-----------------------------------------------------------------------------
// Template version to add a source
//-----------------------------------------------------------------------------
template< class T >
T* CDmeMakefile::AddSource( const char *pFullPath )
{
	return CastElement< T >( AddSource( g_pDataModel->GetString( T::GetStaticTypeSymbol() ), pFullPath ) );
}


//-----------------------------------------------------------------------------
// Template version to set a single source
//-----------------------------------------------------------------------------
template< class T >
T* CDmeMakefile::SetSingleSource( const char *pFullPath )
{
	return CastElement< T >( SetSingleSource( g_pDataModel->GetString( T::GetStaticTypeSymbol() ), pFullPath ) );
}


#endif // DMEMAKEFILE_H
