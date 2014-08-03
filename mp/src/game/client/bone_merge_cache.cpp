//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "bone_merge_cache.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CBoneMergeCache
//-----------------------------------------------------------------------------

CBoneMergeCache::CBoneMergeCache()
{
	m_pOwner = NULL;
	m_pFollow = NULL;
	m_pFollowHdr = NULL;
	m_pFollowRenderHdr = NULL;
	m_pOwnerHdr = NULL;
	m_nFollowBoneSetupMask = 0;
}

void CBoneMergeCache::Init( C_BaseAnimating *pOwner )
{
	m_pOwner = pOwner;
	m_pFollow = NULL;
	m_pFollowHdr = NULL;
	m_pFollowRenderHdr = NULL;
	m_pOwnerHdr = NULL;
	m_nFollowBoneSetupMask = 0;
}

void CBoneMergeCache::UpdateCache()
{
	CStudioHdr *pOwnerHdr = m_pOwner ? m_pOwner->GetModelPtr() : NULL;
	if ( !pOwnerHdr )
	{
		if ( m_pOwnerHdr )
		{
			// Owner's model got swapped out
			m_MergedBones.Purge();
			m_BoneMergeBits.Purge();
			m_pFollow = NULL;
			m_pFollowHdr = NULL;
			m_pFollowRenderHdr = NULL;
			m_pOwnerHdr = NULL;
			m_nFollowBoneSetupMask = 0;
		}
		return;
	}

	C_BaseAnimating *pTestFollow = m_pOwner->FindFollowedEntity();
	CStudioHdr *pTestHdr = (pTestFollow ? pTestFollow->GetModelPtr() : NULL);
	const studiohdr_t *pTestStudioHDR = (pTestHdr ? pTestHdr->GetRenderHdr() : NULL);
	if ( pTestFollow != m_pFollow || pTestHdr != m_pFollowHdr || pTestStudioHDR != m_pFollowRenderHdr || pOwnerHdr != m_pOwnerHdr )
	{
		m_MergedBones.Purge();
		m_BoneMergeBits.Purge();
	
		// Update the cache.
		if ( pTestFollow && pTestHdr && pOwnerHdr )
		{
			m_pFollow = pTestFollow;
			m_pFollowHdr = pTestHdr;
			m_pFollowRenderHdr = pTestStudioHDR;
			m_pOwnerHdr = pOwnerHdr;

			m_BoneMergeBits.SetSize( pOwnerHdr->numbones() / 8 + 1 );
			memset( m_BoneMergeBits.Base(), 0, m_BoneMergeBits.Count() );

			mstudiobone_t *pOwnerBones = m_pOwnerHdr->pBone( 0 );
			
			m_nFollowBoneSetupMask = BONE_USED_BY_BONE_MERGE;
			for ( int i = 0; i < m_pOwnerHdr->numbones(); i++ )
			{
				int parentBoneIndex = Studio_BoneIndexByName( m_pFollowHdr, pOwnerBones[i].pszName() );
				if ( parentBoneIndex < 0 )
					continue;

				// Add a merged bone here.
				CMergedBone mergedBone;
				mergedBone.m_iMyBone = i;
				mergedBone.m_iParentBone = parentBoneIndex;
				m_MergedBones.AddToTail( mergedBone );

				m_BoneMergeBits[i>>3] |= ( 1 << ( i & 7 ) );

				if ( ( m_pFollowHdr->boneFlags( parentBoneIndex ) & BONE_USED_BY_BONE_MERGE ) == 0 )
				{
					m_nFollowBoneSetupMask = BONE_USED_BY_ANYTHING;
//					Warning("Performance warning: Merge with '%s'. Mark bone '%s' in model '%s' as being used by bone merge in the .qc!\n",
//						pOwnerHdr->pszName(), m_pFollowHdr->pBone( parentBoneIndex )->pszName(), m_pFollowHdr->pszName() ); 
				}
			}

			// No merged bones found? Slam the mask to 0
			if ( !m_MergedBones.Count() )
			{
				m_nFollowBoneSetupMask = 0;
			}
		}
		else
		{
			m_pFollow = NULL;
			m_pFollowHdr = NULL;
			m_pFollowRenderHdr = NULL;
			m_pOwnerHdr = NULL;
			m_nFollowBoneSetupMask = 0;
		}
	}
}


#ifdef STAGING_ONLY
ConVar r_captain_canteen_is_angry ( "r_captain_canteen_is_angry", "1" );
#endif

