//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#include <windows.h>
#include <tier0/dbg.h>
#include <io.h>
#include <WorldSize.h>
#include "fgdlib/GameData.h"
#include "fgdlib/HelperInfo.h"
#include "KeyValues.h"
#include "filesystem_tools.h"
#include "tier1/strtools.h"
#include "utlmap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning(disable:4244)


const int MAX_ERRORS = 5;


static GameDataMessageFunc_t g_pMsgFunc = NULL;


//-----------------------------------------------------------------------------
// Sets the function used for emitting error messages while loading gamedata files.
//-----------------------------------------------------------------------------
void GDSetMessageFunc(GameDataMessageFunc_t pFunc)
{
	g_pMsgFunc = pFunc;
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - 
//			ppszStore - Destination buffer, one of the following:
//				pointer to NULL - token will be placed in an allocated buffer
//				pointer to non-NULL buffer - token will be placed in buffer
//			ttexpecting - 
//			pszExpecting - 
// Output : 
//-----------------------------------------------------------------------------
static bool DoGetToken(TokenReader &tr, char **ppszStore, int nSize, trtoken_t ttexpecting, const char *pszExpecting)
{
	trtoken_t ttype;

	if (*ppszStore != NULL)
	{
		// Reads the token into the given buffer.
		ttype = tr.NextToken(*ppszStore, nSize);
	}
	else
	{
		// Allocates a buffer to hold the token.
		ttype = tr.NextTokenDynamic(ppszStore);
	}

	if (ttype == TOKENSTRINGTOOLONG)
	{
		GDError(tr, "unterminated string or string too long");
		return false;
	}

	//
	// Check for a bad token type.
	//
	char *pszStore = *ppszStore;
	bool bBadTokenType = false;
	if ((ttype != ttexpecting) && (ttexpecting != TOKENNONE))
	{
		//
		// If we were expecting a string and got an integer, don't worry about it.
		// We can translate from integer to string.
		//
		if (!((ttexpecting == STRING) && (ttype == INTEGER)))
		{
			bBadTokenType = true;
		}
	}

	if (bBadTokenType && (pszExpecting == NULL))
	{
		//
		// We didn't get the expected token type but no expected
		// string was specified.
		//
		char *pszTokenName;
		switch (ttexpecting)
		{
			case IDENT:
			{
				pszTokenName = "identifier";
				break;
			}

			case INTEGER:
			{
				pszTokenName = "integer";
				break;
			}

			case STRING:
			{
				pszTokenName = "string";
				break;
			}

			case OPERATOR:
			default:
			{
				pszTokenName = "symbol";
				break;
			}
		}
		
		GDError(tr, "expecting %s", pszTokenName);
		return false;
	}
	else if (bBadTokenType || ((pszExpecting != NULL) && !IsToken(pszStore, pszExpecting)))
	{
		//
		// An expected string was specified, and we got either the wrong type or
		// the right type but the wrong string,
		//
		GDError(tr, "expecting '%s', but found '%s'", pszExpecting, pszStore);
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tr - 
//			error - 
// Output : 
//-----------------------------------------------------------------------------
bool GDError(TokenReader &tr, const char *error, ...)
{
	char szBuf[128];
	va_list vl;
	va_start(vl, error);
	vsprintf(szBuf, error, vl);
	va_end(vl);

	if (g_pMsgFunc)
	{
		// HACK: should use an enumeration for error level
		g_pMsgFunc(1, tr.Error(szBuf));
	}
	
	if (tr.GetErrorCount() >= MAX_ERRORS)
	{
		if (g_pMsgFunc)
		{
			// HACK: should use an enumeration for error level
			g_pMsgFunc(1, "   - too many errors; aborting.");
		}
		
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - The token reader object with which to fetch the token.
//			pszStore - Buffer in which to place the token, NULL to discard the token.
//			ttexpecting - The token type that we are expecting. If this is not TOKENNONE
//				and token type read is different, the operation will fail.
//			pszExpecting - The token string that we are expecting. If this string
//				is not NULL and the token string read is different, the operation will fail.
// Output : Returns TRUE if the operation succeeded, FALSE if there was an error.
//			If there was an error, the error will be reported in the message window.
//-----------------------------------------------------------------------------
bool GDGetToken(TokenReader &tr, char *pszStore, int nSize, trtoken_t ttexpecting, const char *pszExpecting)
{
	Assert(pszStore != NULL);
	if (pszStore != NULL)
	{
		return DoGetToken(tr, &pszStore, nSize, ttexpecting, pszExpecting);
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - The token reader object with which to fetch the token.
//			pszStore - Buffer in which to place the token, NULL to discard the token.
//			ttexpecting - The token type that we are expecting. If this is not TOKENNONE
//				and token type read is different, the operation will fail.
//			pszExpecting - The token string that we are expecting. If this string
//				is not NULL and the token string read is different, the operation will fail.
// Output : Returns TRUE if the operation succeeded, FALSE if there was an error.
//			If there was an error, the error will be reported in the message window.
//-----------------------------------------------------------------------------
bool GDSkipToken(TokenReader &tr, trtoken_t ttexpecting, const char *pszExpecting)
{
	//
	// Read the next token into a buffer and discard it.
	//
	char szDiscardBuf[MAX_TOKEN];
	char *pszDiscardBuf = szDiscardBuf;
	return DoGetToken(tr, &pszDiscardBuf, sizeof(szDiscardBuf), ttexpecting, pszExpecting);
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file, allocating a buffer exactly
//			large enough to hold the token.
// Input  : tr - 
//			ppszStore - 
//			ttexpecting - 
//			pszExpecting - 
// Output : 
//-----------------------------------------------------------------------------
bool GDGetTokenDynamic(TokenReader &tr, char **ppszStore, trtoken_t ttexpecting, const char *pszExpecting)
{
	if (ppszStore == NULL)
	{
		return false;
	}

	*ppszStore = NULL;
	return DoGetToken(tr, ppszStore, -1, ttexpecting, pszExpecting);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
GameData::GameData(void)
{
	m_nMaxMapCoord = 8192;
	m_nMinMapCoord = -8192;
	m_InstanceClass = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
GameData::~GameData(void)
{
	ClearData();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GameData::ClearData(void)
{
	// delete classes.
	int nCount = m_Classes.Count();
	for (int i = 0; i < nCount; i++)
	{
		GDclass *pm = m_Classes.Element(i);
		delete pm;
	}
	m_Classes.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Loads a gamedata (FGD) file into this object.
// Input  : pszFilename - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL GameData::Load(const char *pszFilename)
{
	TokenReader tr;

	if(GetFileAttributes(pszFilename) == 0xffffffff)
		return FALSE;

	if(!tr.Open(pszFilename))
		return FALSE;

	trtoken_t ttype;
	char szToken[128];

	while (1)
	{
		if (tr.GetErrorCount() >= MAX_ERRORS)
		{
			break;
		}

		ttype = tr.NextToken(szToken, sizeof(szToken));

		if(ttype == TOKENEOF)
			break;

		if(ttype != OPERATOR || !IsToken(szToken, "@"))
		{
			if(!GDError(tr, "expected @"))
				return FALSE;
		}

		// check what kind it is, and parse a new object
		if (tr.NextToken(szToken, sizeof(szToken)) != IDENT)
		{
			if(!GDError(tr, "expected identifier after @"))
				return FALSE;
		}

		if (IsToken(szToken, "baseclass") || IsToken(szToken, "pointclass") || IsToken(szToken, "solidclass") || IsToken(szToken, "keyframeclass") ||
			IsToken(szToken, "moveclass") || IsToken(szToken, "npcclass") || IsToken(szToken, "filterclass"))
		{
			//
			// New class.
			//
			GDclass *pNewClass = new GDclass;
			if (!pNewClass->InitFromTokens(tr, this))
			{
				tr.IgnoreTill(OPERATOR, "@");	// go to next section
				delete pNewClass;
			}
			else
			{
				if (IsToken(szToken, "baseclass"))			// Not directly available to user.
				{
					pNewClass->SetBaseClass(true);
				}
				else if (IsToken(szToken, "pointclass"))	// Generic point class.
				{
					pNewClass->SetPointClass(true);
				}
				else if (IsToken(szToken, "solidclass"))	// Tied to solids.
				{
					pNewClass->SetSolidClass(true);
				}
				else if (IsToken(szToken, "npcclass"))		// NPC class - can be spawned by npc_maker.
				{
					pNewClass->SetPointClass(true);
					pNewClass->SetNPCClass(true);
				}
				else if (IsToken(szToken, "filterclass"))	// Filter class - can be used as a filter
				{
					pNewClass->SetPointClass(true);
					pNewClass->SetFilterClass(true);
				}
				else if (IsToken(szToken, "moveclass"))		// Animating
				{
					pNewClass->SetMoveClass(true);
					pNewClass->SetPointClass(true);
				}
				else if (IsToken(szToken, "keyframeclass"))	// Animation keyframes
				{
					pNewClass->SetKeyFrameClass(true);
					pNewClass->SetPointClass(true);
				}

				// Check and see if this new class matches an existing one. If so we will override the previous definition.
				int nExistingClassIndex = 0;
				GDclass *pExistingClass = ClassForName(pNewClass->GetName(), &nExistingClassIndex);
				if (NULL != pExistingClass)
				{
					m_Classes.InsertAfter(nExistingClassIndex, pNewClass);
					m_Classes.Remove(nExistingClassIndex);
				}
				else
				{
					m_Classes.AddToTail(pNewClass);
				}
			}
		}
		else if (IsToken(szToken, "include"))
		{
			if (GDGetToken(tr, szToken, sizeof(szToken), STRING))
			{
				// Let's assume it's in the same directory.
				char justPath[MAX_PATH], loadFilename[MAX_PATH];
				if ( Q_ExtractFilePath( pszFilename, justPath, sizeof( justPath ) ) )
				{
					Q_snprintf( loadFilename, sizeof( loadFilename ), "%s%s", justPath, szToken );
				}
				else
				{
					Q_strncpy( loadFilename, szToken, sizeof( loadFilename ) );
				}

				// First try our fully specified directory
				if (!Load(loadFilename))
				{
					// Failing that, try our start directory
					if (!Load(szToken))
					{
						GDError(tr, "error including file: %s", szToken);
					}
				}
			}
		}
		else if (IsToken(szToken, "mapsize"))
		{
			if (!ParseMapSize(tr))
			{
				// Error in map size specifier, skip to next @ sign. 
				tr.IgnoreTill(OPERATOR, "@");
			}
		}
		else if ( IsToken( szToken, "materialexclusion" ) )
		{
			if ( !LoadFGDMaterialExclusions( tr ) )
			{
				// FGD exclusions not defined; skip to next @ sign. 
				tr.IgnoreTill(OPERATOR, "@");
			}
		}
		else if ( IsToken( szToken, "autovisgroup" ) )
		{
			if ( !LoadFGDAutoVisGroups( tr ) )
			{
				// FGD AutoVisGroups not defined; skip to next @ sign. 
				tr.IgnoreTill(OPERATOR, "@");
			}
		}
		else
		{
			GDError(tr, "unrecognized section name %s", szToken);
			tr.IgnoreTill(OPERATOR, "@");
		}
	}

	if (tr.GetErrorCount() > 0)
	{
		return FALSE;
	}

	tr.Close();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Parses the "mapsize" specifier, which should be of the form:
//
//			mapsize(min, max)
//
//			ex: mapsize(-8192, 8192)
//
// Input  : tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GameData::ParseMapSize(TokenReader &tr)
{
	if (!GDSkipToken(tr, OPERATOR, "("))
	{
		return false;
	}

	char szToken[128];
	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return false;
	}
	int nMin = atoi(szToken);	

	if (!GDSkipToken(tr, OPERATOR, ","))
	{
		return false;
	}

	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return false;
	}
	int nMax = atoi(szToken);	

	if (nMin != nMax)
	{
		m_nMinMapCoord = min(nMin, nMax);
		m_nMaxMapCoord = max(nMin, nMax);
	}

	if (!GDSkipToken(tr, OPERATOR, ")"))
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			piIndex - 
// Output : 
//-----------------------------------------------------------------------------
GDclass *GameData::ClassForName(const char *pszName, int *piIndex)
{
	int nCount = m_Classes.Count();
	for (int i = 0; i < nCount; i++)
	{
		GDclass *mp = m_Classes.Element(i);
		if(!strcmp(mp->GetName(), pszName))
		{
			if(piIndex)
				piIndex[0] = i;
			return mp;
		}
	}

	return NULL;
}


// These are 'standard' keys that every entity uses, but they aren't specified that way in the .fgd
static const char *RequiredKeys[] =
{
	"Origin",
	"Angles",
	NULL
};

//-----------------------------------------------------------------------------
// Purpose: this function will set up the initial class about to be instanced
// Input  : pszClassName - the class name of the entity to be instanced
//			pszInstancePrefix - the prefix to be used for all name fields
//			Origin - the origin offset of the instance
//			Angles - the angle rotation of the instance
// Output : if successful, will return the game data class of the class name
//-----------------------------------------------------------------------------
GDclass *GameData::BeginInstanceRemap( const char *pszClassName, const char *pszInstancePrefix, Vector &Origin, QAngle &Angle )
{
	m_InstanceOrigin = Origin;
	m_InstanceAngle = Angle;
	AngleMatrix( m_InstanceAngle, m_InstanceOrigin, m_InstanceMat );

	strcpy( m_InstancePrefix, pszInstancePrefix );

	if ( m_InstanceClass )
	{
		delete m_InstanceClass;
		m_InstanceClass = NULL;
	}

	if ( strcmpi( pszClassName, "info_overlay_accessor" ) == 0 )
	{	// yucky hack for a made up entity in the bsp process
		pszClassName = "info_overlay";
	}

	GDclass	*BaseClass = ClassForName( pszClassName );
	if ( BaseClass )
	{
		m_InstanceClass = new GDclass();
		m_InstanceClass->Parent = this;
		m_InstanceClass->AddBase( BaseClass );

		for( int i = 0; RequiredKeys[ i ]; i++ )
		{
			if ( m_InstanceClass->VarForName( RequiredKeys[ i ] ) == NULL )
			{
				BaseClass = ClassForName( RequiredKeys[ i ] );
				if ( BaseClass )
				{
					m_InstanceClass->AddBase( BaseClass );
				}
			}
		}
	}
	else
	{
		m_InstanceClass = NULL;
	}

	return m_InstanceClass;
}


enum tRemapOperation
{
	REMAP_NAME = 0,
	REMAP_POSITION,
	REMAP_ANGLE,
	REMAP_ANGLE_NEGATIVE_PITCH,
};


static CUtlMap< GDIV_TYPE, tRemapOperation > RemapOperation;


//-----------------------------------------------------------------------------
// Purpose: function to sort the class type for the RemapOperations map
// Input  : type1 - the first type to compare against
//			type2 - the second type to compare against
// Output : returns true if the first type is less than the second one
//-----------------------------------------------------------------------------
static bool CUtlType_LessThan( const GDIV_TYPE &type1, const GDIV_TYPE &type2 )
{
	return ( type1 < type2 );
}


//-----------------------------------------------------------------------------
// Purpose: this function will attempt to remap a key's value
// Input  : pszKey - the name of the key
//			pszInvalue - the original value
//			AllowNameRemapping - only do name remapping if this parameter is true.  
//				this is generally only false on the instance level.
// Output : returns true if the value changed
//			pszOutValue - the new value if changed
//-----------------------------------------------------------------------------
bool GameData::RemapKeyValue( const char *pszKey, const char *pszInValue, char *pszOutValue, TNameFixup NameFixup )
{
	if ( RemapOperation.Count() == 0 )
	{
		RemapOperation.SetLessFunc( &CUtlType_LessThan );
		RemapOperation.Insert( ivAngle, REMAP_ANGLE );
		RemapOperation.Insert( ivTargetDest, REMAP_NAME );
		RemapOperation.Insert( ivTargetSrc, REMAP_NAME );
		RemapOperation.Insert( ivOrigin, REMAP_POSITION );
		RemapOperation.Insert( ivAxis, REMAP_ANGLE );
		RemapOperation.Insert( ivAngleNegativePitch, REMAP_ANGLE_NEGATIVE_PITCH );
	}

	if ( !m_InstanceClass )
	{
		return false;
	}

	GDinputvariable *KVVar = m_InstanceClass->VarForName( pszKey );
	if ( !KVVar )
	{
		return false;
	}

	GDIV_TYPE	KVType = KVVar->GetType();
	int			KVRemapIndex = RemapOperation.Find( KVType );
	if ( KVRemapIndex == RemapOperation.InvalidIndex() )
	{
		return false;
	}

	strcpy( pszOutValue, pszInValue );

	switch( RemapOperation[ KVRemapIndex ] )
	{
		case REMAP_NAME:
			if ( KVType != ivInstanceVariable )
			{
				RemapNameField( pszInValue, pszOutValue, NameFixup );
			}
			break;

		case REMAP_POSITION:
			{
				Vector	inPoint( 0.0f, 0.0f, 0.0f ), outPoint;

				sscanf ( pszInValue, "%f %f %f", &inPoint.x, &inPoint.y, &inPoint.z );
				VectorTransform( inPoint, m_InstanceMat, outPoint );
				sprintf( pszOutValue, "%g %g %g", outPoint.x, outPoint.y, outPoint.z );
			}
			break;
			
		case REMAP_ANGLE:
			if ( m_InstanceAngle.x != 0.0f || m_InstanceAngle.y != 0.0f || m_InstanceAngle.z != 0.0f )
			{
				QAngle		inAngles( 0.0f, 0.0f, 0.0f ), outAngles;
				matrix3x4_t angToWorld, localMatrix;

				sscanf ( pszInValue, "%f %f %f", &inAngles.x, &inAngles.y, &inAngles.z );

				AngleMatrix( inAngles, angToWorld );
				MatrixMultiply( m_InstanceMat, angToWorld, localMatrix );
				MatrixAngles( localMatrix, outAngles );

				sprintf( pszOutValue, "%g %g %g", outAngles.x, outAngles.y, outAngles.z );
			}
			break;

		case REMAP_ANGLE_NEGATIVE_PITCH:
			if ( m_InstanceAngle.x != 0.0f || m_InstanceAngle.y != 0.0f || m_InstanceAngle.z != 0.0f )
			{
				QAngle		inAngles( 0.0f, 0.0f, 0.0f ), outAngles;
				matrix3x4_t angToWorld, localMatrix;

				sscanf ( pszInValue, "%f", &inAngles.x );	// just the pitch
				inAngles.x = -inAngles.x;

				AngleMatrix( inAngles, angToWorld );
				MatrixMultiply( m_InstanceMat, angToWorld, localMatrix );
				MatrixAngles( localMatrix, outAngles );

				sprintf( pszOutValue, "%g", -outAngles.x );	// just the pitch
			}
			break;
	}

	return ( strcmpi( pszInValue, pszOutValue ) != 0 );
}


//-----------------------------------------------------------------------------
// Purpose: this function will attempt to remap a name field.
// Input  : pszInvalue - the original value
//			AllowNameRemapping - only do name remapping if this parameter is true.  
//				this is generally only false on the instance level.
// Output : returns true if the value changed
//			pszOutValue - the new value if changed
//-----------------------------------------------------------------------------
bool GameData::RemapNameField( const char *pszInValue, char *pszOutValue, TNameFixup NameFixup )
{
	strcpy( pszOutValue, pszInValue );

	if ( pszInValue[ 0 ] && pszInValue[ 0 ] != '@' )
	{	// ! at the start of a value means it is global and should not be remaped
		switch( NameFixup )
		{
			case NAME_FIXUP_PREFIX:
				sprintf( pszOutValue, "%s-%s", m_InstancePrefix, pszInValue );
				break;

			case NAME_FIXUP_POSTFIX:
				sprintf( pszOutValue, "%s-%s", pszInValue, m_InstancePrefix );
				break;
		}
	}

	return ( strcmpi( pszInValue, pszOutValue ) != 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Gathers any FGD-defined material directory exclusions
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
bool GameData::LoadFGDMaterialExclusions( TokenReader &tr )
{
	if ( !GDSkipToken( tr, OPERATOR, "[" ) )
	{
		return false;
	}
	while ( 1 )
	{
		char szToken[128];
		bool bMatchFound = false;

		if ( tr.PeekTokenType( szToken, sizeof( szToken ) ) == OPERATOR )
		{
			break;
		}
		else if ( GDGetToken( tr, szToken, sizeof( szToken ), STRING ) )
		{		
			// Make sure we haven't loaded this from another FGD
			for ( int i = 0; i < m_FGDMaterialExclusions.Count(); i++ )
			{
				if ( !stricmp( szToken, m_FGDMaterialExclusions[i].szDirectory ) )
				{			
					bMatchFound = true;
					break;
				}
			}

			// Parse the string
			if ( bMatchFound == false )
			{
				int index = m_FGDMaterialExclusions.AddToTail();
				Q_strncpy( m_FGDMaterialExclusions[index].szDirectory, szToken, sizeof( m_FGDMaterialExclusions[index].szDirectory ) );
				m_FGDMaterialExclusions[index].bUserGenerated = false;
			}
		}
	}

	//
	// Closing square brace.
	//
	if ( !GDSkipToken( tr, OPERATOR, "]" ) )
	{
		return( FALSE );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gathers any FGD-defined Auto VisGroups
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
bool GameData::LoadFGDAutoVisGroups( TokenReader &tr )
{
	int gindex = 0; // Index of AutoVisGroups
	int cindex = 0;	// Index of Classes

	char szToken[128];
	
	// Handle the Parent -- World Geometry, Entities, World Detail
	if ( GDSkipToken( tr, OPERATOR, "=" ) )
	{
		// We expect a name
		if ( !GDGetToken( tr, szToken, sizeof( szToken ), STRING ) )
		{
			return( FALSE );
		}
		
		gindex = m_FGDAutoVisGroups.AddToTail();
		Q_strncpy( m_FGDAutoVisGroups[gindex].szParent, szToken, sizeof( m_FGDAutoVisGroups[gindex].szParent ) );

		// We expect a Class
		if ( !GDSkipToken( tr, OPERATOR, "[" ) )
		{
			return( FALSE );
		}
	}

	// Handle the Class(es) -- Brush Entities, Occluders, Lights
	while ( 1 )
	{
		if ( GDGetToken( tr, szToken, sizeof( szToken ), STRING ) )
		{
			cindex = m_FGDAutoVisGroups[gindex].m_Classes.AddToTail();
			Q_strncpy( m_FGDAutoVisGroups[gindex].m_Classes[cindex].szClass, szToken, sizeof( m_FGDAutoVisGroups[gindex].m_Classes[cindex].szClass ) );

			if ( !GDSkipToken( tr, OPERATOR, "[" ) )
			{
				return( FALSE );
			}

			// Parse objects/entities -- func_detail, point_template, light_spot
			while ( 1 )
			{
				if ( tr.PeekTokenType( szToken, sizeof( szToken ) ) == OPERATOR )
				{
					break;
				}

				if ( !GDGetToken( tr, szToken, sizeof( szToken ), STRING ) )
				{
					return( FALSE );
				}

				m_FGDAutoVisGroups[gindex].m_Classes[cindex].szEntities.CopyAndAddToTail( szToken );

			}

			if ( !GDSkipToken( tr, OPERATOR, "]" ) )
			{
				return( FALSE );
			}

			// See if we have another Class coming up
			if ( tr.PeekTokenType( szToken, sizeof( szToken ) ) == STRING )
			{
				continue;
			}

			// If no more Classes, we now expect a terminating ']'
			if ( !GDSkipToken( tr, OPERATOR, "]" ) )
			{
				return( FALSE );
			}

			// We're done
			return true;
		}
		// We don't have another Class; look for a terminating brace
		else
		{
			if ( !GDSkipToken( tr, OPERATOR, "]" ) )
			{
				return( FALSE );
			}
		}
	}

	// Safety net
	GDError( tr, "Malformed AutoVisGroup -- Last processed:  %s", szToken );
	return( FALSE );
}

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgoff.h"
