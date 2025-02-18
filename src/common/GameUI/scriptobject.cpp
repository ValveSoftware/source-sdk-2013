//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
// CScriptObject and CDescription class definitions
// 
#include "scriptobject.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <vgui_controls/Label.h>
#include "filesystem.h"
#include "tier1/convar.h"
#include "cdll_int.h"
#include "vgui/IVGui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
static char token[ 1024 ];

extern IVEngineClient *engine;

void StripFloatTrailingZeros(char *str)
{
	// scan for a '.'
	char *period = strchr(str, '.');
	if (!period)
		return;

	// start at the end and scan back to the period
	char *end = 0;
	for ( end = str + strlen(str) - 1; end > period; --end )
	{
		if (*end == '0')
		{
			*end = '\0';
		}
		else
		{
			// we've hit a real value, stop truncating
			break;
		}
	}

	// if we've made it up to the period, kill that to
	if ( *end == '.' )
	{
		*end = '\0';
	}
}

/////////////////////
objtypedesc_t objtypes[] =
{
	{ O_BOOL  , "BOOL" }, 
	{ O_NUMBER, "NUMBER" }, 
	{ O_LIST  , "LIST" }, 
	{ O_STRING, "STRING" },
	{ O_OBSOLETE  , "OBSOLETE" }, 
	{ O_SLIDER , "SLIDER" }, 
	{ O_CATEGORY, "CATEGORY" }, 
	{ O_BUTTON, "BUTTON" }, 
};

mpcontrol_t::mpcontrol_t( Panel *parent, char const *panelName )
: Panel( parent, panelName )
{
	type = O_BADTYPE;
	pControl = NULL;
	pPrompt = NULL;
	pScrObj = NULL;

	next = NULL;

	SetPaintBackgroundEnabled( false );
}

void mpcontrol_t::OnSizeChanged( int wide, int tall )
{
	int inset = 4;

	if ( pPrompt )
	{
		int w = wide / 2;

		if ( pControl )
		{
			pControl->SetBounds( w + 20, inset, w - 20, tall - 2 * inset );
		}
		pPrompt->SetBounds( 0, inset, w + 20, tall - 2 * inset  );
	}
	else
	{
		if ( pControl )
		{
			pControl->SetBounds( 0, inset, wide, tall - 2 * inset  );
		}
	}
}

CScriptListItem::CScriptListItem()
{
	pNext = NULL;
	memset( szItemText, 0, 128 ); 
	memset( szValue, 0, 256 );
}

CScriptListItem::CScriptListItem( char const *strItem, char const *strValue )
{
	pNext = NULL;
	Q_strncpy( szItemText, strItem, sizeof( szItemText ) );
	Q_strncpy( szValue   , strValue, sizeof( szValue ) );
}

CScriptObject::CScriptObject( void )
{
	type = O_BOOL;
	bSetInfo = false;  // Prepend "Setinfo" to keyvalue pair in config?
	pNext = NULL;
	pListItems = NULL;
	tooltip[0] = '\0';
}

CScriptObject::~CScriptObject()
{
	RemoveAndDeleteAllItems();
}

void CScriptObject::RemoveAndDeleteAllItems( void )
{
	CScriptListItem *p, *n;

	p = pListItems;
	while ( p )
	{
		n = p->pNext;
		delete p;
		p = n;
	}
	pListItems = NULL;
}

void CScriptObject::SetCurValue( char const *strValue )
{ 
	Q_strncpy( curValue, strValue, sizeof( curValue ) );

	fcurValue = (float)atof( curValue ); 

	if ( type == O_NUMBER || type == O_BOOL || type == O_SLIDER )
	{
		StripFloatTrailingZeros( curValue );
	}
}

void CScriptObject::AddItem( CScriptListItem *pItem )
{
	// Link it into the end of the list;
	CScriptListItem *p;
	p = pListItems;
	if ( !p )
	{
		pListItems = pItem;
		pItem->pNext = NULL;
		return;
	}

	while ( p )
	{
		if ( !p->pNext )
		{
			p->pNext = pItem;
			pItem->pNext = NULL;
			return;
		}
		p = p->pNext;
	}
}

