//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"
#include "tier1/utlbuffer.h"
#include "tier1/tier1.h"
#include "tier1/convar_serverbounded.h"
#include "icvar.h"
#include "tier0/dbg.h"
#include "Color.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
#include "tier0/memdbgon.h"

#ifndef NDEBUG
// Comment this out when we release.
#define ALLOW_DEVELOPMENT_CVARS
#endif



//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
ConCommandBase			*ConCommandBase::s_pConCommandBases = NULL;
IConCommandBaseAccessor	*ConCommandBase::s_pAccessor = NULL;
static int s_nCVarFlag = 0;
static int s_nDLLIdentifier = -1;	// A unique identifier indicating which DLL this convar came from
static bool s_bRegistered = false;

class CDefaultAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase( ConCommandBase *pVar )
	{
		// Link to engine's list instead
		g_pCVar->RegisterConCommand( pVar );
		return true;
	}
};

static CDefaultAccessor s_DefaultAccessor;

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int nCVarFlag, IConCommandBaseAccessor *pAccessor )
{
	if ( !g_pCVar || s_bRegistered )
		return;

	Assert( s_nDLLIdentifier < 0 );
	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;
	s_nDLLIdentifier = g_pCVar->AllocateDLLIdentifier();

	ConCommandBase *pCur, *pNext;

	ConCommandBase::s_pAccessor = pAccessor ? pAccessor : &s_DefaultAccessor;
	pCur = ConCommandBase::s_pConCommandBases;
	while ( pCur )
	{
		pNext = pCur->m_pNext;
		pCur->AddFlags( s_nCVarFlag );
		pCur->Init();
		pCur = pNext;
	}

	g_pCVar->ProcessQueuedMaterialThreadConVarSets();
	ConCommandBase::s_pConCommandBases = NULL;
}

void ConVar_Unregister( )
{
	if ( !g_pCVar || !s_bRegistered )
		return;

	Assert( s_nDLLIdentifier >= 0 );
	g_pCVar->UnregisterConCommands( s_nDLLIdentifier );
	s_nDLLIdentifier = -1;
	s_bRegistered = false;
}


