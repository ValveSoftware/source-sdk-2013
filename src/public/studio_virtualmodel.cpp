//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <limits.h>
#include "studio.h"
#include "tier1/utlmap.h"
#include "tier1/utldict.h"
#include "tier1/utlbuffer.h"
#include "filesystem.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IFileSystem *		g_pFileSystem;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

// a string table to speed up searching for sequences in the current virtual model
struct modellookup_t
{
	CUtlDict<short,short> seqTable;
	CUtlDict<short,short> animTable;
};

static CUtlVector<modellookup_t> g_ModelLookup;
static int g_ModelLookupIndex = -1;

inline bool HasLookupTable()
{
	return g_ModelLookupIndex >= 0 ? true : false;
}

inline CUtlDict<short,short> *GetSeqTable()
{
	return &g_ModelLookup[g_ModelLookupIndex].seqTable;
}

inline CUtlDict<short,short> *GetAnimTable()
{
	return &g_ModelLookup[g_ModelLookupIndex].animTable;
}

class CModelLookupContext
{
public:
	CModelLookupContext(int group, const studiohdr_t *pStudioHdr);
	~CModelLookupContext();

private:
	int		m_lookupIndex;
};

CModelLookupContext::CModelLookupContext(int group, const studiohdr_t *pStudioHdr)
{
	m_lookupIndex = -1;
	if ( group == 0 && pStudioHdr->numincludemodels )
	{
		m_lookupIndex = g_ModelLookup.AddToTail();
		g_ModelLookupIndex = g_ModelLookup.Count()-1;
	}
}

CModelLookupContext::~CModelLookupContext()
{
	if ( m_lookupIndex >= 0 )
	{
		Assert(m_lookupIndex == (g_ModelLookup.Count()-1));
		g_ModelLookup.FastRemove(m_lookupIndex);
		g_ModelLookupIndex = g_ModelLookup.Count()-1;
	}
}

void virtualmodel_t::AppendModels( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );

	// build a search table if necesary
	CModelLookupContext ctx(group, pStudioHdr);

	AppendSequences( group, pStudioHdr );
	AppendAnimations( group, pStudioHdr );
	AppendBonemap( group, pStudioHdr );
	AppendAttachments( group, pStudioHdr );
	AppendPoseParameters( group, pStudioHdr );
	AppendNodes( group, pStudioHdr );
	AppendIKLocks( group, pStudioHdr );

	struct HandleAndHeader_t
	{
		void				*handle;
		const studiohdr_t	*pHdr;
	};
	HandleAndHeader_t list[64];

	// determine quantity of valid include models in one pass only
	// temporarily cache results off, otherwise FindModel() causes ref counting problems
	int j;
	int nValidIncludes = 0;
	for (j = 0; j < pStudioHdr->numincludemodels; j++)
	{
		// find model (increases ref count)
		void *tmp = NULL;
		const studiohdr_t *pTmpHdr = pStudioHdr->FindModel( &tmp, pStudioHdr->pModelGroup( j )->pszName() );
		if ( pTmpHdr )
		{
			if ( nValidIncludes >= ARRAYSIZE( list ) )
			{
				// would cause stack overflow
				Assert( 0 );
				break;
			}

			list[nValidIncludes].handle = tmp;
			list[nValidIncludes].pHdr = pTmpHdr;
			nValidIncludes++;
		}
	}

	if ( nValidIncludes )
	{
		m_group.EnsureCapacity( m_group.Count() + nValidIncludes );
		for (j = 0; j < nValidIncludes; j++)
		{
			MEM_ALLOC_CREDIT();
			int group = m_group.AddToTail();
			m_group[group].cache = list[j].handle;
			AppendModels( group, list[j].pHdr );
		}
	}

	UpdateAutoplaySequences( pStudioHdr );
}

