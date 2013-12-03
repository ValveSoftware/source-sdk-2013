//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
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
// Purpose: Glow Sprite TE
//-----------------------------------------------------------------------------
class C_TEGlowSprite : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEGlowSprite, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEGlowSprite( void );
	virtual			~C_TEGlowSprite( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	int				m_nModelIndex;
	float			m_fScale;
	float			m_fLife;
	int				m_nBrightness;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEGlowSprite, DT_TEGlowSprite, CTEGlowSprite)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_fScale )),
	RecvPropFloat( RECVINFO(m_fLife )),
	RecvPropInt( RECVINFO(m_nBrightness)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEGlowSprite::C_TEGlowSprite( void )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_fLife = 0;
	m_nBrightness = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEGlowSprite::~C_TEGlowSprite( void )
{
}

//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordGlowSprite( const Vector &start, int nModelIndex, 
	float flDuration, float flSize, int nBrightness )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		const model_t* pModel = (nModelIndex != 0) ? modelinfo->GetModel( nModelIndex ) : NULL;
		const char *pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_GLOW_SPRITE );
 		msg->SetString( "name", "TE_GlowSprite" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
  		msg->SetString( "model", pModelName );
		msg->SetFloat( "duration", flDuration );
		msg->SetFloat( "size", flSize );
		msg->SetInt( "brightness", nBrightness );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEGlowSprite::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEGlowSprite::PostDataUpdate" );

	float a = ( 1.0 / 255.0 ) * m_nBrightness;
	C_LocalTempEntity *ent = tempents->TempSprite( m_vecOrigin, vec3_origin, m_fScale, m_nModelIndex, kRenderTransAdd, 0, a, m_fLife, FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP );
	if ( ent )
	{
		ent->bounceFactor = 0.2;
	}
	RecordGlowSprite( m_vecOrigin, m_nModelIndex, m_fLife, m_fScale, m_nBrightness );
}

void TE_GlowSprite( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float life, float size, int brightness )
{
	float a = ( 1.0 / 255.0 ) * brightness;
	C_LocalTempEntity *ent = tempents->TempSprite( *pos, vec3_origin, size, modelindex, kRenderTransAdd, 0, a, life, FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP );
	if ( ent )
	{
		ent->bounceFactor = 0.2;
	}
	RecordGlowSprite( *pos, modelindex, life, size, brightness );
}

void TE_GlowSprite( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	const char *pModelName = pKeyValues->GetString( "model" );
	int nModelIndex = pModelName[0] ? modelinfo->GetModelIndex( pModelName ) : 0;
	float flDuration = pKeyValues->GetFloat( "duration" );
	float flSize = pKeyValues->GetFloat( "size" );
	int nBrightness = pKeyValues->GetFloat( "brightness" );

	TE_GlowSprite( filter, delay, &vecOrigin, nModelIndex, flDuration, flSize, nBrightness );
}

