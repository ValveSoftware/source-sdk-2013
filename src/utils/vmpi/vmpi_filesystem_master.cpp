//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <winsock2.h>
#include "vmpi_filesystem_internal.h"
#include "zlib.h"
#include "vstdlib/random.h"
#include "tier0/fasttimer.h"


#define MINIMUM_SLEEP_MS				1

// NOTE: This number comes from measurements on our network to find out how fast
// we can broadcast without the network freaking out.
//
// This number can be changed on the command line with the -mpi_FileTransmitRate parameter.
int MULTICAST_TRANSMIT_RATE = (1024*1000);	// N megs per second

// Defines when we'll stop transmitting a file to a client.
// (After we've transmitted the file to the client N times and we haven't heard an ack back for M seconds).
#define MIN_FILE_CYCLE_COUNT		5
#define CLIENT_FILE_ACK_TIMEOUT		20


// Register all our packet IDs so they show up in debugging.
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_REQUEST )
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_RESPONSE	)
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_CHUNK_RECEIVED )
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_RECEIVED	)
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_MULTICAST_ADDR )
VMPI_REGISTER_SUBPACKET_ID( VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_CHUNK )


// ------------------------------------------------------------------------------------------------------------------------ //
// Global helpers.
// ------------------------------------------------------------------------------------------------------------------------ //

static void SendMulticastIP( const CIPAddr *pAddr )
{
	unsigned char packetID[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_MULTICAST_ADDR };
	VMPI_Send2Chunks( 
		packetID, sizeof( packetID ),
		pAddr, sizeof( *pAddr ),
		VMPI_PERSISTENT );
}


static bool IsOpeningForWriteAccess( const char *pOptions )
{
	return strchr( pOptions, 'w' ) || strchr( pOptions, 'a' ) || strchr( pOptions, '+' );
}


// This does a fast zlib compression of the source data into the 'out' buffer.
static bool ZLibCompress( const void *pData, int len, CUtlVector<char> &out )
{
	if ( len == 0 )
	{
		out.Purge();
		return true;
	}

	int outStartLen = len;
RETRY:;

	// Prepare the compression stream.
	z_stream zs;
	memset( &zs, 0, sizeof( zs ) );
	
	if ( deflateInit( &zs, 1 ) != Z_OK )
		return false;
	

	// Now compress it into the output buffer.
	out.SetSize( outStartLen );

	zs.next_in = (unsigned char*)pData;
	zs.avail_in = len;

	zs.next_out = (unsigned char*)out.Base();
	zs.avail_out = out.Count();

	int ret = deflate( &zs, Z_FINISH );
	deflateEnd( &zs );

	if ( ret == Z_STREAM_END )
	{
		// Get rid of whatever was left over.
		out.RemoveMultiple( zs.total_out, out.Count() - zs.total_out );
		return true;
	}
	else if ( ret == Z_OK )
	{
		// Need more space in the output buffer.
		outStartLen += 1024 * 128;
		goto RETRY;
	}
	else
	{
		return false;
	}
}


// ------------------------------------------------------------------------------------------------------------------------ //
// CVMPIFile_PassThru
// ------------------------------------------------------------------------------------------------------------------------ //

class CVMPIFile_PassThru : public IVMPIFile
{
public:
	void Init( IBaseFileSystem *pPassThru, FileHandle_t fp )
	{
		m_pPassThru = pPassThru;
		m_fp = fp;
	}

	virtual void Close()
	{
		m_pPassThru->Close( m_fp );
		delete this;
	}

	virtual void Seek( int pos, FileSystemSeek_t seekType )
	{
		m_pPassThru->Seek( m_fp, pos, seekType );
	}

	virtual unsigned int Tell()
	{
		return m_pPassThru->Tell( m_fp );
	}

	virtual unsigned int Size()
	{
		return m_pPassThru->Size( m_fp );
	}

	virtual void Flush()
	{
		m_pPassThru->Flush( m_fp );
	}
	
	virtual int Read( void* pOutput, int size )
	{
		return m_pPassThru->Read( pOutput, size, m_fp );
	}

	virtual int Write( void const* pInput, int size )
	{
		return m_pPassThru->Write( pInput, size, m_fp ); 
	}


private:
	IBaseFileSystem *m_pPassThru;
	FileHandle_t m_fp;
};



// ---------------------------------------------------------------------------------------------------- //
// CTransmitRateMgr coordinates with any other currently-running VMPI jobs, and they all will cut
// down their transmission rate to stay within MULTICAST_TRANSMIT_RATE.
// ---------------------------------------------------------------------------------------------------- //

#define TRANSMITRATEMGR_BROADCAST_INVERVAL	(1.0 / 3.0)	// How many times per second we broadcast our presence.
#define TRANSMITRATEMGR_EXPIRE_TIME			0.7			// How long it'll go before deciding a guy is not transmitting anymore.

static char s_cTransmitRateMgrPacket[] = {2,6,-3,2,1,-66};

class CTransmitRateMgr
{
public:
	CTransmitRateMgr();

	void ReadPackets();
	void BroadcastPresence();

	double GetMicrosecondsPerByte() const;

private:
	class CMachineRecord
	{
	public:
		unsigned long m_UniqueID;
		float m_flLastTime;
	};
	CUtlVector<CMachineRecord> m_MachineRecords;

	unsigned long m_UniqueID;
	float m_flLastBroadcastTime;
	double	m_nMicrosecondsPerByte;
	ISocket *m_pSocket;
};

CTransmitRateMgr::CTransmitRateMgr()
{
	m_nMicrosecondsPerByte = 1000000.0 / (double)MULTICAST_TRANSMIT_RATE;
	m_flLastBroadcastTime = 0;

	// Build a (hopefully) unique ID.
	m_UniqueID = (unsigned long)this;
	CCycleCount cnt;
	cnt.Sample();
	m_UniqueID += cnt.GetMicroseconds();
	Sleep( 1 );
	m_UniqueID += cnt.GetMicroseconds();

	m_pSocket = CreateIPSocket();
	if ( m_pSocket )
	{
		m_pSocket->BindToAny( VMPI_MASTER_FILESYSTEM_BROADCAST_PORT );
	}
}

