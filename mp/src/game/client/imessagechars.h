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
#if !defined( IMESSAGECHARS_H )
#define IMESSAGECHARS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{
class Panel;
typedef unsigned long HFont;
}

abstract_class IMessageChars
{
public:
	enum
	{
		MESSAGESTRINGID_NONE = -1,
		MESSAGESTRINGID_BASE = 0
	};
	
	virtual	void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;

	// messageID can be MESSAGESTRINGID_NONE or MESSAGESTRINGID_BASE plus some offset. You can refer to the message by
	// its ID later.
	virtual int			DrawString( vgui::HFont pCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID, ... ) = 0;
	virtual int			DrawString( vgui::HFont pCustomFont, int x, int y, const char *fmt, int messageID, ... ) = 0;
	
	virtual int			DrawStringForTime( float flTime, vgui::HFont pCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID,  ... ) = 0;
	virtual int			DrawStringForTime( float flTime, vgui::HFont pCustomFont, int x, int y, const char *fmt, int messageID, ... ) = 0;
	
	// Remove all messages with the specified ID (passed into DrawStringForTime).
	virtual void		RemoveStringsByID( int messageID ) = 0;

	virtual void		GetStringLength( vgui::HFont pCustomFont, int *width, int *height, PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;

	virtual void		Clear( void ) = 0;
};

extern IMessageChars *messagechars;

#endif // IMESSAGECHARS_H