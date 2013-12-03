//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "citadel_effects_shared.h"
#include "particles_attractor.h"
#include "iefx.h"
#include "dlight.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"
#include "fx_quad.h"

#include "c_ai_basenpc.h"

// For material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

#define NUM_INTERIOR_PARTICLES	8

#define DLIGHT_RADIUS (150.0f)
#define DLIGHT_MINLIGHT (40.0f/255.0f)

class C_NPC_Vortigaunt : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_Vortigaunt, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );
	virtual void	ReceiveMessage( int classID, bf_read &msg );

public:
	bool  m_bIsBlue;           ///< wants to fade to blue
	float m_flBlueEndFadeTime; ///< when to end fading from one skin to another

	bool  m_bIsBlack;    ///< wants to fade to black (networked)
	float m_flBlackFade; ///< [0.00 .. 1.00] where 1.00 is all black. Locally interpolated.
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Vortigaunt, DT_NPC_Vortigaunt, CNPC_Vortigaunt )
	RecvPropTime( RECVINFO(m_flBlueEndFadeTime ) ),
	RecvPropBool( RECVINFO(m_bIsBlue) ),
	RecvPropBool( RECVINFO(m_bIsBlack) ),
END_RECV_TABLE()


#define	VORTIGAUNT_BLUE_FADE_TIME			2.25f		// takes this long to fade from green to blue or back
#define VORT_BLACK_FADE_TIME 2.2f	// time to interpolate up or down in fading to black


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Vortigaunt::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// start thinking if we need to fade.
	if ( m_flBlackFade != (m_bIsBlack ? 1.0f : 0.0f) )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Vortigaunt::ClientThink( void )
{
	// Don't update if our frame hasn't moved forward (paused)
	if ( gpGlobals->frametime <= 0.0f )
		return;

	if ( m_bIsBlack )
	{
		// are we done?
		if ( m_flBlackFade >= 1.0f )
		{
			m_flBlackFade = 1.0f;
			SetNextClientThink( CLIENT_THINK_NEVER );
		}
		else // interpolate there
		{
			float lerpQuant = gpGlobals->frametime / VORT_BLACK_FADE_TIME;
			m_flBlackFade += lerpQuant;
			if ( m_flBlackFade > 1.0f )
			{
				m_flBlackFade = 1.0f;
			}
		}
	}
	else 
	{
		// are we done?
		if ( m_flBlackFade <= 0.0f )
		{
			m_flBlackFade = 0.0f;
			SetNextClientThink( CLIENT_THINK_NEVER );
		}
		else // interpolate there
		{
			float lerpQuant = gpGlobals->frametime / VORT_BLACK_FADE_TIME;
			m_flBlackFade -= lerpQuant;
			if ( m_flBlackFade < 0.0f )
			{
				m_flBlackFade = 0.0f;
			}
		}
	}
}

// FIXME: Move to shared code!
#define VORTFX_ZAPBEAM	0
#define VORTFX_ARMBEAM	1

//-----------------------------------------------------------------------------
// Purpose: Receive messages from the server
//-----------------------------------------------------------------------------
void C_NPC_Vortigaunt::ReceiveMessage( int classID, bf_read &msg )
{
	// Is the message for a sub-class?
	if ( classID != GetClientClass()->m_ClassID )
	{
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}
	
	int messageType = msg.ReadByte();
	switch( messageType )
	{
	case VORTFX_ZAPBEAM:
		{
			// Find our attachment point
			unsigned char nAttachment = msg.ReadByte();
			
			// Get our attachment position
			Vector vecStart;
			QAngle vecAngles;
			GetAttachment( nAttachment, vecStart, vecAngles );

			// Get the final position we'll strike
			Vector vecEndPos;
			msg.ReadBitVec3Coord( vecEndPos );

			// Place a beam between the two points
			CNewParticleEffect *pEffect = ParticleProp()->Create( "vortigaunt_beam", PATTACH_POINT_FOLLOW, nAttachment );
			if ( pEffect )
			{
				pEffect->SetControlPoint( 0, vecStart );
				pEffect->SetControlPoint( 1, vecEndPos );
			}
		}
		break;

	case VORTFX_ARMBEAM:
		{
			int nIndex = msg.ReadLong();
			C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( ClientEntityList().EntIndexToHandle( nIndex ) );

			if ( pEnt )
			{
				unsigned char nAttachment = msg.ReadByte();
				Vector vecEndPos;
				msg.ReadBitVec3Coord( vecEndPos );
				
				Vector vecNormal;
				msg.ReadBitVec3Normal( vecNormal );
				
				CNewParticleEffect *pEffect = pEnt->ParticleProp()->Create( "vortigaunt_beam_charge", PATTACH_POINT_FOLLOW, nAttachment );
				if ( pEffect )
				{
					// Set the control point's angles to be the surface normal we struct
					Vector vecRight, vecUp;
					VectorVectors( vecNormal, vecRight, vecUp );
					pEffect->SetControlPointOrientation( 1, vecNormal, vecRight, vecUp );
					pEffect->SetControlPoint( 1, vecEndPos );
				}
			}
		}
		break;
	default:
		AssertMsg1( false, "Received unknown message %d", messageType);
	}
}

