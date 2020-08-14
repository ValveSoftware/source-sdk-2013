//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: The flimsy MapEdit system that was
//			heavily inspired by Synergy's MapEdit, completely based on the Commentary System
//			and originally used for Lambda Fortress.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "MapEdit.h"
#include "filesystem.h"

#ifndef CLIENT_DLL
#include <KeyValues.h>
#include "utldict.h"
#include "isaverestore.h"
#include "eventqueue.h"
#include "saverestore_utlvector.h"
#include "ai_basenpc.h"
#include "triggers.h"
#include "mapbase/SystemConvarMod.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL

#define MAPEDIT_SPAWNED_SEMAPHORE		"mapedit_semaphore"
#define MAPEDIT_DEFAULT_FILE		UTIL_VarArgs("maps/%s_auto.txt", STRING(gpGlobals->mapname))

ConVar mapedit_enabled("mapedit_enabled", "1", FCVAR_ARCHIVE, "Is automatic MapEdit enabled?");
ConVar mapedit_stack("mapedit_stack", "1", FCVAR_ARCHIVE, "If multiple MapEdit scripts are loaded, should they stack or replace each other?");
ConVar mapedit_debug("mapedit_debug", "0", FCVAR_NONE, "Should MapEdit give debug messages?");

inline void DebugMsg(const tchar *pMsg, ...)
{
	if (mapedit_debug.GetBool() == true)
	{
		Msg("%s", pMsg);
	}
}

//bool g_bMapEditAvailable;
bool g_bMapEditLoaded = false;
bool IsMapEditLoaded( void )
{
	return g_bMapEditLoaded;
}

bool IsAutoMapEditAllowed(void)
{
	return mapedit_enabled.GetBool();
}

void CV_GlobalChange_MapEdit( IConVar *var, const char *pOldString, float flOldValue );

//-----------------------------------------------------------------------------
// Purpose: Game system for MapEdit stuff
//-----------------------------------------------------------------------------
class CMapEdit : public CAutoGameSystemPerFrame
{
public:
	DECLARE_DATADESC();

	virtual void LevelInitPreEntity()
	{
		m_bMapEditConvarsChanging = false;
		CalculateAvailableState();
	}

	void CalculateAvailableState( void )
	{
		// Set the available cvar if we can find commentary data for this level
		char szFullName[512];
		Q_snprintf(szFullName,sizeof(szFullName), "maps/%s_auto.txt", STRING( gpGlobals->mapname) );
		if ( filesystem->FileExists( szFullName ) )
		{
			bool bAllowed = IsAutoMapEditAllowed();
			g_bMapEditLoaded = bAllowed;
			//if (bAllowed)
			//	gEntList.AddListenerEntity( this );
		}
		else
		{
			g_bMapEditLoaded = false;
			//gEntList.RemoveListenerEntity( this );
		}
	}

	virtual void LevelShutdownPreEntity()
	{
		ShutDownMapEdit();
	}

