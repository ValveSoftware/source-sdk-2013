//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface that entities can use to redirect scoring to other entities.
//			i.e. A rocket redirects scoring to the player that fired it.
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISCORER_H
#define ISCORER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Interface that entities can use to redirect scoring to other entities.
//			i.e. A rocket redirects scoring to the player that fired it.
//-----------------------------------------------------------------------------
abstract_class IScorer
{
public:
	// Return the entity that should receive the score
	virtual CBasePlayer *GetScorer( void ) = 0;
	// Return the entity that should get assistance credit
	virtual CBasePlayer *GetAssistant( void ) = 0;
};


#endif // ISCORER_H
