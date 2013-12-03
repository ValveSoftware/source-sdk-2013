//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IP4_H
#define IP4_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlsymbol.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "appframework/IAppSystem.h"


//-----------------------------------------------------------------------------
// Current perforce file state
//-----------------------------------------------------------------------------
enum P4FileState_t
{
	P4FILE_UNOPENED = 0,
	P4FILE_OPENED_FOR_ADD,
	P4FILE_OPENED_FOR_EDIT,
	P4FILE_OPENED_FOR_DELETE,
	P4FILE_OPENED_FOR_INTEGRATE,
};


//-----------------------------------------------------------------------------
// Purpose: definition of a file
//-----------------------------------------------------------------------------
struct P4File_t
{
	CUtlSymbol m_sName;			// file name
	CUtlSymbol m_sPath;			// residing folder
	CUtlSymbol m_sDepotFile;	// the name in the depot
	CUtlSymbol m_sClientFile;	// the name on the client in Perforce syntax
	CUtlSymbol m_sLocalFile;	// the name on the client in local syntax
	int m_iHeadRevision;		// head revision number
	int m_iHaveRevision;		// the revision the clientspec has synced locally
	bool m_bDir;				// directory
	bool m_bDeleted;			// deleted
	P4FileState_t m_eOpenState;	// current change state
	int m_iChangelist;			// changelist current opened in
};


//-----------------------------------------------------------------------------
// Purpose: a single revision of a file
//-----------------------------------------------------------------------------
struct P4Revision_t
{
	int m_iChange;		// changelist number
	int m_nYear, m_nMonth, m_nDay;
	int m_nHour, m_nMinute, m_nSecond;

	CUtlSymbol m_sUser;		// submitting user
	CUtlSymbol m_sClient;	// submitting client 
	CUtlString m_Description;
};


//-----------------------------------------------------------------------------
// Purpose: a single clientspec
//-----------------------------------------------------------------------------
struct P4Client_t
{
	CUtlSymbol m_sName;
	CUtlSymbol m_sUser;
	CUtlSymbol m_sHost;			// machine name this client is on
	CUtlSymbol m_sLocalRoot;	// local path
};


//-----------------------------------------------------------------------------
// Purpose: Interface to accessing P4 commands
//-----------------------------------------------------------------------------
#define P4_INTERFACE_VERSION		"VP4001"
// Vitaliy - 09-Feb-'07: if anybody ups the version of this interface, please
// move the method "SetOpenFileChangeList" into the appropriate section.

abstract_class IP4  : public IAppSystem
{
public:
	// name of the current clientspec
	virtual P4Client_t &GetActiveClient() = 0;

	// changes the current client
	virtual void SetActiveClient(const char *clientname) = 0;

	// Refreshes the current client from p4 settings
	virtual void RefreshActiveClient() = 0;

	// translate filespecs into the desired syntax
	virtual void GetDepotFilePath(char *depotFilePath, const char *filespec, int size) = 0;
	virtual void GetClientFilePath(char *clientFilePath, const char *filespec, int size) = 0;
	virtual void GetLocalFilePath(char *localFilePath, const char *filespec, int size) = 0;

	// retreives the list of files in a path
	virtual CUtlVector<P4File_t> &GetFileList( const char *path ) = 0;

	// returns the list of files opened for edit/integrate/delete 
	virtual void GetOpenedFileList( CUtlVector<P4File_t> &fileList ) = 0;
	virtual void GetOpenedFileList( const char *pRootDirectory, CUtlVector<P4File_t> &fileList ) = 0;
	virtual void GetOpenedFileListInPath( const char *pPathID, CUtlVector<P4File_t> &fileList ) = 0;

	// retrieves revision history for a file or directory
	virtual CUtlVector<P4Revision_t> &GetRevisionList( const char *path, bool bIsDir ) = 0;

	// returns a list of clientspecs
	virtual CUtlVector<P4Client_t> &GetClientList() = 0;

	// changes the clientspec to remove the specified path (cloaking)
	virtual void RemovePathFromActiveClientspec( const char *path ) = 0;

	// file manipulation
	virtual bool OpenFileForAdd( const char *pFullPath ) = 0;
	virtual bool OpenFileForEdit( const char *pFullPath ) = 0;
	virtual bool OpenFileForDelete( const char *pFullPath ) = 0;

	// submit/revert
	virtual bool SubmitFile( const char *pFullPath, const char *pDescription ) = 0;
	virtual bool RevertFile( const char *pFullPath ) = 0;

	// file checkin/checkout for multiple files
	virtual bool OpenFilesForAdd( int nCount, const char **ppFullPathList ) = 0;
	virtual bool OpenFilesForEdit( int nCount, const char **ppFullPathList ) = 0;
	virtual bool OpenFilesForDelete( int nCount, const char **ppFullPathList ) = 0;

	// submit/revert for multiple files
	virtual bool SubmitFiles( int nCount, const char **ppFullPathList, const char *pDescription ) = 0;
	virtual bool RevertFiles( int nCount, const char **ppFullPathList ) = 0;

	// Is this file in perforce?
	virtual bool IsFileInPerforce( const char *pFullPath ) = 0;

	// Get the perforce file state
	virtual P4FileState_t GetFileState( const char *pFullPath ) = 0;

	// depot root
	virtual const char *GetDepotRoot() = 0;
	virtual int GetDepotRootLength() = 0;

	// local root
	virtual const char *GetLocalRoot() = 0;
	virtual int GetLocalRootLength() = 0;

	// Gets a string for a symbol
	virtual const char *String( CUtlSymbol s ) const = 0;

	// Returns which clientspec a file lies under. This will
	// search for p4config files in root directories of the file
	// It returns false if it didn't find a p4config file.
	virtual bool GetClientSpecForFile( const char *pFullPath, char *pClientSpec, int nMaxLen ) = 0;
	virtual bool GetClientSpecForDirectory( const char *pFullPathDir, char *pClientSpec, int nMaxLen ) = 0;

	// Returns which clientspec a filesystem path ID lies under. This will
	// search for p4config files in all root directories of the all paths in
	// the search path. NOTE: All directories in a path need to use the same clientspec
	// or this function will return false.
	// It returns false if it didn't find a p4config file.
	virtual bool GetClientSpecForPath( const char *pPathId, char *pClientSpec, int nMaxLen ) = 0;

	// Opens a file in p4 win
	virtual void OpenFileInP4Win( const char *pFullPath ) = 0;

	// have we connected? if not, nothing works
	virtual bool IsConnectedToServer( bool bRetry = true ) = 0;

	// Returns file information for a single file
	virtual bool GetFileInfo( const char *pFullPath, P4File_t *pFileInfo ) = 0;

	// retreives the list of files in a path, using a known client spec
	virtual CUtlVector<P4File_t> &GetFileListUsingClientSpec( const char *pPath, const char *pClientSpec ) = 0;

	// retrieves the last error from the last op (which is likely to span multiple lines)
	// this is only valid after OpenFile[s]For{Add,Edit,Delete} or {Submit,Revert}File[s]
	virtual const char *GetLastError() = 0;

	// sets the name of the changelist to open files under, NULL for "Default" changelist
	virtual void SetOpenFileChangeList( const char *pChangeListName ) = 0;
};



#endif // IP4_H
