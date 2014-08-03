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
#include "c_te_legacytempents.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Breakable Model TE
//-----------------------------------------------------------------------------
class C_TEBreakModel : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBreakModel, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBreakModel( void );
	virtual			~C_TEBreakModel( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	QAngle			m_angRotation;
	Vector			m_vecSize;
	Vector			m_vecVelocity;
	int				m_nRandomization;
	int				m_nModelIndex;
	int				m_nCount;
	float			m_fTime;
	int				m_nFlags;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBreakModel, DT_TEBreakModel, CTEBreakModel)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropFloat( RECVINFO( m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO( m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO( m_angRotation[2] ) ),
	RecvPropVector( RECVINFO(m_vecSize)),
	RecvPropVector( RECVINFO(m_vecVelocity)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nRandomization)),
	RecvPropInt( RECVINFO(m_nCount)),
	RecvPropFloat( RECVINFO(m_fTime)),
	RecvPropInt( RECVINFO(m_nFlags)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBreakModel::C_TEBreakModel( void )
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_vecSize.Init();
	m_vecVelocity.Init();
	m_nModelIndex		= 0;
	m_nRandomization	= 0;
	m_nCount			= 0;
	m_fTime				= 0.0;
	m_nFlags			= 0;
}

C_TEBreakModel::~C_TEBreakModel( void )
{
}


//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
static inline void RecordBreakModel( const Vector &start, const QAngle &angles, const Vector &size,
	const Vector &vel, int nModelIndex, int nRandomization, int nCount, float flDuration, int nFlags )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		const model_t* pModel = (nModelIndex != 0) ? modelinfo->GetModel( nModelIndex ) : NULL;
		const char *pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_BREAK_MODEL );
 		msg->SetString( "name", "TE_BreakModel" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "anglesx", angles.x );
		msg->SetFloat( "anglesy", angles.y );
		msg->SetFloat( "anglesz", angles.z );
		msg->SetFloat( "sizex", size.x );
		msg->SetFloat( "sizey", size.y );
		msg->SetFloat( "sizez", size.z );
		msg->SetFloat( "velx", vel.x );
		msg->SetFloat( "vely", vel.y );
		msg->SetFloat( "velz", vel.z );
  		msg->SetString( "model", pModelName );
		msg->SetInt( "randomization", nRandomization );
		msg->SetInt( "count", nCount );
		msg->SetFloat( "duration", flDuration );
		msg->SetInt( "flags", nFlags );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_BreakModel( IRecipientFilter& filter, float delay,
	const Vector& pos, const QAngle &angles, const Vector& size, const Vector& vel, 
	int modelindex, int randomization, int count, float time, int flags )
{
	tempents->BreakModel( pos, angles, size, vel, randomization, time, count, modelindex, flags );
	RecordBreakModel( pos, angles, size, vel, randomization, time, count, modelindex, flags ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEBreakModel::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEBreakModel::PostDataUpdate" );

	tempents->BreakModel( m_vecOrigin, m_angRotation, m_vecSize, m_vecVelocity,
		m_nRandomization, m_fTime, m_nCount, m_nModelIndex, m_nFlags );
	RecordBreakModel( m_vecOrigin, m_angRotation, m_vecSize, m_vecVelocity,
		m_nRandomization, m_fTime, m_nCount, m_nModelIndex, m_nFlags ); 
}

void TE_BreakModel( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecSize, vecVel;
	QAngle angles;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	angles.x = pKeyValues->GetFloat( "anglesx" );
	angles.y = pKeyValues->GetFloat( "anglesy" );
	angles.z = pKeyValues->GetFloat( "anglesz" );
	vecSize.x = pKeyValues->GetFloat( "sizex" );
	vecSize.y = pKeyValues->GetFloat( "sizey" );
	vecSize.z = pKeyValues->GetFloat( "sizez" );
	vecVel.x = pKeyValues->GetFloat( "velx" );
	vecVel.y = pKeyValues->GetFloat( "vely" );
	vecVel.z = pKeyValues->GetFloat( "velz" );
	Color c = pKeyValues->GetColor( "color" );
	const char *pModelName = pKeyValues->GetString( "model" );
	int nModelIndex = pModelName[0] ? modelinfo->GetModelIndex( pModelName ) : 0;
	int nRandomization = pKeyValues->GetInt( "randomization" );
	int nCount = pKeyValues->GetInt( "count" );
	float flDuration = pKeyValues->GetFloat( "duration" );
	int nFlags = pKeyValues->GetInt( "flags" );
	TE_BreakModel( filter, 0.0f, vecOrigin, angles, vecSize, vecVel,
		nModelIndex, nRandomization, nCount, flDuration, nFlags );
}

