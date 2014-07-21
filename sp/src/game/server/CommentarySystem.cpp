//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The system for handling director's commentary style production info in-game.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifndef _XBOX
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "utldict.h"
#include "isaverestore.h"
#include "eventqueue.h"
#include "saverestore_utlvector.h"
#include "gamestats.h"
#include "ai_basenpc.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static bool g_bTracingVsCommentaryNodes = false;
static const char *s_pCommentaryUpdateViewThink = "CommentaryUpdateViewThink";

#define COMMENTARY_SPAWNED_SEMAPHORE		"commentary_semaphore"

extern ConVar commentary;
ConVar commentary_available("commentary_available", "0", FCVAR_NONE, "Automatically set by the game when a commentary file is available for the current map." );

enum teleport_stages_t
{
	TELEPORT_NONE,
	TELEPORT_FADEOUT,
	TELEPORT_TELEPORT,
	TELEPORT_FADEIN,
};

// Convar restoration save/restore
#define MAX_MODIFIED_CONVAR_STRING		128
struct modifiedconvars_t 
{
	DECLARE_SIMPLE_DATADESC();

	char pszConvar[MAX_MODIFIED_CONVAR_STRING];
	char pszCurrentValue[MAX_MODIFIED_CONVAR_STRING];
	char pszOrgValue[MAX_MODIFIED_CONVAR_STRING];
};

bool g_bInCommentaryMode = false;
bool IsInCommentaryMode( void )
{
	return g_bInCommentaryMode;
}

//-----------------------------------------------------------------------------
// Purpose: An entity that marks a spot for a piece of commentary
//-----------------------------------------------------------------------------
class CPointCommentaryNode : public CBaseAnimating
{
	DECLARE_CLASS( CPointCommentaryNode, CBaseAnimating );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void Spawn( void );
	void Precache( void );
	void Activate( void );
	void SpinThink( void );
	void StartCommentary( void );
	void FinishCommentary( bool bBlendOut = true );
	void CleanupPostCommentary( void );
	void UpdateViewThink( void );
	void UpdateViewPostThink( void );
	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	bool HasViewTarget( void ) { return (m_hViewTarget != NULL || m_hViewPosition.Get() != NULL); }
	bool PreventsMovement( void );
	bool CannotBeStopped( void ) { return (m_bUnstoppable || m_bPreventChangesWhileMoving); }
	int  UpdateTransmitState( void );
	void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	void SetDisabled( bool bDisabled );
	void SetNodeNumber( int iCount ) { m_iNodeNumber = iCount; }

	// Called to tell the node when it's moved under/not-under the player's crosshair
	void SetUnderCrosshair( bool bUnderCrosshair );

	// Called when the player attempts to activate the node
	void PlayerActivated( void );
	void StopPlaying( void );
	void AbortPlaying( void );
	void TeleportTo( CBasePlayer *pPlayer );
	bool CanTeleportTo( void );

	// Inputs
	void InputStartCommentary( inputdata_t &inputdata );
	void InputStartUnstoppableCommentary( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:
	string_t	m_iszPreCommands;
	string_t	m_iszPostCommands;
	CNetworkVar( string_t, m_iszCommentaryFile );
	CNetworkVar( string_t, m_iszCommentaryFileNoHDR );
	string_t	m_iszViewTarget;
	EHANDLE		m_hViewTarget;
	EHANDLE		m_hViewTargetAngles;		// Entity used to blend view angles to look at the target
	string_t	m_iszViewPosition;
	CNetworkVar( EHANDLE, m_hViewPosition );
	EHANDLE		m_hViewPositionMover;		// Entity used to blend the view to the viewposition entity
	bool		m_bPreventMovement;
	bool		m_bUnderCrosshair;
	bool		m_bUnstoppable;
	float		m_flFinishedTime;
	Vector		m_vecFinishOrigin;
	QAngle		m_vecOriginalAngles;
	QAngle		m_vecFinishAngles;
	bool		m_bPreventChangesWhileMoving;
	bool		m_bDisabled;
	Vector		m_vecTeleportOrigin;

	COutputEvent	m_pOnCommentaryStarted;
	COutputEvent	m_pOnCommentaryStopped;

	CNetworkVar( bool, m_bActive );
	CNetworkVar( float, m_flStartTime );
	CNetworkVar( string_t, m_iszSpeakers );
	CNetworkVar( int, m_iNodeNumber );
	CNetworkVar( int, m_iNodeNumberMax );
};

BEGIN_DATADESC( CPointCommentaryNode )
	DEFINE_KEYFIELD( m_iszPreCommands,	FIELD_STRING,	"precommands" ),
	DEFINE_KEYFIELD( m_iszPostCommands,	FIELD_STRING,	"postcommands" ),
	DEFINE_KEYFIELD( m_iszCommentaryFile, FIELD_STRING,	"commentaryfile" ),
	DEFINE_KEYFIELD( m_iszCommentaryFileNoHDR, FIELD_STRING,	"commentaryfile_nohdr" ),
	DEFINE_KEYFIELD( m_iszViewTarget, FIELD_STRING,	"viewtarget" ),
	DEFINE_FIELD( m_hViewTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hViewTargetAngles, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszViewPosition, FIELD_STRING,	"viewposition" ),
	DEFINE_FIELD( m_hViewPosition, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hViewPositionMover, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_bPreventMovement, FIELD_BOOLEAN,	"prevent_movement" ),
	DEFINE_FIELD( m_bUnderCrosshair, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUnstoppable, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flFinishedTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecFinishOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecOriginalAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecFinishAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_iszSpeakers, FIELD_STRING, "speakers" ),
	DEFINE_FIELD( m_iNodeNumber, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNodeNumberMax, FIELD_INTEGER ),
	DEFINE_FIELD( m_bPreventChangesWhileMoving, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "start_disabled" ),
	DEFINE_KEYFIELD( m_vecTeleportOrigin, FIELD_VECTOR, "teleport_origin" ),

