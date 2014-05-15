//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "particle_property.h"
#include "utlvector.h"

#ifdef CLIENT_DLL

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "recvproxy.h"
#include "particles_new.h"
#include "engine/ivdebugoverlay.h"
#include "bone_setup.h"
#else

#include "baseentity.h"
#include "baseanimating.h"
#include "sendproxy.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef STAGING_ONLY
#ifdef TF_CLIENT_DLL
extern ConVar tf_unusual_effect_offset;
#endif
#endif

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC_NO_BASE( CParticleProperty )
	//		DEFINE_FIELD( m_pOuter, FIELD_CLASSPTR ),
END_DATADESC()

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Prediction
//-----------------------------------------------------------------------------
BEGIN_PREDICTION_DATA_NO_BASE( CParticleProperty )
	//DEFINE_PRED_FIELD( m_vecMins, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CParticleProperty, DT_ParticleProperty )
#ifdef CLIENT_DLL
//RecvPropVector( RECVINFO(m_vecMins), 0, RecvProxy_OBBMins ),
#else
//SendPropVector( SENDINFO(m_vecMins), 0, SPROP_NOSCALE),
#endif
END_NETWORK_TABLE()


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CParticleProperty::CParticleProperty()
{
	Init( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CParticleProperty::~CParticleProperty()
{
	// We're being removed. Call StopEmission() on any particle system
	// that has an unlimited number of particles to emit.
	StopEmission( NULL, false, true );
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CParticleProperty::Init( CBaseEntity *pEntity )
{
	m_pOuter = pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CParticleProperty::GetParticleAttachment( C_BaseEntity *pEntity, const char *pszAttachmentName, const char *pszParticleName )
{
	Assert( pEntity && pEntity->GetBaseAnimating() );
	if ( !pEntity || !pEntity->GetBaseAnimating() )
		return INVALID_PARTICLE_ATTACHMENT;

	// Find the attachment point index
	int iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
	if ( iAttachment == INVALID_PARTICLE_ATTACHMENT )
	{
		Warning("Model '%s' doesn't have attachment '%s' to attach particle system '%s' to.\n", STRING(pEntity->GetBaseAnimating()->GetModelName()), pszAttachmentName, pszParticleName );
	}

	return iAttachment;
}

//-----------------------------------------------------------------------------
// Purpose: Create a new particle system and attach it to our owner
//-----------------------------------------------------------------------------
CNewParticleEffect *CParticleProperty::Create( const char *pszParticleName, ParticleAttachment_t iAttachType, const char *pszAttachmentName )
{
	int iAttachment = GetParticleAttachment( GetOuter(), pszAttachmentName, pszParticleName );
	if ( iAttachment == INVALID_PARTICLE_ATTACHMENT )
		return NULL;

	// Create the system
	return Create( pszParticleName, iAttachType, iAttachment );
}
	  
//-----------------------------------------------------------------------------
// Purpose: Create a new particle system and attach it to our owner
//-----------------------------------------------------------------------------
static ConVar cl_particle_batch_mode( "cl_particle_batch_mode", "1" );
CNewParticleEffect *CParticleProperty::Create( const char *pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset )
{
	if ( GameRules() )
	{
		pszParticleName = GameRules()->TranslateEffectForVisionFilter( "particles", pszParticleName );
	}

	int nBatchMode = cl_particle_batch_mode.GetInt();
	CParticleSystemDefinition *pDef = g_pParticleSystemMgr->FindParticleSystem( pszParticleName );
	bool bRequestedBatch = ( nBatchMode == 2 ) || ( ( nBatchMode == 1 ) && pDef && pDef->ShouldBatch() ); 
	if ( ( iAttachType == PATTACH_CUSTOMORIGIN ) && bRequestedBatch )
	{
		int iIndex = FindEffect( pszParticleName );
		if ( iIndex >= 0 )
		{
			CNewParticleEffect *pEffect = m_ParticleEffects[iIndex].pParticleEffect.GetObject();
			pEffect->Restart();
			return pEffect;
		}
	}

	if ( !pDef )
	{
		AssertMsg( 0, "Attempting to create unknown particle system" );
		Warning( "Attempting to create unknown particle system '%s' \n", pszParticleName );
		return NULL;
	}

	int iIndex = m_ParticleEffects.AddToTail();
	ParticleEffectList_t *newEffect = &m_ParticleEffects[iIndex];
	newEffect->pParticleEffect = CNewParticleEffect::Create( m_pOuter, pDef );

	if ( !newEffect->pParticleEffect->IsValid() )
	{
		// Caused by trying to spawn an unregistered particle effect. Remove it.
		ParticleMgr()->RemoveEffect( newEffect->pParticleEffect.GetObject() );
		return NULL;
	}

	AddControlPoint( iIndex, 0, GetOuter(), iAttachType, iAttachmentPoint, vecOriginOffset );

	if ( m_pOuter )
	{
		m_pOuter->OnNewParticleEffect( pszParticleName, newEffect->pParticleEffect.GetObject() );
	}
	
	return newEffect->pParticleEffect.GetObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::AddControlPoint( CNewParticleEffect *pEffect, int iPoint, C_BaseEntity *pEntity, ParticleAttachment_t iAttachType, const char *pszAttachmentName, Vector vecOriginOffset )
{
	int iAttachment = INVALID_PARTICLE_ATTACHMENT;
	if ( pszAttachmentName )
	{
		iAttachment = GetParticleAttachment( pEntity, pszAttachmentName, pEffect->GetEffectName() );
	}

	for ( int i = 0; i < m_ParticleEffects.Count(); i++ )
	{
		if ( m_ParticleEffects[i].pParticleEffect == pEffect )
		{
			AddControlPoint( i, iPoint, pEntity, iAttachType, iAttachment, vecOriginOffset );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::AddControlPoint( int iEffectIndex, int iPoint, C_BaseEntity *pEntity, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset )
{
	Assert( iEffectIndex >= 0 && iEffectIndex < m_ParticleEffects.Count() );
	ParticleEffectList_t *pEffect = &m_ParticleEffects[iEffectIndex];
	Assert( pEffect->pControlPoints.Count() < MAX_PARTICLE_CONTROL_POINTS );

	// If the control point is already used, override it
	ParticleControlPoint_t *pNewPoint = NULL;
	int iIndex = iPoint;
	FOR_EACH_VEC( pEffect->pControlPoints, i )
	{
		if ( pEffect->pControlPoints[i].iControlPoint == iPoint )
		{
			pNewPoint = &pEffect->pControlPoints[i];
		}
	}

	if ( !pNewPoint )
	{
		iIndex = pEffect->pControlPoints.AddToTail();
		pNewPoint = &pEffect->pControlPoints[iIndex];
	}
	
	pNewPoint->iControlPoint = iPoint;
	pNewPoint->hEntity = pEntity;
	pNewPoint->iAttachType = iAttachType;
	pNewPoint->iAttachmentPoint = iAttachmentPoint;
	pNewPoint->vecOriginOffset = vecOriginOffset;

	UpdateParticleEffect( pEffect, true, iIndex );
}


//-----------------------------------------------------------------------------
// Used to replace a particle effect with a different one; attaches the control point updating to the new one
//-----------------------------------------------------------------------------
void CParticleProperty::ReplaceParticleEffect( CNewParticleEffect *pOldEffect, CNewParticleEffect *pNewEffect )
{
	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pOldEffect != m_ParticleEffects[i].pParticleEffect.GetObject() )
			continue;

		m_ParticleEffects[i].pParticleEffect = pNewEffect;
		UpdateParticleEffect( &m_ParticleEffects[i], true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set the parent of a given control point to the index of some other
//			control point.
//-----------------------------------------------------------------------------
void CParticleProperty::SetControlPointParent( int iEffectIndex, int whichControlPoint, int parentIdx )
{

}

//-----------------------------------------------------------------------------
// Purpose: Stop effects from emitting more particles. If no effect is 
//			specified, all effects attached to this entity are stopped.
//-----------------------------------------------------------------------------
void CParticleProperty::StopEmission( CNewParticleEffect *pEffect, bool bWakeOnStop, bool bDestroyAsleepSystems )
{
	// If we return from dormancy and are then told to stop emitting,
	// we should have died while dormant. Remove ourselves immediately.
	bool bRemoveInstantly = (m_iDormancyChangedAtFrame == gpGlobals->framecount);

	if ( pEffect )
	{
		if ( FindEffect( pEffect ) != -1 )
		{
			pEffect->StopEmission( false, bRemoveInstantly, bWakeOnStop );
		}
	}
	else
	{
		// Stop all effects
		float flNow = g_pParticleSystemMgr->GetLastSimulationTime();
		int nCount = m_ParticleEffects.Count();
		for ( int i = nCount-1; i >= 0; i-- )
		{
			CNewParticleEffect *pTmp = m_ParticleEffects[i].pParticleEffect.GetObject();
			bool bRemoveSystem = bRemoveInstantly || ( bDestroyAsleepSystems && ( flNow >= pTmp->m_flNextSleepTime ) );
			if ( bRemoveSystem )
			{
				m_ParticleEffects.Remove( i );
				pTmp->SetOwner( NULL );
			}
			pTmp->StopEmission( false, bRemoveSystem, !bRemoveSystem && bWakeOnStop );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove effects immediately, including all current particles. If no
// effect is specified, all effects attached to this entity are removed.
//-----------------------------------------------------------------------------
void CParticleProperty::StopEmissionAndDestroyImmediately( CNewParticleEffect *pEffect )
{
	if ( pEffect )
	{
		int iIndex = FindEffect( pEffect );
		//Assert( iIndex != -1 );
		if ( iIndex != -1 )
		{
			m_ParticleEffects.Remove( iIndex );

			// Clear the owner so it doesn't try to call back to us on deletion
			pEffect->SetOwner( NULL );
			pEffect->StopEmission( false, true );
		}
	}
	else
	{
		// Immediately destroy all effects
		int nCount = m_ParticleEffects.Count();
		for ( int i = nCount-1; i >= 0; i-- )
		{
			CNewParticleEffect *pTmp = m_ParticleEffects[i].pParticleEffect.GetObject();
			m_ParticleEffects.Remove( i );

			// Clear the owner so it doesn't try to call back to us on deletion
			pTmp->SetOwner( NULL );
			pTmp->StopEmission( false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop all effects that have  a control point associated with the given
//          entity.
//-----------------------------------------------------------------------------
void CParticleProperty::StopParticlesInvolving( CBaseEntity *pEntity )
{
	Assert( pEntity );

	EHANDLE entHandle(pEntity);

	// If we return from dormancy and are then told to stop emitting,
	// we should have died while dormant. Remove ourselves immediately.
	bool bRemoveInstantly = (m_iDormancyChangedAtFrame == gpGlobals->framecount);
	
	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		// for each effect...
		ParticleEffectList_t &part = m_ParticleEffects[i];
		// look through all the control points to see if any mention the given object
		int cpCount = part.pControlPoints.Count();
		for (int j = 0; j < cpCount ; ++j )
		{
			// if any control points respond to the given handle...
			if (part.pControlPoints[j].hEntity == entHandle)
			{
				part.pParticleEffect->StopEmission( false, bRemoveInstantly );
				part.pControlPoints.Remove( j );
				break; // break out of the inner loop (to where it says BREAK TO HERE)
			}
		}
		// BREAK TO HERE
	}
}

//g_pParticleSystemMgr->FindParticleSystem( pParticleSystemName );

//-----------------------------------------------------------------------------
// Purpose: Stop all effects that were created using the given definition
//			name.
//-----------------------------------------------------------------------------
void CParticleProperty::StopParticlesNamed( const char *pszEffectName, bool bForceRemoveInstantly /* =false */ )
{
	CParticleSystemDefinition *pDef = g_pParticleSystemMgr->FindParticleSystem( pszEffectName );
	AssertMsg1(pDef, "Could not find particle definition %s", pszEffectName );
	if (!pDef)
		return;


	// If we return from dormancy and are then told to stop emitting,
	// we should have died while dormant. Remove ourselves immediately.
	bool bRemoveInstantly = (m_iDormancyChangedAtFrame == gpGlobals->framecount);
	// force remove particles instantly if caller specified
	bRemoveInstantly |= bForceRemoveInstantly;

	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		// for each effect...
		CNewParticleEffect *pParticleEffect = m_ParticleEffects[i].pParticleEffect.GetObject();
		if (pParticleEffect->m_pDef() == pDef)
		{
			pParticleEffect->StopEmission( false, bRemoveInstantly );
		}
	}
}

void CParticleProperty::StopParticlesWithNameAndAttachment( const char *pszEffectName, int iAttachmentPoint, bool bForceRemoveInstantly /* =false */ )
{
	CParticleSystemDefinition *pDef = g_pParticleSystemMgr->FindParticleSystem( pszEffectName );
	AssertMsg1(pDef, "Could not find particle definition %s", pszEffectName );
	if (!pDef)
		return;


	// If we return from dormancy and are then told to stop emitting,
	// we should have died while dormant. Remove ourselves immediately.
	bool bRemoveInstantly = (m_iDormancyChangedAtFrame == gpGlobals->framecount);
	// force remove particles instantly if caller specified
	bRemoveInstantly |= bForceRemoveInstantly;

	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		// for each effect...
		ParticleEffectList_t *pParticleEffectList = &m_ParticleEffects[i];
		CNewParticleEffect *pParticleEffect = pParticleEffectList->pParticleEffect.GetObject();
		if (pParticleEffect->m_pDef() == pDef)
		{
			int nControlPointCount = pParticleEffectList->pControlPoints.Count();
			for ( int j = 0; j < nControlPointCount; ++j )
			{
				if ( pParticleEffectList->pControlPoints[j].iAttachmentPoint == iAttachmentPoint )
				{
					pParticleEffect->StopEmission( false, bRemoveInstantly );
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::OnParticleSystemUpdated( CNewParticleEffect *pEffect, float flTimeDelta )
{
	int iIndex = FindEffect( pEffect );
	Assert( iIndex != -1 );
	if ( iIndex == -1 )
		return;

	// Enable FP exceptions here when FP_EXCEPTIONS_ENABLED is defined,
	// to help track down bad math.
	FPExceptionEnabler enableExceptions;

	UpdateParticleEffect( &m_ParticleEffects[iIndex] );

	/*
	// Display the bounding box of the particle effect
	Vector vecMins, vecMaxs;
	pEffect->GetRenderBounds( vecMins, vecMaxs );
	debugoverlay->AddBoxOverlay( pEffect->GetRenderOrigin(), vecMins, vecMaxs, QAngle( 0, 0, 0 ), 0, 255, 255, 0, 0 );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::OnParticleSystemDeleted( CNewParticleEffect *pEffect )
{
	int iIndex = FindEffect( pEffect );
	if ( iIndex == -1 )
		return;

	m_ParticleEffects[iIndex].pParticleEffect.MarkDeleted();
	m_ParticleEffects.Remove( iIndex );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: The entity we're attached to has change dormancy state on our client
//-----------------------------------------------------------------------------
void CParticleProperty::OwnerSetDormantTo( bool bDormant )
{
	m_iDormancyChangedAtFrame = gpGlobals->framecount;

	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		//m_ParticleEffects[i].pParticleEffect->SetShouldSimulate( !bDormant );
		m_ParticleEffects[i].pParticleEffect->SetDormant( bDormant );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CParticleProperty::FindEffect( CNewParticleEffect *pEffect )
{
	for ( int i = 0; i < m_ParticleEffects.Count(); i++ )
	{
		if ( m_ParticleEffects[i].pParticleEffect == pEffect )
			return i;
	}

	return -1;
}

int CParticleProperty::FindEffect( const char *pEffectName, int nStart /*= 0*/ )
{
	for ( int i = nStart; i < m_ParticleEffects.Count(); i++ )
	{
		if ( !Q_stricmp( m_ParticleEffects[i].pParticleEffect->GetName(), pEffectName ) )
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::UpdateParticleEffect( ParticleEffectList_t *pEffect, bool bInitializing, int iOnlyThisControlPoint )
{
	if ( iOnlyThisControlPoint != -1 )
	{
		UpdateControlPoint( pEffect, iOnlyThisControlPoint, bInitializing );
		return;
	}

	// Loop through our control points and update them all
	for ( int i = 0; i < pEffect->pControlPoints.Count(); i++ )
	{
		UpdateControlPoint( pEffect, i, bInitializing );
	}
}

extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleProperty::UpdateControlPoint( ParticleEffectList_t *pEffect, int iPoint, bool bInitializing )
{
	ParticleControlPoint_t *pPoint = &pEffect->pControlPoints[iPoint];

	if ( !pPoint->hEntity.Get() )
	{
		if ( pPoint->iAttachType == PATTACH_WORLDORIGIN && bInitializing )
		{
			pEffect->pParticleEffect->SetControlPointOrientation( pPoint->iControlPoint, Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) );
			pEffect->pParticleEffect->SetControlPoint( pPoint->iControlPoint, pPoint->vecOriginOffset );
			pEffect->pParticleEffect->SetSortOrigin( pPoint->vecOriginOffset );
		}

		pEffect->pParticleEffect->SetControlPointEntity( pPoint->iControlPoint, NULL );
		return;
	}

	// Only update non-follow particles when we're initializing, 
	// unless we're parented to something, in which case we should always update
	if ( !bInitializing && !pPoint->hEntity->GetMoveParent() && (pPoint->iAttachType == PATTACH_ABSORIGIN || pPoint->iAttachType == PATTACH_POINT ) )
		return;

	if ( pPoint->iAttachType == PATTACH_CUSTOMORIGIN )
		return;

	Vector vecOrigin, vecForward, vecRight, vecUp;

	float flOffset = 0.0f;
	bool bUsingHeadOrigin = false;

#ifdef TF_CLIENT_DLL

	CBaseEntity *pWearable = (CBaseEntity*) pPoint->hEntity.Get();
	if ( pWearable && dynamic_cast<IHasAttributes*>( pWearable ) && !pWearable->IsPlayer() )
	{
		C_BaseAnimating *pAnimating = pPoint->hEntity->GetBaseAnimating();
		if ( pAnimating )
		{
			int bUseHeadOrigin = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pAnimating, bUseHeadOrigin, particle_effect_use_head_origin );
			if ( bUseHeadOrigin > 0 )
			{
				int iBone = Studio_BoneIndexByName( pAnimating->GetModelPtr(), "bip_head" );
				if ( iBone < 0 )
				{
					iBone = Studio_BoneIndexByName( pAnimating->GetModelPtr(), "prp_helmet" );
					if ( iBone < 0 )
					{
						iBone = Studio_BoneIndexByName( pAnimating->GetModelPtr(), "prp_hat" );
					}
				}
				if ( iBone < 0 )
				{
					iBone = 0;
				}

				bUsingHeadOrigin = true;
				const matrix3x4_t headBone = pAnimating->GetBone( iBone );
				MatrixVectors( headBone, &vecForward, &vecRight, &vecUp );
				MatrixPosition( headBone, vecOrigin );

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pAnimating, flOffset, particle_effect_vertical_offset );	
			}
		}
	}
#endif

	if ( !bUsingHeadOrigin )
	{
		switch ( pPoint->iAttachType )
		{
		case PATTACH_POINT:
		case PATTACH_POINT_FOLLOW:
			{
				C_BaseAnimating *pAnimating = pPoint->hEntity->GetBaseAnimating();

				Assert( pAnimating );
				if ( pAnimating )
				{
					matrix3x4_t attachmentToWorld;

					if ( !pAnimating->GetAttachment( pPoint->iAttachmentPoint, attachmentToWorld ) )
					{
						// try C_BaseAnimating if attach point is not on the weapon
						if ( !pAnimating->C_BaseAnimating::GetAttachment( pPoint->iAttachmentPoint, attachmentToWorld ) )
						{
							Warning( "Cannot update control point %d for effect '%s'.\n", pPoint->iAttachmentPoint, pEffect->pParticleEffect->GetEffectName() );
							attachmentToWorld = pAnimating->RenderableToWorldTransform();
						}
					}

					VMatrix vMat(attachmentToWorld);
					MatrixTranslate( vMat, pPoint->vecOriginOffset );
					MatrixVectors( vMat.As3x4(), &vecForward, &vecRight, &vecUp );
					MatrixPosition( vMat.As3x4(), vecOrigin );

					if ( pEffect->pParticleEffect->m_pDef->IsViewModelEffect() )
					{
						FormatViewModelAttachment( vecOrigin, true );
					}
				}
			}
			break;

		case PATTACH_ABSORIGIN:
		case PATTACH_ABSORIGIN_FOLLOW:
		default:
			{
				vecOrigin = pPoint->hEntity->GetAbsOrigin() + pPoint->vecOriginOffset;
				pPoint->hEntity->GetVectors( &vecForward, &vecRight, &vecUp );
			}
			break;

		case PATTACH_ROOTBONE_FOLLOW:
			{
				C_BaseAnimating *pAnimating = pPoint->hEntity->GetBaseAnimating();

				Assert( pAnimating );
				if ( pAnimating )
				{
					matrix3x4_t rootBone;
					if ( pAnimating->GetRootBone( rootBone ) )
					{
						MatrixVectors( rootBone, &vecForward, &vecRight, &vecUp );
						MatrixPosition( rootBone, vecOrigin );
					}
				}
			}
			break;
		}
	}

	Vector vecForcedOriginOffset( 0, 0, flOffset );
	pEffect->pParticleEffect->SetControlPointOrientation( pPoint->iControlPoint, vecForward, vecRight, vecUp );
	pEffect->pParticleEffect->SetControlPointEntity( pPoint->iControlPoint, pPoint->hEntity );
	pEffect->pParticleEffect->SetControlPoint( pPoint->iControlPoint, vecOrigin + vecForcedOriginOffset );
	pEffect->pParticleEffect->SetSortOrigin( vecOrigin + vecForcedOriginOffset);
}

//-----------------------------------------------------------------------------
// Purpose: Output all active effects
//-----------------------------------------------------------------------------
void CParticleProperty::DebugPrintEffects( void )
{
	int nCount = m_ParticleEffects.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		// for each effect...
		CNewParticleEffect *pParticleEffect = m_ParticleEffects[i].pParticleEffect.GetObject();

		if ( !pParticleEffect )
			continue;
	
		Msg( "(%d)  EffectName \"%s\"  Dormant? %s  Emission Stopped? %s \n",
			i,
			pParticleEffect->GetEffectName(),
			( pParticleEffect->m_bDormant ) ? "yes" : "no",
			( pParticleEffect->m_bEmissionStopped ) ? "yes" : "no" );
	}
}
