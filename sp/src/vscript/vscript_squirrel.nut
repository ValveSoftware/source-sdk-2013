static char g_Script_vscript_squirrel[] = R"vscript(
//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose:
//
//=============================================================================//

Warning <- error;

function clamp( val, min, max )
{
	if ( max < min )
		return max;
	if ( val < min )
		return min;
	if ( val > max )
		return max;
	return val;
}

function max( a, b )
{
	if ( a > b )
		return a;
	return b;
}

function min( a, b )
{
	if ( a < b )
		return a;
	return b;
}

function RemapVal( val, A, B, C, D )
{
	if ( A == B )
	{
		if ( val >= B )
			return D;
		return C;
	};
	return C + (D - C) * (val - A) / (B - A);
}

function RemapValClamped( val, A, B, C, D )
{
	if ( A == B )
	{
		if ( val >= B )
			return D;
		return C;
	};

	local cVal = (val - A) / (B - A);

	if ( cVal <= 0.0 )
		return C;

	if ( cVal >= 1.0 )
		return D;

	return C + (D - C) * cVal;
}

function Approach( target, value, speed )
{
	local delta = target - value;

	if ( delta > speed )
		return value + speed;
	if ( -speed > delta )
		return value - speed;
	return target;
}

function AngleDistance( next, cur )
{
	local delta = next - cur

	if ( delta > 180.0 )
		return delta - 360.0;
	if ( -180.0 > delta )
		return delta + 360.0;
	return delta;
}

function FLerp( f1, f2, i1, i2, x )
{
	return f1+(f2-f1)*(x-i1)/(i2-i1);
}

function Lerp( f, A, B )
{
	return A + ( B - A ) * f
}

function SimpleSpline( f )
{
	local ff = f * f;
	return 3.0 * ff - 2.0 * ff * f;
}

function printl( text )
{
	return print(text + "\n");
}

class CSimpleCallChainer
{
	constructor(prefixString, scopeForThis, exactMatch)
	{
		prefix = prefixString;
		scope = scopeForThis;
		chain = [];
		scope["Dispatch" + prefixString] <- Call.bindenv(this);
	}

	function PostScriptExecute()
	{
		if ( prefix in scope )
		{
			local func = scope[prefix];
			if ( typeof func == "function" )
			{
				chain.push(func);
			}
		}
	}

	function Call()
	{
		foreach (func in chain)
		{
			func.pcall(scope);
		}
	}

	prefix = null;
	scope = null;
	chain = null;
}

//---------------------------------------------------------
// Hook handler
//---------------------------------------------------------
local s_List = {}

Hooks <-
{
	// table, string, closure, string
	function Add( scope, event, callback, context )
	{
		switch ( typeof scope )
		{
			case "table":
			case "instance":
			case "class":
				break;
			default:
				throw "invalid scope param";
		}

		if ( typeof event != "string" )
			throw "invalid event param";

		if ( typeof callback != "function" )
			throw "invalid callback param";

		if ( typeof context != "string" )
			throw "invalid context param";

		if ( !(event in s_List) )
			s_List[event] <- {};

		local t = s_List[event];

		if ( !(scope in t) )
			t[scope] <- {};

		t[scope][context] <- callback;

		return __UpdateHooks();
	}

	function Remove( event, context )
	{
		local rem;

		if ( event )
		{
			if ( event in s_List )
			{
				foreach ( scope, ctx in s_List[event] )
				{
					if ( context in ctx )
					{
						delete ctx[context];
					}

					if ( !ctx.len() )
					{
						if ( !rem )
							rem = [];
						rem.append( event );
						rem.append( scope );
					}
				}
			}
		}
		else
		{
			foreach ( ev, t in s_List )
			{
				foreach ( scope, ctx in t )
				{
					if ( context in ctx )
					{
						delete ctx[context];
					}

					if ( !ctx.len() )
					{
						if ( !rem )
							rem = [];
						rem.append( ev );
						rem.append( scope );
					}
				}
			}
		}

		if ( rem )
		{
			local c = rem.len() - 1;
			for ( local i = 0; i < c; i += 2 )
			{
				local ev = rem[i];
				local scope = rem[i+1];

				if ( !s_List[ev][scope].len() )
					delete s_List[ev][scope];

				if ( !s_List[ev].len() )
					delete s_List[ev];
			}
		}

		return __UpdateHooks();
	}

	function Call( event, scope, ... )
	{
		local firstReturn;

		if ( event in s_List )
		{
			vargv.insert( 0, scope );

			local t = s_List[event];
			if ( scope in t )
			{
				foreach ( fn in t[scope] )
				{
					//printf( "(%.4f) Calling hook %s:%s\n", Time(), event, context );

					local r = fn.acall( vargv );
					if ( firstReturn == null )
						firstReturn = r;
				}
			}
			else if ( !scope ) // global hook
			{
				foreach ( sc, ctx in t )
				{
					vargv[0] = sc;

					foreach ( context, fn in ctx )
					{
						//printf( "(%.4f) Calling hook (g) %s:%s\n", Time(), event, context );

						local r = fn.acall( vargv );
						if ( firstReturn == null )
							firstReturn = r;
					}
				}
			}
		}

		return firstReturn;
	}

	function __UpdateHooks()
	{
		return __UpdateScriptHooks( s_List );
	}
}