/*
===================
UTIL_StripInvalidCharacters

Removes any possible formatting codes and double quote characters from the input string
===================
*/
void UTIL_StripInvalidCharacters( char *pszInput, int maxlen )
{
	char szOutput[4096];
	char *pIn, *pOut;
	
	pIn = pszInput;
	pOut = szOutput;

	*pOut = '\0';

	while ( *pIn )
	{
		if ( ( *pIn != '"' ) &&
			 ( *pIn != '%' ) )
		{
			*pOut++ = *pIn;
		}
		pIn++;
	}

	*pOut = '\0';

	// Copy back over, in place
	Q_strncpy( pszInput, szOutput, maxlen );
}

void FixupString( char *inString, int maxlen )
{
	char szBuffer[ 4096 ];
	Q_strncpy( szBuffer, inString, sizeof( szBuffer ) );
	UTIL_StripInvalidCharacters( szBuffer, sizeof( szBuffer ) );
	Q_strncpy( inString, szBuffer, maxlen );
}

/*
===================
CleanFloat

Removes any ".000" from the end of floats
===================
*/
char * CleanFloat( float val )
{
	static int curstring  = 0;
	static char string[2][32];
	curstring = ( curstring + 1 ) % 2;

	Q_snprintf( string[curstring], sizeof( string[curstring] ), "%f", val );

	char * str = string[curstring];

	char * tmp = str;
	if ( !str || !*str || !strchr( str, '.' ) )
		return str;

	while ( *tmp )
		++tmp;
	--tmp;

	while ( *tmp == '0' && tmp > str )
	{
		*tmp = 0;
		--tmp;
	}

	if ( *tmp == '.' )
	{
		*tmp = 0;
	}

	return str;
}

void CScriptObject::WriteToScriptFile( FileHandle_t fp )
{
	if ( type == O_OBSOLETE )
		return;

	FixupString( cvarname, sizeof( cvarname ) );
	g_pFullFileSystem->FPrintf( fp, "\t\"%s\"\r\n", cvarname );

	g_pFullFileSystem->FPrintf( fp, "\t{\r\n" );

	CScriptListItem *pItem;

	FixupString( prompt, sizeof( prompt ) );
	FixupString( tooltip, sizeof( tooltip ) );

	switch ( type )
	{
	case O_BOOL:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ BOOL }\r\n" );
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%i\" }\r\n", (int)fcurValue ? 1 : 0 );
		break;
	case O_NUMBER:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ NUMBER %s %s }\r\n", CleanFloat(fMin), CleanFloat(fMax) );
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%s\" }\r\n", CleanFloat(fcurValue) );
		break;
	case O_STRING:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ STRING }\r\n" );
		FixupString( curValue, sizeof( curValue ) );
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%s\" }\r\n", curValue );
		break;
	case O_LIST:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{\r\n\t\t\tLIST\r\n" );
		
		pItem = pListItems;
		while ( pItem )
		{
			UTIL_StripInvalidCharacters( pItem->szItemText, sizeof( pItem->szItemText ) );
			UTIL_StripInvalidCharacters( pItem->szValue, sizeof( pItem->szValue ) );
			g_pFullFileSystem->FPrintf( fp, "\t\t\t\"%s\" \"%s\"\r\n",
				pItem->szItemText, pItem->szValue );

			pItem = pItem->pNext;
		}

		g_pFullFileSystem->FPrintf( fp, "\t\t}\r\n");
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%s\" }\r\n", CleanFloat(fcurValue) );
		break;
	case O_SLIDER:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ SLIDER %s %s }\r\n", CleanFloat(fMin), CleanFloat(fMax) );
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%s\" }\r\n", CleanFloat(fcurValue) );
		break;
	case O_CATEGORY:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[0] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ CATEGORY }\r\n" );
		break;
	case O_BUTTON:
		g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", prompt );
		if ( tooltip[ 0 ] )
		{
			g_pFullFileSystem->FPrintf( fp, "\t\t\"%s\"\r\n", tooltip );
		}
		g_pFullFileSystem->FPrintf( fp, "\t\t{ BUTTON }\r\n" );
		FixupString( curValue, sizeof( curValue ) );
		g_pFullFileSystem->FPrintf( fp, "\t\t{ \"%s\" }\r\n", curValue );
		break;
	}

	if ( bSetInfo )
		g_pFullFileSystem->FPrintf( fp, "\t\tSetInfo\r\n" );

	g_pFullFileSystem->FPrintf( fp, "\t}\r\n\r\n" );
}

