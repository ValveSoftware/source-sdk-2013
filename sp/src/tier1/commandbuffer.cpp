//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#include "tier1/CommandBuffer.h"
#include "tier1/utlbuffer.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	MAX_ALIAS_NAME	32
#define MAX_COMMAND_LENGTH 1024

struct cmdalias_t
{
	cmdalias_t	*next;
	char		name[ MAX_ALIAS_NAME ];
	char		*value;
};


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CCommandBuffer::CCommandBuffer( ) : m_Commands( 32, 32 )
{
	m_hNextCommand = m_Commands.InvalidIndex();
	m_nWaitDelayTicks = 1;
	m_nCurrentTick = 0;
	m_nLastTickToProcess = -1;
	m_nArgSBufferSize = 0;
	m_bIsProcessingCommands = false;
	m_nMaxArgSBufferLength = ARGS_BUFFER_LENGTH;
}

CCommandBuffer::~CCommandBuffer()
{
}


//-----------------------------------------------------------------------------
// Indicates how long to delay when encoutering a 'wait' command
//-----------------------------------------------------------------------------
void CCommandBuffer::SetWaitDelayTime( int nTickDelay )
{
	Assert( nTickDelay >= 0 );
	m_nWaitDelayTicks = nTickDelay;
}

	
//-----------------------------------------------------------------------------
// Specifies a max limit of the args buffer. For unittesting. Size == 0 means use default
//-----------------------------------------------------------------------------
void CCommandBuffer::LimitArgumentBufferSize( int nSize )
{
	if ( nSize > ARGS_BUFFER_LENGTH )
	{
		nSize = ARGS_BUFFER_LENGTH;
	}

	m_nMaxArgSBufferLength = ( nSize == 0 ) ? ARGS_BUFFER_LENGTH : nSize;
}

	
//-----------------------------------------------------------------------------
// Parses argv0 out of the buffer
//-----------------------------------------------------------------------------
bool CCommandBuffer::ParseArgV0( CUtlBuffer &buf, char *pArgV0, int nMaxLen, const char **pArgS )
{
	pArgV0[0] = 0;
	*pArgS = NULL;

	if ( !buf.IsValid() )
		return false;

	int	nSize = buf.ParseToken( CCommand::DefaultBreakSet(), pArgV0, nMaxLen );
	if ( ( nSize <= 0 ) || ( nMaxLen == nSize ) )
		return false;

	int nArgSLen = buf.TellMaxPut() - buf.TellGet();
	*pArgS = (nArgSLen > 0) ? (const char*)buf.PeekGet() : NULL;
	return true;
}


//-----------------------------------------------------------------------------
// Insert a command into the command queue
//-----------------------------------------------------------------------------
void CCommandBuffer::InsertCommandAtAppropriateTime( int hCommand )
{
	int i;
	Command_t &command = m_Commands[hCommand];
	for ( i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next(i) )
	{
		if ( m_Commands[i].m_nTick > command.m_nTick )
			break;
	}
	m_Commands.LinkBefore( i, hCommand );
}


//-----------------------------------------------------------------------------
// Insert a command into the command queue at the appropriate time
//-----------------------------------------------------------------------------
void CCommandBuffer::InsertImmediateCommand( int hCommand )
{
	m_Commands.LinkBefore( m_hNextCommand, hCommand );
}