	// Outputs
	DEFINE_OUTPUT( m_pOnCommentaryStarted, "OnCommentaryStarted" ),
	DEFINE_OUTPUT( m_pOnCommentaryStopped, "OnCommentaryStopped" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "StartCommentary", InputStartCommentary ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartUnstoppableCommentary", InputStartUnstoppableCommentary ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Functions
	DEFINE_THINKFUNC( SpinThink ),
	DEFINE_THINKFUNC( UpdateViewThink ),
	DEFINE_THINKFUNC( UpdateViewPostThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointCommentaryNode, DT_PointCommentaryNode )
	SendPropBool( SENDINFO(m_bActive) ),
	SendPropStringT( SENDINFO(m_iszCommentaryFile) ),
	SendPropStringT( SENDINFO(m_iszCommentaryFileNoHDR) ),
	SendPropTime( SENDINFO(m_flStartTime) ),
	SendPropStringT( SENDINFO(m_iszSpeakers) ),
	SendPropInt( SENDINFO(m_iNodeNumber), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iNodeNumberMax), 8, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO(m_hViewPosition) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( point_commentary_node, CPointCommentaryNode );

//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------
class CCommentaryViewPosition : public CSprite 
{
	DECLARE_CLASS( CCommentaryViewPosition, CSprite );
public:
	virtual void Spawn( void )
	{
		Precache();
		SetModelName( MAKE_STRING("sprites/redglow1.vmt") );

		BaseClass::Spawn();

		SetMoveType( MOVETYPE_NONE );
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddEffects( EF_NOSHADOW );
		UTIL_SetSize( this, vec3_origin, vec3_origin );
	}

	virtual void Precache( void )
	{
		PrecacheModel( "sprites/redglow1.vmt" );
	}
};

LINK_ENTITY_TO_CLASS( point_commentary_viewpoint, CCommentaryViewPosition );

//-----------------------------------------------------------------------------
// Purpose: In multiplayer, always return player 1
//-----------------------------------------------------------------------------
CBasePlayer *GetCommentaryPlayer( void )
{
	CBasePlayer *pPlayer;

	if ( gpGlobals->maxClients <= 1 )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}
	else
	{
		// only respond to the first player
		pPlayer = UTIL_PlayerByIndex(1);
	}

	return pPlayer;
}

//===========================================================================================================
// COMMENTARY GAME SYSTEM
//===========================================================================================================
void CV_GlobalChange_Commentary( IConVar *var, const char *pOldString, float flOldValue );

//-----------------------------------------------------------------------------
// Purpose: Game system to kickstart the director's commentary
//-----------------------------------------------------------------------------
class CCommentarySystem : public CAutoGameSystemPerFrame
{
public:
	DECLARE_DATADESC();

	CCommentarySystem() : CAutoGameSystemPerFrame( "CCommentarySystem" )
	{
		m_iCommentaryNodeCount = 0;
	}

	virtual void LevelInitPreEntity()
	{
		m_hCurrentNode = NULL;
		m_bCommentaryConvarsChanging = false;
		m_iClearPressedButtons = 0;

		// If the map started via the map_commentary cmd, start in commentary
		g_bInCommentaryMode = (engine->IsInCommentaryMode() != 0);

		CalculateCommentaryState();
	}

	void CalculateCommentaryState( void )
	{
		// Set the available cvar if we can find commentary data for this level
		char szFullName[512];
		Q_snprintf(szFullName,sizeof(szFullName), "maps/%s_commentary.txt", STRING( gpGlobals->mapname) );
		if ( filesystem->FileExists( szFullName ) )
		{
			commentary_available.SetValue( true );

			// If the user wanted commentary, kick it on
			if ( commentary.GetBool() )
			{
				g_bInCommentaryMode = true;
			}
		}
		else
		{
			g_bInCommentaryMode = false;
			commentary_available.SetValue( false );
		}
	}

	virtual void LevelShutdownPreEntity()
	{
		ShutDownCommentary();
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
				#define COMMENTARY_STRING_LENGTH_MAX		1024

				const char *pszValue = pkvNodeData->GetString();
				Assert( Q_strlen(pszValue) < COMMENTARY_STRING_LENGTH_MAX );
				if ( Q_strnchr(pszValue, '^', COMMENTARY_STRING_LENGTH_MAX) )
				{
					// We want to support quotes in our strings so that we can specify multiple parameters in
					// an output inside our commentary files. We convert ^s to "s here.
					char szTmp[COMMENTARY_STRING_LENGTH_MAX];
					Q_strncpy( szTmp, pszValue, COMMENTARY_STRING_LENGTH_MAX );
					int len = Q_strlen( szTmp );
					for ( int i = 0; i < len; i++ )
					{
						if ( szTmp[i] == '^' )
						{
							szTmp[i] = '"';
						}
					}

					pNode->KeyValue( pkvNodeData->GetName(), szTmp );
				}
				else
				{
					pNode->KeyValue( pkvNodeData->GetName(), pszValue );
				}
			}