void CScriptObject::WriteToFile( FileHandle_t fp )
{
	if ( type == O_OBSOLETE || type == O_CATEGORY || type == O_BUTTON )
		return;

	FixupString( cvarname, sizeof( cvarname ) );
	g_pFullFileSystem->FPrintf( fp, "\"%s\"\t\t", cvarname );

	CScriptListItem *pItem;
	float fVal;

	switch ( type )
	{
	case O_BOOL:
		g_pFullFileSystem->FPrintf( fp, "\"%s\"\r\n", fcurValue != 0.0 ? "1" : "0" );
		break;
	case O_NUMBER:
	case O_SLIDER:
		fVal = fcurValue;
		if ( fMin != -1.0 )
			fVal = max( fVal, fMin );
		if ( fMax != -1.0 )
			fVal = min( fVal, fMax );
		g_pFullFileSystem->FPrintf( fp, "\"%f\"\r\n", fVal );
		break;
	case O_STRING:
		FixupString( curValue, sizeof( curValue ) );
		g_pFullFileSystem->FPrintf( fp, "\"%s\"\r\n", curValue );
		break;
	case O_LIST:
		pItem = pListItems;
		while ( pItem )
		{
			if ( !Q_stricmp( pItem->szValue, curValue ) )
				break;

			pItem = pItem->pNext;
		}

		if ( pItem )
		{
			UTIL_StripInvalidCharacters( pItem->szValue, sizeof( pItem->szValue ) );
			g_pFullFileSystem->FPrintf( fp, "\"%s\"\r\n", pItem->szValue );
		}
		else  //Couln't find index
		{
			g_pFullFileSystem->FPrintf( fp, "\"0\"\r\n" );
		}
		break;
	}
}

void CScriptObject::WriteToConfig( void )
{
	if ( type == O_OBSOLETE || type == O_CATEGORY || type == O_BUTTON )
		return;

	char *pszKey;
	char szValue[2048];

	pszKey = ( char * )cvarname;

	CScriptListItem *pItem;
	float fVal;

	switch ( type )
	{
	case O_BOOL:
		Q_snprintf( szValue, sizeof( szValue ), "%s", fcurValue != 0.0 ? "1" : "0" );
		break;
	case O_NUMBER:
	case O_SLIDER:
		fVal = fcurValue;
		if ( fMin != -1.0 )
			fVal = max( fVal, fMin );
		if ( fMax != -1.0 )
			fVal = min( fVal, fMax );
		Q_snprintf( szValue, sizeof( szValue ), "%f", fVal );
		break;
	case O_STRING:
		Q_snprintf( szValue, sizeof( szValue ), "\"%s\"", (char *)curValue );
		UTIL_StripInvalidCharacters( szValue, sizeof( szValue ) );
		break;
	case O_LIST:
		pItem = pListItems;
		while ( pItem )
		{
			if ( !Q_stricmp( pItem->szValue, curValue ) )
				break;

			pItem = pItem->pNext;
		}

		if ( pItem )
		{
			Q_snprintf( szValue, sizeof( szValue ), "%s", pItem->szValue );
			UTIL_StripInvalidCharacters( szValue, sizeof( szValue ) );
		}
		else  //Couldn't find index
		{
			Q_strncpy( szValue, "0", sizeof( szValue ) );
		}
		break;
	}

	char command[ 256 ];
	if ( bSetInfo )
	{
		Q_snprintf( command, sizeof(command), "setinfo %s \"%s\"\n", pszKey, szValue );
	}
	else
	{
		Q_snprintf( command, sizeof(command), "%s \"%s\"\n", pszKey, szValue );
	}

	engine->ClientCmd_Unrestricted( command );

	//	CFG_SetKey( g_szCurrentConfigFile, pszKey, szValue, bSetInfo );
}


objtype_t CScriptObject::GetType( char *pszType )
{
	int i;
	int nTypes;

	nTypes = sizeof( objtypes ) / sizeof( objtypedesc_t);

	for ( i = 0; i < nTypes; i++ )
	{
		if ( !stricmp( objtypes[i].szDescription, pszType ) )
			return objtypes[i].type;
	}

	return O_BADTYPE;
}

