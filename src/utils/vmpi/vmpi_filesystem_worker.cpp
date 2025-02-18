//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <winsock2.h>
#include "vmpi_filesystem_internal.h"
#include "threadhelpers.h"
#include "zlib.h"
#include "tier0/fasttimer.h"
#include "tslist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NUM_BUFFERED_CHUNK_ACKS	512
#define ACK_FLUSH_INTERVAL		500	// Flush the ack queue twice per second.


static bool g_bReceivedMulticastIP = false;
static CIPAddr g_MulticastIP;

CVMPICriticalSection g_OpenCS;

class CFileResponse
{
public:
	int m_RequestID;
	int m_Response;
	bool m_bZeroLength;
};

CUtlVectorMT< CUtlVector< CFileResponse > > g_FileResponses;
int g_RequestID = 0;
CVMPICriticalSection g_csRequestID;


struct FileChunkPacket_t
{
	byte *m_pData;
	int m_nSize;
};
CTSQueue< FileChunkPacket_t > g_FileChunkPackets;



// ------------------------------------------------------------------------------------------------------------------------ //
// Classes.
// ------------------------------------------------------------------------------------------------------------------------ //

class CWorkerFile
{
public:
	const char* GetFilename() { return m_Filename.Base(); }
	const char* GetPathID() { return m_PathID.Base(); }
	bool IsReadyToRead() const { return m_nChunksToReceive == 0; }


public:
	CFastTimer m_Timer; // To see how long it takes to download the file.

	// This has to be sent explicitly as part of the file info or else the protocol 
	// breaks on empty files.
	bool m_bZeroLength;

	// This is false until we get any packets about the file. In the packets,
	// we find out what the size is supposed to be.
	bool m_bGotCompressedSize;

	// The ID the master uses to refer to this file.
	int m_FileID;

	CUtlVector<char> m_Filename;
	CUtlVector<char> m_PathID;

	// First data comes in here, then when it's all there, it is inflated into m_UncompressedData.
	CUtlVector<char> m_CompressedData;
	
	// 1 bit for each chunk.
	CUtlVector<unsigned char> m_ChunksReceived;

	// When this is zero, the file is done being received and m_UncompressedData is valid.
	int m_nChunksToReceive;
	CUtlVector<char> m_UncompressedData;
};

class CNonExistentFile
{
public:
	bool Equals( const char *pFilename, const char *pPathID )
	{
		return V_stricmp( pFilename, m_Filename.Get() ) == 0 &&
		       V_stricmp( pPathID,   m_PathID.Get()   ) == 0;
	}

public:
	CUtlString m_Filename;
	CUtlString m_PathID;
};



// ------------------------------------------------------------------------------------------------------------------------ //
// Global helpers.
// ------------------------------------------------------------------------------------------------------------------------ //

static void RecvMulticastIP( CIPAddr *pAddr )
{
	while ( !g_bReceivedMulticastIP )
		VMPI_DispatchNextMessage();

	*pAddr = g_MulticastIP;
}


static bool ZLibDecompress( const void *pInput, int inputLen, void *pOut, int outLen )
{
	if ( inputLen == 0 )
	{
		// Zero-length file?
		return true;
	}
	
	z_stream decompressStream;
	
	// Initialize the decompression stream.
	memset( &decompressStream, 0, sizeof( decompressStream ) );
	if ( inflateInit( &decompressStream ) != Z_OK )
		return false;

	// Decompress all this stuff and write it to the file.
	decompressStream.next_in = (unsigned char*)pInput;
	decompressStream.avail_in = inputLen;

	char *pOutChar = (char*)pOut;
	while ( decompressStream.avail_in )
	{
		decompressStream.total_out = 0;
		decompressStream.next_out = (unsigned char*)pOutChar;
		decompressStream.avail_out = outLen - (pOutChar - (char*)pOut);

		int ret = inflate( &decompressStream, Z_NO_FLUSH );
		if ( ret != Z_OK && ret != Z_STREAM_END )
			return false;


		pOutChar += decompressStream.total_out;

		if ( ret == Z_STREAM_END )
		{
			if ( (pOutChar - (char*)pOut) == outLen )
			{
				return true;
			}
			else
			{
				Assert( false );
				return false;
			}
		}
	}

	Assert( false ); // Should have gotten to Z_STREAM_END.
	return false;
}


