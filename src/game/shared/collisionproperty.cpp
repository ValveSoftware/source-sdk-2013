//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "collisionproperty.h"
#include "igamesystem.h"
#include "utlvector.h"
#include "tier0/threadtools.h"
#include "tier0/tslist.h"

#ifdef CLIENT_DLL

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "recvproxy.h"

#else

#include "baseentity.h"
#include "baseanimating.h"
#include "sendproxy.h"
#include "hierarchy.h"
#endif

#include "predictable_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// KD tree query callbacks
//-----------------------------------------------------------------------------
class CDirtySpatialPartitionEntityList : public CAutoGameSystem, public IPartitionQueryCallback
{
public:
	CDirtySpatialPartitionEntityList( char const *name );

	// Members of IGameSystem
	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelShutdownPostEntity();

	// Members of IPartitionQueryCallback
	virtual void OnPreQuery_V1()	{ Assert( 0 ); }
	virtual void OnPreQuery( SpatialPartitionListMask_t listMask );
	virtual void OnPostQuery( SpatialPartitionListMask_t listMask );

	void AddEntity( CBaseEntity *pEntity );
	
	~CDirtySpatialPartitionEntityList();
	void LockPartitionForRead()
	{
		if ( m_readLockCount == 0 )
		{
			m_partitionMutex.LockForRead();
		}
		m_readLockCount++;
	}
	void UnlockPartitionForRead()
	{
		m_readLockCount--;
		if ( m_readLockCount == 0 )
		{
			m_partitionMutex.UnlockRead();
		}
	}


private:
	CTSListWithFreeList<CBaseHandle> m_DirtyEntities;
	CThreadSpinRWLock	 m_partitionMutex;
	uint32			 m_partitionWriteId;
	CThreadLocalInt<>	 m_readLockCount;
};


//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CDirtySpatialPartitionEntityList s_DirtyKDTree( "CDirtySpatialPartitionEntityList" );