void CTransmitRateMgr::ReadPackets()
{
	if ( !m_pSocket )
		return;

	float flCurTime = Plat_FloatTime();

	// First, update/add records.
	while ( 1 )
	{
		char data[VMPI_PACKET_SIZE];
		CIPAddr ipFrom;
		int len = m_pSocket->RecvFrom( data, sizeof( data ), &ipFrom );
		if ( len == -1 )
			break;
		
		if ( len == sizeof( s_cTransmitRateMgrPacket ) + sizeof( unsigned long ) && 
			 memcmp( data, s_cTransmitRateMgrPacket, sizeof( s_cTransmitRateMgrPacket ) ) == 0 )
		{
			unsigned long id = *((unsigned long*)&data[sizeof(s_cTransmitRateMgrPacket)]);
			if ( id == m_UniqueID )
				continue;

			int i;
			for ( i=0; i < m_MachineRecords.Count(); i++ )
			{
				if ( m_MachineRecords[i].m_UniqueID == id )
				{
					m_MachineRecords[i].m_flLastTime = flCurTime;
					break;
				}
			}

			if ( i == m_MachineRecords.Count() )			   
			{
				int index = m_MachineRecords.AddToTail();
				m_MachineRecords[index].m_UniqueID = id;
				m_MachineRecords[index].m_flLastTime = flCurTime;
			}
		}
	}

	// Now, expire any old records.
	for ( int i=0; i < m_MachineRecords.Count(); i++ )
	{
		if ( (flCurTime - m_MachineRecords[i].m_flLastTime) > TRANSMITRATEMGR_EXPIRE_TIME )
		{
			m_MachineRecords.Remove( i );
			--i;
		}
	}

	// Recalculate our transmit rate (assuming we're receiving our own broadcast packets).
	m_nMicrosecondsPerByte = 1000000.0 / (double)(MULTICAST_TRANSMIT_RATE / (m_MachineRecords.Count() + 1));
}

void CTransmitRateMgr::BroadcastPresence()
{
	if ( !m_pSocket )
		return;

	float flCurTime = Plat_FloatTime();
	if ( (flCurTime - m_flLastBroadcastTime) < TRANSMITRATEMGR_BROADCAST_INVERVAL )
		return;

	m_flLastBroadcastTime = flCurTime;
	
	char cData[sizeof( s_cTransmitRateMgrPacket ) + sizeof( unsigned long )];    
	memcpy( cData, s_cTransmitRateMgrPacket, sizeof( s_cTransmitRateMgrPacket ) );
	*((unsigned long*)&cData[ sizeof( s_cTransmitRateMgrPacket ) ] ) = m_UniqueID;

	m_pSocket->Broadcast( cData, sizeof( cData ), VMPI_MASTER_FILESYSTEM_BROADCAST_PORT );
}

inline double CTransmitRateMgr::GetMicrosecondsPerByte() const
{
	return m_nMicrosecondsPerByte;
}


// ---------------------------------------------------------------------------------------------------- //
// CRateLimiter manages waiting for small periods of time between packets so the rate is
// whatever we want it to be.
//
// It also will give up some CPU time to other processes every 50 milliseconds.
// ---------------------------------------------------------------------------------------------------- //

class CRateLimiter
{
public:

			CRateLimiter();
	
	void	GiveUpTimeSlice();
	void	NoteExcessTimeTaken( unsigned long excessTimeInMicroseconds );


public:

	DWORD m_SleepIntervalMS; // Give up a timeslice every N milliseconds.

	// Since we sleep once in a while, we time how long the sleep took and we beef 
	// up the transmit rate until we've accounted for the time lost during the sleep.
	DWORD m_AccumulatedSleepMicroseconds;

	// When was the last time we gave up a little bit of CPU to other programs.
	CCycleCount m_LastSleepTime;
};

CRateLimiter::CRateLimiter()
{
	m_SleepIntervalMS = 50;
	m_AccumulatedSleepMicroseconds = 0;
	m_LastSleepTime.Sample();
}

void CRateLimiter::GiveUpTimeSlice()
{
	// Sleep again?
	CCycleCount currentTime, dtSinceLastSleep;
	currentTime.Sample();
	CCycleCount::Sub( currentTime, m_LastSleepTime, dtSinceLastSleep );

	if ( dtSinceLastSleep.GetMilliseconds() >= m_SleepIntervalMS )
	{
		CFastTimer sleepTimer;
		
		sleepTimer.Start();
		Sleep( 10 );
		sleepTimer.End();

		m_AccumulatedSleepMicroseconds += sleepTimer.GetDuration().GetMicroseconds();
		m_LastSleepTime.Sample();
	}
}


void CRateLimiter::NoteExcessTimeTaken( unsigned long excessTimeInMicroseconds )
{
	// Note: we give up time slices above.
	if ( excessTimeInMicroseconds > m_AccumulatedSleepMicroseconds )
	{ 
		excessTimeInMicroseconds -= m_AccumulatedSleepMicroseconds;
		m_AccumulatedSleepMicroseconds = 0;

		CCycleCount startCount;
		startCount.Sample();
		while ( 1 )
		{
			CCycleCount curCount, diff;
			curCount.Sample();

			CCycleCount::Sub( curCount, startCount, diff );
			if ( diff.GetMicroseconds() >= excessTimeInMicroseconds )
				break;
		}
	}
	else
	{
		m_AccumulatedSleepMicroseconds -= excessTimeInMicroseconds;
		excessTimeInMicroseconds = 0;
	}
}


// ------------------------------------------------------------------------------------------------------------------------ //
// CMasterMulticastThread.
// ------------------------------------------------------------------------------------------------------------------------ //

class CMasterMulticastThread
{
public:

					CMasterMulticastThread();
					~CMasterMulticastThread();

	// This creates the socket and starts the thread (initially in an idle state since it doesn't
	// know of any files anyone wants).
	bool			Init( IBaseFileSystem *pPassThru, unsigned short localPort, const CIPAddr *pAddr, int maxFileSystemMemoryUsage );
	void			Term();

	// Returns -1 if there is an error.
	int				FindOrAddFile( const char *pFilename, const char *pPathID );	
	const CUtlVector<char>& GetFileData( int iFile ) const;

	// When a client requests a files, this is called to tell the thread to start
	// adding chunks from the specified file into the queue it's multicasting.
	//
	// Returns -1 if the file isn't there. Otherwise, it returns the file ID
	// that will be sent along with the file's chunks in the multicast packets.
	int				AddFileRequest( const char *pFilename, const char *pPathID, int clientID, bool *bZeroLength );
	
	// As each client receives multicasted chunks, they ack them so the master can
	// stop transmitting any chunks it knows nobody wants.
	void			OnChunkReceived( int fileID, int clientID, int iChunk );
	void			OnFileReceived( int fileID, int clientID );

	// Call this if a client disconnects so it can stop sending anything this client wants.
	void			OnClientDisconnect( int clientID, bool bGrabCriticalSection=true );

	void CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength );

