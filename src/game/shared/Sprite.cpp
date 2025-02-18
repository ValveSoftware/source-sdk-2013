//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "Sprite.h"
#include "model_types.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"

#if defined( CLIENT_DLL )
	#include "enginesprite.h"
	#include "iclientmode.h"
	#include "c_baseviewmodel.h"
#	ifdef PORTAL
		#include "c_prop_portal.h"
#	endif //ifdef PORTAL
#else
	#include "baseviewmodel.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float MAX_SPRITE_SCALE = 64.0f;
const float MAX_GLOW_PROXY_SIZE = 64.0f;

LINK_ENTITY_TO_CLASS( env_sprite, CSprite );
LINK_ENTITY_TO_CLASS( env_sprite_oriented, CSpriteOriented );
#if !defined( CLIENT_DLL )
LINK_ENTITY_TO_CLASS( env_glow, CSprite ); // For backwards compatibility, remove when no longer needed.
#endif

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CSprite )

	DEFINE_FIELD( m_flLastTime, FIELD_TIME ),
	DEFINE_FIELD( m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_hAttachedToEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_flDieTime, FIELD_TIME ),

	DEFINE_FIELD( m_nBrightness,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flBrightnessTime,	FIELD_FLOAT ),

	DEFINE_KEYFIELD( m_flSpriteScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_flSpriteFramerate, FIELD_FLOAT, "framerate" ),
	DEFINE_KEYFIELD( m_flFrame, FIELD_FLOAT, "frame" ),
#ifdef PORTAL
	DEFINE_FIELD( m_bDrawInMainRender, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDrawInPortalRender, FIELD_BOOLEAN ),