//-----------------------------------------------------------------------------
// Force spatial partition updates (to avoid threading problems caused by lazy update)
//-----------------------------------------------------------------------------
void UpdateDirtySpatialPartitionEntities()
{
	SpatialPartitionListMask_t listMask;
#ifdef CLIENT_DLL
	listMask = PARTITION_CLIENT_GAME_EDICTS;
#else
	listMask = PARTITION_SERVER_GAME_EDICTS;
#endif
	s_DirtyKDTree.OnPreQuery( listMask );
	s_DirtyKDTree.OnPostQuery( listMask );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CDirtySpatialPartitionEntityList::CDirtySpatialPartitionEntityList( char const *name ) : CAutoGameSystem( name )
{
	m_DirtyEntities.Purge();
	m_readLockCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor.
//-----------------------------------------------------------------------------
CDirtySpatialPartitionEntityList::~CDirtySpatialPartitionEntityList()
{
	m_DirtyEntities.Purge();
}

//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CDirtySpatialPartitionEntityList::Init()
{
	partition->InstallQueryCallback( this );
	return true;
}

void CDirtySpatialPartitionEntityList::Shutdown()
{
	partition->RemoveQueryCallback( this );
}


//-----------------------------------------------------------------------------
// Makes sure all entries in the KD tree are in the correct position
//-----------------------------------------------------------------------------
void CDirtySpatialPartitionEntityList::AddEntity( CBaseEntity *pEntity )
{
	m_DirtyEntities.PushItem( pEntity->GetRefEHandle() );
}


//-----------------------------------------------------------------------------
// Members of IGameSystem
//-----------------------------------------------------------------------------
void CDirtySpatialPartitionEntityList::LevelShutdownPostEntity()
{
	m_DirtyEntities.RemoveAll();
}


//-----------------------------------------------------------------------------
// Makes sure all entries in the KD tree are in the correct position
//-----------------------------------------------------------------------------
void CDirtySpatialPartitionEntityList::OnPreQuery( SpatialPartitionListMask_t listMask )
{
#ifdef CLIENT_DLL
	const int validMask = PARTITION_CLIENT_GAME_EDICTS;
#else
	const int validMask = PARTITION_SERVER_GAME_EDICTS;
#endif

	if ( !( listMask & validMask ) )
		return;

	if ( m_partitionWriteId != 0 && m_partitionWriteId == ThreadGetCurrentId() )
		return;

#ifdef CLIENT_DLL
	// FIXME: This should really be an assertion... feh!
	if ( !C_BaseEntity::IsAbsRecomputationsEnabled() )
	{
		LockPartitionForRead();
		return;
	}
#endif

	// if you're holding a read lock, then these are entities that were still dirty after your trace started
	// or became dirty due to some other thread or callback. Updating them may cause corruption further up the
	// stack (e.g. partition iterator).  Ignoring the state change should be safe since it happened after the 
	// trace was requested or was unable to be resolved in a previous attempt (still dirty).
	if ( m_DirtyEntities.Count() && !m_readLockCount )
	{
		CUtlVector< CBaseHandle > vecStillDirty;
		m_partitionMutex.LockForWrite();
		m_partitionWriteId = ThreadGetCurrentId();
		CTSListWithFreeList<CBaseHandle>::Node_t *pCurrent, *pNext;
		while ( ( pCurrent = m_DirtyEntities.Detach() ) != NULL )
		{
			while ( pCurrent )
			{
				CBaseHandle handle = pCurrent->elem;
				pNext = (CTSListWithFreeList<CBaseHandle>::Node_t *)pCurrent->Next;
				m_DirtyEntities.FreeNode( pCurrent );
				pCurrent = pNext;

#ifndef CLIENT_DLL
				CBaseEntity *pEntity = gEntList.GetBaseEntity( handle );
#else
				CBaseEntity *pEntity = cl_entitylist->GetBaseEntityFromHandle( handle );
#endif

				if ( pEntity )
				{
					// If an entity is in the middle of bone setup, don't call UpdatePartition
					//  which can cause it to redo bone setup on the same frame causing a recursive
					//  call to bone setup.
					if ( !pEntity->IsEFlagSet( EFL_SETTING_UP_BONES ) )
					{
						pEntity->CollisionProp()->UpdatePartition();
					}
					else
					{
						vecStillDirty.AddToTail( handle );
					}
				}
			}
		}
		if ( vecStillDirty.Count() > 0 )
		{
			for ( int i = 0; i < vecStillDirty.Count(); i++ )
			{
				m_DirtyEntities.PushItem( vecStillDirty[i] );
			}
		}
		m_partitionWriteId = 0;
		m_partitionMutex.UnlockWrite();
	}
	LockPartitionForRead();
}

//-----------------------------------------------------------------------------
// Makes sure all entries in the KD tree are in the correct position
//-----------------------------------------------------------------------------
void CDirtySpatialPartitionEntityList::OnPostQuery( SpatialPartitionListMask_t listMask )
{
#ifdef CLIENT_DLL
	if ( !( listMask & PARTITION_CLIENT_GAME_EDICTS ) )
		return;
#else
	if ( !( listMask & PARTITION_SERVER_GAME_EDICTS ) )
		return;
#endif

	if ( m_partitionWriteId != 0 )
		return;

	UnlockPartitionForRead();
}


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------

#ifndef CLIENT_DLL

	BEGIN_DATADESC_NO_BASE( CCollisionProperty )

//		DEFINE_FIELD( m_pOuter, FIELD_CLASSPTR ),
		DEFINE_GLOBAL_FIELD( m_vecMinsPreScaled, FIELD_VECTOR ),
		DEFINE_GLOBAL_FIELD( m_vecMaxsPreScaled, FIELD_VECTOR ),
		DEFINE_GLOBAL_FIELD( m_vecMins, FIELD_VECTOR ),
		DEFINE_GLOBAL_FIELD( m_vecMaxs, FIELD_VECTOR ),
		DEFINE_KEYFIELD( m_nSolidType, FIELD_CHARACTER, "solid" ),
		DEFINE_FIELD( m_usSolidFlags, FIELD_SHORT ),
		DEFINE_FIELD( m_nSurroundType, FIELD_CHARACTER ),
		DEFINE_FIELD( m_flRadius, FIELD_FLOAT ),
		DEFINE_FIELD( m_triggerBloat, FIELD_CHARACTER ),
		DEFINE_FIELD( m_vecSpecifiedSurroundingMinsPreScaled, FIELD_VECTOR ),
		DEFINE_FIELD( m_vecSpecifiedSurroundingMaxsPreScaled, FIELD_VECTOR ),
		DEFINE_FIELD( m_vecSpecifiedSurroundingMins, FIELD_VECTOR ),
		DEFINE_FIELD( m_vecSpecifiedSurroundingMaxs, FIELD_VECTOR ),
		DEFINE_FIELD( m_vecSurroundingMins, FIELD_VECTOR ),
		DEFINE_FIELD( m_vecSurroundingMaxs, FIELD_VECTOR ),
//		DEFINE_FIELD( m_Partition, FIELD_SHORT ),
//		DEFINE_PHYSPTR( m_pPhysicsObject ),

	END_DATADESC()

#else

//-----------------------------------------------------------------------------
// Prediction
//-----------------------------------------------------------------------------
BEGIN_PREDICTION_DATA_NO_BASE( CCollisionProperty )

	DEFINE_PRED_FIELD( m_vecMinsPreScaled, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vecMaxsPreScaled, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vecMins, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vecMaxs, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nSolidType, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_usSolidFlags, FIELD_SHORT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_triggerBloat, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()

#endif

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL

static void RecvProxy_Solid( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((CCollisionProperty*)pStruct)->SetSolid( (SolidType_t)pData->m_Value.m_Int );
}

static void RecvProxy_SolidFlags( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((CCollisionProperty*)pStruct)->SetSolidFlags( pData->m_Value.m_Int );
}

static void RecvProxy_OBBMinsPreScaled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCollisionProperty *pProp = ((CCollisionProperty*)pStruct);
	Vector &vecMins = *((Vector*)pData->m_Value.m_Vector);
	pProp->SetCollisionBounds( vecMins, pProp->OBBMaxsPreScaled() );
}

static void RecvProxy_OBBMaxsPreScaled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCollisionProperty *pProp = ((CCollisionProperty*)pStruct);
	Vector &vecMaxs = *((Vector*)pData->m_Value.m_Vector);
	pProp->SetCollisionBounds( pProp->OBBMinsPreScaled(), vecMaxs );
}

static void RecvProxy_VectorDirtySurround( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	Vector &vecold = *((Vector*)pOut);
	Vector vecnew( pData->m_Value.m_Vector[0], pData->m_Value.m_Vector[1], pData->m_Value.m_Vector[2] );

	if ( vecold != vecnew )
	{
		vecold = vecnew;
		((CCollisionProperty*)pStruct)->MarkSurroundingBoundsDirty();
	}
}

static void RecvProxy_IntDirtySurround( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	if ( *((unsigned char*)pOut) != pData->m_Value.m_Int )
	{
		*((unsigned char*)pOut) = pData->m_Value.m_Int;
		((CCollisionProperty*)pStruct)->MarkSurroundingBoundsDirty();
	}
}

#else

static void SendProxy_Solid( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Int = ((CCollisionProperty*)pStruct)->GetSolid();
}

static void SendProxy_SolidFlags( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Int = ((CCollisionProperty*)pStruct)->GetSolidFlags();
}

#endif

BEGIN_NETWORK_TABLE_NOBASE( CCollisionProperty, DT_CollisionProperty )

#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO(m_vecMinsPreScaled), 0, RecvProxy_OBBMinsPreScaled ),
	RecvPropVector( RECVINFO(m_vecMaxsPreScaled), 0, RecvProxy_OBBMaxsPreScaled ),
	RecvPropVector( RECVINFO(m_vecMins), 0 ),
	RecvPropVector( RECVINFO(m_vecMaxs), 0 ),
	RecvPropInt( RECVINFO( m_nSolidType ),		0, RecvProxy_Solid ),
	RecvPropInt( RECVINFO( m_usSolidFlags ),	0, RecvProxy_SolidFlags ),
	RecvPropInt( RECVINFO(m_nSurroundType), 0, RecvProxy_IntDirtySurround ),
	RecvPropInt( RECVINFO(m_triggerBloat), 0, RecvProxy_IntDirtySurround ), 
	RecvPropVector( RECVINFO(m_vecSpecifiedSurroundingMinsPreScaled), 0, RecvProxy_VectorDirtySurround ),
	RecvPropVector( RECVINFO(m_vecSpecifiedSurroundingMaxsPreScaled), 0, RecvProxy_VectorDirtySurround ),
	RecvPropVector( RECVINFO(m_vecSpecifiedSurroundingMins), 0, RecvProxy_VectorDirtySurround ),
	RecvPropVector( RECVINFO(m_vecSpecifiedSurroundingMaxs), 0, RecvProxy_VectorDirtySurround ),
#else
	SendPropVector( SENDINFO(m_vecMinsPreScaled), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecMaxsPreScaled), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecMins), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecMaxs), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO( m_nSolidType ),		3, SPROP_UNSIGNED, SendProxy_Solid ),
	SendPropInt( SENDINFO( m_usSolidFlags ),	FSOLID_MAX_BITS, SPROP_UNSIGNED, SendProxy_SolidFlags ),
	SendPropInt( SENDINFO( m_nSurroundType ), SURROUNDING_TYPE_BIT_COUNT, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_triggerBloat), 0, SPROP_UNSIGNED),
	SendPropVector( SENDINFO(m_vecSpecifiedSurroundingMinsPreScaled), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecSpecifiedSurroundingMaxsPreScaled), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecSpecifiedSurroundingMins), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO(m_vecSpecifiedSurroundingMaxs), 0, SPROP_NOSCALE),