private:

	class CChunkInfo
	{
	public:
		unsigned short	m_iChunk;
		unsigned short	m_RefCount;				// How many clients want this chunk.
		unsigned short	m_iActiveChunksIndex;	// Index into m_ActiveChunks.
	};


	// This stores a client's reference to a file so it knows which pieces of the file the client needs.
	class CClientFileInfo
	{
	public:
		bool NeedsChunk( int i ) const { return (m_ChunksToSend[i>>3] & (1 << (i&7))) != 0; }	
	
	public:
		int							m_ClientID;
		CUtlVector<unsigned char>	m_ChunksToSend;	// One bit for each chunk that this client still wants.
		int m_nChunksLeft;

		// TCP transmission only.
		int m_TCP_LastChunkAcked;
		int m_TCP_LastChunkSent;
		
		float m_flTransmitStartTime;
		
		float m_flLastAckTime;		// Last time we heard an ack back from this client about this file.
									// If this goes for too long, then we assume that the client is
									// in a screwed state, and we stop sending the file to him.
		int m_nTimesFileCycled;		// How many times has the master multicast thread cycled over this file?
									// We won't kick the client until we've cycled over the file a few times
									// after the client asked for it.
	};


	class CMulticastFile
	{
	public:
		~CMulticastFile()
		{
			m_Clients.PurgeAndDeleteElements();
		}

		const char* GetFilename() { return m_Filename.Base(); }
		const char* GetPathID() { return m_PathID.Base(); }


	public:
		int m_nCycles; // How many times has the multicast thread visited this file?

		// This is sent along with every packet. If a client gets a chunk and doesn't have that file's
		// info, the client will receive that file too.
		CUtlVector<char> m_Filename;
		CUtlVector<char> m_PathID;

		CMulticastFileInfo m_Info;

		// This is stored so the app can read out the uncompressed data.
		CUtlVector<char>				m_UncompressedData;

		// zlib-compressed file data
		CUtlVector<char>				m_Data;	

		// This gets set to false if we run over our memory limit and start caching file data out.
		// Then it'll reload the data if a client requests the file.
		bool m_bDataLoaded;

		// m_Chunks holds the chunks by index.
		// m_ActiveChunks holds them sorted by whether they're active or not.
		// 
		// Each chunk has a refcount. While the refcount is > 0, the chunk is in the first
		// half of m_ActiveChunks. When the refcount gets to 0, the chunk is moved to the end of 
		// m_ActiveChunks. That way, we can iterate through the chunks that need to be sent and 
		// stop iterating the first time we hit one with a refcount of 0.
		CUtlVector<CChunkInfo>			m_Chunks;
		CUtlLinkedList<CChunkInfo*,int>	m_ActiveChunks;

		// This tells which clients want pieces of this file.
		CUtlLinkedList<CClientFileInfo*,int>	m_Clients;
	};


private:

	static DWORD WINAPI StaticMulticastThread( LPVOID pParameter );
	DWORD MulticastThread();

	bool CheckClientTimeouts();
	bool Thread_SendFileChunk_Multicast( int *pnBytesSent );
	void Thread_SeekToNextActiveChunk();

	// In TCP mode, we send new chunks as they are acked.
	void TCP_SendNextChunk( CMulticastFile *pFile, CClientFileInfo *pClient );

	void EnsureMemoryLimit( CMulticastFile *pIgnore );

	// Called after pFile->m_UncompressedData has been setup. This compresses the data, prepares the header,
	// copies the filename, and adds it into the queue for the multicast thread.
	int FinishFileSetup( CMulticastFile *pFile, const char *pFilename, const char *pPathID, bool bFileAlreadyExisted );

	void IncrementChunkRefCount( CMasterMulticastThread::CMulticastFile *pFile, int iChunk );
	void DecrementChunkRefCount( int iFile, int iChunk );
	
	int FindFile( const char *pFilename, const char *pPathID );

	bool FindWarningSuppression( const char *pFilename );
	void AddWarningSuppression( const char *pFilename );

private:
	
	CUtlLinkedList<CMulticastFile*,int>		m_Files;
	
	unsigned long m_nCurMemoryUsage;	// Total of all the file data we have loaded.
	unsigned long m_nMaxMemoryUsage;	// 0 means that there is no limit.

	// This tracks how many chunks we have that want to be sent.
	int m_nTotalActiveChunks;

	SOCKET m_Socket;
	sockaddr_in m_MulticastAddr;

	HANDLE m_hMainThread;
	IBaseFileSystem *m_pPassThru;
	
	HANDLE m_hThread;
	CRITICAL_SECTION m_CS;

	// Events used to communicate with our thread.
	HANDLE m_hTermEvent;

	// The thread walks through this as it spews chunks of data.
	volatile int m_iCurFile;			// Index into m_Files.
	volatile int m_iCurActiveChunk;		// Current index into CMulticastFile::m_ActiveChunks.

	CUtlLinkedList<char*,int> m_WarningSuppressions;
};


CMasterMulticastThread::CMasterMulticastThread()
{
	m_hThread = m_hMainThread = NULL;
	m_Socket = INVALID_SOCKET;
	m_nTotalActiveChunks = 0;
	m_iCurFile = m_iCurActiveChunk = -1;
	m_pPassThru = NULL;
	
	m_hTermEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	InitializeCriticalSection( &m_CS );
	m_nCurMemoryUsage = m_nMaxMemoryUsage = 0;
}


CMasterMulticastThread::~CMasterMulticastThread()
{
	Term();
	
	CloseHandle( m_hTermEvent );

	DeleteCriticalSection( &m_CS );
}


bool CMasterMulticastThread::Init( IBaseFileSystem *pPassThru, unsigned short localPort, const CIPAddr *pAddr, int maxMemoryUsage )
{
	Term();

	m_nMaxMemoryUsage = maxMemoryUsage;
	Assert( m_nCurMemoryUsage == 0 );
	m_nCurMemoryUsage = 0;

	if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_TCP )
	{
		// No need for an extra socket in this mode.
		m_Socket = INVALID_SOCKET;
	}
	else
	{
		// First, create our socket.
		m_Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
		if ( m_Socket == INVALID_SOCKET )
		{
			Warning( "CMasterMulticastThread::Init - socket() failed\n" );
			return false;
		}

		// Bind to INADDR_ANY.
		CIPAddr localAddr( 0, 0, 0, 0, localPort );
		
		sockaddr_in addr;
		IPAddrToSockAddr( &localAddr, &addr );

		int status = bind( m_Socket, (sockaddr*)&addr, sizeof(addr) );
		if ( status != 0 )
		{
			Term();
			Warning( "CMasterMulticastThread::Init - bind( %d.%d.%d.%d:%d ) failed\n", EXPAND_ADDR( *pAddr ) );
			return false;
		}
		
		if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_BROADCAST )
		{
			// Set up for broadcast
			BOOL bBroadcast = TRUE;
			if ( setsockopt( m_Socket, SOL_SOCKET, SO_BROADCAST, (char*)&bBroadcast, bBroadcast ) == SOCKET_ERROR )
			{
				Term();
				Warning( "CMasterMulticastThread::Init - setsockopt() failed to set broadcast mode\n" );
				return false;
			}
		}

		// Remember the address we want to send to.
		IPAddrToSockAddr( pAddr, &m_MulticastAddr );

		// Now create our thread.
		DWORD dwThreadID = 0;
		m_hThread = CreateThread( NULL, 0, &CMasterMulticastThread::StaticMulticastThread, this, 0, &dwThreadID );
		if ( !m_hThread )
		{
			Term();
			Warning( "CMasterMulticastThread::Init - CreateThread failed\n" );
			return false;
		}

		SetThreadPriority( m_hThread, THREAD_PRIORITY_LOWEST );
	}

	// For debug mode to verify that we don't try to open files while in another thread.
	m_hMainThread = GetCurrentThread();

	m_pPassThru = pPassThru;
	return true;
}