			pkvNodeData = pkvNodeData->GetNextKey();
		}
	}

	virtual void LevelInitPostEntity( void )
	{
		if ( !IsInCommentaryMode() )
			return;

		// Don't spawn commentary entities when loading a savegame
		if ( gpGlobals->eLoadType == MapLoad_LoadGame || gpGlobals->eLoadType == MapLoad_Background )
			return;

		m_bCommentaryEnabledMidGame = false;
		InitCommentary();

		IGameEvent *event = gameeventmanager->CreateEvent( "playing_commentary" );
		gameeventmanager->FireEventClientSide( event );
	}

	CPointCommentaryNode *GetNodeUnderCrosshair()
	{
		CBasePlayer *pPlayer = GetCommentaryPlayer();
		if ( !pPlayer )
			return NULL;

		// See if the player's looking at a commentary node
		trace_t tr;
		Vector vecSrc = pPlayer->EyePosition();
		Vector vecForward = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DIRECT_ONLY );	

		g_bTracingVsCommentaryNodes = true;
		UTIL_TraceLine( vecSrc, vecSrc + vecForward * MAX_TRACE_LENGTH, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		g_bTracingVsCommentaryNodes = false;

		if ( !tr.m_pEnt )
			return NULL;

		return dynamic_cast<CPointCommentaryNode*>(tr.m_pEnt);
	}

	void PrePlayerRunCommand( CBasePlayer *pPlayer, CUserCmd *pUserCmds )
	{
		if ( !IsInCommentaryMode() )
			return;

		if ( pPlayer->IsFakeClient() )
			return;

		CPointCommentaryNode *pCurrentNode = GetNodeUnderCrosshair();

		// Changed nodes?
 		if ( m_hCurrentNode != pCurrentNode )
		{
			// Stop animating the old one
 			if ( m_hCurrentNode.Get() )
			{
				m_hCurrentNode->SetUnderCrosshair( false );
			}

			// Start animating the new one
			if ( pCurrentNode )
			{
				pCurrentNode->SetUnderCrosshair( true );
			}

			m_hCurrentNode = pCurrentNode;
		}

		// Check for commentary node activations
		if ( pPlayer )
		{
			// Has the player pressed down an attack button?
			int buttonsChanged = m_afPlayersLastButtons ^ pUserCmds->buttons;
			int buttonsPressed = buttonsChanged & pUserCmds->buttons;
			m_afPlayersLastButtons = pUserCmds->buttons;

			if ( !(pUserCmds->buttons & COMMENTARY_BUTTONS) )
			{
				m_iClearPressedButtons &= ~COMMENTARY_BUTTONS;
			}

			// Detect press events to start/stop commentary nodes
			if (buttonsPressed & COMMENTARY_BUTTONS) 
			{
				if ( buttonsPressed & IN_ATTACK2 )
				{
					if ( !(GetActiveNode() && GetActiveNode()->CannotBeStopped()) )
					{
						JumpToNextNode( pPlayer );
						pUserCmds->buttons &= ~COMMENTARY_BUTTONS;
						m_iClearPressedButtons |= (buttonsPressed & COMMENTARY_BUTTONS);
					}
				}
				else
				{
 					// Looking at a node?
					if ( m_hCurrentNode )
					{
						// Ignore input while an unstoppable node is playing
						if ( !GetActiveNode() || !GetActiveNode()->CannotBeStopped() )
						{
							// If we have an active node already, stop it
							if ( GetActiveNode() && GetActiveNode() != m_hCurrentNode )
							{
								GetActiveNode()->StopPlaying();
 							}

							m_hCurrentNode->PlayerActivated();
						}

						// Prevent weapon firing when toggling nodes
						pUserCmds->buttons &= ~COMMENTARY_BUTTONS;
						m_iClearPressedButtons |= (buttonsPressed & COMMENTARY_BUTTONS);
					}
					else if ( GetActiveNode() && GetActiveNode()->HasViewTarget() )
					{
						if ( !GetActiveNode()->CannotBeStopped() )
						{
							GetActiveNode()->StopPlaying();
						}

						// Prevent weapon firing when toggling nodes
						pUserCmds->buttons &= ~COMMENTARY_BUTTONS;
						m_iClearPressedButtons |= (buttonsPressed & COMMENTARY_BUTTONS);
					}
				}
			}

			if ( GetActiveNode() && GetActiveNode()->PreventsMovement() )
			{
 				pUserCmds->buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK );
				pUserCmds->upmove = 0;
				pUserCmds->sidemove = 0;
				pUserCmds->forwardmove = 0;
			}

			// When we swallow button down events, we have to keep clearing that button
			// until the player releases the button. Otherwise, the frame after we swallow
			// it, the code detects the button down and goes ahead as normal.
			pUserCmds->buttons &= ~m_iClearPressedButtons;
		}

		if ( m_iTeleportStage != TELEPORT_NONE )
		{
			if ( m_flNextTeleportTime <= gpGlobals->curtime )
			{
				if ( m_iTeleportStage == TELEPORT_FADEOUT )
				{
					m_iTeleportStage = TELEPORT_TELEPORT;
					m_flNextTeleportTime = gpGlobals->curtime + 0.35;

					color32_s clr = { 0,0,0,255 };
					UTIL_ScreenFade( pPlayer, clr, 0.3, 0, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT );
				}
				else if ( m_iTeleportStage == TELEPORT_TELEPORT )
				{
					if ( m_hLastCommentaryNode )
					{
						m_hLastCommentaryNode->TeleportTo( pPlayer );
					}

					m_iTeleportStage = TELEPORT_FADEIN;
					m_flNextTeleportTime = gpGlobals->curtime + 0.6;
				}
				else if ( m_iTeleportStage == TELEPORT_FADEIN )
				{
					m_iTeleportStage = TELEPORT_NONE;
					m_flNextTeleportTime = gpGlobals->curtime + 0.25;

					color32_s clr = { 0,0,0,255 };
					UTIL_ScreenFade( pPlayer, clr, 0.3, 0, FFADE_IN | FFADE_PURGE );
				}
			}
		}
	}

	CPointCommentaryNode *GetActiveNode( void )
	{
		return m_hActiveCommentaryNode;
	}

	void SetActiveNode( CPointCommentaryNode *pNode )
	{
		m_hActiveCommentaryNode = pNode;
		if ( pNode )
		{
			m_hLastCommentaryNode = pNode;
		}
	}

	int GetCommentaryNodeCount( void )
	{
		return m_iCommentaryNodeCount;
	}

	bool CommentaryConvarsChanging( void )
	{
		return m_bCommentaryConvarsChanging;
	}

	void SetCommentaryConvarsChanging( bool bChanging )
	{
		m_bCommentaryConvarsChanging = bChanging;
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

	void InitCommentary( void )
	{
		// Install the global cvar callback
		cvar->InstallGlobalChangeCallback( CV_GlobalChange_Commentary );

		m_flNextTeleportTime = 0;
		m_iTeleportStage = TELEPORT_NONE;
		m_hLastCommentaryNode = NULL;

		// If we find the commentary semaphore, the commentary entities already exist.
		// This occurs when you transition back to a map that has saved commentary nodes in it.
		if ( gEntList.FindEntityByName( NULL, COMMENTARY_SPAWNED_SEMAPHORE ) )
			return;

		// Spawn the commentary semaphore entity
		CBaseEntity *pSemaphore = CreateEntityByName( "info_target" );
		pSemaphore->SetName( MAKE_STRING(COMMENTARY_SPAWNED_SEMAPHORE) );

		bool oldLock = engine->LockNetworkStringTables( false );

		// Find the commentary file
		char szFullName[512];
		Q_snprintf(szFullName,sizeof(szFullName), "maps/%s_commentary.txt", STRING( gpGlobals->mapname ));
		KeyValues *pkvFile = new KeyValues( "Commentary" );
		if ( pkvFile->LoadFromFile( filesystem, szFullName, "MOD" ) )
		{
			Msg( "Commentary: Loading commentary data from %s. \n", szFullName );

			// Load each commentary block, and spawn the entities
			KeyValues *pkvNode = pkvFile->GetFirstSubKey();
			while ( pkvNode )
			{
				// Get node name
				const char *pNodeName = pkvNode->GetName();

				// Skip the trackinfo
				if ( !Q_strncmp( pNodeName, "trackinfo", 9 ) )
				{
					pkvNode = pkvNode->GetNextKey();
					continue;
				}

				KeyValues *pClassname = pkvNode->FindKey( "classname" );
				if ( pClassname )
				{
					// Use the classname instead
					pNodeName = pClassname->GetString();
				}

				// Spawn the commentary entity
				CBaseEntity *pNode = CreateEntityByName( pNodeName );
				if ( pNode )
				{
					ParseEntKVBlock( pNode, pkvNode );
					DispatchSpawn( pNode );

					EHANDLE hHandle;
					hHandle = pNode;
					m_hSpawnedEntities.AddToTail( hHandle );

					CPointCommentaryNode *pCommNode = dynamic_cast<CPointCommentaryNode*>(pNode);
					if ( pCommNode )
					{
						m_iCommentaryNodeCount++;
						pCommNode->SetNodeNumber( m_iCommentaryNodeCount );
					}
				}
				else
				{
					Warning("Commentary: Failed to spawn commentary entity, type: '%s'\n", pNodeName );
				}

				// Move to next entity
				pkvNode = pkvNode->GetNextKey();
			}

			// Then activate all the entities
			for ( int i = 0; i < m_hSpawnedEntities.Count(); i++ )
			{
				m_hSpawnedEntities[i]->Activate();
			}
		}
		else
		{
			Msg( "Commentary: Could not find commentary data file '%s'. \n", szFullName );
		}

		engine->LockNetworkStringTables( oldLock );
	}

	void ShutDownCommentary( void )
	{
		if ( GetActiveNode() )
		{
			GetActiveNode()->AbortPlaying();
		}

		// Destroy all the entities created by commentary
		for ( int i = m_hSpawnedEntities.Count()-1; i >= 0; i-- )
		{
			if ( m_hSpawnedEntities[i] )
			{
				UTIL_Remove( m_hSpawnedEntities[i] );
			}
		}
		m_hSpawnedEntities.Purge();
		m_iCommentaryNodeCount = 0;

		// Remove the commentary semaphore
		CBaseEntity *pSemaphore = gEntList.FindEntityByName( NULL, COMMENTARY_SPAWNED_SEMAPHORE );
		if ( pSemaphore )
		{
			UTIL_Remove( pSemaphore );
		}

		// Remove our global convar callback
		cvar->RemoveGlobalChangeCallback( CV_GlobalChange_Commentary );

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

		m_hCurrentNode = NULL;
		m_hActiveCommentaryNode = NULL;
		m_hLastCommentaryNode = NULL;
		m_flNextTeleportTime = 0;
		m_iTeleportStage = TELEPORT_NONE;
	}

	void SetCommentaryMode( bool bCommentaryMode )
	{
		g_bInCommentaryMode = bCommentaryMode;
		CalculateCommentaryState();

		// If we're turning on commentary, create all the entities.
		if ( IsInCommentaryMode() )
		{
			m_bCommentaryEnabledMidGame = true;
			InitCommentary();
		}
		else
		{
			ShutDownCommentary();
		}
	}

	void OnRestore( void )
	{
		cvar->RemoveGlobalChangeCallback( CV_GlobalChange_Commentary );

		if ( !IsInCommentaryMode() )
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
		cvar->InstallGlobalChangeCallback( CV_GlobalChange_Commentary );
	}

	bool CommentaryWasEnabledMidGame( void ) 
	{
		return m_bCommentaryEnabledMidGame;
	}

	void JumpToNextNode( CBasePlayer *pPlayer )
	{
		if ( m_flNextTeleportTime > gpGlobals->curtime || m_iTeleportStage != TELEPORT_NONE )
			return;

		CBaseEntity *pEnt = m_hLastCommentaryNode;
		while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "point_commentary_node" ) ) != m_hLastCommentaryNode )
		{
			CPointCommentaryNode *pNode = dynamic_cast<CPointCommentaryNode *>( pEnt );
			if ( pNode && pNode->CanTeleportTo() )
			{
				m_iTeleportStage = TELEPORT_FADEOUT;
				m_hLastCommentaryNode = pNode;
				m_flNextTeleportTime = gpGlobals->curtime;

				// Stop any active nodes
				if ( m_hActiveCommentaryNode )
				{
					m_hActiveCommentaryNode->StopPlaying();
				}
				break;
			}
		}
	}

