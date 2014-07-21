//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Sprite Spray TE
//-----------------------------------------------------------------------------
class C_TESpriteSpray : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TESpriteSpray, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TESpriteSpray( void );
	virtual			~C_TESpriteSpray( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	Vector			m_vecDirection;
	int				m_nModelIndex;
	int				m_nSpeed;
	float			m_fNoise;
	int				m_nCount;
};


//-----------------------------------------------------------------------------
// Networking 
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TESpriteSpray, DT_TESpriteSpray, CTESpriteSpray)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecDirection)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_fNoise )),
	RecvPropInt( RECVINFO(m_nCount)),
	RecvPropInt( RECVINFO(m_nSpeed)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESpriteSpray::C_TESpriteSpray( void )
{
	m_vecOrigin.Init();
	m_vecDirection.Init();
	m_nModelIndex = 0;
	m_fNoise = 0;
	m_nSpeed = 0;
	m_nCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESpriteSpray::~C_TESpriteSpray( void )
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordSpriteSpray( const Vector& start, const Vector &direction, 
	int nModelIndex, int nSpeed, float flNoise, int nCount )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		const model_t* pModel = (nModelIndex != 0) ? modelinfo->GetModel( nModelIndex ) : NULL;
		const char *pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_SPRITE_SPRAY );
 		msg->SetString( "name", "TE_SpriteSpray" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );
  		msg->SetString( "model", pModelName );
 		msg->SetInt( "speed", nSpeed );
 		msg->SetFloat( "noise", flNoise );
 		msg->SetInt( "count", nCount );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TESpriteSpray::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TESpriteSpray::PostDataUpdate" );

	tempents->Sprite_Spray( m_vecOrigin, m_vecDirection, m_nModelIndex, m_nCount, m_nSpeed * 0.2, m_fNoise * 100.0 );
	RecordSpriteSpray( m_vecOrigin, m_vecDirection, m_nModelIndex, m_nSpeed, m_fNoise, m_nCount );
}

void TE_SpriteSpray( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir, int modelindex, int speed, float noise, int count )
{
	tempents->Sprite_Spray( *pos, *dir, modelindex, count, speed * 0.2, noise * 100.0 );
	RecordSpriteSpray( *pos, *dir, modelindex, speed, noise, count );
}

void TE_SpriteSpray( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecDirection;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	vecDirection.x = pKeyValues->GetFloat( "directionx" );
	vecDirection.y = pKeyValues->GetFloat( "directiony" );
	vecDirection.z = pKeyValues->GetFloat( "directionz" );
	const char *pModelName = pKeyValues->GetString( "model" );
	int nModelIndex = pModelName[0] ? modelinfo->GetModelIndex( pModelName ) : 0;
	int nSpeed = pKeyValues->GetInt( "speed" );
	float flNoise = pKeyValues->GetFloat( "noise" );
	int nCount = pKeyValues->GetInt( "count" );

	TE_SpriteSpray( filter, delay, &vecOrigin, &vecDirection, nModelIndex, nSpeed, flNoise, nCount );
}

