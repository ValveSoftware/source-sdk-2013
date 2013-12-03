//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Special handling for hl2 usable ladders
//
//=============================================================================//

#include "gamemovement.h"
#include "func_ladder.h"

#if defined( CLIENT_DLL )

#include "c_basehlplayer.h"
#define CHL2_Player C_BaseHLPlayer
#else

#include "hl2_player.h"

#endif

struct LadderMove_t;
class CInfoLadderDismount;

struct NearbyDismount_t
{
	CInfoLadderDismount		*dismount;
	float					distSqr;
};

//-----------------------------------------------------------------------------
// Purpose: HL2 specific movement code
//-----------------------------------------------------------------------------
class CHL2GameMovement : public CGameMovement
{
	typedef CGameMovement BaseClass;
public:

	CHL2GameMovement();

// Overrides
	virtual void FullLadderMove();
	virtual bool LadderMove( void );
	virtual bool OnLadder( trace_t &trace );
	virtual int GetCheckInterval( IntervalType_t type );
	virtual void	SetGroundEntity( trace_t *pm );
	virtual bool CanAccelerate( void );

private:

	// See if we are pressing use near a ladder "mount" point and if so, latch us onto the ladder
	bool		CheckLadderAutoMount( CFuncLadder *ladder, const Vector& bestOrigin );

	bool		CheckLadderAutoMountCone( CFuncLadder *ladder, const Vector& bestOrigin, float maxAngleDelta, float maxDistToLadder );
	bool		CheckLadderAutoMountEndPoint(CFuncLadder *ladder, const Vector& bestOrigin );


	bool		LookingAtLadder( CFuncLadder *ladder );

	// Are we forcing the user's position to a new spot
	bool		IsForceMoveActive();
	// Start forcing player position
	void		StartForcedMove( bool mounting, float transit_speed, const Vector& goalpos, CFuncLadder *ladder );
	// Returns false when finished
	bool		ContinueForcedMove();

	// Given a list of nearby ladders, find the best ladder and the "mount" origin
	void		Findladder( float maxdist, CFuncLadder **ppLadder, Vector& ladderOrigin, const CFuncLadder *skipLadder );

	// Debounce the +USE key
	void		SwallowUseKey();

	// Returns true if the player will auto-exit the ladder via a dismount node
	bool		ExitLadderViaDismountNode( CFuncLadder *ladder, bool strict, bool useAlternate = false );
	void		GetSortedDismountNodeList( const Vector &org, float radius, CFuncLadder *ladder, CUtlRBTree< NearbyDismount_t, int >& list );

	LadderMove_t *GetLadderMove();
	CHL2_Player	*GetHL2Player();

	void		SetLadder( CFuncLadder *ladder );
	CFuncLadder *GetLadder();
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHL2_Player	*CHL2GameMovement::GetHL2Player()
{
	return static_cast< CHL2_Player * >( player );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline LadderMove*
//-----------------------------------------------------------------------------
inline LadderMove_t *CHL2GameMovement::GetLadderMove()
{
	CHL2_Player *p = GetHL2Player();
	if ( !p )
	{
		return NULL;
	}
	return p->GetLadderMove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ladder - 
//-----------------------------------------------------------------------------
inline void CHL2GameMovement::SetLadder( CFuncLadder *ladder )
{
	CFuncLadder* oldLadder = GetLadder();

	if ( !ladder && oldLadder )
	{
		oldLadder->PlayerGotOff( GetHL2Player() );
	}


	GetHL2Player()->m_HL2Local.m_hLadder.Set( ladder );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CFuncLadder
//-----------------------------------------------------------------------------
inline CFuncLadder *CHL2GameMovement::GetLadder()
{
	return static_cast<CFuncLadder*>( static_cast<CBaseEntity *>( GetHL2Player()->m_HL2Local.m_hLadder.Get() ) );
}
