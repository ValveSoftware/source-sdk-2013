//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vmpi_filesystem_internal.h"
#include "tier1/utlbuffer.h"


bool g_bDisableFileAccess = false;


CBaseVMPIFileSystem *g_pBaseVMPIFileSystem = NULL;
IFileSystem *g_pOriginalPassThruFileSystem = NULL;

void* GetVMPIFileSystem()
{
	return (IBaseFileSystem*)g_pBaseVMPIFileSystem;
}

EXPOSE_INTERFACE_FN( GetVMPIFileSystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION )

void *GetFullFileSystem()
{
	return (IFileSystem*)g_pOriginalPassThruFileSystem;
}

EXPOSE_INTERFACE_FN( GetFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION )

IFileSystem* VMPI_FileSystem_Init( int maxMemoryUsage, IFileSystem *pPassThru )
{
	Assert( g_bUseMPI );
	Assert( !g_pBaseVMPIFileSystem );
	g_pOriginalPassThruFileSystem = pPassThru;

	if ( g_bMPIMaster )
	{
		extern CBaseVMPIFileSystem* CreateMasterVMPIFileSystem( int maxMemoryUsage, IFileSystem *pPassThru );
		CreateMasterVMPIFileSystem( maxMemoryUsage, pPassThru );
	}
	else
	{
		extern CBaseVMPIFileSystem* CreateWorkerVMPIFileSystem();
		CreateWorkerVMPIFileSystem();
	}
	
	// The Create function should have set this. Normally, we'd set g_pBaseVMPIFileSystem right here, but 
	// the create functions may want to receive some messages, in which case they need to set g_pBaseVMPIFileSystem
	// so the packets get routed appropriately.
	Assert( g_pBaseVMPIFileSystem );
	return g_pBaseVMPIFileSystem;
}


IFileSystem* VMPI_FileSystem_Term()
{
	if ( g_pBaseVMPIFileSystem )
	{
		g_pBaseVMPIFileSystem->Release();
		g_pBaseVMPIFileSystem = NULL;

		if ( g_iVMPIVerboseLevel >= 1 )
		{
			if ( g_bMPIMaster )
				Msg( "Multicast send: %dk\n", (g_nMulticastBytesSent + 511) / 1024 );
			else
				Msg( "Multicast recv: %dk\n", (g_nMulticastBytesReceived + 511) / 1024 );
		}
	}
	
	IFileSystem *pRet = g_pOriginalPassThruFileSystem;
	g_pOriginalPassThruFileSystem = NULL;
	return pRet;
}


void VMPI_FileSystem_DisableFileAccess()
{
	g_bDisableFileAccess = true;
}


CreateInterfaceFn VMPI_FileSystem_GetFactory()
{
	return Sys_GetFactoryThis();
}


void VMPI_FileSystem_CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength )
{
	g_pBaseVMPIFileSystem->CreateVirtualFile( pFilename, pData, fileLength );
}


// Register our packet ID.
bool FileSystemRecv( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	if ( g_pBaseVMPIFileSystem )
		return g_pBaseVMPIFileSystem->HandleFileSystemPacket( pBuf, iSource, iPacketID );
	else
		return false;
}


CDispatchReg g_DispatchReg_FileSystem( VMPI_PACKETID_FILESYSTEM, FileSystemRecv );

VMPI_REGISTER_PACKET_ID( VMPI_PACKETID_FILESYSTEM );



// ------------------------------------------------------------------------------------------------------------------------ //
// CVMPIFile_Memory implementation.
// ------------------------------------------------------------------------------------------------------------------------ //

void CVMPIFile_Memory::Init( const char *pData, long len, char chMode /* = 'b' */ )
{
	m_pData = pData;
	m_DataLen = len;
	m_iCurPos = 0;
	m_chMode = chMode;
}

void CVMPIFile_Memory::Close()
{
	delete this;
}

void CVMPIFile_Memory::Seek( int pos, FileSystemSeek_t seekType )
{
	if ( seekType == FILESYSTEM_SEEK_HEAD )
		m_iCurPos = pos;
	else if ( seekType == FILESYSTEM_SEEK_CURRENT )
		m_iCurPos += pos;
	else
		m_iCurPos = m_DataLen - pos;
}

unsigned int CVMPIFile_Memory::Tell()
{
	return m_iCurPos; 
}

unsigned int CVMPIFile_Memory::Size() 
{
	return m_DataLen; 
}

void CVMPIFile_Memory::Flush()
{
}

