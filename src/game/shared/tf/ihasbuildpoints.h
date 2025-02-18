//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IHASBUILDPOINTS_H
#define IHASBUILDPOINTS_H
#ifdef _WIN32
#pragma once
#endif

class CBaseObject;

// Derive from this interface if your entity can have objects placed on build points on it
class IHasBuildPoints
{
public:
	// Tell me how many build points you have
	virtual int			GetNumBuildPoints( void ) const = 0;

	// Give me the origin & angles of the specified build point
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles ) = 0;

	// If the build point wants to parent built objects to an attachment point on the entity,
	// it'll return a value >= 1 here specifying which attachment to sit on.
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const = 0;

	// Can I build the specified object on the specified build point?
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType ) = 0;

	// I've finished building the specified object on the specified build point
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject ) = 0;

	// Get the number of objects build on this entity
	virtual int			GetNumObjectsOnMe( void ) = 0;

	// Get the first object of type, return NULL if no such type available
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType ) = 0;

	// Remove all objects built on me
	virtual void		RemoveAllObjects( void ) = 0;

	// Return the maximum distance that this entity's build points can be snapped to
	virtual float		GetMaxSnapDistance( int iPoint ) = 0;

	// Return true if it's possible that build points on this entity may move in local space (i.e. due to animation)
	virtual bool		ShouldCheckForMovement( void ) = 0;

	// I've finished building the specified object on the specified build point
	virtual int			FindObjectOnBuildPoint( CBaseObject *pObject ) = 0;
};

#endif // IHASBUILDPOINTS_H