//---------------------------------------------------------
// Documentation
//---------------------------------------------------------
__Documentation <- {}

local developer = (delete developer)()

if (developer)
{
	local DocumentedFuncs   = {}
	local DocumentedClasses = {}
	local DocumentedEnums   = {}
	local DocumentedConsts  = {}
	local DocumentedHooks   = {}
	local DocumentedMembers = {}

	local function AddAliasedToTable(name, signature, description, table)
	{
		// This is an alias function, could use split() if we could guarantee
		// that ':' would not occur elsewhere in the description and Squirrel had
		// a convience join() function -- It has split()
		local colon = description.find(":");
		if (colon == null)
			colon = description.len();
		local alias = description.slice(1, colon);
		description = description.slice(colon + 1);
		name = alias;
		signature = null;

		table[name] <- [signature, description];
	}

	function __Documentation::RegisterHelp(name, signature, description)
	{
		if (description.len() && description[0] == '#')
		{
			AddAliasedToTable(name, signature, description, DocumentedFuncs)
		}
		else
		{
			DocumentedFuncs[name] <- [signature, description];
		}
	}

	function __Documentation::RegisterClassHelp(name, baseclass, description)
	{
		DocumentedClasses[name] <- [baseclass, description];
	}

	function __Documentation::RegisterEnumHelp(name, num_elements, description)
	{
		DocumentedEnums[name] <- [num_elements, description];
	}

	function __Documentation::RegisterConstHelp(name, signature, description)
	{
		if (description.len() && description[0] == '#')
		{
			AddAliasedToTable(name, signature, description, DocumentedConsts)
		}
		else
		{
			DocumentedConsts[name] <- [signature, description];
		}
	}

	function __Documentation::RegisterHookHelp(name, signature, description)
	{
		DocumentedHooks[name] <- [signature, description];
	}

	function __Documentation::RegisterMemberHelp(name, signature, description)
	{
		DocumentedMembers[name] <- [signature, description];
	}

	local function printdoc( text )
	{
		return ::printc(200,224,255,text);
	}

	local function printdocl( text )
	{
		return printdoc(text + "\n");
	}

	local function PrintClass(name, doc)
	{
		local text = "=====================================\n";
		text += ("Class:       " + name + "\n");
		text += ("Base:        " + doc[0] + "\n");
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		text += "=====================================\n\n";

		printdoc(text);
	}

	local function PrintFunc(name, doc)
	{
		local text = "Function:    " + name + "\n"

		if (doc[0] == null)
		{
			// Is an aliased function
			text += ("Signature:   function " + name + "(");
			foreach(k,v in this[name].getinfos().parameters)
			{
				if (k == 0 && v == "this") continue;
				if (k > 1) text += (", ");
				text += (v);
			}
			text += (")\n");
		}
		else
		{
			text += ("Signature:   " + doc[0] + "\n");
		}
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		printdocl(text);
	}

	local function PrintMember(name, doc)
	{
		local text = ("Member:      " + name + "\n");
		text += ("Signature:   " + doc[0] + "\n");
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		printdocl(text);
	}

	local function PrintEnum(name, doc)
	{
		local text = "=====================================\n";
		text += ("Enum:        " + name + "\n");
		text += ("Elements:    " + doc[0] + "\n");
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		text += "=====================================\n\n";

		printdoc(text);
	}

	local function PrintConst(name, doc)
	{
		local text = ("Constant:    " + name + "\n");
		if (doc[0] == null)
		{
			text += ("Value:       null\n");
		}
		else
		{
			text += ("Value:       " + doc[0] + "\n");
		}
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		printdocl(text);
	}

	local function PrintHook(name, doc)
	{
		local text = ("Hook:        " + name + "\n");
		if (doc[0] == null)
		{
			// Is an aliased function
			text += ("Signature:   function " + name + "(");
			foreach(k,v in this[name].getinfos().parameters)
			{
				if (k == 0 && v == "this") continue;
				if (k > 1) text += (", ");
				text += (v);
			}
			text += (")\n");
		}
		else
		{
			text += ("Signature:   " + doc[0] + "\n");
		}
		if (doc[1].len())
			text += ("Description: " + doc[1] + "\n");
		printdocl(text);
	}

	local function PrintMatches( pattern, docs, printfunc )
	{
		local matches = [];
		local always = pattern == "*";

		foreach( name, doc in docs )
		{
			if (always || name.tolower().find(pattern) != null || (doc[1].len() && doc[1].tolower().find(pattern) != null))
			{
				matches.append( name );
			}
		}

		if ( !matches.len() )
			return 0;

		matches.sort();

		foreach( name in matches )
			printfunc( name, docs[name] );

		return 1;
	}

	function __Documentation::PrintHelp(pattern = "*")
	{
		local patternLower = pattern.tolower();

		// Have a specific order
		if (!(
			PrintMatches( patternLower, DocumentedEnums, PrintEnum )		|
			PrintMatches( patternLower, DocumentedConsts, PrintConst )		|
			PrintMatches( patternLower, DocumentedClasses, PrintClass )		|
			PrintMatches( patternLower, DocumentedFuncs, PrintFunc )		|
			PrintMatches( patternLower, DocumentedMembers, PrintMember )	|
			PrintMatches( patternLower, DocumentedHooks, PrintHook )
		   ))
		{
			printdocl("Pattern " + pattern + " not found");
		}
	}
}
else
{
	__Documentation.RegisterHelp <-
	__Documentation.RegisterClassHelp <-
	__Documentation.RegisterEnumHelp <-
	__Documentation.RegisterConstHelp <-
	__Documentation.RegisterHookHelp <-
	__Documentation.RegisterMemberHelp <- dummy

	function __Documentation::PrintHelp( pattern = null )
	{
		printcl(200, 224, 255, "Documentation is not enabled. To enable documentation, restart the server with the 'developer' cvar set to 1 or higher.");
	}
}