int CVMPIFile_Memory::Read( void* pOutput, int size ) 
{ 
	Assert( m_iCurPos >= 0 );
	int nToRead = min( (int)(m_DataLen - m_iCurPos), size );
	
	if ( m_chMode != 't' )
	{
		memcpy( pOutput, &m_pData[m_iCurPos], nToRead );
		m_iCurPos += nToRead;

		return nToRead;
	}
	else
	{
		int iRead = 0;
		const char *pData = m_pData + m_iCurPos;
		int len = m_DataLen - m_iCurPos;
		
		// Perform crlf translation
		while ( const char *crlf = ( const char * ) memchr( pData, '\r', len ) )
		{
			int canCopy = min( size, crlf - pData );
			memcpy( pOutput, pData, canCopy );
			
			m_iCurPos += canCopy;
			pData += canCopy;
			len -= canCopy;
			
			iRead += canCopy;
			( char * & ) pOutput += canCopy;
			size -= canCopy;

			if ( size && len )
			{
				if ( ( len > 1 ) && ( pData[1] == '\n' ) )
				{
					++ m_iCurPos;
					++ pData;
					-- len;
				}

				* ( char * & ) pOutput = *pData;
				
				++ m_iCurPos;
				++ pData;
				-- len;

				++ iRead;
				++ ( char * & ) pOutput;
				-- size;
			}
			else
				break;
		}
		
		if ( size && len )
		{
			// No crlf characters left
			int canCopy = min( size, len );
			memcpy( pOutput, pData, canCopy );

			m_iCurPos += canCopy;
			pData += canCopy;
			len -= canCopy;

			iRead += canCopy;
			( char * & ) pOutput += canCopy;
			size -= canCopy;
		}

		return iRead;
	}
}

int CVMPIFile_Memory::Write( void const* pInput, int size )
{
	Assert( false ); return 0; 
}


// ------------------------------------------------------------------------------------------------------------------------ //
// CBaseVMPIFileSystem implementation.
// ------------------------------------------------------------------------------------------------------------------------ //

CBaseVMPIFileSystem::~CBaseVMPIFileSystem()
{
}


void CBaseVMPIFileSystem::Release()
{
	delete this;
}


void CBaseVMPIFileSystem::Close( FileHandle_t file )
{
	if ( file )
		((IVMPIFile*)file)->Close();
}

int CBaseVMPIFileSystem::Read( void* pOutput, int size, FileHandle_t file )
{
	return ((IVMPIFile*)file)->Read( pOutput, size );
}

int CBaseVMPIFileSystem::Write( void const* pInput, int size, FileHandle_t file )
{
	return ((IVMPIFile*)file)->Write( pInput, size );
}

void CBaseVMPIFileSystem::Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType )
{
	((IVMPIFile*)file)->Seek( pos, seekType );
}

unsigned int CBaseVMPIFileSystem::Tell( FileHandle_t file )
{
	return ((IVMPIFile*)file)->Tell();
}

unsigned int CBaseVMPIFileSystem::Size( FileHandle_t file )
{
	return ((IVMPIFile*)file)->Size();
}

unsigned int CBaseVMPIFileSystem::Size( const char *pFilename, const char *pathID = 0 )
{
	FileHandle_t hFile = Open( pFilename, "rb", NULL );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		return 0;
	}
	else
	{
		unsigned int ret = Size( hFile );
		Close( hFile );
		return ret;
	}
}

bool CBaseVMPIFileSystem::FileExists( const char *pFileName, const char *pPathID )
{
	FileHandle_t hFile = Open( pFileName, "rb", NULL );
	if ( hFile )
	{
		Close( hFile );
		return true;
	}
	else
	{
		return false;
	}
}

void CBaseVMPIFileSystem::Flush( FileHandle_t file )
{
	((IVMPIFile*)file)->Flush();
}	

bool CBaseVMPIFileSystem::Precache( const char* pFileName, const char *pPathID )
{
	return false;
} 


//-----------------------------------------------------------------------------
// NOTE: This is an exact copy of code in BaseFileSystem.cpp which
// has to be here because they want to call
// the implementation of Open/Size/Read/Write in CBaseVMPIFileSystem
//-----------------------------------------------------------------------------
bool CBaseVMPIFileSystem::ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes, int nStartingByte, FSAllocFunc_t pfnAlloc )
{
	const char *pReadFlags = "rb";
	if ( buf.IsText() && !buf.ContainsCRLF() )
	{
		pReadFlags = "rt";
	}

	FileHandle_t fp = Open( pFileName, buf.IsText() ? "rt" : "rb", pPath );
	if ( !fp )
		return false;

	int nBytesToRead = Size( fp );
	if ( nMaxBytes > 0 )
	{
		nBytesToRead = min( nMaxBytes, nBytesToRead );
	}
	buf.EnsureCapacity( nBytesToRead + buf.TellPut() );

	if ( nStartingByte != 0 )
	{
		Seek( fp, nStartingByte, FILESYSTEM_SEEK_HEAD );
	}

	int nBytesRead = Read( buf.PeekPut(), nBytesToRead, fp );
	buf.SeekPut( CUtlBuffer::SEEK_CURRENT, nBytesRead );

	Close( fp );
	return (nBytesRead != 0);
}

bool CBaseVMPIFileSystem::WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf )
{
	const char *pWriteFlags = "wb";
	if ( buf.IsText() && !buf.ContainsCRLF() )
	{
		pWriteFlags = "wt";
	}

	FileHandle_t fp = Open( pFileName, buf.IsText() ? "wt" : "wb", pPath );
	if ( !fp )
		return false;

	int nBytesWritten = Write( buf.Base(), buf.TellMaxPut(), fp );

	Close( fp );
	return (nBytesWritten != 0);
}