void CMasterMulticastThread::Term()
{
	// Stop the thread if it is running.
	if ( m_hThread )
	{
		SetEvent( m_hTermEvent );
		WaitForSingleObject( m_hThread, INFINITE );
		CloseHandle( m_hThread );

		m_hThread = NULL;
	}

	// Close the socket.
	if ( m_Socket != INVALID_SOCKET )
	{
		closesocket( m_Socket );
		m_Socket = INVALID_SOCKET;
	}

	// Free up other data.
	m_Files.PurgeAndDeleteElements();
	m_nCurMemoryUsage = m_nMaxMemoryUsage = 0;
}


void CMasterMulticastThread::TCP_SendNextChunk( CMulticastFile *pFile, CClientFileInfo *pClient )
{
	// No more chunks to send?
	if ( (pClient->m_TCP_LastChunkSent+1) >= pFile->m_Info.m_nChunks )
		return;
	
	// Figure out what data we'd be sending.
	int iChunkToSend = pClient->m_TCP_LastChunkSent + 1;
	int iStartByte = iChunkToSend * TCP_CHUNK_PAYLOAD_SIZE;
	int iEndByte = min( iStartByte + TCP_CHUNK_PAYLOAD_SIZE, pFile->m_Data.Count() );
	
	// If the start point is past the end, then we're done sending the file to this client.
	if ( iStartByte >= pFile->m_Data.Count() )
		return;

	// Record that we sent this data.
	pClient->m_TCP_LastChunkSent = iChunkToSend;

	// Assemble the packet.
	unsigned char cPacket[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_CHUNK };

	const void *chunks[5] =
	{
		cPacket,
		&pFile->m_Info,
		&iChunkToSend,
		pFile->GetFilename(),
		&pFile->m_Data[iStartByte]
	};

	int chunkLengths[5] =
	{
		sizeof( cPacket ),
		sizeof( pFile->m_Info ),
		sizeof( m_iCurActiveChunk ),
		strlen( pFile->GetFilename() ) + 1,
		iEndByte - iStartByte
	};
	
	VMPI_SendChunks( chunks, chunkLengths, 5, pClient->m_ClientID );
}


int CMasterMulticastThread::AddFileRequest( const char *pFilename, const char *pPathID, int clientID, bool *bZeroLength )
{
	// Firstly, do we already have this file?
	int iFile = FindOrAddFile( pFilename, pPathID );
	if ( iFile == -1 )
		return -1;

	CMulticastFile *pFile = m_Files[iFile];

	// Now that we have a file setup, merge in this client's info.
	EnterCriticalSection( &m_CS );

		CClientFileInfo *pClient = new CClientFileInfo;
		pClient->m_TCP_LastChunkAcked = -1;
		pClient->m_TCP_LastChunkSent = -1;
		pClient->m_ClientID = clientID;
		pClient->m_flLastAckTime = Plat_FloatTime();
		pClient->m_flTransmitStartTime = pClient->m_flLastAckTime;
		pClient->m_nTimesFileCycled = 0;
		pClient->m_nChunksLeft = pFile->m_Info.m_nChunks;
		pClient->m_ChunksToSend.SetSize( PAD_NUMBER( pFile->m_Info.m_nChunks, 8 ) / 8 );
		memset( pClient->m_ChunksToSend.Base(), 0xFF, pClient->m_ChunksToSend.Count() );
		pFile->m_Clients.AddToTail( pClient );

		for ( int i=0; i < pFile->m_Chunks.Count(); i++ )
		{
			IncrementChunkRefCount( pFile, i );
		}

		// In TCP mode, let's get the sliding window started..
		if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_TCP )
		{
			for ( int iDepth=0; iDepth < TCP_CHUNK_QUEUE_LEN; iDepth++ )
				TCP_SendNextChunk( pFile, pClient );
		}
	
	LeaveCriticalSection( &m_CS );

	*bZeroLength = (pFile->m_Info.m_UncompressedSize == 0);

	return iFile;
}


void CMasterMulticastThread::OnChunkReceived( int fileID, int clientID, int iChunk )
{
	if ( !m_Files.IsValidIndex( fileID ) )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid file (%d) from client %d\n", fileID, clientID );
		return;
	}

	CMulticastFile *pFile = m_Files[fileID];
	CClientFileInfo *pClient = NULL;
	FOR_EACH_LL( pFile->m_Clients, iClient )
	{
		if ( pFile->m_Clients[iClient]->m_ClientID == clientID )
		{
			pClient = pFile->m_Clients[iClient];
			break;
		}
	}
	if ( !pClient )
	{
		// This will spam sometimes if a worker stops responding and we timeout on it,
		// but then it comes back alive and starts responding. So let's ignore its packets silently.
		//Warning( "CMasterMulticastThread::OnChunkReceived: invalid client ID (%d) for file %s\n", clientID, pFile->GetFilename() );
		return;
	}
	
	if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_TCP )
	{
		// Send the next chunk, if there is one.
		EnterCriticalSection( &m_CS );
		TCP_SendNextChunk( pFile, pClient );
		LeaveCriticalSection( &m_CS );
	}
	else
	{
		if ( !pFile->m_Chunks.IsValidIndex( iChunk ) )
		{
			Warning( "CMasterMulticastThread::OnChunkReceived: invalid chunk index (%d) for file %s\n", iChunk, pFile->GetFilename() );
			return;
		}

		// Mark that this client doesn't need this chunk anymore.
		pClient->m_ChunksToSend[iChunk >> 3] &= ~(1 << (iChunk & 7));
		pClient->m_nChunksLeft--;

		pClient->m_flLastAckTime = Plat_FloatTime();
		if ( pClient->m_nChunksLeft == 0 && g_iVMPIVerboseLevel >= 2 )
			Warning( "Client %d got file %s\n", clientID, pFile->GetFilename() );
		
		EnterCriticalSection( &m_CS );
			DecrementChunkRefCount( fileID, iChunk );
		LeaveCriticalSection( &m_CS );
	}
}


void CMasterMulticastThread::OnFileReceived( int fileID, int clientID )
{
	if ( !m_Files.IsValidIndex( fileID ) )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid file (%d) from client %d\n", fileID, clientID );
		return;
	}

	CMulticastFile *pFile = m_Files[fileID];
	for ( int i=0; i < pFile->m_Info.m_nChunks; i++ )
		OnChunkReceived( fileID, clientID, i );
}


