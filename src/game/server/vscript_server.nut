//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

function UniqueString( string = "" )
{
	return DoUniqueString( string.tostring() );
}

function EntFire( target, action, value = null, delay = 0.0, activator = null )
{
	if ( !value )
	{
		value = "";
	}
	
	local caller = null;
	if ( "self" in this )
	{
		caller = self;
		if ( !activator )
		{
			activator = self;
		}
	}
	
	DoEntFire( target.tostring(), action.tostring(), value.tostring(), delay, activator, caller ); 
}

function __ReplaceClosures( script, scope )
{
	if ( !scope )
	{
		scope = getroottable();
	}
	
	local tempParent = { getroottable = function() { return null; } };
	local temp = { runscript = script };
	temp.setdelegate( tempParent );
	
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

/*
UNDONE FOR PORTAL2 BRANCH:
We're not suing the auto-connecting of outputs, always calling ConnectOuput explicitly in our scripts.
The regexp object doesn't save/load properly and causes a crash when used to match after a save/load.
Instead of fixing this, we're disabling the feature. If this class of problem comes up more we might
revisit, otherwise we'll leave if off and broken.

__OutputsPattern <- regexp("^On.*Output$");

function ConnectOutputs( table )
{
	const nCharsToStrip = 6;
	foreach( key, val in table )
	{
		if ( typeof( val ) == "function" && __OutputsPattern.match( key ) )
		{
			//printl(key.slice( 0, nCharsToStrip ) );
			table.self.ConnectOutput( key.slice( 0, key.len() - nCharsToStrip ), key );
		}
	}
}
*/

function IncludeScript( name, scope = null )
{
	if ( scope == null )
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

//---------------------------------------------------------
function ClearGameEventCallbacks()
{
	::GameEventCallbacks <- {};
	::ScriptEventCallbacks <- {};
	::ScriptHookCallbacks <- {};
}

//---------------------------------------------------------
// Collect functions of the form OnGameEventXXX and store them in a table.
//---------------------------------------------------------
function __CollectEventCallbacks( scope, prefix, globalTableName, regFunc )
{
	if ( !(typeof( scope ) == "table" ) )
	{
		print( "__CollectEventCallbacks[" + prefix +"]: NOT TABLE! : " + typeof ( scope ) + "\n" );
		return;
	}

	if ( !(globalTableName in getroottable())  )
	{
		getroottable()[globalTableName] <- {};
	}
	local useTable = getroottable()[globalTableName] 
	foreach( key,value in scope )
	{
		if ( typeof( value ) == "function" )
		{
			if ( typeof( key ) == "string" && key.find( prefix, 0 ) == 0 )
			{
				local eventName = key.slice( prefix.len() ); 
				if ( eventName.len() > 0 )
				{
					// First time we've seen this event: Make an array for callbacks and
					// tell the game engine's listener we want to be notified.
					if ( !(eventName in useTable) )
					{
						useTable[eventName] <- [];
						if (regFunc)
							regFunc( eventName );
					}
					// Don't add duplicates. TODO: Perf on this...
					else if ( useTable[eventName].find( scope ) != null )
					{
						continue;
					}
					useTable[eventName].append( scope.weakref() );
				}
			}
		}
	}	
}

function __CollectGameEventCallbacks( scope )
{
	__CollectEventCallbacks( scope, "OnGameEvent_", "GameEventCallbacks", ::RegisterScriptGameEventListener )
	__CollectEventCallbacks( scope, "OnScriptEvent_", "ScriptEventCallbacks", null )
	__CollectEventCallbacks( scope, "OnScriptHook_", "ScriptHookCallbacks", ::RegisterScriptHookListener )
}

//---------------------------------------------------------
// Call all functions in the callback array for the given game event.
//---------------------------------------------------------
function __RunEventCallbacks( event, params, prefix, globalTableName, bWarnIfMissing )
{
	local useTable = getroottable()[globalTableName] 
	if ( !(event in useTable) )
	{
		if (bWarnIfMissing)
		    print( "__RunEventCallbacks[" + prefix + "]: Invalid 'event' name: " + event + ". No listeners registered for that event.\n" );
		return;
	}

	for ( local idx = useTable[event].len()-1; idx >= 0; --idx )
	{
		local funcName = prefix + event;
		if ( useTable[event][idx] == null )
		{
			//TODO: Not a great way to deal with cleanup...
			useTable[event].remove(idx);
		}
		else
		{
			//PERF TODO: This is a hash lookup for a function we know exists...
			// should be caching it off in CollectGameEventCallbacks.
			useTable[event][idx][funcName]( params );
		}
	}
}

function __RunGameEventCallbacks( event, params )
{
	__RunEventCallbacks( event, params, "OnGameEvent_", "GameEventCallbacks", true )
}

function __RunScriptHookCallbacks( event, params )
{
	__RunEventCallbacks( event, params, "OnScriptHook_", "ScriptHookCallbacks", false )
}

// kinda want to rename this "SendScriptEvent" - since we just send it to script
function FireScriptEvent( event, params )
{
	__RunEventCallbacks( event, params, "OnScriptEvent_", "ScriptEventCallbacks", false )
}


//-----------------------------------------------------------------------------
// Debug watches & trace
//-----------------------------------------------------------------------------

const ScriptDebugFirstLine 				= 6
const ScriptDebugTextLines 				= 20
const ScriptDebugTextTime 				= 10.0
const ScriptDebugWatchFistLine 			= 26
const NDEBUG_PERSIST_TILL_NEXT_SERVER 	= 0.01023
ScriptDebugDefaultWatchColor <- [ 0, 192, 0 ]

//-----------------------------------------------------------------------------

// Text is stored as an array of [ time, string, [ r, g, b ] ]
ScriptDebugText 		<- []
ScriptDebugTextIndent 	<- 0
ScriptDebugTextFilters	<- {}

ScriptDebugInDebugDraw <- false

ScriptDebugDrawWatchesEnabled <- true
ScriptDebugDrawTextEnabled <- true

// A watch is [ { key, function, color = [ r, g, b ], lastValue, lastChangeText } ]
ScriptDebugWatches 		<- []

ScriptDebugTraces 		<- {}
ScriptDebugTraceAllOn	<- false

//-----------------------------------------------------------------------------

function ScriptDebugDraw()
{
	ScriptDebugInDebugDraw = true

	if ( ScriptDebugDrawTextEnabled || ScriptDebugDrawWatchesEnabled )
	{
		ScriptDebugTextDraw( ScriptDebugFirstLine )
	}

	if ( ScriptDebugDrawWatchesEnabled )
	{
		ScriptDebugDrawWatches( ScriptDebugWatchFistLine )
	}

	ScriptDebugInDebugDraw = false
}

//-----------------------------------------------------------------------------

function ScriptDebugDrawWatches( line )
{
	local nWatches = ScriptDebugWatches.len()
	local curWatchKey
	local curWatchColor
	local curWatchValue
	local curWatchPath
	local curWatchString
	local ignored
	local bRedoExpand
	local changed
	
	for ( local i = 0; i < ScriptDebugWatches.len(); i++ )
	{
		curWatchKey = ScriptDebugWatches[i].key;
		curWatchColor = ScriptDebugWatches[i].color;
		
		if ( typeof( curWatchKey ) == "function" )
		{
			curWatchString = "" 
		}
		else
		{
			curWatchString = curWatchKey + ": "
		}
		
		try
		{
			local watchResult = ScriptDebugWatches[i].func.pcall(::getroottable())
			changed = false;
			if ( watchResult != null )
			{
				if ( watchResult != ScriptDebugWatches[i].lastValue )
				{
					if ( ScriptDebugWatches[i].lastValue != null )
					{
						ScriptDebugWatches[i].lastChangeText = " (was " + ScriptDebugWatches[i].lastValue + " @ " + Time() + ")"
						changed = true
					}
					ScriptDebugWatches[i].lastValue = watchResult
				}
				
				curWatchString = curWatchString + watchResult.tostring() + ScriptDebugWatches[i].lastChangeText
				if ( changed) 
				{
					ScriptDebugTextPrint( curWatchString, [ 0, 255, 0 ], true );
				}
			}
			else
			{
				curWatchString = curWatchString + "<<null>>"
			}
		}
		catch ( error )
		{
			curWatchString = curWatchString + "Watch failed - " + error.tostring()
		}
		
		DebugDrawScreenTextLine( 0.5, 0.0, line++, curWatchString, curWatchColor[0], curWatchColor[1], curWatchColor[2], 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}
	
	return line
}

//-----------------------------------------------------------------------------

function ScriptDebugAddWatch( watch )
{
	local watchType = typeof(watch)
	local i

	switch ( watchType )
	{
	case "function":
		{
			watch = { key = watch, func = watch, color = ScriptDebugDefaultWatchColor, lastValue = null, lastChangeText = "" }
			break;
		}
		
	case "string":
		{
			local closure
			try
			{
				closure = compilestring( "return " + watch, "" )
			}
			catch ( error )
			{
				ScriptDebugTextPrint( "Failed to add watch \"" + watch + "\": " + error.tostring() )
				return
			}
			watch = { key = watch, func = closure, color = ScriptDebugDefaultWatchColor, lastValue = null, lastChangeText = "" }
			break;
		}
	default:
		throw "Illegal type passed to ScriptDebugAddWatch: " + watchType
	}

	local function FindExisting( watch )
	{
		foreach( key, val in ScriptDebugWatches )
		{
			if ( val.key == watch.key )
			{
				return key
			}
		}
		return -1
	}
	
	local iExisting
	if ( ( iExisting = FindExisting( watch ) ) == -1 )
	{
		ScriptDebugWatches.append( watch )
	}
	else
	{
		// just update the color
		ScriptDebugWatches[iExisting].color = watch.color
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugAddWatches( watchArray )
{
	if ( typeof( watchArray ) == "array" )
	{
		for ( local i = 0; i < watchArray.len(); i++ )
		{
			ScriptDebugAddWatch( watchArray[i] );
		}
	}
	else
	{
		throw "ScriptDebugAddWatches() expected an array!"
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugRemoveWatch( watch )
{
	for ( local i = ScriptDebugWatches.len() - 1; i >= 0; --i )
	{
		if ( ScriptDebugWatches[i].key == watch )
		{
			ScriptDebugWatches.remove( i );
		}
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugRemoveWatches( watchArray )
{
	if ( typeof( watchArray ) == "array" )
	{
		for ( local i = 0; i < watchArray.len(); i++ )
		{
			ScriptDebugRemoveWatch( watchArray[i] );
		}
	}
	else
	{
		throw "ScriptDebugAddWatches() expected an array!"
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugAddWatchPattern( name )
{
	if ( name == "" )
	{
		Msg( "Cannot find an empty string" )
		return;
	}
	
	local function OnKey( keyPath, key, value )
	{
		if ( keyPath.find( "Documentation." ) != 0 )
		{
			ScriptDebugAddWatch( keyPath );
		}
	}
	
	ScriptDebugIterateKeys( name, OnKey );
}

//-----------------------------------------------------------------------------

function ScriptDebugRemoveWatchPattern( name )
{
	if ( name == "" )
	{
		Msg( "Cannot find an empty string" )
		return;
	}
	
	local function OnKey( keyPath, key, value )
	{
		ScriptDebugRemoveWatch( keyPath ); 
	}
	
	ScriptDebugIterateKeys( name, OnKey );
}

//-----------------------------------------------------------------------------

function ScriptDebugClearWatches()
{
	ScriptDebugWatches.clear()
}

//-----------------------------------------------------------------------------

function ScriptDebugTraceAll( bValue = true )
{
	ScriptDebugTraceAllOn = bValue
}

function ScriptDebugAddTrace( traceTarget )
{
	local type = typeof( traceTarget )
	if (  type == "string" || type == "table" || type == "instance" )
	{
		ScriptDebugTraces[traceTarget] <- true
	}
}

function ScriptDebugRemoveTrace( traceTarget )
{
	if ( traceTarget in ScriptDebugTraces )
	{
		delete ScriptDebugTraces[traceTarget]
	}
}

function ScriptDebugClearTraces()
{
	ScriptDebugTraceAllOn = false
	ScriptDebugTraces.clear()
}

function ScriptDebugTextTrace( text, color = [ 255, 255, 255 ] )
{
	local bPrint = ScriptDebugTraceAllOn

	if ( !bPrint && ScriptDebugTraces.len() )
	{
		local stackinfo = getstackinfos( 2 )
		if ( stackinfo != null )
		{
			if ( ( stackinfo.func in ScriptDebugTraces ) ||
				 ( stackinfo.src in ScriptDebugTraces ) ||
				 ( stackinfo.locals["this"] in ScriptDebugTraces ) )
			{
				bPrint = true
			}
		}
	}
	
	if ( bPrint )
	{
		ScriptDebugTextPrint( text, color )
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugTextPrint( text, color = [ 255, 255, 255 ], isWatch = false )
{
	foreach( key, val in ScriptDebugTextFilters )
	{
		if ( text.find( key ) != null )
		{
			return;
		}
	}

	local timeString = format( "(%0.2f) ", Time() ) 

	if ( ScriptDebugDrawTextEnabled || ( isWatch && ScriptDebugDrawWatchesEnabled ) )
	{
		local indentString = "";
		local i = ScriptDebugTextIndent
		while ( i-- )
		{
			indentString = indentString + "   "
		}
		
		// Screen overlay
		local debugString = timeString + indentString + text.tostring()
		ScriptDebugText.append( [ Time(), debugString.tostring(), color ] )
		if ( ScriptDebugText.len() > ScriptDebugTextLines )
		{
			ScriptDebugText.remove( 0 )
		}
	}
	
	// Console
	printl( text + " " + timeString );
}

//-----------------------------------------------------------------------------

function ScriptDebugTextDraw( line )
{
	local i
	local alpha
	local curtime = Time()
	local age
	for ( i = 0; i < ScriptDebugText.len(); i++ )
	{
		age = curtime - ScriptDebugText[i][0]
		if ( age < -1.0 )
		{
			// Started a new server
			ScriptDebugText.clear()
			break;
		}

		if ( age < ScriptDebugTextTime )
		{
			if ( age >= ScriptDebugTextTime - 1.0 )
			{
				alpha = ( 255.0 * ( ScriptDebugTextTime - age ) ).tointeger()
				Assert( alpha >= 0 )
			}
			else
			{
				alpha = 255
			}
			
			DebugDrawScreenTextLine( 0.5, 0.0, line++, ScriptDebugText[i][1], ScriptDebugText[i][2][0], ScriptDebugText[i][2][1], ScriptDebugText[i][2][2], alpha, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}
	
	return line + ScriptDebugTextLines - i;
}

//-----------------------------------------------------------------------------

function ScriptDebugAddTextFilter( filter )
{
	ScriptDebugTextFilters[filter] <- true
}

//-----------------------------------------------------------------------------

function ScriptDebugRemoveTextFilter( filter )
{
	if ( filter in ScriptDebugTextFilters )
	{
		delete ScriptDebugTextFilters[filter]
	}
}

//-----------------------------------------------------------------------------

function ScriptDebugHook( type, file, line, funcname )
{
	if ( ScriptDebugInDebugDraw )
	{
		return
	}

	if ( ( type == 'c' || type == 'r' ) && file != "unnamed" && file != "" && file != "game_debug.nut" && funcname != null )
	{
		local functionString = funcname + "() [ " + file + "(" + line + ") ]"

		foreach( key, val in ScriptDebugTextFilters )
		{
			if ( file.find( key ) != null || functionString.find( key ) != null )
			{
				return;
			}
		}
		
		if ( type == 'c' )
		{
			local indentString = "";
			local i = ScriptDebugTextIndent
			while ( i-- )
			{
				indentString = indentString + "   "
			}
			
			// Screen overlay
			local timeString = format( "(%0.2f) ", Time() ) 
			local debugString = timeString + indentString + functionString
			ScriptDebugTextPrint( functionString );
			ScriptDebugTextIndent++;
			
			// Console
			printl( "{" ); 
			print_indent++;
		}
		else
		{
			ScriptDebugTextIndent--;
			print_indent--;
			printl( "}" );
			
			if ( ScriptDebugTextIndent == 0 )
			{
				ScriptDebugExpandWatches()
			}
		}
	}
}

//-----------------------------------------------------------------------------

function __VScriptServerDebugHook( type, file, line, funcname )
{
	ScriptDebugHook( type, file, line, funcname ) // route to support debug script reloading during development 
}

function BeginScriptDebug()
{
	setdebughook( __VScriptServerDebugHook );
}

function EndScriptDebug()
{
	setdebughook( null );
}

//-----------------------------------------------------------------------------
