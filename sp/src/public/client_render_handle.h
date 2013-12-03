//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENT_RENDER_HANDLE_H
#define CLIENT_RENDER_HANDLE_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Foward declarations
//-----------------------------------------------------------------------------
class IClientRenderable;


//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------
typedef unsigned short ClientRenderHandle_t;

enum
{
	INVALID_CLIENT_RENDER_HANDLE = (ClientRenderHandle_t)0xffff,
};


#endif // CLIENT_RENDER_HANDLE_H