	void ParseEntKVBlock( CBaseEntity *pNode, KeyValues *pkvNode )
	{
		KeyValues *pkvNodeData = pkvNode->GetFirstSubKey();
		while ( pkvNodeData )
		{
			// Handle the connections block
			if ( !Q_strcmp(pkvNodeData->GetName(), "connections") )
			{
				ParseEntKVBlock( pNode, pkvNodeData );
			}
			else
			{ 
				#define MAPEDIT_STRING_LENGTH_MAX		1024

				const char *pszValue = pkvNodeData->GetString();
				Assert( Q_strlen(pszValue) < MAPEDIT_STRING_LENGTH_MAX );
				if ( Q_strnchr(pszValue, '^', MAPEDIT_STRING_LENGTH_MAX) )
				{
					// We want to support quotes in our strings so that we can specify multiple parameters in
					// an output inside our commentary files. We convert ^s to "s here.
					char szTmp[MAPEDIT_STRING_LENGTH_MAX];
					Q_strncpy( szTmp, pszValue, MAPEDIT_STRING_LENGTH_MAX );
					int len = Q_strlen( szTmp );
					for ( int i = 0; i < len; i++ )
					{
						if ( szTmp[i] == '^' )
						{
							szTmp[i] = '"';
						}
					}

					pszValue = szTmp;
				}

				char cOperatorChar = pszValue[0];
				if (cOperatorChar == '+')
				{
					pszValue++;
					char szExistingValue[MAPEDIT_STRING_LENGTH_MAX];
					if (pNode->GetKeyValue(pkvNodeData->GetName(), szExistingValue, sizeof(szExistingValue) ))
					{
						// Right now, this only supports adding floats/integers.
						// Add Vector support later.
						float flResult = atof(szExistingValue) + atof(pszValue);
						pszValue = UTIL_VarArgs("%f", flResult);
					}
				}
				else if (cOperatorChar == '-')
				{
					pszValue++;
					char szExistingValue[MAPEDIT_STRING_LENGTH_MAX];
					if (pNode->GetKeyValue(pkvNodeData->GetName(), szExistingValue, sizeof(szExistingValue) ))
					{
						// Right now, this only supports subtracting floats/integers.
						// Add Vector support later.
						float flResult = atof(szExistingValue) - atof(pszValue);
						pszValue = UTIL_VarArgs("%f", flResult);
					}
				}
				else if (cOperatorChar == '|' /*&& pszValue[1] == '='*/)
				{
					pszValue++;
					char szExistingValue[MAPEDIT_STRING_LENGTH_MAX];
					if (pNode->GetKeyValue(pkvNodeData->GetName(), szExistingValue, sizeof(szExistingValue)))
					{
						int iResult = atoi(szExistingValue) | atoi(pszValue);
						pszValue = UTIL_VarArgs("%i", iResult);
					}
				}

				pNode->KeyValue(pkvNodeData->GetName(), pszValue);
			}

			pkvNodeData = pkvNodeData->GetNextKey();
		}
	}

	virtual void LevelInitPostEntity( void )
	{
		if ( !IsMapEditLoaded() )
		{
			return;
		}

		if ( gpGlobals->eLoadType == MapLoad_LoadGame || gpGlobals->eLoadType == MapLoad_Background )
		{
			return;
		}

		m_bMapEditLoadedMidGame = false;
		InitMapEdit();

		//IGameEvent *event = gameeventmanager->CreateEvent( "playing_mapedit" );
		//gameeventmanager->FireEventClientSide( event );
	}

	bool MapEditConvarsChanging( void )
	{
		return m_bMapEditConvarsChanging;
	}

	void SetMapEditConvarsChanging( bool bChanging )
	{
		m_bMapEditConvarsChanging = bChanging;
	}

	void ConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
	{
		ConVarRef var( pConVar );

		// A convar has been changed by a commentary node. We need to store
		// the old state. If the engine shuts down, we need to restore any
		// convars that the commentary changed to their previous values.
		for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
		{
			// If we find it, just update the current value
			if ( !Q_strncmp( var.GetName(), m_ModifiedConvars[i].pszConvar, MAX_MODIFIED_CONVAR_STRING ) )
			{
				Q_strncpy( m_ModifiedConvars[i].pszCurrentValue, var.GetString(), MAX_MODIFIED_CONVAR_STRING );
				//Msg("    Updating Convar %s: value %s (org %s)\n", m_ModifiedConvars[i].pszConvar, m_ModifiedConvars[i].pszCurrentValue, m_ModifiedConvars[i].pszOrgValue );
				return;
			}
		}

		// We didn't find it in our list, so add it
		modifiedconvars_t newConvar;
		Q_strncpy( newConvar.pszConvar, var.GetName(), MAX_MODIFIED_CONVAR_STRING );
		Q_strncpy( newConvar.pszCurrentValue, var.GetString(), MAX_MODIFIED_CONVAR_STRING );
		Q_strncpy( newConvar.pszOrgValue, pOldString, MAX_MODIFIED_CONVAR_STRING );
		m_ModifiedConvars.AddToTail( newConvar );

		/*
		Msg(" Commentary changed '%s' to '%s' (was '%s')\n", var->GetName(), var->GetString(), pOldString );
		Msg(" Convars stored: %d\n", m_ModifiedConvars.Count() );
		for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
		{
			Msg("    Convar %d: %s, value %s (org %s)\n", i, m_ModifiedConvars[i].pszConvar, m_ModifiedConvars[i].pszCurrentValue, m_ModifiedConvars[i].pszOrgValue );
		}
		*/
	}