#endif

END_NETWORK_TABLE()

																							
//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CCollisionProperty::CCollisionProperty()
{
	m_Partition = PARTITION_INVALID_HANDLE;
	Init( NULL );
}

CCollisionProperty::~CCollisionProperty()
{
	DestroyPartitionHandle();
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CCollisionProperty::Init( CBaseEntity *pEntity )
{
	m_pOuter = pEntity;
	m_vecMinsPreScaled.GetForModify().Init();
	m_vecMaxsPreScaled.GetForModify().Init();
	m_vecMins.GetForModify().Init();
	m_vecMaxs.GetForModify().Init();
	m_flRadius = 0.0f;
	m_triggerBloat = 0;
	m_usSolidFlags = 0;
	m_nSolidType = SOLID_NONE;

	// NOTE: This replicates previous behavior; we may always want to use BEST_COLLISION_BOUNDS
	m_nSurroundType = USE_OBB_COLLISION_BOUNDS;
	m_vecSurroundingMins = vec3_origin;
	m_vecSurroundingMaxs = vec3_origin;
	m_vecSpecifiedSurroundingMinsPreScaled.GetForModify().Init();
	m_vecSpecifiedSurroundingMaxsPreScaled.GetForModify().Init();
	m_vecSpecifiedSurroundingMins.GetForModify().Init();
	m_vecSpecifiedSurroundingMaxs.GetForModify().Init();
}


//-----------------------------------------------------------------------------
// EntityHandle
//-----------------------------------------------------------------------------
IHandleEntity *CCollisionProperty::GetEntityHandle()
{
	return m_pOuter;
}


//-----------------------------------------------------------------------------
// Collision group
//-----------------------------------------------------------------------------
int CCollisionProperty::GetCollisionGroup() const
{
	return m_pOuter->GetCollisionGroup();
}


bool CCollisionProperty::ShouldTouchTrigger( int triggerSolidFlags ) const
{
	// debris only touches certain triggers
	if ( GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
	{
		if ( triggerSolidFlags & FSOLID_TRIGGER_TOUCH_DEBRIS )
			return true;

		return false;
	}

	// triggers don't touch other triggers (might be solid to other ents as well as trigger)
	if ( IsSolidFlagSet( FSOLID_TRIGGER ) )
		return false;

	return true;
}

const matrix3x4_t *CCollisionProperty::GetRootParentToWorldTransform() const
{
	if ( IsSolidFlagSet( FSOLID_ROOT_PARENT_ALIGNED ) )
	{
		CBaseEntity *pEntity = m_pOuter->GetRootMoveParent();
		Assert(pEntity);
		if ( pEntity )
		{
			return &pEntity->CollisionProp()->CollisionToWorldTransform();
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// IClientUnknown
//-----------------------------------------------------------------------------
IClientUnknown* CCollisionProperty::GetIClientUnknown()
{
#ifdef CLIENT_DLL
	return m_pOuter->GetIClientUnknown();
#else
	return NULL;
#endif
}



//-----------------------------------------------------------------------------
// Check for untouch
//-----------------------------------------------------------------------------
void CCollisionProperty::CheckForUntouch()
{
#ifndef CLIENT_DLL
	if ( !IsSolid() && !IsSolidFlagSet(FSOLID_TRIGGER))
	{
		// If this ent's touch list isn't empty, it's transitioning to not solid
		if ( m_pOuter->IsCurrentlyTouching() )
		{
			// mark ent so that at the end of frame it will check to 
			// see if it's no longer touching ents
			m_pOuter->SetCheckUntouch( true );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Sets the solid type
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolid( SolidType_t val )
{
	if ( m_nSolidType == val )
		return;

#ifndef CLIENT_DLL
	bool bWasNotSolid = IsSolid();
#endif

	MarkSurroundingBoundsDirty();

	// OBB is not yet implemented
	if ( val == SOLID_BSP )
	{
		if ( GetOuter()->GetMoveParent() )
		{
			if ( GetOuter()->GetRootMoveParent()->GetSolid() != SOLID_BSP )
			{
				// must be SOLID_VPHYSICS because parent might rotate
				val = SOLID_VPHYSICS;
			}
		}
#ifndef CLIENT_DLL
		// UNDONE: This should be fine in the client DLL too.  Move GetAllChildren() into shared code.
		// If the root of the hierarchy is SOLID_BSP, then assume that the designer
		// wants the collisions to rotate with this hierarchy so that the player can
		// move while riding the hierarchy.
		if ( !GetOuter()->GetMoveParent() )
		{
			// NOTE: This assumes things don't change back from SOLID_BSP
			// NOTE: This is 100% true for HL2 - need to support removing the flag to support changing from SOLID_BSP
			CUtlVector<CBaseEntity *> list;
			GetAllChildren( GetOuter(), list );
			for ( int i = list.Count()-1; i>=0; --i )
			{
				list[i]->AddSolidFlags( FSOLID_ROOT_PARENT_ALIGNED );
			}
		}
#endif
	}

	m_nSolidType = val;

#ifndef CLIENT_DLL
	m_pOuter->CollisionRulesChanged();

	UpdateServerPartitionMask( );

	if ( bWasNotSolid != IsSolid() )
	{
		CheckForUntouch();
	}
#endif
}

SolidType_t CCollisionProperty::GetSolid() const
{
	return (SolidType_t)m_nSolidType.Get();
}


//-----------------------------------------------------------------------------
// Sets the solid flags
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolidFlags( int flags )
{
	int oldFlags = m_usSolidFlags;
	m_usSolidFlags = (unsigned short)(flags & 0xFFFF);
	if ( oldFlags == m_usSolidFlags )
		return;

	// These two flags, if changed, can produce different surrounding bounds
	if ( (oldFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) != 
		 (m_usSolidFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) )
	{
		MarkSurroundingBoundsDirty();
	}

	if ( (oldFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) )
	{
		m_pOuter->CollisionRulesChanged();
	}

#ifndef CLIENT_DLL
	if ( (oldFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) )
	{
		UpdateServerPartitionMask( );
		CheckForUntouch();
	}
#endif
}


//-----------------------------------------------------------------------------
// Coordinate system of the collision model
//-----------------------------------------------------------------------------
const Vector& CCollisionProperty::GetCollisionOrigin() const
{
	return m_pOuter->GetAbsOrigin();
}

const QAngle& CCollisionProperty::GetCollisionAngles() const
{
	if ( IsBoundsDefinedInEntitySpace() )
	{
		return m_pOuter->GetAbsAngles();
	}

	return vec3_angle;
}

const matrix3x4_t& CCollisionProperty::CollisionToWorldTransform() const
{
	static matrix3x4_t s_matTemp[4];
	static int s_nIndex = 0;

	matrix3x4_t &matResult = s_matTemp[s_nIndex];
	s_nIndex = (s_nIndex+1) & 0x3;

	if ( IsBoundsDefinedInEntitySpace() )
	{
		return m_pOuter->EntityToWorldTransform();
	}

	SetIdentityMatrix( matResult );
	MatrixSetColumn( GetCollisionOrigin(), 3, matResult );
	return matResult;
}


//-----------------------------------------------------------------------------
// Sets the collision bounds + the size
//-----------------------------------------------------------------------------
void CCollisionProperty::SetCollisionBounds( const Vector &mins, const Vector &maxs )
{
	if ( ( m_vecMinsPreScaled != mins ) || ( m_vecMaxsPreScaled != maxs ) )
	{
		m_vecMinsPreScaled = mins;
		m_vecMaxsPreScaled = maxs;
	}

	bool bDirty = false;

	// Check if it's a scaled model
	CBaseAnimating *pAnim = GetOuter()->GetBaseAnimating();
	if ( pAnim && pAnim->GetModelScale() != 1.0f )
	{
		// Do the scaling
		Vector vecNewMins = mins * pAnim->GetModelScale();
		Vector vecNewMaxs = maxs * pAnim->GetModelScale();

		if ( ( m_vecMins != vecNewMins ) || ( m_vecMaxs != vecNewMaxs ) )
		{
			m_vecMins = vecNewMins;
			m_vecMaxs = vecNewMaxs;
			bDirty = true;
		}
	}
	else
	{
		// No scaling needed!
		if ( ( m_vecMins != mins ) || ( m_vecMaxs != maxs ) )
		{
			m_vecMins = mins;
			m_vecMaxs = maxs;
			bDirty = true;
		}
	}
	
	if ( bDirty )
	{
		//ASSERT_COORD( m_vecMins.Get() );
		//ASSERT_COORD( m_vecMaxs.Get() );

		Vector vecSize;
		VectorSubtract( m_vecMaxs, m_vecMins, vecSize );
		m_flRadius = vecSize.Length() * 0.5f;

		MarkSurroundingBoundsDirty();
	}
}

//-----------------------------------------------------------------------------
// Rebuilds the scaled bounds from the prescaled bounds after a model's scale has changed
//-----------------------------------------------------------------------------
void CCollisionProperty::RefreshScaledCollisionBounds( void )
{
	SetCollisionBounds( m_vecMinsPreScaled, m_vecMaxsPreScaled );

	SurroundingBoundsType_t nSurroundType = static_cast< SurroundingBoundsType_t >( m_nSurroundType.Get() );
	if ( nSurroundType == USE_SPECIFIED_BOUNDS )
	{
		SetSurroundingBoundsType( nSurroundType, 
								  &(m_vecSpecifiedSurroundingMinsPreScaled.Get()), 
								  &(m_vecSpecifiedSurroundingMaxsPreScaled.Get()) );
	}
	else
	{
		SetSurroundingBoundsType( nSurroundType );
	}
}


//-----------------------------------------------------------------------------
// Lazily calculates the 2D bounding radius. If we do this enough, we should
// calculate this in SetCollisionBounds above and cache the results in a data member!
//-----------------------------------------------------------------------------
float CCollisionProperty::BoundingRadius2D() const
{
	Vector vecSize;
	VectorSubtract( m_vecMaxs, m_vecMins, vecSize );

	vecSize.z = 0;	
	return vecSize.Length() * 0.5f;
}


//-----------------------------------------------------------------------------
// Special trigger representation (OBB)
//-----------------------------------------------------------------------------
void CCollisionProperty::WorldSpaceTriggerBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs ) const
{
	WorldSpaceAABB( pVecWorldMins, pVecWorldMaxs );
	if ( ( GetSolidFlags() & FSOLID_USE_TRIGGER_BOUNDS ) == 0 )
		return;

	// Don't bloat below, we don't want to trigger it with our heads
	pVecWorldMins->x -= m_triggerBloat;
	pVecWorldMins->y -= m_triggerBloat;

	pVecWorldMaxs->x += m_triggerBloat;
	pVecWorldMaxs->y += m_triggerBloat;
	pVecWorldMaxs->z += (float)m_triggerBloat * 0.5f;
}

void CCollisionProperty::UseTriggerBounds( bool bEnable, float flBloat )
{
	Assert( flBloat <= 127.0f );
	m_triggerBloat = (char )flBloat;
	if ( bEnable )
	{
		AddSolidFlags( FSOLID_USE_TRIGGER_BOUNDS );
		Assert( flBloat > 0.0f );
	}
	else
	{
		RemoveSolidFlags( FSOLID_USE_TRIGGER_BOUNDS );
	}
}


//-----------------------------------------------------------------------------
// Collision model (BSP)
//-----------------------------------------------------------------------------
int CCollisionProperty::GetCollisionModelIndex()
{
	return m_pOuter->GetModelIndex();
}

const model_t* CCollisionProperty::GetCollisionModel()
{
	return m_pOuter->GetModel();
}


//-----------------------------------------------------------------------------
// Collision methods implemented in the entity
// FIXME: This shouldn't happen there!!
//-----------------------------------------------------------------------------
bool CCollisionProperty::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return m_pOuter->TestCollision( ray, fContentsMask, tr );
}

bool CCollisionProperty::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return m_pOuter->TestHitboxes( ray, fContentsMask, tr );
}


//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in collision space
//-----------------------------------------------------------------------------
const Vector & CCollisionProperty::NormalizedToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	pResult->x = Lerp( in.x, m_vecMins.Get().x, m_vecMaxs.Get().x );
	pResult->y = Lerp( in.y, m_vecMins.Get().y, m_vecMaxs.Get().y );
	pResult->z = Lerp( in.z, m_vecMins.Get().z, m_vecMaxs.Get().z );
	return *pResult;
}


//-----------------------------------------------------------------------------
// Transforms a point in collision space to normalized space
//-----------------------------------------------------------------------------
const Vector &	CCollisionProperty::CollisionToNormalizedSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecSize = OBBSize( );
	pResult->x = ( vecSize.x != 0.0f ) ? ( in.x - m_vecMins.Get().x ) / vecSize.x : 0.5f;
	pResult->y = ( vecSize.y != 0.0f ) ? ( in.y - m_vecMins.Get().y ) / vecSize.y : 0.5f;
	pResult->z = ( vecSize.z != 0.0f ) ? ( in.z - m_vecMins.Get().z ) / vecSize.z : 0.5f;
	return *pResult;
}


//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in world space
//-----------------------------------------------------------------------------
const Vector & CCollisionProperty::NormalizedToWorldSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecCollisionSpace;
	NormalizedToCollisionSpace( in, &vecCollisionSpace );
	CollisionToWorldSpace( vecCollisionSpace, pResult );
	return *pResult;
}


//-----------------------------------------------------------------------------
// Transforms a point in world space to normalized space
//-----------------------------------------------------------------------------
const Vector & CCollisionProperty::WorldToNormalizedSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecCollisionSpace;
	WorldToCollisionSpace( in, &vecCollisionSpace );
	CollisionToNormalizedSpace( vecCollisionSpace, pResult );
	return *pResult;
}


//-----------------------------------------------------------------------------
// Selects a random point in the bounds given the normalized 0-1 bounds 
//-----------------------------------------------------------------------------
void CCollisionProperty::RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const
{
	Vector vecNormalizedSpace;
	vecNormalizedSpace.x = random->RandomFloat( vecNormalizedMins.x, vecNormalizedMaxs.x );
	vecNormalizedSpace.y = random->RandomFloat( vecNormalizedMins.y, vecNormalizedMaxs.y );
	vecNormalizedSpace.z = random->RandomFloat( vecNormalizedMins.z, vecNormalizedMaxs.z );
	NormalizedToWorldSpace( vecNormalizedSpace, pPoint );
}


//-----------------------------------------------------------------------------
// Transforms an AABB measured in entity space to a box that surrounds it in world space
//-----------------------------------------------------------------------------
void CCollisionProperty::CollisionAABBToWorldAABB( const Vector &entityMins, 
	const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle) )
	{
		VectorAdd( entityMins, GetCollisionOrigin(), *pWorldMins );
		VectorAdd( entityMaxs, GetCollisionOrigin(), *pWorldMaxs );
	}
	else
	{
		TransformAABB( CollisionToWorldTransform(), entityMins, entityMaxs, *pWorldMins, *pWorldMaxs );
	}
}

