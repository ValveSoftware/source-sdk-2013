//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Play VCD on taunt prop
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "c_basecombatcharacter.h"
#include "choreoevent.h"
#include "c_sceneentity.h"

#include "c_tf_taunt_prop.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFTauntProp, DT_TFTauntProp, CTFTauntProp )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFTauntProp::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
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
			SetCycle( scene->GetTime() / scene->GetDuration() );
		}
		return true;
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFTauntProp::ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled )
{
	switch ( info->m_pEvent->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		//return StopGestureSceneEvent( info, fastKill, canceled );
	default:
		return BaseClass::ClearSceneEvent( info, fastKill, canceled );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFTauntProp::UpdateOnRemove()
{
	ParticleProp()->StopEmissionAndDestroyImmediately();
	BaseClass::UpdateOnRemove();
}