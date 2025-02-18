//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_baseanimating.h"
#include "c_sprite.h"
#include "model_types.h"
#include "bone_setup.h"
#include "ivrenderview.h"
#include "r_efx.h"
#include "dlight.h"
#include "beamdraw.h"
#include "cl_animevent.h"
#include "engine/IEngineSound.h"
#include "c_te_legacytempents.h"
#include "activitylist.h"
#include "animation.h"
#include "tier0/vprof.h"
#include "clienteffectprecachesystem.h"
#include "IEffects.h"
#include "engine/ivmodelinfo.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include <KeyValues.h>
#include "c_rope.h"
#include "isaverestore.h"
#include "datacache/imdlcache.h"
#include "eventlist.h"
#include "saverestore.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "ragdoll_shared.h"
#include "view.h"
#include "c_ai_basenpc.h"
#include "c_entitydissolve.h"
#include "saverestoretypes.h"
#include "c_fire_smoke.h"
#include "input.h"
#include "soundinfo.h"
#include "tools/bonelist.h"
#include "toolframework/itoolframework.h"
#include "datacache/idatacache.h"
#include "gamestringpool.h"
#include "jigglebones.h"
#include "toolframework_client.h"
#include "vstdlib/jobthread.h"
#include "bonetoworldarray.h"
#include "posedebugger.h"
#include "tier0/icommandline.h"
#include "prediction.h"
#include "replay/replay_ragdoll.h"
#include "studio_stats.h"
#include "tier1/callqueue.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_player.h"
#include "c_baseobject.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cl_SetupAllBones( "cl_SetupAllBones", "0" );
ConVar r_sequence_debug( "r_sequence_debug", "" );

// If an NPC is moving faster than this, he should play the running footstep sound
const float RUN_SPEED_ESTIMATE_SQR = 150.0f * 150.0f;

// Removed macro used by shared code stuff
#if defined( CBaseAnimating )
#undef CBaseAnimating
#endif


#ifdef DEBUG
static ConVar dbganimmodel( "dbganimmodel", "" );
#endif

#if defined( STAGING_ONLY )
	static ConVar dbg_bonestack_perturb( "dbg_bonestack_perturb", "0", 0);
	static CInterlockedInt dbg_bonestack_reentrant_count = 0;
#endif // STAGING_ONLY

mstudioevent_t *GetEventIndexForSequence( mstudioseqdesc_t &seqdesc );

C_EntityDissolve *DissolveEffect( C_BaseEntity *pTarget, float flTime );
C_EntityFlame *FireEffect( C_BaseAnimating *pTarget, C_BaseEntity *pServerFire, float *flScaleEnd, float *flTimeStart, float *flTimeEnd );
bool NPC_IsImportantNPC( C_BaseAnimating *pAnimating );
void VCollideWireframe_ChangeCallback( IConVar *pConVar, char const *pOldString, float flOldValue );

ConVar vcollide_wireframe( "vcollide_wireframe", "0", FCVAR_CHEAT, "Render physics collision models in wireframe", VCollideWireframe_ChangeCallback );

bool C_AnimationLayer::IsActive( void )
{
	return (m_nOrder != C_BaseAnimatingOverlay::MAX_OVERLAYS);
}

//-----------------------------------------------------------------------------
// Relative lighting entity
//-----------------------------------------------------------------------------
class C_InfoLightingRelative : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_InfoLightingRelative, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	void GetLightingOffset( matrix3x4_t &offset );

private:
	EHANDLE			m_hLightingLandmark;
};

IMPLEMENT_CLIENTCLASS_DT(C_InfoLightingRelative, DT_InfoLightingRelative, CInfoLightingRelative)
	RecvPropEHandle(RECVINFO(m_hLightingLandmark)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Relative lighting entity
//-----------------------------------------------------------------------------
void C_InfoLightingRelative::GetLightingOffset( matrix3x4_t &offset )
{
	if ( m_hLightingLandmark.Get() )
	{
		matrix3x4_t matWorldToLandmark;
 		MatrixInvert( m_hLightingLandmark->EntityToWorldTransform(), matWorldToLandmark );
		ConcatTransforms( EntityToWorldTransform(), matWorldToLandmark, offset );
	}
	else
	{
		SetIdentityMatrix( offset );
	}
}


//-----------------------------------------------------------------------------
// Base Animating
//-----------------------------------------------------------------------------

struct clientanimating_t
{
	C_BaseAnimating *pAnimating;
	unsigned int	flags;
	clientanimating_t(C_BaseAnimating *_pAnim, unsigned int _flags ) : pAnimating(_pAnim), flags(_flags) {}
};

const unsigned int FCLIENTANIM_SEQUENCE_CYCLE = 0x00000001;

static CUtlVector< clientanimating_t >	g_ClientSideAnimationList;

BEGIN_RECV_TABLE_NOBASE( C_BaseAnimating, DT_ServerAnimationData )
	RecvPropFloat(RECVINFO(m_flCycle)),
END_RECV_TABLE()


void RecvProxy_Sequence( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// Have the regular proxy store the data.
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	C_BaseAnimating *pAnimating = (C_BaseAnimating *)pStruct;

	if ( !pAnimating )
		return;

	pAnimating->SetReceivedSequence();

	// render bounds may have changed
	pAnimating->UpdateVisibility();
}

IMPLEMENT_CLIENTCLASS_DT(C_BaseAnimating, DT_BaseAnimating, CBaseAnimating)
	RecvPropInt(RECVINFO(m_nSequence), 0, RecvProxy_Sequence),
	RecvPropInt(RECVINFO(m_nForceBone)),
	RecvPropVector(RECVINFO(m_vecForce)),
	RecvPropInt(RECVINFO(m_nSkin)),
	RecvPropInt(RECVINFO(m_nBody)),
	RecvPropInt(RECVINFO(m_nHitboxSet)),

	RecvPropFloat(RECVINFO(m_flModelScale)),
	RecvPropFloat(RECVINFO_NAME(m_flModelScale, m_flModelWidthScale)), // for demo compatibility only

//	RecvPropArray(RecvPropFloat(RECVINFO(m_flPoseParameter[0])), m_flPoseParameter),
	RecvPropArray3(RECVINFO_ARRAY(m_flPoseParameter), RecvPropFloat(RECVINFO(m_flPoseParameter[0])) ),
	
	RecvPropFloat(RECVINFO(m_flPlaybackRate)),

	RecvPropArray3( RECVINFO_ARRAY(m_flEncodedController), RecvPropFloat(RECVINFO(m_flEncodedController[0]))),

	RecvPropInt( RECVINFO( m_bClientSideAnimation )),
	RecvPropInt( RECVINFO( m_bClientSideFrameReset )),

	RecvPropInt( RECVINFO( m_nNewSequenceParity )),
	RecvPropInt( RECVINFO( m_nResetEventsParity )),
	RecvPropInt( RECVINFO( m_nMuzzleFlashParity ) ),

	RecvPropEHandle(RECVINFO(m_hLightingOrigin)),
	RecvPropEHandle(RECVINFO(m_hLightingOriginRelative)),

	RecvPropDataTable( "serveranimdata", 0, 0, &REFERENCE_RECV_TABLE( DT_ServerAnimationData ) ),

	RecvPropFloat( RECVINFO( m_fadeMinDist ) ), 
	RecvPropFloat( RECVINFO( m_fadeMaxDist ) ), 
	RecvPropFloat( RECVINFO( m_flFadeScale ) ), 

END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseAnimating )

	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( m_nHitboxSet, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( m_flModelScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
//	DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_INSENDTABLE, 0.02f ),

	DEFINE_FIELD( m_nPrevSequence, FIELD_INTEGER ),
	//DEFINE_FIELD( m_flPrevEventCycle, FIELD_FLOAT ),
	//DEFINE_FIELD( m_flEventCycle, FIELD_FLOAT ),
	//DEFINE_FIELD( m_nEventSequence, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	// DEFINE_PRED_FIELD( m_nPrevResetEventsParity, FIELD_INTEGER, 0 ),

	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),
	//DEFINE_FIELD( m_nOldMuzzleFlashParity, FIELD_CHARACTER ),

	//DEFINE_FIELD( m_nPrevNewSequenceParity, FIELD_INTEGER ),
	//DEFINE_FIELD( m_nPrevResetEventsParity, FIELD_INTEGER ),

	// DEFINE_PRED_FIELD( m_vecForce, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( m_nForceBone, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( m_bClientSideAnimation, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	// DEFINE_PRED_FIELD( m_bClientSideFrameReset, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	
	// DEFINE_FIELD( m_pRagdollInfo, RagdollInfo_t ),
	// DEFINE_FIELD( m_CachedBones, CUtlVector < CBoneCacheEntry > ),
	// DEFINE_FIELD( m_pActualAttachmentAngles, FIELD_VECTOR ),
	// DEFINE_FIELD( m_pActualAttachmentOrigin, FIELD_VECTOR ),

	// DEFINE_FIELD( m_animationQueue, CUtlVector < C_AnimationLayer > ),
	// DEFINE_FIELD( m_pIk, CIKContext ),
	// DEFINE_FIELD( m_bLastClientSideFrameReset, FIELD_BOOLEAN ),
	// DEFINE_FIELD( hdr, studiohdr_t ),
	// DEFINE_FIELD( m_pRagdoll, IRagdoll ),
	// DEFINE_FIELD( m_bStoreRagdollInfo, FIELD_BOOLEAN ),

	// DEFINE_FIELD( C_BaseFlex, m_iEyeAttachment, FIELD_INTEGER ),

END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( client_ragdoll, C_ClientRagdoll );

BEGIN_DATADESC( C_ClientRagdoll )
	DEFINE_FIELD( m_bFadeOut, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bImportant, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iCurrentFriction, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMinFriction, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMaxFriction, FIELD_INTEGER ),
	DEFINE_FIELD( m_flFrictionModTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flFrictionTime, FIELD_TIME ),
	DEFINE_FIELD( m_iFrictionAnimState, FIELD_INTEGER ),
	DEFINE_FIELD( m_bReleaseRagdoll, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nBody, FIELD_INTEGER ),
	DEFINE_FIELD( m_nSkin, FIELD_INTEGER ),
	DEFINE_FIELD( m_nRenderFX, FIELD_CHARACTER ),
	DEFINE_FIELD( m_nRenderMode, FIELD_CHARACTER ),
	DEFINE_FIELD( m_clrRender, FIELD_COLOR32 ),
	DEFINE_FIELD( m_flEffectTime, FIELD_TIME ),
	DEFINE_FIELD( m_bFadingOut, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY( m_flScaleEnd, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( m_flScaleTimeStart, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( m_flScaleTimeEnd, FIELD_FLOAT ),
	DEFINE_EMBEDDEDBYREF( m_pRagdoll ),

END_DATADESC()

C_ClientRagdoll::C_ClientRagdoll( bool bRestoring )
{
	m_iCurrentFriction = 0;
	m_iFrictionAnimState = RAGDOLL_FRICTION_NONE;
	m_bReleaseRagdoll = false;
	m_bFadeOut = false;
	m_bFadingOut = false;
	m_bImportant = false;
	m_bNoModelParticles = false;

	SetClassname("client_ragdoll");

	if ( bRestoring == true )
	{
		m_pRagdoll = new CRagdoll;
	}
}

void C_ClientRagdoll::OnSave( void )
{
}

void C_ClientRagdoll::OnRestore( void )
{
	CStudioHdr *hdr = GetModelPtr();

	if ( hdr == NULL )
	{
		const char *pModelName = STRING( GetModelName() );
		SetModel( pModelName );

		hdr = GetModelPtr();

		if ( hdr == NULL )
			return;
	}
	
	if ( m_pRagdoll == NULL )
		 return;

	ragdoll_t *pRagdollT = m_pRagdoll->GetRagdoll();

	if ( pRagdollT == NULL || pRagdollT->list[0].pObject == NULL )
	{
		m_bReleaseRagdoll = true;
		m_pRagdoll = NULL;
		Assert( !"Attempted to restore a ragdoll without physobjects!" );
		return;
	}

	if ( GetFlags() & FL_DISSOLVING )
	{
		DissolveEffect( this, m_flEffectTime );
	}
	else if ( GetFlags() & FL_ONFIRE )
	{
		C_EntityFlame *pFireChild = dynamic_cast<C_EntityFlame *>( GetEffectEntity() );
		C_EntityFlame *pNewFireChild = FireEffect( this, pFireChild, m_flScaleEnd, m_flScaleTimeStart, m_flScaleTimeEnd );

		//Set the new fire child as the new effect entity.
		SetEffectEntity( pNewFireChild );
	}

	VPhysicsSetObject( NULL );
	VPhysicsSetObject( pRagdollT->list[0].pObject );

	SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

	pRagdollT->list[0].parentIndex = -1;
	pRagdollT->list[0].originParentSpace.Init();

	RagdollActivate( *pRagdollT, modelinfo->GetVCollide( GetModelIndex() ), GetModelIndex(), true );
	RagdollSetupAnimatedFriction( physenv, pRagdollT, GetModelIndex() );

	m_pRagdoll->BuildRagdollBounds( this );

	// UNDONE: The shadow & leaf system cleanup should probably be in C_BaseEntity::OnRestore()
	// this must be recomputed because the model was NULL when this was set up
	RemoveFromLeafSystem();
	AddToLeafSystem( RENDER_GROUP_OPAQUE_ENTITY );

	DestroyShadow();
 	CreateShadow();

	SetNextClientThink( CLIENT_THINK_ALWAYS );
	
	if ( m_bFadeOut == true )
	{
		s_RagdollLRU.MoveToTopOfLRU( this, m_bImportant );
	}

	NoteRagdollCreationTick( this );
	
	BaseClass::OnRestore();

	RagdollMoved();
}

void C_ClientRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_ClientRagdoll::ImpactTrace" );

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

ConVar g_debug_ragdoll_visualize( "g_debug_ragdoll_visualize", "0", FCVAR_CHEAT );

void C_ClientRagdoll::HandleAnimatedFriction( void )
{
	if ( m_iFrictionAnimState == RAGDOLL_FRICTION_OFF )
		 return;

	ragdoll_t *pRagdollT = NULL;
	int iBoneCount = 0;

	if ( m_pRagdoll )
	{
		pRagdollT = m_pRagdoll->GetRagdoll();
		iBoneCount = m_pRagdoll->RagdollBoneCount();

	}

	if ( pRagdollT == NULL )
		 return;
	
	switch ( m_iFrictionAnimState )
	{
		case RAGDOLL_FRICTION_NONE:
		{
			m_iMinFriction = pRagdollT->animfriction.iMinAnimatedFriction;
			m_iMaxFriction = pRagdollT->animfriction.iMaxAnimatedFriction;

			if ( m_iMinFriction != 0 || m_iMaxFriction != 0 )
			{
				m_iFrictionAnimState = RAGDOLL_FRICTION_IN;

				m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeIn;
				m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;
				
				m_iCurrentFriction = m_iMinFriction;
			}
			else
			{
				m_iFrictionAnimState = RAGDOLL_FRICTION_OFF;
			}
			
			break;
		}

		case RAGDOLL_FRICTION_IN:
		{
			float flDeltaTime = (m_flFrictionTime - gpGlobals->curtime);

			m_iCurrentFriction = RemapValClamped( flDeltaTime , m_flFrictionModTime, 0, m_iMinFriction, m_iMaxFriction );

			if ( flDeltaTime <= 0.0f )
			{
				m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeHold;
				m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;
				m_iFrictionAnimState = RAGDOLL_FRICTION_HOLD;
			}
			break;
		}

		case RAGDOLL_FRICTION_HOLD:
		{
			if ( m_flFrictionTime < gpGlobals->curtime )
			{
				m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeOut;
				m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;
				m_iFrictionAnimState = RAGDOLL_FRICTION_OUT;
			}
			
			break;
		}

		case RAGDOLL_FRICTION_OUT:
		{
			float flDeltaTime = (m_flFrictionTime - gpGlobals->curtime);

			m_iCurrentFriction = RemapValClamped( flDeltaTime , 0, m_flFrictionModTime, m_iMinFriction, m_iMaxFriction );

			if ( flDeltaTime <= 0.0f )
			{
				m_iFrictionAnimState = RAGDOLL_FRICTION_OFF;
			}

			break;
		}
	}

	for ( int i = 0; i < iBoneCount; i++ )
	{
		if ( pRagdollT->list[i].pConstraint )
			 pRagdollT->list[i].pConstraint->SetAngularMotor( 0, m_iCurrentFriction );
	}

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if ( pPhysicsObject )
	{
			pPhysicsObject->Wake();
	}
}

ConVar g_ragdoll_fadespeed( "g_ragdoll_fadespeed", "600" );
ConVar g_ragdoll_lvfadespeed( "g_ragdoll_lvfadespeed", "100" );

void C_ClientRagdoll::OnPVSStatusChanged( bool bInPVS )
{
	if ( bInPVS )
	{
		CreateShadow();
	}
	else
	{
		DestroyShadow();
	}
}

void C_ClientRagdoll::FadeOut( void )
{
	if ( m_bFadingOut == false )
	{
		 return;
	}

	int iAlpha = GetRenderColor().a;
	int iFadeSpeed = ( g_RagdollLVManager.IsLowViolence() ) ? g_ragdoll_lvfadespeed.GetInt() : g_ragdoll_fadespeed.GetInt();

	iAlpha = MAX( iAlpha - ( iFadeSpeed * gpGlobals->frametime ), 0 );

	SetRenderMode( kRenderTransAlpha );
	SetRenderColorA( iAlpha );

	if ( iAlpha == 0 )
	{
		m_bReleaseRagdoll = true;
	}
}

void C_ClientRagdoll::SUB_Remove( void )
{
	m_bFadingOut = true;
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_ClientRagdoll::ClientThink( void )
{
	if ( m_bReleaseRagdoll == true )
	{
		DestroyBoneAttachments();
		Release();
		return;
	}

	if ( g_debug_ragdoll_visualize.GetBool() )
	{
		Vector vMins, vMaxs;
			
		Vector origin = m_pRagdoll->GetRagdollOrigin();
		m_pRagdoll->GetRagdollBounds( vMins, vMaxs );

		debugoverlay->AddBoxOverlay( origin, vMins, vMaxs, QAngle( 0, 0, 0 ), 0, 255, 0, 16, 0 );
	}

	HandleAnimatedFriction();

	FadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
float C_ClientRagdoll::LastBoneChangedTime()
{
	// When did this last change?
	return m_pRagdoll ? m_pRagdoll->GetLastVPhysicsUpdateTime() : -FLT_MAX;
}


//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_ClientRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	int nFlexDescCount = hdr->numflexdesc();
	if ( nFlexDescCount )
	{
		Assert( !pFlexDelayedWeights );
		memset( pFlexWeights, 0, nFlexWeightCount * sizeof(float) );
	}

	if ( m_iEyeAttachment > 0 )
	{
		matrix3x4_t attToWorld;
		if ( GetAttachment( m_iEyeAttachment, attToWorld ) )
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
		}
	}
}

void C_ClientRagdoll::Release( void )
{
	C_BaseEntity *pChild = GetEffectEntity();

	if ( pChild && pChild->IsMarkedForDeletion() == false )
	{
		pChild->Release();
	}

	if ( GetThinkHandle() != INVALID_THINK_HANDLE )
	{
		ClientThinkList()->RemoveThinkable( GetClientHandle() );
	}
	ClientEntityList().RemoveEntity( GetClientHandle() );

	if ( CollisionProp()->GetPartitionHandle() != PARTITION_INVALID_HANDLE )
	{
		partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle() );
	}
	RemoveFromLeafSystem();

	BaseClass::Release();
}

//-----------------------------------------------------------------------------
// Incremented each frame in InvalidateModelBones. Models compare this value to what it
// was last time they setup their bones to determine if they need to re-setup their bones.
static unsigned long	g_iModelBoneCounter = 0;
CUtlVector<C_BaseAnimating *> g_PreviousBoneSetups;
static unsigned long	g_iPreviousBoneCounter = (unsigned)-1;

class C_BaseAnimatingGameSystem : public CAutoGameSystem
{
	void LevelShutdownPostEntity()
	{
		g_iPreviousBoneCounter = (unsigned)-1;
		if ( g_PreviousBoneSetups.Count() != 0 )
		{
			Msg( "%d entities in bone setup array. Should have been cleaned up by now\n", g_PreviousBoneSetups.Count() );
			g_PreviousBoneSetups.RemoveAll();
		}
	}
} g_BaseAnimatingGameSystem;


//-----------------------------------------------------------------------------
// Purpose: convert axis rotations to a quaternion
//-----------------------------------------------------------------------------
C_BaseAnimating::C_BaseAnimating() :
	m_iv_flCycle( "C_BaseAnimating::m_iv_flCycle" ),
	m_iv_flPoseParameter( "C_BaseAnimating::m_iv_flPoseParameter" ),
	m_iv_flEncodedController("C_BaseAnimating::m_iv_flEncodedController")
{
	m_vecForce.Init();
	m_nForceBone = -1;
	
	m_ClientSideAnimationListHandle = INVALID_CLIENTSIDEANIMATION_LIST_HANDLE;

	m_nPrevSequence = -1;
	m_nRestoreSequence = -1;
	m_pRagdoll		= NULL;
	m_builtRagdoll = false;
	m_hitboxBoneCacheHandle = 0;
	int i;
	for ( i = 0; i < ARRAYSIZE( m_flEncodedController ); i++ )
	{
		m_flEncodedController[ i ] = 0.0f;
	}

	AddBaseAnimatingInterpolatedVars();

	m_iMostRecentModelBoneCounter = 0xFFFFFFFF;
	m_iMostRecentBoneSetupRequest = g_iPreviousBoneCounter - 1;
	m_flLastBoneSetupTime = -FLT_MAX;

	m_vecPreRagdollMins = vec3_origin;
	m_vecPreRagdollMaxs = vec3_origin;

	m_bStoreRagdollInfo = false;
	m_pRagdollInfo = NULL;

	m_flPlaybackRate = 1.0f;

	m_nEventSequence = -1;

	m_pIk = NULL;

	// Assume false.  Derived classes might fill in a receive table entry
	// and in that case this would show up as true
	m_bClientSideAnimation = false;

	m_nPrevNewSequenceParity = -1;
	m_nPrevResetEventsParity = -1;

	m_nOldMuzzleFlashParity = 0;
	m_nMuzzleFlashParity = 0;

	m_flModelScale = 1.0f;

	m_iEyeAttachment = 0;
#ifdef _XBOX
	m_iAccumulatedBoneMask = 0;
#endif
	m_pStudioHdr = NULL;
	m_hStudioHdr = MDLHANDLE_INVALID;

	m_bReceivedSequence = false;

	m_boneIndexAttached = -1;
	m_flOldModelScale = 0.0f;

	m_pAttachedTo = NULL;

	m_bDynamicModelAllowed = false;
	m_bDynamicModelPending = false;
	m_bResetSequenceInfoOnLoad = false;

	Q_memset(&m_mouth, 0, sizeof(m_mouth));
	m_flCycle = 0;
	m_flOldCycle = 0;
}

//-----------------------------------------------------------------------------
// Purpose: cleanup
//-----------------------------------------------------------------------------
C_BaseAnimating::~C_BaseAnimating()
{
	int i = g_PreviousBoneSetups.Find( this );
	if ( i != -1 )
		g_PreviousBoneSetups.FastRemove( i );

	TermRopes();

	Assert( !m_pRagdoll );

	delete m_pRagdollInfo;
	m_pRagdollInfo = NULL;

	delete m_pIk;
	m_pIk = NULL;

	delete m_pBoneMergeCache;
	m_pBoneMergeCache = NULL;

	Studio_DestroyBoneCache( m_hitboxBoneCacheHandle );

	delete m_pJiggleBones;
	m_pJiggleBones = NULL;

	InvalidateMdlCache();

	// Kill off anything bone attached to us.
	DestroyBoneAttachments();

	// If we are bone attached to something, remove us from the list.
	if ( m_pAttachedTo )
	{
		m_pAttachedTo->RemoveBoneAttachment( this );
		m_pAttachedTo = NULL;
	}
}

bool C_BaseAnimating::UsesPowerOfTwoFrameBufferTexture( void )
{
	return modelinfo->IsUsingFBTexture( GetModel(), GetSkin(), GetBody(), GetClientRenderable() );
}

//-----------------------------------------------------------------------------
// VPhysics object
//-----------------------------------------------------------------------------
int C_BaseAnimating::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	if ( IsRagdoll() )
	{
		int i;
		for ( i = 0; i < m_pRagdoll->RagdollBoneCount(); ++i )
		{
			if ( i >= listMax )
				break;

			pList[i] = m_pRagdoll->GetElement(i);
		}
		return i;
	}

	return BaseClass::VPhysicsGetObjectList( pList, listMax );
}


//-----------------------------------------------------------------------------
// Should this object cast render-to-texture shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_BaseAnimating::ShadowCastType()
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	if (pStudioHdr->GetNumSeq() == 0)
		return SHADOWS_RENDER_TO_TEXTURE;
		  
	if ( !IsRagdoll() )
	{
		// If we have pose parameters, always update
		if ( pStudioHdr->GetNumPoseParameters() > 0 )
			return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
		
		// If we have bone controllers, always update
		if ( pStudioHdr->numbonecontrollers() > 0 )
			return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;

		// If we use IK, always update
		if ( pStudioHdr->numikchains() > 0 )
			return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
	}

	// FIXME: Do something to check to see how many frames the current animation has
	// If we do this, we have to be able to handle the case of changing ShadowCastTypes
	// at the moment, they are assumed to be constant.
	return SHADOWS_RENDER_TO_TEXTURE;
}

//-----------------------------------------------------------------------------
// Purpose: convert axis rotations to a quaternion
//-----------------------------------------------------------------------------

void C_BaseAnimating::SetPredictable( bool state )
{
	BaseClass::SetPredictable( state );

	UpdateRelevantInterpolatedVars();
}

//-----------------------------------------------------------------------------
// Purpose: sets client side animation
//-----------------------------------------------------------------------------
void C_BaseAnimating::UseClientSideAnimation()
{
	m_bClientSideAnimation = true;
}

void C_BaseAnimating::UpdateRelevantInterpolatedVars()
{
	MDLCACHE_CRITICAL_SECTION();
	// Remove any interpolated vars that need to be removed.
	if ( !IsMarkedForDeletion() && !GetPredictable() && !IsClientCreated() && GetModelPtr() && GetModelPtr()->SequencesAvailable() )
	{
		AddBaseAnimatingInterpolatedVars();
	}			
	else
	{
		RemoveBaseAnimatingInterpolatedVars();
	}
}


void C_BaseAnimating::AddBaseAnimatingInterpolatedVars()
{
	AddVar( m_flEncodedController, &m_iv_flEncodedController, LATCH_ANIMATION_VAR, true );
	AddVar( m_flPoseParameter, &m_iv_flPoseParameter, LATCH_ANIMATION_VAR, true );
	
	int flags = LATCH_ANIMATION_VAR;
	if ( m_bClientSideAnimation )
		flags |= EXCLUDE_AUTO_INTERPOLATE;
		
	AddVar( &m_flCycle, &m_iv_flCycle, flags, true );
}

void C_BaseAnimating::RemoveBaseAnimatingInterpolatedVars()
{
	RemoveVar( m_flEncodedController, false );
	RemoveVar( m_flPoseParameter, false );

#ifdef HL2MP
	// HACK:  Don't want to remove interpolation for predictables in hl2dm, though
	// The animation state stuff sets the pose parameters -- so they should interp
	//  but m_flCycle is not touched, so it's only set during prediction (which occurs on tick boundaries)
	//  and so needs to continue to be interpolated for smooth rendering of the lower body of the local player in third person, etc.
	if ( !GetPredictable() )
#endif
	{
		RemoveVar( &m_flCycle, false );
	}
}

/*
 From Ken: Lock() and Unlock() are render frame only, it’s just so the mdlcache
 doesn’t toss the memory when it reshuffles the data, or at least used to. I
 don't have any idea if mdlcache even does that anymore, but at one point it would
 happily throw away the animation data if you ran out of memory on the
 consoles. Jay adds: Ken is correct and the pointer should be valid until the end
 of the frame lock (provided you are within a MDLCACHE_LOCK() block or whatever 
 
 Jay also recommends running with a forced small cache size (1MB) to put maximum
 pressure on the cache when testing changes. Look for datacache ConVar in datacache.cpp.
 */
void C_BaseAnimating::LockStudioHdr()
{
	Assert( m_hStudioHdr == MDLHANDLE_INVALID && m_pStudioHdr == NULL );
	
	AUTO_LOCK( m_StudioHdrInitLock );

	if ( m_hStudioHdr != MDLHANDLE_INVALID || m_pStudioHdr != NULL )
	{
		Assert( m_pStudioHdr ? m_pStudioHdr->GetRenderHdr() == mdlcache->GetStudioHdr(m_hStudioHdr) : m_hStudioHdr == MDLHANDLE_INVALID );
		return;
	}

	const model_t *mdl = GetModel();
	if ( !mdl )
		return;

	m_hStudioHdr = modelinfo->GetCacheHandle( mdl );
	if ( m_hStudioHdr == MDLHANDLE_INVALID )
		return;

	const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr( m_hStudioHdr );
	if ( !pStudioHdr )
	{
		m_hStudioHdr = MDLHANDLE_INVALID;
		return;
	}

	CStudioHdr *pNewWrapper = new CStudioHdr;
	pNewWrapper->Init( pStudioHdr, mdlcache );
	Assert( pNewWrapper->IsValid() );
	
	if ( pNewWrapper->GetVirtualModel() )
	{
		MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(pStudioHdr->virtualModel)&0xffff;
		mdlcache->LockStudioHdr( hVirtualModel );
	}

	m_pStudioHdr = pNewWrapper; // must be last to ensure virtual model correctly set up
}

void C_BaseAnimating::UnlockStudioHdr()
{
	if ( m_hStudioHdr != MDLHANDLE_INVALID )
	{
		studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( m_hStudioHdr );
		Assert( m_pStudioHdr && m_pStudioHdr->GetRenderHdr() == pStudioHdr );

#if 0
		// XXX need to figure out where to flush the queue on map change to not crash
		if ( ICallQueue *pCallQueue = materials->GetRenderContext()->GetCallQueue() )
		{
			// Parallel rendering: don't unlock model data until end of rendering
			if ( pStudioHdr->GetVirtualModel() )
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)pStudioHdr->virtualModel&0xffff;
				pCallQueue->QueueCall( mdlcache, &IMDLCache::UnlockStudioHdr, hVirtualModel );
			}
			pCallQueue->QueueCall( mdlcache, &IMDLCache::UnlockStudioHdr, m_hStudioHdr );
		}
		else
#endif
		{
			// Immediate-mode rendering, can unlock immediately
			if ( pStudioHdr->GetVirtualModel() )
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)pStudioHdr->virtualModel&0xffff;
				mdlcache->UnlockStudioHdr( hVirtualModel );
			}
			mdlcache->UnlockStudioHdr( m_hStudioHdr );
		}
		m_hStudioHdr = MDLHANDLE_INVALID;

		delete m_pStudioHdr;
		m_pStudioHdr = NULL;
	}
}

