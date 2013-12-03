//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "ragdollexplosionenumerator.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Concussive explosion entity
//-----------------------------------------------------------------------------
class C_TEConcussiveExplosion : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEConcussiveExplosion, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	void			AffectRagdolls( void );

	Vector	m_vecNormal;
	float	m_flScale;
	int		m_nRadius;
	int		m_nMagnitude;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TEConcussiveExplosion, DT_TEConcussiveExplosion, CTEConcussiveExplosion )
	RecvPropVector( RECVINFO(m_vecNormal)),
	RecvPropFloat( RECVINFO(m_flScale)),
	RecvPropInt( RECVINFO(m_nRadius)),	
	RecvPropInt( RECVINFO(m_nMagnitude)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEConcussiveExplosion::AffectRagdolls( void )
{
	if ( ( m_nRadius == 0 ) || ( m_nMagnitude == 0 ) )
		return;

	CRagdollExplosionEnumerator	ragdollEnum( m_vecOrigin, m_nRadius, m_nMagnitude );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_RESPONSIVE_EDICTS, m_vecOrigin, m_nRadius, false, &ragdollEnum );
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordConcussiveExplosion( const Vector& start, const Vector &vecDirection )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_CONCUSSIVE_EXPLOSION );
 		msg->SetString( "name", "TE_ConcussiveExplosion" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", vecDirection.x );
		msg->SetFloat( "directiony", vecDirection.y );
		msg->SetFloat( "directionz", vecDirection.z );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEConcussiveExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	AffectRagdolls();

	FX_ConcussiveExplosion( m_vecOrigin, m_vecNormal );
	RecordConcussiveExplosion( m_vecOrigin, m_vecNormal );
}
						  
void TE_ConcussiveExplosion( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecDirection;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	vecDirection.x = pKeyValues->GetFloat( "directionx" );
	vecDirection.y = pKeyValues->GetFloat( "directiony" );
	vecDirection.z = pKeyValues->GetFloat( "directionz" );
	FX_ConcussiveExplosion( vecOrigin, vecDirection );
}