void CBoneMergeCache::MergeMatchingBones( int boneMask )
{
	UpdateCache();

	// If this is set, then all the other cache data is set.
	if ( !m_pOwnerHdr || m_MergedBones.Count() == 0 )
		return;

	// Have the entity we're following setup its bones.
	bool bWorked = m_pFollow->SetupBones( NULL, -1, m_nFollowBoneSetupMask, gpGlobals->curtime );
	// We suspect there's some cases where SetupBones couldn't do its thing, and then this causes Captain Canteen.
	Assert ( bWorked );
	if ( !bWorked )
	{
		// Usually this means your parent is invisible or gone or whatever.
		// This routine has no way to tell its caller not to draw itself unfortunately.
		// But we can shrink all the bones down to zero size.
		// But it might still spawn particle systems? :-(
		matrix3x4_t NewBone;
		MatrixScaleByZero ( NewBone );
		MatrixSetTranslation ( Vector ( 0.0f, 0.0f, 0.0f ), NewBone );
#ifdef STAGING_ONLY
		if ( r_captain_canteen_is_angry.GetBool() )
		{
			// We actually want to see when Captain Canteen happened, and make it really obvious that (a) he was here and (b) this code would have fixed him.
			float HowAngry = 20.0f;		// Leon's getting larger!
			MatrixSetColumn ( Vector ( HowAngry, 0.0f, 0.0f ), 0, NewBone );
			MatrixSetColumn ( Vector ( 0.0f, HowAngry, 0.0f ), 1, NewBone );
			MatrixSetColumn ( Vector ( 0.0f, 0.0f, HowAngry ), 2, NewBone );
		}
#endif

		for ( int i=0; i < m_MergedBones.Count(); i++ )
		{
			int iOwnerBone = m_MergedBones[i].m_iMyBone;
		
			// Only update bones reference by the bone mask.
			if ( !( m_pOwnerHdr->boneFlags( iOwnerBone ) & boneMask ) )
				continue;

			m_pOwner->GetBoneForWrite( iOwnerBone ) = NewBone;
		}
	}
	else
	{
		// Now copy the bone matrices.
		for ( int i=0; i < m_MergedBones.Count(); i++ )
		{
			int iOwnerBone = m_MergedBones[i].m_iMyBone;
			int iParentBone = m_MergedBones[i].m_iParentBone;
		
			// Only update bones reference by the bone mask.
			if ( !( m_pOwnerHdr->boneFlags( iOwnerBone ) & boneMask ) )
				continue;

			MatrixCopy( m_pFollow->GetBone( iParentBone ), m_pOwner->GetBoneForWrite( iOwnerBone ) );
		}
	}
}


	// copy bones instead of matrices
void CBoneMergeCache::CopyParentToChild( const Vector parentPos[], const Quaternion parentQ[], Vector childPos[], Quaternion childQ[], int boneMask )
{
	UpdateCache();

	// If this is set, then all the other cache data is set.
	if ( !m_pOwnerHdr || m_MergedBones.Count() == 0 )
		return;

	// Now copy the bone matrices.
	for ( int i=0; i < m_MergedBones.Count(); i++ )
	{
		int iOwnerBone = m_MergedBones[i].m_iMyBone;
		int iParentBone = m_MergedBones[i].m_iParentBone;
		
		if ( m_pOwnerHdr->boneParent( iOwnerBone ) == -1 || m_pFollowHdr->boneParent( iParentBone ) == -1 )
			continue;

		// Only update bones reference by the bone mask.
		if ( !( m_pOwnerHdr->boneFlags( iOwnerBone ) & boneMask ) )
			continue;

		childPos[ iOwnerBone ] = parentPos[ iParentBone ];
		childQ[ iOwnerBone ] = parentQ[ iParentBone ];
	}
}

void CBoneMergeCache::CopyChildToParent( const Vector childPos[], const Quaternion childQ[], Vector parentPos[], Quaternion parentQ[], int boneMask )
{
	UpdateCache();

	// If this is set, then all the other cache data is set.
	if ( !m_pOwnerHdr || m_MergedBones.Count() == 0 )
		return;

	// Now copy the bone matrices.
	for ( int i=0; i < m_MergedBones.Count(); i++ )
	{
		int iOwnerBone = m_MergedBones[i].m_iMyBone;
		int iParentBone = m_MergedBones[i].m_iParentBone;
		
		if ( m_pOwnerHdr->boneParent( iOwnerBone ) == -1 || m_pFollowHdr->boneParent( iParentBone ) == -1 )
			continue;

		// Only update bones reference by the bone mask.
		if ( !( m_pOwnerHdr->boneFlags( iOwnerBone ) & boneMask ) )
			continue;

		parentPos[ iParentBone ] = childPos[ iOwnerBone ];
		parentQ[ iParentBone ] = childQ[ iOwnerBone ];
	}
}


bool CBoneMergeCache::GetAimEntOrigin( Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	UpdateCache();

	// If this is set, then all the other cache data is set.
	if ( !m_pOwnerHdr || m_MergedBones.Count() == 0 )
		return false;

	// We want the abs origin such that if we put the entity there, the first merged bone
	// will be aligned. This way the entity will be culled in the correct position.
	//
	// ie: mEntity * mBoneLocal = mFollowBone
	// so: mEntity = mFollowBone * Inverse( mBoneLocal )
	//
	// Note: the code below doesn't take animation into account. If the attached entity animates
	// all over the place, then this won't get the right results.
	
	// Get mFollowBone.
	m_pFollow->SetupBones( NULL, -1, m_nFollowBoneSetupMask, gpGlobals->curtime );
	const matrix3x4_t &mFollowBone = m_pFollow->GetBone( m_MergedBones[0].m_iParentBone );

	// Get Inverse( mBoneLocal )
	matrix3x4_t mBoneLocal, mBoneLocalInv;
	SetupSingleBoneMatrix( m_pOwnerHdr, m_pOwner->GetSequence(), 0, m_MergedBones[0].m_iMyBone, mBoneLocal );
	MatrixInvert( mBoneLocal, mBoneLocalInv );

	// Now calculate mEntity = mFollowBone * Inverse( mBoneLocal )
	matrix3x4_t mEntity;
	ConcatTransforms( mFollowBone, mBoneLocalInv, mEntity );
	MatrixAngles( mEntity, *pAbsAngles, *pAbsOrigin );

	return true;
}

bool CBoneMergeCache::GetRootBone( matrix3x4_t &rootBone )
{
	UpdateCache();

	// If this is set, then all the other cache data is set.
	if ( !m_pOwnerHdr || m_MergedBones.Count() == 0 )
		return false;

	// Get mFollowBone.
	m_pFollow->SetupBones( NULL, -1, m_nFollowBoneSetupMask, gpGlobals->curtime );
	rootBone = m_pFollow->GetBone( m_MergedBones[0].m_iParentBone );
	return true;
}


