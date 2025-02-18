//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VPHYSICSUPDATEAI_H
#define VPHYSICSUPDATEAI_H
#ifdef _WIN32
#pragma once
#endif


// this is used to temporarily allow the vphysics shadow object to update the entity's position
// for entities that typically ignore those updates.
struct vphysicsupdateai_t
{
	float	startUpdateTime;
	float	stopUpdateTime;
	float	savedShadowControllerMaxSpeed;
};


#endif // VPHYSICSUPDATEAI_H