// ------------------------------------------------------------------------------------------------------------------------ //
// CWorkerMulticastListener implementation.
// ------------------------------------------------------------------------------------------------------------------------ //

class CWorkerMulticastListener
{
public:
	CWorkerMulticastListener()
	{	   
		m_nUnfinishedFiles = 0;
	}
	
	~CWorkerMulticastListener()
	{
		Term();
	}

	bool Init( const CIPAddr &mcAddr )
	{
		m_MulticastAddr = mcAddr;
		m_hMainThread = GetCurrentThread();
		return true;
	}

	void Term()
	{
		m_WorkerFiles.PurgeAndDeleteElements();
	}

	
	CWorkerFile* RequestFileFromServer( const char *pFilename, const char *pPathID )
	{
		Assert( pPathID );
		Assert( FindWorkerFile( pFilename, pPathID ) == NULL );

		// Send a request to the master to find out if this file even exists.
		CVMPICriticalSectionLock lock( &g_csRequestID );
		lock.Lock();
		int requestID = g_RequestID++;
		lock.Unlock();

		unsigned char packetID[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_REQUEST };
		const void *pChunks[4] = { packetID, &requestID, (void*)pFilename, pPathID };
		int chunkLengths[4]  = { sizeof( packetID ), sizeof( requestID ), V_strlen( pFilename ) + 1, V_strlen( pPathID ) + 1 };
		VMPI_SendChunks( pChunks, chunkLengths, ARRAYSIZE( pChunks ), 0 );

		// Wait for the file ID to come back.
		CFileResponse response;
		response.m_Response = -1;
		response.m_bZeroLength = true;

		// We're in a worker thread.. the main thread should be dispatching all the messages, so let it
		// do that until we get our response.
		while ( 1 )
		{
			bool bGotIt = false;
			g_FileResponses.Lock();
			for ( int iResponse=0; iResponse < g_FileResponses.Count(); iResponse++ )
			{
				if ( g_FileResponses[iResponse].m_RequestID == requestID )
				{
					response = g_FileResponses[iResponse];
					g_FileResponses.Remove( iResponse );
					bGotIt = true;
					break;
				}
			}
			g_FileResponses.Unlock();

			if ( bGotIt )
				break;

			if ( GetCurrentThread() == m_hMainThread )
				VMPI_DispatchNextMessage( 20 );
			else
				Sleep( 20 );
		}
	
		// If we get -1 back, it means the file doesn't exist.
		int fileID = response.m_Response;
		if ( fileID == -1 )
		{
			// Remember that this file is bogus!
			bool foundIt = false;
			FOR_EACH_VEC( m_NonExistentFiles, i )
			{
				if ( m_NonExistentFiles[i].Equals( pFilename, pPathID ) )
					foundIt = true;
			}
			if ( !foundIt )
			{
				CNonExistentFile &notFile = m_NonExistentFiles[ m_NonExistentFiles.AddToTail() ];
				notFile.m_Filename = pFilename;
				notFile.m_PathID = pPathID;
			}

			// Get outta here!
			return NULL;
		}

		CWorkerFile *pTestFile = new CWorkerFile;
		
		pTestFile->m_Filename.SetCount( V_strlen( pFilename ) + 1 );
		strcpy( pTestFile->m_Filename.Base(), pFilename );

		pTestFile->m_PathID.SetCount( V_strlen( pPathID ) + 1 );
		strcpy( pTestFile->m_PathID.Base(), pPathID );

		pTestFile->m_FileID = fileID;
		pTestFile->m_nChunksToReceive = 9999;
		pTestFile->m_Timer.Start();
		pTestFile->m_bGotCompressedSize = false;
		pTestFile->m_bZeroLength = response.m_bZeroLength;

		m_WorkerFiles.AddToTail( pTestFile );
		++m_nUnfinishedFiles;

		return pTestFile;
	}

