//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "mathlib/vmatrix.h"
#include "ragdoll_shared.h"
#include "bone_setup.h"
#include "materialsystem/imesh.h"
#include "engine/ivmodelinfo.h"
#include "iviewrender.h"
#include "tier0/vprof.h"
#include "view.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
extern ConVar r_FadeProps;
#endif

CRagdoll::CRagdoll()
{
	m_ragdoll.listCount = 0;
	m_vecLastOrigin.Init();
	m_flLastOriginChangeTime = - 1.0f;
	
	m_lastUpdate = -FLT_MAX;
}

#define DEFINE_RAGDOLL_ELEMENT( i ) \
	DEFINE_FIELD( m_ragdoll.list[i].originParentSpace, FIELD_VECTOR ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pObject ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pConstraint ), \
	DEFINE_FIELD( m_ragdoll.list[i].parentIndex, FIELD_INTEGER )

BEGIN_SIMPLE_DATADESC( CRagdoll )

	DEFINE_AUTO_ARRAY( m_ragdoll.boneIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_ragdoll.listCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_ragdoll.allowStretch, FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( m_ragdoll.pGroup ),

	DEFINE_RAGDOLL_ELEMENT( 0 ),
	DEFINE_RAGDOLL_ELEMENT( 1 ),
	DEFINE_RAGDOLL_ELEMENT( 2 ),
	DEFINE_RAGDOLL_ELEMENT( 3 ),
	DEFINE_RAGDOLL_ELEMENT( 4 ),
	DEFINE_RAGDOLL_ELEMENT( 5 ),
	DEFINE_RAGDOLL_ELEMENT( 6 ),
	DEFINE_RAGDOLL_ELEMENT( 7 ),
	DEFINE_RAGDOLL_ELEMENT( 8 ),
	DEFINE_RAGDOLL_ELEMENT( 9 ),
	DEFINE_RAGDOLL_ELEMENT( 10 ),
	DEFINE_RAGDOLL_ELEMENT( 11 ),
	DEFINE_RAGDOLL_ELEMENT( 12 ),
	DEFINE_RAGDOLL_ELEMENT( 13 ),
	DEFINE_RAGDOLL_ELEMENT( 14 ),
	DEFINE_RAGDOLL_ELEMENT( 15 ),
	DEFINE_RAGDOLL_ELEMENT( 16 ),
	DEFINE_RAGDOLL_ELEMENT( 17 ),
	DEFINE_RAGDOLL_ELEMENT( 18 ),
	DEFINE_RAGDOLL_ELEMENT( 19 ),
	DEFINE_RAGDOLL_ELEMENT( 20 ),
	DEFINE_RAGDOLL_ELEMENT( 21 ),
	DEFINE_RAGDOLL_ELEMENT( 22 ),
	DEFINE_RAGDOLL_ELEMENT( 23 ),

END_DATADESC()


IPhysicsObject *CRagdoll::GetElement( int elementNum )
{ 
	return m_ragdoll.list[elementNum].pObject;
}

void CRagdoll::BuildRagdollBounds( C_BaseEntity *ent )
{
	Vector mins, maxs, size;
	modelinfo->GetModelBounds( ent->GetModel(), mins, maxs );
	size = (maxs - mins) * 0.5;
	m_radius = size.Length();

	m_mins.Init(-m_radius,-m_radius,-m_radius);
	m_maxs.Init(m_radius,m_radius,m_radius);
}

void CRagdoll::Init( 
	C_BaseEntity *ent, 
	CStudioHdr *pstudiohdr, 
	const Vector &forceVector, 
	int forceBone, 
	const matrix3x4_t *pDeltaBones0, 
	const matrix3x4_t *pDeltaBones1, 
	const matrix3x4_t *pCurrentBonePosition, 
	float dt,
	bool bFixedConstraints )
{
	ragdollparams_t params;
	params.pGameData = static_cast<void *>( ent );
	params.modelIndex = ent->GetModelIndex();
	params.pCollide = modelinfo->GetVCollide( params.modelIndex );
	params.pStudioHdr = pstudiohdr;
	params.forceVector = forceVector;
	params.forceBoneIndex = forceBone;
	params.forcePosition.Init();
	params.pCurrentBones = pCurrentBonePosition;
	params.jointFrictionScale = 1.0;
	params.allowStretch = false;
	params.fixedConstraints = bFixedConstraints;
	RagdollCreate( m_ragdoll, params, physenv );
	ent->VPhysicsSetObject( NULL );
	ent->VPhysicsSetObject( m_ragdoll.list[0].pObject );
	// Mark the ragdoll as debris.
	ent->SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	RagdollApplyAnimationAsVelocity( m_ragdoll, pDeltaBones0, pDeltaBones1, dt );
	RagdollActivate( m_ragdoll, params.pCollide, ent->GetModelIndex() );

	// It's moving now...
	m_flLastOriginChangeTime = gpGlobals->curtime;

	// So traces hit it.
	ent->AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	if ( !m_ragdoll.listCount )
		return;

	BuildRagdollBounds( ent );

	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		g_pPhysSaveRestoreManager->AssociateModel( m_ragdoll.list[i].pObject, ent->GetModelIndex() );
	}

#if RAGDOLL_VISUALIZE
	memcpy( m_savedBone1, &pDeltaBones0[0], sizeof(matrix3x4_t) * pstudiohdr->numbones() );
	memcpy( m_savedBone2, &pDeltaBones1[0], sizeof(matrix3x4_t) * pstudiohdr->numbones() );
	memcpy( m_savedBone3, &pCurrentBonePosition[0], sizeof(matrix3x4_t) * pstudiohdr->numbones() );
#endif
}