/*
void CCollisionProperty::WorldAABBToCollisionAABB( const Vector &worldMins, const Vector &worldMaxs, Vector *pEntityMins, Vector *pEntityMaxs ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle) )
	{
		VectorSubtract( worldMins, GetAbsOrigin(), *pEntityMins );
		VectorSubtract( worldMaxs, GetAbsOrigin(), *pEntityMaxs );
	}
	else
	{
		ITransformAABB( CollisionToWorldTransform(), worldMins, worldMaxs, *pEntityMins, *pEntityMaxs );
	}
}
*/


//-----------------------------------------------------------------------------
// Is a worldspace point within the bounds of the OBB?
//-----------------------------------------------------------------------------
bool CCollisionProperty::IsPointInBounds( const Vector &vecWorldPt ) const
{
	Vector vecLocalSpace;
	WorldToCollisionSpace( vecWorldPt, &vecLocalSpace );
	return ( ( vecLocalSpace.x >= m_vecMins.Get().x && vecLocalSpace.x <= m_vecMaxs.Get().x ) &&
			( vecLocalSpace.y >= m_vecMins.Get().y && vecLocalSpace.y <= m_vecMaxs.Get().y ) &&
			( vecLocalSpace.z >= m_vecMins.Get().z && vecLocalSpace.z <= m_vecMaxs.Get().z ) );
}

	
//-----------------------------------------------------------------------------
// Computes the nearest point in the OBB to a point specified in world space
//-----------------------------------------------------------------------------
void CCollisionProperty::CalcNearestPoint( const Vector &vecWorldPt, Vector *pVecNearestWorldPt ) const
{
	// Calculate physics force
	Vector localPt, localClosestPt;
	WorldToCollisionSpace( vecWorldPt, &localPt );
	CalcClosestPointOnAABB( m_vecMins.Get(), m_vecMaxs.Get(), localPt, localClosestPt );
	CollisionToWorldSpace( localClosestPt, pVecNearestWorldPt );
}