bool CScriptObject::ReadFromBuffer( const char **pBuffer, bool isNewObject )
{
	// Get the first token.
	// The cvar we are setting
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( isNewObject )
	{
		Q_strncpy( cvarname, token, sizeof( cvarname ) );
	}

	// Parse the {
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( strcmp( token, "{" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}

	// Parse the Prompt
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( isNewObject )
	{
		Q_strncpy( prompt, token, sizeof( prompt ) );
	}

	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	// If it's not a {, consider it the optional tooltip
	if ( strcmp( token, "{" ) )
	{
		Q_strncpy( tooltip, token, sizeof( tooltip ) );

		// Parse the next {
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;
	}

	if ( strcmp( token, "{" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}

	// Now parse the type:
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	objtype_t newType = GetType( token );
	if ( isNewObject )
	{
		type = newType;
	}
	if ( newType == O_BADTYPE )
	{
		Msg( "Type '%s' unknown", token );
		return false;
	}

	// If it's a category, we're done.
	if ( newType == O_CATEGORY )
	{
		// Parse the }
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;
		if ( strcmp( token, "}" ) )
		{
			Msg( "Expecting '{', got '%s'", token );
			return false;
		}

		// Parse the final }
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;
		if ( strcmp( token, "}" ) )
		{
			Msg( "Expecting '{', got '%s'", token );
			return false;
		}
		return true;
	}

	switch ( newType )
	{
	case O_OBSOLETE:
	case O_BOOL:
	case O_STRING:
	case O_BUTTON:
		// Parse the next {
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;

		if ( strcmp( token, "}" ) )
		{
			Msg( "Expecting '{', got '%s'", token );
			return false;
		}
		break;
	case O_NUMBER:
	case O_SLIDER:
		// Parse the Min
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;
	
		if ( isNewObject )
		{
			fMin = (float)atof( token );
		}

		// Parse the Min
		*pBuffer = engine->ParseFile( *pBuffer, token , sizeof( token ));
		if ( strlen( token ) <= 0 )
			return false;
	
		if ( isNewObject )
		{
			fMax = (float)atof( token );
		}

		// Parse the next {
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;

		if ( strcmp( token, "}" ) )
		{
			Msg( "Expecting '{', got '%s'", token );
			return false;
		}
		break;
	case O_LIST:
		// Parse items until we get the }
		while ( 1 )
		{
			// Parse the next {
			*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
			if ( strlen( token ) <= 0 )
				return false;

			// Done?
			if ( !strcmp( token, "}" ) )
				break;

			//
			// Add the item to a list somewhere
			// AddItem( token )
			char strItem[ 128 ];
			char strValue[128];

			Q_strncpy( strItem, token, sizeof( strItem ) );

			// Parse the value
				*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
			if ( strlen( token ) <= 0 )
				return false;

			Q_strncpy( strValue, token, sizeof( strValue ) );

			if ( isNewObject )
			{
				CScriptListItem *pItem;
				pItem = new CScriptListItem( strItem, strValue );

				AddItem( pItem );
			}
		}
		break;
	}

	//
	// Now read in the default value

	// Parse the {
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( strcmp( token, "{" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}

	// Parse the default
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	//if ( strlen( token ) <= 0 )
	//	return false;

	// Set the values
	Q_strncpy( defValue, token, sizeof( defValue ) );
	fdefValue = (float)atof( token );

	if (type == O_NUMBER || type == O_SLIDER)
	{
		StripFloatTrailingZeros( defValue );
	}

	SetCurValue( defValue );

	// Parse the }
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( strcmp( token, "}" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}

	// Parse the final }
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( !stricmp( token, "SetInfo" ) )
	{
		bSetInfo = true;
		// Parse the final }
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;
	}

	if ( strcmp( token, "}" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}
	
	return true;
}

/////////////////////////
CDescription::CDescription( void )
{
	pObjList = NULL;
}

CDescription::~CDescription()
{
	CScriptObject *p, *n;

	p = pObjList;
	while ( p )
	{
		n = p->pNext;
		p->pNext = NULL;
		p->MarkForDeletion();
		//delete p;
		p = n;
	}

	pObjList = NULL;
	
	if ( m_pszHintText )
		free( m_pszHintText );
	if ( m_pszDescriptionType )
		free( m_pszDescriptionType );
}

CScriptObject * CDescription::FindObject( const char *pszObjectName )
{
	if ( !pszObjectName )
		return NULL;

	CScriptObject *p;
	p = pObjList;
	while ( p )
	{
		if ( !stricmp( pszObjectName, p->cvarname ) )
			return p;
		p = p->pNext;
	}
	return NULL;
}

void CDescription::AddObject( CScriptObject *pObj )
{
	CScriptObject *p;
	p = pObjList;
	if ( !p )
	{
		pObjList = pObj;
		pObj->pNext = NULL;
		return;
	}

	while ( p )
	{
		if ( !p->pNext )
		{
			p->pNext = pObj;
			pObj->pNext = NULL;
			return;
		}
		p = p->pNext;
	}
}

bool CDescription::ReadFromBuffer( const char **pBuffer, bool bAllowNewObject )
{
	// Get the first token.
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	// Read VERSION #
	if ( stricmp ( token, "VERSION" ) )
	{
		Msg( "Expecting 'VERSION', got '%s'", token );
		return false;
	}

	// Parse in the version #
	// Get the first token.
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
	{
		Msg( "Expecting version #" );
		return false;
	}

	float fVer;
	fVer = (float)atof( token );

	if ( fVer != SCRIPT_VERSION )
	{
		Msg( "Version mismatch, expecting %f, got %f", SCRIPT_VERSION, fVer );
		return false;
	}

	// Get the "DESCRIPTION"
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	// Read DESCRIPTION
	if ( stricmp ( token, "DESCRIPTION" ) )
	{
		Msg( "Expecting 'DESCRIPTION', got '%s'", token );
		return false;
	}

	// Parse in the description type
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
	{
		Msg( "Expecting '%s'", m_pszDescriptionType );
		return false;
	}

	if ( stricmp ( token, m_pszDescriptionType ) )
	{
		Msg( "Expecting %s, got %s", m_pszDescriptionType, token );
		return false;
	}

	// Parse the {
	*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
	if ( strlen( token ) <= 0 )
		return false;

	if ( strcmp( token, "{" ) )
	{
		Msg( "Expecting '{', got '%s'", token );
		return false;
	}

	const char *pStart;
	CScriptObject *pObj;
	// Now read in the objects and link them in
	while ( 1 )
	{
		pStart = *pBuffer;

		// Get the first token.
		*pBuffer = engine->ParseFile( *pBuffer, token, sizeof( token ) );
		if ( strlen( token ) <= 0 )
			return false;

		// Read "cvar name" or  } when done
		if ( !stricmp ( token, "}" ) )
			break;

		// Unget the token
		*pBuffer = pStart;

		// Create a new object
		bool mustAdd = bAllowNewObject;
		pObj = FindObject( token );
		if ( pObj )
		{
			pObj->ReadFromBuffer( &pStart, false );
			mustAdd = false; // already in list
		}
		else
		{
			pObj = new CScriptObject();
			if ( !pObj )
			{
				Msg( "Couldn't create script object" );
				return false;
			}

			if ( !pObj->ReadFromBuffer( &pStart, true ) )
			{
				delete pObj;
				return false;
			}

			// throw away the object if we're not adding it.
			if ( !mustAdd )
			{
				delete pObj;
			}
		}

		*pBuffer = pStart;

		// Add to list
		// Fixme, move to end of list first
		if ( mustAdd )
		{
			AddObject( pObj );
		}
	}

	return true;
}

bool CDescription::InitFromFile( const char *pszFileName, bool bAllowNewObject /*= true*/ )
{

	// Load file into memory
	FileHandle_t file = g_pFullFileSystem->Open( pszFileName, "rb" );
	if ( !file )
		return false;

	int len =g_pFullFileSystem->Size( file );

	// read the file
	byte *buffer = new unsigned char[ len ];
	Assert( buffer );
	g_pFullFileSystem->Read( buffer, len, file );
	g_pFullFileSystem->Close( file );

	const char *pBuffer = (const char*)buffer;
	
	ReadFromBuffer( &pBuffer, bAllowNewObject );

	delete[] buffer;

	return true;
}

void CDescription::WriteToFile( FileHandle_t fp )
{
	CScriptObject *pObj;

	WriteFileHeader( fp );

	pObj = pObjList;
	while ( pObj )
	{
		pObj->WriteToFile( fp );
		pObj = pObj->pNext;
	}
}

void CDescription::WriteToConfig( void )
{
	CScriptObject *pObj;

	pObj = pObjList;
	while ( pObj )
	{
		pObj->WriteToConfig();
		pObj = pObj->pNext;
	}
}

void CDescription::WriteToScriptFile( FileHandle_t fp )
{
	CScriptObject *pObj;

	WriteScriptHeader( fp );

	pObj = pObjList;
	while ( pObj )
	{
		pObj->WriteToScriptFile( fp );
		pObj = pObj->pNext;
	}

	g_pFullFileSystem->FPrintf( fp, "}\r\n" );
}

void CDescription::TransferCurrentValues( const char *pszConfigFile )
{
	char szValue[ 1024 ];

	CScriptObject *pObj;

	pObj = pObjList;
	while ( pObj )
	{

		/*
			TODO: if/when prefixed keys are implemented
		  const char *value;
		if ( pObj->bSetInfo )
		{
			value = engine->LocalPlayerInfo_ValueForKey( pObj->cvarname ); // use LocalPlayerInfo because PlayerInfo strips keys prefixed with "_"
		}
		else
		{
			value = engine->pfnGetCvarString( pObj->cvarname );
		}
		*/

		UIConVarRef var( g_pVGui->GetVGUIEngine(), pObj->cvarname, true );
		if ( !var.IsValid() )
		{
			if ( pObj->type != O_CATEGORY && pObj->type != O_BUTTON )
			{
				DevMsg( "Could not find '%s'\n", pObj->cvarname );
			}
			pObj = pObj->pNext;
			continue;
		}
		const char *value = var.GetString();

		if ( value && value[ 0 ] )
		//if ( CFG_GetValue( pszConfigFile, pObj->cvarname, szValue ) )
		{
			Q_strncpy( szValue, value, sizeof( szValue ) );

			// Fill in better default value
			// 
			Q_strncpy( pObj->curValue,  szValue, sizeof( pObj->curValue ) );
			pObj->fcurValue = (float)atof( szValue );

			Q_strncpy( pObj->defValue, szValue, sizeof( pObj->defValue ) );
			pObj->fdefValue = (float)atof( szValue );
		}

		pObj = pObj->pNext;
	}
}

void CDescription::setDescription( const char *pszDesc )
{ 
	m_pszDescriptionType = strdup( pszDesc );
}

void CDescription::setHint( const char *pszHint )
{ 
	m_pszHintText = strdup( pszHint );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor, load/save client settings object
//-----------------------------------------------------------------------------
CInfoDescription::CInfoDescription( void )
	: CDescription(  )
{
	setHint( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n\
//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n\
// User options script\r\n\
//\r\n\
// Format:\r\n\
//  Version [float]\r\n\
//  Options description followed by \r\n\
//  Options defaults\r\n\
//\r\n\
// Option description syntax:\r\n\
//\r\n\
//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n\
//\r\n\
//  type = \r\n\
//   BOOL   (a yes/no toggle)\r\n\
//   STRING\r\n\
//   NUMBER\r\n\
//   LIST\r\n\
//\r\n\
// type info:\r\n\
// BOOL                 no type info\r\n\
// NUMBER       min max range, use -1 -1 for no limits\r\n\
// STRING       no type info\r\n\
// LIST         "" delimited list of options value pairs\r\n\
//\r\n\
//\r\n\
// default depends on type\r\n\
// BOOL is \"0\" or \"1\"\r\n\
// NUMBER is \"value\"\r\n\
// STRING is \"value\"\r\n\
// LIST is \"index\", where index \"0\" is the first element of the list\r\n\r\n\r\n" );

	setDescription( "INFO_OPTIONS" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInfoDescription::WriteScriptHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
	tm newtime;
	VCRHook_LocalTime( &newtime );

	g_pFullFileSystem->FPrintf( fp, (char *)getHint() );

// Write out the comment and Cvar Info:
	g_pFullFileSystem->FPrintf( fp, "// Half-Life User Info Configuration Layout Script (stores last settings chosen, too)\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	g_pFullFileSystem->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
	g_pFullFileSystem->FPrintf( fp, "VERSION %.1f\r\n\r\n", SCRIPT_VERSION );
	g_pFullFileSystem->FPrintf( fp, "DESCRIPTION INFO_OPTIONS\r\n{\r\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInfoDescription::WriteFileHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
	tm newtime;
	VCRHook_LocalTime( &newtime );

	g_pFullFileSystem->FPrintf( fp, "// Half-Life User Info Configuration Settings\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	g_pFullFileSystem->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}

//-----------------------------------------------------------------------------