void virtualmodel_t::AppendSequences( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_seq.Count();

	int j, k;

	MEM_ALLOC_CREDIT();

	CUtlVector< virtualsequence_t > seq;

	seq = m_seq;

	m_group[ group ].masterSeq.SetCount( pStudioHdr->numlocalseq );

	for (j = 0; j < pStudioHdr->numlocalseq; j++)
	{
		const mstudioseqdesc_t *seqdesc = pStudioHdr->pLocalSeqdesc( j );
		char *s1 = seqdesc->pszLabel();

		if ( HasLookupTable() )
		{
			k = numCheck;
			short index = GetSeqTable()->Find( s1 );
			if ( index != GetSeqTable()->InvalidIndex() )
			{
				k = GetSeqTable()->Element(index);
			}
		}
		else
		{
			for (k = 0; k < numCheck; k++)
			{
				const studiohdr_t *hdr = m_group[ seq[k].group ].GetStudioHdr();
				char *s2 = hdr->pLocalSeqdesc( seq[k].index )->pszLabel();
				if ( !stricmp( s1, s2 ) )
				{
					break;
				}
			}
		}
		// no duplication
		if (k == numCheck)
		{
			virtualsequence_t tmp;
			tmp.group = group;
			tmp.index = j;
			tmp.flags = seqdesc->flags;
			tmp.activity = seqdesc->activity;
			k = seq.AddToTail( tmp );
		}
		else if (m_group[ seq[k].group ].GetStudioHdr()->pLocalSeqdesc( seq[k].index )->flags & STUDIO_OVERRIDE)
		{
			// the one in memory is a forward declared sequence, override it
			virtualsequence_t tmp;
			tmp.group = group;
			tmp.index = j;
			tmp.flags = seqdesc->flags;
			tmp.activity = seqdesc->activity;
			seq[k] = tmp;
		}
		m_group[ group ].masterSeq[ j ] = k;
	}

	if ( HasLookupTable() )
	{
		for ( j = numCheck; j < seq.Count(); j++ )
		{
			const studiohdr_t *hdr = m_group[ seq[j].group ].GetStudioHdr();
			const char *s1 = hdr->pLocalSeqdesc( seq[j].index )->pszLabel();
			GetSeqTable()->Insert( s1, j );
		}
	}

	m_seq = seq;
}


