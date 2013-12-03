//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "game.h"
#include "ai_looktarget.h"

// Mothballing this entity to get rid of it. info_hint does its job better (sjb)
//LINK_ENTITY_TO_CLASS( ai_looktarget, CAI_LookTarget );

BEGIN_DATADESC( CAI_LookTarget )

	// Keyfields
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iContext, FIELD_INTEGER, "context" ),
	DEFINE_KEYFIELD( m_iPriority, FIELD_INTEGER, "priority" ),
	DEFINE_KEYFIELD( m_flMaxDist, FIELD_FLOAT, "maxdist" ),

	// Fields
	DEFINE_FIELD( m_flTimeNextAvailable, FIELD_TIME ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
int CAI_LookTarget::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_BBOX_BIT)
	{
		int color = random->RandomInt( 50, 255 );
		NDebugOverlay::Cross3D( GetAbsOrigin(), 12, color, color, color, false, 0.1 );
	}

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		if( !IsEnabled() )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"DISABLED" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		if( IsEligible( NULL ) )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Eligible" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"NOT Eligible for selection");
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}
	}
	return text_offset;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_LookTarget::IsEligible( CBaseEntity *pLooker )
{
	if( !IsEnabled() )
		return false;

	if( !IsAvailable() )
		return false;

	if( pLooker )
	{
		float maxdistsqr = m_flMaxDist * m_flMaxDist;
		
		Vector vecPos = GetAbsOrigin();

		if( vecPos.DistToSqr( pLooker->WorldSpaceCenter() ) > maxdistsqr )
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------
// Someone's reserving this entity because they're going 
// to attempt to look at it for flDuration seconds. We'll
// make it unavailable to anyone else for that time.
//---------------------------------------------------------
void CAI_LookTarget::Reserve( float flDuration )
{
	m_flTimeNextAvailable = gpGlobals->curtime + flDuration;

	if( HasSpawnFlags( SF_LOOKTARGET_ONLYONCE ) )
	{
		// No one will look at this again.
		Disable();
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
CAI_LookTarget *CAI_LookTarget::GetFirstLookTarget()
{
	CBaseEntity		*pEnt;

	string_t iszLookTarget = FindPooledString( "ai_looktarget" );
	if( iszLookTarget == NULL_STRING )
	{
		return NULL;
	}

	pEnt = gEntList.FirstEnt();
	while( pEnt && pEnt->m_iClassname != iszLookTarget )
	{
		pEnt = gEntList.NextEnt( pEnt );
	}

	return (CAI_LookTarget*)pEnt;
}

//---------------------------------------------------------
//---------------------------------------------------------
CAI_LookTarget *CAI_LookTarget::GetNextLookTarget( CAI_LookTarget *pCurrentTarget )
{
	CBaseEntity		*pEnt;

	string_t iszLookTarget = FindPooledString( "ai_looktarget" );
	if( iszLookTarget == NULL_STRING )
	{
		return NULL;
	}

	pEnt = gEntList.NextEnt( pCurrentTarget );
	while( pEnt && pEnt->m_iClassname != iszLookTarget )
	{
		pEnt = gEntList.NextEnt( pEnt );
	}

	return (CAI_LookTarget*)pEnt;
}