//-----------------------------------------------------------------------------
// Insert a command into the command queue
//-----------------------------------------------------------------------------
bool CCommandBuffer::InsertCommand( const char *pArgS, int nCommandSize, int nTick )
{
	if ( nCommandSize >= CCommand::MaxCommandLength() )
	{
		Warning( "WARNING: Command too long... ignoring!\n%s\n", pArgS );
		return false;
	}

	// Add one for null termination
	if ( m_nArgSBufferSize + nCommandSize + 1 > m_nMaxArgSBufferLength )
	{
		Compact();
		if ( m_nArgSBufferSize + nCommandSize + 1 > m_nMaxArgSBufferLength )
			return false;
	}
	
	memcpy( &m_pArgSBuffer[m_nArgSBufferSize], pArgS, nCommandSize );
	m_pArgSBuffer[m_nArgSBufferSize + nCommandSize] = 0;
	++nCommandSize;

	int hCommand = m_Commands.Alloc();
	Command_t &command = m_Commands[hCommand];
	command.m_nTick = nTick;
	command.m_nFirstArgS = m_nArgSBufferSize;
	command.m_nBufferSize = nCommandSize;

	m_nArgSBufferSize += nCommandSize;

	if ( !m_bIsProcessingCommands || ( nTick > m_nCurrentTick ) )
	{
		InsertCommandAtAppropriateTime( hCommand );
	}
	else
	{
		InsertImmediateCommand( hCommand );
	}
	return true;
}

		
//-----------------------------------------------------------------------------
// Returns the length of the next command
//-----------------------------------------------------------------------------
void CCommandBuffer::GetNextCommandLength( const char *pText, int nMaxLen, int *pCommandLength, int *pNextCommandOffset )
{
	int nCommandLength = 0;
	int nNextCommandOffset;
	bool bIsQuoted = false;
	bool bIsCommented = false;
	for ( nNextCommandOffset=0; nNextCommandOffset < nMaxLen; ++nNextCommandOffset, nCommandLength += bIsCommented ? 0 : 1 )
	{
		char c = pText[nNextCommandOffset];
		if ( !bIsCommented )
		{
			if ( c == '"' )
			{
				bIsQuoted = !bIsQuoted;
				continue;
			}

			// don't break if inside a C++ style comment
			if ( !bIsQuoted && c == '/' )
			{
				bIsCommented = ( nNextCommandOffset < nMaxLen-1 ) && pText[nNextCommandOffset+1] == '/';
				if ( bIsCommented )
				{
					++nNextCommandOffset;
					continue;
				}
			}

			// don't break if inside a quoted string
			if ( !bIsQuoted && c == ';' )
				break;	
		}

		// FIXME: This is legacy behavior; should we not break if a \n is inside a quoted string?
		if ( c == '\n' )
			break;
	}

	*pCommandLength = nCommandLength;
	*pNextCommandOffset = nNextCommandOffset;
}


