static char g_Script_vscript_client[] = R"vscript(
//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

local DoUniqueString = DoUniqueString
local DoDispatchParticleEffect = DoDispatchParticleEffect

function UniqueString( string = "" )
{
	return DoUniqueString( "" + string );
}

function IncludeScript( name, scope = null )
{
	if ( !scope )
	{
		scope = this;
	}
	return ::DoIncludeScript( name, scope );
}

function DispatchParticleEffect( particleName, origin, angles, entity = null )
{
	DoDispatchParticleEffect( particleName, origin, angles, entity );
}

)vscript";