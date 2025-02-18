//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "actanimating.h"
#include "animation.h"
#include "activitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CActAnimating )
	DEFINE_CUSTOM_FIELD( m_Activity, ActivityDataOps() ),
END_DATADESC()


void CActAnimating::SetActivity( Activity act ) 
{ 
	int sequence = SelectWeightedSequence( act ); 
	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		ResetSequence( sequence );
		m_Activity = act; 
		SetCycle( 0 );
	}
}

