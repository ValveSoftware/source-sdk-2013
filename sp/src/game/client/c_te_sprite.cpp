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
#include "tempent.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Sprite TE
//-----------------------------------------------------------------------------
class C_TESprite : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TESprite, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TESprite( void );
	virtual			~C_TESprite( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	int				m_nModelIndex;
	float			m_fScale;
	int				m_nBrightness;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TESprite, DT_TESprite, CTESprite)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_fScale )),
	RecvPropInt( RECVINFO(m_nBrightness)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESprite::C_TESprite( void )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_nBrightness = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TESprite::~C_TESprite( void )
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordSprite( const Vector& start, int nModelIndex, 
								 float flScale, int nBrightness )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		const model_t* pModel = (nModelIndex != 0) ? modelinfo->GetModel( nModelIndex ) : NULL;
		const char *pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_SPRITE_SINGLE );
 		msg->SetString( "name", "TE_Sprite" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
  		msg->SetString( "model", pModelName );
 		msg->SetFloat( "scale", flScale );
 		msg->SetInt( "brightness", nBrightness );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TESprite::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TESprite::PostDataUpdate" );

	float a = ( 1.0 / 255.0 ) * m_nBrightness;
	tempents->TempSprite( m_vecOrigin, vec3_origin, m_fScale, m_nModelIndex, kRenderTransAdd, 0, a, 0, FTENT_SPRANIMATE );
	RecordSprite( m_vecOrigin, m_nModelIndex, m_fScale, m_nBrightness );
}

void TE_Sprite( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float size, int brightness )
{
	float a = ( 1.0 / 255.0 ) * brightness;
	tempents->TempSprite( *pos, vec3_origin, size, modelindex, kRenderTransAdd, 0, a, 0, FTENT_SPRANIMATE );
	RecordSprite( *pos, modelindex, size, brightness );
}

void TE_Sprite( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecDirection;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	const char *pModelName = pKeyValues->GetString( "model" );
	int nModelIndex = pModelName[0] ? modelinfo->GetModelIndex( pModelName ) : 0;
	float flScale = pKeyValues->GetFloat( "scale" );
	int nBrightness = pKeyValues->GetInt( "brightness" );

	TE_Sprite( filter, delay, &vecOrigin, nModelIndex, flScale, nBrightness );
}
