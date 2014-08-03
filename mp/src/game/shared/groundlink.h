//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GROUNDLINK_H
#define GROUNDLINK_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Used for tracking many to one ground entity chains ( many ents can share a single ground entity )
//-----------------------------------------------------------------------------
struct groundlink_t
{
	EHANDLE					entity;
	groundlink_t			*nextLink;
	groundlink_t			*prevLink;
};

#endif // GROUNDLINK_H