void C_BaseAnimating::OnModelLoadComplete( const model_t* pModel )
{
	Assert( m_bDynamicModelPending && pModel == GetModel() );
	if ( m_bDynamicModelPending && pModel == GetModel() )
	{
		m_bDynamicModelPending = false;
		OnNewModel();
		UpdateVisibility();
	}
}

void C_BaseAnimating::ValidateModelIndex()
{
	BaseClass::ValidateModelIndex();
	Assert( m_nModelIndex == 0 || m_AutoRefModelIndex.Get() );
}

CStudioHdr *C_BaseAnimating::OnNewModel()
{
	InvalidateMdlCache();

	// remove transition animations playback
	m_SequenceTransitioner.RemoveAll();

	if (m_pJiggleBones)
	{
		delete m_pJiggleBones;
		m_pJiggleBones = NULL;
	}

	if ( m_bDynamicModelPending )
	{
		modelinfo->UnregisterModelLoadCallback( -1, this );
		m_bDynamicModelPending = false;
	}

	m_AutoRefModelIndex.Clear();

	if ( !GetModel() || modelinfo->GetModelType( GetModel() ) != mod_studio )
		return NULL;

	// Reference (and thus start loading) dynamic model
	int nNewIndex = m_nModelIndex;
	if ( modelinfo->GetModel( nNewIndex ) != GetModel() )
	{
		// XXX what's authoritative? the model pointer or the model index? what a mess.
		nNewIndex = modelinfo->GetModelIndex( modelinfo->GetModelName( GetModel() ) );
		Assert( nNewIndex < 0 || modelinfo->GetModel( nNewIndex ) == GetModel() );
		if ( nNewIndex < 0 )
			nNewIndex = m_nModelIndex;
	}

	m_AutoRefModelIndex = nNewIndex;
	if ( IsDynamicModelIndex( nNewIndex ) && modelinfo->IsDynamicModelLoading( nNewIndex ) )
	{
		m_bDynamicModelPending = true;
		modelinfo->RegisterModelLoadCallback( nNewIndex, this );
	}

	if ( IsDynamicModelLoading() )
	{
		// Called while dynamic model still loading -> new model, clear deferred state
		m_bResetSequenceInfoOnLoad = false;
		return NULL;
	}

	CStudioHdr *hdr = GetModelPtr();
	if (hdr == NULL)
		return NULL;

	InvalidateBoneCache();
	if ( m_pBoneMergeCache )
	{
		delete m_pBoneMergeCache;
		m_pBoneMergeCache = NULL;
		// recreated in BuildTransformations
	}

	Studio_DestroyBoneCache( m_hitboxBoneCacheHandle );
	m_hitboxBoneCacheHandle = 0;

	// Make sure m_CachedBones has space.
	if ( m_CachedBoneData.Count() != hdr->numbones() )
	{
		m_CachedBoneData.SetSize( hdr->numbones() );
		for ( int i=0; i < hdr->numbones(); i++ )
		{
			SetIdentityMatrix( m_CachedBoneData[i] );
		}
	}
	m_BoneAccessor.Init( this, m_CachedBoneData.Base() ); // Always call this in case the studiohdr_t has changed.

	// Free any IK data
	if (m_pIk)
	{
		delete m_pIk;
		m_pIk = NULL;
	}

	// Don't reallocate unless a different size. 
	if ( m_Attachments.Count() != hdr->GetNumAttachments() )
	{
		m_Attachments.SetSize( hdr->GetNumAttachments() );

		// This is to make sure we don't use the attachment before its been set up
		for ( int i=0; i < m_Attachments.Count(); i++ )
		{
			m_Attachments[i].m_bAnglesComputed = false;
			m_Attachments[i].m_nLastFramecount = 0;
#ifdef _DEBUG
			m_Attachments[i].m_AttachmentToWorld.Invalidate();
			m_Attachments[i].m_angRotation.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
			m_Attachments[i].m_vOriginVelocity.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
#endif
		}

	}

	Assert( hdr->GetNumPoseParameters() <= ARRAYSIZE( m_flPoseParameter ) );

	m_iv_flPoseParameter.SetMaxCount( hdr->GetNumPoseParameters() );
	
	int i;
	for ( i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		const mstudioposeparamdesc_t &Pose = hdr->pPoseParameter( i );
		m_iv_flPoseParameter.SetLooping( Pose.loop != 0.0f, i );
		// Note:  We can't do this since if we get a DATA_UPDATE_CREATED (i.e., new entity) with both a new model and some valid pose parameters this will slam the 
		//  pose parameters to zero and if the model goes dormant the pose parameter field will never be set to the true value.  We shouldn't have to zero these out
		//  as they are under the control of the server and should be properly set
		if ( !IsServerEntity() )
		{
			SetPoseParameter( hdr, i, 0.0 );
		}
	}

	int boneControllerCount = MIN( hdr->numbonecontrollers(), ARRAYSIZE( m_flEncodedController ) );

	m_iv_flEncodedController.SetMaxCount( boneControllerCount );

	for ( i = 0; i < boneControllerCount ; i++ )
	{
		bool loop = (hdr->pBonecontroller( i )->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) != 0;
		m_iv_flEncodedController.SetLooping( loop, i );
		SetBoneController( i, 0.0 );
	}

	InitModelEffects();

	// lookup generic eye attachment, if exists
	m_iEyeAttachment = LookupAttachment( "eyes" );

	// If we didn't have a model before, then we might need to go in the interpolation list now.
	if ( ShouldInterpolate() )
		AddToInterpolationList();

	// objects with attachment points need to be queryable even if they're not solid
	if ( hdr->GetNumAttachments() != 0 )
	{
		AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
	}


	// Most entities clear out their sequences when they change models on the server, but 
	// not all entities network down their m_nSequence (like multiplayer game player entities), 
	// so we may need to clear it out here. Force a SetSequence call no matter what, though.
	int forceSequence = ShouldResetSequenceOnNewModel() ? 0 : m_nSequence;

	if ( GetSequence() >= hdr->GetNumSeq() )
	{
		forceSequence = 0;
	}

	m_nSequence = -1;
	SetSequence( forceSequence );

	if ( m_bResetSequenceInfoOnLoad )
	{
		m_bResetSequenceInfoOnLoad = false;
		ResetSequenceInfo();
	}

	UpdateRelevantInterpolatedVars();
		
	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Returns index number of a given named bone
// Input  : name of a bone
// Output :	Bone index number or -1 if bone not found
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupBone( const char *szName )
{
	Assert( GetModelPtr() );

	return Studio_BoneIndexByName( GetModelPtr(), szName );
}

//=========================================================
//=========================================================
void C_BaseAnimating::GetBonePosition ( int iBone, Vector &origin, QAngle &angles )
{
	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	
	MatrixAngles( bonetoworld, angles, origin );
}

void C_BaseAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld )
{
	Assert( GetModelPtr() && iBone >= 0 && iBone < GetModelPtr()->numbones() );
	CBoneCache *pcache = GetBoneCache( NULL );

	matrix3x4_t *pmatrix = pcache->GetCachedBone( iBone );

	if ( !pmatrix )
	{
		MatrixCopy( EntityToWorldTransform(), pBoneToWorld );
		return;
	}

	Assert( pmatrix );
	
	// FIXME
	MatrixCopy( *pmatrix, pBoneToWorld );
}
//=============================================================================
// HPE_BEGIN:
// [menglish] Finds the bone associated with the given hitbox
//=============================================================================

int C_BaseAnimating::GetHitboxBone( int hitboxIndex )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( pStudioHdr )
	{
		mstudiohitboxset_t *set =pStudioHdr->pHitboxSet( m_nHitboxSet );
		if ( set && hitboxIndex < set->numhitboxes )
		{
			return set->pHitbox( hitboxIndex )->bone;
		}
	}
	return 0;
}

//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Setup to initialize our model effects once the model's loaded
//-----------------------------------------------------------------------------
void C_BaseAnimating::InitModelEffects( void )
{
	m_bInitModelEffects = true;
	TermRopes();
}

//-----------------------------------------------------------------------------
// Purpose: Load the model's keyvalues section and create effects listed inside it
//-----------------------------------------------------------------------------
void C_BaseAnimating::DelayedInitModelEffects( void )
{
	m_bInitModelEffects = false;

	// Parse the keyvalues and see if they want to make ropes on this model.
	KeyValues * modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		// Do we have a cables section?
		KeyValues *pkvAllCables = modelKeyValues->FindKey("Cables");
		if ( pkvAllCables )
		{
			// Start grabbing the sounds and slotting them in
			for ( KeyValues *pSingleCable = pkvAllCables->GetFirstSubKey(); pSingleCable; pSingleCable = pSingleCable->GetNextKey() )
			{
				C_RopeKeyframe *pRope = C_RopeKeyframe::CreateFromKeyValues( this, pSingleCable );
				m_Ropes.AddToTail( pRope );
			}
		}

 		if ( !m_bNoModelParticles )
		{
			// Do we have a particles section?
			KeyValues *pkvAllParticleEffects = modelKeyValues->FindKey("Particles");
			if ( pkvAllParticleEffects )
			{
				// Start grabbing the sounds and slotting them in
				for ( KeyValues *pSingleEffect = pkvAllParticleEffects->GetFirstSubKey(); pSingleEffect; pSingleEffect = pSingleEffect->GetNextKey() )
				{
					const char *pszParticleEffect = pSingleEffect->GetString( "name", "" );
					const char *pszAttachment = pSingleEffect->GetString( "attachment_point", "" );
					const char *pszAttachType = pSingleEffect->GetString( "attachment_type", "" );

					// Convert attach type
					int iAttachType = GetAttachTypeFromString( pszAttachType );
					if ( iAttachType == -1 )
					{
						Warning("Invalid attach type specified for particle effect in model '%s' keyvalues section. Trying to spawn effect '%s' with attach type of '%s'\n", GetModelName(), pszParticleEffect, pszAttachType );
						return;
					}

					// Convert attachment point
					int iAttachment = atoi(pszAttachment);
					// See if we can find any attachment points matching the name
					if ( pszAttachment[0] != '0' && iAttachment == 0 )
					{
						iAttachment = LookupAttachment( pszAttachment );
						if ( iAttachment <= 0 )
						{
							Warning("Failed to find attachment point specified for particle effect in model '%s' keyvalues section. Trying to spawn effect '%s' on attachment named '%s'\n", GetModelName(), pszParticleEffect, pszAttachment );
							return;
						}
					}
					#ifdef TF_CLIENT_DLL
					// Halloween Hack for Sentry Rockets
					if ( !V_strcmp( "sentry_rocket", pszParticleEffect ) )
					{
						// Halloween Spell Effect Check
						int iHalloweenSpell = 0;
						// if the owner is a Sentry, Check its owner
						CBaseObject *pSentry = dynamic_cast<CBaseObject*>( GetOwnerEntity() );
						if ( pSentry )
						{
							CALL_ATTRIB_HOOK_INT_ON_OTHER( pSentry->GetOwner(), iHalloweenSpell, halloween_pumpkin_explosions );
						}
						else
						{
							CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwnerEntity(), iHalloweenSpell, halloween_pumpkin_explosions );
						}

						if ( iHalloweenSpell > 0 )
						{
							pszParticleEffect = "halloween_rockettrail";
						}
					}
					#endif
					// Spawn the particle effect
					ParticleProp()->Create( pszParticleEffect, (ParticleAttachment_t)iAttachType, iAttachment );
				}
			}
		}
	}

	modelKeyValues->deleteThis();
}


void C_BaseAnimating::TermRopes()
{
	FOR_EACH_LL( m_Ropes, i )
		m_Ropes[i]->Release();

	m_Ropes.Purge();
}


// FIXME: redundant?
void C_BaseAnimating::GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	// interpolate two 0..1 encoded controllers to a single 0..1 controller
	int i;
	for( i=0; i < MAXSTUDIOBONECTRLS; i++)
	{
		controllers[ i ] = m_flEncodedController[ i ];
	}
}

float C_BaseAnimating::GetPoseParameter( int iPoseParameter )
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if ( pStudioHdr == NULL )
		return 0.0f;

	if ( pStudioHdr->GetNumPoseParameters() < iPoseParameter )
		return 0.0f;

	if ( iPoseParameter < 0 )
		return 0.0f;

	return m_flPoseParameter[iPoseParameter];
}