if (developer)
{
	__Documentation.RegisterClassHelp( "Vector", "", "Basic 3-float Vector class." );
	__Documentation.RegisterHelp( "Vector::Length", "float Vector::Length()", "Return the vector's length." );
	__Documentation.RegisterHelp( "Vector::LengthSqr", "float Vector::LengthSqr()", "Return the vector's squared length." );
	__Documentation.RegisterHelp( "Vector::Length2D", "float Vector::Length2D()", "Return the vector's 2D length." );
	__Documentation.RegisterHelp( "Vector::Length2DSqr", "float Vector::Length2DSqr()", "Return the vector's squared 2D length." );

	__Documentation.RegisterHelp( "Vector::Normalized", "float Vector::Normalized()", "Return a normalized version of the vector." );
	__Documentation.RegisterHelp( "Vector::Norm", "void Vector::Norm()", "Normalize the vector in place." );
	__Documentation.RegisterHelp( "Vector::Scale", "vector Vector::Scale(float)", "Scale the vector's magnitude and return the result." );
	__Documentation.RegisterHelp( "Vector::Dot", "float Vector::Dot(vector)", "Return the dot/scalar product of two vectors." );
	__Documentation.RegisterHelp( "Vector::Cross", "float Vector::Cross(vector)", "Return the vector product of two vectors." );

	__Documentation.RegisterHelp( "Vector::ToKVString", "string Vector::ToKVString()", "Return a vector as a string in KeyValue form, without separation commas." );

	__Documentation.RegisterMemberHelp( "Vector.x", "float Vector.x", "The vector's X coordinate on the cartesian X axis." );
	__Documentation.RegisterMemberHelp( "Vector.y", "float Vector.y", "The vector's Y coordinate on the cartesian Y axis." );
	__Documentation.RegisterMemberHelp( "Vector.z", "float Vector.z", "The vector's Z coordinate on the cartesian Z axis." );

	__Documentation.RegisterHelp( "clamp", "float clamp(float, float, float)", "" );
	__Documentation.RegisterHelp( "max", "float max(float, float)", "" );
	__Documentation.RegisterHelp( "min", "float min(float, float)", "" );
	__Documentation.RegisterHelp( "RemapVal", "float RemapVal(float, float, float, float, float)", "" );
	__Documentation.RegisterHelp( "RemapValClamped", "float RemapValClamped(float, float, float, float, float)", "" );
	__Documentation.RegisterHelp( "Approach", "float Approach(float, float, float)", "" );
	__Documentation.RegisterHelp( "AngleDistance", "float AngleDistance(float, float)", "" );
	__Documentation.RegisterHelp( "FLerp", "float FLerp(float, float, float, float, float)", "" );
	__Documentation.RegisterHelp( "Lerp", "float Lerp(float, float, float)", "" );
	__Documentation.RegisterHelp( "SimpleSpline", "float SimpleSpline(float)", "" );
}
)vscript";