//-----------------------------------------------------------------------------
// Computes the nearest point in the OBB to a point specified in world space
//-----------------------------------------------------------------------------
float CCollisionProperty::CalcDistanceFromPoint( const Vector &vecWorldPt ) const
{
	// Calculate physics force
	Vector localPt, localClosestPt;
	WorldToCollisionSpace( vecWorldPt, &localPt );
	CalcClosestPointOnAABB( m_vecMins.Get(), m_vecMaxs.Get(), localPt, localClosestPt );
	return localPt.DistTo( localClosestPt );
}


//-----------------------------------------------------------------------------
// Compute the largest dot product of the OBB and the specified direction vector
//-----------------------------------------------------------------------------
float CCollisionProperty::ComputeSupportMap( const Vector &vecDirection ) const
{
	Vector vecCollisionDir;
	WorldDirectionToCollisionSpace( vecDirection, &vecCollisionDir );

	float flResult = DotProduct( GetCollisionOrigin(), vecDirection );
	flResult += (( vecCollisionDir.x >= 0.0f ) ? m_vecMaxs.Get().x : m_vecMins.Get().x) * vecCollisionDir.x;
	flResult += (( vecCollisionDir.y >= 0.0f ) ? m_vecMaxs.Get().y : m_vecMins.Get().y) * vecCollisionDir.y;
	flResult += (( vecCollisionDir.z >= 0.0f ) ? m_vecMaxs.Get().z : m_vecMins.Get().z) * vecCollisionDir.z;

	return flResult;
}