#endif
	DEFINE_KEYFIELD( m_flHDRColorScale, FIELD_FLOAT, "HDRColorScale" ),

	DEFINE_KEYFIELD( m_flGlowProxySize,	FIELD_FLOAT, "GlowProxySize" ),
	
	DEFINE_FIELD( m_flScaleTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartScale,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flDestScale,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flScaleTimeStart,	FIELD_TIME ),
	DEFINE_FIELD( m_nStartBrightness,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nDestBrightness,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flBrightnessTimeStart, FIELD_TIME ),
	DEFINE_FIELD( m_bWorldSpaceScale,	FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_FUNCTION( AnimateThink ),
	DEFINE_FUNCTION( ExpandThink ),
	DEFINE_FUNCTION( AnimateUntilDead ),
	DEFINE_FUNCTION( BeginFadeOutThink ),

	// Inputs
	DEFINE_INPUT( m_flSpriteScale, FIELD_FLOAT, "SetScale" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HideSprite", InputHideSprite ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowSprite", InputShowSprite ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleSprite", InputToggleSprite ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorRedValue", InputColorRedValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorGreenValue", InputColorGreenValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorBlueValue", InputColorBlueValue ),

END_DATADESC()

#else

BEGIN_PREDICTION_DATA( CSprite )

	// Networked
	DEFINE_PRED_FIELD( m_hAttachedToEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAttachment, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flScaleTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSpriteScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSpriteFramerate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flFrame, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#ifdef PORTAL
	DEFINE_PRED_FIELD( m_bDrawInMainRender, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDrawInPortalRender, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
	DEFINE_PRED_FIELD( m_flBrightnessTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBrightness, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( m_flLastTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDieTime, FIELD_FLOAT ),

//	DEFINE_FIELD( m_flHDRColorScale, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flStartScale, FIELD_FLOAT ),			//Starting scale
//	DEFINE_FIELD( m_flDestScale, FIELD_FLOAT ),			//Destination scale
//	DEFINE_FIELD( m_flScaleTimeStart, FIELD_FLOAT ),		//Real time for start of scale
//	DEFINE_FIELD( m_nStartBrightness, FIELD_INTEGER ),		//Starting brightness
//	DEFINE_FIELD( m_nDestBrightness, FIELD_INTEGER ),		//Destination brightness
//	DEFINE_FIELD( m_flBrightnessTimeStart, FIELD_FLOAT ),	//Real time for brightness

END_PREDICTION_DATA()

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( Sprite, DT_Sprite );

#if defined( CLIENT_DLL )

static void RecvProxy_SpriteScale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((CSprite*)pStruct)->SetSpriteScale( pData->m_Value.m_Float );
}

#endif

BEGIN_NETWORK_TABLE( CSprite, DT_Sprite )
#if !defined( CLIENT_DLL )
	SendPropEHandle( SENDINFO(m_hAttachedToEntity )),
	SendPropInt( SENDINFO(m_nAttachment ), 8 ),
	SendPropFloat( SENDINFO(m_flScaleTime ), 0,	SPROP_NOSCALE ),

#ifdef HL2_DLL
	SendPropFloat( SENDINFO(m_flSpriteScale ), 0,	SPROP_NOSCALE),
#else
	SendPropFloat( SENDINFO(m_flSpriteScale ), 8,	SPROP_ROUNDUP,	0.0f,	MAX_SPRITE_SCALE),
#endif
	SendPropFloat( SENDINFO(m_flGlowProxySize ), 6,	SPROP_ROUNDUP,	0.0f,	MAX_GLOW_PROXY_SIZE),

	SendPropFloat( SENDINFO(m_flHDRColorScale ), 0,	SPROP_NOSCALE,	0.0f,	100.0f),

	SendPropFloat( SENDINFO(m_flSpriteFramerate ), 8,	SPROP_ROUNDUP,	0,	60.0f),
	SendPropFloat( SENDINFO(m_flFrame),		20, SPROP_ROUNDDOWN,	0.0f,   256.0f),
#ifdef PORTAL
	SendPropBool( SENDINFO(m_bDrawInMainRender) ),
	SendPropBool( SENDINFO(m_bDrawInPortalRender) ),
#endif //#ifdef PORTAL
	SendPropFloat( SENDINFO(m_flBrightnessTime ), 0,	SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_nBrightness), 8, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO(m_bWorldSpaceScale) ),
#else
	RecvPropEHandle(RECVINFO(m_hAttachedToEntity)),
	RecvPropInt(RECVINFO(m_nAttachment)),
	RecvPropFloat(RECVINFO(m_flScaleTime)),
	RecvPropFloat(RECVINFO(m_flSpriteScale), 0, RecvProxy_SpriteScale),
	RecvPropFloat(RECVINFO(m_flSpriteFramerate)),
	RecvPropFloat(RECVINFO(m_flGlowProxySize)),

	RecvPropFloat( RECVINFO(m_flHDRColorScale )),

	RecvPropFloat(RECVINFO(m_flFrame)),
#ifdef PORTAL
	RecvPropBool( RECVINFO(m_bDrawInMainRender) ),
	RecvPropBool( RECVINFO(m_bDrawInPortalRender) ),
#endif //#ifdef PORTAL
	RecvPropFloat(RECVINFO(m_flBrightnessTime)),
	RecvPropInt(RECVINFO(m_nBrightness)),
	RecvPropBool( RECVINFO(m_bWorldSpaceScale) ),
#endif
END_NETWORK_TABLE()


CSprite::CSprite()
{
	m_flGlowProxySize = 2.0f;
	m_flHDRColorScale = 1.0f;

#ifdef PORTAL
	m_bDrawInMainRender = true;
	m_bDrawInPortalRender = true;
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	m_flFrame = 0;

	Precache();
	SetModel( STRING( GetModelName() ) );
	CollisionProp()->SetSurroundingBoundsType( USE_GAME_CODE );

	m_flMaxFrame = (float)modelinfo->GetModelFrameCount( GetModel() ) - 1;
	AddEffects( EF_NOSHADOW | EF_NORECEIVESHADOW );

#if defined( CLIENT_DLL )
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif

#if !defined( CLIENT_DLL )
	if ( GetEntityName() != NULL_STRING && !(m_spawnflags & SF_SPRITE_STARTON) )
	{
		TurnOff();
	}
	else
#endif
	{
		TurnOn();
	}
	
	// Worldcraft only sets y rotation, copy to Z
	if ( GetLocalAngles().y != 0 && GetLocalAngles().z == 0 )
	{
		QAngle angles = GetLocalAngles();

		angles.z = angles.y;
		angles.y = 0;

		SetLocalAngles( angles );
	}

	// Clamp our scale if necessary
	float scale = m_flSpriteScale;
	
	if ( scale < 0 || scale > MAX_SPRITE_SCALE )
	{
#if !defined( CLIENT_DLL ) 
		DevMsg( "LEVEL DESIGN ERROR: Sprite %s with bad scale %f [0..%f]\n", GetDebugName(), m_flSpriteScale.Get(), MAX_SPRITE_SCALE );
#endif
		scale = clamp( (float) m_flSpriteScale, 0.f, MAX_SPRITE_SCALE );
	}

	//Set our state
	SetBrightness( m_clrRender->a );
	SetScale( scale );

#if defined( CLIENT_DLL )
	m_flStartScale = m_flDestScale = m_flSpriteScale;
	m_nStartBrightness = m_nDestBrightness = m_nBrightness;
#endif

}


//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CSprite::EnableWorldSpaceScale( bool bEnable )
{
	m_bWorldSpaceScale = bEnable;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CSprite::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	float flScale = m_flSpriteScale * 0.5f;

	if ( m_bWorldSpaceScale == false )
	{
		// Find the height and width of the source of the sprite
		float width = modelinfo->GetModelSpriteWidth( GetModel() );
		float height = modelinfo->GetModelSpriteHeight( GetModel() );
		flScale *= MAX( width, height );
	}

	pVecWorldMins->Init( -flScale, -flScale, -flScale );
	pVecWorldMaxs->Init( flScale, flScale, flScale );
	*pVecWorldMins += GetAbsOrigin();
	*pVecWorldMaxs += GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CSprite::SetModel( const char *szModelName )
{
	int index_ = modelinfo->GetModelIndex( szModelName );
	const model_t *pModel = modelinfo->GetModel( index_ );
	if ( pModel && modelinfo->GetModelType( pModel ) != mod_sprite )
	{
		Msg( "Setting CSprite to non-sprite model %s\n", szModelName?szModelName:"NULL" );
	}

#if !defined( CLIENT_DLL )
	UTIL_SetModel( this, szModelName );
#else
	BaseClass::SetModel( szModelName );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::Precache( void )
{
	if ( GetModelName() != NULL_STRING )
	{
		PrecacheModel( STRING( GetModelName() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//-----------------------------------------------------------------------------
void CSprite::SpriteInit( const char *pSpriteName, const Vector &origin )
{
	SetModelName( MAKE_STRING(pSpriteName) );
	SetLocalOrigin( origin );
	Spawn();
}

#if !defined( CLIENT_DLL )

int CSprite::UpdateTransmitState( void )
{
	if ( GetMoveParent() )
	{
		// we must call ShouldTransmit() if we have a move parent
		return SetTransmitState( FL_EDICT_FULLCHECK );
	}
	else
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
}

int CSprite::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// Certain entities like sprites and ropes are strewn throughout the level and they rarely change.
	// For these entities, it's more efficient to transmit them once and then always leave them on
	// the client. Otherwise, the server will have to send big bursts of data with the entity states
	// as they come in and out of the PVS.
	
	if ( GetMoveParent() )
	{
		CBaseViewModel *pViewModel = dynamic_cast<CBaseViewModel *>( GetMoveParent() );

		if ( pViewModel )
		{
			return pViewModel->ShouldTransmit( pInfo );
		}
	}
	
	return FL_EDICT_ALWAYS;
}
 
//-----------------------------------------------------------------------------
// Purpose: Fixup parent after restore
//-----------------------------------------------------------------------------
void CSprite::OnRestore()
{
	BaseClass::OnRestore();

	// Reset attachment after save/restore
	if ( GetFollowedEntity() )
	{
		SetAttachment( GetFollowedEntity(), m_nAttachment );
	}
	else
	{
		// Clear attachment
		m_hAttachedToEntity = NULL;
		m_nAttachment = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSprite
//-----------------------------------------------------------------------------
CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CSprite *pSprite = CREATE_ENTITY( CSprite, "env_sprite" );
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->SetSolid( SOLID_NONE );
	UTIL_SetSize( pSprite, vec3_origin, vec3_origin );
	pSprite->SetMoveType( MOVETYPE_NONE );
	if ( animate )
		pSprite->TurnOn();

	return pSprite;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSprite
//-----------------------------------------------------------------------------
CSprite *CSprite::SpriteCreatePredictable( const char *module, int line, const char *pSpriteName, const Vector &origin, bool animate )
{
	CSprite *pSprite = ( CSprite * )CBaseEntity::CreatePredictedEntityByName( "env_sprite", module, line );
	if ( pSprite )
	{
		pSprite->SpriteInit( pSpriteName, origin );
		pSprite->SetSolid( SOLID_NONE );
		pSprite->SetSize( vec3_origin, vec3_origin );
		pSprite->SetMoveType( MOVETYPE_NONE );
		if ( animate )
			pSprite->TurnOn();
	}

	return pSprite;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::AnimateThink( void )
{
	Animate( m_flSpriteFramerate * (gpGlobals->curtime - m_flLastTime) );

	SetNextThink( gpGlobals->curtime );
	m_flLastTime			= gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::AnimateUntilDead( void )
{
	if ( gpGlobals->curtime > m_flDieTime )
	{
		Remove( );
	}
	else
	{
		AnimateThink();
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scaleSpeed - 
//			fadeSpeed - 
//-----------------------------------------------------------------------------
void CSprite::Expand( float scaleSpeed, float fadeSpeed )
{
	m_flSpeed = scaleSpeed;
	m_iHealth = fadeSpeed;
	SetThink( &CSprite::ExpandThink );

	SetNextThink( gpGlobals->curtime );
	m_flLastTime	= gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::ExpandThink( void )
{
	float frametime = gpGlobals->curtime - m_flLastTime;
	SetSpriteScale( m_flSpriteScale + m_flSpeed * frametime );

	int sub = (int)(m_iHealth * frametime);
	if ( sub > m_clrRender->a )
	{
		SetRenderColorA( 0 );
		Remove( );
	}
	else
	{
		SetRenderColorA( m_clrRender->a - sub );
		SetNextThink( gpGlobals->curtime );
		m_flLastTime		= gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frames - 
//-----------------------------------------------------------------------------
void CSprite::Animate( float frames )
{ 
	m_flFrame += frames;
	if ( m_flFrame > m_flMaxFrame )
	{
#if !defined( CLIENT_DLL )
		if ( m_spawnflags & SF_SPRITE_ONCE )
		{
			TurnOff();
		}
		else
#endif
		{
			if ( m_flMaxFrame > 0 )
				m_flFrame = fmod( m_flFrame, m_flMaxFrame );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSprite::SetBrightness( int brightness, float time )
{
	m_nBrightness			= brightness;	//Take our current position as our starting position
	m_flBrightnessTime		= time;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::SetSpriteScale( float scale )
{
	if ( scale != m_flSpriteScale )
	{
		m_flSpriteScale		= scale;	//Take our current position as our new starting position
		// The surrounding box is based on sprite scale... it changes, box is dirty
		CollisionProp()->MarkSurroundingBoundsDirty();
	}
}

void CSprite::SetScale( float scale, float time )
{
	m_flScaleTime		= time;
	SetSpriteScale( scale );
	// The surrounding box is based on sprite scale... it changes, box is dirty
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::TurnOff( void )
{
	AddEffects( EF_NODRAW );
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::TurnOn( void )
{
	RemoveEffects( EF_NODRAW );
	if ( (m_flSpriteFramerate && m_flMaxFrame > 1.0)
#if !defined( CLIENT_DLL )
		|| (m_spawnflags & SF_SPRITE_ONCE) 
#endif
		)
	{
		SetThink( &CSprite::AnimateThink );
		SetNextThink( gpGlobals->curtime );
		m_flLastTime = gpGlobals->curtime;
	}
	m_flFrame = 0;
}

#if !defined( CLIENT_DLL )
// DVS TODO: Obsolete Use handler
void CSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int on = !IsEffectActive( EF_NODRAW );
	if ( ShouldToggle( useType, on ) )
	{
		if ( on )
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that hides the sprite.
//-----------------------------------------------------------------------------
void CSprite::InputHideSprite( inputdata_t &inputdata )
{
	TurnOff();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that hides the sprite.
//-----------------------------------------------------------------------------
void CSprite::InputShowSprite( inputdata_t &inputdata )
{
	TurnOn();
}

void CSprite::InputColorRedValue( inputdata_t &inputdata )
{
	int nNewColor = clamp( FastFloatToSmallInt( inputdata.value.Float() ), 0, 255 );
	SetColor( nNewColor, m_clrRender->g, m_clrRender->b );
}

void CSprite::InputColorGreenValue( inputdata_t &inputdata )
{
	int nNewColor = clamp( FastFloatToSmallInt( inputdata.value.Float() ), 0, 255 );
	SetColor( m_clrRender->r, nNewColor, m_clrRender->b );
}

void CSprite::InputColorBlueValue( inputdata_t &inputdata )
{
	int nNewColor = clamp( FastFloatToSmallInt( inputdata.value.Float() ), 0, 255 );
	SetColor( m_clrRender->r, m_clrRender->g, nNewColor );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that toggles the sprite between hidden and shown.
//-----------------------------------------------------------------------------
void CSprite::InputToggleSprite( inputdata_t &inputdata )
{
	if ( !IsEffectActive( EF_NODRAW ) )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}
#endif

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CSprite::GetRenderScale( void )
{
	//See if we're done scaling
	if ( ( m_flScaleTime == 0 ) || ( (m_flScaleTimeStart+m_flScaleTime) < gpGlobals->curtime ) )
		return m_flSpriteScale;

	//Get our percentage
	float timeDelta = ( gpGlobals->curtime - m_flScaleTimeStart ) / m_flScaleTime;

	//Return the result
	return ( m_flStartScale + ( ( m_flDestScale - m_flStartScale  ) * timeDelta ) );
}

//-----------------------------------------------------------------------------
// Purpose: Get the rendered extents of the sprite
//-----------------------------------------------------------------------------
void CSprite::GetRenderBounds( Vector &vecMins, Vector &vecMaxs )
{
	float flScale = GetRenderScale() * 0.5f;

	// If our scale is normalized we need to convert that to actual world units
	if ( m_bWorldSpaceScale == false )
	{
		CEngineSprite *psprite = (CEngineSprite *) modelinfo->GetModelExtraData( GetModel() );
		if ( psprite )
		{
			float flSize = MAX( psprite->GetWidth(), psprite->GetHeight() );
			flScale *= flSize;
		}
	}

	vecMins.Init( -flScale, -flScale, -flScale );
	vecMaxs.Init(  flScale,  flScale,  flScale );

#if 0
	// Visualize the bounds
	if ( debugoverlay )
	{
		debugoverlay->AddBoxOverlay( GetRenderOrigin(), vecMins, vecMaxs, GetRenderAngles(), 255, 255, 255, 0, 0.01f );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSprite::GetRenderBrightness( void )
{
	//See if we're done scaling
	if ( ( m_flBrightnessTime == 0 ) || ( (m_flBrightnessTimeStart+m_flBrightnessTime) < gpGlobals->curtime ) )
	{
		return m_nBrightness;
	}

	//Get our percentage
	float timeDelta = ( gpGlobals->curtime - m_flBrightnessTimeStart ) / m_flBrightnessTime;

	float brightness = ( (float) m_nStartBrightness + ( (float) ( m_nDestBrightness - m_nStartBrightness  ) * timeDelta ) );

	//Return the result
	return (int) brightness;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Only think when sapping
	SetNextClientThink( CLIENT_THINK_ALWAYS );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flStartScale = m_flDestScale = m_flSpriteScale;
		m_nStartBrightness = m_nDestBrightness = m_nBrightness;
	}

	UpdateVisibility();
}

void CSprite::ClientThink( void )
{
	BaseClass::ClientThink();

	// Module render colors over time
	if ( m_flSpriteScale != m_flDestScale )
	{
		m_flStartScale		= m_flDestScale;
		m_flDestScale		= m_flSpriteScale;
		m_flScaleTimeStart	= gpGlobals->curtime;
	}

	if ( m_nBrightness != m_nDestBrightness )
	{
		m_nStartBrightness		= m_nDestBrightness;
		m_nDestBrightness		= m_nBrightness;
		m_flBrightnessTimeStart = gpGlobals->curtime;
	}
}

extern bool g_bRenderingScreenshot;
extern ConVar r_drawviewmodel;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int CSprite::DrawModel( int flags )
{
	VPROF_BUDGET( "CSprite::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	//See if we should draw
	if ( !IsVisible() || ( m_bReadyToDraw == false ) )
		return 0;

#ifdef PORTAL
	if ( ( !g_pPortalRender->IsRenderingPortal() && !m_bDrawInMainRender ) || 
		( g_pPortalRender->IsRenderingPortal() && !m_bDrawInPortalRender ) )
	{
		return 0;
	}
#endif //#ifdef PORTAL

	// Tracker 16432:  If rendering a savegame screenshot then don't draw sprites 
	//   who have viewmodels as their moveparent
	if ( g_bRenderingScreenshot || !r_drawviewmodel.GetBool() )
	{
		C_BaseViewModel *vm = dynamic_cast< C_BaseViewModel * >( GetMoveParent() );
		if ( vm )
		{
			return 0;
		}
	}

	//Must be a sprite
	if ( modelinfo->GetModelType( GetModel() ) != mod_sprite )
	{
		Assert( 0 );
		return 0;
	}

	float renderscale = GetRenderScale();
	if ( m_bWorldSpaceScale )
	{
		CEngineSprite *psprite = ( CEngineSprite * )modelinfo->GetModelExtraData( GetModel() );
		float flMinSize = MIN( psprite->GetWidth(), psprite->GetHeight() );
		renderscale /= flMinSize;
	}

	//Draw it
	int drawn = DrawSprite( 
		this,
		GetModel(), 
		GetAbsOrigin(), 
		GetAbsAngles(), 
		m_flFrame,				// sprite frame to render
		m_hAttachedToEntity,	// attach to
		m_nAttachment,			// attachment point
		GetRenderMode(),		// rendermode
		m_nRenderFX,
		GetRenderBrightness(),	// alpha
		m_clrRender->r,
		m_clrRender->g,
		m_clrRender->b,
		renderscale,			// sprite scale
		GetHDRColorScale()		// HDR Color Scale
		);

	return drawn;
}


const Vector& CSprite::GetRenderOrigin()
{
	static Vector vOrigin;
	vOrigin = GetAbsOrigin();

	if ( m_hAttachedToEntity )
	{
		C_BaseEntity *ent = m_hAttachedToEntity->GetBaseEntity();
		if ( ent )
		{
			QAngle dummyAngles;
			ent->GetAttachment( m_nAttachment, vOrigin, dummyAngles );
		}
	}

	return vOrigin;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: oriented sprites
//			CSprites swap the roll and yaw angle inputs, and rotate the yaw 180 degrees
//-----------------------------------------------------------------------------

#if !defined( CLIENT_DLL )
IMPLEMENT_SERVERCLASS_ST( CSpriteOriented, DT_SpriteOriented )
END_SEND_TABLE()
#else
#undef CSpriteOriented
IMPLEMENT_CLIENTCLASS_DT(C_SpriteOriented, DT_SpriteOriented, CSpriteOriented)
#define CSpriteOriented C_SpriteOriented
END_RECV_TABLE()
#endif

#if !defined( CLIENT_DLL )

void CSpriteOriented::Spawn( void )
{
	// save a copy of the angles, CSprite swaps the yaw and roll
	QAngle angles = GetAbsAngles();
	BaseClass::Spawn();
	// ORIENTED sprites "forward" vector points in the players "view" direction, not the direction "out" from the sprite (gah)
	angles.y = anglemod( angles.y + 180 );
	SetAbsAngles( angles );
}

#else

bool CSpriteOriented::IsTransparent( void )
{
	return true;
}

#endif
