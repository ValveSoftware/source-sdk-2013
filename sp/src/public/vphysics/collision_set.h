//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// A set of collision rules
// NOTE: Defaults to all indices disabled
class IPhysicsCollisionSet
{
public:
	~IPhysicsCollisionSet() {}

	virtual void EnableCollisions( int index0, int index1 ) = 0;
	virtual void DisableCollisions( int index0, int index1 ) = 0;

	virtual bool ShouldCollide( int index0, int index1 ) = 0;
};