void CMasterMulticastThread::OnClientDisconnect( int clientID, bool bGrabCriticalSection )
{
	if ( bGrabCriticalSection )
		EnterCriticalSection( &m_CS );

	// Remove all references from this client.
	FOR_EACH_LL( m_Files, iFile )
	{
		CMulticastFile *pFile = m_Files[iFile];

		FOR_EACH_LL( pFile->m_Clients, iClient )
		{
			CClientFileInfo *pClient = pFile->m_Clients[iClient];
			
			if ( pClient->m_ClientID != clientID )
				continue;

			// Ok, this is our man. Decrement the refcount of any chunks this client wanted.
			for ( int iChunk=0; iChunk < pFile->m_Info.m_nChunks; iChunk++ )
			{
				if ( pClient->NeedsChunk( iChunk ) )
				{
					DecrementChunkRefCount( iFile, iChunk );
				}
			}

			delete pClient;
			pFile->m_Clients.Remove( iClient );

			break;
		}
	}

	if ( bGrabCriticalSection )
		LeaveCriticalSection( &m_CS );
}


void CMasterMulticastThread::CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength )
{
	const char *pPathID = VMPI_VIRTUAL_FILES_PATH_ID;

	int iFile = FindFile( pFilename, pPathID );
	if ( iFile != -1 )
		Error( "CMasterMulticastThread::CreateVirtualFile( %s ) - file already exists!", pFilename );

	CMulticastFile *pFile = new CMulticastFile;
	pFile->m_UncompressedData.CopyArray( (const char*)pData, fileLength );

	FinishFileSetup( pFile, pFilename, pPathID, false );
}


DWORD WINAPI CMasterMulticastThread::StaticMulticastThread( LPVOID pParameter )
{
	return ((CMasterMulticastThread*)pParameter)->MulticastThread();
}


bool CMasterMulticastThread::CheckClientTimeouts()
{
	bool bRet = false;
	CMulticastFile *pFile = m_Files[m_iCurFile];

	float flCurTime = Plat_FloatTime();

	int iNext;
	for( int iCur=pFile->m_Clients.Head(); iCur != pFile->m_Clients.InvalidIndex(); iCur=iNext )
	{
		iNext = pFile->m_Clients.Next( iCur );
		CClientFileInfo *pInfo = pFile->m_Clients[iCur];

		// If the client has already fully received this file, don't bother timing out on it.
		if ( pInfo->m_nChunksLeft == 0 )
			continue;

		++pInfo->m_nTimesFileCycled;
		if ( pInfo->m_nTimesFileCycled >= MIN_FILE_CYCLE_COUNT && (flCurTime - pInfo->m_flLastAckTime) > CLIENT_FILE_ACK_TIMEOUT )
		{
			// For debug output, get the most recent time we heard any ack from this client at all.
			float flMostRecentTime = pInfo->m_flLastAckTime;
			FOR_EACH_LL( m_Files, iTestFile )
			{
				CMulticastFile *pTestFile = m_Files[iTestFile];
				FOR_EACH_LL( pTestFile->m_Clients, iTestClient )
				{
					if ( pTestFile->m_Clients[iTestClient]->m_ClientID == pInfo->m_ClientID )
					{
						flMostRecentTime = max( flMostRecentTime, pTestFile->m_Clients[iTestClient]->m_flLastAckTime );
					}
				}
			}

			Warning( "\nClient %s timed out on file %s (latest: %.2f / cur: %.2f).\n", 
				VMPI_GetMachineName( pInfo->m_ClientID ), pFile->GetFilename(), flMostRecentTime, flCurTime );
			OnClientDisconnect( pInfo->m_ClientID, false );
			bRet = true; // yes, we booted a client.
		}
	}
	
	return bRet;
}

inline bool CMasterMulticastThread::Thread_SendFileChunk_Multicast( int *pnBytesSent )
{
	// Send the next chunk (file, size, time, chunk data).
	CMulticastFile *pFile = m_Files[m_iCurFile];
	
	int iStartByte = m_iCurActiveChunk * MULTICAST_CHUNK_PAYLOAD_SIZE;
	int iEndByte = min( iStartByte + MULTICAST_CHUNK_PAYLOAD_SIZE, pFile->m_Data.Count() );

	WSABUF bufs[4];
	bufs[0].buf = (char*)&pFile->m_Info;
	bufs[0].len = sizeof( pFile->m_Info );

	bufs[1].buf = (char*)&m_iCurActiveChunk;
	bufs[1].len = sizeof( m_iCurActiveChunk );

	bufs[2].buf = (char*)pFile->GetFilename();
	bufs[2].len = strlen( pFile->GetFilename() ) + 1;

	bufs[3].buf = &pFile->m_Data[iStartByte];
	bufs[3].len = iEndByte - iStartByte;

	DWORD nBytesSent = 0;
	DWORD nWantedBytes = ( bufs[0].len + bufs[1].len + bufs[2].len + bufs[3].len );
	bool bSuccess;

	if ( m_MulticastAddr.sin_addr.S_un.S_un_b.s_b1 == 127 && 
			m_MulticastAddr.sin_addr.S_un.S_un_b.s_b2 == 0 && 
			m_MulticastAddr.sin_addr.S_un.S_un_b.s_b3 == 0 && 
			m_MulticastAddr.sin_addr.S_un.S_un_b.s_b4 == 1 )
	{
		// For some mysterious reason, WSASendTo only sends the first buffer
		// if we're sending to 127.0.0.1 (ie: in local mode).
		char allData[1024*8];
		if ( nWantedBytes > sizeof( allData ) )
			Error( "nWantedBytes > sizeof( allData )" );

		memcpy( &allData[0], bufs[0].buf, bufs[0].len );
		memcpy( &allData[bufs[0].len], bufs[1].buf, bufs[1].len );
		memcpy( &allData[bufs[0].len+bufs[1].len], bufs[2].buf, bufs[2].len );
		memcpy( &allData[bufs[0].len+bufs[1].len+bufs[2].len], bufs[3].buf, bufs[3].len );
		int ret = sendto( m_Socket, allData, nWantedBytes, 0, (sockaddr*)&m_MulticastAddr, sizeof( m_MulticastAddr ) );
		bSuccess = (ret == (int)nWantedBytes);
	}
	else
	{
		WSASendTo( 
			m_Socket, 
			bufs, 
			ARRAYSIZE( bufs ), 
			&nBytesSent, 
			0, 
			(sockaddr*)&m_MulticastAddr, 
			sizeof( m_MulticastAddr ),
			NULL,
			NULL );
		bSuccess = (nBytesSent == nWantedBytes);
	}

	// Handle errors.. let it get a few errors, then quit.
	if ( bSuccess )
	{
		*pnBytesSent = (int)nBytesSent;
	}
	else
	{
		static int nWarnings = 0;
		++nWarnings;
		if ( nWarnings < 10 )
		{
			Warning( "\nMulticastThread: WSASendTo with %d bytes sent %d bytes.\n", nWantedBytes, nBytesSent );

			char *lpMsgBuf;
			if ( FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(char*)&lpMsgBuf,
				0,
				NULL 
			) )
			{
				Warning( "%s", lpMsgBuf );
				LocalFree( lpMsgBuf );	
			}
		}
		else if ( nWarnings == 10 )
		{
			Warning( "\nThis machine's ability to multicast may be broken. Please reboot and try again.\n" );
		}
	}

	return bSuccess;
}