CRagdoll::~CRagdoll( void )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		IPhysicsObject *pObject = m_ragdoll.list[i].pObject;
		if ( pObject )
		{
			g_pPhysSaveRestoreManager->ForgetModel( m_ragdoll.list[i].pObject );
			// Disable collision on all ragdoll parts before calling RagdollDestroy
			// (which might cause touch callbacks on the ragdoll otherwise, which is
			// very bad for a half deleted ragdoll).
			pObject->EnableCollisions( false );
		}
	}

	RagdollDestroy( m_ragdoll );
}


void CRagdoll::RagdollBone( C_BaseEntity *ent, mstudiobone_t *pbones, int boneCount, bool *boneSimulated, CBoneAccessor &pBoneToWorld )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( RagdollGetBoneMatrix( m_ragdoll, pBoneToWorld, i ) )
		{
			boneSimulated[m_ragdoll.boneIndex[i]] = true;
		}
	}
}

const Vector& CRagdoll::GetRagdollOrigin( )
{
	m_ragdoll.list[0].pObject->GetPosition( &m_origin, 0 );
	return m_origin;
}

void CRagdoll::GetRagdollBounds( Vector &theMins, Vector &theMaxs )
{
	theMins = m_mins;
	theMaxs = m_maxs;
}

void CRagdoll::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_lastUpdate == gpGlobals->curtime )
		return;
	m_lastUpdate = gpGlobals->curtime;
	m_allAsleep = RagdollIsAsleep( m_ragdoll );
	if ( m_allAsleep )
	{
		// NOTE: This is the bbox of the ragdoll's physics
		// It's not always correct to use for culling, but it sure beats 
		// using the radius box!
		Vector origin = GetRagdollOrigin();
		RagdollComputeExactBbox( m_ragdoll, origin, m_mins, m_maxs );
		m_mins -= origin;
		m_maxs -= origin;
	}
	else
	{
		m_mins.Init(-m_radius,-m_radius,-m_radius);
		m_maxs.Init(m_radius,m_radius,m_radius);

		if ( m_ragdoll.pGroup->IsInErrorState() )
		{
			C_BaseEntity *pEntity = static_cast<C_BaseEntity *>(m_ragdoll.list[0].pObject->GetGameData());
			RagdollSolveSeparation( m_ragdoll, pEntity );
		}
	}

	// See if we should go to sleep...
	CheckSettleStationaryRagdoll();
}

