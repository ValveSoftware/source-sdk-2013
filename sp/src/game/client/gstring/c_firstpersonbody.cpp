
#include "cbase.h"
#include "c_firstpersonbody.h"

#include "bone_setup.h"
#include "jigglebones.h"
#include "viewrender.h"

static ConVar gstring_firstpersonbody_scale( "gstring_firstpersonbody_scale", "1" );
static ConVar gstring_firstpersonbody_hiddenbone_scale( "gstring_firstpersonbody_hiddenbone_scale", "0.01" );

C_FirstpersonBody::C_FirstpersonBody()
	: m_iBoneNeck( -1 )
	, m_iBoneArmL( -1 )
	, m_iBoneArmR( -1 )
	, m_iPoseParam_MoveYaw( -1 )
{
}

CStudioHdr *C_FirstpersonBody::OnNewModel()
{
	CStudioHdr *pRet = BaseClass::OnNewModel();

	m_iBoneNeck = LookupBone( "ValveBiped.Bip01_Neck1" );
	m_iBoneArmL = LookupBone( "ValveBiped.Bip01_L_UpperArm" );
	m_iBoneArmR = LookupBone( "ValveBiped.Bip01_R_UpperArm" );
	m_iPoseParam_MoveYaw = LookupPoseParameter( "move_yaw" );

	return pRet;
}

ShadowType_t C_FirstpersonBody::ShadowCastType()
{
	return SHADOWS_SIMPLE;
}

void C_FirstpersonBody::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
	const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
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
					ConcatTransforms( GetBone( pbones[i].parent ), bonematrix, goalMX );
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
			MatrixScaleBy( gstring_firstpersonbody_scale.GetFloat(), GetBoneForWrite( i ) );
		}

		if ( i == m_iBoneNeck
			|| i == m_iBoneArmR
			|| i == m_iBoneArmL )
		{
			MatrixScaleBy( gstring_firstpersonbody_hiddenbone_scale.GetFloat(), GetBoneForWrite( i ) );
		}
	}
}

int C_FirstpersonBody::DrawModel( int flags )
{
	if ( CurrentViewID() == VIEW_SHADOW_DEPTH_TEXTURE )
		return 0;

	return BaseClass::DrawModel( flags );
}