//-----------------------------------------------------------------------------
// Add text to command buffer, return false if it couldn't owing to overflow
//-----------------------------------------------------------------------------
bool CCommandBuffer::AddText( const char *pText, int nTickDelay )
{
	Assert( nTickDelay >= 0 );

	int	nLen = Q_strlen( pText );
	int nTick = m_nCurrentTick + nTickDelay;

	// Parse the text into distinct commands
	const char *pCurrentCommand = pText;
	int nOffsetToNextCommand;
	for( ; nLen > 0; nLen -= nOffsetToNextCommand+1, pCurrentCommand += nOffsetToNextCommand+1 )
	{
		// find a \n or ; line break
		int nCommandLength;
		GetNextCommandLength( pCurrentCommand, nLen, &nCommandLength, &nOffsetToNextCommand );
		if ( nCommandLength <= 0 )
			continue;

		const char *pArgS;
		char *pArgV0 = (char*)_alloca( nCommandLength+1 );
		CUtlBuffer bufParse( pCurrentCommand, nCommandLength, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY ); 
		ParseArgV0( bufParse, pArgV0, nCommandLength+1, &pArgS );
		if ( pArgV0[0] == 0 )
			continue;

		// Deal with the special 'wait' command
		if ( !Q_stricmp( pArgV0, "wait" ) && IsWaitEnabled() )
		{
			int nDelay = pArgS ? atoi( pArgS ) : m_nWaitDelayTicks;
			nTick += nDelay;
			continue;
		}

		if ( !InsertCommand( pCurrentCommand, nCommandLength, nTick ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Are we in the middle of processing commands?
//-----------------------------------------------------------------------------
bool CCommandBuffer::IsProcessingCommands()
{
	return m_bIsProcessingCommands;
}
	
	
//-----------------------------------------------------------------------------
// Delays all queued commands to execute at a later time
//-----------------------------------------------------------------------------
void CCommandBuffer::DelayAllQueuedCommands( int nDelay )
{
	if ( nDelay <= 0 )
		return;

	for ( int i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next(i) )
	{
		m_Commands[i].m_nTick += nDelay;			
	}
}

	
//-----------------------------------------------------------------------------
// Call this to begin iterating over all commands up to flCurrentTime
//-----------------------------------------------------------------------------
void CCommandBuffer::BeginProcessingCommands( int nDeltaTicks )
{
	if ( nDeltaTicks == 0 )
		return;

	Assert( !m_bIsProcessingCommands );
	m_bIsProcessingCommands = true;
	m_nLastTickToProcess = m_nCurrentTick + nDeltaTicks - 1;

	// Necessary to insert commands while commands are being processed
	m_hNextCommand = m_Commands.Head();
}


//-----------------------------------------------------------------------------
// Returns the next command
//-----------------------------------------------------------------------------
bool CCommandBuffer::DequeueNextCommand( )
{
	m_CurrentCommand.Reset();

	Assert( m_bIsProcessingCommands );
	if ( m_Commands.Count() == 0 )
		return false;

	int nHead = m_Commands.Head();
	Command_t &command = m_Commands[ nHead ];
	if ( command.m_nTick > m_nLastTickToProcess )
		return false;

	m_nCurrentTick = command.m_nTick;

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	if ( command.m_nBufferSize > 0 )
	{
		m_CurrentCommand.Tokenize( &m_pArgSBuffer[command.m_nFirstArgS] );
	}

	m_Commands.Remove( nHead );

	// Necessary to insert commands while commands are being processed
	m_hNextCommand = m_Commands.Head();

//	Msg("Dequeue : ");
//	for ( int i = 0; i < nArgc; ++i )
//	{
//		Msg("%s ", m_pCurrentArgv[i] ); 
//	}
//	Msg("\n");
	return true;
}


//-----------------------------------------------------------------------------
// Returns the next command
//-----------------------------------------------------------------------------
int CCommandBuffer::DequeueNextCommand( const char **& ppArgv )
{
	DequeueNextCommand();
	ppArgv = ArgV();
	return ArgC();
}


//-----------------------------------------------------------------------------
// Compacts the command buffer
//-----------------------------------------------------------------------------
void CCommandBuffer::Compact()
{
	// Compress argvbuffer + argv
	// NOTE: I'm using this choice instead of calling malloc + free
	// per command to allocate arguments because I expect to post a
	// bunch of commands but not have many delayed commands; 
	// avoiding the allocation cost seems more important that the memcpy 
	// cost here since I expect to not have much to copy.
	m_nArgSBufferSize = 0;

	char pTempBuffer[ ARGS_BUFFER_LENGTH ];
	for ( int i = m_Commands.Head(); i != m_Commands.InvalidIndex(); i = m_Commands.Next(i) )
	{
		Command_t &command = m_Commands[ i ];

		memcpy( &pTempBuffer[m_nArgSBufferSize], &m_pArgSBuffer[command.m_nFirstArgS], command.m_nBufferSize );
		command.m_nFirstArgS = m_nArgSBufferSize;
		m_nArgSBufferSize += command.m_nBufferSize;
	}

	// NOTE: We could also store 2 buffers in the command buffer and switch
	// between the two to avoid the 2nd memcpy; but again I'm guessing the memory
	// tradeoff isn't worth it
	memcpy( m_pArgSBuffer, pTempBuffer, m_nArgSBufferSize );
}


//-----------------------------------------------------------------------------
// Call this to finish iterating over all commands
//-----------------------------------------------------------------------------
void CCommandBuffer::EndProcessingCommands()
{
	Assert( m_bIsProcessingCommands );
	m_bIsProcessingCommands = false;
	m_nCurrentTick = m_nLastTickToProcess + 1;
	m_hNextCommand = m_Commands.InvalidIndex();

	// Extract commands that are before the end time
	// NOTE: This is a bug for this to 
	int i = m_Commands.Head();
	if ( i == m_Commands.InvalidIndex() )
	{
		m_nArgSBufferSize = 0;
		return;
	}

	while ( i != m_Commands.InvalidIndex() )
	{
		if ( m_Commands[i].m_nTick >= m_nCurrentTick )
			break;

		AssertMsgOnce( false, "CCommandBuffer::EndProcessingCommands() called before all appropriate commands were dequeued.\n" );
		int nNext = i;
		Msg( "Warning: Skipping command %s\n", &m_pArgSBuffer[ m_Commands[i].m_nFirstArgS ] );
		m_Commands.Remove( i );
		i = nNext;
	}

	Compact();
}


//-----------------------------------------------------------------------------
// Returns a handle to the next command to process
//-----------------------------------------------------------------------------
CommandHandle_t CCommandBuffer::GetNextCommandHandle()
{
	Assert( m_bIsProcessingCommands );
	return m_Commands.Head();
}


#if 0
/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[MAX_COMMAND_LENGTH];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		Q_strncat(cmd, Cmd_Argv(i), sizeof( cmd ), COPY_ALL_CHARACTERS);
		if (i != c)
		{
			Q_strncat (cmd, " ", sizeof( cmd ), COPY_ALL_CHARACTERS );
		}
	}
	Q_strncat (cmd, "\n", sizeof( cmd ), COPY_ALL_CHARACTERS);

	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			if ( !strcmp( a->value, cmd ) )		// Re-alias the same thing
				return;

			delete[] a->value;
			break;
		}
	}

	if (!a)
	{
		a = (cmdalias_t *)new cmdalias_t;
		a->next = cmd_alias;
		cmd_alias = a;
	}
	Q_strncpy (a->name, s, sizeof( a->name ) );	

	a->value = COM_StringCopy(cmd);
}


	
/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	const char	*cmd_args = NULL;

