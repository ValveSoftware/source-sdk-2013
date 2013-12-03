//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ANTLION_MAKER_H
#define ANTLION_MAKER_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_antlion.h"
#include "monstermaker.h"
#include "igamesystem.h"
#include "ai_hint.h"

//
// Antlion maker class
//

#define	SF_ANTLIONMAKER_RANDOM_SPAWN_NODE	0x00000400
#define	SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET	0x00000800
#define	SF_ANTLIONMAKER_RANDOM_FIGHT_TARGET	0x00001000
#define	SF_ANTLIONMAKER_DO_BLOCKEDEFFECTS	0x00002000

class CAntlionTemplateMaker : public CTemplateNPCMaker
{
	public:

	DECLARE_CLASS( CAntlionTemplateMaker, CTemplateNPCMaker );

			CAntlionTemplateMaker( void );
			~CAntlionTemplateMaker( void );

	virtual int DrawDebugTextOverlays( void );
	virtual void DrawDebugGeometryOverlays( void );

	void	MakeNPC( void );
	void	ChildPreSpawn( CAI_BaseNPC *pChild );
	void	ChildPostSpawn( CAI_BaseNPC *pChild );

	void	InputSetFightTarget( inputdata_t &inputdata );
	void	InputSetFollowTarget( inputdata_t &inputdata );
	void	InputClearFightTarget( inputdata_t &inputdata );
	void	InputClearFollowTarget( inputdata_t &inputdata );
	void	InputSetSpawnRadius( inputdata_t &inputdata );
	void	InputAddToPool( inputdata_t &inputdata );
	void	InputSetMaxPool( inputdata_t &inputdata );
	void	InputSetPoolRegenAmount( inputdata_t &inputdata );
	void	InputSetPoolRegenTime( inputdata_t &inputdata );
	void	InputChangeDestinationGroup( inputdata_t &inputdata );

	void	Activate( void );
	
	// Do not transition
	int		ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	
	bool	CanMakeNPC( bool bIgnoreSolidEntities = false );
	bool	ShouldAlwaysThink( void ) { return true; }

	void	AddChild( CNPC_Antlion *pAnt );
	void	RemoveChild( CNPC_Antlion *pAnt );

	void	FixupOrphans( void );
	void	UpdateChildren( void );

	void	CreateProxyTarget( const Vector &position );
	void	DestroyProxyTarget( void );

	void	SetFightTarget( string_t strTarget, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void	SetFightTarget( CBaseEntity *pEntity );
	void	SetFightTarget( const Vector &position );
	
	void	SetFollowTarget( string_t strTarget, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void	SetFollowTarget( CBaseEntity *pEntity );

	void	SetChildMoveState( AntlionMoveState_e state );

	void	DeathNotice( CBaseEntity *pVictim );
	bool	IsDepleted( void );

	bool	ShouldHearBugbait( void ) { return (m_bIgnoreBugbait==false); }

	CBaseEntity	*GetFightTarget( void );
	CBaseEntity *GetFollowTarget( void );

	virtual void Enable( void );
	virtual void Disable( void );


	void	BlockedCheckFunc( void );
	void	FindNodesCloseToPlayer( void );
	void	DoBlockedEffects( CBaseEntity *pBlocker, Vector vOrigin );

	CBaseEntity	*AllHintsFromClusterBlocked( CAI_Hint *pNode, bool &bChosenHintBlocked );

	void	ActivateAllSpores( void );
	void	ActivateSpore( const char* sporename, Vector vOrigin );
	void	DisableSpore( const char* sporename );
	void	DisableAllSpores( void );

protected:

	void		PrecacheTemplateEntity( CBaseEntity *pEntity );

	bool		FindHintSpawnPosition( const Vector &origin, float radius, string_t hintGroupName, CAI_Hint **pHint, bool bRandom = false );
	bool		FindNearTargetSpawnPosition( Vector &origin, float radius, CBaseEntity *pTarget );

	//These are used by FindNearTargetSpawnPosition
	bool		FindPositionOnFoot( Vector &origin, float radius, CBaseEntity *pTarget );
	bool		FindPositionOnVehicle( Vector &origin, float radius, CBaseEntity *pTarget );
	bool		ValidateSpawnPosition( Vector &vOrigin, CBaseEntity *pTarget = NULL );

	// Pool behavior for coast
	void		PoolAdd( int iNumToAdd );
	void		PoolRegenThink( void );

protected:
	// FIXME: The m_strSpawnGroup is redundant to the m_iszDestinationGroup in the base class NPC template maker
	string_t	m_strSpawnGroup;	// if present, spawn children on the nearest node of this group (to the player)
	string_t	m_strSpawnTarget;	// name of target to spawn near
	float		m_flSpawnRadius;	// radius around target to attempt to spawn in
	float		m_flWorkerSpawnRate;	// Percentage chance of spawning a worker when we spawn an antlion [0..1].
	
	string_t	m_strFightTarget;	// target entity name that all children will be told to fight to
	string_t	m_strFollowTarget;	// entity name that all children will follow

	bool		m_bIgnoreBugbait;		// Whether or not to ignore bugbait
	
	AntlionMoveState_e	m_nChildMoveState;

	EHANDLE		m_hFightTarget;		// A normal entity pointer for fight position
	EHANDLE		m_hProxyTarget;		// This is a self-held target that is created and used when a vector is passed in as a fight
									// goal, instead of an entity
	EHANDLE		m_hFollowTarget;	// Target to follow

	CUtlVector< CHandle< CNPC_Antlion > >	m_Children;

	// Pool behavior for coast
	int			m_iPool;
	int			m_iMaxPool;
	int			m_iPoolRegenAmount;
	float		m_flPoolRegenTime;

	float		m_flVehicleSpawnDistance;
	
	int			m_iSkinCount;

	float		m_flBlockedBumpTime;

	bool		m_bBlocked;
	COutputEvent m_OnAllBlocked;

	bool		m_bCreateSpores;
		
	DECLARE_DATADESC();
};

// ========================================================
// Antlion maker manager
// ========================================================

class CAntlionMakerManager : public CAutoGameSystem
{
public:
	CAntlionMakerManager( char const *name ) : CAutoGameSystem( name )
	{
	}

	void	LevelInitPostEntity( void );

	void	BroadcastFightGoal( const Vector &vFightGoal );
	void	BroadcastFightGoal( CBaseEntity *pFightGoal );
	void	BroadcastFollowGoal( CBaseEntity *pFollowGoal );

protected:
	
	void	GatherMakers( void );

	CUtlVector< CHandle< CAntlionTemplateMaker > >	m_Makers;
};

extern CAntlionMakerManager g_AntlionMakerManager;

#endif // ANTLION_MAKER_H
