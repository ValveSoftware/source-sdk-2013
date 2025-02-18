//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "bone_setup.h"
#include <string.h>

#include "collisionutils.h"
#include "vstdlib/random.h"
#include "tier0/vprof.h"
#include "bone_accessor.h"
#include "mathlib/ssequaternion.h"
#include "bitvec.h"
#include "datamanager.h"
#include "convar.h"
#include "tier0/tslist.h"
#include "vphysics_interface.h"
#ifdef CLIENT_DLL
	#include "posedebugger.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CBoneSetup
{
public:
	CBoneSetup( const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger = NULL );
	void InitPose( Vector pos[], Quaternion q[] );
	void AccumulatePose( Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext );
	void CalcAutoplaySequences(	Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext );
private:
	void AddSequenceLayers( Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext );
	void AddLocalLayers( Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext );
public:
	const CStudioHdr *m_pStudioHdr;
	int m_boneMask;
	const float *m_flPoseParameter;
	IPoseDebugger *m_pPoseDebugger;
};

// -----------------------------------------------------------------
template <typename T>
class CBoneSetupMemoryPool
{
public:
	T *Alloc()
	{
		T *p = (T *)m_FreeBlocks.Pop();
		if ( !p )
		{
			p = new T[MAXSTUDIOBONES];
			if ( ((size_t)p) % TSLIST_NODE_ALIGNMENT != 0 )
			{
				DebuggerBreak();
			}
		}

		return p;
	}

	void Free( T *p )
	{
		m_FreeBlocks.Push( (TSLNodeBase_t *)p );
	}

private:
	CTSListBase m_FreeBlocks;
};

CBoneSetupMemoryPool<Quaternion> g_QaternionPool;
CBoneSetupMemoryPool<Vector> g_VectorPool;
CBoneSetupMemoryPool<matrix3x4_t> g_MatrixPool;

// -----------------------------------------------------------------
CBoneCache *CBoneCache::CreateResource( const bonecacheparams_t &params )
{
	short studioToCachedIndex[MAXSTUDIOBONES];
	short cachedToStudioIndex[MAXSTUDIOBONES];
	int cachedBoneCount = 0;
	for ( int i = 0; i < params.pStudioHdr->numbones(); i++ )
	{
		// skip bones that aren't part of the boneMask (and aren't the root bone)
		if (i != 0 && !(params.pStudioHdr->boneFlags(i) & params.boneMask))
		{
			studioToCachedIndex[i] = -1;
			continue;
		}
		studioToCachedIndex[i] = cachedBoneCount;
		cachedToStudioIndex[cachedBoneCount] = i;
		cachedBoneCount++;
	}
	int tableSizeStudio = sizeof(short) * params.pStudioHdr->numbones();
	int tableSizeCached = sizeof(short) * cachedBoneCount;
	int matrixSize = sizeof(matrix3x4_t) * cachedBoneCount;
	int size = ( sizeof(CBoneCache) + tableSizeStudio + tableSizeCached + matrixSize + 3 ) & ~3;
	
	CBoneCache *pMem = (CBoneCache *)malloc( size );
	Construct( pMem );
	pMem->Init( params, size, studioToCachedIndex, cachedToStudioIndex, cachedBoneCount );
	return pMem;
}

unsigned int CBoneCache::EstimatedSize( const bonecacheparams_t &params )
{
	// conservative estimate - max size
	return ( params.pStudioHdr->numbones() * (sizeof(short) + sizeof(short) + sizeof(matrix3x4_t)) + 3 ) & ~3;
}

void CBoneCache::DestroyResource()
{
	free( this );
}


CBoneCache::CBoneCache()
{
	m_size = 0;
	m_cachedBoneCount = 0;
}

void CBoneCache::Init( const bonecacheparams_t &params, unsigned int size, short *pStudioToCached, short *pCachedToStudio, int cachedBoneCount ) 
{
	m_cachedBoneCount = cachedBoneCount;
	m_size = size;
	m_timeValid = params.curtime;
	m_boneMask = params.boneMask;

	int studioTableSize = params.pStudioHdr->numbones() * sizeof(short);
	m_cachedToStudioOffset = studioTableSize;
	memcpy( StudioToCached(), pStudioToCached, studioTableSize );

	int cachedTableSize = cachedBoneCount * sizeof(short);
	memcpy( CachedToStudio(), pCachedToStudio, cachedTableSize );

	m_matrixOffset = ( m_cachedToStudioOffset + cachedTableSize + 3 ) & ~3;
	
	UpdateBones( params.pBoneToWorld, params.pStudioHdr->numbones(), params.curtime );
}

void CBoneCache::UpdateBones( const matrix3x4_t *pBoneToWorld, int numbones, float curtime )
{
	matrix3x4_t *pBones = BoneArray();
	const short *pCachedToStudio = CachedToStudio();

	for ( int i = 0; i < m_cachedBoneCount; i++ )
	{
		int index = pCachedToStudio[i];
		MatrixCopy( pBoneToWorld[index], pBones[i] );
	}
	m_timeValid = curtime;
}

matrix3x4_t *CBoneCache::GetCachedBone( int studioIndex )
{
	int cachedIndex = StudioToCached()[studioIndex];
	if ( cachedIndex >= 0 )
	{
		return BoneArray() + cachedIndex;
	}
	return NULL;
}

void CBoneCache::ReadCachedBones( matrix3x4_t *pBoneToWorld )
{
	matrix3x4_t *pBones = BoneArray();
	const short *pCachedToStudio = CachedToStudio();
	for ( int i = 0; i < m_cachedBoneCount; i++ )
	{
		MatrixCopy( pBones[i], pBoneToWorld[pCachedToStudio[i]] );
	}
}

void CBoneCache::ReadCachedBonePointers( matrix3x4_t **bones, int numbones )
{
	memset( bones, 0, sizeof(matrix3x4_t *) * numbones );
	matrix3x4_t *pBones = BoneArray();
	const short *pCachedToStudio = CachedToStudio();
	for ( int i = 0; i < m_cachedBoneCount; i++ )
	{
		bones[pCachedToStudio[i]] = pBones + i;
	}
}

bool CBoneCache::IsValid( float curtime, float dt )
{
	if ( curtime - m_timeValid <= dt )
		return true;
	return false;
}


// private functions
matrix3x4_t *CBoneCache::BoneArray()
{
	return (matrix3x4_t *)( (char *)(this+1) + m_matrixOffset );
}

short *CBoneCache::StudioToCached()
{
	return (short *)( (char *)(this+1) );
}

short *CBoneCache::CachedToStudio()
{
	return (short *)( (char *)(this+1) + m_cachedToStudioOffset );
}

// Construct a singleton
static CDataManager<CBoneCache, bonecacheparams_t, CBoneCache *, CThreadFastMutex> g_StudioBoneCache( 128 * 1024L );

CBoneCache *Studio_GetBoneCache( memhandle_t cacheHandle )
{
	AUTO_LOCK( g_StudioBoneCache.AccessMutex() );
	return g_StudioBoneCache.GetResource_NoLock( cacheHandle );
}

memhandle_t Studio_CreateBoneCache( bonecacheparams_t &params )
{
	AUTO_LOCK( g_StudioBoneCache.AccessMutex() );
	return g_StudioBoneCache.CreateResource( params );
}

void Studio_DestroyBoneCache( memhandle_t cacheHandle )
{
	AUTO_LOCK( g_StudioBoneCache.AccessMutex() );
	g_StudioBoneCache.DestroyResource( cacheHandle );
}

void Studio_InvalidateBoneCache( memhandle_t cacheHandle )
{
	AUTO_LOCK( g_StudioBoneCache.AccessMutex() );
	CBoneCache *pCache = g_StudioBoneCache.GetResource_NoLock( cacheHandle );
	if ( pCache )
	{
		pCache->m_timeValid = -1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void BuildBoneChain(
	const CStudioHdr *pStudioHdr,
	const matrix3x4_t &rootxform,
	const Vector pos[], 
	const Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld )
{
	CBoneBitList boneComputed;
	BuildBoneChain( pStudioHdr, rootxform, pos, q, iBone, pBoneToWorld, boneComputed );
	return;
}


//-----------------------------------------------------------------------------
// Purpose: return a sub frame rotation for a single bone
//-----------------------------------------------------------------------------
void ExtractAnimValue( int frame, mstudioanimvalue_t *panimvalue, float scale, float &v1, float &v2 )
{
	if ( !panimvalue )
	{
		v1 = v2 = 0;
		return;
	}

	// Avoids a crash reading off the end of the data
	// There is probably a better long-term solution; Ken is going to look into it.
	if ( ( panimvalue->num.total == 1 ) && ( panimvalue->num.valid == 1 ) )
	{
		v1 = v2 = panimvalue[1].value * scale;
		return;
	}

	int k = frame;

	// find the data list that has the frame
	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if ( panimvalue->num.total == 0 )
		{
			Assert( 0 ); // running off the end of the animation stream is bad
			v1 = v2 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		// has valid animation data
		v1 = panimvalue[k+1].value * scale;

		if (panimvalue->num.valid > k + 1)
		{
			// has valid animation blend data
			v2 = panimvalue[k+2].value * scale;
		}
		else
		{
			if (panimvalue->num.total > k + 1)
			{
				// data repeats, no blend
				v2 = v1;
			}
			else
			{
				// pull blend from first data block in next list
				v2 = panimvalue[panimvalue->num.valid+2].value * scale;
			}
		}
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
		if (panimvalue->num.total > k + 1)
		{
			// data repeats, no blend
			v2 = v1;
		}
		else
		{
			// pull blend from first data block in next list
			v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
		}
	}
}


void ExtractAnimValue( int frame, mstudioanimvalue_t *panimvalue, float scale, float &v1 )
{
	if ( !panimvalue )
	{
		v1 = 0;
		return;
	}

	int k = frame;

	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if ( panimvalue->num.total == 0 )
		{
			Assert( 0 ); // running off the end of the animation stream is bad
			v1 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		v1 = panimvalue[k+1].value * scale;
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: return a sub frame rotation for a single bone
//-----------------------------------------------------------------------------
void CalcBoneQuaternion( int frame, float s, 
						const Quaternion &baseQuat, const RadianEuler &baseRot, const Vector &baseRotScale, 
						int iBaseFlags, const Quaternion &baseAlignment, 
						const mstudioanim_t *panim, Quaternion &q )
{
	if ( panim->flags & STUDIO_ANIM_RAWROT )
	{
		q = *(panim->pQuat48());
		Assert( q.IsValid() );
		return;
	} 
	
	if ( panim->flags & STUDIO_ANIM_RAWROT2 )
	{
		q = *(panim->pQuat64());
		Assert( q.IsValid() );
		return;
	}

	if ( !(panim->flags & STUDIO_ANIM_ANIMROT) )
	{
		if (panim->flags & STUDIO_ANIM_DELTA)
		{
			q.Init( 0.0f, 0.0f, 0.0f, 1.0f );
		}
		else
		{
			q = baseQuat;
		}
		return;
	}

	mstudioanim_valueptr_t *pValuesPtr = panim->pRotV();

	if (s > 0.001f)
	{
		QuaternionAligned	q1, q2;
		RadianEuler			angle1, angle2;

		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 0 ), baseRotScale.x, angle1.x, angle2.x );
		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 1 ), baseRotScale.y, angle1.y, angle2.y );
		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 2 ), baseRotScale.z, angle1.z, angle2.z );

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle1.x = angle1.x + baseRot.x;
			angle1.y = angle1.y + baseRot.y;
			angle1.z = angle1.z + baseRot.z;
			angle2.x = angle2.x + baseRot.x;
			angle2.y = angle2.y + baseRot.y;
			angle2.z = angle2.z + baseRot.z;
		}

		Assert( angle1.IsValid() && angle2.IsValid() );
		if (angle1.x != angle2.x || angle1.y != angle2.y || angle1.z != angle2.z)
		{
			AngleQuaternion( angle1, q1 );
			AngleQuaternion( angle2, q2 );

	#ifdef _X360
			fltx4 q1simd, q2simd, qsimd;
			q1simd = LoadAlignedSIMD( q1 );
			q2simd = LoadAlignedSIMD( q2 );
			qsimd = QuaternionBlendSIMD( q1simd, q2simd, s );
			StoreUnalignedSIMD( q.Base(), qsimd );
	#else
			QuaternionBlend( q1, q2, s, q );
	#endif
		}
		else
		{
			AngleQuaternion( angle1, q );
		}
	}
	else
	{
		RadianEuler			angle;

		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 0 ), baseRotScale.x, angle.x );
		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 1 ), baseRotScale.y, angle.y );
		ExtractAnimValue( frame, pValuesPtr->pAnimvalue( 2 ), baseRotScale.z, angle.z );

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle.x = angle.x + baseRot.x;
			angle.y = angle.y + baseRot.y;
			angle.z = angle.z + baseRot.z;
		}

		Assert( angle.IsValid() );
		AngleQuaternion( angle, q );
	}

	Assert( q.IsValid() );

	// align to unified bone
	if (!(panim->flags & STUDIO_ANIM_DELTA) && (iBaseFlags & BONE_FIXED_ALIGNMENT))
	{
		QuaternionAlign( baseAlignment, q, q );
	}
}

inline void CalcBoneQuaternion( int frame, float s, 
						const mstudiobone_t *pBone,
						const mstudiolinearbone_t *pLinearBones,
						const mstudioanim_t *panim, Quaternion &q )
{
	if (pLinearBones)
	{
		CalcBoneQuaternion( frame, s, pLinearBones->quat(panim->bone), pLinearBones->rot(panim->bone), pLinearBones->rotscale(panim->bone), pLinearBones->flags(panim->bone), pLinearBones->qalignment(panim->bone), panim, q );
	}
	else
	{
		CalcBoneQuaternion( frame, s, pBone->quat, pBone->rot, pBone->rotscale, pBone->flags, pBone->qAlignment, panim, q );
	}
}

						



//-----------------------------------------------------------------------------
// Purpose: return a sub frame position for a single bone
//-----------------------------------------------------------------------------
void CalcBonePosition(	int frame, float s,
						const Vector &basePos, const Vector &baseBoneScale, 
						const mstudioanim_t *panim, Vector &pos	)
{
	if (panim->flags & STUDIO_ANIM_RAWPOS)
	{
		pos = *(panim->pPos());
		Assert( pos.IsValid() );

		return;
	}
	else if (!(panim->flags & STUDIO_ANIM_ANIMPOS))
	{
		if (panim->flags & STUDIO_ANIM_DELTA)
		{
			pos.Init( 0.0f, 0.0f, 0.0f );
		}
		else
		{
			pos = basePos;
		}
		return;
	}

	mstudioanim_valueptr_t *pPosV = panim->pPosV();
	int					j;

	if (s > 0.001f)
	{
		float v1, v2;
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue( frame, pPosV->pAnimvalue( j ), baseBoneScale[j], v1, v2 );
			pos[j] = v1 * (1.0 - s) + v2 * s;
		}
	}
	else
	{
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue( frame, pPosV->pAnimvalue( j ), baseBoneScale[j], pos[j] );
		}
	}

	if (!(panim->flags & STUDIO_ANIM_DELTA))
	{
		pos.x = pos.x + basePos.x;
		pos.y = pos.y + basePos.y;
		pos.z = pos.z + basePos.z;
	}

	Assert( pos.IsValid() );
}


inline void CalcBonePosition( int frame, float s, 
						const mstudiobone_t *pBone,
						const mstudiolinearbone_t *pLinearBones,
						const mstudioanim_t *panim, Vector &pos )
{
	if (pLinearBones)
	{
		CalcBonePosition( frame, s, pLinearBones->pos(panim->bone), pLinearBones->posscale(panim->bone), panim, pos );
	}
	else
	{
		CalcBonePosition( frame, s, pBone->pos, pBone->posscale, panim, pos );
	}
}