cmd_source_t	cmd_source;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Cmd_Init
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cmd_Shutdown( void )
{
	// TODO, cleanup
	while ( cmd_alias )
	{
		cmdalias_t *next = cmd_alias->next;
		delete cmd_alias->value;	// created by StringCopy()
		delete cmd_alias;
		cmd_alias = next;
	}
}



/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
const ConCommandBase *Cmd_ExecuteString (const char *text, cmd_source_t src)
{	
	cmdalias_t		*a;

	cmd_source = src;
	Cmd_TokenizeString (text);
			
// execute the command line
	if (!Cmd_Argc())
		return NULL;		// no tokens

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			Cbuf_InsertText (a->value);
			return NULL;
		}
	}
	
// check ConCommands
	ConCommandBase const *pCommand = ConCommandBase::FindCommand( cmd_argv[ 0 ] );
	if ( pCommand && pCommand->IsCommand() )
	{
		bool isServerCommand = ( pCommand->IsBitSet( FCVAR_GAMEDLL ) && 
								// Typed at console
								cmd_source == src_command &&
								// Not HLDS
								!sv.IsDedicated() );

		// Hook to allow game .dll to figure out who type the message on a listen server
		if ( serverGameClients )
		{
			// We're actually the server, so set it up locally
			if ( sv.IsActive() )
			{
				g_pServerPluginHandler->SetCommandClient( -1 );
						
#ifndef SWDS
				// Special processing for listen server player
				if ( isServerCommand )
				{
					g_pServerPluginHandler->SetCommandClient( cl.m_nPlayerSlot );
				}
#endif
			}
			// We're not the server, but we've been a listen server (game .dll loaded)
			//  forward this command tot he server instead of running it locally if we're still
			//  connected
			// Otherwise, things like "say" won't work unless you quit and restart
			else if ( isServerCommand )
			{
				if ( cl.IsConnected() )
				{
					Cmd_ForwardToServer();
					return NULL;
				}
				else
				{
					// It's a server command, but we're not connected to a server.  Don't try to execute it.
					return NULL;
				}
			}
		}

		// Allow cheat commands in singleplayer, debug, or multiplayer with sv_cheats on
#ifndef _DEBUG
		if ( pCommand->IsBitSet( FCVAR_CHEAT ) )
		{
			if ( !Host_IsSinglePlayerGame() && sv_cheats.GetInt() == 0 )
			{
				Msg( "Can't use cheat command %s in multiplayer, unless the server has sv_cheats set to 1.\n", pCommand->GetName() );
				return NULL;
			}
		}
#endif

		(( ConCommand * )pCommand )->Dispatch();
		return pCommand;
	}

	// check cvars
	if ( cv->IsCommand() )
	{
		return pCommand;
	}

	// forward the command line to the server, so the entity DLL can parse it
	if ( cmd_source == src_command )
	{
		if ( cl.IsConnected() )
		{
			Cmd_ForwardToServer();
			return NULL;
		}
	}
	
	Msg("Unknown command \"%s\"\n", Cmd_Argv(0));

	return NULL;
}
#endif