void CMasterMulticastThread::Thread_SeekToNextActiveChunk()
{
	// Make sure we're on a valid chunk.
	if ( m_iCurFile == -1 )
	{
		Assert( m_Files.Count() > 0 );
		m_iCurFile = m_Files.Head();
		m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Head();
	}

	while ( 1 )
	{
		if ( m_iCurActiveChunk == m_Files[m_iCurFile]->m_ActiveChunks.InvalidIndex() ||
			m_Files[m_iCurFile]->m_ActiveChunks[m_iCurActiveChunk]->m_RefCount == 0 )
		{
			// Now check for client timeouts.
			// (This is kicking clients unjustly for some reason.. need to debug).
			if ( CheckClientTimeouts() && m_nTotalActiveChunks == 0 )
				break;

			// Finished with that file. Send the next one.
			m_iCurFile = m_Files.Next( m_iCurFile );
			if ( m_iCurFile == m_Files.InvalidIndex() )
				m_iCurFile = m_Files.Head();

			m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Head();
		}

		if ( m_iCurActiveChunk != m_Files[m_iCurFile]->m_ActiveChunks.InvalidIndex() )
		{
			// Only break if we're on an active chunk.
			if ( m_Files[m_iCurFile]->m_ActiveChunks[m_iCurActiveChunk]->m_RefCount != 0 )
			{
				break;
			}

			m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Next( m_iCurActiveChunk );
		}
	}
}


DWORD CMasterMulticastThread::MulticastThread()
{
	CTransmitRateMgr transmitRateMgr;
	CRateLimiter rateLimiter;

	
	DWORD msToWait = 0; // Only temporarily used if we don't have any data to send.

	while ( WaitForSingleObject( m_hTermEvent, msToWait ) != WAIT_OBJECT_0 )
	{
		rateLimiter.GiveUpTimeSlice();
		msToWait = 0;

		EnterCriticalSection( &m_CS );
			
			transmitRateMgr.ReadPackets();
		
			// If we have nothing to send then kick back for a while.
			if ( m_nTotalActiveChunks == 0 )
			{
				LeaveCriticalSection( &m_CS );
				msToWait = 50;
				continue;
			}

			// Ok, now we're active, so send out our presence to other CTransmitRateMgrs on the network.
			transmitRateMgr.BroadcastPresence();

			
			// We're going to time how long this chunk took to send.
			CFastTimer timer;
			timer.Start();

			Thread_SeekToNextActiveChunk();

			// We have to do this check a second time here because the CheckClientTimeouts() call may have
			// booted our last client. If we don't check it here, we might be transmitting
			// something we don't want to transmit. Also, if we don't break out of the loop above,
			// it can prevent the process from ever exiting because it'll never exit that while() loop.
			if ( m_nTotalActiveChunks == 0 )
			{
				LeaveCriticalSection( &m_CS );
				msToWait = 50;
				continue;
			}

			int nBytesSent = 0;

			bool bSuccess;
			bSuccess = Thread_SendFileChunk_Multicast( &nBytesSent );

			g_nMulticastBytesSent += (int)nBytesSent;

			// Move to the next chunk.
			m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Next( m_iCurActiveChunk );

		LeaveCriticalSection( &m_CS );


		// Measure how long it took to send this.
		timer.End();
		unsigned long timeTaken = timer.GetDuration().GetMicroseconds();

		
		// Measure how long it should have taken.
		unsigned long estimatedPacketHeaderSize = 32;
		unsigned long optimalTimeTaken = (unsigned long)( transmitRateMgr.GetMicrosecondsPerByte() * (nBytesSent + estimatedPacketHeaderSize) );

		
		// If we went faster than we should have, then wait for the difference in time.
		if ( timeTaken < optimalTimeTaken )
		{
			rateLimiter.NoteExcessTimeTaken( optimalTimeTaken - timeTaken );
		}
	}

	return 0;
}


void CMasterMulticastThread::IncrementChunkRefCount( CMasterMulticastThread::CMulticastFile *pFile, int iChunk )
{
	CChunkInfo *pChunk = &pFile->m_Chunks[iChunk];

	if ( pChunk->m_RefCount == 0 )
	{
		++m_nTotalActiveChunks;
		
		// Move the chunk to the head of the list since it is now active.
		pFile->m_ActiveChunks.Remove( pChunk->m_iActiveChunksIndex );
		pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToHead( pChunk );
	}

	pChunk->m_RefCount++;
}


void CMasterMulticastThread::DecrementChunkRefCount( int iFile, int iChunk )
{
	CMulticastFile *pFile = m_Files[iFile];
	CChunkInfo *pChunk = &pFile->m_Chunks[iChunk];

	if ( pChunk->m_RefCount == 0 )
	{
		Error( "CMasterMulticastThread::DecrementChunkRefCount - refcount already zero!\n" );
	}

	pChunk->m_RefCount--;
	if ( pChunk->m_RefCount == 0 )
	{
		--m_nTotalActiveChunks;
		
		// If this is the current chunk the thread is reading on, seek up to the next chunk so
		// the thread doesn't spin off into the next file and skip its current file's contents.
		if ( iFile == m_iCurFile && pChunk->m_iActiveChunksIndex == m_iCurActiveChunk )
		{
			m_iCurActiveChunk = pFile->m_ActiveChunks.Next( pChunk->m_iActiveChunksIndex );
		}

		// Move the chunk to the end of the list since it is now inactive.
		pFile->m_ActiveChunks.Remove( pChunk->m_iActiveChunksIndex );
		pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToTail( pChunk );
	}
}


int CMasterMulticastThread::FindFile( const char *pName, const char *pPathID )
{
	FOR_EACH_LL( m_Files, i )
	{
		CMulticastFile *pFile = m_Files[i];
		if ( stricmp( pFile->GetFilename(), pName ) == 0 && stricmp( pFile->GetPathID(), pPathID ) == 0 )
			return i;
	}
	return -1;
}


bool CMasterMulticastThread::FindWarningSuppression( const char *pFilename )
{
	FOR_EACH_LL( m_WarningSuppressions, i )
	{
		if ( Q_stricmp( m_WarningSuppressions[i], pFilename ) == 0 )
			return true;
	}
	return false;
}


void CMasterMulticastThread::AddWarningSuppression( const char *pFilename )
{
	char *pBlah = new char[ strlen( pFilename ) + 1 ];
	strcpy( pBlah, pFilename );
	m_WarningSuppressions.AddToTail( pBlah );
}