void SetupSingleBoneMatrix( 
	CStudioHdr *pOwnerHdr, 
	int nSequence, 
	int iFrame,
	int iBone, 
	matrix3x4_t &mBoneLocal )
{
	mstudioseqdesc_t &seqdesc = pOwnerHdr->pSeqdesc( nSequence );
	mstudioanimdesc_t &animdesc = pOwnerHdr->pAnimdesc( seqdesc.anim( 0, 0 ) );
	int iLocalFrame = iFrame;
	mstudioanim_t *panim = animdesc.pAnim( &iLocalFrame );
	float s = 0;
	mstudiobone_t *pbone = pOwnerHdr->pBone( iBone );

	Quaternion boneQuat;
	Vector bonePos;

	// search for bone
	while (panim && panim->bone != iBone)
	{
		panim = panim->pNext();
	}

	// look up animation if found, if not, initialize
	if (panim && seqdesc.weight(iBone) > 0)
	{
		CalcBoneQuaternion( iLocalFrame, s, pbone, NULL, panim, boneQuat );
		CalcBonePosition  ( iLocalFrame, s, pbone, NULL, panim, bonePos );
	}
	else if (animdesc.flags & STUDIO_DELTA)
	{
		boneQuat.Init( 0.0f, 0.0f, 0.0f, 1.0f );
		bonePos.Init( 0.0f, 0.0f, 0.0f );
	}
	else
	{
		boneQuat = pbone->quat;
		bonePos = pbone->pos;
	}

	QuaternionMatrix( boneQuat, bonePos, mBoneLocal );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

static void CalcDecompressedAnimation( const mstudiocompressedikerror_t *pCompressed, int iFrame, float fraq, Vector &pos, Quaternion &q )
{
	if (fraq > 0.0001f)
	{
		Vector p1, p2;
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 0 ), pCompressed->scale[0], p1.x, p2.x );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 1 ), pCompressed->scale[1], p1.y, p2.y );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 2 ), pCompressed->scale[2], p1.z, p2.z );
		pos = p1 * (1 - fraq) + p2 * fraq;

		Quaternion			q1, q2;
		RadianEuler			angle1, angle2;
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 3 ), pCompressed->scale[3], angle1.x, angle2.x );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 4 ), pCompressed->scale[4], angle1.y, angle2.y );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 5 ), pCompressed->scale[5], angle1.z, angle2.z );

		if (angle1.x != angle2.x || angle1.y != angle2.y || angle1.z != angle2.z)
		{
			AngleQuaternion( angle1, q1 );
			AngleQuaternion( angle2, q2 );
			QuaternionBlend( q1, q2, fraq, q );
		}
		else
		{
			AngleQuaternion( angle1, q );
		}
	}
	else
	{
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 0 ), pCompressed->scale[0], pos.x );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 1 ), pCompressed->scale[1], pos.y );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 2 ), pCompressed->scale[2], pos.z );

		RadianEuler			angle;
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 3 ), pCompressed->scale[3], angle.x );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 4 ), pCompressed->scale[4], angle.y );
		ExtractAnimValue( iFrame, pCompressed->pAnimvalue( 5 ), pCompressed->scale[5], angle.z );

		AngleQuaternion( angle, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose: translate animations done in a non-standard parent space
//-----------------------------------------------------------------------------
static void CalcLocalHierarchyAnimation( 
	const CStudioHdr *pStudioHdr,
	matrix3x4_t *boneToWorld,
	CBoneBitList &boneComputed,
	Vector *pos, 
	Quaternion *q,
	//const mstudioanimdesc_t &animdesc,
	const mstudiobone_t *pbone,
	mstudiolocalhierarchy_t *pHierarchy,
	int iBone,
	int iNewParent,
	float cycle, 
	int iFrame,
	float flFraq,
	int boneMask
	)
{

	Vector localPos;
	Quaternion localQ;

	// make fake root transform
	static ALIGN16 matrix3x4_t rootXform ALIGN16_POST ( 1.0f, 0, 0, 0,   0, 1.0f, 0, 0,  0, 0, 1.0f, 0 );

	// FIXME: missing check to see if seq has a weight for this bone
	float weight = 1.0f;

	// check to see if there's a ramp on the influence
	if ( pHierarchy->tail - pHierarchy->peak < 1.0f  )
	{
		float index = cycle;

		if (pHierarchy->end > 1.0f && index < pHierarchy->start)
			index += 1.0f;

		if (index < pHierarchy->start)
			return;
		if (index >= pHierarchy->end)
			return;

		if (index < pHierarchy->peak && pHierarchy->start != pHierarchy->peak)
		{
			weight = (index - pHierarchy->start) / (pHierarchy->peak - pHierarchy->start);
		}
		else if (index > pHierarchy->tail && pHierarchy->end != pHierarchy->tail)
		{
			weight = (pHierarchy->end - index) / (pHierarchy->end - pHierarchy->tail);
		}

		weight = SimpleSpline( weight );
	}

	CalcDecompressedAnimation( pHierarchy->pLocalAnim(), iFrame - pHierarchy->iStart, flFraq, localPos, localQ );

	BuildBoneChain( pStudioHdr, rootXform, pos, q, iBone, boneToWorld, boneComputed );

	matrix3x4_t localXform;
	AngleMatrix( localQ, localPos, localXform );

	if ( iNewParent != -1 )
	{
		BuildBoneChain( pStudioHdr, rootXform, pos, q, iNewParent, boneToWorld, boneComputed );
		ConcatTransforms( boneToWorld[iNewParent], localXform, boneToWorld[iBone] );
	}
	else
	{
		boneToWorld[iBone] = localXform;
	}

	// back solve
	Vector p1;
	Quaternion q1;
	int n = pbone[iBone].parent;
	if (n == -1)
	{
		if (weight == 1.0f)
		{
			MatrixAngles( boneToWorld[iBone], q[iBone], pos[iBone] );
		}
		else
		{
			MatrixAngles( boneToWorld[iBone], q1, p1 );
			QuaternionSlerp( q[iBone], q1, weight, q[iBone] );
			pos[iBone] = Lerp( weight, p1, pos[iBone] );
		}
	}
	else
	{
		matrix3x4_t worldToBone;
		MatrixInvert( boneToWorld[n], worldToBone );

		matrix3x4_t local;
		ConcatTransforms( worldToBone, boneToWorld[iBone], local );
		if (weight == 1.0f)
		{
			MatrixAngles( local, q[iBone], pos[iBone] );
		}
		else
		{
			MatrixAngles( local, q1, p1 );
			QuaternionSlerp( q[iBone], q1, weight, q[iBone] );
			pos[iBone] = Lerp( weight, p1, pos[iBone] );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Calc Zeroframe Data
//-----------------------------------------------------------------------------

static void CalcZeroframeData( const CStudioHdr *pStudioHdr, const studiohdr_t *pAnimStudioHdr, const virtualgroup_t *pAnimGroup, const mstudiobone_t *pAnimbone, mstudioanimdesc_t &animdesc, float fFrame, Vector *pos, Quaternion *q, int boneMask, float flWeight )
{
	byte *pData = animdesc.pZeroFrameData();

	if (!pData)
		return;

	int i, j;

	// Msg("zeroframe %s\n", animdesc.pszName() );
	if (animdesc.zeroframecount == 1)
	{
		for (j = 0; j < pAnimStudioHdr->numbones; j++)
		{
			if (pAnimGroup)
				i = pAnimGroup->masterBone[j];
			else
				i = j;

			if (pAnimbone[j].flags & BONE_HAS_SAVEFRAME_POS)
			{
				if ((i >= 0) && (pStudioHdr->boneFlags(i) & boneMask))
				{
					Vector p = *(Vector48 *)pData;
					pos[i] = pos[i] * (1.0f - flWeight) + p * flWeight;
					Assert( pos[i].IsValid() );
				}
				pData += sizeof( Vector48 );
			}
			if (pAnimbone[j].flags & BONE_HAS_SAVEFRAME_ROT)
			{
				if ((i >= 0) && (pStudioHdr->boneFlags(i) & boneMask))
				{
					Quaternion q0 = *(Quaternion64 *)pData;
					QuaternionBlend( q[i], q0, flWeight, q[i] );
					Assert( q[i].IsValid() );
				}
				pData += sizeof( Quaternion64 );
			}
		}
	}
	else
	{
		float s1;
		int index = fFrame / animdesc.zeroframespan;
		if (index >= animdesc.zeroframecount - 1)
		{
			index = animdesc.zeroframecount - 2;
			s1 = 1.0f;
		}
		else
		{
			s1 = clamp( (fFrame - index * animdesc.zeroframespan) / animdesc.zeroframespan, 0.0f, 1.0f );
		}
		int i0 = max( index - 1, 0 );
		int i1 = index;
		int i2 = min( index + 1, animdesc.zeroframecount - 1 );
		for (j = 0; j < pAnimStudioHdr->numbones; j++)
		{
			if (pAnimGroup)
				i = pAnimGroup->masterBone[j];
			else
				i = j;

			if (pAnimbone[j].flags & BONE_HAS_SAVEFRAME_POS)
			{
				if ((i >= 0) && (pStudioHdr->boneFlags(i) & boneMask))
				{
					Vector p0 = *(((Vector48 *)pData) + i0);
					Vector p1 = *(((Vector48 *)pData) + i1);
					Vector p2 = *(((Vector48 *)pData) + i2);
					Vector p3;
					Hermite_Spline( p0, p1, p2, s1, p3 );
					pos[i] = pos[i] * (1.0f - flWeight) + p3 * flWeight;
					Assert( pos[i].IsValid() );
				}
				pData += sizeof( Vector48 ) * animdesc.zeroframecount;
			}
			if (pAnimbone[j].flags & BONE_HAS_SAVEFRAME_ROT)
			{
				if ((i >= 0) && (pStudioHdr->boneFlags(i) & boneMask))
				{
					Quaternion q0 = *(((Quaternion64 *)pData) + i0);
					Quaternion q1 = *(((Quaternion64 *)pData) + i1);
					Quaternion q2 = *(((Quaternion64 *)pData) + i2);
					if (flWeight == 1.0f)
					{
						Hermite_Spline( q0, q1, q2, s1, q[i] );
					}
					else
					{
						Quaternion q3;
						Hermite_Spline( q0, q1, q2, s1, q3 );
						QuaternionBlend( q[i], q3, flWeight, q[i] );
					}
					Assert( q[i].IsValid() );
				}
				pData += sizeof( Quaternion64 ) * animdesc.zeroframecount;
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Find and decode a sub-frame of animation, remapping the skeleton bone indexes
//-----------------------------------------------------------------------------
static void CalcVirtualAnimation( virtualmodel_t *pVModel, const CStudioHdr *pStudioHdr, Vector *pos, Quaternion *q, 
	mstudioseqdesc_t &seqdesc, int sequence, int animation,
	float cycle, int boneMask )
{
	//int	i, k;

	const mstudiobone_t *pbone;
	const virtualgroup_t *pSeqGroup;
	const studiohdr_t *pSeqStudioHdr;
	const mstudiolinearbone_t *pSeqLinearBones;
	const mstudiobone_t *pSeqbone;
	const mstudioanim_t *panim;
	const studiohdr_t *pAnimStudioHdr;
	const mstudiolinearbone_t *pAnimLinearBones;
	const mstudiobone_t *pAnimbone;
	const virtualgroup_t *pAnimGroup;

	pSeqGroup = pVModel->pSeqGroup( sequence );
	int baseanimation = pStudioHdr->iRelativeAnim( sequence, animation );
	mstudioanimdesc_t &animdesc = ((CStudioHdr *)pStudioHdr)->pAnimdesc( baseanimation );
	pSeqStudioHdr = ((CStudioHdr *)pStudioHdr)->pSeqStudioHdr( sequence );
	pSeqLinearBones = pSeqStudioHdr->pLinearBones();
	pSeqbone = pSeqStudioHdr->pBone( 0 );
	pAnimGroup = pVModel->pAnimGroup( baseanimation );
	pAnimStudioHdr = ((CStudioHdr *)pStudioHdr)->pAnimStudioHdr( baseanimation );
	pAnimLinearBones = pAnimStudioHdr->pLinearBones();
	pAnimbone = pAnimStudioHdr->pBone( 0 );

	int					iFrame;
	float				s;

	float fFrame = cycle * (animdesc.numframes - 1);

	iFrame = (int)fFrame;
	s = (fFrame - iFrame);

	int iLocalFrame = iFrame;
	float flStall;
	panim = animdesc.pAnim( &iLocalFrame, flStall );

	float *pweight = seqdesc.pBoneweight( 0 );
	pbone = pStudioHdr->pBone( 0 );

	for (int i = 0; i < pStudioHdr->numbones(); i++)
	{
		if (pStudioHdr->boneFlags(i) & boneMask)
		{
			int j = pSeqGroup->boneMap[i];
			if (j >= 0 && pweight[j] > 0.0f)
			{
				if (animdesc.flags & STUDIO_DELTA)
				{
					q[i].Init( 0.0f, 0.0f, 0.0f, 1.0f );
					pos[i].Init( 0.0f, 0.0f, 0.0f );
				}
				else if (pSeqLinearBones)
				{
					q[i] = pSeqLinearBones->quat(j);
					pos[i] = pSeqLinearBones->pos(j);
				}
				else 
				{
					q[i] = pSeqbone[j].quat;
					pos[i] = pSeqbone[j].pos;
				}
#ifdef STUDIO_ENABLE_PERF_COUNTERS
				pStudioHdr->m_nPerfUsedBones++;
#endif
			}
		}
	}

	// if the animation isn't available, look for the zero frame cache
	if (!panim)
	{
		CalcZeroframeData( ((CStudioHdr *)pStudioHdr), pAnimStudioHdr, pAnimGroup, pAnimbone, animdesc, fFrame, pos, q, boneMask, 1.0 );
		return;
	}

	// FIXME: change encoding so that bone -1 is never the case
	while (panim && panim->bone < 255)
	{
		int j = pAnimGroup->masterBone[panim->bone];
		if ( j >= 0 && ( pStudioHdr->boneFlags(j) & boneMask ) )
		{
			int k = pSeqGroup->boneMap[j];

			if (k >= 0 && pweight[k] > 0.0f)
			{
				CalcBoneQuaternion( iLocalFrame, s, &pAnimbone[panim->bone], pAnimLinearBones, panim, q[j] );
				CalcBonePosition  ( iLocalFrame, s, &pAnimbone[panim->bone], pAnimLinearBones, panim, pos[j] );
#ifdef STUDIO_ENABLE_PERF_COUNTERS
				pStudioHdr->m_nPerfAnimatedBones++;
#endif
			}
		}
		panim = panim->pNext();
	}

	// cross fade in previous zeroframe data
	if (flStall > 0.0f)
	{
		CalcZeroframeData( pStudioHdr, pAnimStudioHdr, pAnimGroup, pAnimbone, animdesc, fFrame, pos, q, boneMask, flStall );
	}

	// calculate a local hierarchy override
	if (animdesc.numlocalhierarchy)
	{
		matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
		CBoneBitList boneComputed;

		int i;
		for (i = 0; i < animdesc.numlocalhierarchy; i++)
		{
			mstudiolocalhierarchy_t *pHierarchy = animdesc.pHierarchy( i );

			if ( !pHierarchy )
				break;

			int iBone = pAnimGroup->masterBone[pHierarchy->iBone];
			if (iBone >= 0 && (pStudioHdr->boneFlags(iBone) & boneMask))
			{
				if ( pHierarchy->iNewParent != -1 )
				{
					int iNewParent = pAnimGroup->masterBone[pHierarchy->iNewParent];
					if (iNewParent >= 0 && (pStudioHdr->boneFlags(iNewParent) & boneMask))
					{
						CalcLocalHierarchyAnimation( pStudioHdr, boneToWorld, boneComputed, pos, q, pbone, pHierarchy, iBone, iNewParent, cycle, iFrame, s, boneMask );
					}
				}
				else
				{
					CalcLocalHierarchyAnimation( pStudioHdr, boneToWorld, boneComputed, pos, q, pbone, pHierarchy, iBone, -1, cycle, iFrame, s, boneMask );
				}
			}
		}

		g_MatrixPool.Free( boneToWorld );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Find and decode a sub-frame of animation
//-----------------------------------------------------------------------------

static void CalcAnimation( const CStudioHdr *pStudioHdr,	Vector *pos, Quaternion *q, 
	mstudioseqdesc_t &seqdesc,
	int sequence, int animation,
	float cycle, int boneMask )
{
#ifdef STUDIO_ENABLE_PERF_COUNTERS
	pStudioHdr->m_nPerfAnimationLayers++;
#endif

	virtualmodel_t *pVModel = pStudioHdr->GetVirtualModel();

	if (pVModel)
	{
		CalcVirtualAnimation( pVModel, pStudioHdr, pos, q, seqdesc, sequence, animation, cycle, boneMask );
		return;
	}

	mstudioanimdesc_t &animdesc = ((CStudioHdr *)pStudioHdr)->pAnimdesc( animation );
	mstudiobone_t *pbone = pStudioHdr->pBone( 0 );
	const mstudiolinearbone_t *pLinearBones = pStudioHdr->pLinearBones();

//	int					i;
	int					iFrame;
	float				s;

	float fFrame = cycle * (animdesc.numframes - 1);

	iFrame = (int)fFrame;
	s = (fFrame - iFrame);

	int iLocalFrame = iFrame;
	float flStall;
	mstudioanim_t *panim = animdesc.pAnim( &iLocalFrame, flStall );

	float *pweight = seqdesc.pBoneweight( 0 );

	// if the animation isn't available, look for the zero frame cache
	if (!panim)
	{
		// Msg("zeroframe %s\n", animdesc.pszName() );
		// pre initialize
		for (int i = 0; i < pStudioHdr->numbones(); i++, pbone++, pweight++)
		{
			if (*pweight > 0 && (pStudioHdr->boneFlags(i) & boneMask))
			{
				if (animdesc.flags & STUDIO_DELTA)
				{
					q[i].Init( 0.0f, 0.0f, 0.0f, 1.0f );
					pos[i].Init( 0.0f, 0.0f, 0.0f );
				}
				else
				{
					q[i] = pbone->quat;
					pos[i] = pbone->pos;
				}
			}
		}

		CalcZeroframeData( pStudioHdr, pStudioHdr->GetRenderHdr(), NULL, pStudioHdr->pBone( 0 ), animdesc, fFrame, pos, q, boneMask, 1.0 );

		return;
	}

	// BUGBUG: the sequence, the anim, and the model can have all different bone mappings.
	for (int i = 0; i < pStudioHdr->numbones(); i++, pbone++, pweight++)
	{
		if (panim && panim->bone == i)
		{
			if (*pweight > 0 && (pStudioHdr->boneFlags(i) & boneMask))
			{
				CalcBoneQuaternion( iLocalFrame, s, pbone, pLinearBones, panim, q[i] );
				CalcBonePosition  ( iLocalFrame, s, pbone, pLinearBones, panim, pos[i] );
#ifdef STUDIO_ENABLE_PERF_COUNTERS
				pStudioHdr->m_nPerfAnimatedBones++;
				pStudioHdr->m_nPerfUsedBones++;
#endif
			}
			panim = panim->pNext();
		}
		else if (*pweight > 0 && (pStudioHdr->boneFlags(i) & boneMask))
		{
			if (animdesc.flags & STUDIO_DELTA)
			{
				q[i].Init( 0.0f, 0.0f, 0.0f, 1.0f );
				pos[i].Init( 0.0f, 0.0f, 0.0f );
			}
			else
			{
				q[i] = pbone->quat;
				pos[i] = pbone->pos;
			}
#ifdef STUDIO_ENABLE_PERF_COUNTERS
			pStudioHdr->m_nPerfUsedBones++;
#endif
		}
	}

	// cross fade in previous zeroframe data
	if (flStall > 0.0f)
	{
		CalcZeroframeData( pStudioHdr, pStudioHdr->GetRenderHdr(), NULL, pStudioHdr->pBone( 0 ), animdesc, fFrame, pos, q, boneMask, flStall );
	}

	if (animdesc.numlocalhierarchy)
	{
		matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
		CBoneBitList boneComputed;

		int i;
		for (i = 0; i < animdesc.numlocalhierarchy; i++)
		{
			mstudiolocalhierarchy_t *pHierarchy = animdesc.pHierarchy( i );

			if ( !pHierarchy )
				break;

			if (pStudioHdr->boneFlags(pHierarchy->iBone) & boneMask)
			{
				if (pStudioHdr->boneFlags(pHierarchy->iNewParent) & boneMask)
				{
					CalcLocalHierarchyAnimation( pStudioHdr, boneToWorld, boneComputed, pos, q, pbone, pHierarchy, pHierarchy->iBone, pHierarchy->iNewParent, cycle, iFrame, s, boneMask );
				}
			}
		}

		g_MatrixPool.Free( boneToWorld );
	}

}


//-----------------------------------------------------------------------------
// Purpose: qt = ( s * p ) * q
//-----------------------------------------------------------------------------
void QuaternionSM( float s, const Quaternion &p, const Quaternion &q, Quaternion &qt )
{
	Quaternion		p1, q1;

	QuaternionScale( p, s, p1 );
	QuaternionMult( p1, q, q1 );
	QuaternionNormalize( q1 );
	qt[0] = q1[0];
	qt[1] = q1[1];
	qt[2] = q1[2];
	qt[3] = q1[3];
}

#if ALLOW_SIMD_QUATERNION_MATH
FORCEINLINE fltx4 QuaternionSMSIMD( float s, const fltx4 &p, const fltx4 &q )
{
	fltx4 p1, q1, result;
	p1 = QuaternionScaleSIMD( p, s );
	q1 = QuaternionMultSIMD( p1, q );
	result = QuaternionNormalizeSIMD( q1 );
	return result;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: qt = p * ( s * q )
//-----------------------------------------------------------------------------
void QuaternionMA( const Quaternion &p, float s, const Quaternion &q, Quaternion &qt )
{
	Quaternion p1, q1;

	QuaternionScale( q, s, q1 );
	QuaternionMult( p, q1, p1 );
	QuaternionNormalize( p1 );
	qt[0] = p1[0];
	qt[1] = p1[1];
	qt[2] = p1[2];
	qt[3] = p1[3];
}

#if ALLOW_SIMD_QUATERNION_MATH
FORCEINLINE fltx4 QuaternionMASIMD( const fltx4 &p, float s, const fltx4 &q )
{
	fltx4 p1, q1, result;
	q1 = QuaternionScaleSIMD( q, s );
	p1 = QuaternionMultSIMD( p, q1 );
	result = QuaternionNormalizeSIMD( p1 );
	return result;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: qt = p + s * q
//-----------------------------------------------------------------------------
void QuaternionAccumulate( const Quaternion &p, float s, const Quaternion &q, Quaternion &qt )
{
	Quaternion q2;
	QuaternionAlign( p, q, q2 );

	qt[0] = p[0] + s * q2[0];
	qt[1] = p[1] + s * q2[1];
	qt[2] = p[2] + s * q2[2];
	qt[3] = p[3] + s * q2[3];
}

#if ALLOW_SIMD_QUATERNION_MATH
FORCEINLINE fltx4 QuaternionAccumulateSIMD( const fltx4 &p, float s, const fltx4 &q )
{
	fltx4 q2, s4, result;
	q2 = QuaternionAlignSIMD( p, q );
	s4 = ReplicateX4( s );
	result = MaddSIMD( s4, q2, p );
	return result;
}
#endif



//-----------------------------------------------------------------------------
// Purpose: blend together in world space q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------

void WorldSpaceSlerp(
	const CStudioHdr *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	mstudioseqdesc_t &seqdesc, 
	int sequence, 
	const Quaternion q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask )
{
	int			i, j;
	float		s1; // weight of parent for q2, pos2
	float		s2; // weight for q2, pos2

	// make fake root transform
	matrix3x4_t rootXform;
	SetIdentityMatrix( rootXform );

	// matrices for q2, pos2
	matrix3x4_t *srcBoneToWorld = g_MatrixPool.Alloc();
	CBoneBitList srcBoneComputed;

	matrix3x4_t *destBoneToWorld = g_MatrixPool.Alloc();
	CBoneBitList destBoneComputed;

	matrix3x4_t *targetBoneToWorld = g_MatrixPool.Alloc();
	CBoneBitList targetBoneComputed;

	virtualmodel_t *pVModel = pStudioHdr->GetVirtualModel();
	const virtualgroup_t *pSeqGroup = NULL;
	if (pVModel)
	{
		pSeqGroup = pVModel->pSeqGroup( sequence );
	}

	mstudiobone_t *pbone = pStudioHdr->pBone( 0 );

	for (i = 0; i < pStudioHdr->numbones(); i++)
	{
		// skip unused bones
		if (!(pStudioHdr->boneFlags(i) & boneMask))
		{
			continue;
		}

		int n = pbone[i].parent;
		s1 = 0.0;
		if (pSeqGroup)
		{
			j = pSeqGroup->boneMap[i];
			if (j >= 0)
			{
				s2 = s * seqdesc.weight( j );	// blend in based on this bones weight
				if (n != -1)
				{
					s1 = s * seqdesc.weight( pSeqGroup->boneMap[n] );
				}
			}
			else
			{
				s2 = 0.0;
			}
		}
		else
		{
			s2 = s * seqdesc.weight( i );	// blend in based on this bones weight
			if (n != -1)
			{
				s1 = s * seqdesc.weight( n );
			}
		}

		if (s1 == 1.0 && s2 == 1.0)
		{
			pos1[i] = pos2[i];
			q1[i] = q2[i];
		}
		else if (s2 > 0.0)
		{
			Quaternion srcQ, destQ;
			Vector srcPos, destPos;
			Quaternion targetQ;
			Vector tmp;

			BuildBoneChain( pStudioHdr, rootXform, pos1, q1, i, destBoneToWorld, destBoneComputed );
			BuildBoneChain( pStudioHdr, rootXform, pos2, q2, i, srcBoneToWorld, srcBoneComputed );

			MatrixAngles( destBoneToWorld[i], destQ, destPos );
			MatrixAngles( srcBoneToWorld[i], srcQ, srcPos );

			QuaternionSlerp( destQ, srcQ, s2, targetQ );
			AngleMatrix( targetQ, destPos, targetBoneToWorld[i] );

			// back solve
			if (n == -1)
			{
				MatrixAngles( targetBoneToWorld[i], q1[i], tmp );
			}
			else
			{
				matrix3x4_t worldToBone;
				MatrixInvert( targetBoneToWorld[n], worldToBone );

				matrix3x4_t local;
				ConcatTransforms( worldToBone, targetBoneToWorld[i], local );
				MatrixAngles( local, q1[i], tmp );

				// blend bone lengths (local space)
				pos1[i] = Lerp( s2, pos1[i], pos2[i] );
			}
		}
	}
	g_MatrixPool.Free( srcBoneToWorld );
	g_MatrixPool.Free( destBoneToWorld );
	g_MatrixPool.Free( targetBoneToWorld );
}



//-----------------------------------------------------------------------------
// Purpose: blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void SlerpBones( 
	const CStudioHdr *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	mstudioseqdesc_t &seqdesc,  // source of q2 and pos2
	int sequence, 
	const QuaternionAligned q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask )
{
	if (s <= 0.0f) 
		return;
	if (s > 1.0f)
	{
		s = 1.0f;		
	}

	if (seqdesc.flags & STUDIO_WORLD)
	{
		WorldSpaceSlerp( pStudioHdr, q1, pos1, seqdesc, sequence, q2, pos2, s, boneMask );
		return;
	}

	int			i, j;
	virtualmodel_t *pVModel = pStudioHdr->GetVirtualModel();
	const virtualgroup_t *pSeqGroup = NULL;
	if (pVModel)
	{
		pSeqGroup = pVModel->pSeqGroup( sequence );
	}

	// Build weightlist for all bones
	int nBoneCount = pStudioHdr->numbones();
	float *pS2 = (float*)stackalloc( nBoneCount * sizeof(float) );
	for (i = 0; i < nBoneCount; i++)
	{
		// skip unused bones
		if (!(pStudioHdr->boneFlags(i) & boneMask))
		{
			pS2[i] = 0.0f;
			continue;
		}

		if ( !pSeqGroup )
		{
			pS2[i] = s * seqdesc.weight( i );	// blend in based on this bones weight
			continue;
		}

		j = pSeqGroup->boneMap[i];
		if ( j >= 0 )
		{
			pS2[i] = s * seqdesc.weight( j );	// blend in based on this bones weight
		}
		else
		{
			pS2[i] = 0.0;
		}
	}

	float s1, s2;
	if ( seqdesc.flags & STUDIO_DELTA )
	{
		for ( i = 0; i < nBoneCount; i++ )
		{
			s2 = pS2[i];
			if ( s2 <= 0.0f )
				continue;

			if ( seqdesc.flags & STUDIO_POST )
			{
#ifndef _X360
				QuaternionMA( q1[i], s2, q2[i], q1[i] );
#else
				fltx4 q1simd = LoadUnalignedSIMD( q1[i].Base() );
				fltx4 q2simd = LoadAlignedSIMD( q2[i] );
				fltx4 result = QuaternionMASIMD( q1simd, s2, q2simd );
				StoreUnalignedSIMD( q1[i].Base(), result );
#endif
				// FIXME: are these correct?
				pos1[i][0] = pos1[i][0] + pos2[i][0] * s2;
				pos1[i][1] = pos1[i][1] + pos2[i][1] * s2;
				pos1[i][2] = pos1[i][2] + pos2[i][2] * s2;
			}
			else
			{
#ifndef _X360
				QuaternionSM( s2, q2[i], q1[i], q1[i] );
#else
				fltx4 q1simd = LoadUnalignedSIMD( q1[i].Base() );
				fltx4 q2simd = LoadAlignedSIMD( q2[i] );
				fltx4 result = QuaternionSMSIMD( s2, q2simd, q1simd );
				StoreUnalignedSIMD( q1[i].Base(), result );
#endif

				// FIXME: are these correct?
				pos1[i][0] = pos1[i][0] + pos2[i][0] * s2;
				pos1[i][1] = pos1[i][1] + pos2[i][1] * s2;
				pos1[i][2] = pos1[i][2] + pos2[i][2] * s2;
			}
		}
		return;
	}

	QuaternionAligned q3;
	for (i = 0; i < nBoneCount; i++)
	{
		s2 = pS2[i];
		if ( s2 <= 0.0f )
			continue;

		s1 = 1.0 - s2;

#ifdef _X360
		fltx4  q1simd, q2simd, result;
		q1simd = LoadUnalignedSIMD( q1[i].Base() );
		q2simd = LoadAlignedSIMD( q2[i] );
#endif
		if ( pStudioHdr->boneFlags(i) & BONE_FIXED_ALIGNMENT )
		{
#ifndef _X360
			QuaternionSlerpNoAlign( q2[i], q1[i], s1, q3 );
#else
			result = QuaternionSlerpNoAlignSIMD( q2simd, q1simd, s1 );
#endif
		}
		else
		{
#ifndef _X360
			QuaternionSlerp( q2[i], q1[i], s1, q3 );
#else
			result = QuaternionSlerpSIMD( q2simd, q1simd, s1 );
#endif
		}

#ifndef _X360
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
#else
		StoreUnalignedSIMD( q1[i].Base(), result );
#endif

		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s2;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s2;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s2;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Inter-animation blend.  Assumes both types are identical.
//			blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void BlendBones( 
	const CStudioHdr *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	mstudioseqdesc_t &seqdesc, 
	int sequence,
	const Quaternion q2[MAXSTUDIOBONES], 
	const Vector pos2[MAXSTUDIOBONES], 
	float s,
	int boneMask )
{
	int			i, j;
	Quaternion		q3;

	virtualmodel_t *pVModel = pStudioHdr->GetVirtualModel();
	const virtualgroup_t *pSeqGroup = NULL;
	if (pVModel)
	{
		pSeqGroup = pVModel->pSeqGroup( sequence );
	}

	if (s <= 0)
	{
		Assert(0); // shouldn't have been called
		return;
	}
	else if (s >= 1.0)
	{
		Assert(0); // shouldn't have been called
		for (i = 0; i < pStudioHdr->numbones(); i++)
		{
			// skip unused bones
			if (!(pStudioHdr->boneFlags(i) & boneMask))
			{
				continue;
			}

			if (pSeqGroup)
			{
				j = pSeqGroup->boneMap[i];
			}
			else
			{
				j = i;
			}

			if (j >= 0 && seqdesc.weight( j ) > 0.0)
			{
				q1[i] = q2[i];
				pos1[i] = pos2[i];
			}
		}
		return;
	}

	float s2 = s;
	float s1 = 1.0 - s2;

	for (i = 0; i < pStudioHdr->numbones(); i++)
	{
		// skip unused bones
		if (!(pStudioHdr->boneFlags(i) & boneMask))
		{
			continue;
		}

		if (pSeqGroup)
		{
			j = pSeqGroup->boneMap[i];
		}
		else
		{
			j = i;
		}

		if (j >= 0 && seqdesc.weight( j ) > 0.0)
		{
			if (pStudioHdr->boneFlags(i) & BONE_FIXED_ALIGNMENT)
			{
				QuaternionBlendNoAlign( q2[i], q1[i], s1, q3 );
			}
			else
			{
				QuaternionBlend( q2[i], q1[i], s1, q3 );
			}
			q1[i][0] = q3[0];
			q1[i][1] = q3[1];
			q1[i][2] = q3[2];
			q1[i][3] = q3[3];
			pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s2;
			pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s2;
			pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s2;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Scale a set of bones.  Must be of type delta
//-----------------------------------------------------------------------------
void ScaleBones( 
	const CStudioHdr *pStudioHdr,
	Quaternion q1[MAXSTUDIOBONES], 
	Vector pos1[MAXSTUDIOBONES], 
	int sequence,
	float s,
	int boneMask )
{
	int			i, j;

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );

	virtualmodel_t *pVModel = pStudioHdr->GetVirtualModel();
	const virtualgroup_t *pSeqGroup = NULL;
	if (pVModel)
	{
		pSeqGroup = pVModel->pSeqGroup( sequence );
	}

	float s2 = s;
	float s1 = 1.0 - s2;

	for (i = 0; i < pStudioHdr->numbones(); i++)
	{
		// skip unused bones
		if (!(pStudioHdr->boneFlags(i) & boneMask))
		{
			continue;
		}

		if (pSeqGroup)
		{
			j = pSeqGroup->boneMap[i];
		}
		else
		{
			j = i;
		}

		if (j >= 0 && seqdesc.weight( j ) > 0.0)
		{
			QuaternionIdentityBlend( q1[i], s1, q1[i] );
			VectorScale( pos1[i], s2, pos1[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: resolve a global pose parameter to the specific setting for this sequence
//-----------------------------------------------------------------------------
void Studio_LocalPoseParameter( const CStudioHdr *pStudioHdr, const float poseParameter[], mstudioseqdesc_t &seqdesc, int iSequence, int iLocalIndex, float &flSetting, int &index )
{
	if (!pStudioHdr)
	{
		flSetting = 0;
		index = 0;
		return;
	}

	int iPose = pStudioHdr->GetSharedPoseParameter( iSequence, seqdesc.paramindex[iLocalIndex] );

	if (iPose == -1)
	{
		flSetting = 0;
		index = 0;
		return;
	}

	const mstudioposeparamdesc_t &Pose = ((CStudioHdr *)pStudioHdr)->pPoseParameter( iPose );

	float flValue = poseParameter[iPose];

	if (Pose.loop)
	{
		float wrap = (Pose.start + Pose.end) / 2.0 + Pose.loop / 2.0;
		float shift = Pose.loop - wrap;

		flValue = flValue - Pose.loop * floor((flValue + shift) / Pose.loop);
	}

	if (seqdesc.posekeyindex == 0)
	{
		float flLocalStart	= ((float)seqdesc.paramstart[iLocalIndex] - Pose.start) / (Pose.end - Pose.start);
		float flLocalEnd	= ((float)seqdesc.paramend[iLocalIndex] - Pose.start) / (Pose.end - Pose.start);

		// convert into local range
		flSetting = (flValue - flLocalStart) / (flLocalEnd - flLocalStart);

		// clamp.  This shouldn't ever need to happen if it's looping.
		if (flSetting < 0)
			flSetting = 0;
		if (flSetting > 1)
			flSetting = 1;

		index = 0;
		if (seqdesc.groupsize[iLocalIndex] > 2 )
		{
			// estimate index
			index = (int)(flSetting * (seqdesc.groupsize[iLocalIndex] - 1));
			if (index == seqdesc.groupsize[iLocalIndex] - 1) index = seqdesc.groupsize[iLocalIndex] - 2;
			flSetting = flSetting * (seqdesc.groupsize[iLocalIndex] - 1) - index;
		}
	}
	else
	{
		flValue = flValue * (Pose.end - Pose.start) + Pose.start;
		index = 0;
			
		// FIXME: this needs to be 2D
		// FIXME: this shouldn't be a linear search

		while (1)
		{
			flSetting = (flValue - seqdesc.poseKey( iLocalIndex, index )) / (seqdesc.poseKey( iLocalIndex, index + 1 ) - seqdesc.poseKey( iLocalIndex, index ));
			/*
			if (index > 0 && flSetting < 0.0)
			{
				index--;
				continue;
			}
			else 
			*/
			if (index < seqdesc.groupsize[iLocalIndex] - 2 && flSetting > 1.0)
			{
				index++;
				continue;
			}
			break;
		}

		// clamp.
		if (flSetting < 0.0f)
			flSetting = 0.0f;
		if (flSetting > 1.0f)
			flSetting = 1.0f;
	}
}

void Studio_CalcBoneToBoneTransform( const CStudioHdr *pStudioHdr, int inputBoneIndex, int outputBoneIndex, matrix3x4_t& matrixOut )
{
	mstudiobone_t *pbone = pStudioHdr->pBone( inputBoneIndex );

	matrix3x4_t inputToPose;
	MatrixInvert( pbone->poseToBone, inputToPose );
	ConcatTransforms( pStudioHdr->pBone( outputBoneIndex )->poseToBone, inputToPose, matrixOut );
}

//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//-----------------------------------------------------------------------------
void InitPose(
	const CStudioHdr *pStudioHdr,
	Vector pos[], 
	Quaternion q[],
	int boneMask 
	)
{
	if (!pStudioHdr->pLinearBones())
	{
		for (int i = 0; i < pStudioHdr->numbones(); i++)
		{
			if (pStudioHdr->boneFlags(  i ) & boneMask ) 
			{
				mstudiobone_t *pbone = pStudioHdr->pBone( i );
				pos[i] = pbone->pos;
				q[i] = pbone->quat;
			}
		}
	}
	else
	{
		mstudiolinearbone_t *pLinearBones = pStudioHdr->pLinearBones();
		for (int i = 0; i < pStudioHdr->numbones(); i++)
		{
			if (pStudioHdr->boneFlags(  i ) & boneMask ) 
			{
				pos[i] = pLinearBones->pos(i);
				q[i] = pLinearBones->quat(i);
			}
		}
	}
}
	

inline bool PoseIsAllZeros( 
	const CStudioHdr *pStudioHdr,
	int sequence, 
	mstudioseqdesc_t	&seqdesc,
	int i0,
	int i1
	)
{
	int baseanim;
		
	// remove "zero" positional blends
	baseanim = pStudioHdr->iRelativeAnim( sequence, seqdesc.anim(i0  ,i1 ) );
	mstudioanimdesc_t		&anim = ((CStudioHdr *)pStudioHdr)->pAnimdesc( baseanim );
	return (anim.flags & STUDIO_ALLZEROS) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: turn a 2x2 blend into a 3 way triangle blend
// Returns: returns the animination indices and barycentric coordinates of a triangle
//			the triangle is a right triangle, and the diagonal is between elements [0] and [2]
//-----------------------------------------------------------------------------

static ConVar anim_3wayblend( "anim_3wayblend", "1", FCVAR_REPLICATED, "Toggle the 3-way animation blending code." );

void Calc3WayBlendIndices( int i0, int i1, float s0, float s1, const mstudioseqdesc_t &seqdesc, int *pAnimIndices, float *pWeight )
{
	// Figure out which bi-section direction we are using to make triangles.
	bool bEven = ( ( ( i0 + i1 ) & 0x1 ) == 0 );

	int x1, y1;
	int x2, y2;
	int x3, y3;

	// diagonal is between elements 1 & 3
	// TL to BR
	if ( bEven )
	{
		if ( s0 > s1 )
		{
			// B
			x1 = 0; y1 = 0;
			x2 = 1; y2 = 0;
			x3 = 1; y3 = 1;
			pWeight[0] = (1.0f - s0);
			pWeight[1] = s0 - s1;
		}
		else
		{
			// C
			x1 = 1; y1 = 1;
			x2 = 0; y2 = 1;
			x3 = 0; y3 = 0;
			pWeight[0] = s0;
			pWeight[1] = s1 - s0;
		}
	}
	// BL to TR
	else
	{
		float flTotal = s0 + s1;

		if( flTotal > 1.0f )
		{
			// D
			x1 = 1; y1 = 0;
			x2 = 1; y2 = 1;
			x3 = 0; y3 = 1;
			pWeight[0] = (1.0f - s1);
			pWeight[1] = s0 - 1.0f + s1;
		}
		else
		{
			// A
			x1 = 0; y1 = 1;
			x2 = 0; y2 = 0;
			x3 = 1; y3 = 0;
			pWeight[0] = s1;
			pWeight[1] = 1.0f - s0 - s1;
		}
	}

	pAnimIndices[0] = seqdesc.anim( i0 + x1, i1 + y1 );
	pAnimIndices[1] = seqdesc.anim( i0 + x2, i1 + y2 );
	pAnimIndices[2] = seqdesc.anim( i0 + x3, i1 + y3 );

	/*
	float w0 = ((x2-x3)*(y3-s1) - (x3-s0)*(y2-y3)) / ((x1-x3)*(y2-y3) - (x2-x3)*(y1-y3));
	float w1 = ((x1-x3)*(y3-s1) - (x3-s0)*(y1-y3)) / ((x2-x3)*(y1-y3) - (x1-x3)*(y2-y3));
	Assert( pWeight[0] == w0 && pWeight[1] == w1 );
	*/

	// clamp the diagonal
	if (pWeight[1] < 0.001f)
		pWeight[1] = 0.0f;
	pWeight[2] = 1.0f - pWeight[0] - pWeight[1];

	Assert( pWeight[0] >= 0.0f && pWeight[0] <= 1.0f );
	Assert( pWeight[1] >= 0.0f && pWeight[1] <= 1.0f );
	Assert( pWeight[2] >= 0.0f && pWeight[2] <= 1.0f );
}



//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//-----------------------------------------------------------------------------
bool CalcPoseSingle(
	const CStudioHdr *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	mstudioseqdesc_t &seqdesc,
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flTime
	)
{
	bool bResult = true;
	
	Vector		*pos2 = g_VectorPool.Alloc();
	Quaternion	*q2 = g_QaternionPool.Alloc();
	Vector		*pos3= g_VectorPool.Alloc();
	Quaternion	*q3 = g_QaternionPool.Alloc();

	if (sequence >= pStudioHdr->GetNumSeq()) 
	{
		sequence = 0;
		seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );
	}


	int i0 = 0, i1 = 0;
	float s0 = 0, s1 = 0;

	Studio_LocalPoseParameter( pStudioHdr, poseParameter, seqdesc, sequence, 0, s0, i0 );
	Studio_LocalPoseParameter( pStudioHdr, poseParameter, seqdesc, sequence, 1, s1, i1 );


	if (seqdesc.flags & STUDIO_REALTIME)
	{
		float cps = Studio_CPS( pStudioHdr, seqdesc, sequence, poseParameter );
		cycle = flTime * cps;
		cycle = cycle - (int)cycle;
	}
	else if (seqdesc.flags & STUDIO_CYCLEPOSE)
	{
		int iPose = pStudioHdr->GetSharedPoseParameter( sequence, seqdesc.cycleposeindex );
		if (iPose != -1)
		{
			/*
			const mstudioposeparamdesc_t &Pose = ((CStudioHdr *)pStudioHdr)->pPoseParameter( iPose );
			cycle = poseParameter[ iPose ] * (Pose.end - Pose.start) + Pose.start;
			*/
			cycle = poseParameter[ iPose ];
		}
		else
		{
			cycle = 0.0f;
		}
	}
	else if (cycle < 0 || cycle >= 1)
	{
		if (seqdesc.flags & STUDIO_LOOPING)
		{
			cycle = cycle - (int)cycle;
			if (cycle < 0) cycle += 1;
		}
		else
		{
			cycle = clamp( cycle, 0.0f, 1.0f );
		}
	}

	if (s0 < 0.001)
	{
		if (s1 < 0.001)
		{
			if (PoseIsAllZeros( pStudioHdr, sequence, seqdesc, i0, i1 ))
			{
				bResult = false;
			}
			else
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  , i1   ), cycle, boneMask );
			}
		}
		else if (s1 > 0.999)
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  , i1+1 ), cycle, boneMask );
		}
		else
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  , i1   ), cycle, boneMask );
			CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0  , i1+1 ), cycle, boneMask );
			BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s1, boneMask );
		}
	}
	else if (s0 > 0.999)
	{
		if (s1 < 0.001)
		{
			if (PoseIsAllZeros( pStudioHdr, sequence, seqdesc, i0+1, i1 ))
			{
				bResult = false;
			}
			else
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0+1, i1   ), cycle, boneMask );
			}
		}
		else if (s1 > 0.999)
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0+1, i1+1 ), cycle, boneMask );
		}
		else
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0+1, i1   ), cycle, boneMask );
			CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0+1, i1+1 ), cycle, boneMask );
			BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s1, boneMask );
		}
	}
	else
	{
		if (s1 < 0.001)
		{
			if (PoseIsAllZeros( pStudioHdr, sequence, seqdesc, i0+1, i1 ))
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  ,i1  ), cycle, boneMask );
				ScaleBones( pStudioHdr, q, pos, sequence, 1.0 - s0, boneMask );
			}
			else if (PoseIsAllZeros( pStudioHdr, sequence, seqdesc, i0, i1 ))
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0+1  ,i1  ), cycle, boneMask );
				ScaleBones( pStudioHdr, q, pos, sequence, s0, boneMask );
			}
			else
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  ,i1  ), cycle, boneMask );
				CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0+1,i1  ), cycle, boneMask );

				BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s0, boneMask );
			}
		}
		else if (s1 > 0.999)
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  ,i1+1  ), cycle, boneMask );
			CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0+1,i1+1  ), cycle, boneMask );
			BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s0, boneMask );
		}
		else if ( !anim_3wayblend.GetBool() )
		{
			CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, seqdesc.anim( i0  ,i1  ), cycle, boneMask );
			CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0+1,i1  ), cycle, boneMask );
			BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s0, boneMask );

			CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, seqdesc.anim( i0  , i1+1), cycle, boneMask );
			CalcAnimation( pStudioHdr, pos3, q3, seqdesc, sequence, seqdesc.anim( i0+1, i1+1), cycle, boneMask );
			BlendBones( pStudioHdr, q2, pos2, seqdesc, sequence, q3, pos3, s0, boneMask );

			BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, s1, boneMask );
		}
		else
		{
			int		iAnimIndices[3];
			float	weight[3];

			Calc3WayBlendIndices( i0, i1, s0, s1, seqdesc, iAnimIndices, weight );

			/*
			char buf[256];
			sprintf( buf, "%d %6.2f  %d %6.2f : %6.2f %6.2f %6.2f\n", i0, s0, i1, s1, weight[0], weight[1], weight[2] );
			OutputDebugString( buf );
			*/

			if (weight[1] < 0.001)
			{
				// on diagonal
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, iAnimIndices[0], cycle, boneMask );
				CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, iAnimIndices[2], cycle, boneMask );
				BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, weight[2] / (weight[0] + weight[2]), boneMask );
			}
			else
			{
				CalcAnimation( pStudioHdr, pos,  q,  seqdesc, sequence, iAnimIndices[0], cycle, boneMask );
				CalcAnimation( pStudioHdr, pos2, q2, seqdesc, sequence, iAnimIndices[1], cycle, boneMask );
				BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, weight[1] / (weight[0] + weight[1]), boneMask );

				CalcAnimation( pStudioHdr, pos3, q3, seqdesc, sequence, iAnimIndices[2], cycle, boneMask );
				BlendBones( pStudioHdr, q, pos, seqdesc, sequence, q3, pos3, weight[2], boneMask );
			}
		}
	}

	g_VectorPool.Free( pos2 );
	g_QaternionPool.Free( q2 );
	g_VectorPool.Free( pos3 );
	g_QaternionPool.Free( q3 );

	return bResult;
}




