//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_FuncForceField : public C_BaseEntity
{
	DECLARE_CLASS( C_FuncForceField, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	virtual int DrawModel( int flags ) OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncForceField, DT_FuncForceField, CFuncForceField )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_FuncForceField::DrawModel( int flags )
{
	// Don't draw for anyone during a team win
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return 1;

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: Enemy players collide with us, except during a team win
//-----------------------------------------------------------------------------
bool C_FuncForceField::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Force fields are off during a team win
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return false;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;

		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;
		}

		return true;
	}

	return false;
}