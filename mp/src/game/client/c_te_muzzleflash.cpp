//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Muzzle flash temp ent
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "IEffects.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: User Tracer TE
//-----------------------------------------------------------------------------
class C_TEMuzzleFlash : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEMuzzleFlash, C_BaseTempEntity );
	
	DECLARE_CLIENTCLASS();

					C_TEMuzzleFlash( void );
	virtual			~C_TEMuzzleFlash( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector		m_vecOrigin;
	QAngle		m_vecAngles;
	float		m_flScale;
	int			m_nType;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEMuzzleFlash::C_TEMuzzleFlash( void )
{
	m_vecOrigin.Init();
	m_vecAngles.Init();
	m_flScale	= 1.0f;
	m_nType		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEMuzzleFlash::~C_TEMuzzleFlash( void )
{
}

//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
static inline void RecordMuzzleFlash( const Vector &start, const QAngle &angles, float scale, int type )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_MUZZLE_FLASH );
 		msg->SetString( "name", "TE_MuzzleFlash" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "anglesx", angles.x );
		msg->SetFloat( "anglesy", angles.y );
		msg->SetFloat( "anglesz", angles.z );
		msg->SetFloat( "scale", scale );
		msg->SetInt( "type", type );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEMuzzleFlash::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEMuzzleFlash::PostDataUpdate" );

	//FIXME: Index is incorrect
	g_pEffects->MuzzleFlash( m_vecOrigin, m_vecAngles, m_flScale, m_nType );	
	RecordMuzzleFlash( m_vecOrigin, m_vecAngles, m_flScale, m_nType ); 
}

void TE_MuzzleFlash( IRecipientFilter& filter, float delay,
	const Vector &start, const QAngle &angles, float scale, int type )
{
	g_pEffects->MuzzleFlash( start, angles, scale, 0 );	
	RecordMuzzleFlash( start, angles, scale, 0 ); 
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEMuzzleFlash, DT_TEMuzzleFlash, CTEMuzzleFlash)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecAngles)),
	RecvPropFloat( RECVINFO(m_flScale)),
	RecvPropInt( RECVINFO(m_nType)),
END_RECV_TABLE()