	CBaseEntity *FindMapEditEntity( CBaseEntity *pStartEntity, const char *szName, const char *szValue = NULL )
	{
		CBaseEntity *pEntity = NULL;
		DebugMsg("MapEdit Find Debug: Starting Search, Name: %s, Value: %s\n", szName, szValue);

		// First, find by targetname/classname
		pEntity = gEntList.FindEntityGeneric(pStartEntity, szName);

		if (!pEntity)
		{
			DebugMsg("MapEdit Find Debug: \"%s\" not found as targetname or classname\n", szName);

			if (szValue)
			{
				if (!Q_strnicmp(szName, "#find_", 6))
				{
					const char *pName = szName + 6;
					if (!Q_stricmp(pName, "by_keyfield"))
					{
						char key[64];
						char value[64];

						// Separate key from value
						char *delimiter = Q_strstr(szValue, " ");
						if (delimiter)
						{
							Q_strncpy(key, szValue, MIN((delimiter - szValue) + 1, sizeof(key)));
							Q_strncpy(value, delimiter + 1, sizeof(value));
						}
						else
						{
							// Assume the value doesn't matter and we're just looking for the key
							Q_strncpy(key, szValue, sizeof(key));
						}

						if (!key)
						{
							Warning("MapEdit: Possible find_by_keyfield syntax error: key not detected in \"%s\"\n", szValue);
						}

						// Find entities with matching keyfield
						variant_t variant;
						const CEntInfo *pInfo = pStartEntity ? gEntList.GetEntInfoPtr(pStartEntity->GetRefEHandle())->m_pNext : gEntList.FirstEntInfo();
						for (; pInfo; pInfo = pInfo->m_pNext)
						{
							CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
							if (!ent)
							{
								DevWarning("NULL entity in global entity list!\n");
								continue;
							}

							if (!ent->edict())
								continue;

							if (ent->ReadKeyField(key, &variant))
							{
								// Does the value matter?
								if (value)
								{
									if (Q_stricmp(variant.String(), value) == 0)
									{
										// The entity has the keyfield and it matches the value.
										return ent;
									}
								}
								else
								{
									// The value doesn't matter and the entity has the keyfield.
									return ent;
								}
							}
						}
					}
					else if (!Q_stricmp(pName, "by_origin"))
					{
						// Find entities at this origin
						Vector vecOrigin;
						UTIL_StringToVector(vecOrigin.Base(), szValue);
						if (vecOrigin.IsValid())
						{
							DebugMsg("MapEdit Find Debug: \"%s\" is valid vector\n", szValue);
							const CEntInfo *pInfo = pStartEntity ? gEntList.GetEntInfoPtr(pStartEntity->GetRefEHandle())->m_pNext : gEntList.FirstEntInfo();
							for (; pInfo; pInfo = pInfo->m_pNext)
							{
								CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
								if (!ent)
								{
									DevWarning("NULL entity in global entity list!\n");
									continue;
								}

								if (!ent->edict())
									continue;

								if (ent->GetLocalOrigin() == vecOrigin)
									return ent;
							}
						}
					}
				}
				else if (!pStartEntity)
				{
					// Try the entity index
					int iEntIndex = atoi(szName);
					if (!pStartEntity && UTIL_EntityByIndex(iEntIndex))
					{
						DebugMsg("MapEdit Find Debug: \"%s\" is valid index\n", szName);
						return CBaseEntity::Instance(iEntIndex);
					}
				}
			}

			DebugMsg("MapEdit Find Debug: \"%s\" not found\n", szName);
			return NULL;
		}

		DebugMsg("MapEdit Find Debug: \"%s\" found as targetname or classname\n", szName);
		return pEntity;
	}