//-----------------------------------------------------------------------------
// Expand trigger bounds..
//-----------------------------------------------------------------------------
void CCollisionProperty::ComputeVPhysicsSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	bool bSetBounds = false;
	IPhysicsObject *pPhysicsObject = GetOuter()->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		if ( pPhysicsObject->GetCollide() )
		{
			physcollision->CollideGetAABB( pVecWorldMins, pVecWorldMaxs, 
				pPhysicsObject->GetCollide(), GetCollisionOrigin(), GetCollisionAngles() );
			bSetBounds = true;
		}
		else if ( pPhysicsObject->GetSphereRadius( ) )
		{
			float flRadius = pPhysicsObject->GetSphereRadius( );
			Vector vecExtents( flRadius, flRadius, flRadius );
			VectorSubtract( GetCollisionOrigin(), vecExtents, *pVecWorldMins );
			VectorAdd( GetCollisionOrigin(), vecExtents, *pVecWorldMaxs );
			bSetBounds = true;
		}
	}

	if ( !bSetBounds )
	{
		*pVecWorldMins = GetCollisionOrigin();
		*pVecWorldMaxs = *pVecWorldMins;
	}

	// Also, lets expand for the trigger bounds also
	if ( IsSolidFlagSet( FSOLID_USE_TRIGGER_BOUNDS ) )
	{
		Vector vecWorldTriggerMins, vecWorldTriggerMaxs;
		WorldSpaceTriggerBounds( &vecWorldTriggerMins, &vecWorldTriggerMaxs );
		VectorMin( vecWorldTriggerMins, *pVecWorldMins, *pVecWorldMins );
		VectorMax( vecWorldTriggerMaxs, *pVecWorldMaxs, *pVecWorldMaxs );
	}
}


