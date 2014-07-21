//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

// Navigation ladders
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_LADDER_H_
#define _NAV_LADDER_H_

#include "nav.h"

class CNavArea;

//--------------------------------------------------------------------------------------------------------------
/**
 * The NavLadder represents ladders in the Navigation Mesh, and their connections to adjacent NavAreas
 * @todo Deal with ladders that allow jumping off to areas in the middle
 */
class CNavLadder
{
public:
	CNavLadder( void )
	{
		m_topForwardArea = NULL;
		m_topRightArea = NULL;
		m_topLeftArea = NULL;
		m_topBehindArea = NULL;
		m_bottomArea = NULL;

		// set an ID for interactive editing - loads will overwrite this
		m_id = m_nextID++;
	}

	~CNavLadder();

	void OnRoundRestart( void );			///< invoked when a game round restarts

	void Save( CUtlBuffer &fileBuffer, unsigned int version ) const;
	void Load( CUtlBuffer &fileBuffer, unsigned int version );

	unsigned int GetID( void ) const	{ return m_id; }		///< return this ladder's unique ID
	static void CompressIDs( void );							///<re-orders ladder ID's so they are continuous

	enum LadderDirectionType
	{
		LADDER_UP = 0,
		LADDER_DOWN,

		NUM_LADDER_DIRECTIONS
	};

	Vector m_top;									///< world coords of the top of the ladder
	Vector m_bottom;								///< world coords of the top of the ladder
	float m_length;									///< the length of the ladder
	float m_width;

	Vector GetPosAtHeight( float height ) const;	///< Compute x,y coordinate of the ladder at a given height

	CNavArea *m_topForwardArea;						///< the area at the top of the ladder
	CNavArea *m_topLeftArea;
	CNavArea *m_topRightArea;
	CNavArea *m_topBehindArea;						///< area at top of ladder "behind" it - only useful for descending
	CNavArea *m_bottomArea;							///< the area at the bottom of the ladder

	bool IsConnected( const CNavArea *area, LadderDirectionType dir ) const;	///< returns true if given area is connected in given direction

	void ConnectGeneratedLadder( float maxHeightAboveTopArea );		///< Connect a generated ladder to nav areas at the end of nav generation

	void ConnectTo( CNavArea *area );				///< connect this ladder to given area
	void Disconnect( CNavArea *area );				///< disconnect this ladder from given area

	void OnSplit( CNavArea *original, CNavArea *alpha, CNavArea *beta );	///< when original is split into alpha and beta, update our connections
	void OnDestroyNotify( CNavArea *dead );			///< invoked when given area is going away

	void DrawLadder( void ) const;					///< Draws ladder and connections
	void DrawConnectedAreas( void );				///< Draws connected areas

	void UpdateDangling( void );					///< Checks if the ladder is dangling (bots cannot go up)

	bool IsInUse( const CBasePlayer *ignore = NULL ) const;	///< return true if someone is on this ladder (other than 'ignore')

	void SetDir( NavDirType dir );
	NavDirType GetDir( void ) const;
	const Vector &GetNormal( void ) const;

	void Shift( const Vector &shift );							///< shift the nav ladder

	bool IsUsableByTeam( int teamNumber ) const;
	CBaseEntity *GetLadderEntity( void ) const;

private:
	void FindLadderEntity( void );

	EHANDLE m_ladderEntity;

	NavDirType m_dir;								///< which way the ladder faces (ie: surface normal of climbable side)
	Vector m_normal;								///< surface normal of the ladder surface (or Vector-ized m_dir, if the traceline fails)

	enum LadderConnectionType						///< Ladder connection directions, to facilitate iterating over connections
	{
		LADDER_TOP_FORWARD = 0,
		LADDER_TOP_LEFT,
		LADDER_TOP_RIGHT,
		LADDER_TOP_BEHIND,
		LADDER_BOTTOM,

		NUM_LADDER_CONNECTIONS
	};

	CNavArea ** GetConnection( LadderConnectionType dir );

	static unsigned int m_nextID;					///< used to allocate unique IDs
	unsigned int m_id;								///< unique area ID
};
typedef CUtlVector< CNavLadder * > NavLadderVector;


//--------------------------------------------------------------------------------------------------------------
inline bool CNavLadder::IsUsableByTeam( int teamNumber ) const
{
	if ( m_ladderEntity.Get() == NULL )
		return true;

	int ladderTeamNumber = m_ladderEntity->GetTeamNumber();
	return ( teamNumber == ladderTeamNumber || ladderTeamNumber == TEAM_UNASSIGNED );
}


//--------------------------------------------------------------------------------------------------------------
inline CBaseEntity *CNavLadder::GetLadderEntity( void ) const
{
	return m_ladderEntity.Get();
}


//--------------------------------------------------------------------------------------------------------------
inline NavDirType CNavLadder::GetDir( void ) const
{
	return m_dir;
}


//--------------------------------------------------------------------------------------------------------------
inline const Vector &CNavLadder::GetNormal( void ) const
{
	return m_normal;
}


#endif // _NAV_LADDER_H_
