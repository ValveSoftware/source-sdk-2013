//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYREPEAT_H
#define KEYREPEAT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

namespace vgui
{

enum KEYREPEAT_ALIASES
{
	KR_ALIAS_UP,
	KR_ALIAS_DOWN,
	KR_ALIAS_LEFT,
	KR_ALIAS_RIGHT,

	FM_NUM_KEYREPEAT_ALIASES,
};

class CKeyRepeatHandler
{
public:
	CKeyRepeatHandler()
	{
		Reset();
		for ( int i = 0; i < FM_NUM_KEYREPEAT_ALIASES; i++ )
		{
			m_flRepeatTimes[i] = 0.16;
		}
	}

	void		Reset( void ) { memset( m_bAliasDown, 0, sizeof(bool) * FM_NUM_KEYREPEAT_ALIASES ); m_bHaveKeyDown = false; }
	void		KeyDown( vgui::KeyCode code );
	void		KeyUp( vgui::KeyCode code );
	vgui::KeyCode	KeyRepeated( void );
	void		SetKeyRepeatTime( vgui::KeyCode code, float flRepeat );

private:
	int			GetIndexForCode( vgui::KeyCode code )
	{ 
		switch ( code )
		{
		case KEY_XBUTTON_DOWN: 
		case KEY_XSTICK1_DOWN:
			return KR_ALIAS_DOWN; break;
		case KEY_XBUTTON_UP: 
		case KEY_XSTICK1_UP:
			return KR_ALIAS_UP; break;
		case KEY_XBUTTON_LEFT: 
		case KEY_XSTICK1_LEFT:
			return KR_ALIAS_LEFT; break;
		case KEY_XBUTTON_RIGHT: 
		case KEY_XSTICK1_RIGHT:
			return KR_ALIAS_RIGHT; break;
		default:
			break;
		}
		return -1;
	}

private:
	bool			m_bAliasDown[FM_NUM_KEYREPEAT_ALIASES];
	float			m_flRepeatTimes[FM_NUM_KEYREPEAT_ALIASES];
	float			m_flNextKeyRepeat;
	bool			m_bHaveKeyDown;
};


} // namespace vgui

#endif // KEYREPEAT_H