void virtualmodel_t::UpdateAutoplaySequences( const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int autoplayCount = pStudioHdr->CountAutoplaySequences();
	m_autoplaySequences.SetCount( autoplayCount );
	pStudioHdr->CopyAutoplaySequences( m_autoplaySequences.Base(), autoplayCount );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void virtualmodel_t::AppendAnimations( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_anim.Count();

	CUtlVector< virtualgeneric_t > anim;
	anim = m_anim;

	MEM_ALLOC_CREDIT();

	int j, k;

	m_group[ group ].masterAnim.SetCount( pStudioHdr->numlocalanim );

	for (j = 0; j < pStudioHdr->numlocalanim; j++)
	{
		char *s1 = pStudioHdr->pLocalAnimdesc( j )->pszName();
		if ( HasLookupTable() )
		{
			k = numCheck;
			short index = GetAnimTable()->Find( s1 );
			if ( index != GetAnimTable()->InvalidIndex() )
			{
				k = GetAnimTable()->Element(index);
			}
		}
		else
		{
			for (k = 0; k < numCheck; k++)
			{
				char *s2 = m_group[ anim[k].group ].GetStudioHdr()->pLocalAnimdesc( anim[k].index )->pszName();
				if (stricmp( s1, s2 ) == 0)
				{
					break;
				}
			}
		}
		// no duplication
		if (k == numCheck)
		{
			virtualgeneric_t tmp;
			tmp.group = group;
			tmp.index = j;
			k = anim.AddToTail( tmp );
		}

		m_group[ group ].masterAnim[ j ] = k;
	}
	
	if ( HasLookupTable() )
	{
		for ( j = numCheck; j < anim.Count(); j++ )
		{
			const char *s1 = m_group[ anim[j].group ].GetStudioHdr()->pLocalAnimdesc( anim[j].index )->pszName();
			GetAnimTable()->Insert( s1, j );
		}
	}

	m_anim = anim;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void virtualmodel_t::AppendBonemap( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	MEM_ALLOC_CREDIT();

	const studiohdr_t *pBaseStudioHdr = m_group[ 0 ].GetStudioHdr( );

	m_group[ group ].boneMap.SetCount( pBaseStudioHdr->numbones );
	m_group[ group ].masterBone.SetCount( pStudioHdr->numbones );

	int j, k;

	if (group == 0)
	{
		for (j = 0; j < pStudioHdr->numbones; j++)
		{
			m_group[ group ].boneMap[ j ] = j;
			m_group[ group ].masterBone[ j ] = j;
		}
	}
	else
	{
		for (j = 0; j < pBaseStudioHdr->numbones; j++)
		{
			m_group[ group ].boneMap[ j ] = -1;
		}
		for (j = 0; j < pStudioHdr->numbones; j++)
		{
			// NOTE: studiohdr has a bone table - using the table is ~5% faster than this for alyx.mdl on a P4/3.2GHz
			for (k = 0; k < pBaseStudioHdr->numbones; k++)
			{
				if (stricmp( pStudioHdr->pBone( j )->pszName(), pBaseStudioHdr->pBone( k )->pszName() ) == 0)
				{
					break;
				}
			}
			if (k < pBaseStudioHdr->numbones)
			{
				m_group[ group ].masterBone[ j ] = k;
				m_group[ group ].boneMap[ k ] = j;

				// FIXME: these runtime messages don't display in hlmv
				if ((pStudioHdr->pBone( j )->parent == -1) || (pBaseStudioHdr->pBone( k )->parent == -1))
				{
					if ((pStudioHdr->pBone( j )->parent != -1) || (pBaseStudioHdr->pBone( k )->parent != -1))
					{
						Warning( "%s/%s : missmatched parent bones on \"%s\"\n", pBaseStudioHdr->pszName(), pStudioHdr->pszName(), pStudioHdr->pBone( j )->pszName() );
					}
				}
				else if (m_group[ group ].masterBone[ pStudioHdr->pBone( j )->parent ] != m_group[ 0 ].masterBone[ pBaseStudioHdr->pBone( k )->parent ])
				{
					Warning( "%s/%s : missmatched parent bones on \"%s\"\n", pBaseStudioHdr->pszName(), pStudioHdr->pszName(), pStudioHdr->pBone( j )->pszName() );
				}
			}
			else
			{
				m_group[ group ].masterBone[ j ] = -1;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void virtualmodel_t::AppendAttachments( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_attachment.Count();

	CUtlVector< virtualgeneric_t > attachment;
	attachment = m_attachment;

	MEM_ALLOC_CREDIT();

	int j, k, n;

	m_group[ group ].masterAttachment.SetCount( pStudioHdr->numlocalattachments );

	for (j = 0; j < pStudioHdr->numlocalattachments; j++)
	{

		n = m_group[ group ].masterBone[ pStudioHdr->pLocalAttachment( j )->localbone ];
		
		// skip if the attachments bone doesn't exist in the root model
		if (n == -1)
		{
			continue;
		}
		
		
		char *s1 = pStudioHdr->pLocalAttachment( j )->pszName();
		for (k = 0; k < numCheck; k++)
		{
			char *s2 = m_group[ attachment[k].group ].GetStudioHdr()->pLocalAttachment( attachment[k].index )->pszName();

			if (stricmp( s1, s2 ) == 0)
			{
				break;
			}
		}
		// no duplication
		if (k == numCheck)
		{
			virtualgeneric_t tmp;
			tmp.group = group;
			tmp.index = j;
			k = attachment.AddToTail( tmp );

			// make sure bone flags are set so attachment calculates
			if ((m_group[ 0 ].GetStudioHdr()->pBone( n )->flags & BONE_USED_BY_ATTACHMENT) == 0)
			{
				while (n != -1)
				{
					m_group[ 0 ].GetStudioHdr()->pBone( n )->flags |= BONE_USED_BY_ATTACHMENT;

					if (m_group[ 0 ].GetStudioHdr()->pLinearBones())
					{
						*m_group[ 0 ].GetStudioHdr()->pLinearBones()->pflags(n) |= BONE_USED_BY_ATTACHMENT;
					}

					n = m_group[ 0 ].GetStudioHdr()->pBone( n )->parent;
				}
				continue;
			}
		}

		m_group[ group ].masterAttachment[ j ] = k;
	}

	m_attachment = attachment;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void virtualmodel_t::AppendPoseParameters( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_pose.Count();

	CUtlVector< virtualgeneric_t > pose;
	pose = m_pose;

	MEM_ALLOC_CREDIT();

	int j, k;

	m_group[ group ].masterPose.SetCount( pStudioHdr->numlocalposeparameters );

	for (j = 0; j < pStudioHdr->numlocalposeparameters; j++)
	{
		char *s1 = pStudioHdr->pLocalPoseParameter( j )->pszName();
		for (k = 0; k < numCheck; k++)
		{
			char *s2 = m_group[ pose[k].group ].GetStudioHdr()->pLocalPoseParameter( pose[k].index )->pszName();

			if (stricmp( s1, s2 ) == 0)
			{
				break;
			}
		}
		if (k == numCheck)
		{
			// no duplication
			virtualgeneric_t tmp;
			tmp.group = group;
			tmp.index = j;
			k = pose.AddToTail( tmp );
		}
		else
		{
			// duplicate, reset start and end to fit full dynamic range
			mstudioposeparamdesc_t *pPose1 = pStudioHdr->pLocalPoseParameter( j );
			mstudioposeparamdesc_t *pPose2 = m_group[ pose[k].group ].GetStudioHdr()->pLocalPoseParameter( pose[k].index );
			float start =  min( pPose2->end, min( pPose1->end, min( pPose2->start, pPose1->start ) ) );
			float end =  max( pPose2->end, max( pPose1->end, max( pPose2->start, pPose1->start ) ) );
			pPose2->start = start;
			pPose2->end = end;
		}

		m_group[ group ].masterPose[ j ] = k;
	}

	m_pose = pose;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void virtualmodel_t::AppendNodes( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_node.Count();

	CUtlVector< virtualgeneric_t > node;
	node = m_node;

	MEM_ALLOC_CREDIT();

	int j, k;

	m_group[ group ].masterNode.SetCount( pStudioHdr->numlocalnodes );

	for (j = 0; j < pStudioHdr->numlocalnodes; j++)
	{
		char *s1 = pStudioHdr->pszLocalNodeName( j );
		for (k = 0; k < numCheck; k++)
		{
			char *s2 = m_group[ node[k].group ].GetStudioHdr()->pszLocalNodeName( node[k].index );

			if (stricmp( s1, s2 ) == 0)
			{
				break;
			}
		}
		// no duplication
		if (k == numCheck)
		{
			virtualgeneric_t tmp;
			tmp.group = group;
			tmp.index = j;
			k = node.AddToTail( tmp );
		}

		m_group[ group ].masterNode[ j ] = k;
	}

	m_node = node;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


void virtualmodel_t::AppendIKLocks( int group, const studiohdr_t *pStudioHdr )
{
	AUTO_LOCK( m_Lock );
	int numCheck = m_iklock.Count();

	CUtlVector< virtualgeneric_t > iklock;
	iklock = m_iklock;

	int j, k;

	for (j = 0; j < pStudioHdr->numlocalikautoplaylocks; j++)
	{
		int chain1 = pStudioHdr->pLocalIKAutoplayLock( j )->chain;
		for (k = 0; k < numCheck; k++)
		{
			int chain2 = m_group[ iklock[k].group ].GetStudioHdr()->pLocalIKAutoplayLock( iklock[k].index )->chain;

			if (chain1 == chain2)
			{
				break;
			}
		}
		// no duplication
		if (k == numCheck)
		{
			MEM_ALLOC_CREDIT();

			virtualgeneric_t tmp;
			tmp.group = group;
			tmp.index = j;
			k = iklock.AddToTail( tmp );
		}
	}

	m_iklock = iklock;

	// copy knee directions for uninitialized knees
	if ( group != 0 )
	{
		studiohdr_t *pBaseHdr = (studiohdr_t *)m_group[ 0 ].GetStudioHdr();
		if ( pStudioHdr->numikchains == pBaseHdr->numikchains )
		{
			for (j = 0; j < pStudioHdr->numikchains; j++)
			{
				if ( pBaseHdr->pIKChain( j )->pLink(0)->kneeDir.LengthSqr() == 0.0f )
				{
					if ( pStudioHdr->pIKChain( j )->pLink(0)->kneeDir.LengthSqr() > 0.0f )
					{
						pBaseHdr->pIKChain( j )->pLink(0)->kneeDir = pStudioHdr->pIKChain( j )->pLink(0)->kneeDir;
					}
				}
			}
		}
	}
}