//=============================================================================
// HPE_BEGIN:
// [menglish] Transforms a vector from the given bone's space to world space
//=============================================================================
 
bool CRagdoll::TransformVectorToWorld(int iBoneIndex, const Vector *vPosition, Vector *vOut)
{
	int listIndex = -1;
	if( iBoneIndex >= 0 && iBoneIndex < m_ragdoll.listCount)
	{
		for ( int i = 0; i < m_ragdoll.listCount; ++i )
		{
			if(m_ragdoll.boneIndex[i] == iBoneIndex)
				listIndex = i;
		}
		if(listIndex != -1)
		{
			m_ragdoll.list[listIndex].pObject->LocalToWorld(vOut, *vPosition);
			return true;
		}
	}
	return false;
}
 
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CRagdoll::PhysForceRagdollToSleep()
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( m_ragdoll.list[i].pObject )
		{
			PhysForceClearVelocity( m_ragdoll.list[i].pObject );
			m_ragdoll.list[i].pObject->Sleep();
		}
	}
}

#define RAGDOLL_SLEEP_TOLERANCE	1.0f
static ConVar ragdoll_sleepaftertime( "ragdoll_sleepaftertime", "5.0f", 0, "After this many seconds of being basically stationary, the ragdoll will go to sleep." );

void CRagdoll::CheckSettleStationaryRagdoll()
{
	Vector delta = GetRagdollOrigin() - m_vecLastOrigin;
	m_vecLastOrigin = GetRagdollOrigin();
	for ( int i = 0; i < 3; ++i )
	{
		// It's still moving...
		if ( fabs( delta[ i ] ) > RAGDOLL_SLEEP_TOLERANCE )
		{
			m_flLastOriginChangeTime = gpGlobals->curtime;
			// Msg( "%d [%p] Still moving\n", gpGlobals->tickcount, this );
			return;
		}
	}

	// It's totally asleep, don't worry about forcing it to settle
	if ( m_allAsleep )
		return;

	// Msg( "%d [%p] Settling\n", gpGlobals->tickcount, this );

	// It has stopped moving, see if it
	float dt = gpGlobals->curtime - m_flLastOriginChangeTime;
	if ( dt < ragdoll_sleepaftertime.GetFloat() )
		return;

	// Msg( "%d [%p] FORCE SLEEP\n",gpGlobals->tickcount, this );

	// Force it to go to sleep
	PhysForceRagdollToSleep();
}

void CRagdoll::ResetRagdollSleepAfterTime( void )
{
	m_flLastOriginChangeTime = gpGlobals->curtime;
}

void CRagdoll::DrawWireframe()
{
	IMaterial *pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);

	int i;
	matrix3x4_t matrix;
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {0,255,255,0};

		// draw the actual physics positions, not the cleaned up animation position
		m_ragdoll.list[i].pObject->GetPositionMatrix( &matrix );
		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, matrix, debugColor );
	}

#if RAGDOLL_VISUALIZE
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {255,0,0,0};

		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, m_savedBone1[m_ragdoll.boneIndex[i]], debugColor );
	}
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {0,255,0,0};

		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, m_savedBone2[m_ragdoll.boneIndex[i]], debugColor );
	}

	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {0,0,255,0};

		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, m_savedBone3[m_ragdoll.boneIndex[i]], debugColor );
	}
#endif
}


