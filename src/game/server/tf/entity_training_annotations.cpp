//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTrainingAnnotation Entity.  This entity is used to place
//          annotations on maps.
//=============================================================================//

#include "cbase.h"
#include "entity_training_annotations.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Show an anotation in 3D space in the hud to point out things of
//			interest to the player.
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTrainingAnnotation )
	DEFINE_KEYFIELD( m_displayText, FIELD_STRING, "display_text" ),
	DEFINE_KEYFIELD( m_flLifetime,  FIELD_FLOAT, "lifetime" ),
	DEFINE_KEYFIELD( m_flVerticalOffset,  FIELD_FLOAT, "offset" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Show", InputShow ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( training_annotation, CTrainingAnnotation );

CTrainingAnnotation::CTrainingAnnotation()
	: m_flLifetime(1.0f),
	  m_flVerticalOffset(0.0f)
{

}

void CTrainingAnnotation::Show()
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "show_annotation" );
	if ( pEvent )
	{
		Vector location = GetAbsOrigin();

		pEvent->SetString( "text", STRING( m_displayText ) );
		pEvent->SetInt( "id", (long)this );
		pEvent->SetFloat( "worldPosX", location.x );
		pEvent->SetFloat( "worldPosY", location.y );
		pEvent->SetFloat( "worldPosZ", location.z + m_flVerticalOffset );
		pEvent->SetFloat( "lifetime", m_flLifetime );
		pEvent->SetInt( "follow_entindex", 0 );

		gameeventmanager->FireEvent( pEvent );
	}
}

void CTrainingAnnotation::Hide()
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "hide_annotation" );
	if ( pEvent )
	{
		pEvent->SetInt( "id", (long)this );
		gameeventmanager->FireEventClientSide( pEvent );
	}
}