	void InitMapEdit( const char* pFile = MAPEDIT_DEFAULT_FILE )
	{
		// Install the global cvar callback
		cvar->InstallGlobalChangeCallback( CV_GlobalChange_MapEdit );


		// If we find the commentary semaphore, the commentary entities already exist.
		// This occurs when you transition back to a map that has saved commentary nodes in it.
		if ( gEntList.FindEntityByName( NULL, MAPEDIT_SPAWNED_SEMAPHORE ) )
			return;

		// Spawn the commentary semaphore entity
		CBaseEntity *pSemaphore = CreateEntityByName( "info_target" );
		pSemaphore->SetName( MAKE_STRING(MAPEDIT_SPAWNED_SEMAPHORE) );

		bool oldLock = engine->LockNetworkStringTables( false );

		SpawnMapEdit(pFile);

		engine->LockNetworkStringTables( oldLock );
	}

	void LoadFromFormat_Original(KeyValues *pkvFile)
	{
		KeyValues *pkvNode = pkvFile->GetFirstSubKey();
		while ( pkvNode )
		{
			const char *pNodeName = pkvNode->GetName();
			if (FStrEq(pNodeName, "create"))
			{
				KeyValues *pkvClassname = pkvNode->GetFirstSubKey();
				while (pkvClassname)
				{
					pNodeName = pkvClassname->GetName();

					CBaseEntity *pNode = CreateEntityByName(pNodeName);
					if (pNode)
					{
						ParseEntKVBlock(pNode, pkvClassname);

						EHANDLE hHandle;
						hHandle = pNode;
						m_hSpawnedEntities.AddToTail(hHandle);
						DebugMsg("MapEdit Debug: Spawned entity %s\n", pNodeName);
					}
					else
					{
						Warning("MapEdit: Failed to spawn mapedit entity, type: '%s'\n", pNodeName);
					}

					pkvClassname = pkvClassname->GetNextKey();
				}
				pkvClassname->deleteThis();
			}
			else if (FStrEq(pNodeName, "edit"))
			{
				KeyValues *pName = pkvNode->GetFirstSubKey();
				while (pName)
				{
					pNodeName = pName->GetName();

					CBaseEntity *pNode = NULL;

					pNode = FindMapEditEntity(NULL, pNodeName, pName->GetString());

					while (pNode)
					{
						DebugMsg("MapEdit Debug: Editing %s (%s)\n", pNodeName, pNode->GetDebugName());

						ParseEntKVBlock(pNode, pName);
						pNode = FindMapEditEntity(pNode, pNodeName, pName->GetString());
					}

					pName = pName->GetNextKey();
				}
				pName->deleteThis();
			}
			else if (FStrEq(pNodeName, "delete"))
			{
				KeyValues *pName = pkvNode->GetFirstSubKey();
				while (pName)
				{
					pNodeName = pName->GetName();

					CBaseEntity *pNode = NULL;

					pNode = FindMapEditEntity(NULL, pNodeName, pName->GetString());

					while (pNode)
					{
						DebugMsg("MapEdit Debug: Deleting %s (%s)\n", pNodeName, pNode->GetDebugName());

						UTIL_Remove(pNode);
						pNode = FindMapEditEntity(pNode, pNodeName, pName->GetString());
					}

					pName = pName->GetNextKey();
				}
				pName->deleteThis();
			}
			else if (FStrEq(pNodeName, "fire"))
			{
				KeyValues *pName = pkvNode->GetFirstSubKey();
				while (pName)
				{
					pNodeName = pName->GetName();

					const char *pInputName = NULL;
					variant_t varInputParam;
					float flInputDelay = 0.0f;
					CBaseEntity *pActivator = NULL;
					CBaseEntity *pCaller = NULL;
					int iOutputID = 0;

					char *pszValue = strdup(pName->GetString());
					int iter = 0;
					char *inputparams = strtok(pszValue, ",");
					while (inputparams)
					{
						switch (iter)
						{
							// Input name
							case 0:
								pInputName = inputparams; break;
							// Input parameter
							case 1:
								varInputParam.SetString(AllocPooledString(inputparams)); break;
							// Input delay
							case 2:
								flInputDelay = atof(inputparams); break;
							// Activator
							case 3:
								pActivator = gEntList.FindEntityByName(NULL, inputparams); break;
							// Caller
							case 4:
								pCaller = gEntList.FindEntityByName(NULL, inputparams); break;
							// Output ID
							case 5:
								iOutputID = atoi(inputparams); break;
						}
						iter++;
						inputparams = strtok(NULL, ",");
					}

					DebugMsg("MapEdit Debug: Firing input %s on %s\n", pInputName, pNodeName);
					g_EventQueue.AddEvent(pNodeName, pInputName, varInputParam, flInputDelay, pActivator, pCaller, iOutputID);

					pName = pName->GetNextKey();
				}
			}
			else if (FStrEq(pNodeName, "console"))
			{
				KeyValues *pkvNodeData = pkvNode->GetFirstSubKey();
				const char *pKey;
				const char *pValue;
				while (pkvNodeData)
				{
					SetMapEditConvarsChanging(true);

					pKey = pkvNodeData->GetName();
					pValue = pkvNodeData->GetString();

					engine->ServerCommand(UTIL_VarArgs("%s %s", pKey, pValue));
					engine->ServerCommand("mapedit_cvarsnotchanging\n");

					pkvNodeData = pkvNodeData->GetNextKey();
				}
				pkvNodeData->deleteThis();
			}

			pkvNode = pkvNode->GetNextKey();
		}
		pkvNode->deleteThis();
	}

