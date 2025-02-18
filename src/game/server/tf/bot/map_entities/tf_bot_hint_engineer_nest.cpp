//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_bot_hint_engineer_nest.h"
#include "tf_obj.h"
#include "tf_obj_teleporter.h"

IMPLEMENT_SERVERCLASS_ST( CTFBotHintEngineerNest, DT_TFBotHintEngineerNest )
	SendPropBool( SENDINFO(m_bHasActiveTeleporter) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFBotHintEngineerNest )
END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_hint_engineer_nest, CTFBotHintEngineerNest );

//------------------------------------------------------------------------------
CTFBotHintEngineerNest::CTFBotHintEngineerNest( void )
{
	m_bHasActiveTeleporter = false;
}


void CTFBotHintEngineerNest::Spawn()
{
	BaseClass::Spawn();

	SetThink( &CTFBotHintEngineerNest::HintThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTFBotHintEngineerNest::HintThink()
{
	// find sentry and teleporter hint
	for ( int i=0; i<ITFBotHintEntityAutoList::AutoList().Count(); ++i )
	{
		CBaseTFBotHintEntity *pHint = static_cast< CBaseTFBotHintEntity* >( ITFBotHintEntityAutoList::AutoList()[i] );
		if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_SENTRYGUN ) && pHint->GetEntityName() == GetEntityName() )
		{
			m_sentries.AddToTail( pHint );
		}
		else if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_TELEPORTER_EXIT ) && pHint->GetEntityName() == GetEntityName() )
		{
			m_teleporters.AddToTail( pHint );
		}
	}

	if ( m_sentries.Count() == 0 && m_teleporters.Count() == 0 )
	{
		AssertMsg( 0, "Must have a teleporter and/or a sentry hint with the same name." );
		Warning( "Must have a teleporter and/or a sentry hint with the same name.\n" );
	}

	SetThink( &CTFBotHintEngineerNest::HintTeleporterThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTFBotHintEngineerNest::HintTeleporterThink()
{
	bool bFoundActiveTeleporter = false;
	for ( int i=0; i<m_teleporters.Count(); ++i )
	{
		CBaseEntity* pOwner = m_teleporters[i]->GetOwnerEntity();
		if ( pOwner && pOwner->IsBaseObject() )
		{
			CObjectTeleporter *pTeleporter = assert_cast< CObjectTeleporter* >( pOwner );
			if ( pTeleporter )
			{
				bFoundActiveTeleporter |= !pTeleporter->IsBuilding();
			}
		}
	}

	// update particle bool
	m_bHasActiveTeleporter = bFoundActiveTeleporter;

	SetNextThink( gpGlobals->curtime + 0.1f );
}


bool CTFBotHintEngineerNest::IsStaleNest() const
{
	for ( int i=0; i<m_sentries.Count(); ++i )
	{
		if ( m_sentries[i]->OwnerObjectHasNoOwner() )
		{
			return true;
		}
	}

	for ( int i=0; i<m_teleporters.Count(); ++i )
	{
		if ( m_teleporters[i]->OwnerObjectHasNoOwner() )
		{
			return true;
		}
	}

	return false;
}


void CTFBotHintEngineerNest::DetonateStaleNest()
{
	DetonateObjectsFromHints( m_sentries );
	DetonateObjectsFromHints( m_teleporters );
}


void CTFBotHintEngineerNest::DetonateObjectsFromHints( const HintVector_t& hints )
{
	for ( int i=0; i<hints.Count(); ++i )
	{
		if ( hints[i]->OwnerObjectHasNoOwner() )
		{
			CBaseObject* pObj = assert_cast< CBaseObject* >( hints[i]->GetOwnerEntity() );
			if ( pObj )
			{
				pObj->DetonateObject();
			}
		}
	}
}


CBaseTFBotHintEntity* CTFBotHintEngineerNest::GetHint( const HintVector_t& hints ) const
{
	if ( hints.Count() == 0 )
	{
		return NULL;
	}

	for ( int i=0; i<hints.Count(); ++i )
	{
		if ( hints[i]->OwnerObjectHasNoOwner() )
		{
			return hints[i];
		}
	}

	int which = RandomInt( 0, hints.Count() - 1 );
	return hints[ which ];
}


CTFBotHintSentrygun* CTFBotHintEngineerNest::GetSentryHint() const
{
	return (CTFBotHintSentrygun*)GetHint( m_sentries );
}


CTFBotHintTeleporterExit* CTFBotHintEngineerNest::GetTeleporterHint() const
{
	return (CTFBotHintTeleporterExit*)GetHint( m_teleporters );
}
