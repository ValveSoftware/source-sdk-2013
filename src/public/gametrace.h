//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMETRACE_H
#define GAMETRACE_H
#ifdef _WIN32
#pragma once
#endif


#include "cmodel.h"
#include "utlvector.h"
#include "ihandleentity.h"
#include "ispatialpartition.h"

#if defined( CLIENT_DLL )
	class C_BaseEntity;
#else
	class CBaseEntity;
#endif


//-----------------------------------------------------------------------------
// Purpose: A trace is returned when a box is swept through the world
// NOTE: eventually more of this class should be moved up into the base class!!
//-----------------------------------------------------------------------------
class CGameTrace : public CBaseTrace
{
public:

	// Returns true if hEnt points at the world entity.
	// If this returns true, then you can't use GetHitBoxIndex().
	bool DidHitWorld() const;
	
	// Returns true if we hit something and it wasn't the world.
	bool DidHitNonWorldEntity() const;

	// Gets the entity's network index if the trace has hit an entity.
	// If not, returns -1.
	int GetEntityIndex() const;

	// Returns true if there was any kind of impact at all
	bool DidHit() const;

	// The engine doesn't know what a CBaseEntity is, so it has a backdoor to 
	// let it get at the edict.
#if defined( ENGINE_DLL )
	void SetEdict( edict_t *pEdict );
	edict_t* GetEdict() const;
#endif	


public:

	float		fractionleftsolid;		// time we left a solid, only valid if we started in solid
	csurface_t	surface;				// surface hit (impact surface)

	int			hitgroup;				// 0 == generic, non-zero is specific body part
	short		physicsbone;			// physics bone hit by trace in studio

#if defined( CLIENT_DLL )
		C_BaseEntity *m_pEnt;
#else
		CBaseEntity *m_pEnt;
#endif

	// NOTE: this member is overloaded.
	// If hEnt points at the world entity, then this is the static prop index.
	// Otherwise, this is the hitbox index.
	int			hitbox;					// box hit by trace in studio

	CGameTrace() = default;

private:
	// No copy constructors allowed
	CGameTrace(const CGameTrace& vOther);
};


//-----------------------------------------------------------------------------
// Returns true if there was any kind of impact at all
//-----------------------------------------------------------------------------
inline bool CGameTrace::DidHit() const 
{ 
	return fraction < 1 || allsolid || startsolid; 
}


typedef CGameTrace trace_t;

//=============================================================================

#define TLD_DEF_LEAF_MAX	256
#define TLD_DEF_ENTITY_MAX	1024

// misyl: Made final to workaround the warning:
//   "deleting object of polymorphic class type 'CTraceListData' which has non-virtual destructor might cause undefined behavior [-Wdelete-non-virtual-dtor]"
// We can't change the ABI of IPartitionEnumerator, so...
// Feel free to make a version of this class that's non final, and a final version for existing code and do your own thing in your own mod, if this breaks your usage.
class CTraceListData final : public IPartitionEnumerator
{
public:

	CTraceListData( int nLeafMax = TLD_DEF_LEAF_MAX, int nEntityMax = TLD_DEF_ENTITY_MAX )
	{
		MEM_ALLOC_CREDIT();
		m_nLeafCount = 0;
		m_aLeafList.SetSize( nLeafMax );

		m_nEntityCount = 0;
		m_aEntityList.SetSize( nEntityMax );
	}

	~CTraceListData()
	{
		m_nLeafCount = 0;
		m_aLeafList.RemoveAll();

		m_nEntityCount = 0;
		m_aEntityList.RemoveAll();
	}

	void Reset( void )
	{
		m_nLeafCount = 0;
		m_nEntityCount = 0;
	}

	bool	IsEmpty( void ) const			{ return ( m_nLeafCount == 0 && m_nEntityCount == 0 ); }

	int		LeafCount( void ) const			{ return m_nLeafCount; }
	int		LeafCountMax( void ) const		{ return m_aLeafList.Count(); }
	void    LeafCountReset( void )			{ m_nLeafCount = 0; }

	int		EntityCount( void ) const		{ return m_nEntityCount; }
	int		EntityCountMax( void ) const	{ return m_aEntityList.Count(); }
	void	EntityCountReset( void )		{ m_nEntityCount = 0; }

	// For leaves...
	void AddLeaf( int iLeaf )
	{
		if ( m_nLeafCount >= m_aLeafList.Count() )
		{
			DevMsg( "CTraceListData: Max leaf count along ray exceeded!\n" );
			m_aLeafList.AddMultipleToTail( m_aLeafList.Count() );
		}

		m_aLeafList[m_nLeafCount] = iLeaf;
		m_nLeafCount++;
	}

	// For entities...
	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		if ( m_nEntityCount >= m_aEntityList.Count() )
		{
			DevMsg( "CTraceListData: Max entity count along ray exceeded!\n" );
			m_aEntityList.AddMultipleToTail( m_aEntityList.Count() );
		}

		m_aEntityList[m_nEntityCount] = pHandleEntity;
		m_nEntityCount++;

		return ITERATION_CONTINUE;
	}
	
public:

	int							m_nLeafCount;
	CUtlVector<int>				m_aLeafList;

	int							m_nEntityCount;
	CUtlVector<IHandleEntity*>	m_aEntityList;
};

#endif // GAMETRACE_H

