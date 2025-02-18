//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_COMPONENT_H
#define AI_COMPONENT_H

#if defined( _WIN32 )
#pragma once
#endif

class CAI_BaseNPC;
class CAI_Enemies;
typedef intp AI_TaskFailureCode_t;
struct Task_t;

//-----------------------------------------------------------------------------
// CAI_Component
//
// Purpose: Shared functionality of all classes that assume some of the 
//			responsibilities of an owner AI. 
//-----------------------------------------------------------------------------

class CAI_Component
{
	DECLARE_CLASS_NOBASE( CAI_Component );
protected:
	CAI_Component( CAI_BaseNPC *pOuter = NULL )
	 : m_pOuter(pOuter)
	{
	}

	virtual ~CAI_Component() {}

public:
	virtual void SetOuter( CAI_BaseNPC *pOuter )	{ m_pOuter = pOuter; }

	CAI_BaseNPC *		GetOuter() 			{ return m_pOuter; }
	const CAI_BaseNPC *	GetOuter() const 	{ return m_pOuter; }

	Hull_t				GetHullType() const;
	float 				GetHullWidth() const;
	float 				GetHullHeight() const;
	const Vector &		GetHullMins() const;
	const Vector &		GetHullMaxs() const;

protected:
	//
	// Common services provided by CAI_BaseNPC, Convenience methods to simplify derived code
	//
	edict_t *			GetEdict();
	
	const Vector &		GetLocalOrigin() const;
	void 				SetLocalOrigin( const Vector &origin );

	const Vector &		GetAbsOrigin() const;
	const QAngle&		GetAbsAngles() const;
	
	void				SetLocalAngles( const QAngle& angles );
	const QAngle &		GetLocalAngles( void ) const;
	
	const Vector&		WorldAlignMins() const;
	const Vector&		WorldAlignMaxs() const;
	Vector 				WorldSpaceCenter() const;
	
	int 				GetCollisionGroup() const;
	
	void				SetSolid( SolidType_t val );
	SolidType_t			GetSolid() const;
	
	float				GetGravity() const;
	void				SetGravity( float );

	CBaseEntity*		GetEnemy();
	const Vector &		GetEnemyLKP() const;
	void				TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition);
	
	CBaseEntity*		GetTarget();
	void				SetTarget( CBaseEntity *pTarget );
	
	const Task_t*		GetCurTask( void );
	virtual void		TaskFail( AI_TaskFailureCode_t );
	void				TaskFail( const char *pszGeneralFailText );
	virtual void		TaskComplete( bool fIgnoreSetFailedCondition = false );
	int					TaskIsRunning();
	inline int			TaskIsComplete();

	Activity			GetActivity();
	void				SetActivity( Activity NewActivity );
	float				GetIdealSpeed() const;
	float				GetIdealAccel() const;
	int					GetSequence();

	int					GetEntFlags() const;
	void				AddEntFlag( int flags );
	void				RemoveEntFlag( int flagsToRemove );
	void				ToggleEntFlag( int flagToToggle );

	void				SetGroundEntity( CBaseEntity *ground );

	CBaseEntity*		GetGoalEnt();
	void				SetGoalEnt( CBaseEntity *pGoalEnt );
	
	void				Remember( int iMemory );
	void				Forget( int iMemory );
	bool				HasMemory( int iMemory );

	CAI_Enemies *		GetEnemies();
	
	const char * 		GetEntClassname();
	
	int					CapabilitiesGet();

	float				GetLastThink( const char *szContext = NULL );

public:
#if defined(new)
#error
#endif

	void *operator new( size_t nBytes )
	{
		MEM_ALLOC_CREDIT();
		void *pResult = MemAlloc_Alloc( nBytes );
		memset( pResult, 0, nBytes );
		return pResult;
	};

	void *operator new( size_t nBytes, int nBlockUse, const char *pFileName, int nLine )
	{
		MEM_ALLOC_CREDIT();
		void *pResult = MemAlloc_Alloc( nBytes, pFileName, nLine );
		memset( pResult, 0, nBytes );
		return pResult;
	}

private:
	CAI_BaseNPC *m_pOuter;
};

//-----------------------------------------------------------------------------

template <class NPC_CLASS, class BASE_COMPONENT = CAI_Component>
class CAI_ComponentWithOuter : public BASE_COMPONENT
{
protected:
	CAI_ComponentWithOuter(NPC_CLASS *pOuter = NULL)
	 : BASE_COMPONENT(pOuter)
	{
	}

public:
	// Hides base version
	void SetOuter( NPC_CLASS *pOuter )		{ BASE_COMPONENT::SetOuter((CAI_BaseNPC *)pOuter); }
	NPC_CLASS * 		GetOuter() 			{ return (NPC_CLASS *)(BASE_COMPONENT::GetOuter()); }
	const NPC_CLASS *	GetOuter() const 	{ return (NPC_CLASS *)(BASE_COMPONENT::GetOuter()); }
};

//-----------------------------------------------------------------------------

#define DEFINE_AI_COMPONENT_OUTER( NPC_CLASS ) \
	void SetOuter( NPC_CLASS *pOuter )		{ CAI_Component::SetOuter((CAI_BaseNPC *)pOuter); } \
	NPC_CLASS * 		GetOuter() 			{ return (NPC_CLASS *)(CAI_Component::GetOuter()); } \
	const NPC_CLASS *	GetOuter() const 	{ return (NPC_CLASS *)(CAI_Component::GetOuter()); }

//-----------------------------------------------------------------------------

#endif // AI_COMPONENT_H
