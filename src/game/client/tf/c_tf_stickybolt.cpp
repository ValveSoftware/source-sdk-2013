//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF version of the stickybolt code.
//			I broke off our own version because I didn't want to accidentally break HL2.
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "fx.h"
#include "decals.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "engine/IEngineTrace.h"
#include "vphysics/constraints.h"
#include "engine/ivmodelinfo.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_player.h"
#include "GameEventListener.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

//-----------------------------------------------------------------------------
// Purpose: Creates a Bolt in the world and Ragdolls
// For Attached Bolts on players look at hud_bowcharge "arrow_impact" which should be moved here
//-----------------------------------------------------------------------------
void CreateCrossbowBoltTF( const Vector &vecOrigin, const Vector &vecDirection, const int iFlags, unsigned char nColor )
{
	const char* pszModelName = NULL;
	float flDirOffset = 5.0f;
	float flScale = 1.0f;
	float flLifeTime = 30.0f;
	switch ( iFlags )
	{
	case TF_PROJECTILE_STICKY_BALL:
		pszModelName = g_pszArrowModels[MODEL_SNOWBALL];
		break;
	case TF_PROJECTILE_ARROW:
		pszModelName = g_pszArrowModels[MODEL_ARROW_REGULAR];
		break;
	case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
		pszModelName = g_pszArrowModels[MODEL_ARROW_BUILDING_REPAIR];
		flDirOffset = -2.0f;
		break;
	case TF_PROJECTILE_FESTIVE_ARROW:
		pszModelName = g_pszArrowModels[MODEL_FESTIVE_ARROW_REGULAR];
		break;
	case TF_PROJECTILE_HEALING_BOLT:
		pszModelName = g_pszArrowModels[MODEL_SYRINGE];
		flDirOffset = 0.0f;
		flScale = 3.0f;
		break;
	case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
		pszModelName = g_pszArrowModels[MODEL_FESTIVE_HEALING_BOLT];
		flScale = 2.5f;
		break;
	case TF_PROJECTILE_BREAD_MONSTER:
	case TF_PROJECTILE_BREADMONSTER_JARATE:
	case TF_PROJECTILE_BREADMONSTER_MADMILK:
		pszModelName = g_pszArrowModels[MODEL_BREAD_MONSTER];
		flLifeTime = 8.0f;
		flScale = 2.5f;
		break;
	case TF_PROJECTILE_GRAPPLINGHOOK:
		pszModelName = g_pszArrowModels[MODEL_GRAPPLINGHOOK];
		flDirOffset = 0.0f;
		flLifeTime = 0.1f;
		break;
	default:
		// Unsupported Model
		Assert( 0 );
		pszModelName = g_pszArrowModels[MODEL_ARROW_REGULAR];
		return;
	}
	model_t *pModel = (model_t *)engine->LoadModel( pszModelName );

	QAngle vAngles;
	VectorAngles( vecDirection, vAngles );	
	C_LocalTempEntity *arrow = tempents->SpawnTempModel( pModel, vecOrigin - vecDirection * flDirOffset, vAngles, Vector(0, 0, 0 ), flLifeTime, FTENT_NONE );

	if ( arrow )
	{
		arrow->SetModelScale( flScale );
		arrow->m_nSkin = nColor;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void StickRagdollNowTF( 
	const Vector &vecOrigin, 
	const Vector &vecDirection, 
	const ClientEntityHandle_t &entHandle, 
	const int boneIndexAttached, 
	const int physicsBoneIndex, 
	const int iShooterIndex, 
	const int iHitGroup, 
	const int iVictim,
	const int iFlags,
	unsigned char nColor
) {
	Ray_t	shotRay;
	trace_t tr;
	
	UTIL_TraceLine( vecOrigin - vecDirection * 16, vecOrigin + vecDirection * 64, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.surface.flags & SURF_SKY )
		return;

	C_BaseAnimating *pModel = dynamic_cast< C_BaseAnimating * >( entHandle.Get() );
	if ( pModel  )
	{
		IPhysicsObject	*pPhysicsObject = NULL;
		ragdoll_t *pRagdollT = NULL;
		if ( pModel->m_pRagdoll )
		{
			CRagdoll *pCRagdoll = dynamic_cast < CRagdoll * > ( pModel->m_pRagdoll );
			if ( pCRagdoll )
			{
				pRagdollT = pCRagdoll->GetRagdoll();
				if ( physicsBoneIndex < pRagdollT->listCount )
				{
					pPhysicsObject = pRagdollT->list[physicsBoneIndex].pObject;
				}
			}
		}

		IPhysicsObject *pReference = GetWorldPhysObject();

		if ( pReference == NULL || pPhysicsObject == NULL )
			return;

		float frand = (float) rand() / VALVE_RAND_MAX;
		Vector adjust = vecDirection*7 + vecDirection * frand * 7;

		Vector vecBonePos;
		QAngle boneAngles;
		pPhysicsObject->GetPosition( &vecBonePos, &boneAngles );

		QAngle angles;
		pPhysicsObject->SetPosition( vecOrigin-adjust, boneAngles, true );

		pPhysicsObject->EnableMotion( false );

		int nNodeIndex = pRagdollT->list[physicsBoneIndex].parentIndex;

		// find largest mass bone
		float flTargetMass = 0;
		for ( int i = 0; i < pRagdollT->listCount; i++ )
		{
			flTargetMass = MAX(flTargetMass, pRagdollT->list[i].pObject->GetMass() );
		}

		// walk the chain of bones from the pinned bone to the root and set each to the max mass
		// This helps transmit the impulses required to stabilize the constraint -- it keeps the body from
		// leaving the constraint because of some high mass bone hanging at the other end of the chain
		while ( nNodeIndex >= 0 )
		{
			if ( pRagdollT->list[nNodeIndex].pConstraint )
			{
				float flCurrentMass = pRagdollT->list[nNodeIndex].pObject->GetMass();
				flCurrentMass = MAX(flCurrentMass, flTargetMass);
				pRagdollT->list[nNodeIndex].pObject->SetMass( flCurrentMass );
			}
			nNodeIndex = pRagdollT->list[nNodeIndex].parentIndex;
		}
	}

	UTIL_ImpactTrace( &tr, 0 );

	CreateCrossbowBoltTF( vecOrigin, vecDirection, iFlags, nColor );

	//Achievement stuff.
	if ( iHitGroup == HITGROUP_HEAD )
	{
		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	
		if ( pLocalPlayer && pLocalPlayer->entindex() == iShooterIndex )
		{
			CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictim ) );

			if ( pVictim && pVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_pinned" );

				if ( event )
				{
					gameeventmanager->FireEventClientSide( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void StickyBoltCallbackTF( const CEffectData &data )
{
	StickRagdollNowTF( 
		data.m_vOrigin, 
		data.m_vNormal, 
		data.m_hEntity, 
		data.m_nAttachmentIndex, 
		data.m_nMaterial, 
		data.m_nHitBox, 
		data.m_nDamageType, 
		data.m_nSurfaceProp, 
		data.m_fFlags, 
		data.m_nColor
	);
}

DECLARE_CLIENT_EFFECT( "TFBoltImpact", StickyBoltCallbackTF );