//-----------------------------------------------------------------------------
// Expand trigger bounds..
//-----------------------------------------------------------------------------
bool CCollisionProperty::ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	CBaseAnimating *pAnim = GetOuter()->GetBaseAnimating();
	if (pAnim)
	{
		return pAnim->ComputeHitboxSurroundingBox( pVecWorldMins, pVecWorldMaxs );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Expand trigger bounds..
//-----------------------------------------------------------------------------
bool CCollisionProperty::ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	CBaseAnimating *pAnim = GetOuter()->GetBaseAnimating();
	if (pAnim)
	{
		return pAnim->ComputeEntitySpaceHitboxSurroundingBox( pVecWorldMins, pVecWorldMaxs );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Computes the surrounding collision bounds from the the OBB (not vphysics)
//-----------------------------------------------------------------------------
void CCollisionProperty::ComputeRotationExpandedBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	if ( !IsBoundsDefinedInEntitySpace() )
	{
		*pVecWorldMins = m_vecMins;
		*pVecWorldMaxs = m_vecMaxs;
	}
	else
	{
		float flMaxVal;
		flMaxVal = MAX( FloatMakePositive(m_vecMins.Get().x), FloatMakePositive(m_vecMaxs.Get().x) );
		pVecWorldMins->x = -flMaxVal;
		pVecWorldMaxs->x = flMaxVal;

		flMaxVal = MAX( FloatMakePositive(m_vecMins.Get().y), FloatMakePositive(m_vecMaxs.Get().y) );
		pVecWorldMins->y = -flMaxVal;
		pVecWorldMaxs->y = flMaxVal;

		flMaxVal = MAX( FloatMakePositive(m_vecMins.Get().z), FloatMakePositive(m_vecMaxs.Get().z) );
		pVecWorldMins->z = -flMaxVal;
		pVecWorldMaxs->z = flMaxVal;
	}
}


//-----------------------------------------------------------------------------
// Computes the surrounding collision bounds based on whatever algorithm we want...
//-----------------------------------------------------------------------------
void CCollisionProperty::ComputeCollisionSurroundingBox( bool bUseVPhysics, Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	Assert( GetSolid() != SOLID_CUSTOM );

	// NOTE: For solid none, we are still going to use the bounds; necessary because
	// the surrounding box is used for the PVS...
	// FIXME: Should we make some other call for the PVS stuff?? If so, we should return
	// a point bounds for SOLID_NONE...
//	if ( GetSolid() == SOLID_NONE )
//	{
//		*pVecWorldMins = GetCollisionOrigin();
//		*pVecWorldMaxs = *pVecWorldMins;
//		return;
//	}

	if ( bUseVPhysics )
	{
		ComputeVPhysicsSurroundingBox( pVecWorldMins, pVecWorldMaxs );
	}
	else
	{
		// Will expand the bounds for the trigger, if it is a trigger
		WorldSpaceTriggerBounds( pVecWorldMins, pVecWorldMaxs );
	}
}

  
//-----------------------------------------------------------------------------
// Computes the surrounding collision bounds based on whatever algorithm we want...
//-----------------------------------------------------------------------------
void CCollisionProperty::ComputeSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	if (( GetSolid() == SOLID_CUSTOM ) && (m_nSurroundType != USE_GAME_CODE ))
	{
		// NOTE: This can only happen in transition periods, say during network
		// reception on the client. We expect USE_GAME_CODE to be used with SOLID_CUSTOM
		*pVecWorldMins = GetCollisionOrigin();
		*pVecWorldMaxs = *pVecWorldMins;
		return;
	}

	switch( m_nSurroundType )
	{
	case USE_OBB_COLLISION_BOUNDS:
		{
			Assert( GetSolid() != SOLID_CUSTOM );
			bool bUseVPhysics = false;
			if ( ( GetSolid() == SOLID_VPHYSICS ) && ( GetOuter()->GetMoveType() == MOVETYPE_VPHYSICS ) )
			{
				// UNDONE: This may not be necessary any more.
				IPhysicsObject *pPhysics = GetOuter()->VPhysicsGetObject();
				bUseVPhysics = pPhysics && pPhysics->IsAsleep();
			}
			ComputeCollisionSurroundingBox( bUseVPhysics, pVecWorldMins, pVecWorldMaxs );
		}
		break;

	case USE_BEST_COLLISION_BOUNDS:
		Assert( GetSolid() != SOLID_CUSTOM );
		ComputeCollisionSurroundingBox( (GetSolid() == SOLID_VPHYSICS), pVecWorldMins, pVecWorldMaxs );
		break;

	case USE_COLLISION_BOUNDS_NEVER_VPHYSICS:
		Assert( GetSolid() != SOLID_CUSTOM );
		ComputeCollisionSurroundingBox( false, pVecWorldMins, pVecWorldMaxs );
		break;

	case USE_HITBOXES:
		ComputeHitboxSurroundingBox( pVecWorldMins, pVecWorldMaxs );
		break;

	case USE_ROTATION_EXPANDED_BOUNDS:
		ComputeRotationExpandedBounds( pVecWorldMins, pVecWorldMaxs );
		break;

	case USE_SPECIFIED_BOUNDS:
		VectorAdd( GetCollisionOrigin(), m_vecSpecifiedSurroundingMins, *pVecWorldMins );
		VectorAdd( GetCollisionOrigin(), m_vecSpecifiedSurroundingMaxs, *pVecWorldMaxs );
		break;

	case USE_GAME_CODE:
		GetOuter()->ComputeWorldSpaceSurroundingBox( pVecWorldMins, pVecWorldMaxs );
		Assert( pVecWorldMins->x <= pVecWorldMaxs->x );
		Assert( pVecWorldMins->y <= pVecWorldMaxs->y );
		Assert( pVecWorldMins->z <= pVecWorldMaxs->z );
		return;
	}

#ifdef DEBUG
	/*
	// For debugging purposes, make sure the bounds actually does surround the thing.
	// Otherwise the optimization we were using isn't really all that great, is it?
	Vector vecTestMins, vecTestMaxs;
	ComputeCollisionSurroundingBox( (GetSolid() == SOLID_VPHYSICS), &vecTestMins, &vecTestMaxs );

	// Now that we have the basics, let's expand for hitboxes if appropriate
	Vector vecWorldHitboxMins, vecWorldHitboxMaxs;
	if ( ComputeHitboxSurroundingBox( &vecWorldHitboxMins, &vecWorldHitboxMaxs ) )
	{
		VectorMin( vecWorldHitboxMaxs, vecTestMins, vecTestMins );
		VectorMax( vecWorldHitboxMaxs, vecTestMaxs, vecTestMaxs );
	}

	Assert( vecTestMins.x >= pVecWorldMins->x && vecTestMins.y >= pVecWorldMins->y && vecTestMins.z >= pVecWorldMins->z );
	Assert( vecTestMaxs.x <= pVecWorldMaxs->x && vecTestMaxs.y <= pVecWorldMaxs->y && vecTestMaxs.z <= pVecWorldMaxs->z );
	*/
#endif
}


//-----------------------------------------------------------------------------
// Sets the method by which the surrounding collision bounds is set
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSurroundingBoundsType( SurroundingBoundsType_t type, const Vector *pMins, const Vector *pMaxs )
{	
	m_nSurroundType = type;
	if (type != USE_SPECIFIED_BOUNDS)
	{
		Assert( !pMins && !pMaxs );
		MarkSurroundingBoundsDirty();
	}
	else
	{
		Assert( pMins && pMaxs );
		m_vecSpecifiedSurroundingMinsPreScaled = *pMins;
		m_vecSpecifiedSurroundingMaxsPreScaled = *pMaxs;

		// Check if it's a scaled model
		CBaseAnimating *pAnim = GetOuter()->GetBaseAnimating();
		if ( pAnim && pAnim->GetModelScale() != 1.0f )
		{
			// Do the scaling
			Vector vecNewMins = *pMins * pAnim->GetModelScale();
			Vector vecNewMaxs = *pMaxs * pAnim->GetModelScale();

			m_vecSpecifiedSurroundingMins = vecNewMins;
			m_vecSpecifiedSurroundingMaxs = vecNewMaxs;
			m_vecSurroundingMins = vecNewMins;
			m_vecSurroundingMaxs = vecNewMaxs;

		}
		else
		{
			// No scaling needed!
			m_vecSpecifiedSurroundingMins = *pMins;
			m_vecSpecifiedSurroundingMaxs = *pMaxs;
			m_vecSurroundingMins = *pMins;
			m_vecSurroundingMaxs = *pMaxs;
			
		}

		ASSERT_COORD( m_vecSurroundingMins );
		ASSERT_COORD( m_vecSurroundingMaxs );
	}
}


//-----------------------------------------------------------------------------
// Marks the entity has having a dirty surrounding box
//-----------------------------------------------------------------------------
void CCollisionProperty::MarkSurroundingBoundsDirty()
{
	GetOuter()->AddEFlags( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS );
	MarkPartitionHandleDirty();

#ifdef CLIENT_DLL
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetOuter()->GetShadowHandle() );
#else
	GetOuter()->NetworkProp()->MarkPVSInformationDirty();
#endif
}


