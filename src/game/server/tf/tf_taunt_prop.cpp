//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Play VCD on taunt prop
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "basecombatcharacter.h"
#include "choreoevent.h"
#include "sceneentity.h"

#include "tf_taunt_prop.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST( CTFTauntProp, DT_TFTauntProp )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_taunt_prop, CTFTauntProp );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTauntProp::CTFTauntProp()
	: m_bAutoRemove( false )
{
	UseClientSideAnimation();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTauntProp::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
	{
		// Get the (gesture) sequence.
		info->m_nSequence = LookupSequence( event->GetParameters() );
		if ( info->m_nSequence < 0 )
			return false;

		SetSequence( info->m_nSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();

		if ( IsUsingClientSideAnimation() )
		{
			ResetClientsideFrame();
		}

		return true;
	}
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTauntProp::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	// Only process sequences
	if ( event->GetType() != CChoreoEvent::SEQUENCE )
		return false;

	return BaseClass::ProcessSceneEvent( info, scene, event );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFTauntProp::PlayScene( const char *pszScene, float flDelay /*= 0.0f*/, AI_Response *response /*= NULL*/, IRecipientFilter *filter /*= NULL*/ )
{
	if ( m_hScene.Get() )
	{
		StopScriptedScene( this, m_hScene );
		m_hScene = NULL;
	}

	MDLCACHE_CRITICAL_SECTION();

	float flDuration = InstancedScriptedScene( this, pszScene, &m_hScene, flDelay, false, response, true, filter );

	// Remove this at the end of the scene if needed
	if ( m_bAutoRemove )
	{
		SetThink( &BaseClass::SUB_Remove );
		SetNextThink( gpGlobals->curtime + flDuration );
	}

	return flDuration;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTauntProp::UpdateOnRemove()
{
	if ( m_hScene.Get() )
	{
		StopScriptedScene( this, m_hScene );
		m_hScene = NULL;
	}
		
	BaseClass::UpdateOnRemove();
}