	void FlushAckChunks( unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], int &nChunksToAck, DWORD &lastAckTime )
	{
		if ( nChunksToAck )
		{
			// Tell the master we received this chunk.
			unsigned char packetID[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_CHUNK_RECEIVED };
			void *pChunks[2] = { packetID, chunksToAck };
			int chunkLengths[2] = { sizeof( packetID ), nChunksToAck * 4 };
			VMPI_SendChunks( pChunks, chunkLengths, 2, 0 );
			nChunksToAck = 0;
		}

		lastAckTime = GetTickCount();
	}

	void MaybeFlushAckChunks( unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], int &nChunksToAck, DWORD &lastAckTime )
	{
		if ( nChunksToAck && GetTickCount() - lastAckTime > ACK_FLUSH_INTERVAL )
			FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
	}

	void AddAckChunk( 
		unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], 
		int &nChunksToAck,
		DWORD &lastAckTime,
		int fileID,
		int iChunk )
	{
		chunksToAck[nChunksToAck][0] = (unsigned short)fileID;
		chunksToAck[nChunksToAck][1] = (unsigned short)iChunk;

		// TCP filesystem acks all chunks immediately so it'll send more.
		++nChunksToAck;
		if ( nChunksToAck == NUM_BUFFERED_CHUNK_ACKS || VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_TCP )
		{
			FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		}
	}

	// Returns the length of the packet's data or -1 if there is nothing.
	int CheckFileChunkPackets( char *data, int dataSize )
	{
		// Using TCP.. pop the next received packet off the stack.
		FileChunkPacket_t packet;
		if ( !g_FileChunkPackets.PopItem( &packet ) )
			return -1;

		// Yes, this is inefficient, but the amount of data we're handling here is tiny so the
		// effect is negligible.
		int len;
		if ( packet.m_nSize > dataSize )
		{
			len = -1;
			Warning( "CWorkerMulticastListener::ListenFor: Got a section of data too long (%d bytes).", packet.m_nSize );
		}
		else
		{
			memcpy( data, packet.m_pData, packet.m_nSize );
			len = packet.m_nSize;
		}

		free( packet.m_pData );
		return len;
	}
	
	void ShowSDKWorkerMsg( const char *pMsg, ... )
	{
		if ( !g_bMPIMaster && VMPI_IsSDKMode() )
		{
			va_list marker;
			va_start( marker, pMsg );
			char str[4096];
			V_vsnprintf( str, sizeof( str ), pMsg, marker );
			va_end( marker );
			Msg( "%s", str );
		}
	}

	// This is the main function the workers use to pick files out of the multicast stream.
	// The app is waiting for a specific file, but we receive and ack any files we can until 
	// we get the file they're looking for, then we return.
	//
	// NOTE: ideally, this would be in a thread, but it adds lots of complications and may
	// not be worth it.
	CWorkerFile* ListenFor( const char *pFilename, const char *pPathID )
	{
		CWorkerFile *pFile = FindWorkerFile( pFilename, pPathID );
		if ( !pFile )
		{
			// Ok, we haven't requested this file yet. Create an entry for it and
			// tell the master we'd like this file.
			pFile = RequestFileFromServer( pFilename, pPathID );
			if ( !pFile )
				return NULL;

			// If it's zero-length, we can return right now.
			if ( pFile->m_bZeroLength )
			{
				--m_nUnfinishedFiles;
				return pFile;
			}
		}

		// Setup a filename to print some debug spew with.
		char printableFilename[58];
		if ( V_strlen( pFilename ) > ARRAYSIZE( printableFilename ) - 1 )
		{
			V_strncpy( printableFilename, "[...]", sizeof( printableFilename ) );
			V_strncat( printableFilename, &pFilename[V_strlen(pFilename) - ARRAYSIZE(printableFilename) + 1 + V_strlen(printableFilename)], sizeof( printableFilename ) );
		}
		else
		{
			V_strncpy( printableFilename, pFilename, sizeof( printableFilename ) );
		}
		ShowSDKWorkerMsg( "\rRecv %s (0%%)  ", printableFilename );
		int iChunkPayloadSize = VMPI_GetChunkPayloadSize();

		// Now start listening to the stream.
		// Note: no need to setup anything when in TCP mode - we just use the regular
		// VMPI dispatch stuff to handle that.
		ISocket *pSocket = NULL;
		if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_MULTICAST )
		{
			pSocket = CreateMulticastListenSocket( m_MulticastAddr );
			
			if ( !pSocket )
			{
				char str[512];
				IP_GetLastErrorString( str, sizeof( str ) );
				Warning( "CreateMulticastListenSocket (%d.%d.%d.%d:%d) failed\n%s\n", EXPAND_ADDR( m_MulticastAddr ), str );
				return NULL;
			}
		}
		else if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_BROADCAST )
		{
			pSocket = CreateIPSocket();
			if ( !pSocket->BindToAny( m_MulticastAddr.port ) )
			{
				pSocket->Release();
				pSocket = NULL;
			}
		}

		unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2];
		int nChunksToAck = 0;
		DWORD lastAckTime = GetTickCount();

		// Now just receive multicast data until this file has been received.
		while ( m_nUnfinishedFiles > 0 )
		{
			char data[MAX_CHUNK_PAYLOAD_SIZE+1024];
			int len = -1;
			
			if ( pSocket )
			{
				CIPAddr ipFrom;
				len = pSocket->RecvFrom( data, sizeof( data ), &ipFrom );
			}
			else
			{
				len = CheckFileChunkPackets( data, sizeof( data ) );
			}

			if ( len == -1 )
			{
				// Sleep for 10ms and also handle socket errors.
				Sleep( 0 );
				VMPI_DispatchNextMessage( 10 );
				continue;
			}

			g_nMulticastBytesReceived += len;

			// Alrighty. Figure out what the deal is with this file.
			CMulticastFileInfo *pInfo = (CMulticastFileInfo*)data;
			int *piChunk = (int*)( pInfo + 1 );
			const char *pTestFilename = (const char*)( piChunk + 1 );
			const char *pPayload = pTestFilename + strlen( pFilename ) + 1;
			int payloadLen = len - ( pPayload - data );
			if ( payloadLen < 0 )
			{
				Warning( "CWorkerMulticastListener::ListenFor: invalid packet received on multicast group\n" );
				continue;
			}


			if ( pInfo->m_FileID != pFile->m_FileID )
				continue;

			CWorkerFile *pTestFile = FindWorkerFile( pInfo->m_FileID );
			if ( !pTestFile )
				Plat_FatalError( "FindWorkerFile( %s ) failed\n", pTestFilename );

			// TODO: reenable this code and disable the if right above here.
			// We always get "invalid payload length" errors on the workers when using this, but
			// I haven't been able to figure out why yet.
			/*
			// Put the data into whatever file it belongs in.
			if ( !pTestFile )
			{
				pTestFile = RequestFileFromServer( pTestFilename );
				if ( !pTestFile )
					continue;
			}
			*/

			// Is this the first packet about this file?
			if ( !pTestFile->m_bGotCompressedSize )
			{
				pTestFile->m_bGotCompressedSize = true;
				pTestFile->m_CompressedData.SetCount( pInfo->m_CompressedSize );
				pTestFile->m_UncompressedData.SetCount( pInfo->m_UncompressedSize );
				pTestFile->m_ChunksReceived.SetCount( PAD_NUMBER( pInfo->m_nChunks, 8 ) / 8 );
				pTestFile->m_nChunksToReceive = pInfo->m_nChunks;
				memset( pTestFile->m_ChunksReceived.Base(), 0, pTestFile->m_ChunksReceived.Count() );
			}

			// Validate the chunk index and uncompressed size.
			int iChunk = *piChunk;
			if ( iChunk < 0 || iChunk >= pInfo->m_nChunks )
			{
				Plat_FatalError( "ListenFor(): invalid chunk index (%d) for file '%s'\n", iChunk, pTestFilename );
			}

			// Only handle this if we didn't already received the chunk.
			if ( !(pTestFile->m_ChunksReceived[iChunk >> 3] & (1 << (iChunk & 7))) )
			{
				// Make sure the file is properly setup to receive the data into.
				if ( (int)pInfo->m_UncompressedSize != pTestFile->m_UncompressedData.Count() ||
					(int)pInfo->m_CompressedSize != pTestFile->m_CompressedData.Count() )
				{
					Plat_FatalError( "ListenFor(): invalid compressed or uncompressed size.\n"
						"pInfo = '%s', pTestFile = '%s'\n"
						"Compressed   (pInfo = %d, pTestFile = %d)\n"
						"Uncompressed (pInfo = %d, pTestFile = %d)\n", 
						pTestFilename,
						pTestFile->GetFilename(),
						pInfo->m_CompressedSize,
						pTestFile->m_CompressedData.Count(),
						pInfo->m_UncompressedSize,
						pTestFile->m_UncompressedData.Count()
						);
				}
				
				int iChunkStart = iChunk * iChunkPayloadSize;
				int iChunkEnd = min( iChunkStart + iChunkPayloadSize, pTestFile->m_CompressedData.Count() );
				int chunkLen = iChunkEnd - iChunkStart;

				if ( chunkLen != payloadLen )
				{
					Plat_FatalError( "ListenFor(): invalid payload length for '%s' (%d should be %d)\n"
						"pInfo = '%s', pTestFile = '%s'\n"
						"Chunk %d out of %d. Compressed size: %d\n", 
						pTestFile->GetFilename(),
						payloadLen, 
						chunkLen,
						pTestFilename,
						pTestFile->GetFilename(),
						iChunk,
						pInfo->m_nChunks,
						pInfo->m_CompressedSize
						);
				}

				memcpy( &pTestFile->m_CompressedData[iChunkStart], pPayload, chunkLen );
				pTestFile->m_ChunksReceived[iChunk >> 3] |= (1 << (iChunk & 7));

				--pTestFile->m_nChunksToReceive;

				if ( pTestFile == pFile )
				{
					int percent = 100 - (100 * pFile->m_nChunksToReceive) / pInfo->m_nChunks;
					ShowSDKWorkerMsg( "\rRecv %s (%d%%) [chunk %d/%d] ", printableFilename, percent, pInfo->m_nChunks - pFile->m_nChunksToReceive, pInfo->m_nChunks );
				}

				// Remember to ack what we received.
				AddAckChunk( chunksToAck, nChunksToAck, lastAckTime, pInfo->m_FileID, iChunk );
				
				// If we're done receiving the data, unpack it.
				if ( pTestFile->m_nChunksToReceive == 0 )
				{
					// Ack the file.				   
					FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );

					pTestFile->m_Timer.End();

					pTestFile->m_UncompressedData.SetCount( pInfo->m_UncompressedSize );
					--m_nUnfinishedFiles;

					if ( !ZLibDecompress( 
						pTestFile->m_CompressedData.Base(), 
						pTestFile->m_CompressedData.Count(),
						pTestFile->m_UncompressedData.Base(),
						pTestFile->m_UncompressedData.Count() ) )
					{
						if ( pSocket )
							pSocket->Release();
						FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
						Plat_FatalError( "ZLibDecompress failed.\n" );
						return NULL;
					}

					char str[512];
					V_snprintf( str, sizeof( str ), "Got %s (%dk) in %.2fs", 
						printableFilename, 
						(pTestFile->m_UncompressedData.Count() + 511) / 1024,
						pTestFile->m_Timer.GetDuration().GetSeconds()
						);
					Msg( "\r%-79s\n", str );

					// Won't be needing this anymore.
					pTestFile->m_CompressedData.Purge();
				}
			}

			MaybeFlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		}

		Assert( pFile->IsReadyToRead() );
		FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		if ( pSocket )
			pSocket->Release();

		return pFile;
	}

	CWorkerFile* FindWorkerFile( const char *pFilename, const char *pPathID ) 
	{
		FOR_EACH_LL( m_WorkerFiles, i )
		{
			CWorkerFile *pWorkerFile = m_WorkerFiles[i];

			if ( V_stricmp( pWorkerFile->GetFilename(), pFilename ) == 0 && V_stricmp( pWorkerFile->GetPathID(), pPathID ) == 0 )
				return pWorkerFile;
		}
		return NULL;
	}

	CWorkerFile* FindWorkerFile( int fileID ) 
	{
		FOR_EACH_LL( m_WorkerFiles, i )
		{
			if ( m_WorkerFiles[i]->m_FileID == fileID )
				return m_WorkerFiles[i];
		}
		return NULL;
	}

	bool FileMightExist( const char *pFilename, const char *pPathID )
	{
		FOR_EACH_VEC( m_NonExistentFiles, i )
		{
			if ( m_NonExistentFiles[i].Equals( pFilename, pPathID ) )
				return false;
		}
		
		return true;
	}