class C_VortigauntChargeToken : public C_BaseEntity
{
	DECLARE_CLASS( C_VortigauntChargeToken, C_BaseEntity );
	DECLARE_CLIENTCLASS();

public:
	virtual void	UpdateOnRemove( void );
	virtual void	ClientThink( void );
	virtual void	NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void	OnDataChanged( DataUpdateType_t type );

	// For RecvProxy handlers
	float							m_flFadeOutTime;
	float							m_flFadeOutStart;

private:
	bool		SetupEmitters( void );

	bool							m_bFadeOut;
	CNewParticleEffect				*m_hEffect;
	dlight_t						*m_pDLight;
};

void RecvProxy_FadeOutDuration( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_VortigauntChargeToken *pVortToken = (C_VortigauntChargeToken *) pStruct;
	Assert( pOut == &pVortToken->m_flFadeOutTime );

	pVortToken->m_flFadeOutStart = gpGlobals->curtime;
	pVortToken->m_flFadeOutTime = ( pData->m_Value.m_Float - gpGlobals->curtime );
}

IMPLEMENT_CLIENTCLASS_DT( C_VortigauntChargeToken, DT_VortigauntChargeToken, CVortigauntChargeToken )
	RecvPropBool( RECVINFO( m_bFadeOut ) ),
END_RECV_TABLE()

