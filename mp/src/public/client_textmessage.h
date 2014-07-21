//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENT_TEXTMESSAGE_H
#define CLIENT_TEXTMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

struct client_textmessage_t
{
	int		effect;
	byte	r1, g1, b1, a1;		// 2 colors for effects
	byte	r2, g2, b2, a2;
	float	x;
	float	y;
	float	fadein;
	float	fadeout;
	float	holdtime;
	float	fxtime;
	const char *pVGuiSchemeFontName; // If null, use default font for messages
	const char *pName;
	const char *pMessage;
	bool    bRoundedRectBackdropBox;
	float	flBoxSize; // as a function of font height
	byte	boxcolor[4];
	char const *pClearMessage; // message to clear
};

#endif // CLIENT_TEXTMESSAGE_H