//-----------------------------------------------------------------------------
// Purpose: Default constructor
//-----------------------------------------------------------------------------
ConCommandBase::ConCommandBase( void )
{
	m_bRegistered   = false;
	m_pszName       = NULL;
	m_pszHelpString = NULL;

	m_nFlags = 0;
	m_pNext  = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
// Input  : *pName - name of variable/command
//			*pHelpString - help text
//			flags - flags
//-----------------------------------------------------------------------------
ConCommandBase::ConCommandBase( const char *pName, const char *pHelpString /*=0*/, int flags /*= 0*/ )
{
	Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConCommandBase::~ConCommandBase( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand( void ) const
{ 
//	Assert( 0 ); This can't assert. . causes a recursive assert in Sys_Printf, etc.
	return true;
}


//-----------------------------------------------------------------------------
// Returns the DLL identifier
//-----------------------------------------------------------------------------
CVarDLLIdentifier_t ConCommandBase::GetDLLIdentifier() const
{
	return s_nDLLIdentifier;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::Create( const char *pName, const char *pHelpString /*= 0*/, int flags /*= 0*/ )
{
	static char *empty_string = "";

	m_bRegistered = false;

	// Name should be static data
	Assert( pName );
	m_pszName = pName;
	m_pszHelpString = pHelpString ? pHelpString : empty_string;

	m_nFlags = flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif

	if ( !( m_nFlags & FCVAR_UNREGISTERED ) )
	{
		m_pNext = s_pConCommandBases;
		s_pConCommandBases = this;
	}
	else
	{
		// It's unregistered
		m_pNext = NULL;
	}

	// If s_pAccessor is already set (this ConVar is not a global variable),
	//  register it.
	if ( s_pAccessor )
	{
		Init();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used internally by OneTimeInit to initialize.
//-----------------------------------------------------------------------------
void ConCommandBase::Init()
{
	if ( s_pAccessor )
	{
		s_pAccessor->RegisterConCommandBase( this );
	}
}

void ConCommandBase::Shutdown()
{
	if ( g_pCVar )
	{
		g_pCVar->UnregisterConCommand( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return name of the command/var
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flag - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet( int flag ) const
{
	return ( flag & m_nFlags ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags( int flags )
{
	m_nFlags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const ConCommandBase
//-----------------------------------------------------------------------------
const ConCommandBase *ConCommandBase::GetNext( void ) const
{
	return m_pNext;
}

ConCommandBase *ConCommandBase::GetNext( void )
{
	return m_pNext;
}


//-----------------------------------------------------------------------------
// Purpose: Copies string using local new/delete operators
// Input  : *from - 
// Output : char
//-----------------------------------------------------------------------------
char *ConCommandBase::CopyString( const char *from )
{
	int		len;
	char	*to;

	len = V_strlen( from );
	if ( len <= 0 )
	{
		to = new char[1];
		to[0] = 0;
	}
	else
	{
		to = new char[len+1];
		Q_strncpy( to, from, len+1 );
	}
	return to;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetHelpText( void ) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Has this cvar been registered
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsRegistered( void ) const
{
	return m_bRegistered;
}


//-----------------------------------------------------------------------------
//
// Con Commands start here
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global methods
//-----------------------------------------------------------------------------
static characterset_t s_BreakSet;
static bool s_bBuiltBreakSet = false;


//-----------------------------------------------------------------------------
// Tokenizer class
//-----------------------------------------------------------------------------
CCommand::CCommand()
{
	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();
}

CCommand::CCommand( int nArgC, const char **ppArgV )
{
	Assert( nArgC > 0 );

	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();

	char *pBuf = m_pArgvBuffer;
	char *pSBuf = m_pArgSBuffer;
	m_nArgc = nArgC;
	for ( int i = 0; i < nArgC; ++i )
	{
		m_ppArgv[i] = pBuf;
		int nLen = Q_strlen( ppArgV[i] );
		memcpy( pBuf, ppArgV[i], nLen+1 );
		if ( i == 0 )
		{
			m_nArgv0Size = nLen;
		}
		pBuf += nLen+1;

		bool bContainsSpace = strchr( ppArgV[i], ' ' ) != NULL;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}
		memcpy( pSBuf, ppArgV[i], nLen );
		pSBuf += nLen;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}

		if ( i != nArgC - 1 )
		{
			*pSBuf++ = ' ';
		}
	}
}

void CCommand::Reset()
{
	m_nArgc = 0;
	m_nArgv0Size = 0;
	m_pArgSBuffer[0] = 0;
}

characterset_t* CCommand::DefaultBreakSet()
{
	return &s_BreakSet;
}

bool CCommand::Tokenize( const char *pCommand, characterset_t *pBreakSet )
{
	Reset();
	if ( !pCommand )
		return false;

	// Use default break set
	if ( !pBreakSet )
	{
		pBreakSet = &s_BreakSet;
	}

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	int nLen = Q_strlen( pCommand );
	if ( nLen >= COMMAND_MAX_LENGTH - 1 )
	{
		Warning( "CCommand::Tokenize: Encountered command which overflows the tokenizer buffer.. Skipping!\n" );
		return false;
	}

	memcpy( m_pArgSBuffer, pCommand, nLen + 1 );

	// Parse the current command into the current command buffer
	CUtlBuffer bufParse( m_pArgSBuffer, nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY ); 
	int nArgvBufferSize = 0;
	while ( bufParse.IsValid() && ( m_nArgc < COMMAND_MAX_ARGC ) )
	{
		char *pArgvBuf = &m_pArgvBuffer[nArgvBufferSize];
		int nMaxLen = COMMAND_MAX_LENGTH - nArgvBufferSize;
		int nStartGet = bufParse.TellGet();
		int	nSize = bufParse.ParseToken( pBreakSet, pArgvBuf, nMaxLen );
		if ( nSize < 0 )
			break;

		// Check for overflow condition
		if ( nMaxLen == nSize )
		{
			Reset();
			return false;
		}

		if ( m_nArgc == 1 )
		{
			// Deal with the case where the arguments were quoted
			m_nArgv0Size = bufParse.TellGet();
			bool bFoundEndQuote = m_pArgSBuffer[m_nArgv0Size-1] == '\"';
			if ( bFoundEndQuote )
			{
				--m_nArgv0Size;
			}
			m_nArgv0Size -= nSize;
			Assert( m_nArgv0Size != 0 );

			// The StartGet check is to handle this case: "foo"bar
			// which will parse into 2 different args. ArgS should point to bar.
			bool bFoundStartQuote = ( m_nArgv0Size > nStartGet ) && ( m_pArgSBuffer[m_nArgv0Size-1] == '\"' );
			Assert( bFoundEndQuote == bFoundStartQuote );
			if ( bFoundStartQuote )
			{
				--m_nArgv0Size;
			}
		}

		m_ppArgv[ m_nArgc++ ] = pArgvBuf;
		if( m_nArgc >= COMMAND_MAX_ARGC )
		{
			Warning( "CCommand::Tokenize: Encountered command which overflows the argument buffer.. Clamped!\n" );
		}

		nArgvBufferSize += nSize + 1;
		Assert( nArgvBufferSize <= COMMAND_MAX_LENGTH );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Helper function to parse arguments to commands.
//-----------------------------------------------------------------------------
const char* CCommand::FindArg( const char *pName ) const
{
	int nArgC = ArgC();
	for ( int i = 1; i < nArgC; i++ )
	{
		if ( !Q_stricmp( Arg(i), pName ) )
			return (i+1) < nArgC ? Arg( i+1 ) : "";
	}
	return 0;
}

int CCommand::FindArgInt( const char *pName, int nDefaultVal ) const
{
	const char *pVal = FindArg( pName );
	if ( pVal )
		return atoi( pVal );
	else
		return nDefaultVal;
}


//-----------------------------------------------------------------------------
// Default console command autocompletion function 
//-----------------------------------------------------------------------------
int DefaultCompletionFunc( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Constructs a console command
//-----------------------------------------------------------------------------
//ConCommand::ConCommand()
//{
//	m_bIsNewConCommand = true;
//}

ConCommand::ConCommand( const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	BaseClass::Create( pName, pHelpString, flags );
}

ConCommand::ConCommand( const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	BaseClass::Create( pName, pHelpString, flags );
}

ConCommand::ConCommand( const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = ( pCompletionCallback != 0 );
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	BaseClass::Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConCommand::~ConCommand( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand( void ) const
{ 
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Invoke the function if there is one
//-----------------------------------------------------------------------------
void ConCommand::Dispatch( const CCommand &command )
{
	if ( m_bUsingNewCommandCallback )
	{
		if ( m_fnCommandCallback )
		{
			( *m_fnCommandCallback )( command );
			return;
		}
	}
	else if ( m_bUsingCommandCallbackInterface )
	{
		if ( m_pCommandCallback )
		{
			m_pCommandCallback->CommandCallback( command );
			return;
		}
	}
	else
	{
		if ( m_fnCommandCallbackV1 )
		{
			( *m_fnCommandCallbackV1 )();
			return;
		}
	}

	// Command without callback!!!
	AssertMsg( 0, ( "Encountered ConCommand '%s' without a callback!\n", GetName() ) );
}


//-----------------------------------------------------------------------------
// Purpose: Calls the autocompletion method to get autocompletion suggestions
//-----------------------------------------------------------------------------
int	ConCommand::AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands )
{
	if ( m_bUsingCommandCallbackInterface )
	{
		if ( !m_pCommandCompletionCallback )
			return 0;
		return m_pCommandCompletionCallback->CommandCompletionCallback( partial, commands );
	}

	Assert( m_fnCompletionCallback );
	if ( !m_fnCompletionCallback )
		return 0;

	char rgpchCommands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ];
	int iret = ( m_fnCompletionCallback )( partial, rgpchCommands );
	for ( int i = 0 ; i < iret; ++i )
	{
		CUtlString str = rgpchCommands[ i ];
		commands.AddToTail( str );
	}
	return iret;
}


//-----------------------------------------------------------------------------
// Returns true if the console command can autocomplete 
//-----------------------------------------------------------------------------
bool ConCommand::CanAutoComplete( void )
{
	return m_bHasCompletionCallback;
}



//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Various constructors
//-----------------------------------------------------------------------------
ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags /* = 0 */ )
{
	Create( pName, pDefaultValue, flags );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString )
{
	Create( pName, pDefaultValue, flags, pHelpString );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, callback );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, callback );
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar( void )
{
	if ( m_pszString )
	{
		delete[] m_pszString;
		m_pszString = NULL;
	}
}


//-----------------------------------------------------------------------------
// Install a change callback (there shouldn't already be one....)
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback( FnChangeCallback_t callback )
{
	Assert( !m_pParent->m_fnChangeCallback || !callback );
	m_pParent->m_fnChangeCallback = callback;

	if ( m_pParent->m_fnChangeCallback )
	{
		// Call it immediately to set the initial value...
		m_pParent->m_fnChangeCallback( this, m_pszString, m_fValue );
	}
}

bool ConVar::IsFlagSet( int flag ) const
{
	return ( flag & m_pParent->m_nFlags ) ? true : false;
}

const char *ConVar::GetHelpText( void ) const
{
	return m_pParent->m_pszHelpString;
}

void ConVar::AddFlags( int flags )
{
	m_pParent->m_nFlags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_pParent->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

bool ConVar::IsRegistered( void ) const
{
	return m_pParent->m_bRegistered;
}

const char *ConVar::GetName( void ) const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConVar::IsCommand( void ) const
{ 
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void ConVar::Init()
{
	BaseClass::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetValue( const char *value )
{
	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, value );
			return;
		}
	}

	float fNewValue;
	char  tempVal[ 32 ];
	char  *val;

	Assert(m_pParent == this); // Only valid for root convars.

	float flOldValue = m_fValue;

	val = (char *)value;
	if ( !value )
		fNewValue = 0.0f;
	else
		fNewValue = ( float )atof( value );

	if ( ClampValue( fNewValue ) )
	{
		Q_snprintf( tempVal,sizeof(tempVal), "%f", fNewValue );
		val = tempVal;
	}
	
	// Redetermine value
	m_fValue		= fNewValue;
	m_nValue		= ( int )( m_fValue );

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		ChangeStringValue( val, flOldValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tempVal - 
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue( const char *tempVal, float flOldValue )
{
	Assert( !( m_nFlags & FCVAR_NEVER_AS_STRING ) );

 	char* pszOldValue = (char*)stackalloc( m_StringLength );
	memcpy( pszOldValue, m_pszString, m_StringLength );
	
	if ( tempVal )
	{
		int len = Q_strlen(tempVal) + 1;

		if ( len > m_StringLength)
		{
			if (m_pszString)
			{
				delete[] m_pszString;
			}

			m_pszString	= new char[len];
			m_StringLength = len;
		}

		memcpy( m_pszString, tempVal, len );
	}
	else 
	{
		*m_pszString = 0;
	}

	// Invoke any necessary callback function
	if ( m_fnChangeCallback )
	{
		m_fnChangeCallback( this, pszOldValue, flOldValue );
	}

	g_pCVar->CallGlobalChangeCallbacks( this, pszOldValue, flOldValue );

	stackfree( pszOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp
// Input  : value - 
// Output : Returns true if value changed
//-----------------------------------------------------------------------------
bool ConVar::ClampValue( float& value )
{
	if ( m_bHasMin && ( value < m_fMinVal ) )
	{
		value = m_fMinVal;
		return true;
	}
	
	if ( m_bHasMax && ( value > m_fMaxVal ) )
	{
		value = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetFloatValue( float fNewValue )
{
	if ( fNewValue == m_fValue )
		return;

	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, fNewValue );
			return;
		}
	}

	Assert( m_pParent == this ); // Only valid for root convars.

	// Check bounds
	ClampValue( fNewValue );

	// Redetermine value
	float flOldValue = m_fValue;
	m_fValue		= fNewValue;
	m_nValue		= ( int )m_fValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal), "%f", m_fValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetIntValue( int nValue )
{
	if ( nValue == m_nValue )
		return;

	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, nValue );
			return;
		}
	}

	Assert( m_pParent == this ); // Only valid for root convars.

	float fValue = (float)nValue;
	if ( ClampValue( fValue ) )
	{
		nValue = ( int )( fValue );
	}

	// Redetermine value
	float flOldValue = m_fValue;
	m_fValue		= fValue;
	m_nValue		= nValue;

	if ( !( m_nFlags & FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%d", m_nValue );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Private creation
//-----------------------------------------------------------------------------
void ConVar::Create( const char *pName, const char *pDefaultValue, int flags /*= 0*/,
	const char *pHelpString /*= NULL*/, bool bMin /*= false*/, float fMin /*= 0.0*/,
	bool bMax /*= false*/, float fMax /*= false*/, FnChangeCallback_t callback /*= NULL*/ )
{
	m_pParent = this;

	// Name should be static data
	SetDefault( pDefaultValue );

	m_StringLength = V_strlen( m_pszDefaultValue ) + 1;
	m_pszString = new char[m_StringLength];
	memcpy( m_pszString, m_pszDefaultValue, m_StringLength );
	
	m_bHasMin = bMin;
	m_fMinVal = fMin;
	m_bHasMax = bMax;
	m_fMaxVal = fMax;
	
	m_fnChangeCallback = callback;

	m_fValue = ( float )atof( m_pszString );
	m_nValue = atoi( m_pszString ); // dont convert from float to int and lose bits

	// Bounds Check, should never happen, if it does, no big deal
	if ( m_bHasMin && ( m_fValue < m_fMinVal ) )
	{
		Assert( 0 );
	}

	if ( m_bHasMax && ( m_fValue > m_fMaxVal ) )
	{
		Assert( 0 );
	}

	BaseClass::Create( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char *value)
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( float value )
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetFloatValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( int value )
{
	ConVar *var = ( ConVar * )m_pParent;
	var->InternalSetIntValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value
//-----------------------------------------------------------------------------
void ConVar::Revert( void )
{
	// Force default value again
	ConVar *var = ( ConVar * )m_pParent;
	var->SetValue( var->m_pszDefaultValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : minVal - 
// Output : true if there is a min set
//-----------------------------------------------------------------------------
bool ConVar::GetMin( float& minVal ) const
{
	minVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : maxVal - 
//-----------------------------------------------------------------------------
bool ConVar::GetMax( float& maxVal ) const
{
	maxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConVar::GetDefault( void ) const
{
	return m_pParent->m_pszDefaultValue;
}

void ConVar::SetDefault( const char *pszDefault ) 
{ 
	static char *empty_string = "";
	m_pszDefaultValue = pszDefault ? pszDefault : empty_string;
	Assert( m_pszDefaultValue );
}

//-----------------------------------------------------------------------------
// This version is simply used to make reading convars simpler.
// Writing convars isn't allowed in this mode
//-----------------------------------------------------------------------------
class CEmptyConVar : public ConVar
{
public:
	CEmptyConVar() : ConVar( "", "0" ) {}
	// Used for optimal read access
	virtual void SetValue( const char *pValue ) {}
	virtual void SetValue( float flValue ) {}
	virtual void SetValue( int nValue ) {}
	virtual const char *GetName( void ) const { return ""; }
	virtual bool IsFlagSet( int nFlags ) const { return false; }
};

static CEmptyConVar s_EmptyConVar;

ConVarRef::ConVarRef( const char *pName )
{
	Init( pName, false );
}

ConVarRef::ConVarRef( const char *pName, bool bIgnoreMissing )
{
	Init( pName, bIgnoreMissing );
}

void ConVarRef::Init( const char *pName, bool bIgnoreMissing )
{
	m_pConVar = g_pCVar ? g_pCVar->FindVar( pName ) : &s_EmptyConVar;
	if ( !m_pConVar )
	{
		m_pConVar = &s_EmptyConVar;
	}
	m_pConVarState = static_cast< ConVar * >( m_pConVar );
	if( !IsValid() )
	{
		static bool bFirst = true;
		if ( g_pCVar || bFirst )
		{
			if ( !bIgnoreMissing )
			{
				Warning( "ConVarRef %s doesn't point to an existing ConVar\n", pName );
			}
			bFirst = false;
		}
	}
}

ConVarRef::ConVarRef( IConVar *pConVar )
{
	m_pConVar = pConVar ? pConVar : &s_EmptyConVar;
	m_pConVarState = static_cast< ConVar * >( m_pConVar );
}

bool ConVarRef::IsValid() const
{
	return m_pConVar != &s_EmptyConVar;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintFlags( const ConCommandBase *var )
{
	bool any = false;
	if ( var->IsFlagSet( FCVAR_GAMEDLL ) )
	{
		ConMsg( " game" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTDLL ) )
	{
		ConMsg( " client" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_ARCHIVE ) )
	{
		ConMsg( " archive" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOTIFY ) )
	{
		ConMsg( " notify" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_SPONLY ) )
	{
		ConMsg( " singleplayer" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOT_CONNECTED ) )
	{
		ConMsg( " notconnected" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CHEAT ) )
	{
		ConMsg( " cheat" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_REPLICATED ) )
	{
		ConMsg( " replicated" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_SERVER_CAN_EXECUTE ) )
	{
		ConMsg( " server_can_execute" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTCMD_CAN_EXECUTE ) )
	{
		ConMsg( " clientcmd_can_execute" );
		any = true;
	}

	if ( any )
	{
		ConMsg( "\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConCommandBase *pVar )
{
	bool bMin, bMax;
	float fMin, fMax;
	const char *pStr;

	assert( pVar );

	Color clr;
	clr.SetColor( 255, 100, 100, 255 );

	if ( !pVar->IsCommand() )
	{
		ConVar *var = ( ConVar * )pVar;
		const ConVar_ServerBounded *pBounded = dynamic_cast<const ConVar_ServerBounded*>( var );

		bMin = var->GetMin( fMin );
		bMax = var->GetMax( fMax );

		const char *value = NULL;
		char tempVal[ 32 ];

		if ( pBounded || var->IsFlagSet( FCVAR_NEVER_AS_STRING ) )
		{
			value = tempVal;
			
			int intVal = pBounded ? pBounded->GetInt() : var->GetInt();
			float floatVal = pBounded ? pBounded->GetFloat() : var->GetFloat();

			if ( fabs( (float)intVal - floatVal ) < 0.000001 )
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%d", intVal );
			}
			else
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%f", floatVal );
			}
		}
		else
		{
			value = var->GetString();
		}

		if ( value )
		{
			ConColorMsg( clr, "\"%s\" = \"%s\"", var->GetName(), value );

			if ( stricmp( value, var->GetDefault() ) )
			{
				ConMsg( " ( def. \"%s\" )", var->GetDefault() );
			}
		}

		if ( bMin )
		{
			ConMsg( " min. %f", fMin );
		}
		if ( bMax )
		{
			ConMsg( " max. %f", fMax );
		}

		ConMsg( "\n" );

		// Handled virtualized cvars.
		if ( pBounded && fabs( pBounded->GetFloat() - var->GetFloat() ) > 0.0001f )
		{
			ConColorMsg( clr, "** NOTE: The real value is %.3f but the server has temporarily restricted it to %.3f **\n",
				var->GetFloat(), pBounded->GetFloat() );
		}
	}
	else
	{
		ConCommand *var = ( ConCommand * )pVar;

		ConColorMsg( clr, "\"%s\"\n", var->GetName() );
	}

	ConVar_PrintFlags( pVar );

	pStr = pVar->GetHelpText();
	if ( pStr && pStr[0] )
	{
		ConMsg( " - %s\n", pStr );
	}
}