CRagdoll *CreateRagdoll( 
	C_BaseEntity *ent, 
	CStudioHdr *pstudiohdr, 
	const Vector &forceVector, 
	int forceBone, 
	const matrix3x4_t *pDeltaBones0, 
	const matrix3x4_t *pDeltaBones1, 
	const matrix3x4_t *pCurrentBonePosition,
	float dt,
	bool bFixedConstraints )
{
	CRagdoll *pRagdoll = new CRagdoll;
	pRagdoll->Init( ent, pstudiohdr, forceVector, forceBone, pDeltaBones0, pDeltaBones1, pCurrentBonePosition, dt, bFixedConstraints );

	if ( !pRagdoll->IsValid() )
	{
		Msg("Bad ragdoll for %s\n", pstudiohdr->pszName() );
		delete pRagdoll;
		pRagdoll = NULL;
	}
	return pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_ServerRagdoll : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ServerRagdoll, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_ServerRagdoll( void );

	virtual void PostDataUpdate( DataUpdateType_t updateType );

	virtual int InternalDrawModel( int flags );
	virtual CStudioHdr *OnNewModel( void );
	virtual unsigned char GetClientSideFade();
	virtual void	SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );

	void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual void AddEntity( void );
	virtual void AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime );
	virtual void BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );
	IPhysicsObject *GetElement( int elementNum );
	virtual void UpdateOnRemove();
	virtual float LastBoneChangedTime();

	// Incoming from network
	Vector		m_ragPos[RAGDOLL_MAX_ELEMENTS];
	QAngle		m_ragAngles[RAGDOLL_MAX_ELEMENTS];

	CInterpolatedVarArray< Vector, RAGDOLL_MAX_ELEMENTS >	m_iv_ragPos;
	CInterpolatedVarArray< QAngle, RAGDOLL_MAX_ELEMENTS >	m_iv_ragAngles;

	int			m_elementCount;
	int			m_boneIndex[RAGDOLL_MAX_ELEMENTS];

private:
	C_ServerRagdoll( const C_ServerRagdoll &src );

	typedef CHandle<C_BaseAnimating> CBaseAnimatingHandle;
	CNetworkVar( CBaseAnimatingHandle, m_hUnragdoll );
	CNetworkVar( float, m_flBlendWeight );
	float m_flBlendWeightCurrent;
	CNetworkVar( int, m_nOverlaySequence );
	float m_flLastBoneChangeTime;
};


EXTERN_RECV_TABLE(DT_Ragdoll);
IMPLEMENT_CLIENTCLASS_DT(C_ServerRagdoll, DT_Ragdoll, CRagdollProp)
	RecvPropArray(RecvPropQAngles(RECVINFO(m_ragAngles[0])), m_ragAngles),
	RecvPropArray(RecvPropVector(RECVINFO(m_ragPos[0])), m_ragPos),
	RecvPropEHandle(RECVINFO(m_hUnragdoll)),
	RecvPropFloat(RECVINFO(m_flBlendWeight)),
	RecvPropInt(RECVINFO(m_nOverlaySequence)),
END_RECV_TABLE()


C_ServerRagdoll::C_ServerRagdoll( void ) :
	m_iv_ragPos("C_ServerRagdoll::m_iv_ragPos"),
	m_iv_ragAngles("C_ServerRagdoll::m_iv_ragAngles")
{
	m_elementCount = 0;
	m_flLastBoneChangeTime = -FLT_MAX;

	AddVar( m_ragPos, &m_iv_ragPos, LATCH_SIMULATION_VAR  );
	AddVar( m_ragAngles, &m_iv_ragAngles, LATCH_SIMULATION_VAR );

	m_flBlendWeight = 0.0f;
	m_flBlendWeightCurrent = 0.0f;
	m_nOverlaySequence = -1;
	m_flFadeScale = 1;
}

void C_ServerRagdoll::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	m_iv_ragPos.NoteChanged( gpGlobals->curtime, true );
	m_iv_ragAngles.NoteChanged( gpGlobals->curtime, true );
	// this is the local client time at which this update becomes stale
	m_flLastBoneChangeTime = gpGlobals->curtime + GetInterpolationAmount(m_iv_ragPos.GetType());
}

