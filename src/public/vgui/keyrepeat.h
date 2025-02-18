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
	CKeyRepeatHandler();

	void			Reset();
	void			KeyDown( vgui::KeyCode code );
	void			KeyUp( vgui::KeyCode code );
	vgui::KeyCode	KeyRepeated();
	void			SetKeyRepeatTime( vgui::KeyCode code, float flRepeat );

private:
	bool			m_bAliasDown[MAX_JOYSTICKS][FM_NUM_KEYREPEAT_ALIASES];
	float			m_flRepeatTimes[FM_NUM_KEYREPEAT_ALIASES];
	float			m_flNextKeyRepeat[MAX_JOYSTICKS];
	bool			m_bHaveKeyDown;
};


} // namespace vgui

#endif // KEYREPEAT_H