int CMasterMulticastThread::FindOrAddFile( const char *pFilename, const char *pPathID )
{
	CMulticastFile *pFile = NULL;
	bool bFileAlreadyExisted = false;

	// See if we've already opened this file.
	int iFile = FindFile( pFilename, pPathID );
	if ( iFile != -1 )
	{
		pFile = m_Files[iFile];
		if ( pFile->m_bDataLoaded )
		{
			return iFile;
		}
		else
		{
			// Ok, we have the file entry, but its data has been freed, so we need to reload it.
			EnterCriticalSection( &m_CS );
			bFileAlreadyExisted = true;
		}
	}

	// Can't open a file outside our main thread, because we have to talk to the filesystem
	// and the filesystem doesn't support that.
	Assert( GetCurrentThread() == m_hMainThread );

	// When the worker originally asked for the path ID, they could pass NULL and it would come through as "".
	// Now set it back to null for the filesystem we're passing the call to.
	FileHandle_t fp = m_pPassThru->Open( pFilename, "rb", pPathID[0] == 0 ? NULL : pPathID );
	if ( !fp )
	{
		if ( bFileAlreadyExisted )
			LeaveCriticalSection( &m_CS );

		return -1;
	}

	if ( !bFileAlreadyExisted )
		pFile = new CMulticastFile;

	pFile->m_UncompressedData.SetSize( m_pPassThru->Size( fp ) );
	m_pPassThru->Read( pFile->m_UncompressedData.Base(), pFile->m_UncompressedData.Count(), fp );
	m_pPassThru->Close( fp );

	int iRet = FinishFileSetup( pFile, pFilename, pPathID, bFileAlreadyExisted );
	if ( bFileAlreadyExisted )
		LeaveCriticalSection( &m_CS );

	return iRet;
}


int CMasterMulticastThread::FinishFileSetup( CMulticastFile *pFile, const char *pFilename, const char *pPathID, bool bFileAlreadyExisted )
{
	// Compress the file's contents.
	if ( !ZLibCompress( pFile->m_UncompressedData.Base(), pFile->m_UncompressedData.Count(), pFile->m_Data ) )
	{
		delete pFile;
		return -1;
	}

	pFile->m_bDataLoaded = true;
	int chunkSize = VMPI_GetChunkPayloadSize();

	// Get this file in the queue.
	if ( !bFileAlreadyExisted )
	{
		pFile->m_Filename.SetSize( strlen( pFilename ) + 1 );
		strcpy( pFile->m_Filename.Base(), pFilename );

		pFile->m_PathID.SetSize( strlen( pPathID ) + 1 );
		strcpy( pFile->m_PathID.Base(), pPathID );

		pFile->m_nCycles = 0;

		pFile->m_Info.m_CompressedSize = pFile->m_Data.Count();
		pFile->m_Info.m_UncompressedSize = pFile->m_UncompressedData.Count();

		pFile->m_Info.m_nChunks = PAD_NUMBER( pFile->m_Info.m_CompressedSize, chunkSize ) / chunkSize;

		// Initialize the chunks.
		pFile->m_Chunks.SetSize( pFile->m_Info.m_nChunks );
		for ( int i=0; i < pFile->m_Chunks.Count(); i++ )
		{
			CChunkInfo *pChunk = &pFile->m_Chunks[i];

			pChunk->m_iChunk = (unsigned short)i;
			pChunk->m_RefCount = 0;
			pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToTail( pChunk );
		}

		EnterCriticalSection( &m_CS );
	}

	// Boot some other file out of memory if we're out of space.		
	m_nCurMemoryUsage += ( pFile->m_Info.m_CompressedSize + pFile->m_Info.m_UncompressedSize );
	EnsureMemoryLimit( pFile );

	if ( !bFileAlreadyExisted )
	{
		pFile->m_Info.m_FileID = m_Files.AddToTail( pFile );
		LeaveCriticalSection( &m_CS );
	}

	return pFile->m_Info.m_FileID;
}


void CMasterMulticastThread::EnsureMemoryLimit( CMulticastFile *pIgnore )
{
	if ( m_nMaxMemoryUsage != 0 && m_nCurMemoryUsage > m_nMaxMemoryUsage )
	{
		// Free all the files that we can.
		FOR_EACH_LL( m_Files, iFile )
		{
			CMulticastFile *pFile = m_Files[iFile];
			if ( pFile == pIgnore || !pFile->m_bDataLoaded )
				continue;

			if ( pFile->m_ActiveChunks.Count() == 0 )
			{
				m_nCurMemoryUsage -= ( pFile->m_Info.m_CompressedSize + pFile->m_Info.m_UncompressedSize );

				pFile->m_Data.Purge();
				pFile->m_UncompressedData.Purge();
				pFile->m_bDataLoaded = false;
			}
		}
	}
}


const CUtlVector<char>& CMasterMulticastThread::GetFileData( int iFile ) const
{
	return m_Files[iFile]->m_UncompressedData;
}


// ------------------------------------------------------------------------------------------------------------------------ //
// CMasterVMPIFileSystem implementation.
// ------------------------------------------------------------------------------------------------------------------------ //

class CMasterVMPIFileSystem : public CBaseVMPIFileSystem
{
public:
	CMasterVMPIFileSystem();
	virtual ~CMasterVMPIFileSystem();
	
	bool Init( int maxMemoryUsage, IFileSystem *pPassThru );
	virtual void Term();

	virtual FileHandle_t Open( const char *pFilename, const char *pOptions, const char *pathID );
	virtual FileHandle_t OpenEx( const char *pFileName, const char *pOptions, unsigned flags = 0, const char *pathID = 0, char **ppszResolvedFilename = NULL ) { return Open( pFileName, pOptions, pathID ); } //pass thru to Open
	virtual bool HandleFileSystemPacket( MessageBuffer *pBuf, int iSource, int iPacketID );

	virtual void CreateVirtualFile( const char *pFilename, const void *pData, int fileLength );

	virtual CSysModule 		*LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly );
	virtual void			UnloadModule( CSysModule *pModule );


private:

	static void OnClientDisconnect( int procID, const char *pReason );


private:
	CMasterMulticastThread m_MasterThread;
	IFileSystem *m_pMasterVMPIFileSystemPassThru;

	static CMasterVMPIFileSystem *s_pMasterVMPIFileSystem;
};


CMasterVMPIFileSystem *CMasterVMPIFileSystem::s_pMasterVMPIFileSystem = NULL;