float C_ServerRagdoll::LastBoneChangedTime()
{
	return m_flLastBoneChangeTime;
}

int C_ServerRagdoll::InternalDrawModel( int flags )
{
	int ret = BaseClass::InternalDrawModel( flags );
	if ( vcollide_wireframe.GetBool() )
	{
		vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
		IMaterial *pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);

		matrix3x4_t matrix;
		for ( int i = 0; i < m_elementCount; i++ )
		{
			static color32 debugColor = {0,255,255,0};

			AngleMatrix( m_ragAngles[i], m_ragPos[i], matrix );
			engine->DebugDrawPhysCollide( pCollide->solids[i], pWireframe, matrix, debugColor );
		}
	}
	return ret;
}


CStudioHdr *C_ServerRagdoll::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if ( !m_elementCount )
	{
		vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
		if ( !pCollide )
		{
			const char *pszName = modelinfo->GetModelName( modelinfo->GetModel( GetModelIndex() ) );
			Msg( "*** ERROR: C_ServerRagdoll::InitModel: %s missing vcollide data ***\n", (pszName) ? pszName : "<null>" );
			m_elementCount = 0;
		}
		else
		{
			m_elementCount = RagdollExtractBoneIndices( m_boneIndex, hdr, pCollide );
		}
		m_iv_ragPos.SetMaxCount( m_elementCount );
		m_iv_ragAngles.SetMaxCount( m_elementCount );
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_ServerRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
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
		if (GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
		}
	}
}


void C_ServerRagdoll::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if( !CollisionProp()->IsBoundsDefinedInEntitySpace() )
	{
		IRotateAABB( EntityToWorldTransform(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), theMins, theMaxs );
	}
	else
	{
		theMins = CollisionProp()->OBBMins();
		theMaxs = CollisionProp()->OBBMaxs();
	}
}

void C_ServerRagdoll::AddEntity( void )
{
	BaseClass::AddEntity();

	// Move blend weight toward target over 0.2 seconds
	m_flBlendWeightCurrent = Approach( m_flBlendWeight, m_flBlendWeightCurrent, gpGlobals->frametime * 5.0f );
}

void C_ServerRagdoll::AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	BaseClass::AccumulateLayers( boneSetup, pos, q, currentTime );

	if ( m_nOverlaySequence >= 0 && m_nOverlaySequence < boneSetup.GetStudioHdr()->GetNumSeq() )
	{
		boneSetup.AccumulatePose( pos, q, m_nOverlaySequence, GetCycle(), m_flBlendWeightCurrent, currentTime, m_pIk );
	}
}

void C_ServerRagdoll::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	if ( !hdr )
		return;
	matrix3x4_t bonematrix;
	bool boneSimulated[MAXSTUDIOBONES];

	// no bones have been simulated
	memset( boneSimulated, 0, sizeof(boneSimulated) );
	mstudiobone_t *pbones = hdr->pBone( 0 );

	mstudioseqdesc_t *pSeqDesc = NULL;
	if ( m_nOverlaySequence >= 0 && m_nOverlaySequence < hdr->GetNumSeq() )
	{
		pSeqDesc = &hdr->pSeqdesc( m_nOverlaySequence );
	}

	int i;
	for ( i = 0; i < m_elementCount; i++ )
	{
		int iBone = m_boneIndex[i];
		if ( iBone >= 0 )
		{
			if ( hdr->boneFlags( iBone ) & boneMask )
			{
				boneSimulated[iBone] = true;
				matrix3x4_t &matrix = GetBoneForWrite( iBone );

				if ( m_flBlendWeightCurrent != 0.0f && pSeqDesc && 
					 // FIXME: this bone access is illegal
					 pSeqDesc->weight( iBone ) != 0.0f )
				{
					// Use the animated bone position instead
					boneSimulated[iBone] = false;
				}
				else
				{	
					AngleMatrix( m_ragAngles[i], m_ragPos[i], matrix );
				}
			}
		}
	}

	for ( i = 0; i < hdr->numbones(); i++ ) 
	{
		if ( !( hdr->boneFlags( i ) & boneMask ) )
			continue;

		// BUGBUG: Merge this code with the code in c_baseanimating somehow!!!
		// animate all non-simulated bones
		if ( boneSimulated[i] || 
			CalcProceduralBone( hdr, i, m_BoneAccessor ) )
		{
			continue;
		}
		else
		{
			QuaternionMatrix( q[i], pos[i], bonematrix );

			if (pbones[i].parent == -1) 
			{
				ConcatTransforms( cameraTransform, bonematrix, GetBoneForWrite( i ) );
			} 
			else 
			{
				ConcatTransforms( GetBone( pbones[i].parent ), bonematrix, GetBoneForWrite( i ) );
			}
		}

		if ( pbones[i].parent == -1 ) 
		{
			// Apply client-side effects to the transformation matrix
		//	ApplyBoneMatrixTransform( GetBoneForWrite( i ) );
		}
	}
}

