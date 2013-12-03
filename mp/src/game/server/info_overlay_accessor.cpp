//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// An entity used to access overlays (and change their texture)
// -------------------------------------------------------------------------------- //

class CInfoOverlayAccessor : public CPointEntity
{
public:

	DECLARE_CLASS( CInfoOverlayAccessor, CPointEntity );

	int  	UpdateTransmitState();

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:

	CNetworkVar( int, m_iOverlayID );
};
							  

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE(CInfoOverlayAccessor, DT_InfoOverlayAccessor)
	SendPropInt	(	SENDINFO(m_iTextureFrameIndex),		8,	SPROP_UNSIGNED ),
	SendPropInt	(	SENDINFO(m_iOverlayID),				32,	SPROP_UNSIGNED ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( info_overlay_accessor, CInfoOverlayAccessor );

BEGIN_DATADESC( CInfoOverlayAccessor )
	DEFINE_KEYFIELD( m_iOverlayID,	FIELD_INTEGER, "OverlayID" ),
END_DATADESC()


int CInfoOverlayAccessor::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
