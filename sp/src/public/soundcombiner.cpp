//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "isoundcombiner.h"
#include "sentence.h"
#include "filesystem.h"
#include "tier2/riff.h"
#include "tier1/utlbuffer.h"
#include "snd_audio_source.h"
#include "snd_wave_source.h"
#include "AudioWaveOutput.h"
#include "ifaceposersound.h"
#include "vstdlib/random.h"
#include "checksum_crc.h"

#define WAVEOUTPUT_BITSPERCHANNEL		16
#define WAVEOUTPUT_FREQUENCY			44100

class CSoundCombiner : public ISoundCombiner
{
public:
	CSoundCombiner() :
		m_pWaveOutput( NULL ),
		m_pOutRIFF( NULL ),
		m_pOutIterator( NULL )
	{
		m_szOutFile[ 0 ] = 0;
	}

	virtual bool CombineSoundFiles( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info );
	virtual bool IsCombinedFileChecksumValid( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info );

private:

	struct CombinerWork
	{
		CombinerWork() :
			sentence(),
			duration( 0.0 ),
			wave( 0 ),
			mixer( 0 ),
			entry( 0 )
		{
		}
		CSentence		sentence;
		float			duration;
		CAudioSource	*wave;
		CAudioMixer		*mixer;
		CombinerEntry	*entry;
	};

	bool	InternalCombineSoundFiles( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info );
	bool	VerifyFilesExist( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info );
	bool	CreateWorkList( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info );

	bool	PerformSplicingOnWorkItems( IFileSystem *filesystem );
	void	CleanupWork();

	// .wav file utils
	int		ComputeBestNumChannels();
	void	ParseSentence( CSentence& sentence, IterateRIFF &walk );
	bool	LoadSentenceFromWavFileUsingIO( char const *wavfile, CSentence& sentence, IFileReadBinary& io );
	bool	LoadSentenceFromWavFile( char const *wavfile, CSentence& sentence );
	void	StoreValveDataChunk( CSentence& sentence );
//	bool	SaveSentenceToWavFile( char const *wavfile, CSentence& sentence );

	bool	InitSplicer( IFileSystem *filesystem, int samplerate, int numchannels, int bitspersample );
	bool	LoadSpliceAudioSources();
	bool	AppendSilence( int &currentsample, float duration );
	bool	AppendStereo16Data( short samples[ 2 ] );
	bool	AppendWaveData( int& currentsample, CAudioSource *wave, CAudioMixer *mixer );
	void	AddSentenceToCombined( float offset, CSentence& sentence );

	unsigned int CheckSumWork( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info );
	unsigned int ComputeChecksum();

	CUtlVector< CombinerWork * >	m_Work;
	CSentence						m_Combined;

	CAudioWaveOutput				*m_pWaveOutput;

	OutFileRIFF						*m_pOutRIFF;
	IterateOutputRIFF				*m_pOutIterator;

	int								m_nSampleRate;
	int								m_nNumChannels;
	int								m_nBitsPerSample;
	int								m_nBytesPerSample;
	char							m_szOutFile[ MAX_PATH ];
};

static CSoundCombiner g_SoundCombiner;
ISoundCombiner *soundcombiner = &g_SoundCombiner;

bool CSoundCombiner::CreateWorkList( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info )
{
	m_Work.RemoveAll();

	int c = info.Count();
	for ( int i = 0; i < c; ++i )
	{
		CombinerWork *workitem = new CombinerWork();

		char fullpath[ MAX_PATH ];
		Q_strncpy( fullpath, info[ i ].wavefile, sizeof( fullpath ) );
		filesystem->GetLocalPath( info[ i ].wavefile, fullpath, sizeof( fullpath ) );

		if ( !LoadSentenceFromWavFile( fullpath, workitem->sentence ) )
		{
			Warning( "CSoundCombiner::CreateWorkList couldn't load %s for work item (%d)\n",
				fullpath, i );
			return false;
		}
		
		workitem->entry = &info[ i ];

		m_Work.AddToTail( workitem );
	}

	return true;
}