IPhysicsObject *C_ServerRagdoll::GetElement( int elementNum ) 
{ 
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 	virtual void
//-----------------------------------------------------------------------------
void C_ServerRagdoll::UpdateOnRemove()
{
	C_BaseAnimating *anim = m_hUnragdoll.Get();
	if ( NULL != anim && 
		anim->GetModel() && 
		( anim->GetModel() == GetModel() ) )
	{
		// Need to tell C_BaseAnimating to blend out of the ragdoll data that we received last
		C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false );
		anim->CreateUnragdollInfo( this );
	}

	// Do last to mimic destrictor order
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Fade out
//-----------------------------------------------------------------------------
unsigned char C_ServerRagdoll::GetClientSideFade()
{
	return UTIL_ComputeEntityFade( this, m_fadeMinDist, m_fadeMaxDist, m_flFadeScale );
}

static int GetHighestBit( int flags )
{
	for ( int i = 31; i >= 0; --i )
	{
		if ( flags & (1<<i) )
			return (1<<i);
	}

	return 0;
}

#define ATTACH_INTERP_TIME	0.2
class C_ServerRagdollAttached : public C_ServerRagdoll
{
	DECLARE_CLASS( C_ServerRagdollAttached, C_ServerRagdoll );
public:
	C_ServerRagdollAttached( void ) 
	{
		m_bHasParent = false;
		m_vecOffset.Init();
	}
	DECLARE_CLIENTCLASS();
	bool SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
	{
		if ( GetMoveParent() )
		{
			// HACKHACK: Force the attached bone to be set up
			int iBone = m_boneIndex[m_ragdollAttachedObjectIndex];
			int boneFlags = GetModelPtr()->boneFlags( iBone );
			if ( !(boneFlags & boneMask) )
			{
				// BUGBUG: The attached bone is required and this call is going to skip it, so force it
				// HACKHACK: Assume the highest bit numbered bone flag is the minimum bone set
				boneMask |= GetHighestBit( boneFlags );
			}
		}
		return BaseClass::SetupBones( pBoneToWorldOut, nMaxBones, boneMask, currentTime );
	}

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
	{
		VPROF_BUDGET( "C_ServerRagdollAttached::SetupBones", VPROF_BUDGETGROUP_CLIENT_ANIMATION );

		if ( !hdr )
			return;

		float frac = RemapVal( gpGlobals->curtime, m_parentTime, m_parentTime+ATTACH_INTERP_TIME, 0, 1 );
		frac = clamp( frac, 0.f, 1.f );
		// interpolate offset over some time
		Vector offset = m_vecOffset * (1-frac);

		C_BaseAnimating *parent = assert_cast< C_BaseAnimating* >( GetMoveParent() );
		Vector worldOrigin;
		worldOrigin.Init();


		if ( parent )
		{
			Assert( parent != this );
			parent->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

			matrix3x4_t boneToWorld;
			parent->GetCachedBoneMatrix( m_boneIndexAttached, boneToWorld );
			VectorTransform( m_attachmentPointBoneSpace, boneToWorld, worldOrigin );
		}
		BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

		if ( parent )
		{
			int iBone = m_boneIndex[m_ragdollAttachedObjectIndex];
			const matrix3x4_t &matrix = GetBone( iBone );
			Vector ragOrigin;
			VectorTransform( m_attachmentPointRagdollSpace, matrix, ragOrigin );
			offset = worldOrigin - ragOrigin;
			// fixes culling
			SetAbsOrigin( worldOrigin );
			m_vecOffset = offset;
		}

		for ( int i = 0; i < hdr->numbones(); i++ )
		{
			if ( !( hdr->boneFlags( i ) & boneMask ) )
				continue;

			Vector vPos;
			matrix3x4_t &matrix = GetBoneForWrite( i );
			MatrixGetColumn( matrix, 3, vPos );
			vPos += offset;
			MatrixSetColumn( vPos, 3, matrix );
		}
	}
	void OnDataChanged( DataUpdateType_t updateType );
	virtual float LastBoneChangedTime() { return FLT_MAX; }

	Vector		m_attachmentPointBoneSpace;
	Vector		m_vecOffset;
	Vector		m_attachmentPointRagdollSpace;
	int			m_ragdollAttachedObjectIndex;
	int			m_boneIndexAttached;
	float		m_parentTime;
	bool		m_bHasParent;
private:
	C_ServerRagdollAttached( const C_ServerRagdollAttached & );
};

EXTERN_RECV_TABLE(DT_Ragdoll_Attached);
IMPLEMENT_CLIENTCLASS_DT(C_ServerRagdollAttached, DT_Ragdoll_Attached, CRagdollPropAttached)
	RecvPropInt( RECVINFO( m_boneIndexAttached ) ),
	RecvPropInt( RECVINFO( m_ragdollAttachedObjectIndex ) ),
	RecvPropVector(RECVINFO(m_attachmentPointBoneSpace) ),
	RecvPropVector(RECVINFO(m_attachmentPointRagdollSpace) ),
END_RECV_TABLE()

void C_ServerRagdollAttached::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	bool bParentNow = GetMoveParent() ? true : false;
	if ( m_bHasParent != bParentNow )
	{
		if ( m_bHasParent )
		{
			m_parentTime = gpGlobals->curtime;
		}
		m_bHasParent = bParentNow;
	}
}

