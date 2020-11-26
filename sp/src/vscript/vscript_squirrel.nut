static char g_Script_vscript_squirrel[] = R"vscript(
//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
// 
//=============================================================================//

Warning <- error;

function clamp(val,min,max)
{
	if ( max < min )
		return max;
	else if( val < min )
		return min;
	else if( val > max )
		return max;
	else
		return val;
}

function max(a,b) return a > b ? a : b

function min(a,b) return a < b ? a : b

function RemapVal(val, A, B, C, D)
{
	if ( A == B )
		return val >= B ? D : C;
	return C + (D - C) * (val - A) / (B - A);
}

function RemapValClamped(val, A, B, C, D)
{
	if ( A == B )
		return val >= B ? D : C;
	local cVal = (val - A) / (B - A);
	cVal = (cVal < 0.0) ? 0.0 : (1.0 < cVal) ? 1.0 : cVal;
	return C + (D - C) * cVal;
}

function Approach( target, value, speed )
{
	local delta = target - value

	if( delta > speed )
		value += speed
	else if( delta < (-speed) )
		value -= speed
	else
		value = target

	return value
}

function AngleDistance( next, cur )
{
	local delta = next - cur

	if ( delta < (-180.0) )
		delta += 360.0
	else if ( delta > 180.0 )
		delta -= 360.0

	return delta
}

function printl( text )
{
	return ::print(text + "\n");
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
		local func;
		try {
			func = scope[prefix];
		} catch(e) {
			return;
		}
		if (typeof(func) != "function")
			return;
		chain.push(func);
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

DocumentedFuncs <- {}
DocumentedClasses <- {}
DocumentedEnums <- {}
DocumentedConsts <- {}
DocumentedHooks <- {}

function AddAliasedToTable(name, signature, description, table)
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

function RegisterHelp(name, signature, description)
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

function RegisterClassHelp(name, baseclass, description)
{
	DocumentedClasses[name] <- [baseclass, description];
}

function RegisterEnumHelp(name, description)
{
	DocumentedEnums[name] <- description;
}

function RegisterConstHelp(name, signature, description)
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

function RegisterHookHelp(name, signature, description)
{
	DocumentedHooks[name] <- [signature, description];
}

function printdoc( text )
{
	return ::printc(200,224,255,text);
}

function printdocl( text )
{
	return printdoc(text + "\n");
}

function PrintClass(name, doc)
{
	printdocl("=====================================");
	printdocl("Class:    " + name);
	printdocl("Base:   " + doc[0]);
	if (doc[1].len())
		printdocl("Description: " + doc[1]);
	printdocl("=====================================");
	print("\n");
}

function PrintFunc(name, doc)
{
	printdocl("Function:    " + name);
	if (doc[0] == null)
	{
		// Is an aliased function
		printdoc("Signature:   function " + name + "(");
		foreach(k,v in this[name].getinfos().parameters)
		{
			if (k == 0 && v == "this") continue;
			if (k > 1) printdoc(", ");
			printdoc(v);
		}
		printdocl(")");
	}
	else
	{
		printdocl("Signature:   " + doc[0]);
	}
	if (doc[1].len())
		printdocl("Description: " + doc[1]);
	print("\n");
}

function PrintEnum(name, doc)
{
	printdocl("=====================================");
	printdocl("Enum:    " + name);
	if (doc.len())
		printdocl("Description: " + doc);
	printdocl("=====================================");
	print("\n");
}

function PrintConst(name, doc)
{
	printdocl("Constant:    " + name);
	if (doc[0] == null)
	{
		// Is an aliased function
		printdoc("Signature:   function " + name + "(");
		foreach(k,v in this[name].getinfos().parameters)
		{
			if (k == 0 && v == "this") continue;
			if (k > 1) printdoc(", ");
			printdoc(v);
		}
		printdocl(")");
	}
	else
	{
		printdocl("Value:   " + doc[0]);
	}
	if (doc[1].len())
		printdocl("Description: " + doc[1]);
	print("\n");
}

function PrintHook(name, doc)
{
	printdocl("Hook:    " + name);
	if (doc[0] == null)
	{
		// Is an aliased function
		printdoc("Signature:   function " + name + "(");
		foreach(k,v in this[name].getinfos().parameters)
		{
			if (k == 0 && v == "this") continue;
			if (k > 1) printdoc(", ");
			printdoc(v);
		}
		printdocl(")");
	}
	else
	{
		printdocl("Signature:   " + doc[0]);
	}
	if (doc[1].len())
		printdocl("Description: " + doc[1]);
	print("\n");
}

function PrintHelp(pattern = "*")
{
	local foundMatches = false;
	foreach(name, doc in DocumentedClasses)
	{
		if (pattern == "*" || name.tolower().find(pattern.tolower()) != null)
		{
			foundMatches = true;
			PrintClass(name, doc)
		}
	}

	foreach(name, doc in DocumentedFuncs)
	{
		if (pattern == "*" || name.tolower().find(pattern.tolower()) != null)
		{
			foundMatches = true;
			PrintFunc(name, doc)
		}
	}

	foreach(name, doc in DocumentedEnums)
	{
		if (pattern == "*" || name.tolower().find(pattern.tolower()) != null)
		{
			foundMatches = true;
			PrintEnum(name, doc)
		}
	}

	foreach(name, doc in DocumentedConsts)
	{
		if (pattern == "*" || name.tolower().find(pattern.tolower()) != null)
		{
			foundMatches = true;
			PrintConst(name, doc)
		}
	}

	foreach(name, doc in DocumentedHooks)
	{
		if (pattern == "*" || name.tolower().find(pattern.tolower()) != null)
		{
			foundMatches = true;
			PrintHook(name, doc)
		}
	}

	if (!foundMatches)
		printdocl("Pattern " + pattern + " not found");
}

)vscript";