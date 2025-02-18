//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "particles_localspace.h"
#include "c_te_effect_dispatch.h"
#include "clienteffectprecachesystem.h"
#include "tier0/vprof.h"
#include "fx.h"
#include "r_efx.h"
#include "tier1/KeyValues.h"
#include "dlight.h"
#include "tf_shareddefs.h"
#include "tf_fx_muzzleflash.h"
#include "toolframework/itoolframework.h"
#include "IEffects.h"
#include "fx_sparks.h"
#include "iefx.h"
#include "fx_quad.h"
#include "fx.h"
#include "toolframework_client.h"

// Precache our effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffect_TF_MuzzleFlash )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash2" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash3" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash4" )
CLIENTEFFECT_REGISTER_END()

ConVar cl_muzzleflash_dlight_1st( "cl_muzzleflash_dlight_1st", "1" );

void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

extern PMaterialHandle g_Material_Spark;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TF_3rdPersonMuzzleFlashCallback( const CEffectData &data )
{
	float scale = data.m_flMagnitude;
	int attachmentIndex = data.m_nAttachmentIndex;
	
	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash", data.m_hEntity, attachmentIndex, 0 );
	
	Vector			forward(1,0,0), offset, right(0,1,0);

	//
	// Flash
	//

	if ( data.m_nHitBox > 0 )	// >0 is a mg flash in script based muzzleflashes ..
	{
		offset = vec3_origin;

		// small inner cone
		scale *= 4;
		float flScale = random->RandomFloat( scale-0.1f, scale+0.1f );

		for ( int i = 1; i < 9; i++ )
		{
			offset = (forward * (i*2.0f*scale));

			SimpleParticle *pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
				
			if ( pParticle == NULL )
				return;

			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= 0.1f;

			pParticle->m_vecVelocity.Init();

			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 255;
			pParticle->m_uchColor[2]	= 255;

			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 128;

			pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= 0.0f;
		}
	}
	else
	{
		scale *= 4;
		float flScale = random->RandomFloat( scale-0.1f, scale+0.1f );
		
		for ( int i = 1; i < 9; i++ )
		{
			offset = (forward * (i*2.0f*scale));

			SimpleParticle *pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
				
			if ( pParticle == NULL )
				return;

			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= 0.1f;

			pParticle->m_vecVelocity.Init();

			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 255;
			pParticle->m_uchColor[2]	= 255;

			pParticle->m_uchStartAlpha	= 128;
			pParticle->m_uchEndAlpha	= 64;

			pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= 0.0f;
		}
	}

	//Smoke
	int i;
	offset = vec3_origin;
	for( i=0;i<3;i++ )
	{
		SimpleParticle *pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], offset );
			
		if ( pParticle )
		{
			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= 0.3f;

			pParticle->m_vecVelocity.Init();
			pParticle->m_vecVelocity = forward * ( random->RandomFloat( 80.0f, 100.0f ) + (2-i)*15 );

			int color = random->RandomInt( 200, 255 );
			pParticle->m_uchColor[0]	= color;
			pParticle->m_uchColor[1]	= color;
			pParticle->m_uchColor[2]	= color;

			pParticle->m_uchStartAlpha	= 32;
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_uchStartSize	= ( 4.0 + 3.0*(2-i) ) * data.m_flMagnitude;
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 13.0f;
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -0.5f, 0.5f );
		}
	}
}

void TF_3rdPersonMuzzleFlashCallback_SentryGun( const CEffectData &data )
{
	int iMuzzleFlashAttachment = data.m_nAttachmentIndex;
	int iUpgradeLevel	= data.m_fFlags;

	C_BaseEntity *pEnt = data.GetEntity();
	if ( pEnt && !pEnt->IsDormant() )
	{
		// The created entity kills itself
		//C_MuzzleFlashModel::CreateMuzzleFlashModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl", pEnt, iMuzzleFlashAttachment );

		const char *pszMuzzleFlashParticleEffect = NULL;
		switch( iUpgradeLevel )
		{
		case 1:
		default:
			pszMuzzleFlashParticleEffect = "muzzle_sentry";
			break;
		case 2:
		case 3:
			pszMuzzleFlashParticleEffect = "muzzle_sentry2";
			break;
		}

		DispatchParticleEffect( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, pEnt, iMuzzleFlashAttachment );
	}
}

