//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A collection of utility classes to simplify file I/O, and
//			as much as possible contain portability problems. Here avoiding 
//			including windows.h.
//
//=============================================================================

#if defined(_WIN32)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502		// ReadDirectoryChangesW
#endif

#if defined(OSX)
#include <CoreServices/CoreServices.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#endif

#define ASYNC_FILEIO
#if defined( LINUX )
// Linux hasn't got a good AIO library that we have found yet, so lets punt for now
#undef ASYNC_FILEIO
#endif

#if defined(_WIN32)
//#include <direct.h>
#include <io.h>
// unset to force to use stdio implementation 
#define WIN32_FILEIO

#if defined(ASYNC_FILEIO) 
#if defined(_WIN32) && !defined(WIN32_FILEIO)
#error "trying to use async io without win32 filesystem API usage, that isn't doable"
#endif
#endif

#else /* not defined (_WIN32) */
#include <utime.h>
#include <dirent.h>
#include <unistd.h> // for unlink
#include <limits.h> // defines PATH_MAX
#include <alloca.h> // 'cause we like smashing the stack
#if defined( _PS3 )
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#include <sys/statvfs.h>
#endif
#include <sched.h>
#define int64 int64_t

#define _A_SUBDIR S_IFDIR

// FUTURE map _A_HIDDEN via checking filename against .*
#define _A_HIDDEN 0

// FUTURE check 'read only' by checking mode against S_IRUSR
#define _A_RDONLY 0

// no files under posix are 'system' or 'archive'
#define _A_SYSTEM 0
#define _A_ARCH   0

#endif 

#include "tier1/fileio.h"
#include "tier1/utlbuffer.h"
#include "tier1/strtools.h"
#include <errno.h>

#if defined( WIN32_FILEIO )
#include "winlite.h"
#endif