void CSoundCombiner::CleanupWork()
{
	int c = m_Work.Count();
	for ( int i = 0; i < c; ++i )
	{
		CombinerWork *workitem = m_Work[ i ];
		delete workitem->mixer;
		delete workitem->wave;

		delete m_Work[ i ];
	}
	m_Work.RemoveAll();

	delete m_pOutIterator;
	m_pOutIterator = NULL;

	delete m_pOutRIFF;
	m_pOutRIFF = NULL;
}

bool CSoundCombiner::InternalCombineSoundFiles( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info )
{
	Q_strncpy( m_szOutFile, outfile, sizeof( m_szOutFile ) );
	if ( info.Count() <= 0 )
	{
		Warning( "CSoundCombiner::InternalCombineSoundFiles:  work item count is zero\n" );
		return false;
	}

	if ( !VerifyFilesExist( filesystem, info ) )
	{
		return false;
	}

	if ( !CreateWorkList( filesystem, info ) )
	{
		return false;
	}

	PerformSplicingOnWorkItems( filesystem );

	return true;
}

bool CSoundCombiner::CombineSoundFiles( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info )
{
	bool bret = InternalCombineSoundFiles( filesystem, outfile, info );
	CleanupWork();
	return bret;
}

unsigned int CSoundCombiner::ComputeChecksum()
{
	CRC32_t crc;
	CRC32_Init( &crc );
	
	int c = m_Work.Count();
	for ( int i = 0; i < c; ++i )
	{
		CombinerWork *curitem = m_Work[ i ];
		unsigned int chk = curitem->sentence.ComputeDataCheckSum();

	//	Msg( "  %i -> sentence %u, startoffset %f fn %s\n",
	//		i, chk, curitem->entry->startoffset, curitem->entry->wavefile );

		CRC32_ProcessBuffer( &crc, &chk, sizeof( unsigned long ) );
		CRC32_ProcessBuffer( &crc, &curitem->entry->startoffset, sizeof( float ) );
		CRC32_ProcessBuffer( &crc, curitem->entry->wavefile, Q_strlen( curitem->entry->wavefile ) );
	}

	CRC32_Final( &crc );
	return ( unsigned int )crc;
}

unsigned int CSoundCombiner::CheckSumWork( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info )
{
	if ( info.Count() <= 0 )
	{
		Warning( "CSoundCombiner::CheckSumWork:  work item count is zero\n" );
		return 0;
	}

	if ( !VerifyFilesExist( filesystem, info ) )
	{
		return 0;
	}

	if ( !CreateWorkList( filesystem, info ) )
	{
		return 0;
	}

	// Checkum work items
	unsigned int checksum = ComputeChecksum();

	return checksum;
}

bool CSoundCombiner::IsCombinedFileChecksumValid( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info )
{
	unsigned int computedChecksum = CheckSumWork( filesystem, info );

	char fullpath[ MAX_PATH ];
	Q_strncpy( fullpath, outfile, sizeof( fullpath ) );
	filesystem->GetLocalPath( outfile, fullpath, sizeof( fullpath ) );

	CSentence sentence;

	bool valid = false;

	if ( LoadSentenceFromWavFile( fullpath, sentence ) )
	{
		unsigned int diskFileEmbeddedChecksum = sentence.GetDataCheckSum();

		valid = computedChecksum == diskFileEmbeddedChecksum;

		if ( !valid )
		{
			Warning( "  checksum computed %u, disk %u\n",
				computedChecksum, diskFileEmbeddedChecksum );
		}
	}
	else
	{
		Warning( "CSoundCombiner::IsCombinedFileChecksumValid:  Unabled to load %s\n", fullpath );
	}

	CleanupWork();
	return valid;
}

