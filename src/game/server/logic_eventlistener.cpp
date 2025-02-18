//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ====
//
// Purpose:		Player for HL2.
//
//=============================================================================

#include "cbase.h"
#include "logic_eventlistener.h"
#include "GameEventListener.h"
#include "igameevents.h"
#ifdef PORTAL2
#include "portal_mp_gamerules.h"
#endif // PORTAL2

LINK_ENTITY_TO_CLASS( logic_eventlistener, CLogicEventListener);

BEGIN_DATADESC( CLogicEventListener )

// Base
DEFINE_KEYFIELD( m_iszEventName, FIELD_STRING, "EventName" ),
DEFINE_KEYFIELD( m_bFetchEventData, FIELD_BOOLEAN, "FetchEventData" ),
DEFINE_KEYFIELD( m_bIsEnabled, FIELD_BOOLEAN, "IsEnabled" ),
DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "TeamNum" ),

// Inputs
DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

// Outputs
DEFINE_OUTPUT( m_OnEventFired, "OnEventFired" ),

END_DATADESC()


class CScriptEventTableWriter : public IGameEventVisitor2
{
public:
	CScriptEventTableWriter( IScriptVM* pVM, HSCRIPT table )
	: mVM(pVM), mTable(table)
	{}

	// IGameEventVisitor2
	virtual bool VisitString( const char* name, const char* value ) OVERRIDE
	{
		mVM->SetValue( mTable, name, value );
		return true;
	}

	virtual bool VisitFloat( const char* name, float value ) OVERRIDE
	{
		mVM->SetValue( mTable, name, value );
		return true;
	}

	virtual bool VisitInt( const char* name, int value ) OVERRIDE
	{
		mVM->SetValue( mTable, name, value );
		return true;
	}

	virtual bool VisitBool( const char*name, bool value ) OVERRIDE
	{
		mVM->SetValue( mTable, name, value );
		return true;
	}

	// TODO: Uint64, wstring?

private:
	IScriptVM* mVM;
	HSCRIPT mTable;
};

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CLogicEventListener::Spawn( void )
{
	BaseClass::Spawn();

	ListenForGameEvent( STRING(m_iszEventName) );
}

//-----------------------------------------------------------------------------
// Purpose: FireGameEvent
//-----------------------------------------------------------------------------
void CLogicEventListener::FireGameEvent( IGameEvent *event )
{
	if ( !m_bIsEnabled )
		return;

	const char *name = event->GetName();
	if ( Q_strcmp( name, STRING(m_iszEventName) ) == 0 )
	{
		if ( m_nTeam > 0 )
		{
			int nPlayerTeam = TEAM_INVALID;
			int	playerId = event->GetInt( "userid", 0 );
			for ( int i = 0; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( pPlayer && pPlayer->GetUserID() == playerId )
				{
					nPlayerTeam = pPlayer->GetTeamNumber();
					break;
				}
			}

			if ( nPlayerTeam != m_nTeam )
				return;
		}

		if ( m_bFetchEventData && ValidateScriptScope() )
		{
			HSCRIPT entityTable = m_ScriptScope;
			IScriptVM* vm = m_ScriptScope.GetVM();

			ScriptVariant_t table;
			vm->CreateTable( table );
			{
				CScriptEventTableWriter tableWriter( vm, table );
				event->ForEventData( &tableWriter );
			}
			vm->SetValue( entityTable, "event_data", table );
			vm->ReleaseValue( table );
		}

		m_OnEventFired.FireOutput( NULL, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLogicEventListener::InputEnable( inputdata_t &inputdata )
{
	m_bIsEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLogicEventListener::InputDisable( inputdata_t &inputdata )
{
	m_bIsEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: an event listener that listens specifically for item_equip and sends an output if the weapon matches
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( logic_eventlistener_itemequip, CLogicEventListenerItemEquip);

BEGIN_DATADESC( CLogicEventListenerItemEquip )

	// Base
	DEFINE_KEYFIELD( m_bIsEnabled, FIELD_BOOLEAN, "IsEnabled" ),
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "TeamNum" ),
	DEFINE_KEYFIELD( m_szWeaponClassname, FIELD_STRING, "WeaponClassname" ),
	DEFINE_KEYFIELD( m_nWeaponType, FIELD_INTEGER, "WeaponType" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs
	DEFINE_OUTPUT( m_OnEventFired,		"OnEventFired" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CLogicEventListenerItemEquip::Spawn( void )
{
	BaseClass::Spawn();

	ListenForGameEvent( "item_equip" );
}

//-----------------------------------------------------------------------------
// Purpose: FireGameEvent
//-----------------------------------------------------------------------------
void CLogicEventListenerItemEquip::FireGameEvent( IGameEvent *event )
{
	if ( !m_bIsEnabled )
		return;

	const char *name = event->GetName();
	if ( Q_strcmp( name, "item_equip" ) == 0 )
	{
		if ( m_nTeam > 0 )
		{
			int nPlayerTeam = TEAM_INVALID;
			int	playerId = event->GetInt( "userid", 0 );
			for ( int i = 0; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( pPlayer && pPlayer->GetUserID() == playerId )
				{
					nPlayerTeam = pPlayer->GetTeamNumber();
					break;
				}
			}

			if ( nPlayerTeam != m_nTeam )
				return;
		}

		if ( m_szWeaponClassname != NULL_STRING )
		{
			if ( Q_strcmp( m_szWeaponClassname.ToCStr(), event->GetString( "item", "" ) ) != 0 )
				return;
		}

		if ( m_nWeaponType > -1 )
		{
			int	nWepType = event->GetInt( "weptype", 0 );
			if ( nWepType != m_nWeaponType )
				return;
		}

		m_OnEventFired.FireOutput( NULL, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLogicEventListenerItemEquip::InputEnable( inputdata_t &inputdata )
{
	m_bIsEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLogicEventListenerItemEquip::InputDisable( inputdata_t &inputdata )
{
	m_bIsEnabled = false;
}