private:
	CIPAddr m_MulticastAddr;

	CUtlLinkedList< CWorkerFile*, int > m_WorkerFiles;
	CUtlVector< CNonExistentFile > m_NonExistentFiles;

	HANDLE m_hMainThread;

	// How many files do we have open that we haven't finished receiving from the server yet?
	// We always keep waiting for data until this is zero.
	int m_nUnfinishedFiles;
};



// ------------------------------------------------------------------------------------------------------------------------ //
// CWorkerVMPIFileSystem implementation.
// ------------------------------------------------------------------------------------------------------------------------ //

class CWorkerVMPIFileSystem : public CBaseVMPIFileSystem
{
public:
	InitReturnVal_t Init();
	virtual void Term();

	virtual FileHandle_t Open( const char *pFilename, const char *pOptions, const char *pathID );
	virtual bool HandleFileSystemPacket( MessageBuffer *pBuf, int iSource, int iPacketID );

	virtual void CreateVirtualFile( const char *pFilename, const void *pData, int fileLength );
	virtual long GetFileTime( const char *pFileName, const char *pathID );
	virtual bool IsFileWritable( const char *pFileName, const char *pPathID );
	virtual bool SetFileWritable( char const *pFileName, bool writable, const char *pPathID );

	virtual CSysModule 		*LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly );
	virtual void			UnloadModule( CSysModule *pModule );