bool CSoundCombiner::VerifyFilesExist( IFileSystem *filesystem, CUtlVector< CombinerEntry >& info )
{
	int c = info.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		CombinerEntry& entry = info[ i ];
		if ( !filesystem->FileExists( entry.wavefile ) )
		{
			Warning( "CSoundCombiner::VerifyFilesExist: missing file %s\n", entry.wavefile );
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Implements the RIFF i/o interface on stdio
//-----------------------------------------------------------------------------
class StdIOReadBinary : public IFileReadBinary
{
public:
	int open( const char *pFileName )
	{
		return (int)filesystem->Open( pFileName, "rb" );
	}

	int read( void *pOutput, int size, int file )
	{
		if ( !file )
			return 0;

		return filesystem->Read( pOutput, size, (FileHandle_t)file );
	}

	void seek( int file, int pos )
	{
		if ( !file )
			return;

		filesystem->Seek( (FileHandle_t)file, pos, FILESYSTEM_SEEK_HEAD );
	}

	unsigned int tell( int file )
	{
		if ( !file )
			return 0;

		return filesystem->Tell( (FileHandle_t)file );
	}

	unsigned int size( int file )
	{
		if ( !file )
			return 0;

		return filesystem->Size( (FileHandle_t)file );
	}

	void close( int file )
	{
		if ( !file )
			return;

		filesystem->Close( (FileHandle_t)file );
	}
};

class StdIOWriteBinary : public IFileWriteBinary
{
public:
	int create( const char *pFileName )
	{
		return (int)filesystem->Open( pFileName, "wb" );
	}

	int write( void *pData, int size, int file )
	{
		return filesystem->Write( pData, size, (FileHandle_t)file );
	}

	void close( int file )
	{
		filesystem->Close( (FileHandle_t)file );
	}

	void seek( int file, int pos )
	{
		filesystem->Seek( (FileHandle_t)file, pos, FILESYSTEM_SEEK_HEAD );
	}

	unsigned int tell( int file )
	{
		return filesystem->Tell( (FileHandle_t)file );
	}
};

static StdIOReadBinary io_in;
static StdIOWriteBinary io_out;

#define RIFF_WAVE			MAKEID('W','A','V','E')
#define WAVE_FMT			MAKEID('f','m','t',' ')
#define WAVE_DATA			MAKEID('d','a','t','a')
#define WAVE_FACT			MAKEID('f','a','c','t')
#define WAVE_CUE			MAKEID('c','u','e',' ')

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &walk - 
//-----------------------------------------------------------------------------
void CSoundCombiner::ParseSentence( CSentence& sentence, IterateRIFF &walk )
{
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	buf.EnsureCapacity( walk.ChunkSize() );
	walk.ChunkRead( buf.Base() );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, walk.ChunkSize() );

	sentence.InitFromDataChunk( buf.Base(), buf.TellPut() );
}

bool CSoundCombiner::LoadSentenceFromWavFileUsingIO( char const *wavfile, CSentence& sentence, IFileReadBinary& io )
{
	sentence.Reset();

	InFileRIFF riff( wavfile, io );

	// UNDONE: Don't use printf to handle errors
	if ( riff.RIFFName() != RIFF_WAVE )
	{
		return false;
	}

	// set up the iterator for the whole file (root RIFF is a chunk)
	IterateRIFF walk( riff, riff.RIFFSize() );

	// This chunk must be first as it contains the wave's format
	// break out when we've parsed it
	bool found = false;
	while ( walk.ChunkAvailable() && !found )
	{
		switch( walk.ChunkName() )
		{
		case WAVE_VALVEDATA:
			{
				found = true;
				CSoundCombiner::ParseSentence( sentence, walk );
			}
			break;
		}
		walk.ChunkNext();
	}

	return true;
}

bool CSoundCombiner::LoadSentenceFromWavFile( char const *wavfile, CSentence& sentence )
{
	return CSoundCombiner::LoadSentenceFromWavFileUsingIO( wavfile, sentence, io_in );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : store - 
//-----------------------------------------------------------------------------
void CSoundCombiner::StoreValveDataChunk( CSentence& sentence )
{
	// Buffer and dump data
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	sentence.SaveToBuffer( buf );

	// Copy into store
	m_pOutIterator->ChunkWriteData( buf.Base(), buf.TellPut() );
}

/*
bool CSoundCombiner::SaveSentenceToWavFile( char const *wavfile, CSentence& sentence )
{
	char tempfile[ 512 ];

	Q_StripExtension( wavfile, tempfile, sizeof( tempfile ) );
	Q_DefaultExtension( tempfile, ".tmp", sizeof( tempfile ) );

	if ( filesystem->FileExists( tempfile, NULL ) )
	{
		filesystem->RemoveFile( tempfile, NULL );
	}

	if ( !filesystem->IsFileWritable( wavfile ) )
	{
		Msg( "%s is not writable, can't save sentence data to file\n", wavfile );
		return false;
	}
	
	// Rename original wavfile to temp
	filesystem->RenameFile( wavfile, tempfile, NULL );

	// NOTE:  Put this in it's own scope so that the destructor for outfileRFF actually closes the file!!!!
	{
		// Read from Temp
		InFileRIFF riff( tempfile, io_in );
		Assert( riff.RIFFName() == RIFF_WAVE );

		// set up the iterator for the whole file (root RIFF is a chunk)
		IterateRIFF walk( riff, riff.RIFFSize() );

		// And put data back into original wavfile by name
		OutFileRIFF riffout( wavfile, io_out );

		IterateOutputRIFF store( riffout );

		bool wordtrackwritten = false;

		// Walk input chunks and copy to output
		while ( walk.ChunkAvailable() )
		{
			m_pOutIterator->ChunkStart( walk.ChunkName() );

			switch ( walk.ChunkName() )
			{
			case WAVE_VALVEDATA:
				{
					// Overwrite data
					CSoundCombiner::StoreValveDataChunk( sentence );
					wordtrackwritten = true;
				}
				break;
			default:
				m_pOutIterator->CopyChunkData( walk );
				break;
			}

			m_pOutIterator->ChunkFinish();

			walk.ChunkNext();
		}

		// If we didn't write it above, write it now
		if ( !wordtrackwritten )
		{
			m_pOutIterator->ChunkStart( WAVE_VALVEDATA );
			CSoundCombiner::StoreValveDataChunk( sentence );
			m_pOutIterator->ChunkFinish();
		}
	}

	// Remove temp file
	filesystem->RemoveFile( tempfile, NULL );

	return true;
}
*/

typedef struct channel_s
{
	int		leftvol;
	int		rightvol;
	int		rleftvol;
	int		rrightvol;
	float	pitch;
} channel_t;

bool CSoundCombiner::InitSplicer( IFileSystem *filesystem, int samplerate, int numchannels, int bitspersample )
{
	m_nSampleRate = samplerate;
	m_nNumChannels = numchannels;
	m_nBitsPerSample = bitspersample;
	m_nBytesPerSample = bitspersample >> 3;

	m_pWaveOutput = ( CAudioWaveOutput * )sound->GetAudioOutput();
	if ( !m_pWaveOutput )
	{
		Warning( "CSoundCombiner::InitSplicer  m_pWaveOutput == NULL\n" );
		return false;
	}

	// Make sure the directory exists
	char basepath[ 512 ];
	Q_ExtractFilePath( m_szOutFile, basepath, sizeof( basepath ) );
	filesystem->CreateDirHierarchy( basepath, "GAME" );

	// Create out put file
	m_pOutRIFF = new OutFileRIFF( m_szOutFile, io_out );
	if ( !m_pOutRIFF )
	{
		Warning( "CSoundCombiner::InitSplicer  m_pOutRIFF == NULL\n" );
		return false;
	}

	// Create output iterator
	m_pOutIterator = new IterateOutputRIFF( *m_pOutRIFF );
	if ( !m_pOutIterator )
	{
		Warning( "CSoundCombiner::InitSplicer  m_pOutIterator == NULL\n" );
		return false;
	}

	WAVEFORMATEX format;
	format.cbSize = sizeof( format );

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nAvgBytesPerSec = m_nSampleRate * m_nNumChannels * m_nBytesPerSample;
	format.nChannels = m_nNumChannels;
	format.wBitsPerSample = m_nBitsPerSample;
	format.nSamplesPerSec = m_nSampleRate;
	format.nBlockAlign = 1;

	// Always store the format chunk first
	m_pOutIterator->ChunkWrite( WAVE_FMT, &format, sizeof( format ) );

	return true;
}

bool CSoundCombiner::LoadSpliceAudioSources()
{
	int c = m_Work.Count();
	for ( int i = 0; i < c; ++i )
	{
		CombinerWork *item = m_Work[ i ];

		CAudioSource *wave = sound->LoadSound( item->entry->wavefile );
		if ( !wave )
		{
			Warning( "CSoundCombiner::LoadSpliceAudioSources  LoadSound failed '%s'\n", item->entry->wavefile );
			return false;
		}

		CAudioMixer *pMixer = wave->CreateMixer();
		if ( !pMixer )
		{
			Warning( "CSoundCombiner::LoadSpliceAudioSources  CreateMixer failed '%s'\n", item->entry->wavefile );
			return false;
		}
		
		item->wave	= wave;
		item->mixer = pMixer;
		item->duration = wave->GetRunningLength();
	}

	return true;
}

bool CSoundCombiner::AppendSilence( int &currentsample, float duration )
{
	int numSamples = duration * m_nSampleRate;

#define MOTION_RANGE 150
#define MOTION_MAXSTEP 20
	int currentValue = 32767;
	int maxValue = currentValue + ( MOTION_RANGE / 2 );
	int minValue = currentValue - ( MOTION_RANGE / 2 );

	short samples[ 2 ];

	while ( --numSamples >= 0 )
	{
		currentValue += random->RandomInt( -MOTION_MAXSTEP, MOTION_MAXSTEP );
		currentValue = min( maxValue, currentValue );
		currentValue = max( minValue, currentValue );

		// Downsample to 0 65556 range
		short s = (float)currentValue / 32768.0f;

		samples[ 0 ] = s;
		samples[ 1 ] = s;

		AppendStereo16Data( samples );
	}

	return true;
}

bool CSoundCombiner::AppendStereo16Data( short samples[ 2 ] )
{
// Convert from 16 bit, 2 channels to output size
	if ( m_nNumChannels == 1 )
	{
		if ( m_nBytesPerSample == 1 )
		{
			// Convert to 8 bit mono
			// left + right (2 channels ) * 16 bits
			float s1 = (float)( samples[ 0 ] >> 8 );
			float s2 = (float)( samples[ 1 ] >> 8 );

			float avg = ( s1 + s2 ) * 0.5f;
			avg = clamp( avg, -127.0f, 127.0f );
			byte chopped = (byte)( avg+ 127 );

			m_pOutIterator->ChunkWriteData( &chopped, sizeof( byte ) );
		}
		else if ( m_nBytesPerSample == 2 )
		{
			// Conver to 16 bit mono
			float s1 = (float)( samples[ 0 ] );
			float s2 = (float)( samples[ 1 ] );

			float avg = ( s1 + s2 ) * 0.5f;
			unsigned short chopped = (unsigned short)( avg );

			m_pOutIterator->ChunkWriteData( &chopped, sizeof( unsigned short ) );
		}
		else
		{
			Assert( 0 );
			return false;
		}
	}
	else if ( m_nNumChannels == 2 )
	{
		if ( m_nBytesPerSample == 1 )
		{
			// Convert to 8 bit stereo
			// left + right (2 channels ) * 16 bits
			float s1 = (float)( samples[ 0 ] >> 8 );
			float s2 = (float)( samples[ 1 ] >> 8 );

			s1 = clamp( s1, -127.0f, 127.0f );
			s2 = clamp( s2, -127.0f, 127.0f );

			byte chopped1 = (byte)( s1 + 127.0f );
			byte chopped2 = (byte)( s2 + 127.0f );

			m_pOutIterator->ChunkWriteData( &chopped1, sizeof( byte ) );
			m_pOutIterator->ChunkWriteData( &chopped2, sizeof( byte ) );
		}
		else if ( m_nBytesPerSample == 2 )
		{
			// Leave as 16 bit stereo
			// Directly store values
			m_pOutIterator->ChunkWriteData( &samples[ 0 ], sizeof( unsigned short ) );
			m_pOutIterator->ChunkWriteData( &samples[ 1 ], sizeof( unsigned short ) );
		}
		else
		{
			Assert( 0 );
			return false;
		}
	}
	else
	{
		Assert( 0 );
		return false;
	}

	return true;
}

bool CSoundCombiner::AppendWaveData( int& currentsample, CAudioSource *wave, CAudioMixer *mixer )
{
	// need a bit of space
	short samples[ 2 ];
	channel_t channel;
	memset( &channel, 0, sizeof( channel ) );
	channel.leftvol		= 255;
	channel.rightvol	= 255;
	channel.pitch		= 1.0;

	while ( 1 )
	{
		m_pWaveOutput->m_audioDevice.MixBegin();

		if ( !mixer->MixDataToDevice( &m_pWaveOutput->m_audioDevice, &channel, currentsample, 1, wave->SampleRate(), true ) )
			break;

		m_pWaveOutput->m_audioDevice.TransferBufferStereo16( samples, 1 );

		currentsample = mixer->GetSamplePosition();

		AppendStereo16Data( samples );
	}

	return true;
}

int CSoundCombiner::ComputeBestNumChannels()
{
	// We prefer mono output unless one of the source wav files is stereo, then we'll do stereo output
	int c = m_Work.Count();
	for ( int i = 0; i < c; ++i )
	{
		CombinerWork *curitem = m_Work[ i ];

		if ( curitem->wave->GetNumChannels() == 2 )
		{
			return 2;
		}
	}
	return 1;
}

bool CSoundCombiner::PerformSplicingOnWorkItems( IFileSystem *filesystem )
{
	if ( !LoadSpliceAudioSources() )
	{
		return false;
	}

	int bestNumChannels = ComputeBestNumChannels();
	int bitsPerChannel = WAVEOUTPUT_BITSPERCHANNEL;

	// Pull in data and write it out
	if ( !InitSplicer( filesystem, WAVEOUTPUT_FREQUENCY, bestNumChannels, bitsPerChannel ) )
	{
		return false;
	}

	m_pOutIterator->ChunkStart( WAVE_DATA );

	float timeoffset = 0.0f;

	m_Combined.Reset();
	m_Combined.SetText( "" );

	int c = m_Work.Count();
	for ( int i = 0; i < c; ++i )
	{
		int currentsample = 0;

		CombinerWork *curitem = m_Work[ i ];
		CombinerWork *nextitem = NULL;
		if ( i != c - 1 )
		{
			nextitem = m_Work[ i + 1 ];
		}

		float duration = curitem->duration;

		AppendWaveData( currentsample, curitem->wave, curitem->mixer );

		AddSentenceToCombined( timeoffset, curitem->sentence );

		timeoffset += duration;

		if ( nextitem != NULL )
		{
			float nextstart = nextitem->entry->startoffset;
			float silence_time = nextstart - timeoffset;

			AppendSilence( currentsample, silence_time );

			timeoffset += silence_time;
		}
	}

	m_pOutIterator->ChunkFinish();

	// Checksum the work items
	unsigned int checksum = ComputeChecksum();

	// Make sure the checksum is embedded in the data file
	m_Combined.SetDataCheckSum( checksum );

	// Msg( "  checksum computed %u\n", checksum );

	m_pOutIterator->ChunkStart( WAVE_VALVEDATA );
	StoreValveDataChunk( m_Combined );
	m_pOutIterator->ChunkFinish();


	return true;
}

void CSoundCombiner::AddSentenceToCombined( float offset, CSentence& sentence )
{
	m_Combined.Append( offset, sentence );
}
