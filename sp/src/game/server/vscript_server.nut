static char g_Script_vscript_server[] = R"vscript(
//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

local DoEntFire = DoEntFire
local DoEntFireByInstanceHandle = DoEntFireByInstanceHandle
local DoDispatchParticleEffect = DoDispatchParticleEffect
local DoUniqueString = DoUniqueString

function UniqueString( string = "" )
{
	return DoUniqueString( "" + string );
}

function EntFire( target, action, value = null, delay = 0.0, activator = null, caller = null )
{
	if ( !value )
	{
		value = "";
	}

	if ( "self" in this )
	{
		if ( !caller )
		{
			caller = self;
		}

		if ( !activator )
		{
			activator = self;
		}
	}

	return DoEntFire( "" + target, "" + action, "" + value, delay, activator, caller );
}

function EntFireByHandle( target, action, value = null, delay = 0.0, activator = null, caller = null )
{
	if ( !value )
	{
		value = "";
	}

	if ( "self" in this )
	{
		if ( !caller )
		{
			caller = self;
		}

		if ( !activator )
		{
			activator = self;
		}
	}

	return DoEntFireByInstanceHandle( target, "" + action, "" + value, delay, activator, caller );
}

function DispatchParticleEffect( particleName, origin, angles, entity = null )
{
	return DoDispatchParticleEffect( particleName, origin, angles, entity );
}

function ImpulseScale( flTargetMass, flDesiredSpeed )
{
	return flTargetMass * flDesiredSpeed;
}
__Documentation.RegisterHelp( "ImpulseScale", "float ImpulseScale(float, float)", "Returns an impulse scale required to push an object." );

local PrecacheModel = PrecacheModel
function PrecacheModel( a, b = true )
{
    return PrecacheModel( a, b )
}

local PrecacheOther = PrecacheOther
function PrecacheOther( a, b = "" )
{
    PrecacheOther( a, b )
}

function __ReplaceClosures( script, scope )
{
	if ( !scope )
	{
		scope = getroottable();
	}

	local tempParent = { getroottable = function() { return null; } };
	local temp = { runscript = script };
	temp.setdelegate(tempParent);

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

local __OutputsPattern = regexp("^On.*Output$");

function ConnectOutputs( table )
{
	local nCharsToStrip = 6;
	foreach( key, val in table )
	{
		if ( typeof( val ) == "function" && __OutputsPattern.match( key ) )
		{
			//printl(key.slice( 0, nCharsToStrip ) );
			table.self.ConnectOutput( key.slice( 0, key.len() - nCharsToStrip ), key );
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

//---------------------------------------------------------
// Text dump this scope's contents to the console.
//---------------------------------------------------------
function __DumpScope( depth, table )
{
	local indent=function( count )
	{
		local i;
		for( i = 0 ; i < count ; i++ )
		{
			print("   ");
		}
	}
	
    foreach(key, value in table)
    {
		indent(depth);
		print( key );
        switch (type(value))
        {
            case "table":
				print("(TABLE)\n");
				indent(depth);
                print("{\n");
                __DumpScope( depth + 1, value);
				indent(depth);
                print("}");
                break;
            case "array":
				print("(ARRAY)\n");
				indent(depth);
                print("[\n")
                __DumpScope( depth + 1, value);
				indent(depth);
                print("]");
                break;
            case "string":
                print(" = \"");
                print(value);
                print("\"");
                break;
            default:
                print(" = ");
                print(value);
                break;
        }
        print("\n");  
	}
}

)vscript";