// FIXME: redundant?
void C_BaseAnimating::GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM])
{
	if ( !pStudioHdr )
		return;

	// interpolate pose parameters
	int i;
	for( i=0; i < pStudioHdr->GetNumPoseParameters(); i++)
	{
		poseParameter[i] = m_flPoseParameter[i];
	}


#if 0 // _DEBUG
	if (/* Q_stristr( pStudioHdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
	{
		DevMsgRT( "%s\n", pStudioHdr->pszName() );
		DevMsgRT( "%6.2f : ", gpGlobals->curtime );
		for( i=0; i < pStudioHdr->GetNumPoseParameters(); i++)
		{
			const mstudioposeparamdesc_t &Pose = pStudioHdr->pPoseParameter( i );

			DevMsgRT( "%s %6.2f ", Pose.pszName(), poseParameter[i] * Pose.end + (1 - poseParameter[i]) * Pose.start );
		}
		DevMsgRT( "\n" );
	}
#endif
}


float C_BaseAnimating::ClampCycle( float flCycle, bool isLooping )
{
	if (isLooping) 
	{
		// FIXME: does this work with negative framerate?
		flCycle -= (int)flCycle;
		if (flCycle < 0.0f)
		{
			flCycle += 1.0f;
		}
	}
	else 
	{
		flCycle = clamp( flCycle, 0.0f, 0.999f );
	}
	return flCycle;
}


void C_BaseAnimating::GetCachedBoneMatrix( int boneIndex, matrix3x4_t &out )
{
	MatrixCopy( GetBone( boneIndex ), out );
}


//-----------------------------------------------------------------------------
// Purpose:	move position and rotation transforms into global matrices
//-----------------------------------------------------------------------------
void C_BaseAnimating::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q, const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	VPROF_BUDGET( "C_BaseAnimating::BuildTransformations", VPROF_BUDGETGROUP_CLIENT_ANIMATION );

	if ( !hdr )
		return;

	matrix3x4_t bonematrix;
	bool boneSimulated[MAXSTUDIOBONES];

	// no bones have been simulated
	memset( boneSimulated, 0, sizeof(boneSimulated) );
	mstudiobone_t *pbones = hdr->pBone( 0 );

	if ( m_pRagdoll )
	{
		// simulate bones and update flags
		int oldWritableBones = m_BoneAccessor.GetWritableBones();
		int oldReadableBones = m_BoneAccessor.GetReadableBones();
		m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
		m_BoneAccessor.SetReadableBones( BONE_USED_BY_ANYTHING );
		
#if defined( REPLAY_ENABLED )
		// If we're playing back a demo, override the ragdoll bones with cached version if available - otherwise, simulate.
		if ( ( !engine->IsPlayingDemo() && !engine->IsPlayingTimeDemo() ) ||
			 !CReplayRagdollCache::Instance().IsInitialized() ||
			 !CReplayRagdollCache::Instance().GetFrame( this, engine->GetDemoPlaybackTick(), boneSimulated, &m_BoneAccessor ) )
#endif
		{
			m_pRagdoll->RagdollBone( this, pbones, hdr->numbones(), boneSimulated, m_BoneAccessor );
		}
		
		m_BoneAccessor.SetWritableBones( oldWritableBones );
		m_BoneAccessor.SetReadableBones( oldReadableBones );
	}

	// For EF_BONEMERGE entities, copy the bone matrices for any bones that have matching names.
	bool boneMerge = IsEffectActive(EF_BONEMERGE);
	if ( boneMerge || m_pBoneMergeCache )
	{
		if ( boneMerge )
		{
			if ( !m_pBoneMergeCache )
			{
				m_pBoneMergeCache = new CBoneMergeCache;
				m_pBoneMergeCache->Init( this );
			}
			m_pBoneMergeCache->MergeMatchingBones( boneMask );
		}
		else
		{
			delete m_pBoneMergeCache;
			m_pBoneMergeCache = NULL;
		}
	}

	for (int i = 0; i < hdr->numbones(); i++) 
	{
		// Only update bones reference by the bone mask.
		if ( !( hdr->boneFlags( i ) & boneMask ) )
		{
			continue;
		}

		if ( m_pBoneMergeCache && m_pBoneMergeCache->IsBoneMerged( i ) )
			continue;

		// animate all non-simulated bones
		if ( boneSimulated[i] || CalcProceduralBone( hdr, i, m_BoneAccessor ))
		{
			continue;
		}
		// skip bones that the IK has already setup
		else if (boneComputed.IsBoneMarked( i ))
		{
			// dummy operation, just used to verify in debug that this should have happened
			GetBoneForWrite( i );
		}
		else
		{
			QuaternionMatrix( q[i], pos[i], bonematrix );

			Assert( fabs( pos[i].x ) < 100000 );
			Assert( fabs( pos[i].y ) < 100000 );
			Assert( fabs( pos[i].z ) < 100000 );

			if ( (hdr->boneFlags( i ) & BONE_ALWAYS_PROCEDURAL) && 
				 (hdr->pBone( i )->proctype & STUDIO_PROC_JIGGLE) )
			{
				//
				// Physics-based "jiggle" bone
				// Bone is assumed to be along the Z axis
				// Pitch around X, yaw around Y
				//

				// compute desired bone orientation
				matrix3x4_t goalMX;

				if (pbones[i].parent == -1) 
				{
					ConcatTransforms( cameraTransform, bonematrix, goalMX );
				}
				else 
				{
					// If the parent bone has been scaled (like with BuildBigHeadTransformations)
					// scale it back down so the jiggly bones show up non-scaled in the correct location.
					matrix3x4_t parentMX = GetBone( pbones[i].parent );

					float fScale = Square( parentMX[0][0] ) + Square( parentMX[1][0] ) + Square( parentMX[2][0] );
					if ( fScale > Square( 1.0001f ) )
					{
						fScale = 1.0f / FastSqrt( fScale );
						MatrixScaleBy( fScale, parentMX );
					}

					ConcatTransforms( parentMX, bonematrix, goalMX );
				}

				// get jiggle properties from QC data
				mstudiojigglebone_t *jiggleInfo = (mstudiojigglebone_t *)pbones[i].pProcedure( );

				if (!m_pJiggleBones)
				{
					m_pJiggleBones = new CJiggleBones;
				}

				// do jiggle physics
				m_pJiggleBones->BuildJiggleTransformations( i, gpGlobals->realtime, jiggleInfo, goalMX, GetBoneForWrite( i ) );

			}
			else if (hdr->boneParent(i) == -1) 
			{
				ConcatTransforms( cameraTransform, bonematrix, GetBoneForWrite( i ) );
			} 
			else 
			{
				ConcatTransforms( GetBone( hdr->boneParent(i) ), bonematrix, GetBoneForWrite( i ) );
			}
		}

		if (hdr->boneParent(i) == -1) 
		{
			// Apply client-side effects to the transformation matrix
			ApplyBoneMatrixTransform( GetBoneForWrite( i ) );
		}
	}
	
	
}

//-----------------------------------------------------------------------------
// Purpose: Special effects
// Input  : transform - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	switch( m_nRenderFX )
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
		if ( RandomInt(0,49) == 0 )
		{
			int axis = RandomInt(0,1);
			if ( axis == 1 ) // Choose between x & z
				axis = 2;
			VectorScale( transform[axis], RandomFloat(1,1.484), transform[axis] );
		}
		else if ( RandomInt(0,49) == 0 )
		{
			float offset;
			int axis = RandomInt(0,1);
			if ( axis == 1 ) // Choose between x & z
				axis = 2;
			offset = RandomFloat(-10,10);
			transform[RandomInt(0,2)][3] += offset;
		}
		break;
	case kRenderFxExplode:
		{
			float scale;
			
			scale = 1.0 + (gpGlobals->curtime - m_flAnimTime) * 10.0;
			if ( scale > 2 )	// Don't blow up more than 200%
				scale = 2;
			transform[0][1] *= scale;
			transform[1][1] *= scale;
			transform[2][1] *= scale;
		}
		break;
	default:
		break;
		
	}

	if ( IsModelScaled() )
	{
		// The bone transform is in worldspace, so to scale this, we need to translate it back
		float scale = GetModelScale();

		Vector pos;
		MatrixGetColumn( transform, 3, pos );
		pos -= GetRenderOrigin();
		pos *= scale;
		pos += GetRenderOrigin();
		MatrixSetColumn( pos, 3, transform );

		VectorScale( transform[0], scale, transform[0] );
		VectorScale( transform[1], scale, transform[1] );
		VectorScale( transform[2], scale, transform[2] );
	}
}

void C_BaseAnimating::CreateUnragdollInfo( C_BaseAnimating *pRagdoll )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// It's already an active ragdoll, sigh
	if ( m_pRagdollInfo && m_pRagdollInfo->m_bActive )
	{
		Assert( 0 );
		return;
	}

	// Now do the current bone setup
	pRagdoll->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

	matrix3x4_t parentTransform;
	QAngle newAngles( 0, pRagdoll->GetAbsAngles()[YAW], 0 );

	AngleMatrix( GetAbsAngles(), GetAbsOrigin(), parentTransform );
	// pRagdoll->SaveRagdollInfo( hdr->numbones, parentTransform, m_BoneAccessor );
	
	if ( !m_pRagdollInfo )
	{
		m_pRagdollInfo = new RagdollInfo_t;
		Assert( m_pRagdollInfo );
		if ( !m_pRagdollInfo )
		{
			Msg( "Memory allocation of RagdollInfo_t failed!\n" );
			return;
		}
	}

	Q_memset( m_pRagdollInfo, 0, sizeof( *m_pRagdollInfo ) );

	int numbones = hdr->numbones();

	m_pRagdollInfo->m_bActive = true;
	m_pRagdollInfo->m_flSaveTime = gpGlobals->curtime;
	m_pRagdollInfo->m_nNumBones = numbones;

	for ( int i = 0;  i < numbones; i++ )
	{
		matrix3x4_t inverted;
		matrix3x4_t output;

		if ( hdr->boneParent(i) == -1 )
		{
			// Decompose into parent space
			MatrixInvert( parentTransform, inverted );
		}
		else
		{
			MatrixInvert( pRagdoll->m_BoneAccessor.GetBone( hdr->boneParent(i) ), inverted );
		}

		ConcatTransforms( inverted, pRagdoll->m_BoneAccessor.GetBone( i ), output );

		MatrixAngles( output, 
			m_pRagdollInfo->m_rgBoneQuaternion[ i ],
			m_pRagdollInfo->m_rgBonePos[ i ] );
	}
}

void C_BaseAnimating::SaveRagdollInfo( int numbones, const matrix3x4_t &cameraTransform, CBoneAccessor &pBoneToWorld )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	if ( !m_pRagdollInfo )
	{
		m_pRagdollInfo = new RagdollInfo_t;
		Assert( m_pRagdollInfo );
		if ( !m_pRagdollInfo )
		{
			Msg( "Memory allocation of RagdollInfo_t failed!\n" );
			return;
		}
		memset( m_pRagdollInfo, 0, sizeof( *m_pRagdollInfo ) );
	}

	mstudiobone_t *pbones = hdr->pBone( 0 );

	m_pRagdollInfo->m_bActive = true;
	m_pRagdollInfo->m_flSaveTime = gpGlobals->curtime;
	m_pRagdollInfo->m_nNumBones = numbones;

	for ( int i = 0;  i < numbones; i++ )
	{
		matrix3x4_t inverted;
		matrix3x4_t output;

		if ( pbones[i].parent == -1 )
		{
			// Decompose into parent space
			MatrixInvert( cameraTransform, inverted );
		}
		else
		{
			MatrixInvert( pBoneToWorld.GetBone( pbones[ i ].parent ), inverted );
		}

		ConcatTransforms( inverted, pBoneToWorld.GetBone( i ), output );

		MatrixAngles( output, 
			m_pRagdollInfo->m_rgBoneQuaternion[ i ],
			m_pRagdollInfo->m_rgBonePos[ i ] );
	}
}

bool C_BaseAnimating::RetrieveRagdollInfo( Vector *pos, Quaternion *q )
{
	if ( !m_bStoreRagdollInfo || !m_pRagdollInfo || !m_pRagdollInfo->m_bActive )
		return false;

	for ( int i = 0; i < m_pRagdollInfo->m_nNumBones; i++ )
	{
		pos[ i ] = m_pRagdollInfo->m_rgBonePos[ i ];
		q[ i ] = m_pRagdollInfo->m_rgBoneQuaternion[ i ];
	}

	return true;
}

//-----------------------------------------------------------------------------
// Should we collide?
//-----------------------------------------------------------------------------

CollideType_t C_BaseAnimating::GetCollideType( void )
{
	if ( IsRagdoll() )
		return ENTITY_SHOULD_RESPOND;

	return BaseClass::GetCollideType();
}

//-----------------------------------------------------------------------------
// Purpose: if the active sequence changes, keep track of the previous ones and decay them based on their decay rate
//-----------------------------------------------------------------------------
void C_BaseAnimating::MaintainSequenceTransitions( IBoneSetup &boneSetup, float flCycle, Vector pos[], Quaternion q[] )
{
	VPROF( "C_BaseAnimating::MaintainSequenceTransitions" );

	if ( !boneSetup.GetStudioHdr() )
		return;

	if ( prediction->InPrediction() )
	{
		m_nPrevNewSequenceParity = m_nNewSequenceParity;
		return;
	}

	m_SequenceTransitioner.CheckForSequenceChange( 
		boneSetup.GetStudioHdr(),
		GetSequence(),
		m_nNewSequenceParity != m_nPrevNewSequenceParity,
		!IsNoInterpolationFrame()
		);

	m_nPrevNewSequenceParity = m_nNewSequenceParity;

	// Update the transition sequence list.
	m_SequenceTransitioner.UpdateCurrent( 
		boneSetup.GetStudioHdr(),
		GetSequence(),
		flCycle,
		m_flPlaybackRate,
		gpGlobals->curtime
		);


	// process previous sequences
	for (int i = m_SequenceTransitioner.m_animationQueue.Count() - 2; i >= 0; i--)
	{
		C_AnimationLayer *blend = &m_SequenceTransitioner.m_animationQueue[i];

		float dt = (gpGlobals->curtime - blend->m_flLayerAnimtime);
		flCycle = blend->m_flCycle + dt * blend->m_flPlaybackRate * GetSequenceCycleRate( boneSetup.GetStudioHdr(), blend->m_nSequence );
		flCycle = ClampCycle( flCycle, IsSequenceLooping( boneSetup.GetStudioHdr(), blend->m_nSequence ) );

#if 1 // _DEBUG
		if (/*Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
		{
			DevMsgRT( "%8.4f : %30s : %5.3f : %4.2f  +\n", gpGlobals->curtime, boneSetup.GetStudioHdr()->pSeqdesc( blend->m_nSequence ).pszLabel(), flCycle, (float)blend->m_flWeight );
		}
#endif

		boneSetup.AccumulatePose( pos, q, blend->m_nSequence, flCycle, blend->m_flWeight, gpGlobals->curtime, m_pIk );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hdr - 
//			pos[] - 
//			q[] - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::UnragdollBlend( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime )
{
	if ( !hdr )
	{
		return;
	}

	if ( !m_pRagdollInfo || !m_pRagdollInfo->m_bActive )
		return;

	float dt = currentTime - m_pRagdollInfo->m_flSaveTime;
	if ( dt > 0.2f )
	{
		m_pRagdollInfo->m_bActive = false;
		return;
	}

	// Slerp bone sets together
	float frac = dt / 0.2f;
	frac = clamp( frac, 0.0f, 1.0f );

	int i;
	for ( i = 0; i < hdr->numbones(); i++ )
	{
		VectorLerp( m_pRagdollInfo->m_rgBonePos[ i ], pos[ i ], frac, pos[ i ] );
		QuaternionSlerp( m_pRagdollInfo->m_rgBoneQuaternion[ i ], q[ i ], frac, q[ i ] );
	}
}

void C_BaseAnimating::AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	// Nothing here
}

void C_BaseAnimating::ChildLayerBlend( Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	return;

	Vector		childPos[MAXSTUDIOBONES];
	Quaternion	childQ[MAXSTUDIOBONES];
	float		childPoseparam[MAXSTUDIOPOSEPARAM];

	// go through all children
	for ( C_BaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		C_BaseAnimating *pChildAnimating = pChild->GetBaseAnimating();

		if ( pChildAnimating )
		{
			CStudioHdr *pChildHdr = pChildAnimating->GetModelPtr();

			// FIXME: needs a new type of EF_BONEMERGE (EF_CHILDMERGE?)
			if ( pChildHdr && pChild->IsEffectActive( EF_BONEMERGE ) && pChildHdr->SequencesAvailable() && pChildAnimating->m_pBoneMergeCache )
			{
				// FIXME: these should Inherit from the parent
				GetPoseParameters( pChildHdr, childPoseparam );

				IBoneSetup childBoneSetup( pChildHdr, boneMask, childPoseparam );
				childBoneSetup.InitPose( childPos, childQ );

				// set up the child into the parent's current pose
				pChildAnimating->m_pBoneMergeCache->CopyParentToChild( pos, q, childPos, childQ, boneMask );

				// FIXME: needs some kind of sequence
				// merge over whatever bones the childs sequence modifies
				childBoneSetup.AccumulatePose( childPos, childQ, 0, GetCycle(), 1.0, currentTime, NULL );

				// copy the result back into the parents bones
				pChildAnimating->m_pBoneMergeCache->CopyChildToParent( childPos, childQ, pos, q, boneMask );

				// probably needs an IK merge system of some sort =(
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Do the default sequence blending rules as done in HL1
//-----------------------------------------------------------------------------
void C_BaseAnimating::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	VPROF( "C_BaseAnimating::StandardBlendingRules" );

	float		poseparam[MAXSTUDIOPOSEPARAM];

	if ( !hdr )
		return;

	if ( !hdr->SequencesAvailable() )
	{
		return;
	}

	if (GetSequence() >= hdr->GetNumSeq() || GetSequence() == -1 ) 
	{
		SetSequence( 0 );
	}

	GetPoseParameters( hdr, poseparam );

	// build root animation
	float fCycle = GetCycle();

#if 1 //_DEBUG
	if (/* Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
	{
		DevMsgRT( "%8.4f : %30s : %5.3f : %4.2f\n", currentTime, hdr->pSeqdesc( GetSequence() ).pszLabel(), fCycle, 1.0 );
	}
#endif

	IBoneSetup boneSetup( hdr, boneMask, poseparam );
	boneSetup.InitPose( pos, q );
	boneSetup.AccumulatePose( pos, q, GetSequence(), fCycle, 1.0, currentTime, m_pIk );

	// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%30s %6.2f : %6.2f", hdr->pSeqdesc( GetSequence() )->pszLabel( ), fCycle, 1.0 );

	MaintainSequenceTransitions( boneSetup, fCycle, pos, q );

	AccumulateLayers( boneSetup, pos, q, currentTime );

	CIKContext auto_ik;
	auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), currentTime, gpGlobals->framecount, boneMask );
	boneSetup.CalcAutoplaySequences( pos, q, currentTime, &auto_ik );

	if ( hdr->numbonecontrollers() )
	{
		float controllers[MAXSTUDIOBONECTRLS];
		GetBoneControllers(controllers);
		boneSetup.CalcBoneAdj( pos, q, controllers );
	}

	ChildLayerBlend( pos, q, currentTime, boneMask );

	UnragdollBlend( hdr, pos, q, currentTime );

#ifdef STUDIO_ENABLE_PERF_COUNTERS
#if _DEBUG
	if (Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL)
	{
		DevMsgRT( "layers %4d : bones %4d : animated %4d\n", hdr->m_nPerfAnimationLayers, hdr->m_nPerfUsedBones, hdr->m_nPerfAnimatedBones );
	}
#endif
#endif

}


//-----------------------------------------------------------------------------
// Purpose: Put a value into an attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseAnimating::PutAttachment( int number, const matrix3x4_t &attachmentToWorld )
{
	if ( number < 1 || number > m_Attachments.Count() )
		return false;

	CAttachmentData *pAtt = &m_Attachments[number-1];
	if ( gpGlobals->frametime > 0 && pAtt->m_nLastFramecount > 0 && pAtt->m_nLastFramecount == gpGlobals->framecount - 1 )
	{
		Vector vecPreviousOrigin, vecOrigin;
		MatrixPosition( pAtt->m_AttachmentToWorld, vecPreviousOrigin );
		MatrixPosition( attachmentToWorld, vecOrigin );
		pAtt->m_vOriginVelocity = (vecOrigin - vecPreviousOrigin) / gpGlobals->frametime;
	}
	else
	{
		pAtt->m_vOriginVelocity.Init();
	}
	pAtt->m_nLastFramecount = gpGlobals->framecount;
	pAtt->m_bAnglesComputed = false;
	pAtt->m_AttachmentToWorld = attachmentToWorld;

#ifdef _DEBUG
	pAtt->m_angRotation.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
#endif

	return true;
}


bool C_BaseAnimating::SetupBones_AttachmentHelper( CStudioHdr *hdr )
{
	if ( !hdr )
		return false;

	// calculate attachment points
	matrix3x4_t world;
	for (int i = 0; i < hdr->GetNumAttachments(); i++)
	{
		const mstudioattachment_t &pattachment = hdr->pAttachment( i );
		int iBone = hdr->GetAttachmentBone( i );
		if ( (pattachment.flags & ATTACHMENT_FLAG_WORLD_ALIGN) == 0 )
		{
			ConcatTransforms( GetBone( iBone ), pattachment.local, world ); 
		}
		else
		{
			Vector vecLocalBonePos, vecWorldBonePos;
			MatrixGetColumn( pattachment.local, 3, vecLocalBonePos );
			VectorTransform( vecLocalBonePos, GetBone( iBone ), vecWorldBonePos );

			SetIdentityMatrix( world );
			MatrixSetColumn( vecWorldBonePos, 3, world );
		}

		// FIXME: this shouldn't be here, it should client side on-demand only and hooked into the bone cache!!
		FormatViewModelAttachment( i, world );
		PutAttachment( i + 1, world );
	}

	return true;
}

bool C_BaseAnimating::CalcAttachments()
{
	VPROF( "C_BaseAnimating::CalcAttachments" );


	// Make sure m_CachedBones is valid.
	return SetupBones( NULL, -1, BONE_USED_BY_ATTACHMENT, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment name
// Output :	location and angles
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles )
{																
	return GetAttachment( LookupAttachment( szName ), absOrigin, absAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Get attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	// Note: this could be more efficient, but we want the matrix3x4_t version of GetAttachment to be the origin of
	// attachment generation, so a derived class that wants to fudge attachments only 
	// has to reimplement that version. This also makes it work like the server in that regard.
	if ( number < 1 || number > m_Attachments.Count() || !CalcAttachments() )
	{
		// Set this to the model origin/angles so that we don't have stack fungus in origin and angles.
		origin = GetAbsOrigin();
		angles = GetAbsAngles();
		return false;
	}

	CAttachmentData *pData = &m_Attachments[number-1];
	if ( !pData->m_bAnglesComputed )
	{
		MatrixAngles( pData->m_AttachmentToWorld, pData->m_angRotation );
		pData->m_bAnglesComputed = true;
	}
	angles = pData->m_angRotation;
	MatrixPosition( pData->m_AttachmentToWorld, origin );
	return true;
}

bool C_BaseAnimating::GetAttachment( int number, matrix3x4_t& matrix )
{
	if ( number < 1 || number > m_Attachments.Count() )
		return false;

	if ( !CalcAttachments() )
		return false;

	matrix = m_Attachments[number-1].m_AttachmentToWorld;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Get attachment point by index (position only)
// Input  : number - which point
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachment( int number, Vector &origin )
{
	// Note: this could be more efficient, but we want the matrix3x4_t version of GetAttachment to be the origin of
	// attachment generation, so a derived class that wants to fudge attachments only 
	// has to reimplement that version. This also makes it work like the server in that regard.
	matrix3x4_t attachmentToWorld;
	if ( !GetAttachment( number, attachmentToWorld ) )
	{
		// Set this to the model origin/angles so that we don't have stack fungus in origin and angles.
		origin = GetAbsOrigin();
		return false;
	}

	MatrixPosition( attachmentToWorld, origin );
	return true;
}


bool C_BaseAnimating::GetAttachment( const char *szName, Vector &absOrigin )
{
	return GetAttachment( LookupAttachment( szName ), absOrigin );
}



bool C_BaseAnimating::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if ( number < 1 || number > m_Attachments.Count() )
	{
		return false;
	}

	if ( !CalcAttachments() )
		return false;

	originVel = m_Attachments[number-1].m_vOriginVelocity;
	angleVel.Init();

	return true;
}


//-----------------------------------------------------------------------------
// Returns the attachment in local space
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal )
{
	matrix3x4_t attachmentToWorld;
	if (!GetAttachment(iAttachment, attachmentToWorld))
		return false;

	matrix3x4_t worldToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );
	ConcatTransforms( worldToEntity, attachmentToWorld, attachmentToLocal ); 
	return true;
}

bool C_BaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	if ( GetAttachmentLocal( iAttachment, attachmentToEntity ) )
	{
		origin.Init( attachmentToEntity[0][3], attachmentToEntity[1][3], attachmentToEntity[2][3] );
		MatrixAngles( attachmentToEntity, angles );
		return true;
	}
	return false;
}

bool C_BaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin )
{
	matrix3x4_t attachmentToEntity;

	if ( GetAttachmentLocal( iAttachment, attachmentToEntity ) )
	{
		MatrixPosition( attachmentToEntity, origin );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetRootBone( matrix3x4_t &rootBone )
{
	Assert( !IsDynamicModelLoading() );

	if ( IsEffectActive( EF_BONEMERGE ) && GetMoveParent() && m_pBoneMergeCache )
		return m_pBoneMergeCache->GetRootBone( rootBone );

	GetBoneTransform( 0, rootBone );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Move sound location to center of body
//-----------------------------------------------------------------------------
bool C_BaseAnimating::GetSoundSpatialization( SpatializationInfo_t& info )
{
	{
		C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false );
		if ( !BaseClass::GetSoundSpatialization( info ) )
			return false;
	}

	// move sound origin to center if npc has IK
	if ( info.pOrigin && IsNPC() && m_pIk)
	{
		*info.pOrigin = GetAbsOrigin();

		Vector mins, maxs, center;

		modelinfo->GetModelBounds( GetModel(), mins, maxs );
		VectorAdd( mins, maxs, center );
		VectorScale( center, 0.5f, center );

		(*info.pOrigin) += center;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsViewModel() const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::UpdateOnRemove( void )
{
	RemoveFromClientSideAnimationList( true );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsMenuModel() const
{
	return false;
}

// UNDONE: Seems kind of silly to have this when we also have the cached bones in C_BaseAnimating
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBoneCache *C_BaseAnimating::GetBoneCache( CStudioHdr *pStudioHdr )
{
	int boneMask = BONE_USED_BY_HITBOX;
	CBoneCache *pcache = Studio_GetBoneCache( m_hitboxBoneCacheHandle );
	if ( pcache )
	{
		if ( pcache->IsValid( gpGlobals->curtime, 0.0 ) )
		{
			// in memory and still valid, use it!
			return pcache;
		}
		// in memory, but not the same bone set, destroy & rebuild
		if ( (pcache->m_boneMask & boneMask) != boneMask )
		{
			Studio_DestroyBoneCache( m_hitboxBoneCacheHandle );
			m_hitboxBoneCacheHandle = 0;
			pcache = NULL;
		}
	}

	if ( !pStudioHdr ) 
		pStudioHdr = GetModelPtr( );
	Assert(pStudioHdr);

	C_BaseAnimating::PushAllowBoneAccess( true, false, "GetBoneCache" );
	SetupBones( NULL, -1, boneMask, gpGlobals->curtime );
	C_BaseAnimating::PopBoneAccess( "GetBoneCache" );

	if ( pcache )
	{
		// still in memory but out of date, refresh the bones.
		pcache->UpdateBones( m_CachedBoneData.Base(), pStudioHdr->numbones(), gpGlobals->curtime );
	}
	else
	{
		bonecacheparams_t params;
		params.pStudioHdr = pStudioHdr;
		// HACKHACK: We need the pointer to all bones here
		params.pBoneToWorld = m_CachedBoneData.Base();
		params.curtime = gpGlobals->curtime;
		params.boneMask = boneMask;

		m_hitboxBoneCacheHandle = Studio_CreateBoneCache( params );
		pcache = Studio_GetBoneCache( m_hitboxBoneCacheHandle );
	}
	Assert(pcache);
	return pcache;
}


class CTraceFilterSkipNPCsAndPlayers : public CTraceFilterSimple
{
public:
	CTraceFilterSkipNPCsAndPlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity(pServerEntity, contentsMask) )
		{
			C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
			if ( !pEntity )
				return true;

			if ( pEntity->IsNPC() || pEntity->IsPlayer() )
				return false;

			return true;
		}
		return false;
	}
};


/*
void drawLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration) 
{
	debugoverlay->AddLineOverlay( origin, dest, r, g, b, noDepthTest, duration );
}
*/

//-----------------------------------------------------------------------------
// Purpose: update latched IK contacts if they're in a moving reference frame.
//-----------------------------------------------------------------------------

void C_BaseAnimating::UpdateIKLocks( float currentTime )
{
	if (!m_pIk) 
		return;

	int targetCount = m_pIk->m_target.Count();
	if ( targetCount == 0 )
		return;

	for (int i = 0; i < targetCount; i++)
	{
		CIKTarget *pTarget = &m_pIk->m_target[i];

		if (!pTarget->IsActive())
			continue;

		if (pTarget->GetOwner() != -1)
		{
			C_BaseEntity *pOwner = cl_entitylist->GetEnt( pTarget->GetOwner() );
			if (pOwner != NULL)
			{
				pTarget->UpdateOwner( pOwner->entindex(), pOwner->GetAbsOrigin(), pOwner->GetAbsAngles() );
			}				
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the ground or external attachment points needed by IK rules
//-----------------------------------------------------------------------------

void C_BaseAnimating::CalculateIKLocks( float currentTime )
{
	if (!m_pIk) 
		return;

	int targetCount = m_pIk->m_target.Count();
	if ( targetCount == 0 )
		return;

	// In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
	// get in here during the view setup code, and it's not normally supposed to be able to access the spatial
	// partition that early in the rendering loop. So we allow access right here for that special case.
	SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
	CBaseEntity::PushEnableAbsRecomputations( false );

	Ray_t ray;
	CTraceFilterSkipNPCsAndPlayers traceFilter( this, GetCollisionGroup() );

	// FIXME: trace based on gravity or trace based on angles?
	Vector up;
	AngleVectors( GetRenderAngles(), NULL, NULL, &up );

	// FIXME: check number of slots?
	float minHeight = FLT_MAX;
	float maxHeight = -FLT_MAX;

	for (int i = 0; i < targetCount; i++)
	{
		trace_t trace;
		CIKTarget *pTarget = &m_pIk->m_target[i];

		if (!pTarget->IsActive())
			continue;

		switch( pTarget->type)
		{
		case IK_GROUND:
			{
				Vector estGround;
				Vector p1, p2;

				// adjust ground to original ground position
				estGround = (pTarget->est.pos - GetRenderOrigin());
				estGround = estGround - (estGround * up) * up;
				estGround = GetAbsOrigin() + estGround + pTarget->est.floor * up;

				VectorMA( estGround, pTarget->est.height, up, p1 );
				VectorMA( estGround, -pTarget->est.height, up, p2 );

				float r = MAX( pTarget->est.radius, 1);

				// don't IK to other characters
				ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,r*2) );
				enginetrace->TraceRay( ray, PhysicsSolidMaskForEntity(), &traceFilter, &trace );

				if ( trace.m_pEnt != NULL && trace.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
				{
					pTarget->SetOwner( trace.m_pEnt->entindex(), trace.m_pEnt->GetAbsOrigin(), trace.m_pEnt->GetAbsAngles() );
				}
				else
				{
					pTarget->ClearOwner( );
				}

				if (trace.startsolid)
				{
					// trace from back towards hip
					Vector tmp = estGround - pTarget->trace.closest;
					tmp.NormalizeInPlace();
					ray.Init( estGround - tmp * pTarget->est.height, estGround, Vector(-r,-r,0), Vector(r,r,1) );

					// debugoverlay->AddLineOverlay( ray.m_Start, ray.m_Start + ray.m_Delta, 255, 0, 0, 0, 0 );

					enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &trace );

					if (!trace.startsolid)
					{
						p1 = trace.endpos;
						VectorMA( p1, - pTarget->est.height, up, p2 );
						ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,1) );

						enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &trace );
					}

					// debugoverlay->AddLineOverlay( ray.m_Start, ray.m_Start + ray.m_Delta, 0, 255, 0, 0, 0 );
				}


				if (!trace.startsolid)
				{
					if (trace.DidHitWorld())
					{
						// clamp normal to 33 degrees
						const float limit = 0.832;
						float dot = DotProduct(trace.plane.normal, up);
						if (dot < limit)
						{
							Assert( dot >= 0 );
							// subtract out up component
							Vector diff = trace.plane.normal - up * dot;
							// scale remainder such that it and the up vector are a unit vector
							float d = sqrt( (1 - limit * limit) / DotProduct( diff, diff ) );
							trace.plane.normal = up * limit + d * diff;
						}
						// FIXME: this is wrong with respect to contact position and actual ankle offset
						pTarget->SetPosWithNormalOffset( trace.endpos, trace.plane.normal );
						pTarget->SetNormal( trace.plane.normal );
						pTarget->SetOnWorld( true );

						// only do this on forward tracking or commited IK ground rules
						if (pTarget->est.release < 0.1)
						{
							// keep track of ground height
							float offset = DotProduct( pTarget->est.pos, up );
							if (minHeight > offset )
								minHeight = offset;

							if (maxHeight < offset )
								maxHeight = offset;
						}
						// FIXME: if we don't drop legs, running down hills looks horrible
						/*
						if (DotProduct( pTarget->est.pos, up ) < DotProduct( estGround, up ))
						{
							pTarget->est.pos = estGround;
						}
						*/
					}
					else if (trace.DidHitNonWorldEntity())
					{
						pTarget->SetPos( trace.endpos );
						pTarget->SetAngles( GetRenderAngles() );

						// only do this on forward tracking or commited IK ground rules
						if (pTarget->est.release < 0.1)
						{
							float offset = DotProduct( pTarget->est.pos, up );
							if (minHeight > offset )
								minHeight = offset;

							if (maxHeight < offset )
								maxHeight = offset;
						}
						// FIXME: if we don't drop legs, running down hills looks horrible
						/*
						if (DotProduct( pTarget->est.pos, up ) < DotProduct( estGround, up ))
						{
							pTarget->est.pos = estGround;
						}
						*/
					}
					else
					{
						pTarget->IKFailed( );
					}
				}
				else
				{
					if (!trace.DidHitWorld())
					{
						pTarget->IKFailed( );
					}
					else
					{
						pTarget->SetPos( trace.endpos );
						pTarget->SetAngles( GetRenderAngles() );
						pTarget->SetOnWorld( true );
					}
				}

				/*
				debugoverlay->AddTextOverlay( p1, i, 0, "%d %.1f %.1f %.1f ", i, 
					pTarget->latched.deltaPos.x, pTarget->latched.deltaPos.y, pTarget->latched.deltaPos.z );
				debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -r, -r, -1 ), Vector( r, r, 1), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );
				*/
				// debugoverlay->AddBoxOverlay( pTarget->latched.pos, Vector( -2, -2, 2 ), Vector( 2, 2, 6), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
			}
			break;

		case IK_ATTACHMENT:
			{
				C_BaseEntity *pEntity = NULL;
				float flDist = pTarget->est.radius;

				// FIXME: make entity finding sticky!
				// FIXME: what should the radius check be?
				for ( CEntitySphereQuery sphere( pTarget->est.pos, 64 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
				{
					C_BaseAnimating *pAnim = pEntity->GetBaseAnimating( );
					if (!pAnim)
						continue;

					int iAttachment = pAnim->LookupAttachment( pTarget->offset.pAttachmentName );
					if (iAttachment <= 0)
						continue;

					Vector origin;
					QAngle angles;
					pAnim->GetAttachment( iAttachment, origin, angles );

					// debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

					float d = (pTarget->est.pos - origin).Length();

					if ( d >= flDist)
						continue;

					flDist = d;
					pTarget->SetPos( origin );
					pTarget->SetAngles( angles );
					// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
				}

				if (flDist >= pTarget->est.radius)
				{
					// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
					// no solution, disable ik rule
					pTarget->IKFailed( );
				}
			}
			break;
		}
	}

#if defined( HL2_CLIENT_DLL )
	if (minHeight < FLT_MAX)
	{
		input->AddIKGroundContactInfo( entindex(), minHeight, maxHeight );
	}
#endif

	CBaseEntity::PopEnableAbsRecomputations();
	partition->SuppressLists( curSuppressed, true );
}

bool C_BaseAnimating::GetPoseParameterRange( int index, float &minValue, float &maxValue )
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if (pStudioHdr)
	{
		if (index >= 0 && index < pStudioHdr->GetNumPoseParameters())
		{
			const mstudioposeparamdesc_t &pose = pStudioHdr->pPoseParameter( index );
			minValue = pose.start;
			maxValue = pose.end;
			return true;
		}
	}
	minValue = 0.0f;
	maxValue = 1.0f;
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Do HL1 style lipsynch
//-----------------------------------------------------------------------------
void C_BaseAnimating::ControlMouth( CStudioHdr *pstudiohdr )
{
	if ( !MouthInfo().NeedsEnvelope() )
		return;

	if ( !pstudiohdr )
		  return;

	int index = LookupPoseParameter( pstudiohdr, LIPSYNC_POSEPARAM_NAME );

	if ( index != -1 )
	{
		float value = GetMouth()->mouthopen / 64.0;

		float raw = value;

		if ( value > 1.0 )  
			 value = 1.0;

		float start, end;
		GetPoseParameterRange( index, start, end );

		value = (1.0 - value) * start + value * end;

		//Adrian - Set the pose parameter value. 
		//It has to be called "mouth".
		SetPoseParameter( pstudiohdr, index, value ); 
		// Reset interpolation here since the client is controlling this rather than the server...
		m_iv_flPoseParameter.SetHistoryValuesForItem( index, raw );
	}
}

CMouthInfo *C_BaseAnimating::GetMouth( void )
{
	return &m_mouth;
}

#ifdef DEBUG_BONE_SETUP_THREADING
ConVar cl_warn_thread_contested_bone_setup("cl_warn_thread_contested_bone_setup", "0" );
#endif
ConVar cl_threaded_bone_setup("cl_threaded_bone_setup", "0", 0, "Enable parallel processing of C_BaseAnimating::SetupBones()" );

//-----------------------------------------------------------------------------
// Purpose: Do the default sequence blending rules as done in HL1
//-----------------------------------------------------------------------------

static void SetupBonesOnBaseAnimating( C_BaseAnimating *&pBaseAnimating )
{
	if ( !pBaseAnimating->GetMoveParent() )
		pBaseAnimating->SetupBones( NULL, -1, -1, gpGlobals->curtime );
}

static void PreThreadedBoneSetup()
{
	mdlcache->BeginLock();
}

static void PostThreadedBoneSetup()
{
	mdlcache->EndLock();
}

static bool g_bInThreadedBoneSetup;
static bool g_bDoThreadedBoneSetup;

void C_BaseAnimating::InitBoneSetupThreadPool()
{
}				 

void C_BaseAnimating::ShutdownBoneSetupThreadPool()
{
}

void C_BaseAnimating::ThreadedBoneSetup()
{
	g_bDoThreadedBoneSetup = cl_threaded_bone_setup.GetBool();
	if ( g_bDoThreadedBoneSetup )
	{
		int nCount = g_PreviousBoneSetups.Count();
		if ( nCount > 1 )
		{
			g_bInThreadedBoneSetup = true;

			ParallelProcess( "C_BaseAnimating::ThreadedBoneSetup", g_PreviousBoneSetups.Base(), nCount, &SetupBonesOnBaseAnimating, &PreThreadedBoneSetup, &PostThreadedBoneSetup );

			g_bInThreadedBoneSetup = false;
		}
	}
	g_iPreviousBoneCounter++;
	g_PreviousBoneSetups.RemoveAll();
}

bool C_BaseAnimating::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	VPROF_BUDGET( "C_BaseAnimating::SetupBones", VPROF_BUDGETGROUP_CLIENT_ANIMATION );

	//=============================================================================
	// HPE_BEGIN:
	// [pfreese] Added the check for pBoneToWorldOut != NULL in this debug warning
	// code. SetupBones is called in the CSS anytime an attachment wants its
	// parent's transform, hence this warning is hit extremely frequently.
	// I'm not actually sure if this is the right "fix" for this, as the bones are
	// actually accessed as part of the setup process, but since I'm not clear on the
	// purpose of this dev warning, I'm including this comment block.
	//=============================================================================

	if ( pBoneToWorldOut != NULL && !IsBoneAccessAllowed() )
	{
		static float lastWarning = 0.0f;

		// Prevent spammage!!!
		if ( gpGlobals->realtime >= lastWarning + 1.0f )
		{
			DevMsgRT( "*** ERROR: Bone access not allowed (entity %i:%s)\n", index, GetClassname() );
			lastWarning = gpGlobals->realtime;
		}
	}

	//boneMask = BONE_USED_BY_ANYTHING; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.
	
	if ( GetSequence() == -1 )
		 return false;

	if ( boneMask == -1 )
	{
		boneMask = m_iPrevBoneMask;
	}

	// We should get rid of this someday when we have solutions for the odd cases where a bone doesn't
	// get setup and its transform is asked for later.
	if ( cl_SetupAllBones.GetInt() )
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}

	// Set up all bones if recording, too
	if ( IsToolRecording() )
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}

	if ( g_bInThreadedBoneSetup )
	{
		if ( !m_BoneSetupLock.TryLock() )
		{
			return false;
		}
	}

