#ifndef V_UTIL_H
#define V_UTIL_H

#include "cbase.h"

#include <vgui/ISurface.h>
#include <vgui_controls/controls.h>

inline void SetupVGUITex( const char *szPath, int &var )
{
	var = vgui::surface()->DrawGetTextureId( szPath );

	if ( vgui::surface()->IsTextureIDValid( var ) && var >= 0 )
		return;

	var = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( var, szPath, false, false );
}

inline void ScaleByFrametime( float &val )
{
	//val = 1.0f - (1.0f - val) * gpGlobals->frametime;
	//float delta = 1.0f - val;
	//val = val * log10f( val ) / log10f( 1.0f - delta * gpGlobals->frametime );
	val = pow( val, gpGlobals->frametime );
}

#endif