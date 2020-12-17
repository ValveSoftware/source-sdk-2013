//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mostly just Mapbase's convar mod code.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "saverestore_utlvector.h"
#include "SystemConvarMod.h"
#include "fmtstr.h"

ConVar g_debug_convarmod( "g_debug_convarmod", "0" );

BEGIN_SIMPLE_DATADESC( modifiedconvars_t )
	DEFINE_ARRAY( pszConvar, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
	DEFINE_ARRAY( pszCurrentValue, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
	DEFINE_ARRAY( pszOrgValue, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
END_DATADESC()


// ======================================================================
// 
// Everything below here is for the Mapbase convar mod system.
// ...which is based off of the Commentary convar mod system.
// 
// ======================================================================

#define MAX_CONVARMOD_STRING_SIZE 512

CHandle<CMapbaseCVarModEntity> m_hConvarsChanging;
CMapbaseCVarModEntity *ChangingCVars( void ) { return m_hConvarsChanging.Get(); }
void SetChangingCVars( CMapbaseCVarModEntity *hEnt ) { m_hConvarsChanging = hEnt; }

CUtlVector< CHandle<CMapbaseCVarModEntity> > m_ModEntities;
bool m_bModActive;

void ConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	CMapbaseCVarModEntity *modent = ChangingCVars();

	for ( int i = 0; i < m_ModEntities.Count(); i++ )
	{
		if (!m_ModEntities[i])
		{
			Warning("NULL mod entity at %i\n", i);
			continue;
		}

		if (m_ModEntities[i].Get()->NewCVar(&var, pOldString, modent))
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CV_GlobalChange_Mapbase( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( !ChangingCVars() )
	{
		// A convar has changed, but not due to Mapbase systems. Ignore it.
		return;
	}

	ConvarChanged( var, pOldString, flOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_MapbaseNotChangingCVars( void )
{
	SetChangingCVars( NULL );
}
static ConCommand mapbase_cvarsnotchanging("mapbase_cvarsnotchanging", CC_MapbaseNotChangingCVars, "An internal command used for ConVar modification.", FCVAR_HIDDEN);

// ------------------------------------------------------------------------

void CV_InitMod()
{
	if (!m_bModActive)
	{
		cvar->InstallGlobalChangeCallback( CV_GlobalChange_Mapbase );
		m_bModActive = true;
	}
}

// ------------------------------------------------------------------------

void CVEnt_Precache(CMapbaseCVarModEntity *modent)
{
	// Now protected by FCVAR_NOT_CONNECTED
	//if (Q_strstr(STRING(modent->m_target), "sv_allow_logic_convar"))
	//	return;

#ifdef MAPBASE_MP
	if (gpGlobals->maxClients > 1 && !modent->m_bUseServer)
	{
		Warning("WARNING: %s is using the local player in a multiplayer game and will not function.\n", modent->GetDebugName());
	}
#endif

	CV_InitMod();
}
void CVEnt_Activate(CMapbaseCVarModEntity *modent)
{
	const char *pszCommands = STRING( modent->m_target );
	if ( Q_strnchr(pszCommands, '^', MAX_CONVARMOD_STRING_SIZE) )
	{
		// Just like the commentary system, we convert ^s to "s here.
		char szTmp[MAX_CONVARMOD_STRING_SIZE];
		Q_strncpy( szTmp, pszCommands, MAX_CONVARMOD_STRING_SIZE );
		int len = Q_strlen( szTmp );
		for ( int i = 0; i < len; i++ )
		{
			if ( szTmp[i] == '^' )
			{
				szTmp[i] = '"';
			}
		}

		pszCommands = szTmp;
	}

	CV_InitMod();

	if (m_ModEntities.Find(modent) == m_ModEntities.InvalidIndex())
		m_ModEntities.AddToTail( modent );

	if (modent->m_bUseServer)
	{
		SetChangingCVars( modent );

		engine->ServerCommand( CFmtStr("%s\n", pszCommands) );
		engine->ServerCommand( "mapbase_cvarsnotchanging\n" );
	}
	else
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		edict_t *edict = pPlayer ? pPlayer->edict() : NULL;
		if (edict)
		{
			SetChangingCVars( modent );

			engine->ClientCommand( edict, pszCommands );
			engine->ClientCommand( edict, "mapbase_cvarsnotchanging" );
		}
		else
		{
			Warning("%s unable to find local player edict\n", modent->GetDebugName());
		}
	}
}
void CVEnt_Deactivate(CMapbaseCVarModEntity *modent)
{
	// Remove our global convar callback
	cvar->RemoveGlobalChangeCallback( CV_GlobalChange_Mapbase );

	// Reset any convars that have been changed by the commentary
	for ( int i = 0; i < modent->m_ModifiedConvars.Count(); i++ )
	{
		ConVar *pConVar = (ConVar *)cvar->FindVar( modent->m_ModifiedConvars[i].pszConvar );
		if ( pConVar )
		{
			pConVar->SetValue( modent->m_ModifiedConvars[i].pszOrgValue );
		}
	}

	modent->m_ModifiedConvars.Purge();

	if (m_bModActive)
	{
		m_ModEntities.FindAndRemove(modent);

		if (m_ModEntities.Count() == 0)
		{
			// No more mod entities
			m_bModActive = false;
		}
		else
		{
			// We're done and we're still active, install our callback again
			cvar->InstallGlobalChangeCallback( CV_GlobalChange_Mapbase );
		}
	}
}
void CVEnt_Restore(CMapbaseCVarModEntity *modent)
{
	m_ModEntities.AddToTail(modent);
}

// ------------------------------------------------------------------------


LINK_ENTITY_TO_CLASS( game_convar_mod, CMapbaseCVarModEntity );

// This classname should be phased out after beta
LINK_ENTITY_TO_CLASS( mapbase_convar_mod, CMapbaseCVarModEntity );

BEGIN_DATADESC( CMapbaseCVarModEntity )

	DEFINE_UTLVECTOR( m_ModifiedConvars, FIELD_EMBEDDED ),
	DEFINE_KEYFIELD( m_bUseServer, FIELD_BOOLEAN, "UseServer" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),

	DEFINE_THINKFUNC( CvarModActivate ),

END_DATADESC()


void CMapbaseCVarModEntity::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	CVEnt_Deactivate(this);
}

void CMapbaseCVarModEntity::Precache( void )
{
	BaseClass::Precache();
	
	CVEnt_Precache(this);
}

void CMapbaseCVarModEntity::Spawn( void )
{
	BaseClass::Spawn();

	if (m_target == NULL_STRING)
	{
		Warning("WARNING: %s has no cvars specified! Removing...\n", GetDebugName());
		UTIL_Remove(this);
		return;
	}

	Precache();

	if (HasSpawnFlags(SF_CVARMOD_START_ACTIVATED))
	{
		if (!m_bUseServer && !UTIL_GetLocalPlayer())
		{
			// The local player doesn't exist yet, so we should wait until they do
			SetThink( &CMapbaseCVarModEntity::CvarModActivate );
			SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
		}
		else
		{
			CVEnt_Activate( this );
		}
	}
}

void CMapbaseCVarModEntity::CvarModActivate()
{
	if (UTIL_GetLocalPlayer())
	{
		CVEnt_Activate( this );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}
}

void CMapbaseCVarModEntity::OnRestore( void )
{
	BaseClass::OnRestore();

	// Set any convars that have already been changed by the mapper before the save
	if (m_ModifiedConvars.Count() > 0)
	{
		for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
		{
			ConVar *pConVar = (ConVar *)cvar->FindVar( m_ModifiedConvars[i].pszConvar );
			if ( pConVar )
			{
				if (g_debug_convarmod.GetBool())
					Msg("    %s Restoring Convar %s: value %s (org %s)\n", GetDebugName(), m_ModifiedConvars[i].pszConvar, m_ModifiedConvars[i].pszCurrentValue, m_ModifiedConvars[i].pszOrgValue );
				pConVar->SetValue( m_ModifiedConvars[i].pszCurrentValue );
			}
		}

		CVEnt_Restore(this);
	}
}

void CMapbaseCVarModEntity::InputActivate(inputdata_t &inputdata)
{
	CVEnt_Activate(this);
}

void CMapbaseCVarModEntity::InputDeactivate(inputdata_t &inputdata)
{
	CVEnt_Deactivate(this);
}

bool CMapbaseCVarModEntity::NewCVar( ConVarRef *var, const char *pOldString, CBaseEntity *modent )
{
	for (int i = 0; i < m_ModifiedConvars.Count(); i++)
	{
		// If we find it, just update the current value
		modifiedconvars_t modvar = m_ModifiedConvars[i];
		if ( !Q_strncmp( var->GetName(), modvar.pszConvar, MAX_MODIFIED_CONVAR_STRING ) )
		{
			if (modent == this)
			{
				Q_strncpy( modvar.pszCurrentValue, var->GetString(), MAX_MODIFIED_CONVAR_STRING );
				if (g_debug_convarmod.GetBool())
					Msg("    %s Updating Convar %s: value %s (org %s)\n", GetDebugName(), modvar.pszConvar, modvar.pszCurrentValue, modvar.pszOrgValue );
				return true;
			}
			else
			{
				// A different entity is using this CVar now, remove ours
				m_ModifiedConvars.Remove(i);
				if (g_debug_convarmod.GetBool())
					Msg("    %s Removing Convar %s: value %s (org %s)\n", GetDebugName(), modvar.pszConvar, modvar.pszCurrentValue, modvar.pszOrgValue );
				return false;
			}
		}
	}

	// Did I do that?
	if (modent == this)
	{
		// Add the CVar to our list.
		modifiedconvars_t newConvar;
		Q_strncpy( newConvar.pszConvar, var->GetName(), MAX_MODIFIED_CONVAR_STRING );
		Q_strncpy( newConvar.pszCurrentValue, var->GetString(), MAX_MODIFIED_CONVAR_STRING );
		Q_strncpy( newConvar.pszOrgValue, pOldString, MAX_MODIFIED_CONVAR_STRING );
		m_ModifiedConvars.AddToTail( newConvar );

		if (g_debug_convarmod.GetBool())
		{
			Msg(" %s changed '%s' to '%s' (was '%s')\n", GetDebugName(), var->GetName(), var->GetString(), pOldString );
			Msg(" Convars stored: %d\n", m_ModifiedConvars.Count() );
			for ( int i = 0; i < m_ModifiedConvars.Count(); i++ )
			{
				Msg("    Convar %d: %s, value %s (org %s)\n", i, m_ModifiedConvars[i].pszConvar, m_ModifiedConvars[i].pszCurrentValue, m_ModifiedConvars[i].pszOrgValue );
			}
		}

		return true;
	}

	return false;
}
