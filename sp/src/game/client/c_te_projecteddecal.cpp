//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "iefx.h"
#include "engine/IStaticPropMgr.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// UNDONE:  Get rid of this?
#define FDECAL_PERMANENT			0x01

//-----------------------------------------------------------------------------
// Purpose: Projected Decal TE
//-----------------------------------------------------------------------------
class C_TEProjectedDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEProjectedDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEProjectedDecal( void );
	virtual			~C_TEProjectedDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecOrigin;
	QAngle			m_angRotation;
	float			m_flDistance;
	int				m_nIndex;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEProjectedDecal, DT_TEProjectedDecal, CTEProjectedDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropQAngles( RECVINFO( m_angRotation )),
	RecvPropFloat( RECVINFO(m_flDistance)),
	RecvPropInt( RECVINFO(m_nIndex)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEProjectedDecal::C_TEProjectedDecal( void )
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_flDistance = 0.0f;
	m_nIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEProjectedDecal::~C_TEProjectedDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEProjectedDecal::Precache( void )
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordProjectDecal( const Vector &pos, const QAngle &angles, 
	float flDistance, int index )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_PROJECT_DECAL );
 		msg->SetString( "name", "TE_ProjectDecal" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", pos.x );
		msg->SetFloat( "originy", pos.y );
		msg->SetFloat( "originz", pos.z );
		msg->SetFloat( "anglesx", angles.x );
		msg->SetFloat( "anglesy", angles.y );
		msg->SetFloat( "anglesz", angles.z );
		msg->SetFloat( "distance", flDistance );
		msg->SetString( "decalname", effects->Draw_DecalNameFromIndex( index ) );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}

void TE_ProjectDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle *angles, float distance, int index )
{
	RecordProjectDecal( *pos, *angles, distance, index );

	trace_t	tr;

	Vector fwd;
	AngleVectors( *angles, &fwd );

	Vector endpos;
	VectorMA( *pos, distance, fwd, endpos );

	CTraceFilterHitAll traceFilter;
	UTIL_TraceLine( *pos, endpos, MASK_ALL, &traceFilter, &tr );

	if ( tr.fraction == 1.0f )
	{
		return;
	}

	C_BaseEntity* ent = tr.m_pEnt;
	Assert( ent );

	int hitbox = tr.hitbox;

	if ( tr.hitbox != 0 )
	{
		staticpropmgr->AddDecalToStaticProp( *pos, endpos, hitbox - 1, index, false, tr );
	}
	else
	{
		// Only decal the world + brush models
		ent->AddDecal( *pos, endpos, endpos, hitbox, 
			index, false, tr );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEProjectedDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEProjectedDecal::PostDataUpdate" );

	CBroadcastRecipientFilter filter;
	TE_ProjectDecal( filter, 0.0f, &m_vecOrigin, &m_angRotation, m_flDistance, m_nIndex );
}


//-----------------------------------------------------------------------------
// Playback
//-----------------------------------------------------------------------------
void TE_ProjectDecal( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin;
	QAngle angles;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	angles.x = pKeyValues->GetFloat( "anglesx" );
	angles.y = pKeyValues->GetFloat( "anglesy" );
	angles.z = pKeyValues->GetFloat( "anglesz" );
	float flDistance = pKeyValues->GetFloat( "distance" );
	const char *pDecalName = pKeyValues->GetString( "decalname" );

	TE_ProjectDecal( filter, 0.0f, &vecOrigin, &angles, flDistance, effects->Draw_DecalIndexFromName( (char*)pDecalName ) );
}