//TODO: Come back and make this guy a nice particle.
DECLARE_CLIENT_EFFECT( "TF_3rdPersonMuzzleFlash", TF_3rdPersonMuzzleFlashCallback );
DECLARE_CLIENT_EFFECT( "TF_3rdPersonMuzzleFlash_SentryGun", TF_3rdPersonMuzzleFlashCallback_SentryGun );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszModelName - 
//			vecOrigin - 
//			vecForceDir - 
//			vecAngularImp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
C_MuzzleFlashModel *C_MuzzleFlashModel::CreateMuzzleFlashModel( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime )
{
	C_MuzzleFlashModel *pFlash = new C_MuzzleFlashModel;
	if ( !pFlash )
		return NULL;

	if ( !pFlash->InitializeMuzzleFlash( pszModelName, pParent, iAttachment, flLifetime ) )
		return NULL;

	return pFlash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_MuzzleFlashModel::InitializeMuzzleFlash( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime )
{
	AddEffects( EF_NORECEIVESHADOW | EF_NOSHADOW );
	if ( InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		Release();
		return false;
	}

	SetParent( pParent, iAttachment );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );

	AddSolidFlags( FSOLID_NOT_SOLID );

	m_flRotateAt = gpGlobals->curtime + 0.2;
	SetLifetime( flLifetime );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	SetCycle( 0 );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::SetLifetime( float flLifetime )
{
	// Expire when the lifetime is up
	m_flExpiresAt = gpGlobals->curtime + flLifetime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::ClientThink( void )
{
	if ( !GetMoveParent() || gpGlobals->curtime > m_flExpiresAt )
	{
		Release();
		return;
	}

	if ( gpGlobals->curtime > m_flRotateAt )
	{
		// Pick a new anim frame
		float flDelta = RandomFloat(0.2,0.4) * (RandomInt(0,1) == 1 ? 1 : -1);
		float flCycle = clamp( GetCycle() + flDelta, 0.f, 1.f );
		SetCycle( flCycle );

		SetLocalAngles( QAngle(0,0,RandomFloat(0,360)) );
		m_flRotateAt = gpGlobals->curtime + 0.075;
	}
}


//-----------------------------------------------------------------------------
// This is an incredibly brutal hack to get muzzle flashes positioned correctly for recording
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::SetIs3rdPersonFlash( bool bEnable )
{
	m_bIs3rdPersonFlash = bEnable;
}

					   
bool C_MuzzleFlashModel::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	// FIXME: This is an incredibly brutal hack to get muzzle flashes positioned correctly for recording
	// NOTE: The correct, long-term solution, is to make weapon models
	// always store the 3rd person model and to have view model entities
	// always store the 1st person model. I didn't have time to do that for Leipzig.
	int nModelIndex = 0;
	int nWorldModelIndex = 0;
	CBaseCombatWeapon *pParent = dynamic_cast<CBaseCombatWeapon*>( GetMoveParent() );
	if ( m_bIs3rdPersonFlash && pParent )
	{
		nModelIndex = pParent->GetModelIndex();
		nWorldModelIndex = pParent->GetWorldModelIndex();
		pParent->SetModelIndex( nWorldModelIndex );
	}
	
	bool bResult = BaseClass::SetupBones( pBoneToWorldOut, nMaxBones, boneMask, currentTime );

	if ( m_bIs3rdPersonFlash && pParent )
	{
		pParent->SetModelIndex( nModelIndex );
	}

	return bResult;
}


//-----------------------------------------------------------------------------
// Recording
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_MuzzleFlashModel::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	BaseClass::GetToolRecordingState( msg );

	C_BaseEntity *pParent = GetMoveParent();
	if ( pParent )
	{
		BaseEntityRecordingState_t *pBaseEntity = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );
		pBaseEntity->m_nOwner = pParent->entindex();

		// FIXME: This recording path is a giant hack in a cascading series of hacks
		C_BaseCombatWeapon *pParentWeapon = dynamic_cast<C_BaseCombatWeapon*>( pParent );
		if ( pParentWeapon )
		{
			if ( pParentWeapon->WeaponState() == WEAPON_NOT_CARRIED )
			{
				pBaseEntity->m_bVisible = false;
			}
		}
	}
}
