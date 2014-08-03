//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "checksum_crc.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined( NO_ENTITY_PREDICTION )
//-----------------------------------------------------------------------------
// Purpose: Helper class for resetting instance numbers, etc.
//-----------------------------------------------------------------------------
class CPredictableIdHelper
{
public:
	CPredictableIdHelper()
	{
		Reset( -1 );
	}

	void	Reset( int command )
	{
		m_nCurrentCommand = command;
		m_nCount = 0;
		memset( m_Entries, 0, sizeof( m_Entries ) );
	}

	int		AddEntry( int command, int hash )
	{
		// Clear list if command number changes
		if ( command != m_nCurrentCommand )
		{
			Reset( command );
		}

		entry *e = FindOrAddEntry( hash );
		if ( !e )
			return 0;
		e->count++;
		return e->count-1;
	}

private:

	enum
	{
		MAX_ENTRIES = 256,
	};

	struct entry
	{
		int		hash;
		int		count;
	};

	entry			*FindOrAddEntry( int hash )
	{
		int i;
		for ( i = 0; i < m_nCount; i++ )
		{
			entry *e = &m_Entries[ i ];
			if ( e->hash == hash )
				return e;
		}

		if ( m_nCount >= MAX_ENTRIES )
		{
			// assert( 0 );
			return NULL;
		}

		entry *e = &m_Entries[ m_nCount++ ];
		e->hash = hash;
		e->count = 0;
		return e;
	}

	int				m_nCurrentCommand;
	int				m_nCount;
	entry			m_Entries[ MAX_ENTRIES ];
};

static CPredictableIdHelper g_Helper;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPredictableId::CPredictableId( void )
{
	memset( &m_PredictableID, 0, sizeof( m_PredictableID ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPredictableId::ResetInstanceCounters( void )
{
	g_Helper.Reset( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Is the Id being used
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPredictableId::IsActive( void ) const
{
	if ( *(const int *)&m_PredictableID == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : playerIndex - 
//-----------------------------------------------------------------------------
void CPredictableId::SetPlayer( int playerIndex )
{
	m_PredictableID.player = (unsigned int)playerIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableId::GetPlayer( void ) const
{
	return (int)m_PredictableID.player;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableId::GetCommandNumber( void ) const
{
	return (int)m_PredictableID.command;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : commandNumber - 
//-----------------------------------------------------------------------------
void CPredictableId::SetCommandNumber( int commandNumber )
{
	m_PredictableID.command = (unsigned int)commandNumber;
}

/*
bool CPredictableId::IsCommandNumberEqual( int testNumber ) const
{
	if ( ( testNumber & ((1<<10) - 1) ) == m_PredictableID.command )
		return true;

	return false;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//			*module - 
//			line - 
// Output : static int
//-----------------------------------------------------------------------------
static int ClassFileLineHash( const char *classname, const char *module, int line )
{
	CRC32_t retval;

	CRC32_Init( &retval );

	char tempbuffer[ 512 ];
	
	// ACK, have to go lower case due to issues with .dsp having different cases of drive
	//  letters, etc.!!!
	Q_strncpy( tempbuffer, classname, sizeof( tempbuffer ) );
	Q_strlower( tempbuffer );
	CRC32_ProcessBuffer( &retval, (void *)tempbuffer, Q_strlen( tempbuffer ) );
	
	Q_strncpy( tempbuffer, module, sizeof( tempbuffer ) );
	Q_strlower( tempbuffer );
	CRC32_ProcessBuffer( &retval, (void *)tempbuffer, Q_strlen( tempbuffer ) );
	
	CRC32_ProcessBuffer( &retval, (void *)&line, sizeof( int ) );

	CRC32_Final( &retval );

	return (int)retval;
}

//-----------------------------------------------------------------------------
// Purpose: Create a predictable id of the specified parameter set
// Input  : player - 
//			command - 
//			*classname - 
//			*module - 
//			line - 
//-----------------------------------------------------------------------------
void CPredictableId::Init( int player, int command, const char *classname, const char *module, int line )
{
	SetPlayer( player );
	SetCommandNumber( command );

	m_PredictableID.hash = ClassFileLineHash( classname, module, line );

	// Use helper to determine instance number this command
	int instance = g_Helper.AddEntry( command, m_PredictableID.hash );

	// Set appropriate instance number
	SetInstanceNumber( instance );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableId::GetHash( void ) const
{
	return (int)m_PredictableID.hash;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : counter - 
//-----------------------------------------------------------------------------
void CPredictableId::SetInstanceNumber( int counter )
{
	m_PredictableID.instance = (unsigned int)counter;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableId::GetInstanceNumber( void ) const
{
	return (int)m_PredictableID.instance;
}

// Client only
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ack - 
//-----------------------------------------------------------------------------
void CPredictableId::SetAcknowledged( bool ack )
{
	m_PredictableID.ack = ack ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPredictableId::GetAcknowledged( void ) const
{
	return m_PredictableID.ack ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableId::GetRaw( void ) const
{
	return *(int *)&m_PredictableID;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : raw - 
//-----------------------------------------------------------------------------
void CPredictableId::SetRaw( int raw )
{
	*(int *)&m_PredictableID = raw;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if one id is == another, ignores Acknowledged state
// Input  : other - 
// Output : bool CPredictableId::operator
//-----------------------------------------------------------------------------
bool CPredictableId::operator ==( const CPredictableId& other ) const
{
	if ( this == &other )
		return true;

	if ( GetPlayer() != other.GetPlayer() )
		return false;
	if ( GetCommandNumber() != other.GetCommandNumber() )
		return false;
	if ( GetHash() != other.GetHash() )
		return false;
	if ( GetInstanceNumber() != other.GetInstanceNumber() )
		return false;
	return true;
}

bool CPredictableId::operator !=( const CPredictableId& other ) const
{
	return !(*this == other);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CPredictableId::Describe( void ) const
{
	static char desc[ 128 ];

	Q_snprintf( desc, sizeof( desc ), "pl(%i) cmd(%i) hash(%i) inst(%i) ack(%s)",
		GetPlayer(),
		GetCommandNumber(),
		GetHash(),
		GetInstanceNumber() ,
		GetAcknowledged() ? "true" : "false" );

	return desc;
}
#endif