private:
	int		m_afPlayersLastButtons;
	int		m_iCommentaryNodeCount;
	bool	m_bCommentaryConvarsChanging;
	int		m_iClearPressedButtons;
	bool	m_bCommentaryEnabledMidGame;
	float	m_flNextTeleportTime;
	int		m_iTeleportStage;

	CUtlVector< modifiedconvars_t > m_ModifiedConvars;
	CUtlVector<EHANDLE>				m_hSpawnedEntities;
	CHandle<CPointCommentaryNode>	m_hCurrentNode;
	CHandle<CPointCommentaryNode>	m_hActiveCommentaryNode;
	CHandle<CPointCommentaryNode>	m_hLastCommentaryNode;
};

CCommentarySystem	g_CommentarySystem;

void CommentarySystem_PePlayerRunCommand( CBasePlayer *player, CUserCmd *ucmd )
{
	g_CommentarySystem.PrePlayerRunCommand( player, ucmd );
}

BEGIN_DATADESC_NO_BASE( CCommentarySystem )
	//int m_afPlayersLastButtons;			DON'T SAVE
	//bool m_bCommentaryConvarsChanging;	DON'T SAVE
	//int m_iClearPressedButtons;			DON'T SAVE

	DEFINE_FIELD( m_bCommentaryEnabledMidGame, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextTeleportTime, FIELD_TIME ),
	DEFINE_FIELD( m_iTeleportStage, FIELD_INTEGER ),

	DEFINE_UTLVECTOR( m_ModifiedConvars, FIELD_EMBEDDED ),
	DEFINE_UTLVECTOR( m_hSpawnedEntities, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCurrentNode, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hActiveCommentaryNode, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLastCommentaryNode, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iCommentaryNodeCount, FIELD_INTEGER ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( modifiedconvars_t )
	DEFINE_ARRAY( pszConvar, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
	DEFINE_ARRAY( pszCurrentValue, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
	DEFINE_ARRAY( pszOrgValue, FIELD_CHARACTER, MAX_MODIFIED_CONVAR_STRING ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CommentaryChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
 	if ( var.GetBool() != g_bInCommentaryMode )
	{
		g_CommentarySystem.SetCommentaryMode( var.GetBool() );
	}
}
ConVar commentary("commentary", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Desired commentary mode state.", CC_CommentaryChanged );

//-----------------------------------------------------------------------------
// Purpose: We need to revert back any convar changes that are made by the
//			commentary system during commentary. This code stores convar changes
//			made by the commentary system, and reverts them when finished.
//-----------------------------------------------------------------------------
void CV_GlobalChange_Commentary( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( !g_CommentarySystem.CommentaryConvarsChanging() )
	{
		// A convar has changed, but not due to commentary nodes. Ignore it.
		return;
	}

	g_CommentarySystem.ConvarChanged( var, pOldString, flOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CommentaryNotChanging( void )
{
	g_CommentarySystem.SetCommentaryConvarsChanging( false );
}
static ConCommand commentary_cvarsnotchanging("commentary_cvarsnotchanging", CC_CommentaryNotChanging, 0 );

bool IsListeningToCommentary( void )
{
	return ( g_CommentarySystem.GetActiveNode() != NULL );
}

//===========================================================================================================
// COMMENTARY NODES
//===========================================================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::Spawn( void )
{
	// No model specified?
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/extras/info_speech.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );
	UTIL_SetSize( this, -Vector(16,16,16), Vector(16,16,16) );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	AddEffects( EF_NOSHADOW );

	// Setup for animation
	ResetSequence( LookupSequence("idle") );
	SetThink( &CPointCommentaryNode::SpinThink );
	SetNextThink( gpGlobals->curtime + 0.1f ); 

	m_iNodeNumber = 0;
	m_iNodeNumberMax = 0;

	SetDisabled( m_bDisabled );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::Activate( void )
{
	m_iNodeNumberMax = g_CommentarySystem.GetCommentaryNodeCount();

	if ( m_iszViewTarget != NULL_STRING )
	{
		m_hViewTarget = gEntList.FindEntityByName( NULL, m_iszViewTarget );
		if ( !m_hViewTarget )
		{
			Warning("%s: %s could not find viewtarget %s.\n", GetClassname(), GetDebugName(), STRING(m_iszViewTarget) );
		}
	}

	if ( m_iszViewPosition != NULL_STRING )
	{
		m_hViewPosition = gEntList.FindEntityByName( NULL, m_iszViewPosition );
		if ( !m_hViewPosition.Get() )
		{
			Warning("%s: %s could not find viewposition %s.\n", GetClassname(), GetDebugName(), STRING(m_iszViewPosition) );
		}
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

	if ( m_iszCommentaryFile.Get() != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszCommentaryFile.Get() ) );
	}
	else
	{
		Warning("%s: %s has no commentary file.\n", GetClassname(), GetDebugName() );
	}

	if ( m_iszCommentaryFileNoHDR.Get() != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszCommentaryFileNoHDR.Get() ) );
	}
	
	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// Purpose: Called to tell the node when it's moved under/not-under the player's crosshair
//-----------------------------------------------------------------------------
void CPointCommentaryNode::SetUnderCrosshair( bool bUnderCrosshair )
{
	if ( bUnderCrosshair )
	{
		// Start animating
		m_bUnderCrosshair = true;
	
		if ( !m_bActive )
		{
			m_flAnimTime = gpGlobals->curtime;
		}
	}
	else
	{
		// Stop animating
		m_bUnderCrosshair = false;
	}
}

//------------------------------------------------------------------------------
// Purpose: Prevent collisions of everything except the trace from the commentary system
//------------------------------------------------------------------------------
bool CPointCommentaryNode::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !g_bTracingVsCommentaryNodes )
		return false;
	if ( m_bDisabled )
		return false;

	return BaseClass::TestCollision( ray, mask, trace );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointCommentaryNode::SpinThink( void )
{
	// Rotate if we're active, or under the crosshair. Don't rotate if we're
	// under the crosshair, but we've already been listened to.
	if ( m_bActive || (m_bUnderCrosshair && m_nSkin == 0) )
	{
		if ( m_bActive )
		{
			m_flPlaybackRate = 3.0;
		}
		else
		{
			m_flPlaybackRate = 1.0;
		}
		StudioFrameAdvance();
		DispatchAnimEvents(this);
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointCommentaryNode::PlayerActivated( void )
{
	gamestats->Event_Commentary();

	if ( m_bActive )
	{
		StopPlaying();
	}
	else
	{
		StartCommentary();
		g_CommentarySystem.SetActiveNode( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::StopPlaying( void )
{
	if ( m_bActive )
	{
		FinishCommentary();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop playing the node, but snap completely out of the node.
//			Used when players shut down commentary while we're in the middle
//			of playing a node, so we can't smoothly blend out (since the 
//			commentary entities need to be removed).
//-----------------------------------------------------------------------------
void CPointCommentaryNode::AbortPlaying( void )
{
	if ( m_bActive )
	{
		FinishCommentary( false );
	}
	else if ( m_bPreventChangesWhileMoving )
	{
		// We're a node that's not active, but is in the process of transitioning the view. Finish movement.
		CleanupPostCommentary();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPointCommentaryNode::CanTeleportTo( void )
{
	//return ( m_vecTeleportOrigin != vec3_origin );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::TeleportTo( CBasePlayer *pPlayer )
{
	Vector vecTarget = m_vecTeleportOrigin;
	if ( m_vecTeleportOrigin == vec3_origin )
	{
		vecTarget = GetAbsOrigin();
	}

	trace_t trace;
	UTIL_TraceHull( vecTarget, vecTarget + Vector( 0, 0, -500 ), pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs(), MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );

	pPlayer->Teleport( &trace.endpos, NULL, &vec3_origin );

	Vector vecToNode = GetAbsOrigin() - pPlayer->EyePosition();
	VectorNormalize( vecToNode );
	QAngle vecAngle;
	VectorAngles( vecToNode, Vector(0,0,1), vecAngle );

	pPlayer->SnapEyeAngles( vecAngle );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointCommentaryNode::StartCommentary( void )
{
	CBasePlayer *pPlayer = GetCommentaryPlayer();

	if ( !pPlayer )
		return;

	m_bActive = true;

	m_flAnimTime = gpGlobals->curtime;
	m_flPrevAnimTime = gpGlobals->curtime;

	// Switch to the greyed out skin 
	m_nSkin = 1;

	m_pOnCommentaryStarted.FireOutput( this, this );

	// Fire off our precommands
	if ( m_iszPreCommands != NULL_STRING )
	{
		g_CommentarySystem.SetCommentaryConvarsChanging( true );
		engine->ClientCommand( pPlayer->edict(), STRING(m_iszPreCommands) );
		engine->ClientCommand( pPlayer->edict(), "commentary_cvarsnotchanging\n" );
	}

	// Start the commentary
	m_flStartTime = gpGlobals->curtime;

	// If we have a view target, start blending towards it
	if ( m_hViewTarget || m_hViewPosition.Get() )
	{
		m_vecOriginalAngles = pPlayer->EyeAngles();
 		SetContextThink( &CPointCommentaryNode::UpdateViewThink, gpGlobals->curtime, s_pCommentaryUpdateViewThink );
	}

	//SetContextThink( &CPointCommentaryNode::FinishCommentary, gpGlobals->curtime + flDuration, s_pFinishCommentaryThink );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CommentaryFinishNode( void )
{
	// We were told by the client DLL that our commentary has finished
	if ( g_CommentarySystem.GetActiveNode() )
	{
		g_CommentarySystem.GetActiveNode()->StopPlaying();
	}
}
static ConCommand commentary_finishnode("commentary_finishnode", CC_CommentaryFinishNode, 0 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::UpdateViewThink( void )
{
	if ( !m_bActive )
		return;
	CBasePlayer *pPlayer = GetCommentaryPlayer();
	if ( !pPlayer )
		return;

	// Swing the view towards the target
	if ( m_hViewTarget )
	{
		if ( !m_hViewTargetAngles && !m_hViewPositionMover )
		{
			// Make an invisible entity to attach view angles to
			m_hViewTargetAngles = CreateEntityByName( "point_commentary_viewpoint" );
			m_hViewTargetAngles->SetAbsOrigin( pPlayer->EyePosition() );
			m_hViewTargetAngles->SetAbsAngles( pPlayer->EyeAngles() );
			pPlayer->SetViewEntity( m_hViewTargetAngles );

			if ( pPlayer->GetActiveWeapon() )
			{
				pPlayer->GetActiveWeapon()->Holster();
			}
		}

 		QAngle angGoal;
 		QAngle angCurrent;
		if ( m_hViewPositionMover )
		{
			angCurrent = m_hViewPositionMover->GetAbsAngles();
			VectorAngles( m_hViewTarget->WorldSpaceCenter() - m_hViewPositionMover->GetAbsOrigin(), angGoal );
		}
		else if ( m_hViewTargetAngles )
		{
			angCurrent = m_hViewTargetAngles->GetAbsAngles();
			m_hViewTargetAngles->SetAbsOrigin( pPlayer->EyePosition() );
			VectorAngles( m_hViewTarget->WorldSpaceCenter() - m_hViewTargetAngles->GetAbsOrigin(), angGoal );
		}
		else
		{
			angCurrent = pPlayer->EyeAngles();
      		VectorAngles( m_hViewTarget->WorldSpaceCenter() - pPlayer->EyePosition(), angGoal );
		}

		// Accelerate towards the target goal angles
  		float dx = AngleDiff( angGoal.x, angCurrent.x );
  		float dy = AngleDiff( angGoal.y, angCurrent.y );
		float mod = 1.0 - ExponentialDecay( 0.5, 0.3, gpGlobals->frametime );
   		float dxmod = dx * mod;
		float dymod = dy * mod;

 		angCurrent.x = AngleNormalize( angCurrent.x + dxmod );
 		angCurrent.y = AngleNormalize( angCurrent.y + dymod );

		if ( m_hViewPositionMover )
		{
			m_hViewPositionMover->SetAbsAngles( angCurrent );
		}
		else if ( m_hViewTargetAngles )
		{
			m_hViewTargetAngles->SetAbsAngles( angCurrent );
			pPlayer->SnapEyeAngles( angCurrent );
		}
		else
		{
			pPlayer->SnapEyeAngles( angCurrent );
		}

		SetNextThink( gpGlobals->curtime, s_pCommentaryUpdateViewThink );
	}

 	if ( m_hViewPosition.Get() )
	{
		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->Holster();
		}

 		if ( !m_hViewPositionMover )
		{
			// Make an invisible info target entity for us to attach the view to, 
			// and move it to the desired view position.
			m_hViewPositionMover = CreateEntityByName( "point_commentary_viewpoint" );
			m_hViewPositionMover->SetAbsAngles( pPlayer->EyeAngles() );
			pPlayer->SetViewEntity( m_hViewPositionMover );
		}

		// Blend to the target position over time. 
 		float flCurTime = (gpGlobals->curtime - m_flStartTime);
 		float flBlendPerc = clamp( flCurTime * 0.5f, 0.f, 1.f );

		// Figure out the current view position
		Vector vecCurEye;
		VectorLerp( pPlayer->EyePosition(), m_hViewPosition.Get()->GetAbsOrigin(), flBlendPerc, vecCurEye );
		m_hViewPositionMover->SetAbsOrigin( vecCurEye ); 

		SetNextThink( gpGlobals->curtime, s_pCommentaryUpdateViewThink );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::UpdateViewPostThink( void )
{
	CBasePlayer *pPlayer = GetCommentaryPlayer();
	if ( !pPlayer )
		return;

 	if ( m_hViewPosition.Get() && m_hViewPositionMover )
	{
 		// Blend back to the player's position over time.
   		float flCurTime = (gpGlobals->curtime - m_flFinishedTime);
		float flTimeToBlend = MIN( 2.0, m_flFinishedTime - m_flStartTime ); 
 		float flBlendPerc = 1.0f - clamp( flCurTime / flTimeToBlend, 0.f, 1.f );

		//Msg("OUT: CurTime %.2f, BlendTime: %.2f, Blend: %.3f\n", flCurTime, flTimeToBlend, flBlendPerc );

		// Only do this while we're still moving
		if ( flBlendPerc > 0 )
		{
			// Figure out the current view position
			Vector vecPlayerPos = pPlayer->EyePosition();
			Vector vecToPosition = (m_vecFinishOrigin - vecPlayerPos); 
			Vector vecCurEye = pPlayer->EyePosition() + (vecToPosition * flBlendPerc);
			m_hViewPositionMover->SetAbsOrigin( vecCurEye ); 

			if ( m_hViewTarget )
			{
				Quaternion quatFinish;
				Quaternion quatOriginal;
				Quaternion quatCurrent;
				AngleQuaternion( m_vecOriginalAngles, quatOriginal );
				AngleQuaternion( m_vecFinishAngles, quatFinish );
				QuaternionSlerp( quatFinish, quatOriginal, 1.0 - flBlendPerc, quatCurrent );
				QAngle angCurrent;
				QuaternionAngles( quatCurrent, angCurrent );
				m_hViewPositionMover->SetAbsAngles( angCurrent );
			}

			SetNextThink( gpGlobals->curtime, s_pCommentaryUpdateViewThink );
			return;
		}

		pPlayer->SnapEyeAngles( m_hViewPositionMover->GetAbsAngles() );
	}

	// We're done
	CleanupPostCommentary();

	m_bPreventChangesWhileMoving = false;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointCommentaryNode::FinishCommentary( bool bBlendOut )
{
	CBasePlayer *pPlayer = GetCommentaryPlayer();
	if ( !pPlayer )
		return;

	// Fire off our postcommands
	if ( m_iszPostCommands != NULL_STRING )
	{
		g_CommentarySystem.SetCommentaryConvarsChanging( true );
		engine->ClientCommand( pPlayer->edict(), STRING(m_iszPostCommands) );
		engine->ClientCommand( pPlayer->edict(), "commentary_cvarsnotchanging\n" );
	}

	// Stop the commentary
	m_flFinishedTime = gpGlobals->curtime;

	if ( bBlendOut && m_hViewPositionMover )
	{
 		m_bActive = false;
		m_flPlaybackRate = 1.0;
		m_vecFinishOrigin = m_hViewPositionMover->GetAbsOrigin();
		m_vecFinishAngles = m_hViewPositionMover->GetAbsAngles();

		m_bPreventChangesWhileMoving = true;

		// We've moved away from the player's position. Move back to it before ending
		SetContextThink( &CPointCommentaryNode::UpdateViewPostThink, gpGlobals->curtime, s_pCommentaryUpdateViewThink );
		return;
	}

	CleanupPostCommentary();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::CleanupPostCommentary( void )
{
	CBasePlayer *pPlayer = GetCommentaryPlayer();
	if ( !pPlayer )
		return;

	if ( ( m_hViewPositionMover || m_hViewTargetAngles ) && pPlayer->GetActiveWeapon() )
	{
		pPlayer->GetActiveWeapon()->Deploy();
	}

	if ( m_hViewTargetAngles && pPlayer->GetViewEntity() == m_hViewTargetAngles )
	{
		pPlayer->SetViewEntity( NULL );
	}
	UTIL_Remove( m_hViewTargetAngles );

	if ( m_hViewPositionMover && pPlayer->GetViewEntity() == m_hViewPositionMover )
	{
		pPlayer->SetViewEntity( NULL );
	}
	UTIL_Remove( m_hViewPositionMover );

	m_bActive = false;
	m_flPlaybackRate = 1.0;
	m_bUnstoppable = false;
	m_flFinishedTime = 0;
	SetContextThink( NULL, 0, s_pCommentaryUpdateViewThink );

	// Clear out any events waiting on our start commentary
	g_EventQueue.CancelEvents( this );

	m_pOnCommentaryStopped.FireOutput( this, this );

	g_CommentarySystem.SetActiveNode( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::InputStartCommentary( inputdata_t &inputdata )
{
 	if ( !m_bActive )
	{
		if ( g_CommentarySystem.GetActiveNode() )
		{
			g_CommentarySystem.GetActiveNode()->StopPlaying();
		}

		PlayerActivated();
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::InputStartUnstoppableCommentary( inputdata_t &inputdata )
{
	if ( !m_bActive )
	{
		m_bUnstoppable = true;
		InputStartCommentary( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( m_bDisabled )
	{
		AddEffects( EF_NODRAW );
	}
	else
	{
		RemoveEffects( EF_NODRAW );
	}
}

//-----------------------------------------------------------------------------
// Purpose Force our lighting landmark to be transmitted
//-----------------------------------------------------------------------------
int CPointCommentaryNode::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCommentaryNode::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	// Force our camera view position entity to be sent
	if ( m_hViewTarget )
	{
		m_hViewTarget->SetTransmit( pInfo, bAlways );
	}
	if ( m_hViewTargetAngles )
	{
		m_hViewTargetAngles->SetTransmit( pInfo, bAlways );
	}
	if ( m_hViewPosition.Get() )
	{
		m_hViewPosition.Get()->SetTransmit( pInfo, bAlways );
	}
	if ( m_hViewPositionMover )
	{
		m_hViewPositionMover->SetTransmit( pInfo, bAlways );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPointCommentaryNode::PreventsMovement( void )
{ 
	// If we're moving the player's view at all, prevent movement
	if ( m_hViewPosition.Get() )
		return true;

	return m_bPreventMovement; 
}

//-----------------------------------------------------------------------------
// COMMENTARY SAVE / RESTORE
//-----------------------------------------------------------------------------
static short COMMENTARY_SAVE_RESTORE_VERSION = 2;

class CCommentary_SaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "Commentary";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->WriteBool( &g_bInCommentaryMode );
		if ( IsInCommentaryMode() )
		{
			pSave->WriteAll( &g_CommentarySystem, g_CommentarySystem.GetDataDescMap() );
			pSave->WriteInt( &CAI_BaseNPC::m_nDebugBits );
		}
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &COMMENTARY_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == COMMENTARY_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( m_fDoLoad )
		{
			pRestore->ReadBool( &g_bInCommentaryMode );
			if ( g_bInCommentaryMode )
			{
				pRestore->ReadAll( &g_CommentarySystem, g_CommentarySystem.GetDataDescMap() );
				CAI_BaseNPC::m_nDebugBits = pRestore->ReadInt();
			}

			// Force the commentary convar to match the saved game state
			commentary.SetValue( g_bInCommentaryMode );
		}
	}

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CCommentary_SaveRestoreBlockHandler g_Commentary_SaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetCommentarySaveRestoreBlockHandler()
{
	return &g_Commentary_SaveRestoreBlockHandler;
}

//-----------------------------------------------------------------------------
// Purpose: Commentary specific logic_auto replacement.
//			Fires outputs based upon how commentary mode has been activated.
//-----------------------------------------------------------------------------
class CCommentaryAuto : public CBaseEntity
{
	DECLARE_CLASS( CCommentaryAuto, CBaseEntity );
public:
	DECLARE_DATADESC();

	void Spawn(void);
	void Think(void);

	void InputMultiplayerSpawned( inputdata_t &inputdata );

private:
	// fired if commentary started due to new map
	COutputEvent m_OnCommentaryNewGame;

	// fired if commentary was turned on in the middle of a map
	COutputEvent m_OnCommentaryMidGame;

	// fired when the player spawns in a multiplayer game
	COutputEvent m_OnCommentaryMultiplayerSpawn;
};

LINK_ENTITY_TO_CLASS(commentary_auto, CCommentaryAuto);

BEGIN_DATADESC( CCommentaryAuto )
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "MultiplayerSpawned", InputMultiplayerSpawned ),

	// Outputs
	DEFINE_OUTPUT(m_OnCommentaryNewGame, "OnCommentaryNewGame"),
	DEFINE_OUTPUT(m_OnCommentaryMidGame, "OnCommentaryMidGame"),
	DEFINE_OUTPUT(m_OnCommentaryMultiplayerSpawn, "OnCommentaryMultiplayerSpawn"),
END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Fire my outputs here if I fire on map reload
//------------------------------------------------------------------------------
void CCommentaryAuto::Spawn(void)
{
	BaseClass::Spawn();
	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryAuto::Think(void)
{
	if ( g_CommentarySystem.CommentaryWasEnabledMidGame() )
	{
		m_OnCommentaryMidGame.FireOutput(NULL, this);
	}
	else
	{
		m_OnCommentaryNewGame.FireOutput(NULL, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCommentaryAuto::InputMultiplayerSpawned( inputdata_t &inputdata )
{
	m_OnCommentaryMultiplayerSpawn.FireOutput( NULL, this );
}

#else

bool IsInCommentaryMode( void )
{
	return false;
}

#endif
