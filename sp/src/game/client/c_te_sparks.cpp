//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_te_particlesystem.h"
#include "IEffects.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Sparks TE
//-----------------------------------------------------------------------------
class C_TESparks : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TESparks, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TESparks( void );
	virtual			~C_TESparks( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual void	Precache( void );

	int m_nMagnitude;
	int m_nTrailLength;
	Vector m_vecDir;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESparks::C_TESparks( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESparks::~C_TESparks( void )
{
}

void C_TESparks::Precache( void )
{
}


//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
static inline void RecordSparks( const Vector &start, int nMagnitude, int nTrailLength, const Vector &direction )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_SPARKS );
 		msg->SetString( "name", "TE_Sparks" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );
		msg->SetInt( "magnitude", nMagnitude );
		msg->SetInt( "traillength", nTrailLength );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TESparks::PostDataUpdate( DataUpdateType_t updateType )
{
	g_pEffects->Sparks( m_vecOrigin, m_nMagnitude, m_nTrailLength, &m_vecDir );
	RecordSparks( m_vecOrigin, m_nMagnitude, m_nTrailLength, m_vecDir );
}

void TE_Sparks( IRecipientFilter& filter, float delay,
	const Vector* pos, int nMagnitude, int nTrailLength, const Vector *pDir )
{
	g_pEffects->Sparks( *pos, nMagnitude, nTrailLength, pDir );
	RecordSparks( *pos, nMagnitude, nTrailLength, *pDir );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TESparks, DT_TESparks, CTESparks)
	RecvPropInt( RECVINFO( m_nMagnitude ) ),
	RecvPropInt( RECVINFO( m_nTrailLength ) ),
	RecvPropVector( RECVINFO( m_vecDir ) ),
END_RECV_TABLE()