#if defined( ASYNC_FILEIO )
#ifdef _WIN32
#include "winlite.h"
#elif defined(_PS3)
// bugbug ps3 - see some aio files under libfs.. skipping for the moment
#elif defined(POSIX)
#include <aio.h>
#else
#error "aio please"
#endif
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor from UTF8
//-----------------------------------------------------------------------------
CPathString::CPathString( const char *pchUTF8Path )
{
	// Need to first turn into an absolute path, so \\?\ pre-pended paths will be ok
	m_pchUTF8Path = new char[ MAX_UNICODE_PATH_IN_UTF8 ];
	m_pwchWideCharPathPrepended = NULL;

	// First, convert to absolute path, which also does Q_FixSlashes for us.
	Q_MakeAbsolutePath( m_pchUTF8Path, MAX_UNICODE_PATH * 4, pchUTF8Path );

	// Second, fix any double slashes
	V_FixDoubleSlashes( m_pchUTF8Path );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPathString::~CPathString()
{
	if ( m_pwchWideCharPathPrepended )
	{
		delete[] m_pwchWideCharPathPrepended;
		m_pwchWideCharPathPrepended = NULL;
	}

	if ( m_pchUTF8Path )
	{
		delete[] m_pchUTF8Path;
		m_pchUTF8Path = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Access UTF8 path
//-----------------------------------------------------------------------------
const char * CPathString::GetUTF8Path()
{
	return m_pchUTF8Path;
}


//-----------------------------------------------------------------------------
// Purpose: Gets wchar_t based path, with \\?\ pre-pended (allowing long paths 
// on Win32, should only be used with unicode extended path aware filesystem calls)
//-----------------------------------------------------------------------------
const wchar_t *CPathString::GetWCharPathPrePended()
{
	PopulateWCharPath();
	return m_pwchWideCharPathPrepended; 
}


//-----------------------------------------------------------------------------
// Purpose: Builds wchar path string
//-----------------------------------------------------------------------------
void CPathString::PopulateWCharPath()
{
	if ( m_pwchWideCharPathPrepended )
		return;

	// Check if the UTF8 path starts with \\, which on Win32 means it's a UNC path, and then needs a different prefix
	if ( m_pchUTF8Path[0] == '\\' && m_pchUTF8Path[1] == '\\' )
	{
		m_pwchWideCharPathPrepended = new wchar_t[MAX_UNICODE_PATH+8];
		Q_memcpy( m_pwchWideCharPathPrepended, L"\\\\?\\UNC\\", 8*sizeof(wchar_t) );
#ifdef DBGFLAG_ASSERT
		int cchResult =
#endif
			Q_UTF8ToUnicode( m_pchUTF8Path+2, m_pwchWideCharPathPrepended+8, MAX_UNICODE_PATH*sizeof(wchar_t) );
		Assert( cchResult );

		// Be sure we NULL terminate within our allocated region incase Q_UTF8ToUnicode failed, though we're already in bad shape then.
		m_pwchWideCharPathPrepended[MAX_UNICODE_PATH+7] = 0;
	}
	else
	{
		m_pwchWideCharPathPrepended = new wchar_t[MAX_UNICODE_PATH+4];
		Q_memcpy( m_pwchWideCharPathPrepended, L"\\\\?\\", 4*sizeof(wchar_t) );
#ifdef DBGFLAG_ASSERT
		int cchResult =
#endif
			Q_UTF8ToUnicode( m_pchUTF8Path, m_pwchWideCharPathPrepended+4, MAX_UNICODE_PATH*sizeof(wchar_t) );
		Assert( cchResult );

		// Be sure we NULL terminate within our allocated region incase Q_UTF8ToUnicode failed, though we're already in bad shape then.
		m_pwchWideCharPathPrepended[MAX_UNICODE_PATH+3] = 0;
	}
}

#ifdef WIN32
struct DirWatcherOverlapped : public OVERLAPPED
{
	CDirWatcher *m_pDirWatcher;
};
#endif

#if !defined(_PS3) && !defined(_X360)
// a buffer full of file names
static const int k_cubDirWatchBufferSize = 8 * 1024;

//-----------------------------------------------------------------------------
// Purpose: directory watching
//-----------------------------------------------------------------------------
CDirWatcher::CDirWatcher()
{
	m_hFile = NULL;
	m_pOverlapped = NULL;
	m_pFileInfo = NULL;
#ifdef OSX
	m_WatcherStream = 0;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: directory watching
//-----------------------------------------------------------------------------
CDirWatcher::~CDirWatcher()
{
#ifdef WIN32
	if ( m_pOverlapped )
	{
		// mark the overlapped structure as gone
		DirWatcherOverlapped *pDirWatcherOverlapped = (DirWatcherOverlapped *)m_pOverlapped;
		pDirWatcherOverlapped->m_pDirWatcher = NULL;
	}

	if ( m_hFile )
	{
		// make sure we flush any pending I/O's on the handle
		::CancelIo( m_hFile );
		::SleepEx( 0, TRUE );
		// close the handle
		::CloseHandle( m_hFile );
	}
#elif defined(OSX)
	if ( m_WatcherStream )
	{
		FSEventStreamStop( (FSEventStreamRef)m_WatcherStream );
		FSEventStreamInvalidate( (FSEventStreamRef)m_WatcherStream );
		FSEventStreamRelease( (FSEventStreamRef)m_WatcherStream );		
		m_WatcherStream = 0;
	}
#endif
	if ( m_pFileInfo )
	{
		free( m_pFileInfo );
	}
	if ( m_pOverlapped )
	{
		free( m_pOverlapped );
	}
}


#ifdef WIN32
//-----------------------------------------------------------------------------
// Purpose: callback watch
//			gets called on the same thread whenever a SleepEx() occurs
//-----------------------------------------------------------------------------
class CDirWatcherFriend
{
public:
	static void WINAPI DirWatchCallback( DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, OVERLAPPED *pOverlapped )
	{
		DirWatcherOverlapped *pDirWatcherOverlapped = (DirWatcherOverlapped *)pOverlapped;

		// see if we've been cancelled
		if ( !pDirWatcherOverlapped->m_pDirWatcher )
			return;

		// parse and pass back
		if ( dwNumberOfBytesTransfered > sizeof(FILE_NOTIFY_INFORMATION) )
		{
			FILE_NOTIFY_INFORMATION *pFileNotifyInformation = (FILE_NOTIFY_INFORMATION *)pDirWatcherOverlapped->m_pDirWatcher->m_pFileInfo;
			do 
			{
				// null terminate the string and turn it to UTF-8
				int cNumWChars = pFileNotifyInformation->FileNameLength / sizeof(wchar_t);
				wchar_t *pwchT = new wchar_t[cNumWChars + 1];
				memcpy( pwchT, pFileNotifyInformation->FileName, pFileNotifyInformation->FileNameLength );
				pwchT[cNumWChars] = 0;
				CStrAutoEncode strAutoEncode( pwchT );

				// add it to our list
				pDirWatcherOverlapped->m_pDirWatcher->AddFileToChangeList( strAutoEncode.ToString() );
				delete[] pwchT;
				if ( pFileNotifyInformation->NextEntryOffset == 0 )
					break;

				// move to the next file
				pFileNotifyInformation = (FILE_NOTIFY_INFORMATION *)(((byte*)pFileNotifyInformation) + pFileNotifyInformation->NextEntryOffset);
			} while ( 1 );
		}


		// watch again
		pDirWatcherOverlapped->m_pDirWatcher->PostDirWatch();
	}
};
#elif defined(OSX)
void CheckDirectoryForChanges( const char *path_buff, CDirWatcher *pDirWatch, bool bRecurse )
{
	DIR *dir = opendir(path_buff);
	char fullpath[MAX_PATH];
	struct dirent *dirent;
	struct timespec ts = { 0, 0 };
	bool bTimeSet = false;
	
	while ( (dirent = readdir(dir)) != NULL ) 
	{
		if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
			continue;
		
		snprintf( fullpath, PATH_MAX, "%s/%s", path_buff, dirent->d_name );
		
		struct stat    st;
		if (lstat(fullpath, &st) != 0)
			continue;
		
		if ( S_ISDIR(st.st_mode) && bRecurse )
		{
			CheckDirectoryForChanges( fullpath, pDirWatch, bRecurse );
		}
		else if ( st.st_mtimespec.tv_sec > pDirWatch->m_modTime.tv_sec ||
				 ( st.st_mtimespec.tv_sec == pDirWatch->m_modTime.tv_sec && st.st_mtimespec.tv_nsec > pDirWatch->m_modTime.tv_nsec ) )
		{
			ts = st.st_mtimespec;
			bTimeSet = true;
			// the win32 size only sends up the dir relative to the watching dir, so replicate that here
			pDirWatch->AddFileToChangeList( fullpath + pDirWatch->m_BaseDir.Length() + 1 );
		}
	}

	if ( bTimeSet )
		pDirWatch->m_modTime = ts;
	closedir(dir);	
}

static void fsevents_callback( ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents,void *eventPaths, 
							  const FSEventStreamEventFlags eventMasks[], const FSEventStreamEventId eventIDs[] )
{
    char  path_buff[PATH_MAX];
	for (int i=0; i < numEvents; i++) 
	{
		char **paths = (char **)eventPaths;
		
        strcpy(path_buff, paths[i]);
        int len = strlen(path_buff);
        if (path_buff[len-1] == '/') 
		{
            // chop off a trailing slash
            path_buff[--len] = '\0';
        }
		
		bool bRecurse = false;
		
        if (eventMasks[i] & kFSEventStreamEventFlagMustScanSubDirs
			|| eventMasks[i] & kFSEventStreamEventFlagUserDropped
			|| eventMasks[i] & kFSEventStreamEventFlagKernelDropped) 
		{
            bRecurse = true;
        } 
		
		CDirWatcher *pDirWatch = (CDirWatcher *)clientCallBackInfo;
		// make sure its in our subdir
		if ( !V_strnicmp( path_buff, pDirWatch->m_BaseDir.String(), pDirWatch->m_BaseDir.Length() ) )
			CheckDirectoryForChanges( path_buff, pDirWatch, bRecurse );
    }
}




#endif

//-----------------------------------------------------------------------------
// Purpose: only one directory can be watched at a time
//-----------------------------------------------------------------------------
void CDirWatcher::SetDirToWatch( const char *pchDir )
{
	if ( !pchDir || !*pchDir )
		return;
	
	CPathString strPath( pchDir );
#ifdef WIN32
	// open the directory
	m_hFile = ::CreateFileW( strPath.GetWCharPathPrePended(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS, NULL );

	// create our buffers
	m_pFileInfo = malloc( k_cubDirWatchBufferSize );
	m_pOverlapped = malloc( sizeof( DirWatcherOverlapped ) );

	// post a watch
	PostDirWatch();
#elif defined(OSX)
	CFStringRef mypath = CFStringCreateWithCString( NULL, strPath.GetUTF8Path(), kCFStringEncodingMacRoman );
	if ( !mypath )
	{
		Assert( !"Failed to CFStringCreateWithCString watcher path" );
		return;
	}
	
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mypath, 1, NULL);
    FSEventStreamContext callbackInfo = {0, this, NULL, NULL, NULL};
    CFAbsoluteTime latency = 1.0; // Latency in seconds

    m_WatcherStream = (void *)FSEventStreamCreate(NULL,
								 &fsevents_callback,
								 &callbackInfo,
								 pathsToWatch,
								 kFSEventStreamEventIdSinceNow, 
								 latency,
								 kFSEventStreamCreateFlagNoDefer
								 );
	
    FSEventStreamScheduleWithRunLoop( (FSEventStreamRef)m_WatcherStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	CFRelease(pathsToWatch );
	CFRelease( mypath );
	
	FSEventStreamStart( (FSEventStreamRef)m_WatcherStream );

	char szFullPath[MAX_PATH];
	Q_MakeAbsolutePath( szFullPath, sizeof(szFullPath), pchDir );
	m_BaseDir = szFullPath;
	
	struct timeval tv;
	gettimeofday( &tv, NULL );
	TIMEVAL_TO_TIMESPEC( &tv, &m_modTime );
		
#else
	Assert( !"Impl me" );
#endif
}


#ifdef WIN32
//-----------------------------------------------------------------------------
// Purpose: used by callback functions to push a file onto the list
//-----------------------------------------------------------------------------
void CDirWatcher::PostDirWatch()
{
	memset( m_pOverlapped, 0, sizeof(DirWatcherOverlapped) );
	DirWatcherOverlapped *pDirWatcherOverlapped = (DirWatcherOverlapped *)m_pOverlapped;
	pDirWatcherOverlapped->m_pDirWatcher = this;

	DWORD dwBytes;
	::ReadDirectoryChangesW( m_hFile, m_pFileInfo, k_cubDirWatchBufferSize, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &dwBytes, (OVERLAPPED *)m_pOverlapped, &CDirWatcherFriend::DirWatchCallback );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: used by callback functions to push a file onto the list
//-----------------------------------------------------------------------------
void CDirWatcher::AddFileToChangeList( const char *pchFile )
{
	// make sure it isn't already in the list
	FOR_EACH_LL( m_listChangedFiles, i )
	{
		if ( !Q_stricmp( m_listChangedFiles[i], pchFile ) )
			return;
	}

	m_listChangedFiles.AddToTail( pchFile );
}


//-----------------------------------------------------------------------------
// Purpose: retrieve any changes
//-----------------------------------------------------------------------------
bool CDirWatcher::GetChangedFile( CUtlString *psFile )
{
#ifdef WIN32
	// this will trigger any pending directory reads
	// this does get hit other places in the code; so the callback can happen at any time
	::SleepEx( 0, TRUE );
#endif

	if ( !m_listChangedFiles.Count() )
		return false;

	*psFile = m_listChangedFiles[m_listChangedFiles.Head()];
	m_listChangedFiles.Remove( m_listChangedFiles.Head() );
	return true;
}



#ifdef DBGFLAG_VALIDATE
void CDirWatcher::Validate( CValidator &validator, const char *pchName )
{
	VALIDATE_SCOPE();

	validator.ClaimMemory( m_pOverlapped );
	validator.ClaimMemory( m_pFileInfo );
	ValidateObj( m_listChangedFiles );
	FOR_EACH_LL( m_listChangedFiles, i )
	{
		ValidateObj( m_listChangedFiles[i] );
	}
}
#endif

#endif // _PS3 || _X360