	void SpawnMapEdit(const char *pFile = NULL)
	{
		// Find the commentary file
		char szFullName[512];
		if (pFile == NULL)
		{
			DebugMsg("MapEdit Debug: NULL file, loading default\n");
			Q_snprintf(szFullName,sizeof(szFullName), "maps/%s_auto.txt", STRING( gpGlobals->mapname ));
		}
		else
		{
			DebugMsg("MapEdit Debug: File not NULL, loading %s\n", pFile);
			Q_strncpy(szFullName, pFile, sizeof(szFullName));
		}
		KeyValues *pkvFile = new KeyValues( "MapEdit" );
		if ( pkvFile->LoadFromFile( filesystem, szFullName, "MOD" ) )
		{
			Msg( "MapEdit: Loading MapEdit data from %s. \n", szFullName );

			if (gpGlobals->eLoadType != MapLoad_LoadGame)
			{
				// Support for multiple formats
				const char *szVersion = pkvFile->GetString("version", "original");
				if (FStrEq(szVersion, "original"))
					LoadFromFormat_Original(pkvFile);
				// TODO: More formats
			}

			// Then activate all the entities
			for ( int i = 0; i < m_hSpawnedEntities.Count(); i++ )
			{
				DispatchSpawn(m_hSpawnedEntities[i]);
			}
		}
		else
		{
			Msg( "MapEdit: Could not find MapEdit data file '%s'. \n", szFullName );
		}

		pkvFile->deleteThis();
	}

	void ShutDownMapEdit( void )
	{
		// Destroy all the entities created by commentary
		for ( int i = m_hSpawnedEntities.Count()-1; i >= 0; i-- )
		{
			if ( m_hSpawnedEntities[i] )
			{
				UTIL_Remove( m_hSpawnedEntities[i] );
			}
		}
		m_hSpawnedEntities.Purge();

		// Remove the semaphore
		CBaseEntity *pSemaphore = gEntList.FindEntityByName( NULL, MAPEDIT_SPAWNED_SEMAPHORE );
		if ( pSemaphore )
		{
			UTIL_Remove( pSemaphore );
		}

		// Remove our global convar callback
		cvar->RemoveGlobalChangeCallback( CV_GlobalChange_MapEdit );

		// Reset any convars that have been changed by the commentary
		for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
		{
			ConVar *pConVar = (ConVar *)cvar->FindVar( m_ModifiedConvars[i].pszConvar );
			if ( pConVar )
			{
				pConVar->SetValue( m_ModifiedConvars[i].pszOrgValue );
			}
		}
		m_ModifiedConvars.Purge();
	}

