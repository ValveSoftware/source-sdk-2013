//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "studio.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// How large the stack is for flex rules
#define STUDIO_FLEX_STACK 32

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioanimdesc_t &studiohdr_t::pAnimdesc( int i ) const
{ 
	if (numincludemodels == 0)
	{
		return *pLocalAnimdesc( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_anim[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalAnimdesc( pVModel->m_anim[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioanim_t *mstudioanimdesc_t::pAnimBlock( int block, int index ) const
{
	if (block == -1)
	{
		return (mstudioanim_t *)NULL;
	}
	if (block == 0)
	{
		return (mstudioanim_t *)(((byte *)this) + index);
	}

	byte *pAnimBlock = pStudiohdr()->GetAnimBlock( block );
	if ( pAnimBlock )
	{
		return (mstudioanim_t *)(pAnimBlock + index);
	}

	return (mstudioanim_t *)NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

static ConVar mod_load_showstall( "mod_load_showstall", "0", 0, "1 - show hitches , 2 - show stalls" );
mstudioanim_t *mstudioanimdesc_t::pAnim( int *piFrame ) const
{
	float flStall;
	return pAnim( piFrame, flStall );
}

mstudioanim_t *mstudioanimdesc_t::pAnim( int *piFrame, float &flStall ) const
{
	mstudioanim_t *panim = NULL;

	int block = animblock;
	int index = animindex;
	int section = 0;

	if (sectionframes != 0)
	{
		if (numframes > sectionframes && *piFrame == numframes - 1)
		{
			// last frame on long anims is stored separately
			*piFrame = 0;
			section = (numframes / sectionframes) + 1;
		}
		else
		{
			section = *piFrame / sectionframes;
			*piFrame -= section * sectionframes;
		}

		block = pSection( section )->animblock;
		index = pSection( section )->animindex;
	}

	if (block == -1)
	{
		// model needs to be recompiled
		return NULL;
	}

	panim = pAnimBlock( block, index );

	// force a preload on the next block
	if ( sectionframes != 0 )
	{
		int count = ( numframes / sectionframes) + 2;
		for ( int i = section + 1; i < count; i++ )
		{
			if ( pSection( i )->animblock != block )
			{
				pAnimBlock( pSection( i )->animblock, pSection( i )->animindex );
				break;
			}
		}
	}

	if (panim == NULL)
	{
		if (section > 0 && mod_load_showstall.GetInt() > 0)
		{
			Msg("[%8.3f] hitch on %s:%s:%d:%d\n", Plat_FloatTime(), pStudiohdr()->pszName(), pszName(), section, block );
		}
		// back up until a previously loaded block is found
		while (--section >= 0)
		{
			block = pSection( section )->animblock;
			index = pSection( section )->animindex;
			panim = pAnimBlock( block, index );
			if (panim)
			{
				// set it to the last frame in the last valid section
				*piFrame = sectionframes - 1;
				break;
			}
		}
	}

	// try to guess a valid stall time interval (tuned for the X360)
	flStall = 0.0f;
	if (panim == NULL && section <= 0)
	{
		zeroframestalltime = Plat_FloatTime();
		flStall = 1.0f;
	}
	else if (panim != NULL && zeroframestalltime != 0.0f)
	{
		float dt = Plat_FloatTime() - zeroframestalltime;
		if (dt >= 0.0)
		{
			flStall = SimpleSpline( clamp( (0.200f - dt) * 5.0f, 0.0f, 1.0f ) );
		}

		if (flStall == 0.0f)
		{
			// disable stalltime
			zeroframestalltime = 0.0f;
		}
		else if (mod_load_showstall.GetInt() > 1)
		{
			Msg("[%8.3f] stall blend %.2f on %s:%s:%d:%d\n", Plat_FloatTime(), flStall, pStudiohdr()->pszName(), pszName(), section, block );
		}
	}

	if (panim == NULL && mod_load_showstall.GetInt() > 1)
	{
		Msg("[%8.3f] stall on %s:%s:%d:%d\n", Plat_FloatTime(), pStudiohdr()->pszName(), pszName(), section, block );
	}

	return panim;
}

mstudioikrule_t *mstudioanimdesc_t::pIKRule( int i ) const
{
	if (ikruleindex)
	{
		return (mstudioikrule_t *)(((byte *)this) + ikruleindex) + i;
	}
	else if (animblockikruleindex)
	{
		if (animblock == 0)
		{
			return  (mstudioikrule_t *)(((byte *)this) + animblockikruleindex) + i;
		}
		else
		{
			byte *pAnimBlocks = pStudiohdr()->GetAnimBlock( animblock );
			
			if ( pAnimBlocks )
			{
				return (mstudioikrule_t *)(pAnimBlocks + animblockikruleindex) + i;
			}
		}
	}

	return NULL;
}


mstudiolocalhierarchy_t *mstudioanimdesc_t::pHierarchy( int i ) const
{
	if (localhierarchyindex)
	{
		if (animblock == 0)
		{
			return  (mstudiolocalhierarchy_t *)(((byte *)this) + localhierarchyindex) + i;
		}
		else
		{
			byte *pAnimBlocks = pStudiohdr()->GetAnimBlock( animblock );
			
			if ( pAnimBlocks )
			{
				return (mstudiolocalhierarchy_t *)(pAnimBlocks + localhierarchyindex) + i;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool studiohdr_t::SequencesAvailable() const
{
	if (numincludemodels == 0)
	{
		return true;
	}

	return ( GetVirtualModel() != NULL );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetNumSeq( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalseq;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );
	return pVModel->m_seq.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioseqdesc_t &studiohdr_t::pSeqdesc( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalSeqdesc( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( !pVModel )
	{
		return *pLocalSeqdesc( i );
	}

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalSeqdesc( pVModel->m_seq[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::iRelativeAnim( int baseseq, int relanim ) const
{
	if (numincludemodels == 0)
	{
		return relanim;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[baseseq].group ];

	return pGroup->masterAnim[ relanim ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::iRelativeSeq( int baseseq, int relseq ) const
{
	if (numincludemodels == 0)
	{
		return relseq;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[baseseq].group ];

	return pGroup->masterSeq[ relseq ];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetNumPoseParameters( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalposeparameters;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_pose.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioposeparamdesc_t &studiohdr_t::pPoseParameter( int i )
{
	if (numincludemodels == 0)
	{
		return *pLocalPoseParameter( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( pVModel->m_pose[i].group == 0)
		return *pLocalPoseParameter( pVModel->m_pose[i].index );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_pose[i].group ];

	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalPoseParameter( pVModel->m_pose[i].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetSharedPoseParameter( int iSequence, int iLocalPose ) const
{
	if (numincludemodels == 0)
	{
		return iLocalPose;
	}

	if (iLocalPose == -1)
		return iLocalPose;

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	int group = pVModel->m_seq[iSequence].group;
	virtualgroup_t* pGroup = pVModel->m_group.IsValidIndex( group ) ? &pVModel->m_group[group] : NULL;

	// Josh: This used to return iLocalPose when it was out of bounds
	// but that is not correct, this should return -1 because
	// otherwise it's just some random unrelated index. 
	if ( !pGroup )
		return -1;

	// Josh: Sometimes the Sniper can try to eat a gun and we hit this.
	// Model pose group data is just complete garbage out of studiomdl, woo!
	if ( !pGroup->masterPose.IsValidIndex( iLocalPose ) )
		return -1;

	return pGroup->masterPose[iLocalPose];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::EntryNode( int iSequence )
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (numincludemodels == 0 || seqdesc.localentrynode == 0)
	{
		return seqdesc.localentrynode;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localentrynode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int studiohdr_t::ExitNode( int iSequence )
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (numincludemodels == 0 || seqdesc.localexitnode == 0)
	{
		return seqdesc.localexitnode;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localexitnode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetNumAttachments( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalattachments;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_attachment.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioattachment_t &studiohdr_t::pAttachment( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalAttachment( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalAttachment( pVModel->m_attachment[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetAttachmentBone( int i )
{
	const mstudioattachment_t &attachment = pAttachment( i );

	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[i].group ];
		int iBone = pGroup->masterBone[attachment.localbone];
		if (iBone == -1)
			return 0;
		return iBone;
	}
	return attachment.localbone;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void studiohdr_t::SetAttachmentBone( int iAttachment, int iBone )
{
	mstudioattachment_t &attachment = (mstudioattachment_t &)pAttachment( iAttachment );

	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[iAttachment].group ];
		iBone = pGroup->boneMap[iBone];
	}
	attachment.localbone = iBone;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

char *studiohdr_t::pszNodeName( int iNode )
{
	if (numincludemodels == 0)
	{
		return pszLocalNodeName( iNode );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( pVModel->m_node.Count() <= iNode-1 )
		return "Invalid node";

	return pVModel->m_group[ pVModel->m_node[iNode-1].group ].GetStudioHdr()->pszLocalNodeName( pVModel->m_node[iNode-1].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetTransition( int iFrom, int iTo ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalTransition( (iFrom-1)*numlocalnodes + (iTo - 1) );
	}

	return iTo;
	/*
	FIXME: not connected
	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_transition.Element( iFrom ).Element( iTo );
	*/
}


int	studiohdr_t::GetActivityListVersion( void )
{
	if (numincludemodels == 0)
	{
		return activitylistversion;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	int ActVersion = activitylistversion;

	int i;
	for (i = 1; i < pVModel->m_group.Count(); i++)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ i ];
		const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();

		Assert( pStudioHdr );

		ActVersion = min( ActVersion, pStudioHdr->activitylistversion );
	}

	return ActVersion;
}

void studiohdr_t::SetActivityListVersion( int ActVersion ) const
{
	activitylistversion = ActVersion;

	if (numincludemodels == 0)
	{
		return;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	int i;
	for (i = 1; i < pVModel->m_group.Count(); i++)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ i ];
		const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();

		Assert( pStudioHdr );

		pStudioHdr->SetActivityListVersion( ActVersion );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int studiohdr_t::GetNumIKAutoplayLocks( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalikautoplaylocks;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_iklock.Count();
}

const mstudioiklock_t &studiohdr_t::pIKAutoplayLock( int i )
{
	if (numincludemodels == 0)
	{
		return *pLocalIKAutoplayLock( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_iklock[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalIKAutoplayLock( pVModel->m_iklock[i].index );
}

int	studiohdr_t::CountAutoplaySequences() const
{
	int count = 0;
	for (int i = 0; i < GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			count++;
		}
	}
	return count;
}

int	studiohdr_t::CopyAutoplaySequences( unsigned short *pOut, int outCount ) const
{
	int outIndex = 0;
	for (int i = 0; i < GetNumSeq() && outIndex < outCount; i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			pOut[outIndex] = i;
			outIndex++;
		}
	}
	return outIndex;
}

//-----------------------------------------------------------------------------
// Purpose:	maps local sequence bone to global bone
//-----------------------------------------------------------------------------

int	studiohdr_t::RemapSeqBone( int iSequence, int iLocalBone ) const	
{
	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		const virtualgroup_t *pSeqGroup = pVModel->pSeqGroup( iSequence );
		return pSeqGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

int	studiohdr_t::RemapAnimBone( int iAnim, int iLocalBone ) const
{
	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		const virtualgroup_t *pAnimGroup = pVModel->pAnimGroup( iAnim );
		return pAnimGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}






//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

CStudioHdr::CStudioHdr( void ) 
{
	// set pointer to bogus value
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init( NULL );
}

CStudioHdr::CStudioHdr( const studiohdr_t *pStudioHdr, IMDLCache *mdlcache ) 
{
	// preset pointer to bogus value (it may be overwritten with legitimate data later)
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init( pStudioHdr, mdlcache );
}


// extern IDataCache *g_pDataCache;

void CStudioHdr::Init( const studiohdr_t *pStudioHdr, IMDLCache *mdlcache )
{
	m_pStudioHdr = pStudioHdr;

	m_pVModel = NULL;
	m_pStudioHdrCache.RemoveAll();

	if (m_pStudioHdr == NULL)
	{
		return;
	}

	if ( mdlcache )
	{
		m_pFrameUnlockCounter = mdlcache->GetFrameUnlockCounterPtr( MDLCACHE_STUDIOHDR );
		m_nFrameUnlockCounter = *m_pFrameUnlockCounter - 1;
	}

	if (m_pStudioHdr->numincludemodels == 0)
	{
#if STUDIO_SEQUENCE_ACTIVITY_LAZY_INITIALIZE
#else
		m_ActivityToSequence.Initialize(this);
#endif
	}
	else
	{
		ResetVModel( m_pStudioHdr->GetVirtualModel() );
#if STUDIO_SEQUENCE_ACTIVITY_LAZY_INITIALIZE
#else
		m_ActivityToSequence.Initialize(this);
#endif
	}

	m_boneFlags.EnsureCount( numbones() );
	m_boneParent.EnsureCount( numbones() );
	for (int i = 0; i < numbones(); i++)
	{
		m_boneFlags[i] = pBone( i )->flags;
		m_boneParent[i] = pBone( i )->parent;
	}
}

void CStudioHdr::Term()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CStudioHdr::SequencesAvailable() const
{
	if (m_pStudioHdr->numincludemodels == 0)
	{
		return true;
	}

	if (m_pVModel == NULL)
	{
		// repoll m_pVModel
		return (ResetVModel( m_pStudioHdr->GetVirtualModel() ) != NULL);
	}
	else
		return true;
}


const virtualmodel_t * CStudioHdr::ResetVModel( const virtualmodel_t *pVModel ) const
{
	if (pVModel != NULL)
	{
		m_pVModel = (virtualmodel_t *)pVModel;
		Assert( !pVModel->m_Lock.GetOwnerId() );
		m_pStudioHdrCache.SetCount( m_pVModel->m_group.Count() );

		int i;
		for (i = 0; i < m_pStudioHdrCache.Count(); i++)
		{
			m_pStudioHdrCache[ i ] = NULL;
		}
		
		return const_cast<virtualmodel_t *>(pVModel);
	}
	else
	{
		m_pVModel = NULL;
		return NULL;
	}
}

const studiohdr_t *CStudioHdr::GroupStudioHdr( int i )
{
	if ( !this )
	{
		ExecuteNTimes( 5, Warning( "Call to NULL CStudioHdr::GroupStudioHdr()\n" ) );
	}

	if ( m_nFrameUnlockCounter != *m_pFrameUnlockCounter )
	{
		m_FrameUnlockCounterMutex.Lock();
		if ( *m_pFrameUnlockCounter != m_nFrameUnlockCounter ) // i.e., this thread got the mutex
		{
			memset( m_pStudioHdrCache.Base(), 0, m_pStudioHdrCache.Count() * sizeof(studiohdr_t *) );
			m_nFrameUnlockCounter = *m_pFrameUnlockCounter;
		}
		m_FrameUnlockCounterMutex.Unlock();
	}

	if ( !m_pStudioHdrCache.IsValidIndex( i ) )
	{
		const char *pszName = ( m_pStudioHdr ) ? m_pStudioHdr->pszName() : "<<null>>";
		ExecuteNTimes( 5, Warning( "Invalid index passed to CStudioHdr(%s)::GroupStudioHdr(): %d, but max is %d\n", pszName, i, m_pStudioHdrCache.Count() ) );
		DebuggerBreakIfDebugging();
		return m_pStudioHdr; // return something known to probably exist, certainly things will be messed up, but hopefully not crash before the warning is noticed
	}

	const studiohdr_t *pStudioHdr = m_pStudioHdrCache[ i ];

	if (pStudioHdr == NULL)
	{
		Assert( !m_pVModel->m_Lock.GetOwnerId() );
		virtualgroup_t *pGroup = &m_pVModel->m_group[ i ];
		pStudioHdr = pGroup->GetStudioHdr();
		m_pStudioHdrCache[ i ] = pStudioHdr;
	}

	Assert( pStudioHdr );
	return pStudioHdr;
}


const studiohdr_t *CStudioHdr::pSeqStudioHdr( int sequence )
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_seq[sequence].group );

	return pStudioHdr;
}


const studiohdr_t *CStudioHdr::pAnimStudioHdr( int animation )
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_anim[animation].group );

	return pStudioHdr;
}



mstudioanimdesc_t &CStudioHdr::pAnimdesc( int i )
{ 
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAnimdesc( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_anim[i].group );

	return *pStudioHdr->pLocalAnimdesc( m_pVModel->m_anim[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetNumSeq( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalseq;
	}

	return m_pVModel->m_seq.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioseqdesc_t &CStudioHdr::pSeqdesc( int i )
{
	Assert( ( i >= 0 && i < GetNumSeq() ) || ( i == 1 && GetNumSeq() <= 1 ) );
	if ( i < 0 || i >= GetNumSeq() )
	{
		if ( GetNumSeq() <= 0 )
		{
			// Return a zero'd out struct reference if we've got nothing.
			// C_BaseObject::StopAnimGeneratedSounds was crashing due to this function
			//	returning a reference to garbage. It should now see numevents is 0,
			//	and bail.
			static mstudioseqdesc_t s_nil_seq;
			return s_nil_seq;
		}

		// Avoid reading random memory.
		i = 0;
	}
	
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalSeqdesc( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_seq[i].group );

	return *pStudioHdr->pLocalSeqdesc( m_pVModel->m_seq[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeAnim( int baseseq, int relanim ) const
{
	if (m_pVModel == NULL)
	{
		return relanim;
	}

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[baseseq].group ];

	return pGroup->masterAnim[ relanim ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeSeq( int baseseq, int relseq ) const
{
	if (m_pVModel == NULL)
	{
		return relseq;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[baseseq].group ];

	return pGroup->masterSeq[ relseq ];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumPoseParameters( void ) const
{
	if (m_pVModel == NULL)
	{
		if ( m_pStudioHdr )
			return m_pStudioHdr->numlocalposeparameters;
		else
			return 0;
	}

	Assert( m_pVModel );

	return m_pVModel->m_pose.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioposeparamdesc_t &CStudioHdr::pPoseParameter( int i )
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalPoseParameter( i );
	}

	if ( m_pVModel->m_pose[i].group == 0)
		return *m_pStudioHdr->pLocalPoseParameter( m_pVModel->m_pose[i].index );

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_pose[i].group );

	return *pStudioHdr->pLocalPoseParameter( m_pVModel->m_pose[i].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetSharedPoseParameter( int iSequence, int iLocalPose ) const
{
	if (m_pVModel == NULL)
	{
		return iLocalPose;
	}

	if (iLocalPose == -1)
		return iLocalPose;

	Assert( m_pVModel );

	int group = m_pVModel->m_seq[iSequence].group;
	virtualgroup_t *pGroup = m_pVModel->m_group.IsValidIndex( group ) ? &m_pVModel->m_group[ group ] : NULL;

	// Josh: This used to return iLocalPose when it was out of bounds
	// but that is not correct, this should return -1 because
	// otherwise it's just some random unrelated index. 
	if ( !pGroup )
		return -1;

	// Josh: Sometimes the Sniper can try to eat a gun and we hit this.
	// Model pose group data is just complete garbage out of studiomdl, woo!
	if ( !pGroup->masterPose.IsValidIndex( iLocalPose ) )
		return -1;

	return pGroup->masterPose[iLocalPose];

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::EntryNode( int iSequence )
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (m_pVModel == NULL || seqdesc.localentrynode == 0)
	{
		return seqdesc.localentrynode;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localentrynode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int CStudioHdr::ExitNode( int iSequence )
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (m_pVModel == NULL || seqdesc.localexitnode == 0)
	{
		return seqdesc.localexitnode;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localexitnode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumAttachments( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalattachments;
	}

	Assert( m_pVModel );

	return m_pVModel->m_attachment.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioattachment_t &CStudioHdr::pAttachment( int i )
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAttachment( i );
	}

	Assert( m_pVModel );

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_attachment[i].group );

	return *pStudioHdr->pLocalAttachment( m_pVModel->m_attachment[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetAttachmentBone( int i )
{
	if (m_pVModel == 0)
	{
		return m_pStudioHdr->pLocalAttachment( i )->localbone;
	}

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_attachment[i].group ];
	const mstudioattachment_t &attachment = pAttachment( i );
	int iBone = pGroup->masterBone[attachment.localbone];
	if (iBone == -1)
		return 0;
	return iBone;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CStudioHdr::SetAttachmentBone( int iAttachment, int iBone )
{
	mstudioattachment_t &attachment = (mstudioattachment_t &)m_pStudioHdr->pAttachment( iAttachment );

	// remap bone
	if (m_pVModel)
	{
		virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_attachment[iAttachment].group ];
		iBone = pGroup->boneMap[iBone];
	}
	attachment.localbone = iBone;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

char *CStudioHdr::pszNodeName( int iNode )
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->pszLocalNodeName( iNode );
	}

	if ( m_pVModel->m_node.Count() <= iNode-1 )
		return "Invalid node";

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_node[iNode-1].group );
	
	return pStudioHdr->pszLocalNodeName( m_pVModel->m_node[iNode-1].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetTransition( int iFrom, int iTo ) const
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalTransition( (iFrom-1)*m_pStudioHdr->numlocalnodes + (iTo - 1) );
	}

	return iTo;
	/*
	FIXME: not connected
	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_transition.Element( iFrom ).Element( iTo );
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetActivityListVersion( void )
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->activitylistversion;
	}

	int version = m_pStudioHdr->activitylistversion;

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		version = min( version, pStudioHdr->activitylistversion );
	}

	return version;
}

void CStudioHdr::SetActivityListVersion( int version )
{
	m_pStudioHdr->activitylistversion = version;

	if (m_pVModel == NULL)
	{
		return;
	}

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		pStudioHdr->SetActivityListVersion( version );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetEventListVersion( void )
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->eventsindexed;
	}

	int version = m_pStudioHdr->eventsindexed;

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		version = min( version, pStudioHdr->eventsindexed );
	}

	return version;
}

void CStudioHdr::SetEventListVersion( int version )
{
	m_pStudioHdr->eventsindexed = version;

	if (m_pVModel == NULL)
	{
		return;
	}

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		pStudioHdr->eventsindexed = version;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int CStudioHdr::GetNumIKAutoplayLocks( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalikautoplaylocks;
	}

	return m_pVModel->m_iklock.Count();
}

const mstudioiklock_t &CStudioHdr::pIKAutoplayLock( int i )
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalIKAutoplayLock( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_iklock[i].group );
	Assert( pStudioHdr );
	return *pStudioHdr->pLocalIKAutoplayLock( m_pVModel->m_iklock[i].index );
}

#if 0
int	CStudioHdr::CountAutoplaySequences() const
{
	int count = 0;
	for (int i = 0; i < GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			count++;
		}
	}
	return count;
}

int	CStudioHdr::CopyAutoplaySequences( unsigned short *pOut, int outCount ) const
{
	int outIndex = 0;
	for (int i = 0; i < GetNumSeq() && outIndex < outCount; i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			pOut[outIndex] = i;
			outIndex++;
		}
	}
	return outIndex;
}

#endif

//-----------------------------------------------------------------------------
// Purpose:	maps local sequence bone to global bone
//-----------------------------------------------------------------------------

int	CStudioHdr::RemapSeqBone( int iSequence, int iLocalBone ) const	
{
	// remap bone
	if (m_pVModel)
	{
		const virtualgroup_t *pSeqGroup = m_pVModel->pSeqGroup( iSequence );
		return pSeqGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

int	CStudioHdr::RemapAnimBone( int iAnim, int iLocalBone ) const
{
	// remap bone
	if (m_pVModel)
	{
		const virtualgroup_t *pAnimGroup = m_pVModel->pAnimGroup( iAnim );
		return pAnimGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

// JasonM hack
//ConVar	flex_maxrule( "flex_maxrule", "0" );


//-----------------------------------------------------------------------------
// Purpose: run the interpreted FAC's expressions, converting flex_controller 
//			values into FAC weights
//-----------------------------------------------------------------------------
void CStudioHdr::RunFlexRules( const float *src, float *dest )
{

	// FIXME: this shouldn't be needed, flex without rules should be stripped in studiomdl
	for (int i = 0; i < numflexdesc(); i++)
	{
		dest[i] = 0;
	}

	for (int i = 0; i < numflexrules(); i++)
	{
		float stack[STUDIO_FLEX_STACK] = {};
		int k = 0;
		mstudioflexrule_t *prule = pFlexRule( i );

		mstudioflexop_t *pops = prule->iFlexOp( 0 );

		if ( prule->flex < 0 || prule->flex >= numflexdesc() )
		{
			AssertMsg( false, "Invalid flex rules in model" );
			continue;
		}
/*
		// JasonM hack for flex perf testing...
		int nFlexRulesToRun = 0;								// 0 means run them all
		const char *pszExpression = flex_maxrule.GetString();
		if ( pszExpression )
		{
			nFlexRulesToRun = atoi(pszExpression);				// 0 will be returned if not a numeric string
		}
		// end JasonM hack
//*/
		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i + 1, 0, "%2d:%d\n", i, prule->flex );

		for (int j = 0; j < prule->numops; j++)
		{
			// Generic precondition for an op - will assert and break from the switch, turning the op into a no-op
			#define CHECK(expr) { if ( !(expr) ) { AssertMsg(false, "Invalid flex rules in model"); break; } };

			// Check that the stack pointer is at least this many elements.  All ops that access previous values in the
			// stack must be annotated with this.
			#define CHECK_STACK_DEPTH(min)       CHECK( k >= min );
			// Check that we can move the stack forward this many units.  All ops that increment the stack must be
			// annotated with this.
			//
			// !! You must CHECK_STACK_SPACE(1) before accessing/assigning stack[k] -- k is the *next* stack location,
			//    and may be one past the end of the array if the stack is full.
			#define CHECK_STACK_SPACE(num)       CHECK( k <= STUDIO_FLEX_STACK - num );
			// Check that the value is a valid controller index.
			#define CHECK_VALID_CONTROLLER_INDEX(idx) CHECK( idx >= 0 && idx < numflexcontrollers() );
			// Check that the value is a valid descriptor index, for access into dest[]
			#define CHECK_VALID_DESCRIPTOR_INDEX(idx) CHECK( idx >= 0 && idx < numflexdesc() );

			switch (pops->op)
			{
			case STUDIO_ADD: CHECK_STACK_DEPTH(2); stack[k-2] = stack[k-2] + stack[k-1]; k--; break;
			case STUDIO_SUB: CHECK_STACK_DEPTH(2); stack[k-2] = stack[k-2] - stack[k-1]; k--; break;
			case STUDIO_MUL: CHECK_STACK_DEPTH(2); stack[k-2] = stack[k-2] * stack[k-1]; k--; break;
			case STUDIO_DIV:
				CHECK_STACK_DEPTH(2);
				if (stack[k-1] > 0.0001)
				{
					stack[k-2] = stack[k-2] / stack[k-1];
				}
				else
				{
					stack[k-2] = 0;
				}
				k--;
				break;
			case STUDIO_NEG: CHECK_STACK_DEPTH(1); stack[k-1] = -stack[k-1]; break;
			case STUDIO_MAX: CHECK_STACK_DEPTH(2); stack[k-2] = max( stack[k-2], stack[k-1] ); k--; break;
			case STUDIO_MIN: CHECK_STACK_DEPTH(2); stack[k-2] = min( stack[k-2], stack[k-1] ); k--; break;
				case STUDIO_CONST: CHECK_STACK_SPACE(1); stack[k] = pops->d.value; k++; break;
			case STUDIO_FETCH1:
				{
				CHECK_STACK_SPACE(1);
				CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
				int m = pFlexcontroller( (LocalFlexController_t)pops->d.index)->localToGlobal;
				stack[k] = src[m];
				k++;
				break;
				}
			case STUDIO_FETCH2:
				{
					CHECK_STACK_SPACE(1);
					CHECK_VALID_DESCRIPTOR_INDEX(pops->d.index);
					stack[k] = dest[pops->d.index]; k++; break;
				}
			case STUDIO_COMBO:
				{
					int m = pops->d.index;
					CHECK_VALID_CONTROLLER_INDEX(m);
					CHECK_STACK_DEPTH(m);
					if ( m == 0 ) { CHECK_STACK_SPACE(1); }

					int km = k - m;
					for ( int iStack = km + 1; iStack < k; ++iStack )
					{
						stack[ km ] *= stack[iStack];
					}
					k = k - m + 1;
				}
				break;
			case STUDIO_DOMINATE:
				{
					int m = pops->d.index;
					CHECK_VALID_CONTROLLER_INDEX(m);
					CHECK_STACK_DEPTH(m + 1);
					int km = k - m;
					float dv = stack[ km ];
					for ( int iStack = km + 1; iStack < k; ++iStack )
					{
						dv *= stack[iStack];
					}
					stack[ km - 1 ] *= 1.0f - dv;
					k -= m;
				}
				break;
			case STUDIO_2WAY_0:
				{
					CHECK_STACK_SPACE(1);
					CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
					int m = pFlexcontroller( (LocalFlexController_t)pops->d.index )->localToGlobal;
					stack[ k ] = RemapValClamped( src[m], -1.0f, 0.0f, 1.0f, 0.0f );
					k++;
				}
				break;
			case STUDIO_2WAY_1:
				{
					CHECK_STACK_SPACE(1);
					CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
					int m = pFlexcontroller( (LocalFlexController_t)pops->d.index )->localToGlobal;
					stack[ k ] = RemapValClamped( src[m], 0.0f, 1.0f, 0.0f, 1.0f );
					k++;
				}
				break;
			case STUDIO_NWAY:
				{
					CHECK_STACK_DEPTH(5);
					CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
					LocalFlexController_t valueControllerIndex = static_cast< LocalFlexController_t >( (int)stack[ k - 1 ] );
					CHECK_VALID_CONTROLLER_INDEX(valueControllerIndex);
					int m = pFlexcontroller( valueControllerIndex )->localToGlobal;
					float flValue = src[ m ];
					int v = pFlexcontroller( (LocalFlexController_t)pops->d.index )->localToGlobal;

					const Vector4D filterRamp( stack[ k - 5 ], stack[ k - 4 ], stack[ k - 3 ], stack[ k - 2 ] );

					// Apply multicontrol remapping
					if ( flValue <= filterRamp.x || flValue >= filterRamp.w )
					{
						flValue = 0.0f;
					}
					else if ( flValue < filterRamp.y )
					{
						flValue = RemapValClamped( flValue, filterRamp.x, filterRamp.y, 0.0f, 1.0f );
					}
					else if ( flValue > filterRamp.z )
					{
						flValue = RemapValClamped( flValue, filterRamp.z, filterRamp.w, 1.0f, 0.0f );
					}
					else
					{
						flValue = 1.0f;
					}

					stack[ k - 5 ] = flValue * src[ v ];

					k -= 4;
				}
				break;
			case STUDIO_DME_LOWER_EYELID:
				{
					CHECK_STACK_DEPTH(3);
					CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 1 ]);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 2 ]);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 3 ]);

					const mstudioflexcontroller_t *const pCloseLidV = pFlexcontroller( (LocalFlexController_t)pops->d.index );
					const float flCloseLidV = RemapValClamped( src[ pCloseLidV->localToGlobal ], pCloseLidV->min, pCloseLidV->max, 0.0f, 1.0f );
					const mstudioflexcontroller_t *const pCloseLid = pFlexcontroller( static_cast< LocalFlexController_t >( (int)stack[ k - 1 ] ) );
					const float flCloseLid = RemapValClamped( src[ pCloseLid->localToGlobal ], pCloseLid->min, pCloseLid->max, 0.0f, 1.0f );

					int nEyeUpDownIndex = static_cast< int >( stack[ k - 3 ] );
					float flEyeUpDown = 0.0f;
					if ( nEyeUpDownIndex >= 0 )
					{
						const mstudioflexcontroller_t *const pEyeUpDown = pFlexcontroller( static_cast< LocalFlexController_t >( (int)stack[ k - 3 ] ) );
						flEyeUpDown = RemapValClamped( src[ pEyeUpDown->localToGlobal ], pEyeUpDown->min, pEyeUpDown->max, -1.0f, 1.0f );
					}

					if ( flEyeUpDown > 0.0 )
					{
						stack [ k - 3 ] = ( 1.0f - flEyeUpDown ) * ( 1.0f - flCloseLidV ) * flCloseLid;
					}
					else
					{
						stack [ k - 3 ] = ( 1.0f - flCloseLidV ) * flCloseLid;
					}
					k -= 2;
				}
				break;
			case STUDIO_DME_UPPER_EYELID:
				{
					CHECK_STACK_DEPTH(3);
					CHECK_VALID_CONTROLLER_INDEX(pops->d.index);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 1 ]);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 2 ]);
					CHECK_VALID_CONTROLLER_INDEX((int)stack[ k - 3 ]);
					const mstudioflexcontroller_t *const pCloseLidV = pFlexcontroller( (LocalFlexController_t)pops->d.index );
					const float flCloseLidV = RemapValClamped( src[ pCloseLidV->localToGlobal ], pCloseLidV->min, pCloseLidV->max, 0.0f, 1.0f );

					const mstudioflexcontroller_t *const pCloseLid = pFlexcontroller( static_cast< LocalFlexController_t >( (int)stack[ k - 1 ] ) );
					const float flCloseLid = RemapValClamped( src[ pCloseLid->localToGlobal ], pCloseLid->min, pCloseLid->max, 0.0f, 1.0f );

					int nEyeUpDownIndex = static_cast< int >( stack[ k - 3 ] );
					float flEyeUpDown = 0.0f;
					if ( nEyeUpDownIndex >= 0 )
					{
						const mstudioflexcontroller_t *const pEyeUpDown = pFlexcontroller( static_cast< LocalFlexController_t >( (int)stack[ k - 3 ] ) );
						flEyeUpDown = RemapValClamped( src[ pEyeUpDown->localToGlobal ], pEyeUpDown->min, pEyeUpDown->max, -1.0f, 1.0f );
					}

					if ( flEyeUpDown < 0.0f )
					{
						stack [ k - 3 ] = ( 1.0f + flEyeUpDown ) * flCloseLidV * flCloseLid;
					}
					else
					{
						stack [ k - 3 ] = flCloseLidV * flCloseLid;
					}
					k -= 2;
				}
				break;
			}
			#undef CHECK
			#undef CHECK_STACK_DEPTH
			#undef CHECK_STACK_SPACE
			#undef CHECK_VALID_CONTROLLER_INDEX

			pops++;
		}

		dest[prule->flex] = stack[0];
/*
		// JasonM hack
		if ( nFlexRulesToRun == 0)					// 0 means run all rules correctly
		{
			dest[prule->flex] = stack[0];
		}
		else // run only up to nFlexRulesToRun correctly...zero out the rest
		{
			if ( j < nFlexRulesToRun )
				dest[prule->flex] = stack[0];
			else
				dest[prule->flex] = 0.0f;
		}

		dest[prule->flex] = 1.0f;
//*/
		// end JasonM hack

	}
}



//-----------------------------------------------------------------------------
//	CODE PERTAINING TO ACTIVITY->SEQUENCE MAPPING SUBCLASS
//-----------------------------------------------------------------------------
#define iabs(i) (( (i) >= 0 ) ? (i) : -(i) )

CUtlSymbolTable g_ActivityModifiersTable;

extern void SetActivityForSequence( CStudioHdr *pstudiohdr, int i );
void CStudioHdr::CActivityToSequenceMapping::Initialize( CStudioHdr * __restrict pstudiohdr )
{
	// Algorithm: walk through every sequence in the model, determine to which activity
	// it corresponds, and keep a count of sequences per activity. Once the total count
	// is available, allocate an array large enough to contain them all, update the 
	// starting indices for every activity's section in the array, and go back through,
	// populating the array with its data.

	AssertMsg1( m_pSequenceTuples == NULL, "Tried to double-initialize sequence mapping for %s", pstudiohdr->pszName() );
	if ( m_pSequenceTuples != NULL )
		return; // don't double initialize.

	SetValidationPair(pstudiohdr);

	if ( ! pstudiohdr->SequencesAvailable() )
		return; // nothing to do.

#if STUDIO_SEQUENCE_ACTIVITY_LAZY_INITIALIZE
	m_bIsInitialized = true;
#endif
	
	// Some studio headers have no activities at all. In those
	// cases we can avoid a lot of this effort.
	bool bFoundOne = false;	

	// for each sequence in the header...
	const int NumSeq = pstudiohdr->GetNumSeq();
	for ( int i = 0 ; i < NumSeq ; ++i )
	{
		const mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
#if defined(SERVER_DLL) || defined(CLIENT_DLL) || defined(GAME_DLL)
		if (!(seqdesc.flags & STUDIO_ACTIVITY))
		{
			// AssertMsg2( false, "Sequence %d on studiohdr %s didn't have its activity initialized!", i, pstudiohdr->pszName() );
			SetActivityForSequence( pstudiohdr, i );
		}
#endif

		// is there an activity associated with this sequence?
		if (seqdesc.activity >= 0)
		{
			bFoundOne = true;

			// look up if we already have an entry. First we need to make a speculative one --
			HashValueType entry(seqdesc.activity, 0, 1, iabs(seqdesc.actweight));
			UtlHashHandle_t handle = m_ActToSeqHash.Find(entry);
			if ( m_ActToSeqHash.IsValidHandle(handle) )
			{	
				// we already have an entry and must update it by incrementing count
				HashValueType * __restrict toUpdate = &m_ActToSeqHash.Element(handle);
				toUpdate->count += 1;
				toUpdate->totalWeight += iabs(seqdesc.actweight);
				if ( !HushAsserts() )
				{
					AssertMsg( toUpdate->totalWeight > 0, "toUpdate->totalWeight: %d", toUpdate->totalWeight );
				}
			}
			else
			{
				// we do not have an entry yet; create one.
				m_ActToSeqHash.Insert(entry);
			}
		}
	}

	// if we found nothing, don't bother with any other initialization!
	if (!bFoundOne)
		return;

	// Now, create starting indices for each activity. For an activity n, 
	// the starting index is of course the sum of counts [0..n-1]. 
	int sequenceCount = 0;
	int topActivity = 0; // this will store the highest seen activity number (used later to make an ad hoc map on the stack)
	for ( UtlHashHandle_t handle = m_ActToSeqHash.GetFirstHandle() ; 
		  m_ActToSeqHash.IsValidHandle(handle) ;
		  handle = m_ActToSeqHash.GetNextHandle(handle) )
	{
		HashValueType &element = m_ActToSeqHash[handle];
		element.startingIdx = sequenceCount;
		sequenceCount += element.count;
		topActivity = max(topActivity, element.activityIdx);
	}
	

	// Allocate the actual array of sequence information. Note the use of restrict;
	// this is an important optimization, but means that you must never refer to this
	// array through m_pSequenceTuples in the scope of this function.
	SequenceTuple * __restrict tupleList = new SequenceTuple[sequenceCount];
	m_pSequenceTuples = tupleList; // save it off -- NEVER USE m_pSequenceTuples in this function!
	m_iSequenceTuplesCount = sequenceCount;



	// Now we're going to actually populate that list with the relevant data. 
	// First, create an array on the stack to store how many sequences we've written
	// so far for each activity. (This is basically a very simple way of doing a map.)
	// This stack may potentially grow very large; so if you have problems with it, 
	// go to a utlmap or similar structure.
	unsigned int allocsize = (topActivity + 1) * sizeof(int);
#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) //  need macro for constant expression
	allocsize = ALIGN_VALUE(allocsize,16);
	int * __restrict seqsPerAct = static_cast<int *>(stackalloc(allocsize));
	memset(seqsPerAct, 0, allocsize);

	// okay, walk through all the sequences again, and write the relevant data into 
	// our little table.
	for ( int i = 0 ; i < NumSeq ; ++i )
	{
		const mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if (seqdesc.activity >= 0)
		{
			const HashValueType &element = m_ActToSeqHash[m_ActToSeqHash.Find(HashValueType(seqdesc.activity, 0, 0, 0))];
			
			// If this assert trips, we've written more sequences per activity than we allocated 
			// (therefore there must have been a miscount in the first for loop above).
			int tupleOffset = seqsPerAct[seqdesc.activity];
			Assert( tupleOffset < element.count );

			if ( seqdesc.numactivitymodifiers > 0 )
			{
				// add entries for this model's activity modifiers
				(tupleList + element.startingIdx + tupleOffset)->pActivityModifiers = new CUtlSymbol[ seqdesc.numactivitymodifiers ];
				(tupleList + element.startingIdx + tupleOffset)->iNumActivityModifiers = seqdesc.numactivitymodifiers;

				for ( int k = 0; k < seqdesc.numactivitymodifiers; k++ )
				{
					(tupleList + element.startingIdx + tupleOffset)->pActivityModifiers[ k ] = g_ActivityModifiersTable.AddString( seqdesc.pActivityModifier( k )->pszName() );
				}
			}
			else
			{
				(tupleList + element.startingIdx + tupleOffset)->pActivityModifiers = NULL;
				(tupleList + element.startingIdx + tupleOffset)->iNumActivityModifiers = 0;
			}

			// You might be tempted to collapse this pointer math into a single pointer --
			// don't! the tuple list is marked __restrict above.
			(tupleList + element.startingIdx + tupleOffset)->seqnum = i; // store sequence number
			(tupleList + element.startingIdx + tupleOffset)->weight = iabs(seqdesc.actweight);

			// We can't have weights of 0
			// Assert( (tupleList + element.startingIdx + tupleOffset)->weight > 0 );
			if ( (tupleList + element.startingIdx + tupleOffset)->weight == 0 )
			{
				(tupleList + element.startingIdx + tupleOffset)->weight = 1;
			}

			seqsPerAct[seqdesc.activity] += 1;
		}
	}

#ifdef DBGFLAG_ASSERT
	// double check that we wrote exactly the right number of sequences.
	unsigned int chkSequenceCount = 0;
	for (int j = 0 ; j <= topActivity ; ++j)
	{
		chkSequenceCount += seqsPerAct[j];
	}
	Assert(chkSequenceCount == m_iSequenceTuplesCount);
#endif

}

/// Force Initialize() to occur again, even if it has already occured.
void CStudioHdr::CActivityToSequenceMapping::Reinitialize( CStudioHdr *pstudiohdr )
{
	m_bIsInitialized = false;
	if (m_pSequenceTuples)
	{
		delete m_pSequenceTuples;
		m_pSequenceTuples = NULL;
	}
	m_ActToSeqHash.RemoveAll();

	Initialize(pstudiohdr);
}

// Look up relevant data for an activity's sequences. This isn't terribly efficient, due to the
// load-hit-store on the output parameters, so the most common case -- SelectWeightedSequence --
// is specially implemented.
const CStudioHdr::CActivityToSequenceMapping::SequenceTuple *CStudioHdr::CActivityToSequenceMapping::GetSequences( int forActivity, int * __restrict outSequenceCount, int * __restrict outTotalWeight )
{
	// Construct a dummy entry so we can do a hash lookup (the UtlHash does not divorce keys from values)

	HashValueType entry(forActivity, 0, 0, 0);
	UtlHashHandle_t handle = m_ActToSeqHash.Find(entry);
	
	if (m_ActToSeqHash.IsValidHandle(handle))
	{
		const HashValueType &element = m_ActToSeqHash[handle];
		const SequenceTuple *retval = m_pSequenceTuples + element.startingIdx;
		*outSequenceCount = element.count;
		*outTotalWeight = element.totalWeight;

		return retval;
	}
	else
	{
		// invalid handle; return NULL.
		// this is actually a legit use case, so no need to assert.
		return NULL;
	}
}

int CStudioHdr::CActivityToSequenceMapping::NumSequencesForActivity( int forActivity )
{
	// If this trips, you've called this function on something that doesn't 
	// have activities.
	//Assert(m_pSequenceTuples != NULL);
	if ( m_pSequenceTuples == NULL )
		return 0;

	HashValueType entry(forActivity, 0, 0, 0);
	UtlHashHandle_t handle = m_ActToSeqHash.Find(entry);
	if (m_ActToSeqHash.IsValidHandle(handle))
	{
		return m_ActToSeqHash[handle].count;
	}
	else
	{
		return 0;
	}
}

// double-check that the data I point to hasn't changed
bool CStudioHdr::CActivityToSequenceMapping::ValidateAgainst( const CStudioHdr * RESTRICT pstudiohdr ) RESTRICT
{
	if (m_bIsInitialized)
	{
		return m_expectedPStudioHdr == pstudiohdr->GetRenderHdr() &&
			   m_expectedVModel == pstudiohdr->GetVirtualModel();
	}
	else
	{
		return true; // Allow an ordinary initialization to take place without printing a panicky assert.
	}
}

void CStudioHdr::CActivityToSequenceMapping::SetValidationPair( const CStudioHdr *RESTRICT pstudiohdr ) RESTRICT
{
	m_expectedPStudioHdr = pstudiohdr->GetRenderHdr();
	m_expectedVModel = pstudiohdr->GetVirtualModel();
}