//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CBoneSetup::AddSequenceLayers(
   Vector pos[], 
   Quaternion q[], 
   mstudioseqdesc_t &seqdesc,
   int sequence, 
   float cycle,
   float flWeight,
   float flTime,
   CIKContext *pIKContext
   )
{
	for (int i = 0; i < seqdesc.numautolayers; i++)
	{
		mstudioautolayer_t *pLayer = seqdesc.pAutolayer( i );

		if (pLayer->flags & STUDIO_AL_LOCAL)
			continue;

		float layerCycle = cycle;
		float layerWeight = flWeight;

		if (pLayer->start != pLayer->end)
		{
			float s = 1.0;
			float index;

			if (!(pLayer->flags & STUDIO_AL_POSE))
			{
				index = cycle;
			}
			else
			{
				int iSequence = m_pStudioHdr->iRelativeSeq( sequence, pLayer->iSequence );
				int iPose = m_pStudioHdr->GetSharedPoseParameter( iSequence, pLayer->iPose );
				if (iPose != -1)
				{
					const mstudioposeparamdesc_t &Pose = ((CStudioHdr *)m_pStudioHdr)->pPoseParameter( iPose );
					index = m_flPoseParameter[ iPose ] * (Pose.end - Pose.start) + Pose.start;
				}
				else
				{
					index = 0;
				}
			}

			if (index < pLayer->start)
				continue;
			if (index >= pLayer->end)
				continue;

			if (index < pLayer->peak && pLayer->start != pLayer->peak)
			{
				s = (index - pLayer->start) / (pLayer->peak - pLayer->start);
			}
			else if (index > pLayer->tail && pLayer->end != pLayer->tail)
			{
				s = (pLayer->end - index) / (pLayer->end - pLayer->tail);
			}

			if (pLayer->flags & STUDIO_AL_SPLINE)
			{
				s = SimpleSpline( s );
			}

			if ((pLayer->flags & STUDIO_AL_XFADE) && (index > pLayer->tail))
			{
				layerWeight = ( s * flWeight ) / ( 1 - flWeight + s * flWeight );
			}
			else if (pLayer->flags & STUDIO_AL_NOBLEND)
			{
				layerWeight = s;
			}
			else
			{
				layerWeight = flWeight * s;
			}

			if (!(pLayer->flags & STUDIO_AL_POSE))
			{
				layerCycle = (cycle - pLayer->start) / (pLayer->end - pLayer->start);
			}
		}

		int iSequence = m_pStudioHdr->iRelativeSeq( sequence, pLayer->iSequence );
		AccumulatePose( pos, q, iSequence, layerCycle, layerWeight, flTime, pIKContext );
	}
}