#ifdef DEBUG_BONE_SETUP_THREADING
	if ( cl_warn_thread_contested_bone_setup.GetBool() )
	{
		if ( !m_BoneSetupLock.TryLock() )
		{
			Msg( "Contested bone setup in frame %d!\n", gpGlobals->framecount );
		}
		else
		{
			m_BoneSetupLock.Unlock();
		}
	}
#endif

	AUTO_LOCK( m_BoneSetupLock );

	if ( g_bInThreadedBoneSetup )
	{
		m_BoneSetupLock.Unlock();
	}

	if ( m_iMostRecentModelBoneCounter != g_iModelBoneCounter )
	{
		// Clear out which bones we've touched this frame if this is 
		// the first time we've seen this object this frame.
		if ( LastBoneChangedTime() >= m_flLastBoneSetupTime )
		{
			m_BoneAccessor.SetReadableBones( 0 );
			m_BoneAccessor.SetWritableBones( 0 );
			m_flLastBoneSetupTime = currentTime;
		}
		m_iPrevBoneMask = m_iAccumulatedBoneMask;
		m_iAccumulatedBoneMask = 0;

#ifdef STUDIO_ENABLE_PERF_COUNTERS
		CStudioHdr *hdr = GetModelPtr();
		if (hdr)
		{
			hdr->ClearPerfCounters();
		}
#endif
	}

	int nBoneCount = m_CachedBoneData.Count();
	if ( g_bDoThreadedBoneSetup && !g_bInThreadedBoneSetup && ( nBoneCount >= 16 ) && !GetMoveParent() && m_iMostRecentBoneSetupRequest != g_iPreviousBoneCounter )
	{
		m_iMostRecentBoneSetupRequest = g_iPreviousBoneCounter;
		Assert( g_PreviousBoneSetups.Find( this ) == -1 );
		g_PreviousBoneSetups.AddToTail( this );
	}

	// Keep track of everthing asked for over the entire frame
	m_iAccumulatedBoneMask |= boneMask;

	// Make sure that we know that we've already calculated some bone stuff this time around.
	m_iMostRecentModelBoneCounter = g_iModelBoneCounter;

	// Have we cached off all bones meeting the flag set?
	if( ( m_BoneAccessor.GetReadableBones() & boneMask ) != boneMask )
	{
		MDLCACHE_CRITICAL_SECTION();

		CStudioHdr *hdr = GetModelPtr();
		if ( !hdr || !hdr->SequencesAvailable() )
			return false;

		// Setup our transform based on render angles and origin.
		matrix3x4_t parentTransform;
		AngleMatrix( GetRenderAngles(), GetRenderOrigin(), parentTransform );

		// Load the boneMask with the total of what was asked for last frame.
		boneMask |= m_iPrevBoneMask;

		// Allow access to the bones we're setting up so we don't get asserts in here.
		int oldReadableBones = m_BoneAccessor.GetReadableBones();
		m_BoneAccessor.SetWritableBones( m_BoneAccessor.GetReadableBones() | boneMask );
		m_BoneAccessor.SetReadableBones( m_BoneAccessor.GetWritableBones() );

		if (hdr->flags() & STUDIOHDR_FLAGS_STATIC_PROP)
		{
			MatrixCopy(	parentTransform, GetBoneForWrite( 0 ) );
		}
		else
		{
			TrackBoneSetupEnt( this );
			
			// This is necessary because it's possible that CalculateIKLocks will trigger our move children
			// to call GetAbsOrigin(), and they'll use our OLD bone transforms to get their attachments
			// since we're right in the middle of setting up our new transforms. 
			//
			// Setting this flag forces move children to keep their abs transform invalidated.
			AddFlag( EFL_SETTING_UP_BONES );

			// NOTE: For model scaling, we need to opt out of IK because it will mark the bones as already being calculated
			if ( !IsModelScaled() )
			{
				// only allocate an ik block if the npc can use it
				if ( !m_pIk && hdr->numikchains() > 0 && !(m_EntClientFlags & ENTCLIENTFLAG_DONTUSEIK) )
				{
					m_pIk = new CIKContext;
				}
			}
			else
			{
				// Reset the IK
				if ( m_pIk )
				{
					delete m_pIk;
					m_pIk = NULL;
				}
			}

			Vector		pos[MAXSTUDIOBONES];
			Quaternion	q[MAXSTUDIOBONES];
#if defined(FP_EXCEPTIONS_ENABLED) || defined(DBGFLAG_ASSERT)
			// Having these uninitialized means that some bugs are very hard
			// to reproduce. A memset of 0xFF is a simple way of getting NaNs.
			memset( pos, 0xFF, sizeof(pos) );
			memset( q, 0xFF, sizeof(q) );
#endif

			int bonesMaskNeedRecalc = boneMask | oldReadableBones; // Hack to always recalc bones, to fix the arm jitter in the new CS player anims until Ken makes the real fix

			if ( m_pIk )
			{
				if (Teleported() || IsNoInterpolationFrame())
					m_pIk->ClearTargets();

				m_pIk->Init( hdr, GetRenderAngles(), GetRenderOrigin(), currentTime, gpGlobals->framecount, bonesMaskNeedRecalc );
			}

			// Let pose debugger know that we are blending
			g_pPoseDebugger->StartBlending( this, hdr );

			StandardBlendingRules( hdr, pos, q, currentTime, bonesMaskNeedRecalc );

			CBoneBitList boneComputed;
			// don't calculate IK on ragdolls
			if ( m_pIk && !IsRagdoll() )
			{
				UpdateIKLocks( currentTime );

				m_pIk->UpdateTargets( pos, q, m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );

				CalculateIKLocks( currentTime );
				m_pIk->SolveDependencies( pos, q, m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );
			}

			BuildTransformations( hdr, pos, q, parentTransform, bonesMaskNeedRecalc, boneComputed );
			
			RemoveFlag( EFL_SETTING_UP_BONES );
			ControlMouth( hdr );
		}
		
		if( !( oldReadableBones & BONE_USED_BY_ATTACHMENT ) && ( boneMask & BONE_USED_BY_ATTACHMENT ) )
		{
			if ( !SetupBones_AttachmentHelper( hdr ) )
			{
				DevWarning( 2, "SetupBones: SetupBones_AttachmentHelper failed.\n" );
				return false;
			}
		}
	}

	// Do they want to get at the bone transforms? If it's just making sure an aiment has 
	// its bones setup, it doesn't need the transforms yet.
	if ( pBoneToWorldOut )
	{
		if ( nMaxBones >= m_CachedBoneData.Count() )
		{
			memcpy( pBoneToWorldOut, m_CachedBoneData.Base(), sizeof( matrix3x4_t ) * m_CachedBoneData.Count() );
		}
		else
		{
			ExecuteNTimes( 25, Warning( "SetupBones: invalid bone array size (%d - needs %d)\n", nMaxBones, m_CachedBoneData.Count() ) );
			return false;
		}
	}

	return true;
}


C_BaseAnimating* C_BaseAnimating::FindFollowedEntity()
{
	C_BaseEntity *follow = GetFollowedEntity();

	if ( !follow )
		return NULL;

	if ( follow->IsDormant() )
		return NULL;

	if ( !follow->GetModel() )
	{
		Warning( "mod_studio: MOVETYPE_FOLLOW with no model.\n" );
		return NULL;
	}

	if ( modelinfo->GetModelType( follow->GetModel() ) != mod_studio )
	{
		Warning( "Attached %s (mod_studio) to %s (%d)\n", 
			modelinfo->GetModelName( GetModel() ), 
			modelinfo->GetModelName( follow->GetModel() ), 
			modelinfo->GetModelType( follow->GetModel() ) );
		return NULL;
	}

	return assert_cast< C_BaseAnimating* >( follow );
}



void C_BaseAnimating::InvalidateBoneCache()
{
	m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1;
	m_flLastBoneSetupTime = -FLT_MAX; 
}


bool C_BaseAnimating::IsBoneCacheValid() const
{
	return m_iMostRecentModelBoneCounter == g_iModelBoneCounter;
}


// Causes an assert to happen if bones or attachments are used while this is false.
struct BoneAccess
{
	BoneAccess()
	{
		bAllowBoneAccessForNormalModels = false;
		bAllowBoneAccessForViewModels = false;
		tag = NULL;
	}

	bool bAllowBoneAccessForNormalModels;
	bool bAllowBoneAccessForViewModels;
	char const *tag;
};

// the modelcache critical section is insufficient for preventing us from getting into the bone cache at the same time. 
// The bonecache itself is protected by a mutex, but the actual bone access stack needs to be protected separately. 
static CThreadFastMutex g_BoneAccessMutex;
static CUtlVector< BoneAccess >		g_BoneAccessStack;
static BoneAccess g_BoneAcessBase;