//-----------------------------------------------------------------------------
// Does VPhysicsUpdate make us need to recompute the surrounding box?
//-----------------------------------------------------------------------------
bool CCollisionProperty::DoesVPhysicsInvalidateSurroundingBox( ) const
{
	switch ( m_nSurroundType )
	{
	case USE_BEST_COLLISION_BOUNDS:
		return true;

	case USE_OBB_COLLISION_BOUNDS:
		return (GetSolid() == SOLID_VPHYSICS) && (GetOuter()->GetMoveType() == MOVETYPE_VPHYSICS) && GetOuter()->VPhysicsGetObject();

	// In the case of game code, we don't really know, so we have to assume it does
	case USE_GAME_CODE:
		return true;

	case USE_COLLISION_BOUNDS_NEVER_VPHYSICS:
	case USE_HITBOXES:
	case USE_ROTATION_EXPANDED_BOUNDS:
	case USE_SPECIFIED_BOUNDS:
		return false;

	default:
		Assert(0);
		return true;
	}
}


//-----------------------------------------------------------------------------
// Computes the surrounding collision bounds based on whatever algorithm we want...
//-----------------------------------------------------------------------------
void CCollisionProperty::WorldSpaceSurroundingBounds( Vector *pVecMins, Vector *pVecMaxs )
{
	const Vector &vecAbsOrigin = GetCollisionOrigin();
	if ( GetOuter()->IsEFlagSet( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS ))
	{
		GetOuter()->RemoveEFlags( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS );
		ComputeSurroundingBox( pVecMins, pVecMaxs );
		VectorSubtract( *pVecMins, vecAbsOrigin, m_vecSurroundingMins );
		VectorSubtract( *pVecMaxs, vecAbsOrigin, m_vecSurroundingMaxs );
		
		ASSERT_COORD( m_vecSurroundingMins );
		ASSERT_COORD( m_vecSurroundingMaxs );
	}
	else
	{
		VectorAdd( m_vecSurroundingMins, vecAbsOrigin, *pVecMins );
		VectorAdd( m_vecSurroundingMaxs, vecAbsOrigin, *pVecMaxs );
	}
}


//-----------------------------------------------------------------------------
// Spatial partition
//-----------------------------------------------------------------------------
void CCollisionProperty::CreatePartitionHandle()
{
	// Put the entity into the spatial partition.
	Assert( m_Partition == PARTITION_INVALID_HANDLE );
	m_Partition = partition->CreateHandle( GetEntityHandle() );
}

void CCollisionProperty::DestroyPartitionHandle()
{
	if ( m_Partition != PARTITION_INVALID_HANDLE )
	{
		partition->DestroyHandle( m_Partition );
		m_Partition = PARTITION_INVALID_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Updates the spatial partition
//-----------------------------------------------------------------------------
void CCollisionProperty::UpdateServerPartitionMask( )
{
#ifndef CLIENT_DLL
	SpatialPartitionHandle_t handle = GetPartitionHandle();
	if ( handle == PARTITION_INVALID_HANDLE )
		return;

	// Remove it from whatever lists it may be in at the moment
	// We'll re-add it below if we need to.
	partition->Remove( handle );

	// Don't bother with deleted things
	if ( !m_pOuter->edict() )
		return;

	// don't add the world
	if ( m_pOuter->entindex() == 0 )
		return;		

	// Make sure it's in the list of all entities
	bool bIsSolid = IsSolid() || IsSolidFlagSet(FSOLID_TRIGGER);
	if ( bIsSolid || m_pOuter->IsEFlagSet(EFL_USE_PARTITION_WHEN_NOT_SOLID) )
	{
		partition->Insert( PARTITION_ENGINE_NON_STATIC_EDICTS, handle );
	}

	if ( !bIsSolid )
		return;

	// Insert it into the appropriate lists.
	// We have to continually reinsert it because its solid type may have changed
	SpatialPartitionListMask_t mask = 0;
	if ( !IsSolidFlagSet(FSOLID_NOT_SOLID) )
	{
		mask |=	PARTITION_ENGINE_SOLID_EDICTS;
	}
	if ( IsSolidFlagSet(FSOLID_TRIGGER) )
	{
		mask |=	PARTITION_ENGINE_TRIGGER_EDICTS;
	}
	Assert( mask != 0 );
	partition->Insert( mask, handle );
#endif
}


//-----------------------------------------------------------------------------
// Marks the spatial partition dirty
//-----------------------------------------------------------------------------
void CCollisionProperty::MarkPartitionHandleDirty()
{
	// don't bother with the world
	if ( m_pOuter->entindex() == 0 )
		return;
	
	if ( !m_pOuter->IsEFlagSet( EFL_DIRTY_SPATIAL_PARTITION ) )
	{
		m_pOuter->AddEFlags( EFL_DIRTY_SPATIAL_PARTITION );
		s_DirtyKDTree.AddEntity( m_pOuter );
	}

#ifdef CLIENT_DLL
	GetOuter()->MarkRenderHandleDirty();
	g_pClientShadowMgr->AddToDirtyShadowList( GetOuter() );
#endif
}


//-----------------------------------------------------------------------------
// Updates the spatial partition
//-----------------------------------------------------------------------------
void CCollisionProperty::UpdatePartition( )
{
	if ( m_pOuter->IsEFlagSet( EFL_DIRTY_SPATIAL_PARTITION ) )
	{
		m_pOuter->RemoveEFlags( EFL_DIRTY_SPATIAL_PARTITION );

#ifndef CLIENT_DLL
		Assert( m_pOuter->entindex() != 0 );

		// Don't bother with deleted things
		if ( !m_pOuter->edict() )
			return;

		if ( GetPartitionHandle() == PARTITION_INVALID_HANDLE )
		{
			CreatePartitionHandle();
			UpdateServerPartitionMask();
		}
#else
		if ( GetPartitionHandle() == PARTITION_INVALID_HANDLE )
			return;
#endif

		// We don't need to bother if it's not a trigger or solid
		if ( IsSolid() || IsSolidFlagSet( FSOLID_TRIGGER ) || m_pOuter->IsEFlagSet( EFL_USE_PARTITION_WHEN_NOT_SOLID ) )
		{
			// Bloat a little bit...
			if ( BoundingRadius() != 0.0f )
			{
				Vector vecSurroundMins, vecSurroundMaxs;
				WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
				vecSurroundMins -= Vector( 1, 1, 1 );
				vecSurroundMaxs += Vector( 1, 1, 1 );
				partition->ElementMoved( GetPartitionHandle(), vecSurroundMins,  vecSurroundMaxs );
			}
			else
			{
				partition->ElementMoved( GetPartitionHandle(), GetCollisionOrigin(),  GetCollisionOrigin() );
			}
		}
	}
}


