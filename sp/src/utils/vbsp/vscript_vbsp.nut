static char g_Script_vscript_vbsp[] = R"vscript(
//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
// 
//=============================================================================//

function UniqueString( string = "" )
{
	return ::DoUniqueString( string.tostring() );
}

function __ReplaceClosures( script, scope )
{
	if ( !scope )
	{
		scope = getroottable();
	}

	local tempParent = { getroottable = function() { return null; } };
	local temp = { runscript = script };
	temp.set_delegate(tempParent);

	temp.runscript()
	foreach( key,val in temp )
	{
		if ( typeof(val) == "function" && key != "runscript" )
		{
			printl( "   Replacing " + key );
			scope[key] <- val;
		}
	}
}

function IncludeScript( name, scope = null )
{
	if ( !scope )
	{
		scope = this;
	}
	return ::DoIncludeScript( name, scope );
}

// VBSP logs don't support ConColorMsg()
print <- Msg

function printdoc( text )
{
	return ::print(text + "\n");
}

)vscript";