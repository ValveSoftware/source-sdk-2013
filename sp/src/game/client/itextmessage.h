//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( ITEXTMESSAGE_H )
#define ITEXTMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/VGUI.h"
#include "fontabc.h"

abstract_class ITextMessage 
{
public:
	virtual void		SetPosition( int x, int y ) = 0;
	virtual void		AddChar( int r, int g, int b, int a, wchar_t ch ) = 0;

	virtual void		GetLength( int *wide, int *tall, const char *string ) = 0;
	virtual int			GetFontInfo( FONTABC *pABCs, vgui::HFont hFont ) = 0;

	virtual void		SetFont( vgui::HFont hCustomFont ) = 0;
	virtual void		SetDefaultFont( void ) = 0;
};

extern ITextMessage *textmessage;

#endif // ITEXTMESSAGE_H