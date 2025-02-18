//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the Sticky Bolt code. This constraints ragdolls to the world
//			after being hit by a crossbow bolt. If something here is acting funny
//			let me know - Adrian.
//
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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

extern ITempEnts* tempents;

class CRagdollBoltEnumerator : public IPartitionEnumerator
{
public:
	//Forced constructor   
	CRagdollBoltEnumerator( Ray_t& shot, Vector vOrigin )
	{
		m_rayShot = shot;
		m_vWorld = vOrigin;
	}

	//Actual work code
	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

		if ( pModel == NULL )
			return ITERATION_CONTINUE;

		trace_t tr;
		enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

		IPhysicsObject	*pPhysicsObject = NULL;
		
		//Find the real object we hit.
		if( tr.physicsbone >= 0 )
		{
			if ( pModel->m_pRagdoll )
			{
				CRagdoll *pCRagdoll = dynamic_cast < CRagdoll * > ( pModel->m_pRagdoll );

				if ( pCRagdoll )
				{
					ragdoll_t *pRagdollT = pCRagdoll->GetRagdoll();

					if ( tr.physicsbone < pRagdollT->listCount )
					{
						pPhysicsObject = pRagdollT->list[tr.physicsbone].pObject;
					}
				}
			}
		}

		if ( pPhysicsObject == NULL )
			return ITERATION_CONTINUE;

		if ( tr.fraction < 1.0 )
		{
			IPhysicsObject *pReference = GetWorldPhysObject();

			if ( pReference == NULL || pPhysicsObject == NULL )
				 return ITERATION_CONTINUE;
			
			float flMass = pPhysicsObject->GetMass();
			pPhysicsObject->SetMass( flMass * 2 );

			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
		
			pReference->WorldToLocal( &ballsocket.constraintPosition[0], m_vWorld );
			pPhysicsObject->WorldToLocal( &ballsocket.constraintPosition[1], tr.endpos );
	
			physenv->CreateBallsocketConstraint( pReference, pPhysicsObject, NULL, ballsocket );

			//Play a sound
			CPASAttenuationFilter filter( pEnt );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  "Weapon_Crossbow.BoltSkewer";
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_NORM;
			ep.m_pOrigin = &pEnt->GetAbsOrigin();

			C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, ep );
	
			return ITERATION_STOP;
		}

		return ITERATION_CONTINUE;
	}

private:
	Ray_t	m_rayShot;
	Vector  m_vWorld;
};

void CreateCrossbowBolt( const Vector &vecOrigin, const Vector &vecDirection )
{
	model_t *pModel = (model_t *)engine->LoadModel( "models/crossbow_bolt.mdl" );

	QAngle vAngles;

	VectorAngles( vecDirection, vAngles );
	
	if ( gpGlobals->maxClients > 1 )
	{
		tempents->SpawnTempModel( pModel, vecOrigin - vecDirection * 8, vAngles, Vector(0, 0, 0 ), 30.0f, FTENT_NONE );
	}
	else
	{
		tempents->SpawnTempModel( pModel, vecOrigin - vecDirection * 8, vAngles, Vector(0, 0, 0 ), 1, FTENT_NEVERDIE );
	}
}

void StickRagdollNow( const Vector &vecOrigin, const Vector &vecDirection )
{
	Ray_t	shotRay;
	trace_t tr;
	
	UTIL_TraceLine( vecOrigin, vecOrigin + vecDirection * 16, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.surface.flags & SURF_SKY )
		return;

	Vector vecEnd = vecOrigin - vecDirection * 128;

	shotRay.Init( vecOrigin, vecEnd );

	CRagdollBoltEnumerator	ragdollEnum( shotRay, vecOrigin );
	::partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );
	
	CreateCrossbowBolt( vecOrigin, vecDirection );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StickyBoltCallback( const CEffectData &data )
{
	 StickRagdollNow( data.m_vOrigin, data.m_vNormal );
}

DECLARE_CLIENT_EFFECT( "BoltImpact", StickyBoltCallback );