	void SetMapEdit( bool bMapEdit, const char *pFile = NULL )
	{
		//g_bMapEditLoaded = bMapEdit;
		//CalculateAvailableState();

		// If we're turning on commentary, create all the entities.
		if ( bMapEdit )
		{
			if (filesystem->FileExists(pFile) || pFile == NULL)
			{
				g_bMapEditLoaded = true;
				m_bMapEditLoadedMidGame = true;
				InitMapEdit(pFile);
			}
			else
			{
				Warning("MapEdit: No such file \"%s\"!\n", pFile);
			}
		}
		else
		{
			ShutDownMapEdit();
		}
	}

	void OnRestore( void )
	{
		cvar->RemoveGlobalChangeCallback( CV_GlobalChange_MapEdit );

		if ( !IsMapEditLoaded() )
			return;

		// Set any convars that have already been changed by the commentary before the save
		for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
		{
			ConVar *pConVar = (ConVar *)cvar->FindVar( m_ModifiedConvars[i].pszConvar );
			if ( pConVar )
			{
				//Msg("    Restoring Convar %s: value %s (org %s)\n", m_ModifiedConvars[i].pszConvar, m_ModifiedConvars[i].pszCurrentValue, m_ModifiedConvars[i].pszOrgValue );
				pConVar->SetValue( m_ModifiedConvars[i].pszCurrentValue );
			}
		}

		// Install the global cvar callback
		cvar->InstallGlobalChangeCallback( CV_GlobalChange_MapEdit );
	}

	bool MapEditLoadedMidGame( void ) 
	{
		return m_bMapEditLoadedMidGame;
	}

private:
	bool	m_bMapEditConvarsChanging;
	bool	m_bMapEditLoadedMidGame;

	CUtlVector< modifiedconvars_t > m_ModifiedConvars;
	CUtlVector<EHANDLE>				m_hSpawnedEntities;
};

CMapEdit	g_MapEdit;

BEGIN_DATADESC_NO_BASE( CMapEdit )

	DEFINE_FIELD( m_bMapEditLoadedMidGame, FIELD_BOOLEAN ),

	DEFINE_UTLVECTOR( m_ModifiedConvars, FIELD_EMBEDDED ),
	DEFINE_UTLVECTOR( m_hSpawnedEntities, FIELD_EHANDLE ),

	//DEFINE_UTLVECTOR( m_SpawnEditLookup, FIELD_EMBEDDED ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: We need to revert back any convar changes that are made by the