private:
	CWorkerMulticastListener m_Listener;
};


CBaseVMPIFileSystem* CreateWorkerVMPIFileSystem()
{
	CWorkerVMPIFileSystem *pRet = new CWorkerVMPIFileSystem;
	g_pBaseVMPIFileSystem = pRet;
	if ( pRet->Init() )
	{
		return pRet;
	}
	else
	{
		delete pRet;
		g_pBaseVMPIFileSystem = NULL;
		return NULL;
	}
}


InitReturnVal_t CWorkerVMPIFileSystem::Init()
{
	// Get the multicast addr to listen on.
	CIPAddr mcAddr;
	RecvMulticastIP( &mcAddr );

	return m_Listener.Init( mcAddr ) ? INIT_OK : INIT_FAILED;
}


void CWorkerVMPIFileSystem::Term()
{
	m_Listener.Term();
}


FileHandle_t CWorkerVMPIFileSystem::Open( const char *pFilename, const char *pOptions, const char *pathID )
{
	Assert( g_bUseMPI );

	// When it finally asks the filesystem for a file, it'll pass NULL for pathID if it's "".
	if ( !pathID )
		pathID = "";

	if ( g_bDisableFileAccess )
		Plat_FatalError( "Open( %s, %s ) - file access has been disabled.", pFilename, pOptions );

	// Workers can't open anything for write access.
	bool bWriteAccess = (V_stristr( pOptions, "w" ) != 0);
	if ( bWriteAccess )
		return FILESYSTEM_INVALID_HANDLE;

	// Don't let multiple threads on a single worker machine
	//   get in here at the same time.
	CVMPICriticalSectionLock lock( &g_OpenCS );
	lock.Lock();

	// Have tried to load this file and failed?
	if ( !m_Listener.FileMightExist( pFilename, pathID ) )
		return FILESYSTEM_INVALID_HANDLE;

	// Do we have this file's data already?
	CWorkerFile *pFile = m_Listener.FindWorkerFile( pFilename, pathID );
	if ( !pFile || !pFile->IsReadyToRead() )
	{
		// Ok, start listening to the multicast stream until we get the file we want.
		
		// NOTE: it might make sense here to have the client ask for a list of ALL the files that
		// the master currently has and wait to receive all of them (so we don't come back a bunch
		// of times and listen 

		// NOTE NOTE: really, the best way to do this is to have a thread on the workers that sits there
		// and listens to the multicast stream. Any time the master opens a new file up, it assumes
		// all the workers need the file, and it starts to send it on the multicast stream until
		// the worker threads respond that they all have it.
		//
		// (NOTE: this probably means that the clients would have to ack the chunks on a UDP socket that
		// the thread owns).
		//
		// This would simplify all the worries about a client missing half the stream and having to 
		// wait for another cycle through it.
		pFile = m_Listener.ListenFor( pFilename, pathID );

		if ( !pFile )
		{
			return FILESYSTEM_INVALID_HANDLE;
		}
	}

	// Ok! Got the file. now setup a memory stream they can read out of it with.
	CVMPIFile_Memory *pOut = new CVMPIFile_Memory;
	pOut->Init( pFile->m_UncompressedData.Base(), pFile->m_UncompressedData.Count(), strchr( pOptions, 't' ) ? 't' : 'b' );
	return (FileHandle_t)pOut;
}


