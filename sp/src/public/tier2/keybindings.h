//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
	   
#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstring.h"
#include "inputsystem/ButtonCode.h"

class CUtlBuffer;


class CKeyBindings
{
public:
	void SetBinding( ButtonCode_t code, const char *pBinding );
	void SetBinding( const char *pButtonName, const char *pBinding );

	void Unbind( ButtonCode_t code );
	void Unbind( const char *pButtonName );
	void UnbindAll();

	int GetBindingCount() const;
	void WriteBindings( CUtlBuffer &buf );
	const char *ButtonNameForBinding( const char *pBinding );
	const char *GetBindingForButton( ButtonCode_t code );

private:
	CUtlString m_KeyInfo[ BUTTON_CODE_LAST ];
};


#endif // KEYBINDINGS_H
