//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "func_areaportalbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// A sphere around the player used for backface culling of areaportals.
#define VIEWER_PADDING	80


CUtlLinkedList<CFuncAreaPortalBase*, unsigned short> g_AreaPortals;



//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CFuncAreaPortalBase )

	DEFINE_FIELD( m_portalNumber,			FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iPortalVersion,		FIELD_INTEGER, "PortalVersion" )
//	DEFINE_FIELD( m_AreaPortalsElement,		FIELD_SHORT ),

END_DATADESC()




CFuncAreaPortalBase::CFuncAreaPortalBase()
{
	m_portalNumber = -1;
	m_AreaPortalsElement = g_AreaPortals.AddToTail( this );
	m_iPortalVersion = 0;
}


CFuncAreaPortalBase::~CFuncAreaPortalBase()
{
	g_AreaPortals.Remove( m_AreaPortalsElement );
}


bool CFuncAreaPortalBase::UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient )
{
	// NOTE: We leave bIsOpenOnClient alone on purpose here. See the header for a description of why.
	
	if( m_portalNumber == -1 )
		return false;

	// See if the viewer is on the backside.
	VPlane plane;
	if( !engine->GetAreaPortalPlane( vOrigin, m_portalNumber, &plane ) )
		return true; // leave it open if there's an error here for some reason

	bool bOpen = false;
	if( plane.DistTo( vOrigin ) + VIEWER_PADDING > 0 )
		bOpen = true;

	return bOpen;
}