void CWorkerVMPIFileSystem::CreateVirtualFile( const char *pFilename, const void *pData, int fileLength )
{
	Plat_FatalError( "CreateVirtualFile not supported in VMPI worker filesystem." );
}


long CWorkerVMPIFileSystem::GetFileTime( const char *pFileName, const char *pathID )
{
	Plat_FatalError( "GetFileTime not supported in VMPI worker filesystem." );
	return 0;
}


bool CWorkerVMPIFileSystem::IsFileWritable( const char *pFileName, const char *pPathID )
{
	Plat_FatalError( "GetFileTime not supported in VMPI worker filesystem." );
	return false;
}


bool CWorkerVMPIFileSystem::SetFileWritable( char const *pFileName, bool writable, const char *pPathID )
{
	Plat_FatalError( "GetFileTime not supported in VMPI worker filesystem." );
	return false;
}

bool CWorkerVMPIFileSystem::HandleFileSystemPacket( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	// Handle this packet.
	int subPacketID = pBuf->data[1];
	switch( subPacketID )
	{
		case VMPI_FSPACKETID_MULTICAST_ADDR:
		{
			char *pInPos = &pBuf->data[2];
			
			g_MulticastIP = *((CIPAddr*)pInPos);
			pInPos += sizeof( g_MulticastIP );
			
			g_bReceivedMulticastIP = true;
		}
		return true;

		case VMPI_FSPACKETID_FILE_RESPONSE:
		{
			CFileResponse res;
			res.m_RequestID = *((int*)&pBuf->data[2]);
			res.m_Response = *((int*)&pBuf->data[6]);
			res.m_bZeroLength = *((bool*)&pBuf->data[10]);

			g_FileResponses.Lock();
			g_FileResponses.AddToTail( res );
			g_FileResponses.Unlock();
		}
		return true;
	
		case VMPI_FSPACKETID_FILE_CHUNK:
		{
			int nDataBytes = pBuf->getLen() - 2;
			
			FileChunkPacket_t packet;
			packet.m_nSize = nDataBytes;
			packet.m_pData = ( byte* )malloc( nDataBytes );
			memcpy( packet.m_pData, &pBuf->data[2], nDataBytes );

			g_FileChunkPackets.PushItem( packet );
		}
		return true;
		
		default:
			return false;
	}
}

CSysModule* CWorkerVMPIFileSystem::LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly )
{
	return Sys_LoadModule( pFileName );
}

void CWorkerVMPIFileSystem::UnloadModule( CSysModule *pModule )
{
	Sys_UnloadModule( pModule );
}