//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CBoneSetup::AddLocalLayers(
	Vector pos[], 
	Quaternion q[], 
	mstudioseqdesc_t &seqdesc,
	int sequence, 
	float cycle,
	float flWeight,
	float flTime,
	CIKContext *pIKContext
	)
{
	if (!(seqdesc.flags & STUDIO_LOCAL))
	{
		return;
	}

	for (int i = 0; i < seqdesc.numautolayers; i++)
	{
		mstudioautolayer_t *pLayer = seqdesc.pAutolayer( i );

		if (!(pLayer->flags & STUDIO_AL_LOCAL))
			continue;

		float layerCycle = cycle;
		float layerWeight = flWeight;

		if (pLayer->start != pLayer->end)
		{
			float s = 1.0;

			if (cycle < pLayer->start)
				continue;
			if (cycle >= pLayer->end)
				continue;

			if (cycle < pLayer->peak && pLayer->start != pLayer->peak)
			{
				s = (cycle - pLayer->start) / (pLayer->peak - pLayer->start);
			}
			else if (cycle > pLayer->tail && pLayer->end != pLayer->tail)
			{
				s = (pLayer->end - cycle) / (pLayer->end - pLayer->tail);
			}

			if (pLayer->flags & STUDIO_AL_SPLINE)
			{
				s = SimpleSpline( s );
			}

			if ((pLayer->flags & STUDIO_AL_XFADE) && (cycle > pLayer->tail))
			{
				layerWeight = ( s * flWeight ) / ( 1 - flWeight + s * flWeight );
			}
			else if (pLayer->flags & STUDIO_AL_NOBLEND)
			{
				layerWeight = s;
			}
			else
			{
				layerWeight = flWeight * s;
			}

			layerCycle = (cycle - pLayer->start) / (pLayer->end - pLayer->start);
		}

		int iSequence = m_pStudioHdr->iRelativeSeq( sequence, pLayer->iSequence );
		AccumulatePose( pos, q, iSequence, layerCycle, layerWeight, flTime, pIKContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: my sleezy attempt at an interface only class
//-----------------------------------------------------------------------------

IBoneSetup::IBoneSetup( const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger )
{
	m_pBoneSetup = new CBoneSetup( pStudioHdr, boneMask, poseParameter, pPoseDebugger );
}

IBoneSetup::~IBoneSetup( void )
{
	if ( m_pBoneSetup )
	{
		delete m_pBoneSetup;
	}
}

void IBoneSetup::InitPose( Vector pos[], Quaternion q[] )
{
	::InitPose( m_pBoneSetup->m_pStudioHdr, pos, q, m_pBoneSetup->m_boneMask );
}

void IBoneSetup::AccumulatePose( Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext )
{
	m_pBoneSetup->AccumulatePose( pos, q, sequence, cycle, flWeight, flTime, pIKContext );
}

void IBoneSetup::CalcAutoplaySequences(	Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext )
{
	m_pBoneSetup->CalcAutoplaySequences( pos, q, flRealTime, pIKContext );
}

void CalcBoneAdj( const CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], const float controllers[], int boneMask );

// takes a "controllers[]" array normalized to 0..1 and adds in the adjustments to pos[], and q[].
void IBoneSetup::CalcBoneAdj( Vector pos[], Quaternion q[], const float controllers[] )
{
	::CalcBoneAdj( m_pBoneSetup->m_pStudioHdr, pos, q, controllers, m_pBoneSetup->m_boneMask );
}

CStudioHdr *IBoneSetup::GetStudioHdr()
{
	return (CStudioHdr *)m_pBoneSetup->m_pStudioHdr;
}

CBoneSetup::CBoneSetup( const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger )
{
	m_pStudioHdr = pStudioHdr;
	m_boneMask = boneMask;
	m_flPoseParameter = poseParameter;
	m_pPoseDebugger = pPoseDebugger;
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: calculate a pose for a single sequence
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CalcPose(
	const CStudioHdr *pStudioHdr,
	CIKContext *pIKContext,
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	const float poseParameter[],
	int boneMask,
	float flWeight,
	float flTime
	)
{
	mstudioseqdesc_t	&seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	// add any IK locks to prevent numautolayers from moving extremities 
	CIKContext seq_ik;
	if (seqdesc.numiklocks)
	{
		seq_ik.Init( pStudioHdr, vec3_angle, vec3_origin, 0.0, 0, boneMask ); // local space relative so absolute position doesn't mater
		seq_ik.AddSequenceLocks( seqdesc, pos, q );
	}

	CalcPoseSingle( pStudioHdr, pos, q, seqdesc, sequence, cycle, poseParameter, boneMask, flTime );

	if ( pIKContext )
	{
		pIKContext->AddDependencies( seqdesc, sequence, cycle, poseParameter, flWeight );
	}
	
	AddSequenceLayers( pStudioHdr, pIKContext, pos, q, seqdesc, sequence, cycle, poseParameter, boneMask, flWeight, flTime );

	if (seqdesc.numiklocks)
	{
		seq_ik.SolveSequenceLocks( seqdesc, pos, q );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: accumulate a pose for a single sequence on top of existing animation
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CBoneSetup::AccumulatePose(
	Vector pos[], 
	Quaternion q[], 
	int sequence, 
	float cycle,
	float flWeight,
	float flTime,
	CIKContext *pIKContext
	)
{
	Vector		pos2[MAXSTUDIOBONES];
	QuaternionAligned	q2[MAXSTUDIOBONES];

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	if ( sequence < 0 )
		return;

#ifdef CLIENT_DLL
	// Trigger pose debugger
	if (m_pPoseDebugger)
	{
		m_pPoseDebugger->AccumulatePose( m_pStudioHdr, pIKContext, pos, q, sequence, cycle, m_flPoseParameter, m_boneMask, flWeight, flTime );
	}
#endif

	mstudioseqdesc_t	&seqdesc = ((CStudioHdr *)m_pStudioHdr)->pSeqdesc( sequence );

	// add any IK locks to prevent extremities from moving
	CIKContext seq_ik;
	if (seqdesc.numiklocks)
	{
		seq_ik.Init( m_pStudioHdr, vec3_angle, vec3_origin, 0.0, 0, m_boneMask );  // local space relative so absolute position doesn't mater
		seq_ik.AddSequenceLocks( seqdesc, pos, q );
	}

	if (seqdesc.flags & STUDIO_LOCAL)
	{
		::InitPose( m_pStudioHdr, pos2, q2, m_boneMask );
	}

	if (CalcPoseSingle( m_pStudioHdr, pos2, q2, seqdesc, sequence, cycle, m_flPoseParameter, m_boneMask, flTime ))
	{
		// this weight is wrong, the IK rules won't composite at the correct intensity
		AddLocalLayers( pos2, q2, seqdesc, sequence, cycle, 1.0, flTime, pIKContext );
		SlerpBones( m_pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, flWeight, m_boneMask );
	}


	if ( pIKContext )
	{
		pIKContext->AddDependencies( seqdesc, sequence, cycle, m_flPoseParameter, flWeight );
	}

	AddSequenceLayers( pos, q, seqdesc, sequence, cycle, flWeight, flTime, pIKContext );

	if (seqdesc.numiklocks)
	{
		seq_ik.SolveSequenceLocks( seqdesc, pos, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose: blend together q1,pos1 with q2,pos2.  Return result in q1,pos1.  
//			0 returns q1, pos1.  1 returns q2, pos2
//-----------------------------------------------------------------------------
void CalcBoneAdj(
	const CStudioHdr *pStudioHdr,
	Vector pos[], 
	Quaternion q[], 
	const float controllers[],
	int boneMask
	)
{
	int					i, j, k;
	float				value;
	mstudiobonecontroller_t *pbonecontroller;
	RadianEuler a0;
	Quaternion q0;
	
	for (j = 0; j < pStudioHdr->numbonecontrollers(); j++)
	{
		pbonecontroller = pStudioHdr->pBonecontroller( j );
		k = pbonecontroller->bone;

		if (pStudioHdr->boneFlags( k ) & boneMask)
		{
			i = pbonecontroller->inputfield;
			value = controllers[i];
			if (value < 0) value = 0;
			if (value > 1.0) value = 1.0;
			value = (1.0 - value) * pbonecontroller->start + value * pbonecontroller->end;

			switch(pbonecontroller->type & STUDIO_TYPES)
			{
			case STUDIO_XR: 
				a0.Init( value * (M_PI / 180.0), 0, 0 ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_YR: 
				a0.Init( 0, value * (M_PI / 180.0), 0 ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_ZR: 
				a0.Init( 0, 0, value * (M_PI / 180.0) ); 
				AngleQuaternion( a0, q0 );
				QuaternionSM( 1.0, q0, q[k], q[k] );
				break;
			case STUDIO_X:	
				pos[k].x += value;
				break;
			case STUDIO_Y:
				pos[k].y += value;
				break;
			case STUDIO_Z:
				pos[k].z += value;
				break;
			}
		}
	}
}


void CalcBoneDerivatives( Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &prev, const matrix3x4_t &current, float dt )
{
	float scale = 1.0;
	if ( dt > 0 )
	{
		scale = 1.0 / dt;
	}
	
	Vector endPosition, startPosition, deltaAxis;
	QAngle endAngles, startAngles;
	float deltaAngle;

	MatrixAngles( prev, startAngles, startPosition );
	MatrixAngles( current, endAngles, endPosition );

	velocity.x = (endPosition.x - startPosition.x) * scale;
	velocity.y = (endPosition.y - startPosition.y) * scale;
	velocity.z = (endPosition.z - startPosition.z) * scale;
	RotationDeltaAxisAngle( startAngles, endAngles, deltaAxis, deltaAngle );
	VectorScale( deltaAxis, (deltaAngle * scale), angVel );
}

void CalcBoneVelocityFromDerivative( const QAngle &vecAngles, Vector &velocity, AngularImpulse &angVel, const matrix3x4_t &current )
{
	Vector vecLocalVelocity;
	AngularImpulse LocalAngVel;
	Quaternion q;
	float angle;
	MatrixAngles( current, q, vecLocalVelocity );
	QuaternionAxisAngle( q, LocalAngVel, angle );
	LocalAngVel *= angle;

	matrix3x4_t matAngles;
	AngleMatrix( vecAngles, matAngles );
	VectorTransform( vecLocalVelocity, matAngles, velocity );
	VectorTransform( LocalAngVel, matAngles, angVel );
}




class CIKSolver
{
public:
//-------- SOLVE TWO LINK INVERSE KINEMATICS -------------
// Author: Ken Perlin
//
// Given a two link joint from [0,0,0] to end effector position P,
// let link lengths be a and b, and let norm |P| = c.  Clearly a+b <= c.
//
// Problem: find a "knee" position Q such that |Q| = a and |P-Q| = b.
//
// In the case of a point on the x axis R = [c,0,0], there is a
// closed form solution S = [d,e,0], where |S| = a and |R-S| = b:
//
//    d2+e2 = a2                  -- because |S| = a
//    (c-d)2+e2 = b2              -- because |R-S| = b
//
//    c2-2cd+d2+e2 = b2           -- combine the two equations
//    c2-2cd = b2 - a2
//    c-2d = (b2-a2)/c
//    d - c/2 = (a2-b2)/c / 2
//
//    d = (c + (a2-b2/c) / 2      -- to solve for d and e.
//    e = sqrt(a2-d2)

   static float findD(float a, float b, float c) {
      return (c + (a*a-b*b)/c) / 2;
   }
   static float findE(float a, float d) { return sqrt(a*a-d*d); } 

// This leads to a solution to the more general problem:
//
//   (1) R = Mfwd(P)         -- rotate P onto the x axis
//   (2) Solve for S
//   (3) Q = Minv(S)         -- rotate back again

   float Mfwd[3][3];
   float Minv[3][3];

   bool solve(float A, float B, float const P[], float const D[], float Q[]) {
      float R[3];
      defineM(P,D);
      rot(Minv,P,R);
	  float r = length(R);
      float d = findD(A,B,r);
      float e = findE(A,d);
      float S[3] = {d,e,0};
      rot(Mfwd,S,Q);
      return d > (r - B) && d < A;
   }

// If "knee" position Q needs to be as close as possible to some point D,
// then choose M such that M(D) is in the y>0 half of the z=0 plane.
//
// Given that constraint, define the forward and inverse of M as follows:

   void defineM(float const P[], float const D[]) {
      float *X = Minv[0], *Y = Minv[1], *Z = Minv[2];

// Minv defines a coordinate system whose x axis contains P, so X = unit(P).
	  int i;
      for (i = 0 ; i < 3 ; i++)
         X[i] = P[i];
      normalize(X);

// Its y axis is perpendicular to P, so Y = unit( E - X(E·X) ).

      float dDOTx = dot(D,X);
      for (i = 0 ; i < 3 ; i++)
         Y[i] = D[i] - dDOTx * X[i];
      normalize(Y);

// Its z axis is perpendicular to both X and Y, so Z = X×Y.

      cross(X,Y,Z);

// Mfwd = (Minv)T, since transposing inverts a rotation matrix.

      for (i = 0 ; i < 3 ; i++) {
         Mfwd[i][0] = Minv[0][i];
         Mfwd[i][1] = Minv[1][i];
         Mfwd[i][2] = Minv[2][i];
      }
   }

//------------ GENERAL VECTOR MATH SUPPORT -----------

   static float dot(float const a[], float const b[]) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

   static float length(float const v[]) { return sqrt( dot(v,v) ); }

   static void normalize(float v[]) {
      float norm = length(v);
      for (int i = 0 ; i < 3 ; i++)
         v[i] /= norm;
   }

   static void cross(float const a[], float const b[], float c[]) {
      c[0] = a[1] * b[2] - a[2] * b[1];
      c[1] = a[2] * b[0] - a[0] * b[2];
      c[2] = a[0] * b[1] - a[1] * b[0];
   }

   static void rot(float const M[3][3], float const src[], float dst[]) {
      for (int i = 0 ; i < 3 ; i++)
         dst[i] = dot(M[i],src);
   }
};



//-----------------------------------------------------------------------------
// Purpose: visual debugging code
//-----------------------------------------------------------------------------
#if 1
inline void debugLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration) { };
#else
extern void drawLine( const Vector &p1, const Vector &p2, int r = 0, int g = 0, int b = 1, bool noDepthTest = true, float duration = 0.1 );
void debugLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration)
{
	drawLine( origin, dest, r, g, b, noDepthTest, duration );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: for a 2 bone chain, find the IK solution and reset the matrices
//-----------------------------------------------------------------------------
bool Studio_SolveIK( mstudioikchain_t *pikchain, Vector &targetFoot, matrix3x4_t *pBoneToWorld )
{
	if (pikchain->pLink(0)->kneeDir.LengthSqr() > 0.0)
	{
		Vector targetKneeDir, targetKneePos;
		// FIXME: knee length should be as long as the legs
		Vector tmp = pikchain->pLink( 0 )->kneeDir;
		VectorRotate( tmp, pBoneToWorld[ pikchain->pLink( 0 )->bone ], targetKneeDir );
		MatrixPosition( pBoneToWorld[ pikchain->pLink( 1 )->bone ], targetKneePos );
		return Studio_SolveIK( pikchain->pLink( 0 )->bone, pikchain->pLink( 1 )->bone, pikchain->pLink( 2 )->bone, targetFoot, targetKneePos, targetKneeDir, pBoneToWorld );
	}
	else
	{
		return Studio_SolveIK( pikchain->pLink( 0 )->bone, pikchain->pLink( 1 )->bone, pikchain->pLink( 2 )->bone, targetFoot, pBoneToWorld );
	}
}


#define KNEEMAX_EPSILON 0.9998 // (0.9998 is about 1 degree)

//-----------------------------------------------------------------------------
// Purpose: Solve Knee position for a known hip and foot location, but no specific knee direction preference
//-----------------------------------------------------------------------------

bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, matrix3x4_t *pBoneToWorld )
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixPosition( pBoneToWorld[ iThigh ], worldThigh );
	MatrixPosition( pBoneToWorld[ iKnee ], worldKnee );
	MatrixPosition( pBoneToWorld[ iFoot ], worldFoot );

	//debugLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	//debugLine( worldKnee, worldFoot, 0, 0, 255, true, 0 );

	Vector ikFoot, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = worldKnee - worldThigh;

	float l1 = (worldKnee-worldThigh).Length();
	float l2 = (worldFoot-worldKnee).Length();
	float l3 = (worldFoot-worldThigh).Length();

	// leg too straight to figure out knee?
	if (l3 > (l1 + l2) * KNEEMAX_EPSILON)
	{
		return false;
	}

	Vector ikHalf = (worldFoot-worldThigh) * (l1 / l3);

	// FIXME: what to do when the knee completely straight?
	Vector ikKneeDir = ikKnee - ikHalf;
	VectorNormalize( ikKneeDir );

	return Studio_SolveIK( iThigh, iKnee, iFoot, targetFoot, worldKnee, ikKneeDir, pBoneToWorld );
}

//-----------------------------------------------------------------------------
// Purpose: Realign the matrix so that its X axis points along the desired axis.
//-----------------------------------------------------------------------------
void Studio_AlignIKMatrix( matrix3x4_t &mMat, const Vector &vAlignTo )
{
	Vector tmp1, tmp2, tmp3;

	// Column 0 (X) becomes the vector.
	tmp1 = vAlignTo;
	VectorNormalize( tmp1 );
	MatrixSetColumn( tmp1, 0, mMat );

	// Column 1 (Y) is the cross of the vector and column 2 (Z).
	MatrixGetColumn( mMat, 2, tmp3 );
	tmp2 = tmp3.Cross( tmp1 );
	VectorNormalize( tmp2 );
	// FIXME: check for X being too near to Z
	MatrixSetColumn( tmp2, 1, mMat );

	// Column 2 (Z) is the cross of columns 0 (X) and 1 (Y).
	tmp3 = tmp1.Cross( tmp2 );
	MatrixSetColumn( tmp3, 2, mMat );
}


//-----------------------------------------------------------------------------
// Purpose: Solve Knee position for a known hip and foot location, and a known knee direction
//-----------------------------------------------------------------------------

bool Studio_SolveIK( int iThigh, int iKnee, int iFoot, Vector &targetFoot, Vector &targetKneePos, Vector &targetKneeDir, matrix3x4_t *pBoneToWorld )
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixPosition( pBoneToWorld[ iThigh ], worldThigh );
	MatrixPosition( pBoneToWorld[ iKnee ], worldKnee );
	MatrixPosition( pBoneToWorld[ iFoot ], worldFoot );

	//debugLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	//debugLine( worldThigh, worldThigh + targetKneeDir, 0, 0, 255, true, 0 );
	// debugLine( worldKnee, targetKnee, 0, 0, 255, true, 0 );

	Vector ikFoot, ikTargetKnee, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = targetKneePos - worldThigh;

	float l1 = (worldKnee-worldThigh).Length();
	float l2 = (worldFoot-worldKnee).Length();

	// exaggerate knee targets for legs that are nearly straight
	// FIXME: should be configurable, and the ikKnee should be from the original animation, not modifed
	float d = (targetFoot-worldThigh).Length() - min( l1, l2 );
	d = max( l1 + l2, d );
	// FIXME: too short knee directions cause trouble
	d = d * 100;

	ikTargetKnee = ikKnee + targetKneeDir * d;

	// debugLine( worldKnee, worldThigh + ikTargetKnee, 0, 0, 255, true, 0 );

	int color[3] = { 0, 255, 0 };

	// too far away? (0.9998 is about 1 degree)
	if (ikFoot.Length() > (l1 + l2) * KNEEMAX_EPSILON)
	{
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, (l1 + l2) * KNEEMAX_EPSILON, ikFoot );
		color[0] = 255; color[1] = 0; color[2] = 0;
	}

	// too close?
	// limit distance to about an 80 degree knee bend
	float minDist = max( fabs(l1 - l2) * 1.15, min( l1, l2 ) * 0.15 );
	if (ikFoot.Length() < minDist)
	{
		// too close to get an accurate vector, just use original vector
		ikFoot = (worldFoot - worldThigh);
		VectorNormalize( ikFoot );
		VectorScale( ikFoot, minDist, ikFoot );
	}

	CIKSolver ik;
	if (ik.solve( l1, l2, ikFoot.Base(), ikTargetKnee.Base(), ikKnee.Base() ))
	{
		matrix3x4_t& mWorldThigh = pBoneToWorld[ iThigh ];
		matrix3x4_t& mWorldKnee = pBoneToWorld[ iKnee ];
		matrix3x4_t& mWorldFoot = pBoneToWorld[ iFoot ];

		//debugLine( worldThigh, ikKnee + worldThigh, 255, 0, 0, true, 0 );
		//debugLine( ikKnee + worldThigh, ikFoot + worldThigh, 255, 0, 0, true,0 );

		// debugLine( worldThigh, ikKnee + worldThigh, color[0], color[1], color[2], true, 0 );
		// debugLine( ikKnee + worldThigh, ikFoot + worldThigh, color[0], color[1], color[2], true,0 );


		// build transformation matrix for thigh
		Studio_AlignIKMatrix( mWorldThigh, ikKnee );
		Studio_AlignIKMatrix( mWorldKnee, ikFoot - ikKnee );


		mWorldKnee[0][3] = ikKnee.x + worldThigh.x;
		mWorldKnee[1][3] = ikKnee.y + worldThigh.y;
		mWorldKnee[2][3] = ikKnee.z + worldThigh.z;

		mWorldFoot[0][3] = ikFoot.x + worldThigh.x;
		mWorldFoot[1][3] = ikFoot.y + worldThigh.y;
		mWorldFoot[2][3] = ikFoot.z + worldThigh.z;

		return true;
	}
	else
	{
		/*
		debugLine( worldThigh, worldThigh + ikKnee, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikKnee, worldThigh + ikFoot, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikFoot, worldThigh, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikKnee, worldThigh + ikTargetKnee, 255, 0, 0, true, 0 );
		*/
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

float Studio_IKRuleWeight( mstudioikrule_t &ikRule, const mstudioanimdesc_t *panim, float flCycle, int &iFrame, float &fraq )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	float value = 0.0f;
	fraq = (panim->numframes - 1) * (flCycle - ikRule.start) + ikRule.iStart;
	iFrame = (int)fraq;
	fraq = fraq - iFrame;

	if (flCycle < ikRule.start)
	{
		iFrame = ikRule.iStart;
		fraq = 0.0f;
		return 0.0f;
	}
	else if (flCycle < ikRule.peak )
	{
		value = (flCycle - ikRule.start) / (ikRule.peak - ikRule.start);
	}
	else if (flCycle < ikRule.tail )
	{
		return 1.0f;
	}
	else if (flCycle < ikRule.end )
	{
		value = 1.0f - ((flCycle - ikRule.tail) / (ikRule.end - ikRule.tail));
	}
	else
	{
		fraq = (panim->numframes - 1) * (ikRule.end - ikRule.start) + ikRule.iStart;
		iFrame = (int)fraq;
		fraq = fraq - iFrame;
	}
	return SimpleSpline( value );
}


float Studio_IKRuleWeight( ikcontextikrule_t &ikRule, float flCycle )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	float value = 0.0f;
	if (flCycle < ikRule.start)
	{
		return 0.0f;
	}
	else if (flCycle < ikRule.peak )
	{
		value = (flCycle - ikRule.start) / (ikRule.peak - ikRule.start);
	}
	else if (flCycle < ikRule.tail )
	{
		return 1.0f;
	}
	else if (flCycle < ikRule.end )
	{
		value = 1.0f - ((flCycle - ikRule.tail) / (ikRule.end - ikRule.tail));
	}
	return 3.0f * value * value - 2.0f * value * value * value;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool Studio_IKShouldLatch( ikcontextikrule_t &ikRule, float flCycle )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	if (flCycle < ikRule.peak )
	{
		return false;
	}
	else if (flCycle < ikRule.end )
	{
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

float Studio_IKTail( ikcontextikrule_t &ikRule, float flCycle )
{
	if (ikRule.end > 1.0f && flCycle < ikRule.start)
	{
		flCycle = flCycle + 1.0f;
	}

	if (flCycle <= ikRule.tail )
	{
		return 0.0f;
	}
	else if (flCycle < ikRule.end )
	{
		return ((flCycle - ikRule.tail) / (ikRule.end - ikRule.tail));
	}
	return 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


bool Studio_IKAnimationError( const CStudioHdr *pStudioHdr, mstudioikrule_t *pRule, const mstudioanimdesc_t *panim, float flCycle, Vector &pos, Quaternion &q, float &flWeight )
{
	float fraq;
	int iFrame;

	flWeight = Studio_IKRuleWeight( *pRule, panim, flCycle, iFrame, fraq );
	Assert( fraq >= 0.0 && fraq < 1.0 );
	Assert( flWeight >= 0.0f && flWeight <= 1.0f );

	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	if (pRule->type != IK_GROUND && flWeight < 0.0001)
		return false;

	mstudioikerror_t *pError = pRule->pError( iFrame );
	if (pError != NULL)
	{
		if (fraq < 0.001)
		{
			q = pError[0].q;
			pos = pError[0].pos;
		}
		else
		{
			QuaternionBlend( pError[0].q, pError[1].q, fraq, q );
			pos = pError[0].pos * (1.0f - fraq) + pError[1].pos * fraq;
		}
		return true;
	}

	mstudiocompressedikerror_t *pCompressed = pRule->pCompressedError();
	if (pCompressed != NULL)
	{
		CalcDecompressedAnimation( pCompressed, iFrame - pRule->iStart, fraq, pos, q );
		return true;
	}
	// no data, disable IK rule
	Assert( 0 );
	flWeight = 0.0f;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For a specific sequence:rule, find where it starts, stops, and what 
//			the estimated offset from the connection point is.
//			return true if the rule is within bounds.
//-----------------------------------------------------------------------------

bool Studio_IKSequenceError( const CStudioHdr *pStudioHdr, mstudioseqdesc_t &seqdesc, int iSequence, float flCycle, int iRule, const float poseParameter[], mstudioanimdesc_t *panim[4], float weight[4], ikcontextikrule_t &ikRule )
{
	int i;

	memset( &ikRule, 0, sizeof(ikRule) );
	ikRule.start = ikRule.peak = ikRule.tail = ikRule.end = 0;


	mstudioikrule_t *prevRule = NULL;

	// find overall influence
	for (i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			if (iRule >= panim[i]->numikrules || panim[i]->numikrules != panim[0]->numikrules)
			{
				Assert( 0 );
				return false;
			}

			mstudioikrule_t *pRule = panim[i]->pIKRule( iRule );
			if (pRule == NULL)
				return false;

			float dt = 0.0;
			if (prevRule != NULL)
			{
				if (pRule->start - prevRule->start > 0.5)
				{
					dt = -1.0;
				}
				else if (pRule->start - prevRule->start < -0.5)
				{
					dt = 1.0;
				}
			}
			else
			{
				prevRule = pRule;
			}

			ikRule.start += (pRule->start + dt) * weight[i];
			ikRule.peak += (pRule->peak + dt) * weight[i];
			ikRule.tail += (pRule->tail + dt) * weight[i];
			ikRule.end += (pRule->end + dt) * weight[i];
		}
	}
	if (ikRule.start > 1.0)
	{
		ikRule.start -= 1.0;
		ikRule.peak -= 1.0;
		ikRule.tail -= 1.0;
		ikRule.end -= 1.0;
	}
	else if (ikRule.start < 0.0)
	{
		ikRule.start += 1.0;
		ikRule.peak += 1.0;
		ikRule.tail += 1.0;
		ikRule.end += 1.0;
	}

	ikRule.flWeight = Studio_IKRuleWeight( ikRule, flCycle );
	if (ikRule.flWeight <= 0.001f)
	{
		// go ahead and allow IK_GROUND rules a virtual looping section
		if ( panim[0]->pIKRule( iRule ) == NULL ) 
			return false;
		if ((panim[0]->flags & STUDIO_LOOPING) && panim[0]->pIKRule( iRule )->type == IK_GROUND && ikRule.end - ikRule.start > 0.75 )
		{
			ikRule.flWeight = 0.001;
			flCycle = ikRule.end - 0.001;
		}
		else
		{
			return false;
		}
	}

	Assert( ikRule.flWeight > 0.0f );

	ikRule.pos.Init();
	ikRule.q.Init();

	// find target error
	float total = 0.0f;
	for (i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			Vector pos1;
			Quaternion q1;
			float w;

			mstudioikrule_t *pRule = panim[i]->pIKRule( iRule );
			if (pRule == NULL)
				return false;

			ikRule.chain = pRule->chain;	// FIXME: this is anim local
			ikRule.bone = pRule->bone;		// FIXME: this is anim local
			ikRule.type = pRule->type;
			ikRule.slot = pRule->slot;

			ikRule.height += pRule->height * weight[i];
			ikRule.floor += pRule->floor * weight[i];
			ikRule.radius += pRule->radius * weight[i];
			ikRule.drop += pRule->drop * weight[i];
			ikRule.top += pRule->top * weight[i];

			// keep track of tail condition
			ikRule.release += Studio_IKTail( ikRule, flCycle ) * weight[i];

			// only check rules with error values
			switch( ikRule.type )
			{
			case IK_SELF:
			case IK_WORLD:
			case IK_GROUND:
			case IK_ATTACHMENT:
				{
					int bResult = Studio_IKAnimationError( pStudioHdr, pRule, panim[i], flCycle, pos1, q1, w );

					if (bResult)
					{
						ikRule.pos = ikRule.pos + pos1 * weight[i];
						QuaternionAccumulate( ikRule.q, weight[i], q1, ikRule.q );
						total += weight[i];
					}
				}
				break;
			default:
				total += weight[i];
				break;
			}

			ikRule.latched = Studio_IKShouldLatch( ikRule, flCycle ) * ikRule.flWeight;

			if (ikRule.type == IK_ATTACHMENT)
			{
				ikRule.szLabel = pRule->pszAttachment();
			}
		}
	}

	if (total <= 0.0001f)
	{
		return false;
	}

	if (total < 0.999f)
	{
		VectorScale( ikRule.pos, 1.0f / total, ikRule.pos );
		QuaternionScale( ikRule.q, 1.0f / total, ikRule.q );
	}

	if (ikRule.type == IK_SELF && ikRule.bone != -1)
	{
		// FIXME: this is anim local, not seq local!
		ikRule.bone = pStudioHdr->RemapSeqBone( iSequence, ikRule.bone );
		if (ikRule.bone == -1)
			return false;
	}

	QuaternionNormalize( ikRule.q );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


CIKContext::CIKContext()
{
	m_target.EnsureCapacity( 12 ); // FIXME: this sucks, shouldn't it be grown?
	m_iFramecounter = -1;
	m_pStudioHdr = NULL;
	m_flTime = -1.0f;
	m_target.SetSize( 0 );
}


void CIKContext::Init( const CStudioHdr *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime, int iFramecounter, int boneMask )
{
	m_pStudioHdr = pStudioHdr;
	m_ikChainRule.RemoveAll(); // m_numikrules = 0;
	if (pStudioHdr->numikchains())
	{
		m_ikChainRule.SetSize( pStudioHdr->numikchains() );

		// FIXME: Brutal hackery to prevent a crash
		if (m_target.Count() == 0)
		{
			m_target.SetSize(12);
			memset( m_target.Base(), 0, sizeof(m_target[0])*m_target.Count() );
			ClearTargets();
		}

	}
	else
	{
		m_target.SetSize( 0 );
	}
	AngleMatrix( angles, pos, m_rootxform );
	m_iFramecounter = iFramecounter;
	m_flTime = flTime;
	m_boneMask = boneMask;
}

void CIKContext::AddDependencies( mstudioseqdesc_t &seqdesc, int iSequence, float flCycle, const float poseParameters[], float flWeight )
{
	int i;

	if ( m_pStudioHdr->numikchains() == 0)
		return;

	if (seqdesc.numikrules == 0)
		return;

	ikcontextikrule_t ikrule;

	Assert( flWeight >= 0.0f && flWeight <= 1.0f );
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp( flWeight, 0.0f, 1.0f );

	// unify this
	if (seqdesc.flags & STUDIO_REALTIME)
	{
		float cps = Studio_CPS( m_pStudioHdr, seqdesc, iSequence, poseParameters );
		flCycle = m_flTime * cps;
		flCycle = flCycle - (int)flCycle;
	}
	else if (flCycle < 0 || flCycle >= 1)
	{
		if (seqdesc.flags & STUDIO_LOOPING)
		{
			flCycle = flCycle - (int)flCycle;
			if (flCycle < 0) flCycle += 1;
		}
		else
		{
			flCycle = max( 0.f, min( flCycle, 0.9999f ) );
		}
	}

	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( m_pStudioHdr, seqdesc, iSequence, poseParameters, panim, weight );

	// FIXME: add proper number of rules!!!
	for (i = 0; i < seqdesc.numikrules; i++)
	{
		if ( !Studio_IKSequenceError( m_pStudioHdr, seqdesc, iSequence, flCycle, i, poseParameters, panim, weight, ikrule ) )
			continue;

		// don't add rule if the bone isn't going to be calculated
		int bone = m_pStudioHdr->pIKChain( ikrule.chain )->pLink( 2 )->bone;
		if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
			continue;

		// or if its relative bone isn't going to be calculated
		if ( ikrule.bone >= 0 && !(m_pStudioHdr->boneFlags( ikrule.bone ) & m_boneMask))
			continue;

		// FIXME: Brutal hackery to prevent a crash
		if (m_target.Count() == 0)
		{
			m_target.SetSize(12);
			memset( m_target.Base(), 0, sizeof(m_target[0])*m_target.Count() );
			ClearTargets();
		}

		ikrule.flRuleWeight = flWeight;

		if (ikrule.flRuleWeight * ikrule.flWeight > 0.999)
		{
			if ( ikrule.type != IK_UNLATCH)
			{
				// clear out chain if rule is 100%
				m_ikChainRule.Element( ikrule.chain ).RemoveAll( );
				if ( ikrule.type == IK_RELEASE)
				{
					continue;
				}
			}
		}

 		int nIndex = m_ikChainRule.Element( ikrule.chain ).AddToTail( );
  		m_ikChainRule.Element( ikrule.chain ).Element( nIndex ) = ikrule;
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::AddAutoplayLocks( Vector pos[], Quaternion q[] )
{
	// skip all array access if no autoplay locks.
	if (m_pStudioHdr->GetNumIKAutoplayLocks() == 0)
	{
		return;
	}

	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;

	int ikOffset = m_ikLock.AddMultipleToTail( m_pStudioHdr->GetNumIKAutoplayLocks() );
	memset( &m_ikLock[ikOffset], 0, sizeof(ikcontextikrule_t)*m_pStudioHdr->GetNumIKAutoplayLocks() );

	for (int i = 0; i < m_pStudioHdr->GetNumIKAutoplayLocks(); i++)
	{
		const mstudioiklock_t &lock = ((CStudioHdr *)m_pStudioHdr)->pIKAutoplayLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( lock.chain );
		int bone = pchain->pLink( 2 )->bone;

		// don't bother with iklock if the bone isn't going to be calculated
		if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
			continue;

		// eval current ik'd bone
		BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

		ikcontextikrule_t &ikrule = m_ikLock[ i + ikOffset ];

		ikrule.chain = lock.chain;
		ikrule.slot = i;
		ikrule.type = IK_WORLD;

		MatrixAngles( boneToWorld[bone], ikrule.q, ikrule.pos );

		// save off current knee direction
		if (pchain->pLink(0)->kneeDir.LengthSqr() > 0.0)
		{
			Vector tmp = pchain->pLink( 0 )->kneeDir;
			VectorRotate( pchain->pLink( 0 )->kneeDir, boneToWorld[ pchain->pLink( 0 )->bone ], ikrule.kneeDir );
			MatrixPosition( boneToWorld[ pchain->pLink( 1 )->bone ], ikrule.kneePos ); 
		}
		else
		{
			ikrule.kneeDir.Init( );
		}
	}
	g_MatrixPool.Free( boneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::AddSequenceLocks( mstudioseqdesc_t &seqdesc, Vector pos[], Quaternion q[] )
{
	if ( m_pStudioHdr->numikchains() == 0)
	{
		return;
	}

	if ( seqdesc.numiklocks == 0 )
	{
		return;
	}

	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;

	int ikOffset = m_ikLock.AddMultipleToTail( seqdesc.numiklocks );
	memset( &m_ikLock[ikOffset], 0, sizeof(ikcontextikrule_t) * seqdesc.numiklocks );

	for (int i = 0; i < seqdesc.numiklocks; i++)
	{
		mstudioiklock_t *plock = seqdesc.pIKLock( i );
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
		int bone = pchain->pLink( 2 )->bone;

		// don't bother with iklock if the bone isn't going to be calculated
		if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
			continue;

		// eval current ik'd bone
		BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

		ikcontextikrule_t &ikrule = m_ikLock[i+ikOffset];
		ikrule.chain = i;
		ikrule.slot = i;
		ikrule.type = IK_WORLD;

		MatrixAngles( boneToWorld[bone], ikrule.q, ikrule.pos );

		// save off current knee direction
		if (pchain->pLink(0)->kneeDir.LengthSqr() > 0.0)
		{
			VectorRotate( pchain->pLink( 0 )->kneeDir, boneToWorld[ pchain->pLink( 0 )->bone ], ikrule.kneeDir );
		}
		else
		{
			ikrule.kneeDir.Init( );
		}
	}
	g_MatrixPool.Free( boneToWorld );
}

//-----------------------------------------------------------------------------
// Purpose: build boneToWorld transforms for a specific bone
//-----------------------------------------------------------------------------
void CIKContext::BuildBoneChain(
	const Vector pos[], 
	const Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	CBoneBitList &boneComputed )
{
	Assert( m_pStudioHdr->boneFlags( iBone ) & m_boneMask );
	::BuildBoneChain( m_pStudioHdr, m_rootxform, pos, q, iBone, pBoneToWorld, boneComputed );
}



//-----------------------------------------------------------------------------
// Purpose: build boneToWorld transforms for a specific bone
//-----------------------------------------------------------------------------
void BuildBoneChain(
	const CStudioHdr *pStudioHdr,
	const matrix3x4_t &rootxform,
	const Vector pos[], 
	const Quaternion q[], 
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	CBoneBitList &boneComputed )
{
	if ( boneComputed.IsBoneMarked(iBone) )
		return;

	matrix3x4_t bonematrix;
	QuaternionMatrix( q[iBone], pos[iBone], bonematrix );

	int parent = pStudioHdr->boneParent( iBone );
	if (parent == -1) 
	{
		ConcatTransforms( rootxform, bonematrix, pBoneToWorld[iBone] );
	}
	else
	{
		// evil recursive!!!
		BuildBoneChain( pStudioHdr, rootxform, pos, q, parent, pBoneToWorld, boneComputed );
		ConcatTransforms( pBoneToWorld[parent], bonematrix, pBoneToWorld[iBone]);
	}
	boneComputed.MarkBone(iBone);
}


//-----------------------------------------------------------------------------
// Purpose: turn a specific bones boneToWorld transform into a pos and q in parents bonespace
//-----------------------------------------------------------------------------
void SolveBone( 
	const CStudioHdr *pStudioHdr,
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	Vector pos[], 
	Quaternion q[]
	)
{
	int iParent = pStudioHdr->boneParent( iBone );

	matrix3x4_t worldToBone;
	MatrixInvert( pBoneToWorld[iParent], worldToBone );

	matrix3x4_t local;
	ConcatTransforms( worldToBone, pBoneToWorld[iBone], local );

	MatrixAngles( local, q[iBone], pos[iBone] );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CIKTarget::SetOwner( int entindex, const Vector &pos, const QAngle &angles )
{
	latched.owner = entindex;
	latched.absOrigin = pos;
	latched.absAngles = angles;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CIKTarget::ClearOwner( void )
{
	latched.owner = -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CIKTarget::GetOwner( void )
{
	return latched.owner;
}

//-----------------------------------------------------------------------------
// Purpose: update the latched IK values that are in a moving frame of reference
//-----------------------------------------------------------------------------

void CIKTarget::UpdateOwner( int entindex, const Vector &pos, const QAngle &angles )
{
	if (pos == latched.absOrigin && angles == latched.absAngles)
		return;

	matrix3x4_t in, out;
	AngleMatrix( angles, pos, in );
	AngleIMatrix( latched.absAngles, latched.absOrigin, out );

	matrix3x4_t tmp1, tmp2;
	QuaternionMatrix( latched.q, latched.pos, tmp1 );
	ConcatTransforms( out, tmp1, tmp2 );
	ConcatTransforms( in, tmp2, tmp1 );
	MatrixAngles( tmp1, latched.q, latched.pos );
}


//-----------------------------------------------------------------------------
// Purpose: sets the ground position of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetPos( const Vector &pos )
{
	est.pos = pos;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ground "identity" orientation of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetAngles( const QAngle &angles )
{
	AngleQuaternion( angles, est.q );
}

//-----------------------------------------------------------------------------
// Purpose: sets the ground "identity" orientation of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetQuaternion( const Quaternion &q )
{
	est.q = q;
}

//-----------------------------------------------------------------------------
// Purpose: calculates a ground "identity" orientation based on the surface
//			normal of the ground and the desired ground identity orientation
//-----------------------------------------------------------------------------

void CIKTarget::SetNormal( const Vector &normal )
{
	// recalculate foot angle based on slope of surface
	matrix3x4_t m1;
	Vector forward, right;
	QuaternionMatrix( est.q, m1 );

	MatrixGetColumn( m1, 1, right );
	forward = CrossProduct( right, normal );
	right = CrossProduct( normal, forward );
	MatrixSetColumn( forward, 0, m1 );
	MatrixSetColumn( right, 1, m1 );
	MatrixSetColumn( normal, 2, m1 );
	Vector p1;
	MatrixAngles( m1, est.q, p1 );
}


//-----------------------------------------------------------------------------
// Purpose: estimates the ground impact at the center location assuming a the edge of 
//			an Z axis aligned disc collided with it the surface.
//-----------------------------------------------------------------------------

void CIKTarget::SetPosWithNormalOffset( const Vector &pos, const Vector &normal )
{
	// assume it's a disc edge intersecting with the floor, so try to estimate the z location of the center
	est.pos = pos;
	if (normal.z > 0.9999)
	{
		return;
	}
	// clamp at 45 degrees
	else if (normal.z > 0.707)
	{
		// tan == sin / cos
		float tan = sqrt( 1 - normal.z * normal.z ) / normal.z;
		est.pos.z = est.pos.z - est.radius * tan;
	}
	else
	{
		est.pos.z = est.pos.z - est.radius;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::SetOnWorld( bool bOnWorld )
{
	est.onWorld = bOnWorld;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool CIKTarget::IsActive()
{ 
	return (est.flWeight > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::IKFailed( void )
{
	latched.deltaPos.Init();
	latched.deltaQ.Init();
	latched.pos = ideal.pos;
	latched.q = ideal.q;
	est.latched = 0.0;
	est.flWeight = 0.0;
	est.onWorld = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::MoveReferenceFrame( Vector &deltaPos, QAngle &deltaAngles )
{
	est.pos -= deltaPos;
	latched.pos -= deltaPos;
	offset.pos -= deltaPos;
	ideal.pos -= deltaPos;
}



//-----------------------------------------------------------------------------
// Purpose: Invalidate any IK locks.
//-----------------------------------------------------------------------------

void CIKContext::ClearTargets( void )
{
	int i;
	for (i = 0; i < m_target.Count(); i++)
	{
		m_target[i].latched.iFramecounter = -9999;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Run through the rules that survived and turn a specific bones boneToWorld 
//			transform into a pos and q in parents bonespace
//-----------------------------------------------------------------------------

void CIKContext::UpdateTargets( Vector pos[], Quaternion q[], matrix3x4_t boneToWorld[], CBoneBitList &boneComputed )
{
	int i, j;

	for (i = 0; i < m_target.Count(); i++)
	{
		m_target[i].est.flWeight = 0.0f;
		m_target[i].est.latched = 1.0f;
		m_target[i].est.release = 1.0f;
		m_target[i].est.height = 0.0f;
		m_target[i].est.floor = 0.0f;
		m_target[i].est.radius = 0.0f;
		m_target[i].offset.pos.Init();
		m_target[i].offset.q.Init();
	}

	AutoIKRelease( );

	for (j = 0; j < m_ikChainRule.Count(); j++)
	{
		for (i = 0; i < m_ikChainRule.Element( j ).Count(); i++)
		{
			ikcontextikrule_t *pRule = &m_ikChainRule.Element( j ).Element( i );

			// ikchainresult_t *pChainRule = &chainRule[ m_ikRule[i].chain ];

			switch( pRule->type )
			{
			case IK_ATTACHMENT:
			case IK_GROUND:
			// case IK_SELF:
				{
					CIKTarget *pTarget = &m_target[pRule->slot];
					pTarget->chain = pRule->chain;
					pTarget->type = pRule->type;

					if (pRule->type == IK_ATTACHMENT)
					{
						pTarget->offset.pAttachmentName = pRule->szLabel;
					}
					else
					{
						pTarget->offset.pAttachmentName = NULL;
					}

					if (pRule->flRuleWeight == 1.0f || pTarget->est.flWeight == 0.0f)
					{
						pTarget->offset.q = pRule->q;
						pTarget->offset.pos = pRule->pos;
						pTarget->est.height = pRule->height;
						pTarget->est.floor = pRule->floor;
						pTarget->est.radius = pRule->radius;
						pTarget->est.latched = pRule->latched * pRule->flRuleWeight;
						pTarget->est.release = pRule->release;
						pTarget->est.flWeight = pRule->flWeight * pRule->flRuleWeight;
					}
					else
					{
						QuaternionSlerp( pTarget->offset.q, pRule->q, pRule->flRuleWeight, pTarget->offset.q );
						pTarget->offset.pos = Lerp( pRule->flRuleWeight, pTarget->offset.pos, pRule->pos );
						pTarget->est.height = Lerp( pRule->flRuleWeight, pTarget->est.height, pRule->height );
						pTarget->est.floor = Lerp( pRule->flRuleWeight, pTarget->est.floor, pRule->floor );
						pTarget->est.radius = Lerp( pRule->flRuleWeight, pTarget->est.radius, pRule->radius );
						//pTarget->est.latched = Lerp( pRule->flRuleWeight, pTarget->est.latched, pRule->latched );
						pTarget->est.latched = min( pTarget->est.latched, pRule->latched );
						pTarget->est.release = Lerp( pRule->flRuleWeight, pTarget->est.release, pRule->release );
						pTarget->est.flWeight = Lerp( pRule->flRuleWeight, pTarget->est.flWeight, pRule->flWeight );
					}

					if ( pRule->type == IK_GROUND )
					{
						pTarget->latched.deltaPos.z = 0;
						pTarget->est.pos.z = pTarget->est.floor + m_rootxform[2][3];
					}
				}
			break;
			case IK_UNLATCH:
				{
					CIKTarget *pTarget = &m_target[pRule->slot];
					if (pRule->latched > 0.0)
						pTarget->est.latched = 0.0;
					else
						pTarget->est.latched = min( pTarget->est.latched, 1.0f - pRule->flWeight );
				}
				break;
			case IK_RELEASE:
				{
					CIKTarget *pTarget = &m_target[pRule->slot];
					if (pRule->latched > 0.0)
						pTarget->est.latched = 0.0;
					else
						pTarget->est.latched = min( pTarget->est.latched, 1.0f - pRule->flWeight );

					pTarget->est.flWeight = (pTarget->est.flWeight) * (1 - pRule->flWeight * pRule->flRuleWeight);
				}
				break;
			}
		}
	}

	for (i = 0; i < m_target.Count(); i++)
	{
		CIKTarget *pTarget = &m_target[i];
		if (pTarget->est.flWeight > 0.0)
		{
			mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( pTarget->chain );
			// ikchainresult_t *pChainRule = &chainRule[ i ];
			int bone = pchain->pLink( 2 )->bone;

			// eval current ik'd bone
			BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

			// xform IK target error into world space
			matrix3x4_t local;
			matrix3x4_t worldFootpad;
			QuaternionMatrix( pTarget->offset.q, pTarget->offset.pos, local );
			MatrixInvert( local, local );
			ConcatTransforms( boneToWorld[bone], local, worldFootpad );

			if (pTarget->est.latched == 1.0)
			{
				pTarget->latched.bNeedsLatch = true;
			}
			else
			{
				pTarget->latched.bNeedsLatch = false;
			}

			// disable latched position if it looks invalid
			if (m_iFramecounter < 0 || pTarget->latched.iFramecounter < m_iFramecounter - 1 || pTarget->latched.iFramecounter > m_iFramecounter)
			{
				pTarget->latched.bHasLatch = false;
				pTarget->latched.influence = 0.0;
			}
			pTarget->latched.iFramecounter = m_iFramecounter;

			// find ideal contact position
			MatrixAngles( worldFootpad, pTarget->ideal.q, pTarget->ideal.pos );
			pTarget->est.q = pTarget->ideal.q;
			pTarget->est.pos = pTarget->ideal.pos;

			float latched = pTarget->est.latched;

			if (pTarget->latched.bHasLatch)
			{
				if (pTarget->est.latched == 1.0)
				{
					// keep track of latch position error from ideal contact position
					pTarget->latched.deltaPos = pTarget->latched.pos - pTarget->est.pos;
					QuaternionSM( -1, pTarget->est.q, pTarget->latched.q, pTarget->latched.deltaQ );
					pTarget->est.q = pTarget->latched.q;
					pTarget->est.pos = pTarget->latched.pos;
				}
				else if (pTarget->est.latched > 0.0)
				{
					// ramp out latch differences during decay phase of rule
					if (latched > 0 && latched < pTarget->latched.influence)
					{
						// latching has decreased
						float dt = pTarget->latched.influence - latched;
						if (pTarget->latched.influence > 0.0)
							dt = dt / pTarget->latched.influence;

						VectorScale( pTarget->latched.deltaPos, (1-dt), pTarget->latched.deltaPos );
						QuaternionScale( pTarget->latched.deltaQ, (1-dt), pTarget->latched.deltaQ );
					}

					// move ideal contact position by latched error factor
					pTarget->est.pos = pTarget->est.pos + pTarget->latched.deltaPos;
					QuaternionMA( pTarget->est.q, 1, pTarget->latched.deltaQ, pTarget->est.q );
					pTarget->latched.q = pTarget->est.q;
					pTarget->latched.pos = pTarget->est.pos;
				}
				else
				{
					pTarget->latched.bHasLatch = false;
					pTarget->latched.q = pTarget->est.q;
					pTarget->latched.pos = pTarget->est.pos;
					pTarget->latched.deltaPos.Init();
					pTarget->latched.deltaQ.Init();
				}
				pTarget->latched.influence = latched;
			}

			// check for illegal requests
			Vector p1, p2, p3;
			MatrixPosition( boneToWorld[pchain->pLink( 0 )->bone], p1 ); // hip
			MatrixPosition( boneToWorld[pchain->pLink( 1 )->bone], p2 ); // knee
			MatrixPosition( boneToWorld[pchain->pLink( 2 )->bone], p3 ); // foot

			float d1 = (p2 - p1).Length();
			float d2 = (p3 - p2).Length();

			if (pTarget->latched.bHasLatch)
			{
				//float d3 = (p3 - p1).Length();
				float d4 = (p3 + pTarget->latched.deltaPos - p1).Length();

				// unstick feet when distance is too great
				if ((d4 < fabs( d1 - d2 ) || d4 * 0.95 > d1 + d2) && pTarget->est.latched > 0.2)
				{
					pTarget->error.flTime = m_flTime;
				}

				// unstick feet when angle is too great
				if (pTarget->est.latched > 0.2)
				{
					float d = fabs( pTarget->latched.deltaQ.w ) * 2.0f - 1.0f; // QuaternionDotProduct( pTarget->latched.q, pTarget->est.q );

					// FIXME: cos(45), make property of chain
					if (d < 0.707)
					{
						pTarget->error.flTime = m_flTime;
					}
				}
			}

			Vector dt = pTarget->est.pos - p1;
			pTarget->trace.hipToFoot = VectorNormalize( dt );
			pTarget->trace.hipToKnee = d1;
			pTarget->trace.kneeToFoot = d2;
			pTarget->trace.hip = p1;
			pTarget->trace.knee = p2;
			pTarget->trace.closest = p1 + dt * (fabs( d1 - d2 ) * 1.01);
			pTarget->trace.farthest = p1 + dt * (d1 + d2) * 0.99;
			pTarget->trace.lowest = p1 + Vector( 0, 0, -1 ) * (d1 + d2) * 0.99;
			// pTarget->trace.endpos = pTarget->est.pos;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: insert release rules if the ik rules were in error
//-----------------------------------------------------------------------------

void CIKContext::AutoIKRelease( void )
{
	int i;

	for (i = 0; i < m_target.Count(); i++)
	{
		CIKTarget *pTarget = &m_target[i];

		float dt = m_flTime - pTarget->error.flTime;
		if (pTarget->error.bInError || dt < 0.5)
		{
			if (!pTarget->error.bInError)
			{
				pTarget->error.ramp = 0.0; 
				pTarget->error.flErrorTime = pTarget->error.flTime;
				pTarget->error.bInError = true;
			}

			float ft = m_flTime - pTarget->error.flErrorTime;
			if (dt < 0.25)
			{
				pTarget->error.ramp = min( pTarget->error.ramp + ft * 4.0, 1.0 );
			}
			else
			{
				pTarget->error.ramp = max( pTarget->error.ramp - ft * 4.0, 0.0 );
			}
			if (pTarget->error.ramp > 0.0)
			{
				ikcontextikrule_t ikrule;

				ikrule.chain = pTarget->chain;
				ikrule.bone = 0;
				ikrule.type = IK_RELEASE;
				ikrule.slot = i;
				ikrule.flWeight = SimpleSpline( pTarget->error.ramp );
				ikrule.flRuleWeight = 1.0;
				ikrule.latched = dt < 0.25 ? 0.0 : ikrule.flWeight;

				// don't bother with AutoIKRelease if the bone isn't going to be calculated
				// this code is crashing for some unknown reason.
				if ( pTarget->chain >= 0 && pTarget->chain < m_pStudioHdr->numikchains())
				{
					mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( pTarget->chain );
					if (pchain != NULL)
					{
						int bone = pchain->pLink( 2 )->bone;
						if (bone >= 0 && bone < m_pStudioHdr->numbones())
						{
							mstudiobone_t *pBone = m_pStudioHdr->pBone( bone );
							if (pBone != NULL)
							{
								if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
								{
									pTarget->error.bInError = false;
									continue;
								}
								/*
								char buf[256];
								sprintf( buf, "dt %.4f ft %.4f weight %.4f latched %.4f\n", dt, ft, ikrule.flWeight, ikrule.latched );
								OutputDebugString( buf );
								*/

								int nIndex = m_ikChainRule.Element( ikrule.chain ).AddToTail( );
  								m_ikChainRule.Element( ikrule.chain ).Element( nIndex ) = ikrule;
							}
							else
							{
								DevWarning( 1, "AutoIKRelease (%s) got a NULL pBone %d\n", m_pStudioHdr->pszName(), bone );
							}
						}
						else
						{
							DevWarning( 1, "AutoIKRelease (%s) got an out of range bone %d (%d)\n", m_pStudioHdr->pszName(), bone, m_pStudioHdr->numbones() );
						}
					}
					else
					{
						DevWarning( 1, "AutoIKRelease (%s) got a NULL pchain %d\n", m_pStudioHdr->pszName(), pTarget->chain );
					}
				}
				else
				{
					DevWarning( 1, "AutoIKRelease (%s) got an out of range chain %d (%d)\n", m_pStudioHdr->pszName(), pTarget->chain, m_pStudioHdr->numikchains());
				}
			}
			else
			{
				pTarget->error.bInError = false;
			}
			pTarget->error.flErrorTime = m_flTime;
		}
	}
}



void CIKContext::SolveDependencies( Vector pos[], Quaternion q[], matrix3x4_t boneToWorld[], CBoneBitList &boneComputed	)
{
//	ASSERT_NO_REENTRY();
	
	matrix3x4_t worldTarget;
	int i, j;

	ikchainresult_t chainResult[32]; // allocate!!!

	// init chain rules
	for (i = 0; i < m_pStudioHdr->numikchains(); i++)
	{
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( i );
		ikchainresult_t *pChainResult = &chainResult[ i ];
		int bone = pchain->pLink( 2 )->bone;

		pChainResult->target = -1;
		pChainResult->flWeight = 0.0;

		// don't bother with chain if the bone isn't going to be calculated
		if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
			continue;

		// eval current ik'd bone
		BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

		MatrixAngles( boneToWorld[bone], pChainResult->q, pChainResult->pos );
	}

	for (j = 0; j < m_ikChainRule.Count(); j++)
	{
		for (i = 0; i < m_ikChainRule.Element( j ).Count(); i++)
		{
			ikcontextikrule_t *pRule = &m_ikChainRule.Element( j ).Element( i );
			ikchainresult_t *pChainResult = &chainResult[ pRule->chain ];
			pChainResult->target = -1;


			switch( pRule->type )
			{
			case IK_SELF:
				{
					// xform IK target error into world space
					matrix3x4_t local;
					QuaternionMatrix( pRule->q, pRule->pos, local );
					// eval target bone space
					if (pRule->bone != -1)
					{
						BuildBoneChain( pos, q, pRule->bone, boneToWorld, boneComputed );
						ConcatTransforms( boneToWorld[pRule->bone], local, worldTarget );
					}
					else
					{
						ConcatTransforms( m_rootxform, local, worldTarget );
					}
			
					float flWeight = pRule->flWeight * pRule->flRuleWeight;
					pChainResult->flWeight = pChainResult->flWeight * (1 - flWeight) + flWeight;

					Vector p2;
					Quaternion q2;
					
					// target p and q
					MatrixAngles( worldTarget, q2, p2 );

					// debugLine( pChainResult->pos, p2, 0, 0, 255, true, 0.1 );

					// blend in position and angles
					pChainResult->pos = pChainResult->pos * (1.0 - flWeight) + p2 * flWeight;
					QuaternionSlerp( pChainResult->q, q2, flWeight, pChainResult->q );
				}
				break;
			case IK_WORLD:
				Assert( 0 );
				break;

			case IK_ATTACHMENT:
				break;

			case IK_GROUND:
				break;

			case IK_RELEASE:
				{
					// move target back towards original location
					float flWeight = pRule->flWeight * pRule->flRuleWeight;
					mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( pRule->chain );
					int bone = pchain->pLink( 2 )->bone;

					Vector p2;
					Quaternion q2;
					
					BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );
					MatrixAngles( boneToWorld[bone], q2, p2 );

					// blend in position and angles
					pChainResult->pos = pChainResult->pos * (1.0 - flWeight) + p2 * flWeight;
					QuaternionSlerp( pChainResult->q, q2, flWeight, pChainResult->q );
				}
				break;
			case IK_UNLATCH:
				{
					/*
					pChainResult->flWeight = pChainResult->flWeight * (1 - pRule->flWeight) + pRule->flWeight;

					pChainResult->pos = pChainResult->pos * (1.0 - pRule->flWeight ) + pChainResult->local.pos * pRule->flWeight;
					QuaternionSlerp( pChainResult->q, pChainResult->local.q, pRule->flWeight, pChainResult->q );
					*/
				}
				break;
			}
		}
	}

	for (i = 0; i < m_target.Count(); i++)
	{
		CIKTarget *pTarget = &m_target[i];

		if (m_target[i].est.flWeight > 0.0)
		{
			matrix3x4_t worldFootpad;
			matrix3x4_t local;
			//mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( m_target[i].chain );
			ikchainresult_t *pChainResult = &chainResult[ pTarget->chain ];

			AngleMatrix(pTarget->offset.q, pTarget->offset.pos, local );

			AngleMatrix( pTarget->est.q, pTarget->est.pos, worldFootpad );

			ConcatTransforms( worldFootpad, local, worldTarget );

			Vector p2;
			Quaternion q2;
			// target p and q
			MatrixAngles( worldTarget, q2, p2 );
			// MatrixAngles( worldTarget, pChainResult->q, pChainResult->pos );

			// blend in position and angles
			pChainResult->flWeight = pTarget->est.flWeight;
			pChainResult->pos = pChainResult->pos * (1.0 - pChainResult->flWeight ) + p2 * pChainResult->flWeight;
			QuaternionSlerp( pChainResult->q, q2, pChainResult->flWeight, pChainResult->q );
		}

		if (pTarget->latched.bNeedsLatch)
		{
			// keep track of latch position
			pTarget->latched.bHasLatch = true;
			pTarget->latched.q = pTarget->est.q;
			pTarget->latched.pos = pTarget->est.pos;
		}
	}

	for (i = 0; i < m_pStudioHdr->numikchains(); i++)
	{
		ikchainresult_t *pChainResult = &chainResult[ i ];
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( i );

		if (pChainResult->flWeight > 0.0)
		{
			Vector tmp;
			MatrixPosition( boneToWorld[pchain->pLink( 2 )->bone], tmp );
			// debugLine( pChainResult->pos, tmp, 255, 255, 255, true, 0.1 );

			// do exact IK solution
			// FIXME: once per link!
			if (Studio_SolveIK(pchain, pChainResult->pos, boneToWorld ))
			{
				Vector p3;
				MatrixGetColumn( boneToWorld[pchain->pLink( 2 )->bone], 3, p3 );
				QuaternionMatrix( pChainResult->q, p3, boneToWorld[pchain->pLink( 2 )->bone] );

				// rebuild chain
				// FIXME: is this needed if everyone past this uses the boneToWorld array?
				SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
				SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
				SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
			}
			else
			{
				// FIXME: need to invalidate the targets that forced this...
				if (pChainResult->target != -1)
				{
					CIKTarget *pTarget = &m_target[pChainResult->target];
					VectorScale( pTarget->latched.deltaPos, 0.8, pTarget->latched.deltaPos );
					QuaternionScale( pTarget->latched.deltaQ, 0.8, pTarget->latched.deltaQ );
				}
			}
		}
	}

#if 0
		Vector p1, p2, p3;
		Quaternion q1, q2, q3;

		// current p and q
		MatrixAngles( boneToWorld[bone], q1, p1 );

		
		// target p and q
		MatrixAngles( worldTarget, q2, p2 );

		// blend in position and angles
		p3 = p1 * (1.0 - m_ikRule[i].flWeight ) + p2 * m_ikRule[i].flWeight;

		// do exact IK solution
		// FIXME: once per link!
		Studio_SolveIK(pchain, p3, boneToWorld );

		// force angle (bad?)
		QuaternionSlerp( q1, q2, m_ikRule[i].flWeight, q3 );
		MatrixGetColumn( boneToWorld[bone], 3, p3 );
		QuaternionMatrix( q3, p3, boneToWorld[bone] );

		// rebuild chain
		SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
		SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
#endif
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::SolveAutoplayLocks(
	Vector pos[], 
	Quaternion q[]
	)
{
	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;
	int i;

	for (i = 0; i < m_ikLock.Count(); i++)
	{
		const mstudioiklock_t &lock = ((CStudioHdr *)m_pStudioHdr)->pIKAutoplayLock( i );
		SolveLock( &lock, i, pos, q, boneToWorld, boneComputed );
	}
	g_MatrixPool.Free( boneToWorld );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::SolveSequenceLocks(
	mstudioseqdesc_t &seqdesc,
	Vector pos[], 
	Quaternion q[]
	)
{
	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;
	int i;

	for (i = 0; i < m_ikLock.Count(); i++)
	{
		mstudioiklock_t *plock = seqdesc.pIKLock( i );
		SolveLock( plock, i, pos, q, boneToWorld, boneComputed );
	}
	g_MatrixPool.Free( boneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKContext::AddAllLocks( Vector pos[], Quaternion q[] )
{
	// skip all array access if no autoplay locks.
	if (m_pStudioHdr->GetNumIKChains() == 0)
	{
		return;
	}

	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;

	int ikOffset = m_ikLock.AddMultipleToTail( m_pStudioHdr->GetNumIKChains() );
	memset( &m_ikLock[ikOffset], 0, sizeof(ikcontextikrule_t)*m_pStudioHdr->GetNumIKChains() );

	for (int i = 0; i < m_pStudioHdr->GetNumIKChains(); i++)
	{
		mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( i );
		int bone = pchain->pLink( 2 )->bone;

		// don't bother with iklock if the bone isn't going to be calculated
		if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
			continue;

		// eval current ik'd bone
		BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

		ikcontextikrule_t &ikrule = m_ikLock[ i + ikOffset ];

		ikrule.chain = i;
		ikrule.slot = i;
		ikrule.type = IK_WORLD;

		MatrixAngles( boneToWorld[bone], ikrule.q, ikrule.pos );

		// save off current knee direction
		if (pchain->pLink(0)->kneeDir.LengthSqr() > 0.0)
		{
			Vector tmp = pchain->pLink( 0 )->kneeDir;
			VectorRotate( pchain->pLink( 0 )->kneeDir, boneToWorld[ pchain->pLink( 0 )->bone ], ikrule.kneeDir );
			MatrixPosition( boneToWorld[ pchain->pLink( 1 )->bone ], ikrule.kneePos ); 
		}
		else
		{
			ikrule.kneeDir.Init( );
		}
	}
	g_MatrixPool.Free( boneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


void CIKContext::SolveAllLocks(
	Vector pos[], 
	Quaternion q[]
	)
{
	matrix3x4_t *boneToWorld = g_MatrixPool.Alloc();
	CBoneBitList boneComputed;
	int i;

	mstudioiklock_t lock;

	for (i = 0; i < m_ikLock.Count(); i++)
	{
		lock.chain = i;
		lock.flPosWeight = 1.0;
		lock.flLocalQWeight = 0.0;
		lock.flags = 0;

		SolveLock( &lock, i, pos, q, boneToWorld, boneComputed );
	}
	g_MatrixPool.Free( boneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


void CIKContext::SolveLock(
	const mstudioiklock_t *plock,
	int i,
	Vector pos[], 
	Quaternion q[],
	matrix3x4_t boneToWorld[], 
	CBoneBitList &boneComputed
	)
{
	mstudioikchain_t *pchain = m_pStudioHdr->pIKChain( plock->chain );
	int bone = pchain->pLink( 2 )->bone;

	// don't bother with iklock if the bone isn't going to be calculated
	if ( !(m_pStudioHdr->boneFlags( bone ) & m_boneMask))
		return;

	// eval current ik'd bone
	BuildBoneChain( pos, q, bone, boneToWorld, boneComputed );

	Vector p1, p3;
	Quaternion q2;

	// current p and q
	MatrixPosition( boneToWorld[bone], p1 );

	// blend in position
	p3 = p1 * (1.0 - plock->flPosWeight ) + m_ikLock[i].pos * plock->flPosWeight;

	// do exact IK solution
	if (m_ikLock[i].kneeDir.LengthSqr() > 0)
	{
		Studio_SolveIK(pchain->pLink( 0 )->bone, pchain->pLink( 1 )->bone, pchain->pLink( 2 )->bone, p3, m_ikLock[i].kneePos, m_ikLock[i].kneeDir, boneToWorld );
	}
	else
	{
		Studio_SolveIK(pchain, p3, boneToWorld );
	}

	// slam orientation
	MatrixPosition( boneToWorld[bone], p3 );
	QuaternionMatrix( m_ikLock[i].q, p3, boneToWorld[bone] );

	// rebuild chain
	q2 = q[ bone ];
	SolveBone( m_pStudioHdr, pchain->pLink( 2 )->bone, boneToWorld, pos, q );
	QuaternionSlerp( q[bone], q2, plock->flLocalQWeight, q[bone] );

	SolveBone( m_pStudioHdr, pchain->pLink( 1 )->bone, boneToWorld, pos, q );
	SolveBone( m_pStudioHdr, pchain->pLink( 0 )->bone, boneToWorld, pos, q );
}


//-----------------------------------------------------------------------------
// Purpose: run all animations that automatically play and are driven off of poseParameters
//-----------------------------------------------------------------------------
void CBoneSetup::CalcAutoplaySequences(
   Vector pos[], 
   Quaternion q[], 
   float flRealTime,
   CIKContext *pIKContext
   )
{
	//	ASSERT_NO_REENTRY();

	int			i;
	if ( pIKContext )
	{
		pIKContext->AddAutoplayLocks( pos, q );
	}

	unsigned short *pList = NULL;
	int count = m_pStudioHdr->GetAutoplayList( &pList );
	for (i = 0; i < count; i++)
	{
		int sequenceIndex = pList[i];
		mstudioseqdesc_t &seqdesc = ((CStudioHdr *)m_pStudioHdr)->pSeqdesc( sequenceIndex );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			float cycle = 0;
			float cps = Studio_CPS( m_pStudioHdr, seqdesc, sequenceIndex, m_flPoseParameter );
			cycle = flRealTime * cps;
			cycle = cycle - (int)cycle;

			AccumulatePose( pos, q, sequenceIndex, cycle, 1.0, flRealTime, pIKContext );
		}
	}

	if ( pIKContext )
	{
		pIKContext->SolveAutoplayLocks( pos, q );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Studio_BuildMatrices(
	const CStudioHdr *pStudioHdr,
	const QAngle& angles, 
	const Vector& origin, 
	const Vector pos[],
	const Quaternion q[],
	int iBone,
	float flScale,
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	int boneMask
	)
{
	int i, j;

	int					chain[MAXSTUDIOBONES] = {};
	int					chainlength = 0;

	if (iBone < -1 || iBone >= pStudioHdr->numbones())
		iBone = 0;

	// build list of what bones to use
	if (iBone == -1)
	{
		// all bones
		chainlength = pStudioHdr->numbones();
		for (i = 0; i < pStudioHdr->numbones(); i++)
		{
			chain[chainlength - i - 1] = i;
		}
	}
	else
	{
		// only the parent bones
		i = iBone;
		while (i != -1)
		{
			chain[chainlength++] = i;
			i = pStudioHdr->boneParent( i );
		}
	}

	matrix3x4_t bonematrix;
	matrix3x4_t rotationmatrix; // model to world transformation
	AngleMatrix( angles, origin, rotationmatrix );

	// Account for a change in scale
	if ( flScale < 1.0f-FLT_EPSILON || flScale > 1.0f+FLT_EPSILON )
	{
		Vector vecOffset;
		MatrixGetColumn( rotationmatrix, 3, vecOffset );
		vecOffset -= origin;
		vecOffset *= flScale;
		vecOffset += origin;
		MatrixSetColumn( vecOffset, 3, rotationmatrix );

		// Scale it uniformly
		VectorScale( rotationmatrix[0], flScale, rotationmatrix[0] );
		VectorScale( rotationmatrix[1], flScale, rotationmatrix[1] );
		VectorScale( rotationmatrix[2], flScale, rotationmatrix[2] );
	}

	for (j = chainlength - 1; j >= 0; j--)
	{
		i = chain[j];
		if (pStudioHdr->boneFlags(i) & boneMask)
		{
			QuaternionMatrix( q[i], pos[i], bonematrix );

			if (pStudioHdr->boneParent(i) == -1) 
			{
				ConcatTransforms (rotationmatrix, bonematrix, bonetoworld[i]);
			} 
			else 
			{
				ConcatTransforms (bonetoworld[pStudioHdr->boneParent(i)], bonematrix, bonetoworld[i]);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: look at single column vector of another bones local transformation 
//			and generate a procedural transformation based on how that column 
//			points down the 6 cardinal axis (all negative weights are clamped to 0).
//-----------------------------------------------------------------------------

void DoAxisInterpBone(
	mstudiobone_t		*pbones,
	int	ibone,
	CBoneAccessor &bonetoworld
	)
{
	matrix3x4_t			bonematrix;
	Vector				control;

	mstudioaxisinterpbone_t *pProc = (mstudioaxisinterpbone_t *)pbones[ibone].pProcedure( );
	const matrix3x4_t &controlBone = bonetoworld.GetBone( pProc->control );
	if (pProc && pbones[pProc->control].parent != -1)
	{
		Vector tmp;
		// pull out the control column
		tmp.x = controlBone[0][pProc->axis];
		tmp.y = controlBone[1][pProc->axis];
		tmp.z = controlBone[2][pProc->axis];

		// invert it back into parent's space.
		VectorIRotate( tmp, bonetoworld.GetBone( pbones[pProc->control].parent ), control );
#if 0
		matrix3x4_t	tmpmatrix;
		matrix3x4_t	controlmatrix;
		MatrixInvert( bonetoworld.GetBone( pbones[pProc->control].parent ), tmpmatrix );
		ConcatTransforms( tmpmatrix, bonetoworld.GetBone( pProc->control ), controlmatrix );

		// pull out the control column
		control.x = controlmatrix[0][pProc->axis];
		control.y = controlmatrix[1][pProc->axis];
		control.z = controlmatrix[2][pProc->axis];
#endif
	}
	else
	{
		// pull out the control column
		control.x = controlBone[0][pProc->axis];
		control.y = controlBone[1][pProc->axis];
		control.z = controlBone[2][pProc->axis];
	}

	Quaternion *q1, *q2, *q3;
	Vector *p1, *p2, *p3;

	// find axial control inputs
	float a1 = control.x;
	float a2 = control.y;
	float a3 = control.z;
	if (a1 >= 0) 
	{ 
		q1 = &pProc->quat[0];
		p1 = &pProc->pos[0];
	} 
	else 
	{ 
		a1 = -a1; 
		q1 = &pProc->quat[1];
		p1 = &pProc->pos[1];
	}

	if (a2 >= 0) 
	{ 
		q2 = &pProc->quat[2]; 
		p2 = &pProc->pos[2];
	} 
	else 
	{ 
		a2 = -a2; 
		q2 = &pProc->quat[3]; 
		p2 = &pProc->pos[3];
	}

	if (a3 >= 0) 
	{ 
		q3 = &pProc->quat[4]; 
		p3 = &pProc->pos[4];
	} 
	else 
	{ 
		a3 = -a3; 
		q3 = &pProc->quat[5]; 
		p3 = &pProc->pos[5];
	}

	// do a three-way blend
	Vector p;
	Quaternion v, tmp;
	if (a1 + a2 > 0)
	{
		float t = 1.0 / (a1 + a2 + a3);
		// FIXME: do a proper 3-way Quat blend!
		QuaternionSlerp( *q2, *q1, a1 / (a1 + a2), tmp );
		QuaternionSlerp( tmp, *q3, a3 * t, v );
		VectorScale( *p1, a1 * t, p );
		VectorMA( p, a2 * t, *p2, p );
		VectorMA( p, a3 * t, *p3, p );
	}
	else
	{
		QuaternionSlerp( *q3, *q3, 0, v ); // ??? no quat copy?
		p = *p3;
	}

	QuaternionMatrix( v, p, bonematrix );

	ConcatTransforms (bonetoworld.GetBone( pbones[ibone].parent ), bonematrix, bonetoworld.GetBoneForWrite( ibone ));
}



//-----------------------------------------------------------------------------
// Purpose: Generate a procedural transformation based on how that another bones 
//			local transformation matches a set of target orientations.
//-----------------------------------------------------------------------------
void DoQuatInterpBone(
	mstudiobone_t		*pbones,
	int	ibone,
	CBoneAccessor &bonetoworld
	)
{
	matrix3x4_t			bonematrix;

	mstudioquatinterpbone_t *pProc = (mstudioquatinterpbone_t *)pbones[ibone].pProcedure( );
	if (pProc && pbones[pProc->control].parent != -1)
	{
		Quaternion	src;
		float		weight[32];
		float		scale = 0.0;
		Quaternion	quat;
		Vector		pos;

		matrix3x4_t	tmpmatrix;
		matrix3x4_t	controlmatrix;
		MatrixInvert( bonetoworld.GetBone( pbones[pProc->control].parent), tmpmatrix );
		ConcatTransforms( tmpmatrix, bonetoworld.GetBone( pProc->control ), controlmatrix );

		MatrixAngles( controlmatrix, src, pos ); // FIXME: make a version without pos

		int i;
		for (i = 0; i < pProc->numtriggers; i++)
		{
			float dot = fabs( QuaternionDotProduct( pProc->pTrigger( i )->trigger, src ) );
			// FIXME: a fast acos should be acceptable
			dot = clamp( dot, -1.f, 1.f );
			weight[i] = 1 - (2 * acos( dot ) * pProc->pTrigger( i )->inv_tolerance );
			weight[i] = max( 0.f, weight[i] );
			scale += weight[i];
		}

		if (scale <= 0.001)  // EPSILON?
		{
			AngleMatrix( pProc->pTrigger( 0 )->quat, pProc->pTrigger( 0 )->pos, bonematrix );
			ConcatTransforms ( bonetoworld.GetBone( pbones[ibone].parent ), bonematrix, bonetoworld.GetBoneForWrite( ibone ) );
			return;
		}

		scale = 1.0 / scale;

		quat.Init( 0, 0, 0, 0);
		pos.Init( );

		for (i = 0; i < pProc->numtriggers; i++)
		{
			if (weight[i])
			{
				float s = weight[i] * scale;
				mstudioquatinterpinfo_t *pTrigger = pProc->pTrigger( i );

				QuaternionAlign( pTrigger->quat, quat, quat );

				quat.x = quat.x + s * pTrigger->quat.x;
				quat.y = quat.y + s * pTrigger->quat.y;
				quat.z = quat.z + s * pTrigger->quat.z;
				quat.w = quat.w + s * pTrigger->quat.w;
				pos.x = pos.x + s * pTrigger->pos.x;
				pos.y = pos.y + s * pTrigger->pos.y;
				pos.z = pos.z + s * pTrigger->pos.z;
			}
		}
		Assert( QuaternionNormalize( quat ) != 0);
		QuaternionMatrix( quat, pos, bonematrix );
	}

	ConcatTransforms (bonetoworld.GetBone( pbones[ibone].parent ), bonematrix, bonetoworld.GetBoneForWrite( ibone ));
}

/*
 * This is for DoAimAtBone below, was just for testing, not needed in general
 * but to turn it back on, uncomment this and the section in DoAimAtBone() below
 *

static ConVar aim_constraint( "aim_constraint", "1", FCVAR_REPLICATED, "Toggle <aimconstraint> Helper Bones" );

*/

//-----------------------------------------------------------------------------
// Purpose: Generate a procedural transformation so that one bone points at
//			another point on the model
//-----------------------------------------------------------------------------
void DoAimAtBone(
	mstudiobone_t *pBones,
	int	iBone,
	CBoneAccessor &bonetoworld,
	const CStudioHdr *pStudioHdr
	)
{
	mstudioaimatbone_t *pProc = (mstudioaimatbone_t *)pBones[iBone].pProcedure();

	if ( !pProc )
	{
		return;
	}

	/*
	 * Uncomment this if the ConVar above is uncommented
	 *

	if ( !aim_constraint.GetBool() )
	{
		// If the aim constraint is turned off then just copy the parent transform
		// plus the offset value

		matrix3x4_t boneToWorldSpace;
		MatrixCopy ( bonetoworld.GetBone( pProc->parent ), boneToWorldSpace );
		Vector boneWorldPosition;
		VectorTransform( pProc->basepos, boneToWorldSpace, boneWorldPosition );
		MatrixSetColumn( boneWorldPosition, 3, boneToWorldSpace );
		MatrixCopy( boneToWorldSpace, bonetoworld.GetBoneForWrite( iBone ) );

		return;
	}

	*/

	// The world matrix of the bone to change
	matrix3x4_t boneMatrix;

	// Guaranteed to be unit length
	const Vector &userAimVector( pProc->aimvector );

	// Guaranteed to be unit length
	const Vector &userUpVector( pProc->upvector );

	// Get to get position of bone but also for up reference
	matrix3x4_t parentSpace;
	MatrixCopy ( bonetoworld.GetBone( pProc->parent ), parentSpace );

	// World space position of the bone to aim
	Vector aimWorldPosition;
	VectorTransform( pProc->basepos, parentSpace, aimWorldPosition );

	// The worldspace matrix of the bone to aim at
	matrix3x4_t aimAtSpace;
	if ( pStudioHdr )
	{
		// This means it's AIMATATTACH
		const mstudioattachment_t &attachment( ((CStudioHdr *)pStudioHdr)->pAttachment( pProc->aim ) );
		ConcatTransforms(
			bonetoworld.GetBone( attachment.localbone ),
			attachment.local,
			aimAtSpace );
	}
	else
	{
		MatrixCopy( bonetoworld.GetBone( pProc->aim ), aimAtSpace );
	}

	Vector aimAtWorldPosition;
	MatrixGetColumn( aimAtSpace, 3, aimAtWorldPosition );

	// make sure the redundant parent info is correct
	Assert( pProc->parent == pBones[iBone].parent );
	// make sure the redundant position info is correct
	Assert( pProc->basepos.DistToSqr( pBones[iBone].pos ) < 0.1 );

	// The aim and up data is relative to this bone, not the parent bone
	matrix3x4_t bonematrix, boneLocalToWorld;
	AngleMatrix( pBones[iBone].quat, pProc->basepos, bonematrix );
	ConcatTransforms( bonetoworld.GetBone( pProc->parent ), bonematrix, boneLocalToWorld );

	Vector aimVector;
	VectorSubtract( aimAtWorldPosition, aimWorldPosition, aimVector );
	VectorNormalizeFast( aimVector );

	Vector axis;
	CrossProduct( userAimVector, aimVector, axis );
	VectorNormalizeFast( axis );
	Assert( 1.0f - fabs( DotProduct( userAimVector, aimVector ) ) > FLT_EPSILON );
	float angle( acosf( DotProduct( userAimVector, aimVector ) ) );
	Quaternion aimRotation;
	AxisAngleQuaternion( axis, RAD2DEG( angle ), aimRotation );

	if ( ( 1.0f - fabs( DotProduct( userUpVector, userAimVector ) ) ) > FLT_EPSILON )
	{
		matrix3x4_t aimRotationMatrix;
		QuaternionMatrix( aimRotation, aimRotationMatrix );

		Vector tmpV;

		Vector tmp_pUp;
		VectorRotate( userUpVector, aimRotationMatrix, tmp_pUp );
		VectorScale( aimVector, DotProduct( aimVector, tmp_pUp ), tmpV );
		Vector pUp;
		VectorSubtract( tmp_pUp, tmpV, pUp );
		VectorNormalizeFast( pUp );

		Vector tmp_pParentUp;
		VectorRotate( userUpVector, boneLocalToWorld, tmp_pParentUp );
		VectorScale( aimVector, DotProduct( aimVector, tmp_pParentUp ), tmpV );
		Vector pParentUp;
		VectorSubtract( tmp_pParentUp, tmpV, pParentUp );
		VectorNormalizeFast( pParentUp );

		Quaternion upRotation;
		//Assert( 1.0f - fabs( DotProduct( pUp, pParentUp ) ) > FLT_EPSILON );
		if( 1.0f - fabs( DotProduct( pUp, pParentUp ) ) > FLT_EPSILON )
		{
			angle = acos( DotProduct( pUp, pParentUp ) );
			CrossProduct( pUp, pParentUp, axis );			
		}
		else
		{
			angle = 0;
			axis = pUp;
		}

		VectorNormalizeFast( axis );
		AxisAngleQuaternion( axis, RAD2DEG( angle ), upRotation );

		Quaternion boneRotation;
		QuaternionMult( upRotation, aimRotation, boneRotation );
		QuaternionMatrix( boneRotation, aimWorldPosition, boneMatrix );
	}
	else
	{
		QuaternionMatrix( aimRotation, aimWorldPosition, boneMatrix );
	}

	MatrixCopy( boneMatrix, bonetoworld.GetBoneForWrite( iBone ) );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CalcProceduralBone(
	const CStudioHdr *pStudioHdr,
	int iBone,
	CBoneAccessor &bonetoworld
	)
{
	mstudiobone_t		*pbones = pStudioHdr->pBone( 0 );

	if ( pStudioHdr->boneFlags(iBone) & BONE_ALWAYS_PROCEDURAL )
	{
		switch( pbones[iBone].proctype )
		{
		case STUDIO_PROC_AXISINTERP:
			DoAxisInterpBone( pbones, iBone, bonetoworld );
			return true;

		case STUDIO_PROC_QUATINTERP:
			DoQuatInterpBone( pbones, iBone, bonetoworld );
			return true;

		case STUDIO_PROC_AIMATBONE:
			DoAimAtBone( pbones, iBone, bonetoworld, NULL );
			return true;

		case STUDIO_PROC_AIMATATTACH:
			DoAimAtBone( pbones, iBone, bonetoworld, pStudioHdr );
			return true;

		default:
			return false;
		}
	}
	return false;
}



//-----------------------------------------------------------------------------
// Purpose:  Lookup a bone controller
//-----------------------------------------------------------------------------



static mstudiobonecontroller_t* FindController( const CStudioHdr *pStudioHdr, int iController)
{
	// find first controller that matches the index
	for (int i = 0; i < pStudioHdr->numbonecontrollers(); i++)
	{
		if (pStudioHdr->pBonecontroller( i )->inputfield == iController)
			return pStudioHdr->pBonecontroller( i );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: converts a ranged bone controller value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetController( const CStudioHdr *pStudioHdr, int iController, float flValue, float &ctlValue )
{
	if (! pStudioHdr)
		return flValue;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if(!pbonecontroller)
	{
		ctlValue = 0;
		return flValue;
	}

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	ctlValue = (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);
	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	float flReturnVal = ((1.0 - ctlValue)*pbonecontroller->start + ctlValue *pbonecontroller->end);

	// ugly hack, invert value if a rotational controller and end < start
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR) &&
		pbonecontroller->end < pbonecontroller->start				)
	{
		flReturnVal *= -1;
	}
	
	return flReturnVal;
}


//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded bone controller value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetController( const CStudioHdr *pStudioHdr, int iController, float ctlValue )
{
	if (!pStudioHdr)
		return 0.0;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if(!pbonecontroller)
		return 0;

	return ctlValue * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


//-----------------------------------------------------------------------------
// Purpose: Calculates default values for the pose parameters
// Output: 	fills in an array
//-----------------------------------------------------------------------------

void Studio_CalcDefaultPoseParameters( const CStudioHdr *pStudioHdr, float flPoseParameter[], int nCount )
{
	int nPoseCount = pStudioHdr->GetNumPoseParameters();
	int nNumParams = MIN( nCount, MAXSTUDIOPOSEPARAM );

	for ( int i = 0; i < nNumParams; ++i )
	{
		// Default to middle of the pose parameter range
		flPoseParameter[ i ] = 0.5f;
		if ( i < nPoseCount )
		{
			const mstudioposeparamdesc_t &Pose = ((CStudioHdr *)pStudioHdr)->pPoseParameter( i );

			// Want to try for a zero state.  If one doesn't exist set it to .5 by default.
			if ( Pose.start < 0.0f && Pose.end > 0.0f )
			{
				float flPoseDelta = Pose.end - Pose.start;
				flPoseParameter[i] = -Pose.start / flPoseDelta;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: converts a ranged pose parameter value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetPoseParameter( const CStudioHdr *pStudioHdr, int iParameter, float flValue, float &ctlValue )
{
	if (iParameter < 0 || iParameter >= pStudioHdr->GetNumPoseParameters())
	{
		return 0;
	}

	const mstudioposeparamdesc_t &PoseParam = ((CStudioHdr *)pStudioHdr)->pPoseParameter( iParameter );

	Assert( IsFinite( flValue ) );

	if (PoseParam.loop)
	{
		float wrap = (PoseParam.start + PoseParam.end) / 2.0 + PoseParam.loop / 2.0;
		float shift = PoseParam.loop - wrap;

		flValue = flValue - PoseParam.loop * floor((flValue + shift) / PoseParam.loop);
	}

	ctlValue = (flValue - PoseParam.start) / (PoseParam.end - PoseParam.start);

	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	Assert( IsFinite( ctlValue ) );

	return ctlValue * (PoseParam.end - PoseParam.start) + PoseParam.start;
}


//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded pose parameter value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetPoseParameter( const CStudioHdr *pStudioHdr, int iParameter, float ctlValue )
{
	if (iParameter < 0 || iParameter >= pStudioHdr->GetNumPoseParameters())
	{
		return 0;
	}

	const mstudioposeparamdesc_t &PoseParam = ((CStudioHdr *)pStudioHdr)->pPoseParameter( iParameter );

	return ctlValue * (PoseParam.end - PoseParam.start) + PoseParam.start;
}


#pragma warning (disable : 4701)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static int ClipRayToHitbox( const Ray_t &ray, mstudiobbox_t *pbox, matrix3x4_t& matrix, trace_t &tr )
{
	const float flProjEpsilon = 0.01f;
	// scale by current t so hits shorten the ray and increase the likelihood of early outs
	Vector delta2;
	VectorScale( ray.m_Delta, (0.5f * tr.fraction), delta2 );

	// OPTIMIZE: Store this in the box instead of computing it here
	// compute center in local space
	Vector boxextents;
	boxextents.x = (pbox->bbmin.x + pbox->bbmax.x) * 0.5; 
	boxextents.y = (pbox->bbmin.y + pbox->bbmax.y) * 0.5; 
	boxextents.z = (pbox->bbmin.z + pbox->bbmax.z) * 0.5; 
	Vector boxCenter;
	// transform to world space
	VectorTransform( boxextents, matrix, boxCenter );
	// calc extents from local center
	boxextents.x = pbox->bbmax.x - boxextents.x;
	boxextents.y = pbox->bbmax.y - boxextents.y;
	boxextents.z = pbox->bbmax.z - boxextents.z;
	// OPTIMIZE: This is optimized for world space.  If the transform is fast enough, it may make more
	// sense to just xform and call UTIL_ClipToBox() instead.  MEASURE THIS.

	// save the extents of the ray along 
	Vector extent, uextent;
	Vector segmentCenter;
	segmentCenter.x = ray.m_Start.x + delta2.x - boxCenter.x;
	segmentCenter.y = ray.m_Start.y + delta2.y - boxCenter.y;
	segmentCenter.z = ray.m_Start.z + delta2.z - boxCenter.z;

	extent.Init();

	// check box axes for separation
	for ( int j = 0; j < 3; j++ )
	{
		extent[j] = delta2.x * matrix[0][j] + delta2.y * matrix[1][j] +	delta2.z * matrix[2][j];
		uextent[j] = fabsf(extent[j]);
		float coord = segmentCenter.x * matrix[0][j] + segmentCenter.y * matrix[1][j] +	segmentCenter.z * matrix[2][j];
		coord = fabsf(coord);

		if ( coord > (boxextents[j] + uextent[j]) )
			return -1;
	}

	// now check cross axes for separation
	float tmp, tmpfix, cextent;
	Vector cross;
	CrossProduct( delta2, segmentCenter, cross );
	cextent = cross.x * matrix[0][0] + cross.y * matrix[1][0] + cross.z * matrix[2][0];
	cextent = fabsf(cextent);
	tmp = boxextents[1]*uextent[2] + boxextents[2]*uextent[1];
	tmpfix = MAX(tmp, flProjEpsilon);
	if ( cextent > tmpfix )
		return -1;
	
//	if ( cextent > tmp && cextent <= tmpfix )
//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

	cextent = cross.x * matrix[0][1] + cross.y * matrix[1][1] + cross.z * matrix[2][1];
	cextent = fabsf(cextent);
	tmp = boxextents[0]*uextent[2] + boxextents[2]*uextent[0];
	tmpfix = MAX(tmp, flProjEpsilon);
	if ( cextent > tmpfix )
		return -1;

//	if ( cextent > tmp && cextent <= tmpfix )
//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

	cextent = cross.x * matrix[0][2] + cross.y * matrix[1][2] + cross.z * matrix[2][2];
	cextent = fabsf(cextent);
	tmp = boxextents[0]*uextent[1] + boxextents[1]*uextent[0];
	tmpfix = MAX(tmp, flProjEpsilon);
	if ( cextent > tmpfix )
		return -1;

//	if ( cextent > tmp && cextent <= tmpfix )
//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

	// !!! We hit this box !!! compute intersection point and return
	Vector start;

	// Compute ray start in bone space
	VectorITransform( ray.m_Start, matrix, start );
	// extent is delta2 in bone space, recompute delta in bone space
	VectorScale( extent, 2, extent );

	// delta was prescaled by the current t, so no need to see if this intersection
	// is closer
	trace_t boxTrace;
	if ( !IntersectRayWithBox( start, extent, pbox->bbmin, pbox->bbmax, 0.0f, &boxTrace ) )
		return -1;

	Assert( IsFinite(boxTrace.fraction) );
	tr.fraction *= boxTrace.fraction;
	tr.startsolid = boxTrace.startsolid;
	int hitside = boxTrace.plane.type;
	if ( boxTrace.plane.normal[hitside] >= 0 )
	{
		hitside += 3;
	}
	return hitside;
}

#pragma warning (default : 4701)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SweepBoxToStudio( IPhysicsSurfaceProps *pProps, const Ray_t& ray, CStudioHdr *pStudioHdr, mstudiohitboxset_t *set, 
				   matrix3x4_t **hitboxbones, int fContentsMask, trace_t &tr )
{
	tr.fraction = 1.0;
	tr.startsolid = false;

	// OPTIMIZE: Partition these?
	Ray_t clippedRay = ray;
	int hitbox = -1;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		// Filter based on contents mask
		int fBoneContents = pStudioHdr->pBone( pbox->bone )->contents;
		if ( ( fBoneContents & fContentsMask ) == 0 )
			continue;
		
		//FIXME: Won't work with scaling!
		trace_t obbTrace;
		if ( IntersectRayWithOBB( clippedRay, *hitboxbones[pbox->bone], pbox->bbmin, pbox->bbmax, 0.0f, &obbTrace ) )
		{
			tr.startpos = obbTrace.startpos;
			tr.endpos = obbTrace.endpos;
			tr.plane = obbTrace.plane;
			tr.startsolid = obbTrace.startsolid;
			tr.allsolid = obbTrace.allsolid;

			// This logic here is to shorten the ray each time to get more early outs
			tr.fraction *= obbTrace.fraction;
			clippedRay.m_Delta *= obbTrace.fraction;
			hitbox = i;
			if (tr.startsolid)
				break;
		}
	}

	if ( hitbox >= 0 )
	{
		tr.hitgroup = set->pHitbox(hitbox)->group;
		tr.hitbox = hitbox;
		const mstudiobone_t *pBone = pStudioHdr->pBone( set->pHitbox(hitbox)->bone );
		tr.contents = pBone->contents | CONTENTS_HITBOX;
		tr.physicsbone = pBone->physicsbone;
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = pProps->GetSurfaceIndex( pBone->pszSurfaceProp() );

		Assert( tr.physicsbone >= 0 );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool TraceToStudio( IPhysicsSurfaceProps *pProps, const Ray_t& ray, CStudioHdr *pStudioHdr, mstudiohitboxset_t *set, 
				   matrix3x4_t **hitboxbones, int fContentsMask, const Vector &vecOrigin, float flScale, trace_t &tr )
{
	if ( !ray.m_IsRay )
	{
		return SweepBoxToStudio( pProps, ray, pStudioHdr, set, hitboxbones, fContentsMask, tr );
	}

	tr.fraction = 1.0;
	tr.startsolid = false;

	// no hit yet
	int hitbox = -1;
	int hitside = -1;

	// OPTIMIZE: Partition these?
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		// Filter based on contents mask
		int fBoneContents = pStudioHdr->pBone( pbox->bone )->contents;
		if ( ( fBoneContents & fContentsMask ) == 0 )
			continue;
		
		// columns are axes of the bones in world space, translation is in world space
		matrix3x4_t& matrix = *hitboxbones[pbox->bone];
		
		// Because we're sending in a matrix with scale data, and because the matrix inversion in the hitbox
		// code does not handle that case, we pre-scale the bones and ray down here and do our collision checks
		// in unscaled space.  We can then rescale the results afterwards.

		int side = -1;
		if ( flScale < 1.0f-FLT_EPSILON || flScale > 1.0f+FLT_EPSILON )
		{
			matrix3x4_t matScaled;
			MatrixCopy( matrix, matScaled );
			
			float invScale = 1.0f / flScale;

			Vector vecBoneOrigin;
			MatrixGetColumn( matScaled, 3, vecBoneOrigin );
			
			// Pre-scale the origin down
			Vector vecNewOrigin = vecBoneOrigin - vecOrigin;
			vecNewOrigin *= invScale;
			vecNewOrigin += vecOrigin;
			MatrixSetColumn( vecNewOrigin, 3, matScaled );

			// Scale it uniformly
			VectorScale( matScaled[0], invScale, matScaled[0] );
			VectorScale( matScaled[1], invScale, matScaled[1] );
			VectorScale( matScaled[2], invScale, matScaled[2] );
			
			// Pre-scale our ray as well
			Vector vecRayStart = ray.m_Start - vecOrigin;
			vecRayStart *= invScale;
			vecRayStart += vecOrigin;
			
			Vector vecRayDelta = ray.m_Delta * invScale;

			Ray_t newRay;
			newRay.Init( vecRayStart, vecRayStart + vecRayDelta );  
			
			side = ClipRayToHitbox( newRay, pbox, matScaled, tr );
		}
		else
		{
			side = ClipRayToHitbox( ray, pbox, matrix, tr );
		}

		if ( side >= 0 )
		{
			hitbox = i;
			hitside = side;
		}
	}

	if ( hitbox >= 0 )
	{
		mstudiobbox_t *pbox = set->pHitbox(hitbox);
		VectorMA( ray.m_Start, tr.fraction, ray.m_Delta, tr.endpos );
		tr.hitgroup = set->pHitbox(hitbox)->group;
		tr.hitbox = hitbox;
		const mstudiobone_t *pBone = pStudioHdr->pBone( pbox->bone );
		tr.contents = pBone->contents | CONTENTS_HITBOX;
		tr.physicsbone = pBone->physicsbone;
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = pProps->GetSurfaceIndex( pBone->pszSurfaceProp() );

		Assert( tr.physicsbone >= 0 );
		matrix3x4_t& matrix = *hitboxbones[pbox->bone];
		if ( hitside >= 3 )
		{
			hitside -= 3;
			tr.plane.normal[0] = matrix[0][hitside];
			tr.plane.normal[1] = matrix[1][hitside];
			tr.plane.normal[2] = matrix[2][hitside];
			//tr.plane.dist = DotProduct( tr.plane.normal, Vector(matrix[0][3], matrix[1][3], matrix[2][3] ) ) + pbox->bbmax[hitside];
		}
		else
		{
			tr.plane.normal[0] = -matrix[0][hitside];
			tr.plane.normal[1] = -matrix[1][hitside];
			tr.plane.normal[2] = -matrix[2][hitside];
			//tr.plane.dist = DotProduct( tr.plane.normal, Vector(matrix[0][3], matrix[1][3], matrix[2][3] ) ) - pbox->bbmin[hitside];
		}
		// simpler plane constant equation
		tr.plane.dist = DotProduct( tr.endpos, tr.plane.normal );
		tr.plane.type = 3;
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: returns array of animations and weightings for a sequence based on current pose parameters
//-----------------------------------------------------------------------------

void Studio_SeqAnims( const CStudioHdr *pStudioHdr, mstudioseqdesc_t &seqdesc, int iSequence, const float poseParameter[], mstudioanimdesc_t *panim[4], float *weight )
{
#if _DEBUG
	VPROF_INCREMENT_COUNTER("SEQ_ANIMS",1);
#endif
	if (!pStudioHdr || iSequence >= pStudioHdr->GetNumSeq())
	{
		weight[0] = weight[1] = weight[2] = weight[3] = 0.0;
		return;
	}

	int i0 = 0, i1 = 0;
	float s0 = 0, s1 = 0;
	
	Studio_LocalPoseParameter( pStudioHdr, poseParameter, seqdesc, iSequence, 0, s0, i0 );
	Studio_LocalPoseParameter( pStudioHdr, poseParameter, seqdesc, iSequence, 1, s1, i1 );

	panim[0] = &((CStudioHdr *)pStudioHdr)->pAnimdesc( pStudioHdr->iRelativeAnim( iSequence, seqdesc.anim( i0  , i1 ) ) );
	weight[0] = (1 - s0) * (1 - s1);

	panim[1] = &((CStudioHdr *)pStudioHdr)->pAnimdesc( pStudioHdr->iRelativeAnim( iSequence, seqdesc.anim( i0+1, i1 ) ) );
	weight[1] = (s0) * (1 - s1);

	panim[2] = &((CStudioHdr *)pStudioHdr)->pAnimdesc( pStudioHdr->iRelativeAnim( iSequence, seqdesc.anim( i0  , i1+1 ) ) );
	weight[2] = (1 - s0) * (s1);

	panim[3] = &((CStudioHdr *)pStudioHdr)->pAnimdesc( pStudioHdr->iRelativeAnim( iSequence, seqdesc.anim( i0+1, i1+1 ) ) );
	weight[3] = (s0) * (s1);

	Assert( weight[0] >= 0.0f && weight[1] >= 0.0f && weight[2] >= 0.0f && weight[3] >= 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: returns max frame number for a sequence
//-----------------------------------------------------------------------------

int Studio_MaxFrame( const CStudioHdr *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );

	float maxFrame = 0;
	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0)
		{
			maxFrame += panim[i]->numframes * weight[i];
		}
	}

	if ( maxFrame > 1 )
		maxFrame -= 1;
	

	// FIXME: why does the weights sometimes not exactly add it 1.0 and this sometimes rounds down?
	return (maxFrame + 0.01);
}


//-----------------------------------------------------------------------------
// Purpose: returns frames per second of a sequence
//-----------------------------------------------------------------------------

float Studio_FPS( const CStudioHdr *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );

	float t = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0)
		{
			t += panim[i]->fps * weight[i];
		}
	}
	return t;
}


//-----------------------------------------------------------------------------
// Purpose: returns cycles per second of a sequence (cycles/second)
//-----------------------------------------------------------------------------

float Studio_CPS( const CStudioHdr *pStudioHdr, mstudioseqdesc_t &seqdesc, int iSequence, const float poseParameter[] )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );

	float t = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i] > 0 && panim[i]->numframes > 1)
		{
			t += (panim[i]->fps / (panim[i]->numframes - 1)) * weight[i];
		}
	}
	return t;
}

//-----------------------------------------------------------------------------
// Purpose: returns length (in seconds) of a sequence (seconds/cycle)
//-----------------------------------------------------------------------------

float Studio_Duration( const CStudioHdr *pStudioHdr, int iSequence, const float poseParameter[] )
{
	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	float cps = Studio_CPS( pStudioHdr, seqdesc, iSequence, poseParameter );

	if( cps == 0 )
		return 0.0f;

	return 1.0f/cps;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle relative to the start of an animations cycle
// Output:	updated position and angle, relative to the origin
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimPosition( mstudioanimdesc_t *panim, float flCycle, Vector &vecPos, QAngle &vecAngle )
{
	float	prevframe = 0;
	vecPos.Init( );
	vecAngle.Init( );

	if (panim->nummovements == 0)
		return false;

	int iLoops = 0;
	if (flCycle > 1.0)
	{
		iLoops = (int)flCycle;
	}
	else if (flCycle < 0.0)
	{
		iLoops = (int)flCycle - 1;
	}
	flCycle = flCycle - iLoops;

	float	flFrame = flCycle * (panim->numframes - 1);

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		if (pmove->endframe >= flFrame)
		{
			float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

			float d = pmove->v0 * f + 0.5 * (pmove->v1 - pmove->v0) * f * f;

			vecPos = vecPos + d * pmove->vector;
			vecAngle.y = vecAngle.y * (1 - f) + pmove->angle * f;
			if (iLoops != 0)
			{
				mstudiomovement_t *pmoveAnim = panim->pMovement( panim->nummovements - 1 );
				vecPos = vecPos + iLoops * pmoveAnim->position;
				vecAngle.y = vecAngle.y + iLoops * pmoveAnim->angle;
			}
			return true;
		}
		else
		{
			prevframe = pmove->endframe;
			vecPos = pmove->position;
			vecAngle.y = pmove->angle;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: calculate instantaneous velocity in ips at a given point 
//			in the animations cycle
// Output:	velocity vector, relative to identity orientation
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimVelocity( mstudioanimdesc_t *panim, float flCycle, Vector &vecVelocity )
{
	float	prevframe = 0;

	float	flFrame = flCycle * (panim->numframes - 1);
	flFrame = flFrame - (int)(flFrame / (panim->numframes - 1));

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		if (pmove->endframe >= flFrame)
		{
			float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

			float vel = pmove->v0 * (1 - f) + pmove->v1 * f;
			// scale from per block to per sec velocity
			vel = vel * panim->fps / (pmove->endframe - prevframe);

			vecVelocity = pmove->vector * vel;
			return true;
		}
		else
		{
			prevframe = pmove->endframe;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle between two points in an animation cycle
// Output:	updated position and angle, relative to CycleFrom being at the origin
//			returns false if animation is not a movement animation
//-----------------------------------------------------------------------------

bool Studio_AnimMovement( mstudioanimdesc_t *panim, float flCycleFrom, float flCycleTo, Vector &deltaPos, QAngle &deltaAngle )
{
	if (panim->nummovements == 0)
		return false;

	Vector startPos;
	QAngle startA;
	Studio_AnimPosition( panim, flCycleFrom, startPos, startA );

	Vector endPos;
	QAngle endA;
	Studio_AnimPosition( panim, flCycleTo, endPos, endA );

	Vector tmp = endPos - startPos;
	deltaAngle.y = endA.y - startA.y;
	VectorYawRotate( tmp, -startA.y, deltaPos );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: finds how much of an animation to play to move given linear distance
//-----------------------------------------------------------------------------

float Studio_FindAnimDistance( mstudioanimdesc_t *panim, float flDist )
{
	float	prevframe = 0;

	if (flDist <= 0)
		return 0.0;

	for (int i = 0; i < panim->nummovements; i++)
	{
		mstudiomovement_t *pmove = panim->pMovement( i );

		float flMove = (pmove->v0 + pmove->v1) * 0.5;

		if (flMove >= flDist)
		{
			float root1, root2;

			// d = V0 * t + 1/2 (V1-V0) * t^2
			if (SolveQuadratic( 0.5 * (pmove->v1 - pmove->v0), pmove->v0, -flDist, root1, root2 ))
			{
				float cpf = 1.0 / (panim->numframes - 1);  // cycles per frame

				return (prevframe + root1 * (pmove->endframe - prevframe)) * cpf;
			}
			return 0.0;
		}
		else
		{
			flDist -= flMove;
			prevframe = pmove->endframe;
		}
	}
	return 1.0;
}


//-----------------------------------------------------------------------------
// Purpose: calculate changes in position and angle between two points in a sequences cycle
// Output:	updated position and angle, relative to CycleFrom being at the origin
//			returns false if sequence is not a movement sequence
//-----------------------------------------------------------------------------

bool Studio_SeqMovement( const CStudioHdr *pStudioHdr, int iSequence, float flCycleFrom, float flCycleTo, const float poseParameter[], Vector &deltaPos, QAngle &deltaAngles )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );

	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );
	
	deltaPos.Init( );
	deltaAngles.Init( );

	bool found = false;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			Vector localPos;
			QAngle localAngles;

			localPos.Init();
			localAngles.Init();

			if (Studio_AnimMovement( panim[i], flCycleFrom, flCycleTo, localPos, localAngles ))
			{
				found = true;
				deltaPos = deltaPos + localPos * weight[i];
				// FIXME: this makes no sense
				deltaAngles = deltaAngles + localAngles * weight[i];
			}
			else if (!(panim[i]->flags & STUDIO_DELTA) && panim[i]->nummovements == 0 && seqdesc.weight(0) > 0.0)
			{
				found = true;
			}
		}
	}
	return found;
}


//-----------------------------------------------------------------------------
// Purpose: calculate instantaneous velocity in ips at a given point in the sequence's cycle
// Output:	velocity vector, relative to identity orientation
//			returns false if sequence is not a movement sequence
//-----------------------------------------------------------------------------

bool Studio_SeqVelocity( const CStudioHdr *pStudioHdr, int iSequence, float flCycle, const float poseParameter[], Vector &vecVelocity )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );
	
	vecVelocity.Init( );

	bool found = false;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			Vector vecLocalVelocity;

			if (Studio_AnimVelocity( panim[i], flCycle, vecLocalVelocity ))
			{
				vecVelocity = vecVelocity + vecLocalVelocity * weight[i];
				found = true;
			}
		}
	}
	return found;
}

//-----------------------------------------------------------------------------
// Purpose: finds how much of an sequence to play to move given linear distance
//-----------------------------------------------------------------------------

float Studio_FindSeqDistance( const CStudioHdr *pStudioHdr, int iSequence, const float poseParameter[], float flDist )
{
	mstudioanimdesc_t *panim[4];
	float	weight[4];

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	Studio_SeqAnims( pStudioHdr, seqdesc, iSequence, poseParameter, panim, weight );
	
	float flCycle = 0;

	for (int i = 0; i < 4; i++)
	{
		if (weight[i])
		{
			float flLocalCycle = Studio_FindAnimDistance( panim[i], flDist );
			flCycle = flCycle + flLocalCycle * weight[i];
		}
	}
	return flCycle;
}

//-----------------------------------------------------------------------------
// Purpose: lookup attachment by name
//-----------------------------------------------------------------------------

int Studio_FindAttachment( const CStudioHdr *pStudioHdr, const char *pAttachmentName )
{
	if ( pStudioHdr && pStudioHdr->SequencesAvailable() )
	{
		// Extract the bone index from the name
		for (int i = 0; i < pStudioHdr->GetNumAttachments(); i++)
		{
			if (!V_stricmp(pAttachmentName,((CStudioHdr *)pStudioHdr)->pAttachment(i).pszName( ))) 
			{
				return i;
			}
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: lookup attachments by substring. Randomly return one of the matching attachments.
//-----------------------------------------------------------------------------

int Studio_FindRandomAttachment( const CStudioHdr *pStudioHdr, const char *pAttachmentName )
{
	if ( pStudioHdr )
	{
		// First move them all matching attachments into a list
		CUtlVector<int> matchingAttachments;

		// Extract the bone index from the name
		for (int i = 0; i < pStudioHdr->GetNumAttachments(); i++)
		{
			if ( strstr( ((CStudioHdr *)pStudioHdr)->pAttachment(i).pszName(), pAttachmentName ) ) 
			{
				matchingAttachments.AddToTail(i);
			}
		}

		// Then randomly return one of the attachments
		if ( matchingAttachments.Size() > 0 )
			return matchingAttachments[ RandomInt( 0, matchingAttachments.Size()-1 ) ];
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: lookup bone by name
//-----------------------------------------------------------------------------

int Studio_BoneIndexByName( const CStudioHdr *pStudioHdr, const char *pName )
{
	if ( pStudioHdr )
	{
		// binary search for the bone matching pName
		int start = 0, end = pStudioHdr->numbones()-1;
		const byte *pBoneTable = pStudioHdr->GetBoneTableSortedByName();
		mstudiobone_t *pbones = pStudioHdr->pBone( 0 );
		while (start <= end)
		{
			int mid = (start + end) >> 1;
			int cmp = Q_stricmp( pbones[pBoneTable[mid]].pszName(), pName );
		
			if ( cmp < 0 )
			{
				start = mid + 1;
			}
			else if ( cmp > 0 )
			{
				end = mid - 1;
			}
			else
			{
				return pBoneTable[mid];
			}
		}
	}

	return -1;
}

const char *Studio_GetDefaultSurfaceProps( CStudioHdr *pstudiohdr )
{
	return pstudiohdr->pszSurfaceProp();
}

float Studio_GetMass( CStudioHdr *pstudiohdr )
{
	return pstudiohdr->mass();
}

//-----------------------------------------------------------------------------
// Purpose: return pointer to sequence key value buffer
//-----------------------------------------------------------------------------

const char *Studio_GetKeyValueText( const CStudioHdr *pStudioHdr, int iSequence )
{
	if (pStudioHdr && pStudioHdr->SequencesAvailable())
	{
		if (iSequence >= 0 && iSequence < pStudioHdr->GetNumSeq())
		{
			return ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence ).KeyValueText();
		}
	}
	return NULL;
}

bool Studio_PrefetchSequence( const CStudioHdr *pStudioHdr, int iSequence )
{
	bool pendingload = false;
	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( iSequence );
	int size0 = seqdesc.groupsize[ 0 ];
	int size1 = seqdesc.groupsize[ 1 ];
	for ( int i = 0; i < size0; ++i )
	{
		for ( int j = 0; j < size1; ++j )
		{
			mstudioanimdesc_t &animdesc = ((CStudioHdr *)pStudioHdr)->pAnimdesc( seqdesc.anim( i, j ) );
			int iFrame = 0;
			mstudioanim_t *panim = animdesc.pAnim( &iFrame );
			if ( !panim )
			{
				pendingload = true;
			}
		}
	}

	// Everything for this sequence is resident?
	return !pendingload;
}


//-----------------------------------------------------------------------------
// Purpose: Drive a flex controller from a component of a bone
//-----------------------------------------------------------------------------
void Studio_RunBoneFlexDrivers( float *pflFlexControllerWeights, const CStudioHdr *pStudioHdr, const Vector *pvPositions, const matrix3x4_t *pBoneToWorld, const matrix3x4_t &mRootToWorld )
{
	bool bRootToWorldInvComputed = false;
	matrix3x4_t mRootToWorldInv;
	matrix3x4_t mParentInv;
	matrix3x4_t mBoneLocal;

	const int nBoneFlexDriverCount = pStudioHdr->BoneFlexDriverCount();

	for ( int i = 0; i < nBoneFlexDriverCount; ++i )
	{
		const mstudioboneflexdriver_t *pBoneFlexDriver = pStudioHdr->BoneFlexDriver( i );
		const mstudiobone_t *pStudioBone = pStudioHdr->pBone( pBoneFlexDriver->m_nBoneIndex );

		const int nControllerCount = pBoneFlexDriver->m_nControlCount;

		if ( pStudioBone->flags & BONE_USED_BY_BONE_MERGE )
		{
			// The local space version of the bone is not available if this is a bonemerged bone
			// so do the slow computation of the local version of the bone from boneToWorld

			if ( pStudioBone->parent < 0 )
			{
				if ( !bRootToWorldInvComputed )
				{
					MatrixInvert( mRootToWorld, mRootToWorldInv );
					bRootToWorldInvComputed = true;
				}

				MatrixMultiply( mRootToWorldInv, pBoneToWorld[ pBoneFlexDriver->m_nBoneIndex ], mBoneLocal );
			}
			else
			{
				MatrixInvert( pBoneToWorld[ pStudioBone->parent ], mParentInv );
				MatrixMultiply( mParentInv, pBoneToWorld[ pBoneFlexDriver->m_nBoneIndex ], mBoneLocal );
			}

			for ( int j = 0; j < nControllerCount; ++j )
			{
				const mstudioboneflexdrivercontrol_t *pController = pBoneFlexDriver->pBoneFlexDriverControl( j );
				const mstudioflexcontroller_t *pFlexController = pStudioHdr->pFlexcontroller( static_cast< LocalFlexController_t >( pController->m_nFlexControllerIndex ) );

				if ( pFlexController->localToGlobal < 0 )
					continue;

				Assert( pController->m_nFlexControllerIndex >= 0 && pController->m_nFlexControllerIndex < pStudioHdr->numflexcontrollers() );
				Assert( pController->m_nBoneComponent >= 0 && pController->m_nBoneComponent <= 2 );
				pflFlexControllerWeights[pFlexController->localToGlobal] =
					RemapValClamped( mBoneLocal[pController->m_nBoneComponent][3], pController->m_flMin, pController->m_flMax, 0.0f, 1.0f );
			}
		}
		else
		{
			// Use the local space version of the bone directly for non-bonemerged bones

			const Vector &position = pvPositions[ pBoneFlexDriver->m_nBoneIndex ];

			for ( int j = 0; j < nControllerCount; ++j )
			{
				const mstudioboneflexdrivercontrol_t *pController = pBoneFlexDriver->pBoneFlexDriverControl( j );
				const mstudioflexcontroller_t *pFlexController = pStudioHdr->pFlexcontroller( static_cast< LocalFlexController_t >( pController->m_nFlexControllerIndex ) );

				if ( pFlexController->localToGlobal < 0 )
					continue;

				Assert( pController->m_nFlexControllerIndex >= 0 && pController->m_nFlexControllerIndex < pStudioHdr->numflexcontrollers() );
				Assert( pController->m_nBoneComponent >= 0 && pController->m_nBoneComponent <= 2 );
				pflFlexControllerWeights[pFlexController->localToGlobal] =
					RemapValClamped( position[pController->m_nBoneComponent], pController->m_flMin, pController->m_flMax, 0.0f, 1.0f );
			}
		}
	}
}
