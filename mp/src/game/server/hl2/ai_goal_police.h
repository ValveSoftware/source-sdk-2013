//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_GOAL_POLICE_H
#define AI_GOAL_POLICE_H
#ifdef _WIN32
#pragma once
#endif

class CAI_PoliceGoal : public CBaseEntity
{
public:

	DECLARE_CLASS( CAI_PoliceGoal, CBaseEntity );

				CAI_PoliceGoal( void );

	float		GetRadius( void );
	CBaseEntity *GetTarget( void );

	bool		ShouldKnockOutTarget( const Vector &targetPos, bool bTargetVisible );	// If the target should be knocked out
	void		KnockOutTarget( CBaseEntity *pTarget );									// Send an output that we've knocked out this target
	bool		ShouldRemainAtPost( void );

	void		InputEnableKnockOut( inputdata_t &data );
	void		InputDisableKnockOut( inputdata_t &data );

	void		FireWarningLevelOutput( int level );

	float		m_flRadius;
	EHANDLE		m_hTarget;
	string_t	m_iszTarget;
	bool		m_bOverrideKnockOut;

	COutputEvent	m_OnKnockOut;
	COutputEvent	m_OnFirstWarning;
	COutputEvent	m_OnSecondWarning;
	COutputEvent	m_OnLastWarning;
	COutputEvent	m_OnSupressingTarget;

	DECLARE_DATADESC();
};

#define	SF_POLICE_GOAL_KNOCKOUT_BEHIND		(1<<1)	// Knockout a target that's behind the plane that cuts perpendicularly through us
#define	SF_POLICE_GOAL_DO_NOT_LEAVE_POST	(1<<2)	// Cop will not come off his policing goal, even when angered

#endif // AI_GOAL_POLICE_H