bool C_BaseAnimating::IsBoneAccessAllowed() const
{
	if ( IsViewModel() )
		return g_BoneAcessBase.bAllowBoneAccessForViewModels;
	else
		return g_BoneAcessBase.bAllowBoneAccessForNormalModels;
}

// (static function)
void C_BaseAnimating::PushAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels, char const *tagPush )
{
	AUTO_LOCK( g_BoneAccessMutex );
	STAGING_ONLY_EXEC( ReentrancyVerifier rv( &dbg_bonestack_reentrant_count, dbg_bonestack_perturb.GetInt() ) );

	BoneAccess save = g_BoneAcessBase;
	g_BoneAccessStack.AddToTail( save );

	Assert( g_BoneAccessStack.Count() < 32 ); // Most likely we are leaking "PushAllowBoneAccess" calls if PopBoneAccess is never called. Consider using AutoAllowBoneAccess.
	g_BoneAcessBase.bAllowBoneAccessForNormalModels = bAllowForNormalModels;
	g_BoneAcessBase.bAllowBoneAccessForViewModels = bAllowForViewModels;
	g_BoneAcessBase.tag = tagPush;
}

void C_BaseAnimating::PopBoneAccess( char const *tagPop )
{
	AUTO_LOCK( g_BoneAccessMutex );
	STAGING_ONLY_EXEC( ReentrancyVerifier rv( &dbg_bonestack_reentrant_count, dbg_bonestack_perturb.GetInt() ) );

	// Validate that pop matches the push
	Assert( ( g_BoneAcessBase.tag == tagPop ) || ( g_BoneAcessBase.tag && g_BoneAcessBase.tag != ( char const * ) 1 && tagPop && tagPop != ( char const * ) 1 && !strcmp( g_BoneAcessBase.tag, tagPop ) ) );
	int lastIndex = g_BoneAccessStack.Count() - 1;
	if ( lastIndex < 0 )
	{
		Assert( !"C_BaseAnimating::PopBoneAccess:  Stack is empty!!!" );
		return;
	}
	g_BoneAcessBase = g_BoneAccessStack[lastIndex ];
	g_BoneAccessStack.Remove( lastIndex );
}

C_BaseAnimating::AutoAllowBoneAccess::AutoAllowBoneAccess( bool bAllowForNormalModels, bool bAllowForViewModels )
{
	C_BaseAnimating::PushAllowBoneAccess( bAllowForNormalModels, bAllowForViewModels, ( char const * ) 1 );
}

C_BaseAnimating::AutoAllowBoneAccess::~AutoAllowBoneAccess( )
{
	C_BaseAnimating::PopBoneAccess( ( char const * ) 1 );
}

// (static function)
void C_BaseAnimating::InvalidateBoneCaches()
{
	g_iModelBoneCounter++;
}

bool C_BaseAnimating::ShouldDraw()
{
	return !IsDynamicModelLoading() && BaseClass::ShouldDraw();
}

ConVar r_drawothermodels( "r_drawothermodels", "1", FCVAR_CHEAT, "0=Off, 1=Normal, 2=Wireframe" );

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseAnimating::DrawModel( int flags )
{
	VPROF_BUDGET( "C_BaseAnimating::DrawModel", VPROF_BUDGETGROUP_MODEL_RENDERING );
	if ( !m_bReadyToDraw )
		return 0;

	int drawn = 0;

#ifdef TF_CLIENT_DLL
	ValidateModelIndex();
#endif

	if ( r_drawothermodels.GetInt() )
	{
		MDLCACHE_CRITICAL_SECTION();

		int extraFlags = 0;
		if ( r_drawothermodels.GetInt() == 2 )
		{
			extraFlags |= STUDIO_WIREFRAME;
		}

		if ( flags & STUDIO_SHADOWDEPTHTEXTURE )
		{
			extraFlags |= STUDIO_SHADOWDEPTHTEXTURE;
		}

		if ( flags & STUDIO_SSAODEPTHTEXTURE )
		{
			extraFlags |= STUDIO_SSAODEPTHTEXTURE;
		}

		if ( ( flags & ( STUDIO_SSAODEPTHTEXTURE | STUDIO_SHADOWDEPTHTEXTURE ) ) == 0 &&
			g_pStudioStatsEntity != NULL && g_pStudioStatsEntity == GetClientRenderable() )
		{
			extraFlags |= STUDIO_GENERATE_STATS;
		}

		// Necessary for lighting blending
		CreateModelInstance();

		if ( !IsFollowingEntity() )
		{
			drawn = InternalDrawModel( flags|extraFlags );
		}
		else
		{
			// this doesn't draw unless master entity is visible and it's a studio model!!!
			C_BaseAnimating *follow = FindFollowedEntity();
			if ( follow )
			{
				// recompute master entity bone structure
				int baseDrawn = follow->DrawModel( 0 );

				// draw entity
				// FIXME: Currently only draws if aiment is drawn.  
				// BUGBUG: Fixup bbox and do a separate cull for follow object
				if ( baseDrawn )
				{
					drawn = InternalDrawModel( STUDIO_RENDER|extraFlags );
				}
			}
		}
	}

	// If we're visualizing our bboxes, draw them
	DrawBBoxVisualizations();

	return drawn;
}