CBaseVMPIFileSystem* CreateMasterVMPIFileSystem( int maxMemoryUsage, IFileSystem *pPassThru )
{
	CMasterVMPIFileSystem *pRet = new CMasterVMPIFileSystem;
	g_pBaseVMPIFileSystem = pRet;
	if ( pRet->Init( maxMemoryUsage, pPassThru ) )
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


CMasterVMPIFileSystem::CMasterVMPIFileSystem()
{
	Assert( !s_pMasterVMPIFileSystem );
	s_pMasterVMPIFileSystem = this;
}


CMasterVMPIFileSystem::~CMasterVMPIFileSystem()
{
	Assert( s_pMasterVMPIFileSystem == this );
	s_pMasterVMPIFileSystem = NULL;
}


bool CMasterVMPIFileSystem::Init( int maxMemoryUsage, IFileSystem *pPassThru )
{
	// Only init the BASE filesystem passthru. Leave the IFileSystem passthru using NULL so it'll crash 
	// immediately if they try to use a function we don't support.
	InitPassThru( pPassThru, false );
	m_pMasterVMPIFileSystemPassThru = pPassThru;

	// Pick a random IP in the multicast range (224.0.0.2 to 239.255.255.255);
	CCycleCount cnt;
	cnt.Sample();
	RandomSeed( (int)cnt.GetMicroseconds() );

	int localPort = 23412; // This can be anything.

	unsigned short port = RandomInt( 22000, 25000 );
	if ( VMPI_GetRunMode() == VMPI_RUN_NETWORKED )
	{
		if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_MULTICAST )
		{
			m_MulticastIP.port = port;
			m_MulticastIP.ip[0] = (unsigned char)RandomInt( 225, 238 );
			m_MulticastIP.ip[1] = (unsigned char)RandomInt( 0, 255 );
			m_MulticastIP.ip[2] = (unsigned char)RandomInt( 0, 255 );
			m_MulticastIP.ip[3] = (unsigned char)RandomInt( 3, 255 );
		}
		else if ( VMPI_GetFileSystemMode() == VMPI_FILESYSTEM_BROADCAST )
		{
			m_MulticastIP.Init( 0xFF, 0xFF, 0xFF, 0xFF, port );
		}
	}
	else
	{
		// Doesn't matter.. we don't use the multicast IP in TCP mode.
		m_MulticastIP.Init( 0, 0, 0, 0, 0 );
	}

	if ( !m_MasterThread.Init( pPassThru, localPort, &m_MulticastIP, maxMemoryUsage ) )
		return false;

	// Send out the multicast addr to all the clients.
	SendMulticastIP( &m_MulticastIP );

	// Make sure we're notified when a client disconnects so we can unlink them from the 
	// multicast thread's structures.
	VMPI_AddDisconnectHandler( &CMasterVMPIFileSystem::OnClientDisconnect );
	return true;
}


void CMasterVMPIFileSystem::Term()
{
	m_MasterThread.Term();
}


FileHandle_t CMasterVMPIFileSystem::Open( const char *pFilename, const char *pOptions, const char *pPathID )
{
	Assert( g_bUseMPI );

	if ( g_bDisableFileAccess )
		Error( "Open( %s, %s ) - file access has been disabled.", pFilename, pOptions );

	// Use a stdio file if they want to write to it.
	bool bWriteAccess = IsOpeningForWriteAccess( pOptions );
	if ( bWriteAccess )
	{
		FileHandle_t fp = m_pBaseFileSystemPassThru->Open( pFilename, pOptions, pPathID );
		if ( fp == FILESYSTEM_INVALID_HANDLE )
			return FILESYSTEM_INVALID_HANDLE;

		CVMPIFile_PassThru *pFile = new CVMPIFile_PassThru;
		pFile->Init( m_pBaseFileSystemPassThru, fp );
		return (FileHandle_t)pFile;
	}

	// Internally, we require path IDs to be non-null. We'll convert it back to null whenever we make filesystem calls though.
	if ( !pPathID )
		pPathID = "";

	// Have our multicast thread load all the data so it's there when workers want it.
	int iFile = m_MasterThread.FindOrAddFile( pFilename, pPathID );
	if ( iFile == -1 )
		return FILESYSTEM_INVALID_HANDLE;

	const CUtlVector<char> &data = m_MasterThread.GetFileData( iFile );

	CVMPIFile_Memory *pFile = new CVMPIFile_Memory;
	pFile->Init( data.Base(), data.Count(), strchr( pOptions, 't' ) ? 't' : 'b' );
	return (FileHandle_t)pFile;
}


void CMasterVMPIFileSystem::OnClientDisconnect( int procID, const char *pReason )
{
	s_pMasterVMPIFileSystem->m_MasterThread.OnClientDisconnect( procID );
}


void CMasterVMPIFileSystem::CreateVirtualFile( const char *pFilename, const void *pData, int fileLength )
{
	m_MasterThread.CreateVirtualFile( pFilename, pData, fileLength );
}

bool CMasterVMPIFileSystem::HandleFileSystemPacket( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	// Handle this packet.
	int subPacketID = pBuf->data[1];
	switch( subPacketID )
	{
		case VMPI_FSPACKETID_FILE_REQUEST:
		{
			int requestID = *((int*)&pBuf->data[2]);
			const char *pFilename = (const char*)&pBuf->data[6];
			const char *pPathID = (const char*)pFilename + strlen( pFilename ) + 1;
			
			if ( g_iVMPIVerboseLevel >= 2 )
				Msg( "Client %d requested '%s'\n", iSource, pFilename );

			bool bZeroLength;
			int fileID = m_MasterThread.AddFileRequest( pFilename, pPathID, iSource, &bZeroLength );
			
			// Send back the file ID.
			unsigned char cPacket[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_RESPONSE };
			void *pChunks[4] = { cPacket, &requestID, &fileID, &bZeroLength };
			int chunkLen[4] = { sizeof( cPacket ), sizeof( requestID ), sizeof( fileID ), sizeof( bZeroLength ) };

			VMPI_SendChunks( pChunks, chunkLen, ARRAYSIZE( pChunks ), iSource );
		}
		return true;

		case VMPI_FSPACKETID_CHUNK_RECEIVED:
		{
			unsigned short *pFileID = (unsigned short*)&pBuf->data[2];
			unsigned short *pChunkID = pFileID+1;

			int nChunks = (pBuf->getLen() - 2) / 4;
			for ( int i=0; i < nChunks; i++ )
			{
				m_MasterThread.OnChunkReceived( *pFileID, iSource, *pChunkID );
				pFileID += 2;
				pChunkID += 2;
			}
		}
		return true;

		case VMPI_FSPACKETID_FILE_RECEIVED:
		{
			unsigned short *pFileID = (unsigned short*)&pBuf->data[2];
			m_MasterThread.OnFileReceived( *pFileID, iSource );
		}
		return true;
		
		default:
			return false;
	}
}


CSysModule* CMasterVMPIFileSystem::LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly )
{
	return m_pMasterVMPIFileSystemPassThru->LoadModule( pFileName, pPathID, bValidatedDllOnly );
}

void CMasterVMPIFileSystem::UnloadModule( CSysModule *pModule )
{
	m_pMasterVMPIFileSystemPassThru->UnloadModule( pModule );
}