void C_VortigauntChargeToken::UpdateOnRemove( void )
{
	if ( m_hEffect )
	{
		m_hEffect->StopEmission();
		m_hEffect = NULL;
	}

	if ( m_pDLight != NULL )
	{
		m_pDLight->die = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change our transmission state 
//-----------------------------------------------------------------------------
void C_VortigauntChargeToken::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit( state );

	// Turn off
	if ( state == SHOULDTRANSMIT_END )
	{
		if ( m_hEffect )
		{
			m_hEffect->StopEmission();
			m_hEffect = NULL;
		}
	}

	// Turn on
	if ( state == SHOULDTRANSMIT_START )
	{
		m_hEffect = ParticleProp()->Create( "vortigaunt_charge_token", PATTACH_ABSORIGIN_FOLLOW );
		m_hEffect->SetControlPointEntity( 0, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_VortigauntChargeToken::OnDataChanged( DataUpdateType_t type )
{
	if ( m_bFadeOut )
	{
		if ( m_hEffect )
		{
			m_hEffect->StopEmission();
			m_hEffect = NULL;
		}
	}

	BaseClass::OnDataChanged( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_VortigauntChargeToken::ClientThink( void )
{
	//
	// -- DLight 
	//

	if ( m_pDLight != NULL )
	{
		m_pDLight->origin = GetAbsOrigin();
		m_pDLight->radius = DLIGHT_RADIUS;
	}
}

//=============================================================================
// 
//  Dispel Effect
//	
//=============================================================================

class C_VortigauntEffectDispel : public C_BaseEntity
{
	DECLARE_CLASS( C_VortigauntEffectDispel, C_BaseEntity );
	DECLARE_CLIENTCLASS();

public:
	virtual void	UpdateOnRemove( void );
	virtual void	ClientThink( void );
	virtual void	NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void	OnDataChanged( DataUpdateType_t type );

	// For RecvProxy handlers
	float							m_flFadeOutTime;
	float							m_flFadeOutStart;

private:
	bool	SetupEmitters( void );

	CNewParticleEffect				*m_hEffect;
	bool							m_bFadeOut;
	dlight_t						*m_pDLight;
};

void RecvProxy_DispelFadeOutDuration( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_VortigauntEffectDispel *pVortToken = (C_VortigauntEffectDispel *) pStruct;
	Assert( pOut == &pVortToken->m_flFadeOutTime );

	pVortToken->m_flFadeOutStart = gpGlobals->curtime;
	pVortToken->m_flFadeOutTime = ( pData->m_Value.m_Float - gpGlobals->curtime );
}

IMPLEMENT_CLIENTCLASS_DT( C_VortigauntEffectDispel, DT_VortigauntEffectDispel, CVortigauntEffectDispel )
	RecvPropBool( RECVINFO( m_bFadeOut ) ),
END_RECV_TABLE()

void C_VortigauntEffectDispel::UpdateOnRemove( void )
{
	if ( m_hEffect )
	{
		m_hEffect->StopEmission();
		m_hEffect = NULL;
	}

	if ( m_pDLight != NULL )
	{
		m_pDLight->die = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_VortigauntEffectDispel::OnDataChanged( DataUpdateType_t type )
{
	if ( m_bFadeOut )
	{
		if ( m_hEffect )
		{
			m_hEffect->StopEmission();
			m_hEffect = NULL;
		}
	}

	BaseClass::OnDataChanged( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_VortigauntEffectDispel::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit( state );

	// Turn off
	if ( state == SHOULDTRANSMIT_END )
	{
		if ( m_hEffect )
		{
			m_hEffect->StopEmission();
			m_hEffect = NULL;
		}
	}

	// Turn on
	if ( state == SHOULDTRANSMIT_START )
	{
		m_hEffect = ParticleProp()->Create( "vortigaunt_hand_glow", PATTACH_ABSORIGIN_FOLLOW );
		m_hEffect->SetControlPointEntity( 0, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create our emitter
//-----------------------------------------------------------------------------
bool C_VortigauntEffectDispel::SetupEmitters( void )
{
	m_pDLight = NULL;

#ifndef _X360
	m_pDLight = effects->CL_AllocDlight ( index );
	m_pDLight->origin = GetAbsOrigin();
	m_pDLight->color.r = 64;
	m_pDLight->color.g = 255;
	m_pDLight->color.b = 64;
	m_pDLight->radius = 0;
	m_pDLight->minlight = DLIGHT_MINLIGHT;
	m_pDLight->die = FLT_MAX;
#endif // _X360

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_VortigauntEffectDispel::ClientThink( void )
{
	if ( m_pDLight != NULL )
	{
		m_pDLight->origin = GetAbsOrigin();
		m_pDLight->radius = DLIGHT_RADIUS;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispelCallback( const CEffectData &data )
{
	// Kaboom!
	Vector startPos = data.m_vOrigin + Vector(0,0,16);
	Vector endPos = data.m_vOrigin + Vector(0,0,-64);

	trace_t tr;
	UTIL_TraceLine( startPos, endPos, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		//Add a ripple quad to the surface
		FX_AddQuad( tr.endpos + ( tr.plane.normal * 8.0f ), 
			Vector( 0, 0, 1 ), 
			64.0f, 
			600.0f, 
			0.8f,
			1.0f,	// start alpha
			0.0f,	// end alpha
			0.3f,
			random->RandomFloat( 0, 360 ),
			0.0f,
			Vector( 0.5f, 1.0f, 0.5f ), 
			0.75f, 
			"effects/ar2_altfire1b", 
			(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA|FXQUAD_COLOR_FADE) );
		
		//Add a ripple quad to the surface
		FX_AddQuad( tr.endpos + ( tr.plane.normal * 8.0f ), 
			Vector( 0, 0, 1 ), 
			16.0f, 
			300.0f,
			0.9f,
			1.0f,	// start alpha
			0.0f,	// end alpha
			0.9f,
			random->RandomFloat( 0, 360 ),
			0.0f,
			Vector( 0.5f, 1.0f, 0.5f ), 
			1.25f, 
			"effects/rollerglow", 
			(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
	}
}

DECLARE_CLIENT_EFFECT( "VortDispel", DispelCallback );

//-----------------------------------------------------------------------------
// Purpose: Used for emissive lightning layer on vort
//-----------------------------------------------------------------------------
class CVortEmissiveProxy : public CEntityMaterialProxy
{
public:
	CVortEmissiveProxy( void );
	virtual				~CVortEmissiveProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

private:

	IMaterialVar *m_pMatEmissiveStrength;
	IMaterialVar *m_pMatDetailBlendStrength;
};

//-----------------------------------------------------------------------------
CVortEmissiveProxy::CVortEmissiveProxy( void )
{
	m_pMatEmissiveStrength = NULL;
	m_pMatDetailBlendStrength = NULL;
}

CVortEmissiveProxy::~CVortEmissiveProxy( void )
{
	// Do nothing
}

//-----------------------------------------------------------------------------
bool CVortEmissiveProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bFound;
	m_pMatEmissiveStrength = pMaterial->FindVar( "$emissiveblendstrength", &bFound );

	if ( bFound )
	{
		// Optional
		bool bFound2;
		m_pMatDetailBlendStrength = pMaterial->FindVar( "$detailblendfactor", &bFound2 );
	}

	return bFound;
}

//-----------------------------------------------------------------------------
void CVortEmissiveProxy::OnBind( C_BaseEntity *pEnt )
{
	C_NPC_Vortigaunt *pVort = dynamic_cast<C_NPC_Vortigaunt *>(pEnt);

	float flBlendValue;

	if (pVort)
	{
		// do we need to crossfade?
		if (gpGlobals->curtime < pVort->m_flBlueEndFadeTime)
		{
			// will be 0 when fully faded and 1 when not faded at all:
			float fadeRatio = (pVort->m_flBlueEndFadeTime - gpGlobals->curtime) / VORTIGAUNT_BLUE_FADE_TIME;
			if (pVort->m_bIsBlue)
			{
				fadeRatio = 1.0f - fadeRatio;
			}
			flBlendValue = clamp( fadeRatio, 0.0f, 1.0f );
		}
		else // no crossfade
		{
			flBlendValue = pVort->m_bIsBlue ? 1.0f : 0.0f;
		}

		// ALEX VLACHOS: 
		// The following variable varies on [0 .. 1]. 0.0 means the vort wants to be his normal
		// color. 1.0 means he wants to be all black. It is interpolated in the
		// C_NPC_Vortigaunt::ClientThink() function. 
		// 
		// pVort->m_flBlackFade
	}
	else
	{	// if you bind this proxy to anything non-vort (eg a ragdoll) it's always green
		flBlendValue = 0.0f;
	}


	/*
	// !!! Change me !!! I'm using a clamped sin wave for debugging
	float flBlendValue = sinf( gpGlobals->curtime * 4.0f ) * 0.75f + 0.25f;

	// Clamp 0-1
	flBlendValue = ( flBlendValue < 0.0f ) ? 0.0f : ( flBlendValue > 1.0f ) ? 1.0f : flBlendValue;
	*/

	if( m_pMatEmissiveStrength != NULL )
	{
		m_pMatEmissiveStrength->SetFloatValue( flBlendValue );
	}

	if( m_pMatDetailBlendStrength != NULL )
	{
		m_pMatDetailBlendStrength->SetFloatValue( flBlendValue );
	}
}

//-----------------------------------------------------------------------------
IMaterial *CVortEmissiveProxy::GetMaterial()
{
	if ( m_pMatEmissiveStrength != NULL )
		return m_pMatEmissiveStrength->GetOwningMaterial();
	else if ( m_pMatDetailBlendStrength != NULL )
		return m_pMatDetailBlendStrength->GetOwningMaterial();
	else
		return NULL;
}

EXPOSE_INTERFACE( CVortEmissiveProxy, IMaterialProxy, "VortEmissive" IMATERIAL_PROXY_INTERFACE_VERSION );