struct ragdoll_remember_t
{
	C_BaseEntity	*ragdoll;
	int				tickCount;
};

struct ragdoll_memory_list_t
{
	CUtlVector<ragdoll_remember_t>	list;

	int tickCount;

	void Update()
	{
		if ( tickCount > gpGlobals->tickcount )
		{
			list.RemoveAll();
			return;
		}

		for ( int i = list.Count()-1; i >= 0; --i )
		{
			if ( list[i].tickCount != gpGlobals->tickcount )
			{
				list.FastRemove(i);
			}
		}
	}

	bool IsInList( C_BaseEntity *pRagdoll )
	{
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			if ( list[i].ragdoll == pRagdoll )
				return true;
		}

		return false;
	}
	void AddToList( C_BaseEntity *pRagdoll )
	{
		Update();
		int index = list.AddToTail();
		list[index].ragdoll = pRagdoll;
		list[index].tickCount = gpGlobals->tickcount;
	}
};

static ragdoll_memory_list_t gRagdolls;

void NoteRagdollCreationTick( C_BaseEntity *pRagdoll )
{
	gRagdolls.AddToList( pRagdoll );
}

// returns true if the ragdoll was created on this tick
bool WasRagdollCreatedOnCurrentTick( C_BaseEntity *pRagdoll )
{
	gRagdolls.Update();
	return gRagdolls.IsInList( pRagdoll );
}