//			commentary system during commentary. This code stores convar changes
//			made by the commentary system, and reverts them when finished.
//-----------------------------------------------------------------------------
void CV_GlobalChange_MapEdit( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( !g_MapEdit.MapEditConvarsChanging() )
	{
		// A convar has changed, but not due to mapedit. Ignore it.
		return;
	}

	g_MapEdit.ConvarChanged( var, pOldString, flOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_MapEditNotChanging( void )
{
	g_MapEdit.SetMapEditConvarsChanging( false );
}
static ConCommand mapedit_cvarsnotchanging( "mapedit_cvarsnotchanging", CC_MapEditNotChanging, 0 );

// ========================================================
// Static functions that can be accessed from outside
// ========================================================

//-----------------------------------------------------------------------------
// Purpose: Reloads automatic MapEdit after cleanup
//-----------------------------------------------------------------------------
void MapEdit_MapReload( void )
{
	Msg("MapEdit: Map reloading\n");

	g_MapEdit.ShutDownMapEdit();

	g_MapEdit.LevelInitPreEntity();

	g_MapEdit.LevelInitPostEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Loads a specific MapEdit file.
//-----------------------------------------------------------------------------
void MapEdit_LoadFile(const char *pFile, bool bStack)
{
	if (!filesystem->FileExists(pFile))
	{
		Warning("MapEdit: No such file \"%s\"!\n", pFile);
		return;
	}
	
	if (IsMapEditLoaded())
	{
		if (bStack)
		{
			g_MapEdit.SpawnMapEdit(pFile);
			return;
		}
		else
		{
			g_MapEdit.SetMapEdit(false);
		}
	}

	g_MapEdit.SetMapEdit(true, pFile);
}


//-----------------------------------------------------------------------------
// Purpose: MapEdit specific logic_auto replacement.
//			Fires outputs based upon how MapEdit has been activated.
//-----------------------------------------------------------------------------
class CMapEditAuto : public CBaseEntity
{
	DECLARE_CLASS( CMapEditAuto, CBaseEntity );
public:
	DECLARE_DATADESC();

	void Spawn(void);
	void Think(void);

private:
	// fired if loaded due to new map
	COutputEvent m_OnMapEditNewGame;

	// fired if loaded in the middle of a map
	COutputEvent m_OnMapEditMidGame;
};

LINK_ENTITY_TO_CLASS(mapedit_auto, CMapEditAuto);

BEGIN_DATADESC( CMapEditAuto )
	// Outputs
	DEFINE_OUTPUT(m_OnMapEditNewGame, "OnMapEditNewGame"),
	DEFINE_OUTPUT(m_OnMapEditMidGame, "OnMapEditMidGame"),
END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Fire my outputs here if I fire on map reload
//------------------------------------------------------------------------------
void CMapEditAuto::Spawn(void)
{
	BaseClass::Spawn();
	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapEditAuto::Think(void)
{
	if ( g_MapEdit.MapEditLoadedMidGame() )
	{
		m_OnMapEditMidGame.FireOutput(NULL, this);
	}
	else
	{
		m_OnMapEditNewGame.FireOutput(NULL, this);
	}
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_MapEdit_Load( const CCommand& args )
{
	const char *sFile = args[1] ? args[1] : NULL;

	MapEdit_LoadFile(sFile, mapedit_stack.GetBool());
}
static ConCommand mapedit_load("mapedit_load", CC_MapEdit_Load, "Forces mapedit to load a specific file. If there is no input value, it will load the map's default file.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : Unloads all MapEdit entities. Does not undo modifications or deletions.
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_MapEdit_Unload( const CCommand& args )
{
	g_MapEdit.SetMapEdit(false);
}
static ConCommand mapedit_unload("mapedit_unload", CC_MapEdit_Unload, "Forces mapedit to unload.", FCVAR_CHEAT);

#else

//------------------------------------------------------------------------------
// Purpose : Prints MapEdit data from a specific file to the console.
// Input   : The file to print
// Output  : The file's data
//------------------------------------------------------------------------------
void CC_MapEdit_Print( const CCommand& args )
{
	const char *szFullName = args[1];
	if (szFullName && filesystem->FileExists(szFullName))
	{
		KeyValues *pkvFile = new KeyValues( "MapEdit" );
		if ( pkvFile->LoadFromFile( filesystem, szFullName, "MOD" ) )
		{
			Msg( "MapEdit: Printing MapEdit data from %s. \n", szFullName );

			// Load each commentary block, and spawn the entities
			KeyValues *pkvNode = pkvFile->GetFirstSubKey();
			while ( pkvNode )
			{
				// Get node name
				const char *pNodeName = pkvNode->GetName();
				Msg("- Section Name: %s\n", pNodeName);

				// Skip the trackinfo
				if ( !Q_strncmp( pNodeName, "trackinfo", 9 ) )
				{
					pkvNode = pkvNode->GetNextKey();
					continue;
				}

				KeyValues *pClassname = pkvNode->GetFirstSubKey();
				while (pClassname)
				{
					// Use the classname instead
					pNodeName = pClassname->GetName();

					Msg("     %s\n", pNodeName);
					for ( KeyValues *sub = pClassname->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
					{
						if (!Q_strcmp(sub->GetName(), "connections"))
						{
							Msg("-         connections\n");
							for ( KeyValues *sub2 = sub->GetFirstSubKey(); sub2; sub2 = sub2->GetNextKey() )
							{
								Msg("-              \"%s\", \"%s\"\n", sub2->GetName(), sub2->GetString());
							}
							continue;
						}
						Msg("-         %s, %s\n", sub->GetName(), sub->GetString());
					}

					pClassname = pClassname->GetNextKey();
				}
				
				pkvNode = pkvNode->GetNextKey();
			}

			pkvNode->deleteThis();
		}
	}
}
static ConCommand mapedit_print("mapedit_print", CC_MapEdit_Print, "Prints a mapedit file in the console.");

#endif