//-----------------------------------------------------------------------------
// Gets the hitbox-to-world transforms, returns false if there was a problem
//-----------------------------------------------------------------------------
bool C_BaseAnimating::HitboxToWorldTransforms( matrix3x4_t *pHitboxToWorld[MAXSTUDIOBONES] )
{
	MDLCACHE_CRITICAL_SECTION();

	if ( !GetModel() )
		return false;

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( GetHitboxSet() );
	if ( !set )
		return false;

	if ( !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache( pStudioHdr );
	pCache->ReadCachedBonePointers( pHitboxToWorld, pStudioHdr->numbones() );
	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( m_hLightingOriginRelative.Get() )
	{
		C_InfoLightingRelative *pInfoLighting = assert_cast<C_InfoLightingRelative*>( m_hLightingOriginRelative.Get() );
		pInfoLighting->GetLightingOffset( pInfo->lightingOffset );
		pInfo->pLightingOffset = &pInfo->lightingOffset;
	}
	if ( m_hLightingOrigin )
	{
		pInfo->pLightingOrigin = &(m_hLightingOrigin->GetAbsOrigin());
	}

	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void C_BaseAnimating::DoInternalDrawModel( ClientModelRenderInfo_t *pInfo, DrawModelState_t *pState, matrix3x4_t *pBoneToWorldArray )
{
	if ( pState)
	{
		modelrender->DrawModelExecute( *pState, *pInfo, pBoneToWorldArray );
	}

	if ( vcollide_wireframe.GetBool() )
	{
		if ( IsRagdoll() )
		{
			m_pRagdoll->DrawWireframe();
		}
		else if ( IsSolid() && CollisionProp()->GetSolid() == SOLID_VPHYSICS )
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			if ( pCollide && pCollide->solidCount == 1 )
			{
				static color32 debugColor = {0,255,255,0};
				matrix3x4_t matrix;
				AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
				engine->DebugDrawPhysCollide( pCollide->solids[0], NULL, matrix, debugColor );
				if ( VPhysicsGetObject() )
				{
					static color32 debugColorPhys = {255,0,0,0};
					matrix3x4_t matrix;
					VPhysicsGetObject()->GetPositionMatrix( &matrix );
					engine->DebugDrawPhysCollide( pCollide->solids[0], NULL, matrix, debugColorPhys );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseAnimating::InternalDrawModel( int flags )
{
	VPROF( "C_BaseAnimating::InternalDrawModel" );

	if ( !GetModel() )
		return 0;

	// This should never happen, but if the server class hierarchy has bmodel entities derived from CBaseAnimating or does a
	//  SetModel with the wrong type of model, this could occur.
	if ( modelinfo->GetModelType( GetModel() ) != mod_studio )
	{
		return BaseClass::DrawModel( flags );
	}

	// Make sure hdr is valid for drawing
	if ( !GetModelPtr() )
		return 0;

	UpdateBoneAttachments( );

	if ( IsEffectActive( EF_ITEM_BLINK ) )
	{
		flags |= STUDIO_ITEM_BLINK;
	}

	ClientModelRenderInfo_t info;
	ClientModelRenderInfo_t *pInfo;

	pInfo = &info;

	pInfo->flags = flags;
	pInfo->pRenderable = this;
	pInfo->instance = GetModelInstance();
	pInfo->entity_index = index;
	pInfo->pModel = GetModel();
	pInfo->origin = GetRenderOrigin();
	pInfo->angles = GetRenderAngles();
	pInfo->skin = GetSkin();
	pInfo->body = GetBody();
	pInfo->hitboxset = m_nHitboxSet;

	if ( !OnInternalDrawModel( pInfo ) )
	{
		return 0;
	}

	Assert( !pInfo->pModelToWorld);
	if ( !pInfo->pModelToWorld )
	{
		pInfo->pModelToWorld = &pInfo->modelToWorld;

		// Turns the origin + angles into a matrix
		AngleMatrix( pInfo->angles, pInfo->origin, pInfo->modelToWorld );
	}

	DrawModelState_t state;
	matrix3x4_t *pBoneToWorld = NULL;
	bool bMarkAsDrawn = modelrender->DrawModelSetup( *pInfo, &state, NULL, &pBoneToWorld );
	
	// Scale the base transform if we don't have a bone hierarchy
	if ( IsModelScaled() )
	{
		CStudioHdr *pHdr = GetModelPtr();
		if ( pHdr && pBoneToWorld && pHdr->numbones() == 1 )
		{
			// Scale the bone to world at this point
			const float flScale = GetModelScale();
			VectorScale( (*pBoneToWorld)[0], flScale, (*pBoneToWorld)[0] );
			VectorScale( (*pBoneToWorld)[1], flScale, (*pBoneToWorld)[1] );
			VectorScale( (*pBoneToWorld)[2], flScale, (*pBoneToWorld)[2] );
		}
	}

	DoInternalDrawModel( pInfo, ( bMarkAsDrawn && ( pInfo->flags & STUDIO_RENDER ) ) ? &state : NULL, pBoneToWorld );

	OnPostInternalDrawModel( pInfo );

	return bMarkAsDrawn;
}

extern ConVar muzzleflash_light;

void C_BaseAnimating::ProcessMuzzleFlashEvent()
{
	// If we have an attachment, then stick a light on it.
	if ( muzzleflash_light.GetBool() )
	{
		//FIXME: We should really use a named attachment for this
		if ( m_Attachments.Count() > 0 )
		{
			Vector vAttachment;
			QAngle dummyAngles;
			GetAttachment( 1, vAttachment, dummyAngles );

			// Make an elight
			dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_MUZZLEFLASH + index );
			el->origin = vAttachment;
			el->radius = random->RandomInt( 32, 64 ); 
			el->decay = el->radius / 0.05f;
			el->die = gpGlobals->curtime + 0.05f;
			el->color.r = 255;
			el->color.g = 192;
			el->color.b = 64;
			el->color.exponent = 5;
		}
	}
}

//-----------------------------------------------------------------------------
// Internal routine to process animation events for studiomodels
//-----------------------------------------------------------------------------
void C_BaseAnimating::DoAnimationEvents( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr )
		return;

#ifdef DEBUG
	bool watch = dbganimmodel.GetString()[0] && V_stristr( pStudioHdr->pszName(), dbganimmodel.GetString() );
#else
	bool watch = false; // Q_strstr( hdr->name, "rifle" ) ? true : false;
#endif

	//Adrian: eh? This should never happen.
	if ( GetSequence() == -1 )
		 return;

	// build root animation
	float flEventCycle = GetCycle();

	// If we're invisible, don't draw the muzzle flash
	bool bIsInvisible = !IsVisible() && !IsViewModel() && !IsMenuModel();

	if ( bIsInvisible && !clienttools->IsInRecordingMode() )
		return;

	// add in muzzleflash effect
	if ( ShouldMuzzleFlash() )
	{
		DisableMuzzleFlash();
		
		ProcessMuzzleFlashEvent();
	}

	// If we're invisible, don't process animation events.
	if ( bIsInvisible )
		return;

	// If we don't have any sequences, don't do anything
	int nStudioNumSeq = pStudioHdr->GetNumSeq();
	if ( nStudioNumSeq < 1 )
	{
		Warning( "%s[%d]: no sequences?\n", GetDebugName(), entindex() );
		Assert( nStudioNumSeq >= 1 );
		return;
	}

	int nSeqNum = GetSequence();
	if ( nSeqNum >= nStudioNumSeq )
	{
		// This can happen e.g. while reloading Heavy's shotgun, switch to the minigun.
		Warning( "%s[%d]: Playing sequence %d but there's only %d in total?\n", GetDebugName(), entindex(), nSeqNum, nStudioNumSeq );
		return;
	}

	mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( nSeqNum );

	if (seqdesc.numevents == 0)
		return;

	// Forces anim event indices to get set and returns pEvent(0);
	mstudioevent_t *pevent = GetEventIndexForSequence( seqdesc );

	if ( watch )
	{
		Msg( "%i cycle %f\n", gpGlobals->tickcount, GetCycle() );
	}

	bool resetEvents = m_nResetEventsParity != m_nPrevResetEventsParity;
	m_nPrevResetEventsParity = m_nResetEventsParity;

	if (m_nEventSequence != GetSequence() || resetEvents )
	{
		if ( watch )
		{
			Msg( "new seq: %i - old seq: %i - reset: %s - m_flCycle %f - Model Name: %s - (time %.3f)\n",
				GetSequence(), m_nEventSequence,
				resetEvents ? "true" : "false",
				GetCycle(), pStudioHdr->pszName(),
				gpGlobals->curtime);
		}

		m_nEventSequence = GetSequence();
		flEventCycle = 0.0f;
		m_flPrevEventCycle = -0.01; // back up to get 0'th frame animations
	}

	// stalled?
	if (flEventCycle == m_flPrevEventCycle)
		return;

	if ( watch )
	{
		 Msg( "%i (seq %d cycle %.3f ) evcycle %.3f prevevcycle %.3f (time %.3f)\n",
			 gpGlobals->tickcount, 
			 GetSequence(),
			 GetCycle(),
			 flEventCycle,
			 m_flPrevEventCycle,
			 gpGlobals->curtime );
	}

	// check for looping
	BOOL bLooped = false;
	if (flEventCycle <= m_flPrevEventCycle)
	{
		if (m_flPrevEventCycle - flEventCycle > 0.5)
		{
			bLooped = true;
		}
		else
		{
			// things have backed up, which is bad since it'll probably result in a hitch in the animation playback
			// but, don't play events again for the same time slice
			return;
		}
	}

	// This makes sure events that occur at the end of a sequence occur are
	// sent before events that occur at the beginning of a sequence.
	if (bLooped)
	{
		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			// ignore all non-client-side events

			if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
			{
				if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
					 continue;
			}
			else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
				continue;
		
			if ( pevent[i].cycle <= m_flPrevEventCycle )
				continue;
			
			if ( watch )
			{
				Msg( "%i FE %i Looped cycle %f, prev %f ev %f (time %.3f)\n",
					gpGlobals->tickcount,
					pevent[i].event,
					pevent[i].cycle,
					m_flPrevEventCycle,
					flEventCycle,
					gpGlobals->curtime );
			}
				
				
			FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
		}

		// Necessary to get the next loop working
		m_flPrevEventCycle = flEventCycle - 0.001f;
	}

	for (int i = 0; i < (int)seqdesc.numevents; i++)
	{
		if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
		{
			if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
				 continue;
		}
		else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
			continue;

		if ( (pevent[i].cycle > m_flPrevEventCycle && pevent[i].cycle <= flEventCycle) )
		{
			if ( watch )
			{
				Msg( "%i (seq: %d) FE %i Normal cycle %f, prev %f ev %f (time %.3f)\n",
					gpGlobals->tickcount,
					GetSequence(),
					pevent[i].event,
					pevent[i].cycle,
					m_flPrevEventCycle,
					flEventCycle,
					gpGlobals->curtime );
			}

			FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
		}
	}

	m_flPrevEventCycle = flEventCycle;
}

//-----------------------------------------------------------------------------
// Purpose: Parses a muzzle effect event and sends it out for drawing
// Input  : *options - event parameters in text format
//			isFirstPerson - whether this is coming from an NPC or the player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseAnimating::DispatchMuzzleEffect( const char *options, bool isFirstPerson )
{
	const char	*p = options;
	char		token[128];
	int			weaponType = 0;

	// Get the first parameter
	p = nexttoken( token, p, ' ' );

	// Find the weapon type
	if ( token[0] ) 
	{
		//TODO: Parse the type from a list instead
		if ( Q_stricmp( token, "COMBINE" ) == 0 )
		{
			weaponType = MUZZLEFLASH_COMBINE;
		}
		else if ( Q_stricmp( token, "SMG1" ) == 0 )
		{
			weaponType = MUZZLEFLASH_SMG1;
		}
		else if ( Q_stricmp( token, "PISTOL" ) == 0 )
		{
			weaponType = MUZZLEFLASH_PISTOL;
		}
		else if ( Q_stricmp( token, "SHOTGUN" ) == 0 )
		{
			weaponType = MUZZLEFLASH_SHOTGUN;
		}
		else if ( Q_stricmp( token, "357" ) == 0 )
		{
			weaponType = MUZZLEFLASH_357;
		}
		else if ( Q_stricmp( token, "RPG" ) == 0 )
		{
			weaponType = MUZZLEFLASH_RPG;
		}
		else
		{
			//NOTENOTE: This means you specified an invalid muzzleflash type, check your spelling?
			Assert( 0 );
		}
	}
	else
	{
		//NOTENOTE: This means that there wasn't a proper parameter passed into the animevent
		Assert( 0 );
		return false;
	}

	// Get the second parameter
	p = nexttoken( token, p, ' ' );

	int	attachmentIndex = -1;

	// Find the attachment name
	if ( token[0] ) 
	{
		attachmentIndex = LookupAttachment( token );

		// Found an invalid attachment
		if ( attachmentIndex <= 0 )
		{
			//NOTENOTE: This means that the attachment you're trying to use is invalid
			Assert( 0 );
			return false;
		}
	}
	else
	{
		//NOTENOTE: This means that there wasn't a proper parameter passed into the animevent
		Assert( 0 );
		return false;
	}

	// Send it out
	tempents->MuzzleFlash( weaponType, GetRefEHandle(), attachmentIndex, isFirstPerson );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MaterialFootstepSound( C_BaseAnimating *pEnt, bool bLeftFoot, float flVolume )
{
	trace_t tr;
	Vector traceStart;
	QAngle angles;

	int attachment;

	//!!!PERF - These string lookups here aren't the swiftest, but
	// this doesn't get called very frequently unless a lot of NPCs
	// are using this code.
	if( bLeftFoot )
	{
		attachment = pEnt->LookupAttachment( "LeftFoot" );
	}
	else
	{
		attachment = pEnt->LookupAttachment( "RightFoot" );
	}

	if( attachment == -1 )
	{
		// Exit if this NPC doesn't have the proper attachments.
		return;
	}

	pEnt->GetAttachment( attachment, traceStart, angles );

	UTIL_TraceLine( traceStart, traceStart - Vector( 0, 0, 48.0f), MASK_SHOT_HULL, pEnt, COLLISION_GROUP_NONE, &tr );
	if( tr.fraction < 1.0 && tr.m_pEnt )
	{
		surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			EmitSound_t params;
			if( bLeftFoot )
			{
				params.m_pSoundName = physprops->GetString(psurf->sounds.stepleft);
			}
			else
			{
				params.m_pSoundName = physprops->GetString(psurf->sounds.stepright);
			}

			CPASAttenuationFilter filter( pEnt, params.m_pSoundName );

			params.m_bWarnOnDirectWaveReference = true;
			params.m_flVolume = flVolume;

			pEnt->EmitSound( filter, pEnt->entindex(), params );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *origin - 
//			*angles - 
//			event - 
//			*options - 
//			numAttachments - 
//			attachments[] - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	Vector attachOrigin;
	QAngle attachAngles; 

	switch( event )
	{
	case AE_CL_CREATE_PARTICLE_EFFECT:
		{
			int iAttachment = -1;
			int iAttachType = PATTACH_ABSORIGIN_FOLLOW;
			char token[256];
			char szParticleEffect[256];

			// Get the particle effect name
			const char *p = options;
			p = nexttoken(token, p, ' ');

			const char* mtoken = ModifyEventParticles( token );
			if ( !mtoken || mtoken[0] == '\0' )
				return;
			Q_strncpy( szParticleEffect, mtoken, sizeof(szParticleEffect) );

			// Get the attachment type
			p = nexttoken(token, p, ' ');

			iAttachType = GetAttachTypeFromString( token );
			if ( iAttachType == -1 )
			{
				Warning("Invalid attach type specified for particle effect anim event. Trying to spawn effect '%s' with attach type of '%s'\n", szParticleEffect, token );
				return;
			}

			// Get the attachment point index
			p = nexttoken(token, p, ' ');
			if ( token[0] )
			{
				iAttachment = atoi(token);

				// See if we can find any attachment points matching the name
				if ( token[0] != '0' && iAttachment == 0 )
				{
					iAttachment = LookupAttachment( token );
					if ( iAttachment <= 0 )
					{
						Warning( "Failed to find attachment point specified for particle effect anim event. Trying to spawn effect '%s' on attachment named '%s'\n", szParticleEffect, token );
						return;
					}
				}
			}

			// Spawn the particle effect
			ParticleProp()->Create( szParticleEffect, (ParticleAttachment_t)iAttachType, iAttachment );
		}
		break;

	case AE_CL_PLAYSOUND:
		{
			CLocalPlayerFilter filter;

			if ( m_Attachments.Count() > 0)
			{
				GetAttachment( 1, attachOrigin, attachAngles );
				EmitSound( filter, GetSoundSourceIndex(), options, &attachOrigin );
			}
			else
			{
				EmitSound( filter, GetSoundSourceIndex(), options, &GetAbsOrigin() );
			} 
		}
		break;
	case AE_CL_STOPSOUND:
		{
			StopSound( GetSoundSourceIndex(), options );
		}
		break;

	case CL_EVENT_FOOTSTEP_LEFT:
		{
#ifndef HL2MP
			char pSoundName[256];
			if ( !options || !options[0] )
			{
				options = "NPC_CombineS";
			}

			Vector vel;
			EstimateAbsVelocity( vel );

			// If he's moving fast enough, play the run sound
			if ( vel.Length2DSqr() > RUN_SPEED_ESTIMATE_SQR )
			{
				Q_snprintf( pSoundName, 256, "%s.RunFootstepLeft", options );
			}
			else
			{
				Q_snprintf( pSoundName, 256, "%s.FootstepLeft", options );
			}
			EmitSound( pSoundName );
#endif
		}
		break;

	case CL_EVENT_FOOTSTEP_RIGHT:
		{
#ifndef HL2MP
			char pSoundName[256];
			if ( !options || !options[0] )
			{
				options = "NPC_CombineS";
			}

			Vector vel;
			EstimateAbsVelocity( vel );
			// If he's moving fast enough, play the run sound
			if ( vel.Length2DSqr() > RUN_SPEED_ESTIMATE_SQR )
			{
				Q_snprintf( pSoundName, 256, "%s.RunFootstepRight", options );
			}
			else
			{
				Q_snprintf( pSoundName, 256, "%s.FootstepRight", options );
			}
			EmitSound( pSoundName );
#endif
		}
		break;

	case CL_EVENT_MFOOTSTEP_LEFT:
		{
			MaterialFootstepSound( this, true, VOL_NORM * 0.5f );
		}
		break;

	case CL_EVENT_MFOOTSTEP_RIGHT:
		{
			MaterialFootstepSound( this, false, VOL_NORM * 0.5f );
		}
		break;

	case CL_EVENT_MFOOTSTEP_LEFT_LOUD:
		{
			MaterialFootstepSound( this, true, VOL_NORM );
		}
		break;

	case CL_EVENT_MFOOTSTEP_RIGHT_LOUD:
		{
			MaterialFootstepSound( this, false, VOL_NORM );
		}
		break;

	// Eject brass
	case CL_EVENT_EJECTBRASS1:
		if ( m_Attachments.Count() > 0 )
		{
			if ( MainViewOrigin().DistToSqr( GetAbsOrigin() ) < (256 * 256) )
			{
				Vector attachOrigin;
				QAngle attachAngles; 
				
				if( GetAttachment( 2, attachOrigin, attachAngles ) )
				{
					tempents->EjectBrass( attachOrigin, attachAngles, GetAbsAngles(), atoi( options ) );
				}
			}
		}
		break;

	case AE_MUZZLEFLASH:
		{
			// Send out the effect for a player
			DispatchMuzzleEffect( options, true );
			break;
		}

	case AE_NPC_MUZZLEFLASH:
		{
			// Send out the effect for an NPC
			DispatchMuzzleEffect( options, false );
			break;
		}

	// OBSOLETE EVENTS. REPLACED BY NEWER SYSTEMS.
	// See below in FireObsoleteEvent() for comments on what to use instead.
	case AE_CLIENT_EFFECT_ATTACH:
	case CL_EVENT_DISPATCHEFFECT0:
	case CL_EVENT_DISPATCHEFFECT1:
	case CL_EVENT_DISPATCHEFFECT2:
	case CL_EVENT_DISPATCHEFFECT3:
	case CL_EVENT_DISPATCHEFFECT4:
	case CL_EVENT_DISPATCHEFFECT5:
	case CL_EVENT_DISPATCHEFFECT6:
	case CL_EVENT_DISPATCHEFFECT7:
	case CL_EVENT_DISPATCHEFFECT8:
	case CL_EVENT_DISPATCHEFFECT9:
	case CL_EVENT_MUZZLEFLASH0:
	case CL_EVENT_MUZZLEFLASH1:
	case CL_EVENT_MUZZLEFLASH2:
	case CL_EVENT_MUZZLEFLASH3:
	case CL_EVENT_NPC_MUZZLEFLASH0:
	case CL_EVENT_NPC_MUZZLEFLASH1:
	case CL_EVENT_NPC_MUZZLEFLASH2:
	case CL_EVENT_NPC_MUZZLEFLASH3:
	case CL_EVENT_SPARK0:
	case CL_EVENT_SOUND:
		FireObsoleteEvent( origin, angles, event, options );
		break;

	case AE_CL_ENABLE_BODYGROUP:
		{
			int index = FindBodygroupByName( options );
			if ( index >= 0 )
			{
				SetBodygroup( index, 1 );
			}
		}
		break;

	case AE_CL_DISABLE_BODYGROUP:
		{
			int index = FindBodygroupByName( options );
			if ( index >= 0 )
			{
				SetBodygroup( index, 0 );
			}
		}
		break;

	case AE_CL_BODYGROUP_SET_VALUE:
		{
			int value;
			char token[256];
			char szBodygroupName[256];

			const char *p = options;

			// Bodygroup Name
			p = nexttoken(token, p, ' ');
			Q_strncpy( szBodygroupName, token, sizeof(szBodygroupName) );

			// Get the desired value
			p = nexttoken(token, p, ' ');
			value = token[0] ? atoi( token ) : 0;

			int index = FindBodygroupByName( szBodygroupName );
			if ( index >= 0 )
			{
				SetBodygroup( index, value );
			}
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: These events are all obsolete events, left here to support old games.
//			Their systems have all been replaced with better ones.
//-----------------------------------------------------------------------------
void C_BaseAnimating::FireObsoleteEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	Vector attachOrigin;
	QAngle attachAngles; 

	switch( event )
	{
	// Obsolete. Use the AE_CL_CREATE_PARTICLE_EFFECT event instead, which uses the artist driven particle system & editor.
	case AE_CLIENT_EFFECT_ATTACH:
		{
			int iAttachment;
			int iParam;
			char token[128];
			char effectFunc[128];

			const char *p = options;

			p = nexttoken(token, p, ' ');
			Q_strncpy( effectFunc, token, sizeof(effectFunc) );

			p = nexttoken(token, p, ' ');
			iAttachment = token[0] ? atoi(token) : -1;

			p = nexttoken(token, p, ' ');
			iParam = token[0] ? atoi(token) : 0;

			if ( iAttachment != -1 && m_Attachments.Count() >= iAttachment )
			{
				GetAttachment( iAttachment, attachOrigin, attachAngles );

				// Fill out the generic data
				CEffectData data;
				data.m_vOrigin = attachOrigin;
				data.m_vAngles = attachAngles;
				AngleVectors( attachAngles, &data.m_vNormal );
				data.m_hEntity = GetRefEHandle();
				data.m_nAttachmentIndex = iAttachment + 1;
				data.m_fFlags = iParam;

				DispatchEffect( effectFunc, data );
			}
		}
		break;

	// Obsolete. Use the AE_CL_CREATE_PARTICLE_EFFECT event instead, which uses the artist driven particle system & editor.
	case CL_EVENT_DISPATCHEFFECT0:
	case CL_EVENT_DISPATCHEFFECT1:
	case CL_EVENT_DISPATCHEFFECT2:
	case CL_EVENT_DISPATCHEFFECT3:
	case CL_EVENT_DISPATCHEFFECT4:
	case CL_EVENT_DISPATCHEFFECT5:
	case CL_EVENT_DISPATCHEFFECT6:
	case CL_EVENT_DISPATCHEFFECT7:
	case CL_EVENT_DISPATCHEFFECT8:
	case CL_EVENT_DISPATCHEFFECT9:
		{
			int iAttachment = -1;

			// First person muzzle flashes
			switch (event) 
			{
			case CL_EVENT_DISPATCHEFFECT0:
				iAttachment = 0;
				break;

			case CL_EVENT_DISPATCHEFFECT1:
				iAttachment = 1;
				break;

			case CL_EVENT_DISPATCHEFFECT2:
				iAttachment = 2;
				break;

			case CL_EVENT_DISPATCHEFFECT3:
				iAttachment = 3;
				break;

			case CL_EVENT_DISPATCHEFFECT4:
				iAttachment = 4;
				break;

			case CL_EVENT_DISPATCHEFFECT5:
				iAttachment = 5;
				break;

			case CL_EVENT_DISPATCHEFFECT6:
				iAttachment = 6;
				break;

			case CL_EVENT_DISPATCHEFFECT7:
				iAttachment = 7;
				break;

			case CL_EVENT_DISPATCHEFFECT8:
				iAttachment = 8;
				break;

			case CL_EVENT_DISPATCHEFFECT9:
				iAttachment = 9;
				break;
			}

			if ( iAttachment != -1 && m_Attachments.Count() > iAttachment )
			{
				GetAttachment( iAttachment+1, attachOrigin, attachAngles );

				// Fill out the generic data
				CEffectData data;
				data.m_vOrigin = attachOrigin;
				data.m_vAngles = attachAngles;
				AngleVectors( attachAngles, &data.m_vNormal );
				data.m_hEntity = GetRefEHandle();
				data.m_nAttachmentIndex = iAttachment + 1;

				DispatchEffect( options, data );
			}
		}
		break;

	// Obsolete. Use the AE_MUZZLEFLASH / AE_NPC_MUZZLEFLASH events instead.
	case CL_EVENT_MUZZLEFLASH0:
	case CL_EVENT_MUZZLEFLASH1:
	case CL_EVENT_MUZZLEFLASH2:
	case CL_EVENT_MUZZLEFLASH3:
	case CL_EVENT_NPC_MUZZLEFLASH0:
	case CL_EVENT_NPC_MUZZLEFLASH1:
	case CL_EVENT_NPC_MUZZLEFLASH2:
	case CL_EVENT_NPC_MUZZLEFLASH3:
		{
			int iAttachment = -1;
			bool bFirstPerson = true;

			// First person muzzle flashes
			switch (event) 
			{
			case CL_EVENT_MUZZLEFLASH0:
				iAttachment = 0;
				break;

			case CL_EVENT_MUZZLEFLASH1:
				iAttachment = 1;
				break;

			case CL_EVENT_MUZZLEFLASH2:
				iAttachment = 2;
				break;

			case CL_EVENT_MUZZLEFLASH3:
				iAttachment = 3;
				break;

				// Third person muzzle flashes
			case CL_EVENT_NPC_MUZZLEFLASH0:
				iAttachment = 0;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH1:
				iAttachment = 1;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH2:
				iAttachment = 2;
				bFirstPerson = false;
				break;

			case CL_EVENT_NPC_MUZZLEFLASH3:
				iAttachment = 3;
				bFirstPerson = false;
				break;
			}

			if ( iAttachment != -1 && m_Attachments.Count() > iAttachment )
			{
				GetAttachment( iAttachment+1, attachOrigin, attachAngles );
				int entId = render->GetViewEntity();
				ClientEntityHandle_t hEntity = ClientEntityList().EntIndexToHandle( entId );
				tempents->MuzzleFlash( attachOrigin, attachAngles, atoi( options ), hEntity, bFirstPerson );
			}
		}
		break;

	// Obsolete: Use the AE_CL_CREATE_PARTICLE_EFFECT event instead, which uses the artist driven particle system & editor.
	case CL_EVENT_SPARK0:
		{
			Vector vecForward;
			GetAttachment( 1, attachOrigin, attachAngles );
			AngleVectors( attachAngles, &vecForward );
			g_pEffects->Sparks( attachOrigin, atoi( options ), 1, &vecForward );
		}
		break;

	// Obsolete: Use the AE_CL_PLAYSOUND event instead, which doesn't rely on a magic number in the .qc
	case CL_EVENT_SOUND:
		{
			CLocalPlayerFilter filter;

			if ( m_Attachments.Count() > 0)
			{
				GetAttachment( 1, attachOrigin, attachAngles );
				EmitSound( filter, GetSoundSourceIndex(), options, &attachOrigin );
			}
			else
			{
				EmitSound( filter, GetSoundSourceIndex(), options );
			}
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsSelfAnimating()
{
	if ( m_bClientSideAnimation )
		return true;

	// Yes, we use animtime.
	int iMoveType = GetMoveType();
	if ( iMoveType != MOVETYPE_STEP && 
		  iMoveType != MOVETYPE_NONE && 
		  iMoveType != MOVETYPE_WALK &&
		  iMoveType != MOVETYPE_FLY &&
		  iMoveType != MOVETYPE_FLYGRAVITY )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called by networking code when an entity is new to the PVS or comes down with the EF_NOINTERP flag set.
//  The position history data is flushed out right after this call, so we need to store off the current data
//  in the latched fields so we try to interpolate
// Input  : *ent - 
//			full_reset - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::ResetLatched( void )
{
	// Reset the IK
	if ( m_pIk )
	{
		delete m_pIk;
		m_pIk = NULL;
	}

	BaseClass::ResetLatched();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------

bool C_BaseAnimating::Interpolate( float flCurrentTime )
{
	// ragdolls don't need interpolation
	if ( m_pRagdoll )
		return true;

	VPROF( "C_BaseAnimating::Interpolate" );

	Vector oldOrigin;
	QAngle oldAngles;
	Vector oldVel;
	float flOldCycle = GetCycle();
	int nChangeFlags = 0;

	if ( !m_bClientSideAnimation )
		m_iv_flCycle.SetLooping( IsSequenceLooping( GetSequence() ) );

	int bNoMoreChanges;
	int retVal = BaseInterpolatePart1( flCurrentTime, oldOrigin, oldAngles, oldVel, bNoMoreChanges );
	if ( retVal == INTERPOLATE_STOP )
	{
		if ( bNoMoreChanges )
			RemoveFromInterpolationList();
		return true;
	}


	// Did cycle change?
	if( GetCycle() != flOldCycle )
		nChangeFlags |= ANIMATION_CHANGED;

	if ( bNoMoreChanges )
		RemoveFromInterpolationList();
	
	BaseInterpolatePart2( oldOrigin, oldAngles, oldVel, nChangeFlags );
	return true;
}


//-----------------------------------------------------------------------------
// returns true if we're currently being ragdolled
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsRagdoll() const
{
	return m_pRagdoll && (m_nRenderFX == kRenderFxRagdoll);
}

//-----------------------------------------------------------------------------
// returns true if we're currently being ragdolled
//-----------------------------------------------------------------------------
bool C_BaseAnimating::IsAboutToRagdoll() const
{
	return (m_nRenderFX == kRenderFxRagdoll);
}


//-----------------------------------------------------------------------------
// Lets us check our sequence number after a network update
//-----------------------------------------------------------------------------
int C_BaseAnimating::RestoreData( const char *context, int slot, int type )
{
	int retVal = BaseClass::RestoreData( context, slot, type );
	CStudioHdr *pHdr = GetModelPtr();
	if( pHdr && m_nSequence >= pHdr->GetNumSeq() )
	{
		// Don't let a network update give us an invalid sequence
		m_nSequence = 0;
	}
	return retVal;
}


//-----------------------------------------------------------------------------
// implements these so ragdolls can handle frustum culling & leaf visibility
//-----------------------------------------------------------------------------

void C_BaseAnimating::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if ( IsRagdoll() )
	{
		m_pRagdoll->GetRagdollBounds( theMins, theMaxs );
	}
	else if ( GetModel() )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		if ( !pStudioHdr|| !pStudioHdr->SequencesAvailable() || GetSequence() == -1 )
		{
			theMins = vec3_origin;
			theMaxs = vec3_origin;
			return;
		} 
		if (!VectorCompare( vec3_origin, pStudioHdr->view_bbmin() ) || !VectorCompare( vec3_origin, pStudioHdr->view_bbmax() ))
		{
			// clipping bounding box
			VectorCopy ( pStudioHdr->view_bbmin(), theMins);
			VectorCopy ( pStudioHdr->view_bbmax(), theMaxs);
		}
		else
		{
			// movement bounding box
			VectorCopy ( pStudioHdr->hull_min(), theMins);
			VectorCopy ( pStudioHdr->hull_max(), theMaxs);
		}

		mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( GetSequence() );
		VectorMin( seqdesc.bbmin, theMins, theMins );
		VectorMax( seqdesc.bbmax, theMaxs, theMaxs );
	}
	else
	{
		theMins = vec3_origin;
		theMaxs = vec3_origin;
	}

	// Scale this up depending on if our model is currently scaling
	const float flScale = GetModelScale();
	theMaxs *= flScale;
	theMins *= flScale;
}


//-----------------------------------------------------------------------------
// implements these so ragdolls can handle frustum culling & leaf visibility
//-----------------------------------------------------------------------------
const Vector& C_BaseAnimating::GetRenderOrigin( void )
{
	if ( IsRagdoll() )
	{
		return m_pRagdoll->GetRagdollOrigin();
	}
	else
	{
		return BaseClass::GetRenderOrigin();	
	}
}

const QAngle& C_BaseAnimating::GetRenderAngles( void )
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
			
	}
	else
	{
		return BaseClass::GetRenderAngles();	
	}
}

void C_BaseAnimating::RagdollMoved( void ) 
{
	SetAbsOrigin( m_pRagdoll->GetRagdollOrigin() );
	SetAbsAngles( vec3_angle );

	Vector mins, maxs;
	m_pRagdoll->GetRagdollBounds( mins, maxs );
	SetCollisionBounds( mins, maxs );

	// If the ragdoll moves, its render-to-texture shadow is dirty
	InvalidatePhysicsRecursive( ANIMATION_CHANGED ); 
}


//-----------------------------------------------------------------------------
// Purpose: My physics object has been updated, react or extract data
//-----------------------------------------------------------------------------
void C_BaseAnimating::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	// FIXME: Should make sure the physics objects being passed in
	// is the ragdoll physics object, but I think it's pretty safe not to check
	if (IsRagdoll())
	{	 
		m_pRagdoll->VPhysicsUpdate( pPhysics );
		
		RagdollMoved();

		return;
	}

	BaseClass::VPhysicsUpdate( pPhysics );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::PreDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_BaseAnimating::PreDataUpdate" );

	m_flOldCycle = GetCycle();
	m_nOldSequence = GetSequence();
	m_flOldModelScale = GetModelScale();

	int i;
	for ( i=0;i<MAXSTUDIOBONECTRLS;i++ )
	{
		m_flOldEncodedController[i] = m_flEncodedController[i];
	}

	for ( i=0;i<MAXSTUDIOPOSEPARAM;i++ )
	{
		 m_flOldPoseParameters[i] = m_flPoseParameter[i];
	}

	BaseClass::PreDataUpdate( updateType );
}

void C_BaseAnimating::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit( state );

	if ( state == SHOULDTRANSMIT_START )
	{
		// If he's been firing a bunch, then he comes back into the PVS, his muzzle flash
		// will show up even if he isn't firing now.
		DisableMuzzleFlash();

		m_nPrevResetEventsParity = m_nResetEventsParity;
		m_nEventSequence = GetSequence();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( m_bClientSideAnimation )
	{
		SetCycle( m_flOldCycle );
		AddToClientSideAnimationList();
	}
	else
	{
		RemoveFromClientSideAnimationList();
	}

	bool bBoneControllersChanged = false;

	int i;
	for ( i=0;i<MAXSTUDIOBONECTRLS && !bBoneControllersChanged;i++ )
	{
		if ( m_flOldEncodedController[i] != m_flEncodedController[i] )
		{
			bBoneControllersChanged = true;
		}
	}

	bool bPoseParametersChanged = false;

	for ( i=0;i<MAXSTUDIOPOSEPARAM && !bPoseParametersChanged;i++ )
	{
		if ( m_flOldPoseParameters[i] != m_flPoseParameter[i] )
		{
			bPoseParametersChanged = true;
		}
	}

	// Cycle change? Then re-render
	bool bAnimationChanged = m_flOldCycle != GetCycle() || bBoneControllersChanged || bPoseParametersChanged;
	bool bSequenceChanged = m_nOldSequence != GetSequence();
	bool bScaleChanged = ( m_flOldModelScale != GetModelScale() );
	if ( bAnimationChanged || bSequenceChanged || bScaleChanged )
	{
		InvalidatePhysicsRecursive( ANIMATION_CHANGED );
	}

	if ( bAnimationChanged || bSequenceChanged )
	{
		if ( m_bClientSideAnimation )
		{
			ClientSideAnimationChanged();
		}
	}

	// reset prev cycle if new sequence
	if (m_nNewSequenceParity != m_nPrevNewSequenceParity)
	{
		// It's important not to call Reset() on a static prop, because if we call
		// Reset(), then the entity will stay in the interpolated entities list
		// forever, wasting CPU.
		MDLCACHE_CRITICAL_SECTION();
		CStudioHdr *hdr = GetModelPtr();
		if ( hdr && !( hdr->flags() & STUDIOHDR_FLAGS_STATIC_PROP ) )
		{
			m_iv_flCycle.Reset();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bLastClientSideFrameReset = m_bClientSideFrameReset;
}

bool C_BaseAnimating::ForceSetupBonesAtTime( matrix3x4_t *pBonesOut, float flTime )
{
	// blow the cached prev bones
	InvalidateBoneCache();

	// reset root position to flTime
	Interpolate( flTime );

	// Setup bone state at the given time
	return SetupBones( pBonesOut, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, flTime );
}

bool C_BaseAnimating::GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt )
{
	bool bSuccess = true;

	if ( !ForceSetupBonesAtTime( pDeltaBones0, gpGlobals->curtime - boneDt ) )
		bSuccess = false;
	if ( !ForceSetupBonesAtTime( pDeltaBones1, gpGlobals->curtime ) )
		bSuccess = false;

	float ragdollCreateTime = PhysGetSyncCreateTime();
	if ( ragdollCreateTime != gpGlobals->curtime )
	{
		// The next simulation frame begins before the end of this frame
		// so initialize the ragdoll at that time so that it will reach the current
		// position at curtime.  Otherwise the ragdoll will simulate forward from curtime
		// and pop into the future a bit at this point of transition
		if ( !ForceSetupBonesAtTime( pCurrentBones, ragdollCreateTime ) )
			bSuccess = false;
	}
	else
	{
		memcpy( pCurrentBones, m_CachedBoneData.Base(), sizeof( matrix3x4_t ) * m_CachedBoneData.Count() );
	}

	return bSuccess;
}

C_BaseAnimating *C_BaseAnimating::CreateRagdollCopy()
{
	//Adrian: We now create a separate entity that becomes this entity's ragdoll.
	//That way the server side version of this entity can go away. 
	//Plus we can hook save/restore code to these ragdolls so they don't fall on restore anymore.
	C_ClientRagdoll *pRagdoll = new C_ClientRagdoll( false );
	if ( pRagdoll == NULL )
		return NULL;

	TermRopes();

	const model_t *model = GetModel();
	const char *pModelName = modelinfo->GetModelName( model );

	if ( pRagdoll->InitializeAsClientEntity( pModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		pRagdoll->Release();
		return NULL;
	}

	// move my current model instance to the ragdoll's so decals are preserved.
	SnatchModelInstance( pRagdoll );

	// We need to take these from the entity
	pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	pRagdoll->SetAbsAngles( GetAbsAngles() );

	pRagdoll->IgniteRagdoll( this );
	pRagdoll->TransferDissolveFrom( this );
	pRagdoll->InitModelEffects();

	if ( AddRagdollToFadeQueue() == true )
	{
		pRagdoll->m_bImportant = NPC_IsImportantNPC( this );
		s_RagdollLRU.MoveToTopOfLRU( pRagdoll, pRagdoll->m_bImportant );
		pRagdoll->m_bFadeOut = true;
	}

	m_builtRagdoll = true;
	AddEffects( EF_NODRAW );

	if ( IsEffectActive( EF_NOSHADOW ) )
	{
		pRagdoll->AddEffects( EF_NOSHADOW );
	}

	pRagdoll->m_nRenderFX = kRenderFxRagdoll;
	pRagdoll->SetRenderMode( GetRenderMode() );
	pRagdoll->SetRenderColor( GetRenderColor().r, GetRenderColor().g, GetRenderColor().b, GetRenderColor().a );

	pRagdoll->m_nBody = m_nBody;
	pRagdoll->m_nSkin = GetSkin();
	pRagdoll->m_vecForce = m_vecForce;
	pRagdoll->m_nForceBone = m_nForceBone;
	pRagdoll->SetNextClientThink( CLIENT_THINK_ALWAYS );

	pRagdoll->SetModelName( AllocPooledString(pModelName) );
	pRagdoll->SetModelScale( GetModelScale() );
	return pRagdoll;
}

C_BaseAnimating *C_BaseAnimating::BecomeRagdollOnClient()
{
	MoveToLastReceivedPosition( true );
	GetAbsOrigin();

	C_BaseAnimating *pRagdoll = CreateRagdollCopy();
	if ( pRagdoll )
	{
		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.1f;

		bool bInitAsClient = false;
		bool bInitBoneArrays = GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );

		if ( bInitBoneArrays )
		{
			bInitAsClient = pRagdoll->InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
		}

		if ( !bInitAsClient || !bInitBoneArrays )
		{
			Warning( "C_BaseAnimating::BecomeRagdollOnClient failed. pRagdoll:%p bInitBoneArrays:%d bInitAsClient:%d\n",
					 pRagdoll, bInitBoneArrays, bInitAsClient );
			pRagdoll->Release();
			return NULL;
		}
	}

	return pRagdoll;
}

bool C_BaseAnimating::InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1, const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr || m_pRagdoll || m_builtRagdoll )
		return false;

	m_builtRagdoll = true;

	// Store off our old mins & maxs
	m_vecPreRagdollMins = WorldAlignMins();
	m_vecPreRagdollMaxs = WorldAlignMaxs();


	// Force MOVETYPE_STEP interpolation
	MoveType_t savedMovetype = GetMoveType();
	SetMoveType( MOVETYPE_STEP );

	// HACKHACK: force time to last interpolation position
	m_flPlaybackRate = 1;
	
	m_pRagdoll = CreateRagdoll( this, hdr, m_vecForce, m_nForceBone, pDeltaBones0, pDeltaBones1, pCurrentBonePosition, boneDt, bFixedConstraints );

	// Cause the entity to recompute its shadow	type and make a
	// version which only updates when physics state changes
	// NOTE: We have to do this after m_pRagdoll is assigned above
	// because that's what ShadowCastType uses to figure out which type of shadow to use.
	DestroyShadow();
	CreateShadow();

	// Cache off ragdoll bone positions/quaternions
	if ( m_bStoreRagdollInfo && m_pRagdoll )
	{
		matrix3x4_t parentTransform;
		AngleMatrix( GetAbsAngles(), GetAbsOrigin(), parentTransform );
		// FIXME/CHECK:  This might be too expensive to do every frame???
		SaveRagdollInfo( hdr->numbones(), parentTransform, m_BoneAccessor );
	}
	
	SetMoveType( savedMovetype );

	// Now set the dieragdoll sequence to get transforms for all
	// non-simulated bones
	m_nRestoreSequence = GetSequence();
    SetSequence( SelectWeightedSequence( ACT_DIERAGDOLL ) );
	m_nPrevSequence = GetSequence();
	m_flPlaybackRate = 0;
	UpdatePartitionListEntry();

	NoteRagdollCreationTick( this );

	UpdateVisibility();

#if defined( REPLAY_ENABLED )
	// If Replay is enabled on server, add an entry to the ragdoll recorder for this entity
	ConVar* pReplayEnable = (ConVar*)cvar->FindVar( "replay_enable" );
	if ( m_pRagdoll && pReplayEnable && pReplayEnable->GetInt() && !engine->IsPlayingDemo() && !engine->IsPlayingTimeDemo() )
	{
		CReplayRagdollRecorder& RagdollRecorder = CReplayRagdollRecorder::Instance();
		int nStartTick = TIME_TO_TICKS( engine->GetLastTimeStamp() );
		RagdollRecorder.AddEntry( this, nStartTick, m_pRagdoll->RagdollBoneCount() );
	}
#endif

	return true;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::OnDataChanged( DataUpdateType_t updateType )
{
	// don't let server change sequences after becoming a ragdoll
	if ( m_pRagdoll && GetSequence() != m_nPrevSequence )
	{
		SetSequence( m_nPrevSequence );
		m_flPlaybackRate = 0;
	}

	if ( !m_pRagdoll && m_nRestoreSequence != -1 )
	{
		SetSequence( m_nRestoreSequence );
		m_nRestoreSequence = -1;
	}

	if (updateType == DATA_UPDATE_CREATED)
	{
		m_nPrevSequence = -1;
		m_nRestoreSequence = -1;
	}



	bool modelchanged = false;

	// UNDONE: The base class does this as well.  So this is kind of ugly
	// but getting a model by index is pretty cheap...
	const model_t *pModel = modelinfo->GetModel( GetModelIndex() );
	
	if ( pModel != GetModel() )
	{
		modelchanged = true;
	}

	BaseClass::OnDataChanged( updateType );

	if ( (updateType == DATA_UPDATE_CREATED) || modelchanged )
	{
		ResetLatched();
		// if you have this pose parameter, activate HL1-style lipsync/wave envelope tracking
		if ( LookupPoseParameter( LIPSYNC_POSEPARAM_NAME ) != -1 )
		{
			MouthInfo().ActivateEnvelope();
		}
	}

	// If there's a significant change, make sure the shadow updates
	if ( modelchanged || (GetSequence() != m_nPrevSequence))
	{
		InvalidatePhysicsRecursive( ANIMATION_CHANGED ); 
		m_nPrevSequence = GetSequence();
	}

	// Only need to think if animating client side
	if ( m_bClientSideAnimation )
	{
		// Check to see if we should reset our frame
		if ( m_bClientSideFrameReset != m_bLastClientSideFrameReset )
		{
			ResetClientsideFrame();
		}
	}
	// build a ragdoll if necessary
	if ( m_nRenderFX == kRenderFxRagdoll && !m_builtRagdoll )
	{
		BecomeRagdollOnClient();
	}

	//HACKHACK!!!
	if ( m_nRenderFX == kRenderFxRagdoll && m_builtRagdoll == true )
	{
		if ( m_pRagdoll == NULL )
			 AddEffects( EF_NODRAW );
	}

	if ( m_pRagdoll && m_nRenderFX != kRenderFxRagdoll )
	{
		ClearRagdoll();
	}

	// If ragdolling and get EF_NOINTERP, we probably were dead and are now respawning,
	//  don't do blend out of ragdoll at respawn spot.
	if ( IsNoInterpolationFrame() && 
		m_pRagdollInfo &&
		m_pRagdollInfo->m_bActive )
	{
		Msg( "delete ragdoll due to nointerp\n" );
		// Remove ragdoll info
		delete m_pRagdollInfo;
		m_pRagdollInfo = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::AddEntity( void )
{
	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsNoInterpolationFrame() )
	{
		ResetLatched();
	}

	BaseClass::AddEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the attachment point with the specified name
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupAttachment( const char *pAttachmentName )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	// NOTE: Currently, the network uses 0 to mean "no attachment" 
	// thus the client must add one to the index of the attachment
	// UNDONE: Make the server do this too to be consistent.
	return Studio_FindAttachment( hdr, pAttachmentName ) + 1;
}

//-----------------------------------------------------------------------------
// Purpose: Get a random index of an attachment point with the specified substring in its name
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupRandomAttachment( const char *pAttachmentNameSubstring )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	// NOTE: Currently, the network uses 0 to mean "no attachment" 
	// thus the client must add one to the index of the attachment
	// UNDONE: Make the server do this too to be consistent.
	return Studio_FindRandomAttachment( hdr, pAttachmentNameSubstring ) + 1;
}


void C_BaseAnimating::ClientSideAnimationChanged()
{
	if ( !m_bClientSideAnimation || m_ClientSideAnimationListHandle == INVALID_CLIENTSIDEANIMATION_LIST_HANDLE )
		return;

	MDLCACHE_CRITICAL_SECTION();
	
	clientanimating_t &anim = g_ClientSideAnimationList.Element(m_ClientSideAnimationListHandle);
	Assert(anim.pAnimating == this);
	anim.flags = ComputeClientSideAnimationFlags();

	m_SequenceTransitioner.CheckForSequenceChange( 
		GetModelPtr(),
		GetSequence(),
		m_nNewSequenceParity != m_nPrevNewSequenceParity,
		!IsNoInterpolationFrame()
		);
}

unsigned int C_BaseAnimating::ComputeClientSideAnimationFlags()
{
	return FCLIENTANIM_SEQUENCE_CYCLE;
}

void C_BaseAnimating::UpdateClientSideAnimation()
{
	// Update client side animation
	if ( m_bClientSideAnimation )
	{
		Assert( m_ClientSideAnimationListHandle != INVALID_CLIENTSIDEANIMATION_LIST_HANDLE );
		if ( GetSequence() != -1 )
		{
			// latch old values
			OnLatchInterpolatedVariables( LATCH_ANIMATION_VAR );
			// move frame forward
			FrameAdvance( 0.0f ); // 0 means to use the time we last advanced instead of a constant
		}
	}
	else
	{
		Assert( m_ClientSideAnimationListHandle == INVALID_CLIENTSIDEANIMATION_LIST_HANDLE );
	}
}


void C_BaseAnimating::Simulate()
{
	if ( m_bInitModelEffects )
	{
		DelayedInitModelEffects();
	}

	if ( gpGlobals->frametime != 0.0f  )
	{
		DoAnimationEvents( GetModelPtr() );
	}
	BaseClass::Simulate();
	if ( IsNoInterpolationFrame() )
	{
		ResetLatched();
	}
	if ( GetSequence() != -1 && m_pRagdoll && ( m_nRenderFX != kRenderFxRagdoll ) )
	{
		ClearRagdoll();
	}
}


bool C_BaseAnimating::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	if ( ray.m_IsRay && IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ))
	{
		if (!TestHitboxes( ray, fContentsMask, tr ))
			return true;

		return tr.DidHit();
	}

	if ( !ray.m_IsRay && IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ))
	{
		if (!TestHitboxes( ray, fContentsMask, tr ))
			return true;

		return true;
	}

	// We shouldn't get here.
	Assert(0);
	return false;
}


// UNDONE: This almost works.  The client entities have no control over their solid box
// Also they have no ability to expose FSOLID_ flags to the engine to force the accurate
// collision tests.
// Add those and the client hitboxes will be robust
bool C_BaseAnimating::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	VPROF( "C_BaseAnimating::TestHitboxes" );

	MDLCACHE_CRITICAL_SECTION();

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	// Use vcollide for box traces.
	if ( !ray.m_IsRay )
		return false;

	// This *has* to be true for the existing code to function correctly.
	Assert( ray.m_StartOffset == vec3_origin );

	CBoneCache *pCache = GetBoneCache( pStudioHdr );
	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	if ( TraceToStudio( physprops, ray, pStudioHdr, set, hitboxbones, fContentsMask, GetRenderOrigin(), GetModelScale(), tr ) )
	{
		mstudiobbox_t *pbox = set->pHitbox( tr.hitbox );
		mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
		if ( IsRagdoll() )
		{
			IPhysicsObject *pReplace = m_pRagdoll->GetElement( tr.physicsbone );
			if ( pReplace )
			{
				VPhysicsSetObject( NULL );
				VPhysicsSetObject( pReplace );
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Check sequence framerate
// Input  : iSequence - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
		return 0.0f;

	return Studio_CPS( pStudioHdr, pStudioHdr->pSeqdesc(iSequence), iSequence, m_flPoseParameter );
}

float C_BaseAnimating::GetAnimTimeInterval( void ) const
{
#define MAX_ANIMTIME_INTERVAL 0.2f

	float flInterval = MIN( gpGlobals->curtime - m_flAnimTime, MAX_ANIMTIME_INTERVAL );
	return flInterval;
}


//-----------------------------------------------------------------------------
// Sets the cycle, marks the entity as being dirty
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetCycle( float flCycle )
{
	if ( m_flCycle != flCycle )
	{
		m_flCycle = flCycle;
		InvalidatePhysicsRecursive( ANIMATION_CHANGED );
	}
}

//-----------------------------------------------------------------------------
// Sets the sequence, marks the entity as being dirty
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetSequence( int nSequence )
{ 
	if ( m_nSequence != nSequence )
	{
		/*
		CStudioHdr *hdr = GetModelPtr();
		// Assert( hdr );
		if ( hdr )
		{
			Assert( nSequence >= 0 && nSequence < hdr->GetNumSeq() );
		}
		*/

		m_nSequence = nSequence; 
		InvalidatePhysicsRecursive( ANIMATION_CHANGED );
		if ( m_bClientSideAnimation )
		{
			ClientSideAnimationChanged();
		}
	}
}


//=========================================================
// StudioFrameAdvance - advance the animation frame up some interval (default 0.1) into the future
//=========================================================
void C_BaseAnimating::StudioFrameAdvance()
{
	if ( m_bClientSideAnimation )
		return;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

#ifdef DEBUG
	bool watch = dbganimmodel.GetString()[0] && V_stristr( hdr->pszName(), dbganimmodel.GetString() );
#else
	bool watch = false; // Q_strstr( hdr->name, "rifle" ) ? true : false;
#endif

	//if (!anim.prevanimtime)
	//{
		//anim.prevanimtime = m_flAnimTime = gpGlobals->curtime;
	//}

	// How long since last animtime
	float flInterval = GetAnimTimeInterval();

	if (flInterval <= 0.001)
	{
		// Msg("%s : %s : %5.3f (skip)\n", STRING(pev->classname), GetSequenceName( GetSequence() ), GetCycle() );
		return;
	}

	UpdateModelScale();

	//anim.prevanimtime = m_flAnimTime;
	float cycleAdvance = flInterval * GetSequenceCycleRate( hdr, GetSequence() ) * m_flPlaybackRate;
	float flNewCycle = GetCycle() + cycleAdvance;
	m_flAnimTime = gpGlobals->curtime;

	if ( watch )
	{
		Msg("%s %6.3f : %6.3f (%.3f)\n", GetClassname(), gpGlobals->curtime, m_flAnimTime, flInterval );
	}

	if ( flNewCycle < 0.0f || flNewCycle >= 1.0f ) 
	{
		if ( IsSequenceLooping( hdr, GetSequence() ) )
		{
			 flNewCycle -= (int)(flNewCycle);
		}
		else
		{
		 	 flNewCycle = (flNewCycle < 0.0f) ? 0.0f : 1.0f;
		}
		
		m_bSequenceFinished = true;	// just in case it wasn't caught in GetEvents
	}

	SetCycle( flNewCycle );

	m_flGroundSpeed = GetSequenceGroundSpeed( hdr, GetSequence() ) * GetModelScale();

#if 0
	// I didn't have a test case for this, but it seems like the right thing to do.  Check multi-player!

	// Msg("%s : %s : %5.1f\n", GetClassname(), GetSequenceName( GetSequence() ), GetCycle() );
	InvalidatePhysicsRecursive( ANIMATION_CHANGED );
#endif

	if ( watch )
	{
		Msg("%s : %s : %5.1f\n", GetClassname(), GetSequenceName( GetSequence() ), GetCycle() );
	}
}

float C_BaseAnimating::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if (t > 0)
	{
		return GetSequenceMoveDist( pStudioHdr, iSequence ) / t;
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::GetSequenceMoveDist( CStudioHdr *pStudioHdr, int iSequence )
{
	Vector				vecReturn;
	
	::GetSequenceLinearMotion( pStudioHdr, iSequence, m_flPoseParameter, &vecReturn );

	return vecReturn.Length();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//			*pVec - 
//	
//-----------------------------------------------------------------------------
void C_BaseAnimating::GetSequenceLinearMotion( int iSequence, Vector *pVec )
{
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, m_flPoseParameter, pVec );
}

void C_BaseAnimating::GetBlendedLinearVelocity( Vector *pVec )
{
	Vector vecDist;
	float flDuration;

	GetSequenceLinearMotion( GetSequence(), &vecDist );
	flDuration = SequenceDuration( GetSequence() );

	VectorScale( vecDist, 1.0 / flDuration, *pVec );

	Vector tmp;
	for (int i = m_SequenceTransitioner.m_animationQueue.Count() - 2; i >= 0; i--)
	{
		C_AnimationLayer *blend = &m_SequenceTransitioner.m_animationQueue[i];
	
		GetSequenceLinearMotion( blend->m_nSequence, &vecDist );
		flDuration = SequenceDuration( blend->m_nSequence );

		VectorScale( vecDist, 1.0 / flDuration, tmp );

		float flWeight = blend->GetFadeout( gpGlobals->curtime );
		*pVec = Lerp( flWeight, *pVec, tmp );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseAnimating::FrameAdvance( float flInterval )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return 0.0f;

#ifdef DEBUG
	bool bWatch = dbganimmodel.GetString()[0] && V_stristr( hdr->pszName(), dbganimmodel.GetString() );
#else
	bool bWatch = false; // Q_strstr( hdr->name, "medkit_large" ) ? true : false;
#endif

	float curtime = gpGlobals->curtime;

	if (flInterval == 0.0f)
	{
		flInterval = ( curtime - m_flAnimTime );
		if (flInterval <= 0.001f)
		{
			return 0.0f;
		}
	}

	if ( !m_flAnimTime )
	{
		flInterval = 0.0f;
	}

	float cyclerate = GetSequenceCycleRate( hdr, GetSequence() );
	float addcycle = flInterval * cyclerate * m_flPlaybackRate;

	if( GetServerIntendedCycle() != -1.0f )
	{
		// The server would like us to ease in a correction so that we will animate the same on the client and server.
		// So we will actually advance the average of what we would have done and what the server wants.
		float serverCycle = GetServerIntendedCycle();
		float serverAdvance = serverCycle - GetCycle();
		bool adjustOkay = serverAdvance > 0.0f;// only want to go forward. backing up looks really jarring, even when slight
		if( serverAdvance < -0.8f )
		{
			// Oh wait, it was just a wraparound from .9 to .1.
			serverAdvance += 1;
			adjustOkay = true;
		}

		if( adjustOkay )
		{
			float originalAdvance = addcycle;
			addcycle = (serverAdvance + addcycle) / 2;

			const float MAX_CYCLE_ADJUSTMENT = 0.1f;
			addcycle = MIN( MAX_CYCLE_ADJUSTMENT, addcycle );// Don't do too big of a jump; it's too jarring as well.

			DevMsg( 2, "(%d): Cycle latch used to correct %.2f in to %.2f instead of %.2f.\n",
				entindex(), GetCycle(), GetCycle() + addcycle, GetCycle() + originalAdvance );
		}

		SetServerIntendedCycle(-1.0f); // Only use a correction once, it isn't valid any time but right now.
	}

	float flNewCycle = GetCycle() + addcycle;
	m_flAnimTime = curtime;

	if ( bWatch )
	{
		Msg("%i CLIENT Time: %6.3f : (Interval %f) : cycle %f rate %f add %f\n", 
			gpGlobals->tickcount, gpGlobals->curtime, flInterval, flNewCycle, cyclerate, addcycle );
	}

	if ( (flNewCycle < 0.0f) || (flNewCycle >= 1.0f) ) 
	{
		if ( IsSequenceLooping( hdr, GetSequence() ) )
		{
			flNewCycle -= (int)(flNewCycle);
		}
		else
		{
			flNewCycle = (flNewCycle < 0.0f) ? 0.0f : 1.0f;
		}
		m_bSequenceFinished = true;
	}

	SetCycle( flNewCycle );

	return flInterval;
}

// Stubs for weapon prediction
void C_BaseAnimating::ResetSequenceInfo( void )
{
	if ( GetSequence() == -1 )
	{
		// This shouldn't happen.  Setting m_nSequence blindly is a horrible coding practice.
		SetSequence( 0 );
	}

	if ( IsDynamicModelLoading() )
	{
		m_bResetSequenceInfoOnLoad = true;
		return;
	}

	CStudioHdr *pStudioHdr = GetModelPtr();
	m_flGroundSpeed = GetSequenceGroundSpeed( pStudioHdr, GetSequence() ) * GetModelScale();
	m_bSequenceLoops = ( ( GetSequenceFlags( pStudioHdr, GetSequence() ) & STUDIO_LOOPING ) != 0 );
	// m_flAnimTime = gpGlobals->time;
	m_flPlaybackRate = 1.0;
	m_bSequenceFinished = false;
	m_flLastEventCheck = 0;

	m_nNewSequenceParity = ( m_nNewSequenceParity + 1 ) & EF_PARITY_MASK;
	m_nResetEventsParity = ( m_nResetEventsParity + 1 ) & EF_PARITY_MASK;

	// FIXME: why is this called here?  Nothing should have changed to make this nessesary
	if ( pStudioHdr )
	{
		SetEventIndexForSequence( pStudioHdr->pSeqdesc( GetSequence() ) );
	}
}

//=========================================================
//=========================================================

bool C_BaseAnimating::IsSequenceLooping( CStudioHdr *pStudioHdr, int iSequence )
{
	return (::GetSequenceFlags( pStudioHdr, iSequence ) & STUDIO_LOOPING) != 0;
}

float C_BaseAnimating::SequenceDuration( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
	{
		return 0.1f;
	}

	if (iSequence >= pStudioHdr->GetNumSeq() || iSequence < 0 )
	{
		DevWarning( 2, "C_BaseAnimating::SequenceDuration( %d ) out of range\n", iSequence );
		return 0.1;
	}

	return Studio_Duration( pStudioHdr, iSequence, m_flPoseParameter );

}

int C_BaseAnimating::FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return -1;
	}

	if (piDir == NULL)
	{
		int iDir = 1;
		int sequence = ::FindTransitionSequence( hdr, iCurrentSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransitionSequence( hdr, iCurrentSequence, iGoalSequence, piDir );

}

void C_BaseAnimating::SetBodygroup( int iGroup, int iValue )
{
	// SetBodygroup is not supported on pending dynamic models. Wait for it to load!
	// XXX TODO we could buffer up the group and value if we really needed to. -henryg
	Assert( GetModelPtr() );
	::SetBodygroup( GetModelPtr( ), m_nBody, iGroup, iValue );
}

int C_BaseAnimating::GetBodygroup( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetBodygroup( GetModelPtr( ), m_nBody, iGroup );
}

const char *C_BaseAnimating::GetBodygroupName( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? "" : ::GetBodygroupName( GetModelPtr( ), iGroup );
}

int C_BaseAnimating::FindBodygroupByName( const char *name )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? -1 : ::FindBodygroupByName( GetModelPtr( ), name );
}

int C_BaseAnimating::GetBodygroupCount( int iGroup )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetBodygroupCount( GetModelPtr( ), iGroup );
}

int C_BaseAnimating::GetNumBodyGroups( void )
{
	Assert( IsDynamicModelLoading() || GetModelPtr() );
	return IsDynamicModelLoading() ? 0 : ::GetNumBodyGroups( GetModelPtr( ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : setnum - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetHitboxSet( int setnum )
{
	if ( IsDynamicModelLoading() )
		return;

#ifdef _DEBUG
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	if (setnum > pStudioHdr->numhitboxsets())
	{
		// Warn if an bogus hitbox set is being used....
		static bool s_bWarned = false;
		if (!s_bWarned)
		{
			Warning("Using bogus hitbox set in entity %s!\n", GetClassname() );
			s_bWarned = true;
		}
		setnum = 0;
	}
#endif

	m_nHitboxSet = setnum;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *setname - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetHitboxSetByName( const char *setname )
{
	if ( IsDynamicModelLoading() )
		return;

	m_nHitboxSet = FindHitboxSetByName( GetModelPtr(), setname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::GetHitboxSet( void )
{
	return m_nHitboxSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetHitboxSetName( void )
{
	if ( IsDynamicModelLoading() )
		return "";

	return ::GetHitboxSetName( GetModelPtr(), m_nHitboxSet );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::GetHitboxSetCount( void )
{
	if ( IsDynamicModelLoading() )
		return 0;

	return ::GetHitboxSetCount( GetModelPtr() );
}

static Vector	hullcolor[8] = 
{
	Vector( 1.0, 1.0, 1.0 ),
	Vector( 1.0, 0.5, 0.5 ),
	Vector( 0.5, 1.0, 0.5 ),
	Vector( 1.0, 1.0, 0.5 ),
	Vector( 0.5, 0.5, 1.0 ),
	Vector( 1.0, 0.5, 1.0 ),
	Vector( 0.5, 1.0, 1.0 ),
	Vector( 1.0, 1.0, 1.0 )
};

//-----------------------------------------------------------------------------
// Purpose: Draw the current hitboxes
//-----------------------------------------------------------------------------
void C_BaseAnimating::DrawClientHitboxes( float duration /*= 0.0f*/, bool monocolor /*= false*/  )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	mstudiohitboxset_t *set =pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set )
		return;

	Vector position;
	QAngle angles;

	int r = 255;
	int g = 0;
	int b = 0;

	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox( i );

		GetBonePosition( pbox->bone, position, angles );

		if ( !monocolor )
		{
			int j = (pbox->group % 8);
			r = ( int ) ( 255.0f * hullcolor[j][0] );
			g = ( int ) ( 255.0f * hullcolor[j][1] );
			b = ( int ) ( 255.0f * hullcolor[j][2] );
		}

		debugoverlay->AddBoxOverlay( position, pbox->bbmin, pbox->bbmax, angles, r, g, b, 0 ,duration );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : int C_BaseAnimating::SelectWeightedSequence
//-----------------------------------------------------------------------------
int C_BaseAnimating::SelectWeightedSequence ( int activity )
{
	Assert( activity != ACT_INVALID );

	return ::SelectWeightedSequence( GetModelPtr(), activity );

}

int C_BaseAnimating::SelectWeightedSequenceFromModifiers( Activity activity, CUtlSymbol *pActivityModifiers, int iModifierCount )
{
	Assert( activity != ACT_INVALID );
	Assert( GetModelPtr() );
	return GetModelPtr()->SelectWeightedSequenceFromModifiers( activity, pActivityModifiers, iModifierCount );
}

//=========================================================
//=========================================================
int C_BaseAnimating::LookupPoseParameter( CStudioHdr *pstudiohdr, const char *szName )
{
	if ( !pstudiohdr )
		return 0;

	for (int i = 0; i < pstudiohdr->GetNumPoseParameters(); i++)
	{
		if (stricmp( pstudiohdr->pPoseParameter( i ).pszName(), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "poseparameter %s couldn't be mapped!!!\n", szName ) );
	return -1; // Error
}

//=========================================================
//=========================================================
float C_BaseAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue )
{
	return SetPoseParameter( pStudioHdr, LookupPoseParameter( pStudioHdr, szName ), flValue );
}

float C_BaseAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue )
{
	if ( !pStudioHdr )
	{
		Assert(!"C_BaseAnimating::SetPoseParameter: model missing");
		return flValue;
	}

	if (iParameter >= 0)
	{
		float flNewValue;
		flValue = Studio_SetPoseParameter( pStudioHdr, iParameter, flValue, flNewValue );
		m_flPoseParameter[ iParameter ] = flNewValue;
	}

	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *label - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupSequence( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupSequence( GetModelPtr(), label );
}

void C_BaseAnimating::Release()
{
	ClearRagdoll();
	BaseClass::Release();
}

void C_BaseAnimating::Clear( void )
{
	InvalidateMdlCache();
	Q_memset(&m_mouth, 0, sizeof(m_mouth));
	m_flCycle = 0;
	m_flOldCycle = 0;
	m_bResetSequenceInfoOnLoad = false;
	m_bDynamicModelPending = false;
	m_AutoRefModelIndex.Clear();
	BaseClass::Clear();	
}

//-----------------------------------------------------------------------------
// Purpose: Clear current ragdoll
//-----------------------------------------------------------------------------
void C_BaseAnimating::ClearRagdoll()
{
	if ( m_pRagdoll )
	{
		// immediately mark the member ragdoll as being NULL,
		// so that we have no reentrancy problems with the delete
		// (such as the disappearance of the ragdoll physics waking up
		// IVP which causes other objects to move and have a touch 
		// callback on the ragdoll entity, which was a crash on TF)
		// That is to say: it is vital that the member be cleared out
		// BEFORE the delete occurs.
		CRagdoll * RESTRICT pDoomed = m_pRagdoll;
		m_pRagdoll = NULL;

		delete pDoomed;

		// Set to null so that the destructor's call to DestroyObject won't destroy
		//  m_pObjects[ 0 ] twice since that's the physics object for the prop
		VPhysicsSetObject( NULL );

		// If we have ragdoll mins/maxs, we've just come out of ragdoll, so restore them
		if ( m_vecPreRagdollMins != vec3_origin || m_vecPreRagdollMaxs != vec3_origin )
		{
			SetCollisionBounds( m_vecPreRagdollMins, m_vecPreRagdollMaxs );
		}

#if defined( REPLAY_ENABLED )
		// Delete entry from ragdoll recorder if Replay is enabled on server
		ConVar* pReplayEnable = (ConVar*)cvar->FindVar( "replay_enable" );
		if ( pReplayEnable && pReplayEnable->GetInt() && !engine->IsPlayingDemo() && !engine->IsPlayingTimeDemo() )
		{
			CReplayRagdollRecorder& RagdollRecorder = CReplayRagdollRecorder::Instance();
			RagdollRecorder.StopRecordingRagdoll( this );
		}
#endif
	}
	m_builtRagdoll = false;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity, ie "ACT_IDLE".
// Output : Returns the activity ID or ACT_INVALID.
//-----------------------------------------------------------------------------
int C_BaseAnimating::LookupActivity( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupActivity( GetModelPtr(), label );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetSequenceActivityName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceActivityName( GetModelPtr(), iSequence );
}

//=========================================================
//=========================================================
float C_BaseAnimating::SetBoneController ( int iController, float flValue )
{
	Assert( GetModelPtr() );

	CStudioHdr *pmodel = GetModelPtr();

	Assert(iController >= 0 && iController < NUM_BONECTRLS);

	float controller = m_flEncodedController[iController];
	float retVal = Studio_SetController( pmodel, iController, flValue, controller );
	m_flEncodedController[iController] = controller;
	return retVal;
}


void C_BaseAnimating::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	CBaseEntity *pMoveParent;
	if ( IsEffectActive( EF_BONEMERGE ) && IsEffectActive( EF_BONEMERGE_FASTCULL ) && (pMoveParent = GetMoveParent()) != NULL )
	{
		// Doing this saves a lot of CPU.
		*pAbsOrigin = pMoveParent->WorldSpaceCenter();
		*pAbsAngles = pMoveParent->GetRenderAngles();
	}
	else
	{
		if ( !m_pBoneMergeCache || !m_pBoneMergeCache->GetAimEntOrigin( pAbsOrigin, pAbsAngles ) )
			BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *C_BaseAnimating::GetSequenceName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceName( GetModelPtr(), iSequence );
}

Activity C_BaseAnimating::GetSequenceActivity( int iSequence )
{
	if( iSequence == -1 )
	{
		return ACT_INVALID;
	}

	if ( !GetModelPtr() )
		return ACT_INVALID;

	return (Activity)::GetSequenceActivity( GetModelPtr(), iSequence );
}



//-----------------------------------------------------------------------------
// returns the sequence keyvalue text as a KeyValues pointer
//-----------------------------------------------------------------------------
KeyValues *C_BaseAnimating::GetSequenceKeyValues( int iSequence )
{
	const char *szText = Studio_GetKeyValueText( GetModelPtr(), iSequence );

	if (szText)
	{
		KeyValues *seqKeyValues = new KeyValues("");
		if ( seqKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), szText ) )
		{
			return seqKeyValues;
		}
		seqKeyValues->deleteThis();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Computes a box that surrounds all hitboxes
//-----------------------------------------------------------------------------
bool C_BaseAnimating::ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Note that this currently should not be called during position recomputation because of IK.
	// The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the computeabsposition phase.

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache( pStudioHdr );
	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init( FLT_MAX, FLT_MAX, FLT_MAX );
	pVecWorldMaxs->Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		TransformAABB( *hitboxbones[pbox->bone], pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );
		VectorMin( *pVecWorldMins, vecBoxAbsMins, *pVecWorldMins );
		VectorMax( *pVecWorldMaxs, vecBoxAbsMaxs, *pVecWorldMaxs );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Computes a box that surrounds all hitboxes, in entity space
//-----------------------------------------------------------------------------
bool C_BaseAnimating::ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Note that this currently should not be called during position recomputation because of IK.
	// The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the computeabsposition phase.

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache( pStudioHdr );
	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init( FLT_MAX, FLT_MAX, FLT_MAX );
	pVecWorldMaxs->Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	matrix3x4_t worldToEntity, boneToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		ConcatTransforms( worldToEntity, *hitboxbones[pbox->bone], boneToEntity );
		TransformAABB( boneToEntity, pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );
		VectorMin( *pVecWorldMins, vecBoxAbsMins, *pVecWorldMins );
		VectorMax( *pVecWorldMaxs, vecBoxAbsMaxs, *pVecWorldMaxs );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scale - 
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetModelScale( float scale, float change_duration /*= 0.0f*/  )
{
	if ( change_duration > 0.0f )
	{
		ModelScale *mvs = ( ModelScale * )CreateDataObject( MODELSCALE );
		mvs->m_flModelScaleStart = m_flModelScale;
		mvs->m_flModelScaleGoal = scale;
		mvs->m_flModelScaleStartTime = gpGlobals->curtime;
		mvs->m_flModelScaleFinishTime = mvs->m_flModelScaleStartTime + change_duration;
	}
	else
	{
		m_flModelScale = scale;
		RefreshCollisionBounds();
		
		if ( HasDataObjectType( MODELSCALE ) )
		{
			DestroyDataObject( MODELSCALE );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::UpdateModelScale()
{
	ModelScale *mvs = ( ModelScale * )GetDataObject( MODELSCALE );
	if ( !mvs )
	{
		return;
	}

	float dt = mvs->m_flModelScaleFinishTime - mvs->m_flModelScaleStartTime;
	Assert( dt > 0.0f );

	float frac = ( gpGlobals->curtime - mvs->m_flModelScaleStartTime ) / dt;
	frac = clamp( frac, 0.0f, 1.0f );

	if ( gpGlobals->curtime >= mvs->m_flModelScaleFinishTime )
	{
		m_flModelScale = mvs->m_flModelScaleGoal;
		DestroyDataObject( MODELSCALE );
	}
	else
	{
		m_flModelScale = Lerp( frac, mvs->m_flModelScaleStart, mvs->m_flModelScaleGoal );
	}

	RefreshCollisionBounds();
}

void C_BaseAnimating::RefreshCollisionBounds( void )
{
	CollisionProp()->RefreshScaledCollisionBounds();
}

//-----------------------------------------------------------------------------
// Purpose: Clientside bone follower class. Used just to visualize them.
//			Bone followers WON'T be sent to the client if VISUALIZE_FOLLOWERS is
//			undefined in the server's physics_bone_followers.cpp
//-----------------------------------------------------------------------------
class C_BoneFollower : public C_BaseEntity
{
	DECLARE_CLASS( C_BoneFollower, C_BaseEntity );
	DECLARE_CLIENTCLASS();
public:
	C_BoneFollower( void )
	{
	}

	bool	ShouldDraw( void );
	int		DrawModel( int flags );
	bool	TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

private:
	int m_modelIndex;
	int m_solidIndex;
};

IMPLEMENT_CLIENTCLASS_DT( C_BoneFollower, DT_BoneFollower, CBoneFollower )
	RecvPropInt( RECVINFO( m_modelIndex ) ),
	RecvPropInt( RECVINFO( m_solidIndex ) ),
END_RECV_TABLE()

void VCollideWireframe_ChangeCallback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	for ( C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity) )
	{
		pEntity->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether object should render.
//-----------------------------------------------------------------------------
bool C_BoneFollower::ShouldDraw( void )
{
	return ( vcollide_wireframe.GetBool() );  //MOTODO
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BoneFollower::DrawModel( int flags )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( m_modelIndex );
	if ( pCollide )
	{
		static color32 debugColor = {0,255,255,0};
		matrix3x4_t matrix;
		AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
		engine->DebugDrawPhysCollide( pCollide->solids[m_solidIndex], NULL, matrix, debugColor );
	}
	return 1;
}

bool C_BoneFollower::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( m_modelIndex );
	Assert( pCollide && pCollide->solidCount > m_solidIndex );

	physcollision->TraceBox( ray, pCollide->solids[m_solidIndex], GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.fraction >= 1 )
		return false;

	// return owner as trace hit
	trace.m_pEnt = GetOwnerEntity();
	trace.hitgroup = 0;//m_hitGroup;
	trace.physicsbone = 0;//m_physicsBone; // UNDONE: Get physics bone index & hitgroup
	return trace.DidHit();
}


void C_BaseAnimating::DisableMuzzleFlash()
{
	m_nOldMuzzleFlashParity = m_nMuzzleFlashParity;
}


void C_BaseAnimating::DoMuzzleFlash()
{
	m_nMuzzleFlashParity = (m_nMuzzleFlashParity+1) & ((1 << EF_MUZZLEFLASH_BITS) - 1);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DevMsgRT( char const* pMsg, ... )
{
	if (gpGlobals->frametime != 0.0f)
	{
		va_list argptr;
		va_start( argptr, pMsg );
		// 
		{
			static char	string[1024];
			Q_vsnprintf (string, sizeof( string ), pMsg, argptr);
			DevMsg( 1, "%s", string );
		}
		// DevMsg( pMsg, argptr );
		va_end( argptr );
	}
}


void C_BaseAnimating::ForceClientSideAnimationOn()
{
	m_bClientSideAnimation = true;
	AddToClientSideAnimationList();
}


void C_BaseAnimating::AddToClientSideAnimationList()
{
	// Already in list
	if ( m_ClientSideAnimationListHandle != INVALID_CLIENTSIDEANIMATION_LIST_HANDLE )
		return;

	clientanimating_t list( this, 0 );
	m_ClientSideAnimationListHandle = g_ClientSideAnimationList.AddToTail( list );
	ClientSideAnimationChanged();

	UpdateRelevantInterpolatedVars();
}

void C_BaseAnimating::RemoveFromClientSideAnimationList( bool bBeingDestroyed /*= false*/ )
{
	// Not in list yet
	if ( INVALID_CLIENTSIDEANIMATION_LIST_HANDLE == m_ClientSideAnimationListHandle )
		return;

	unsigned int c = g_ClientSideAnimationList.Count();

	Assert( m_ClientSideAnimationListHandle < c );

	unsigned int last = c - 1;

	if ( last == m_ClientSideAnimationListHandle )
	{
		// Just wipe the final entry
		g_ClientSideAnimationList.FastRemove( last );
	}
	else
	{
		clientanimating_t lastEntry = g_ClientSideAnimationList[ last ];
		// Remove the last entry
		g_ClientSideAnimationList.FastRemove( last );

		// And update it's handle to point to this slot.
		lastEntry.pAnimating->m_ClientSideAnimationListHandle = m_ClientSideAnimationListHandle;
		g_ClientSideAnimationList[ m_ClientSideAnimationListHandle ] = lastEntry;
	}

	// Invalidate our handle no matter what.
	m_ClientSideAnimationListHandle = INVALID_CLIENTSIDEANIMATION_LIST_HANDLE;

	if ( !bBeingDestroyed )
	{
		UpdateRelevantInterpolatedVars();
	}
}


// static method
void C_BaseAnimating::UpdateClientSideAnimations()
{
	VPROF_BUDGET( "UpdateClientSideAnimations", VPROF_BUDGETGROUP_CLIENT_ANIMATION );

	int c = g_ClientSideAnimationList.Count();
	for ( int i = 0; i < c ; ++i )
	{
		clientanimating_t &anim = g_ClientSideAnimationList.Element(i);
		if ( !(anim.flags & FCLIENTANIM_SEQUENCE_CYCLE) )
			continue;
		Assert( anim.pAnimating );
		anim.pAnimating->UpdateClientSideAnimation();
	}
}

CBoneList *C_BaseAnimating::RecordBones( CStudioHdr *hdr, matrix3x4_t *pBoneState )
{
	if ( !ToolsEnabled() )
		return NULL;
		
	VPROF_BUDGET( "C_BaseAnimating::RecordBones", VPROF_BUDGETGROUP_TOOLS );

	// Possible optimization: Instead of inverting everything while recording, record the pos/q stuff into a structure instead?
	Assert( hdr );

	// Setup our transform based on render angles and origin.
	matrix3x4_t parentTransform;
	AngleMatrix( GetRenderAngles(), GetRenderOrigin(), parentTransform );

	CBoneList *boneList = CBoneList::Alloc();
	Assert( boneList );

	boneList->m_nBones = hdr->numbones();

	for ( int i = 0;  i < hdr->numbones(); i++ )
	{
		matrix3x4_t inverted;
		matrix3x4_t output;

		mstudiobone_t *bone = hdr->pBone( i );

		// Only update bones referenced during setup
		if ( !(bone->flags & BONE_USED_BY_ANYTHING ) )
		{
			boneList->m_quatRot[ i ].Init( 0.0f, 0.0f, 0.0f, 1.0f ); // Init by default sets all 0's, which is invalid
			boneList->m_vecPos[ i ].Init();
			continue;
		}

		if ( bone->parent == -1 )
		{
			// Decompose into parent space
			MatrixInvert( parentTransform, inverted );
		}
		else
		{
			MatrixInvert( pBoneState[ bone->parent ], inverted );
		}

		ConcatTransforms( inverted, pBoneState[ i ], output );

		MatrixAngles( output, 
			boneList->m_quatRot[ i ],
			boneList->m_vecPos[ i ] );
	}

	return boneList;
}

void C_BaseAnimating::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_BaseAnimating::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	// Force the animation to drive bones
	CStudioHdr *hdr = GetModelPtr();
	matrix3x4_t *pBones = (matrix3x4_t*)_alloca( ( hdr ? hdr->numbones() : 1 ) * sizeof(matrix3x4_t) );
	if ( hdr )
	{
		SetupBones( pBones, hdr->numbones(), BONE_USED_BY_ANYTHING, gpGlobals->curtime );
	}
	else
	{
		SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );
	}

	BaseClass::GetToolRecordingState( msg );

	static BaseAnimatingRecordingState_t state;
	state.m_nSkin = GetSkin();
	state.m_nBody = m_nBody;
	state.m_nSequence = m_nSequence;
	state.m_pBoneList = NULL;
	msg->SetPtr( "baseanimating", &state );
	msg->SetInt( "viewmodel", IsViewModel() ? 1 : 0 );

	if ( hdr )
	{
		state.m_pBoneList = RecordBones( hdr, pBones );
	}
}

void C_BaseAnimating::CleanupToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;
		    
	BaseAnimatingRecordingState_t *pState = (BaseAnimatingRecordingState_t*)msg->GetPtr( "baseanimating" );
	if ( pState && pState->m_pBoneList )
	{
		pState->m_pBoneList->Release();
	}

	BaseClass::CleanupToolRecordingState( msg );
}

LocalFlexController_t C_BaseAnimating::GetNumFlexControllers( void )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return LocalFlexController_t(0);

	return pstudiohdr->numflexcontrollers();
}

const char *C_BaseAnimating::GetFlexDescFacs( int iFlexDesc )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexdesc_t *pflexdesc = pstudiohdr->pFlexdesc( iFlexDesc );

	return pflexdesc->pszFACS( );
}

const char *C_BaseAnimating::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

const char *C_BaseAnimating::GetFlexControllerType( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszType( );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the fade scale of the entity in question
// Output : unsigned char - 0 - 255 alpha value
//-----------------------------------------------------------------------------
unsigned char C_BaseAnimating::GetClientSideFade( void )
{
	return UTIL_ComputeEntityFade( this, m_fadeMinDist, m_fadeMaxDist, m_flFadeScale );
}

//-----------------------------------------------------------------------------
// Purpose: Note that we've been transmitted a sequence
//-----------------------------------------------------------------------------
void C_BaseAnimating::SetReceivedSequence( void )
{
	m_bReceivedSequence = true;
}

//-----------------------------------------------------------------------------
// Purpose: See if we should force reset our sequence on a new model
//-----------------------------------------------------------------------------
bool C_BaseAnimating::ShouldResetSequenceOnNewModel( void )
{
	return ( m_bReceivedSequence == false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::UpdateBoneAttachments( void )
{
	if ( !m_pAttachedTo )
		return;

//	Assert( IsFollowingEntity() );
//	Assert( m_boneIndexAttached >= 0 );

	C_BaseAnimating *follow = FindFollowedEntity();
	if ( follow && (m_boneIndexAttached >= 0) )
	{
		matrix3x4_t boneToWorld, localSpace;
		follow->GetCachedBoneMatrix( m_boneIndexAttached, boneToWorld );
		AngleMatrix( m_boneAngles, m_bonePosition, localSpace );
		ConcatTransforms( boneToWorld, localSpace, GetBoneForWrite( 0 ) );

		Vector absOrigin;
		MatrixGetColumn( GetBone( 0 ), 3, absOrigin );
		SetAbsOrigin( absOrigin );

		QAngle absAngle;
		MatrixAngles( GetBone( 0 ), absAngle );
		SetAbsAngles( absAngle);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::AttachEntityToBone( C_BaseAnimating* attachTarget, int boneIndexAttached, Vector bonePosition, QAngle boneAngles )
{
	if ( !attachTarget )
		return;

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	FollowEntity( attachTarget );
	SetOwnerEntity( attachTarget );

//	Assert( boneIndexAttached >= 0 );		// We should be attaching to a bone.

	if ( boneIndexAttached >= 0 )
	{
		m_boneIndexAttached = boneIndexAttached;
		m_bonePosition = bonePosition;
		m_boneAngles = boneAngles;
	}

	m_BoneAccessor.SetReadableBones( BONE_USED_BY_ANYTHING );
	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );

	attachTarget->AddBoneAttachment( this );

	NotifyBoneAttached( attachTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::NotifyBoneAttached( C_BaseAnimating* attachTarget )
{
	// If we're already attached to something, remove us from it.
	if ( m_pAttachedTo )
	{
		m_pAttachedTo->RemoveBoneAttachment( this );
		m_pAttachedTo = NULL;
	}

	// Remember the new attach target.
	m_pAttachedTo = attachTarget;

	// Special case: if we just attached to the local player and he is hidden, hide us as well.
	C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer*>(attachTarget);
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		if ( !C_BasePlayer::ShouldDrawLocalPlayer() )
		{
			AddEffects( EF_NODRAW );
		}
	}
	else
	{
		RemoveEffects( EF_NODRAW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::AddBoneAttachment( C_BaseAnimating* newBoneAttachment )
{
	if ( !newBoneAttachment )
		return;

	m_BoneAttachments.AddToTail( newBoneAttachment );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::RemoveBoneAttachment( C_BaseAnimating* boneAttachment )
{
	if ( !boneAttachment )
		return;

	m_BoneAttachments.FindAndRemove( boneAttachment );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseAnimating::GetNumBoneAttachments()
{
	return m_BoneAttachments.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseAnimating* C_BaseAnimating::GetBoneAttachment( int i )
{
	if ( m_BoneAttachments.IsValidIndex(i) )
	{
		return m_BoneAttachments[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::DestroyBoneAttachments()
{
	while ( GetNumBoneAttachments() )
	{
		C_BaseAnimating *pAttachment = GetBoneAttachment(0);
		if ( pAttachment )
		{
			pAttachment->Release();
		}
		else
		{
			m_BoneAttachments.Remove(0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseAnimating::MoveBoneAttachments( C_BaseAnimating* attachTarget )
{
	if ( !attachTarget )
		return;

	// Move all of our bone attachments to this new object.
	// Preserves the specific bone and attachment location information.
	while ( GetNumBoneAttachments() )
	{
		C_BaseAnimating *pAttachment = GetBoneAttachment(0);
		if ( pAttachment )
		{
			pAttachment->AttachEntityToBone( attachTarget );
		}
		else
		{
			m_BoneAttachments.Remove(0);
		}
	}
}
