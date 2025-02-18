//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_respawnroom.h"
#include "func_no_build.h"
#include "tf_team.h"
#include "ndebugoverlay.h"
#include "tf_gamerules.h"
#include "entity_tfstart.h"
#include "modelentities.h"
#include "tf_obj_sentrygun.h"
#include "entity_rune.h"
#include "tf_item.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Visualizes a respawn room to the enemy team
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IFuncRespawnRoomVisualizerAutoList );

class CFuncRespawnRoomVisualizer : public CFuncBrush, public IFuncRespawnRoomVisualizerAutoList
{
	DECLARE_CLASS( CFuncRespawnRoomVisualizer, CFuncBrush );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CFuncRespawnRoomVisualizer();

	virtual void Spawn( void );
	void	InputRoundActivate( inputdata_t &inputdata );
	int		DrawDebugTextOverlays( void );
	CFuncRespawnRoom *GetRespawnRoom( void ) { return m_hRespawnRoom; }

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void			InputSetSolid( inputdata_t &inputdata );

	void SetActive( bool bActive );

protected:
	string_t					m_iszRespawnRoomName;
	CHandle<CFuncRespawnRoom>	m_hRespawnRoom;
	bool						m_bSolid;
};

IMPLEMENT_AUTO_LIST( IFuncRespawnRoomVisualizerAutoList );

IMPLEMENT_AUTO_LIST( IFuncRespawnRoomAutoList );

LINK_ENTITY_TO_CLASS( func_respawnroom, CFuncRespawnRoom);

BEGIN_DATADESC( CFuncRespawnRoom )
	DEFINE_FUNCTION( CFuncRespawnRoomShim::Touch ),
	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleActive", InputToggleActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncRespawnRoom, DT_FuncRespawnRoom )
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncRespawnRoom::CFuncRespawnRoom()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::Spawn( void )
{
	AddSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS);

	BaseClass::Spawn();
	InitTrigger();

	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );

	m_bActive = true;
	SetTouch( &CFuncRespawnRoom::RespawnRoomTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::Activate( void )
{
	BaseClass::Activate();
	m_iOriginalTeam = GetTeamNumber();
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - The thing that touched us.
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::RespawnRoomTouch(CBaseEntity *pOther)
{
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			return;
		}
	}

	if ( PassesTriggerFilters( pOther ) )
	{
		if ( pOther->IsPlayer() && InSameTeam( pOther ) )
		{
			// Players carrying the flag drop it if they try to run into a respawn room
			CTFPlayer *pPlayer = ToTFPlayer( pOther );
			if ( pPlayer->HasTheFlag() )
			{
				pPlayer->DropFlag();
			}
			else if ( TFGameRules() && TFGameRules()->GetGameType() == TF_GAMETYPE_PD && pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
			{
				pPlayer->GetItem()->Drop( pPlayer, true, true, true );
			}

			if ( pPlayer->m_Shared.IsCarryingObject() && TFGameRules()->IsMannVsMachineMode() )
			{
				CObjectSentrygun *pSentry = dynamic_cast< CObjectSentrygun* >( pPlayer->m_Shared.GetCarriedObject() );
				if ( pSentry )
				{
					pSentry->UpdatePlacement();
					pSentry->DetonateObject();
				}
			}
			// Drop your powerup rune when entering a respawn room. 
			// False parameter ensures rune isn't unintentionally 'thrown' into the respawn room
			pPlayer->DropRune( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::StartTouch(CBaseEntity *pOther)
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.IncrementRespawnTouchCount();
	}
	
	BaseClass::StartTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::EndTouch(CBaseEntity *pOther)
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.DecrementRespawnTouchCount();
	}
	
	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::InputToggleActive( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		SetActive( false );
	}
	else
	{
		SetActive( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::InputRoundActivate( inputdata_t &input )
{
	if ( m_iOriginalTeam == TEAM_UNASSIGNED )
	{
		ChangeTeam( TEAM_UNASSIGNED );

		// If we don't have a team, find a respawn point inside us that we can derive a team from.
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( PointIsWithin( pTFSpawn->GetAbsOrigin() ) )
			{
				if ( !pTFSpawn->IsDisabled() && pTFSpawn->GetTeamNumber() > LAST_SHARED_TEAM )
				{
					ChangeTeam( pTFSpawn->GetTeamNumber() );
					break;
				}
			}
		}

		if ( GetTeamNumber() == TEAM_UNASSIGNED )
		{
			DevMsg( "Unassigned %s(%s) failed to find an info_player_teamspawn within it to use.\n", GetClassname(), GetDebugName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::ChangeTeam( int iTeamNum )
{
	BaseClass::ChangeTeam( iTeamNum );

	for ( int i = m_hVisualizers.Count()-1; i >= 0; i-- )
	{
		if ( m_hVisualizers[i] )
		{
			Assert( m_hVisualizers[i]->GetRespawnRoom() == this );
			m_hVisualizers[i]->ChangeTeam( iTeamNum );
		}
		else
		{
			m_hVisualizers.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::SetActive( bool bActive )
{
	m_bActive = bActive;
	if ( m_bActive )
	{
		Enable();
	}
	else
	{
		Disable();
	}

	for ( int i = m_hVisualizers.Count()-1; i >= 0; i-- )
	{
		if ( m_hVisualizers[i] )
		{
			Assert( m_hVisualizers[i]->GetRespawnRoom() == this );
			m_hVisualizers[i]->SetActive( m_bActive );
		}
		else
		{
			m_hVisualizers.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncRespawnRoom::GetActive() const
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoom::AddVisualizer( CFuncRespawnRoomVisualizer *pViz )
{
	if ( m_hVisualizers.Find(pViz) == m_hVisualizers.InvalidIndex() )
	{
		m_hVisualizers.AddToTail( pViz );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is a given point contained within a respawn room?
//-----------------------------------------------------------------------------
bool PointInRespawnRoom( const CBaseEntity *pTarget, const Vector &vecOrigin, bool bTouching_SameTeamOnly /*= false*/ )
{
	// Find out whether we're in a respawn room or not
	for ( int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i )
	{
		CFuncRespawnRoom *pRespawnRoom = static_cast< CFuncRespawnRoom* >( IFuncRespawnRoomAutoList::AutoList()[i] );

		// Are we within this respawn room?
		if ( pRespawnRoom->GetActive() )
		{
			if ( pRespawnRoom->PointIsWithin( vecOrigin ) )
			{
				if ( !pTarget || pRespawnRoom->GetTeamNumber() == TEAM_UNASSIGNED || pRespawnRoom->InSameTeam( pTarget ) )
					return true;
			}
			else 
			{
				if ( pTarget && pRespawnRoom->IsTouching( pTarget ) )
				{
					if ( !bTouching_SameTeamOnly || ( pRespawnRoom->GetTeamNumber() == TEAM_UNASSIGNED || pRespawnRoom->InSameTeam( pTarget ) ) )
						return true;
				}
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PointsCrossRespawnRoomVisualizer( const Vector& vecStart, const Vector &vecEnd, int nTeamToIgnore )
{
	// Setup the ray.
	Ray_t ray;
	ray.Init( vecStart, vecEnd );

	for ( int i=0; i<IFuncRespawnRoomVisualizerAutoList::AutoList().Count(); ++i )
	{
		CFuncRespawnRoomVisualizer *pEntity = static_cast< CFuncRespawnRoomVisualizer* >( IFuncRespawnRoomVisualizerAutoList::AutoList()[i] );

		if( pEntity->GetTeamNumber() == nTeamToIgnore && nTeamToIgnore != TEAM_UNASSIGNED )
			continue;

		trace_t trace;
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pEntity, &trace );
		if ( trace.fraction < 1.0f )
		{
			return true;
		}
	}

	return false;
}

//===========================================================================================================

LINK_ENTITY_TO_CLASS( func_respawnroomvisualizer, CFuncRespawnRoomVisualizer);

BEGIN_DATADESC( CFuncRespawnRoomVisualizer )
	DEFINE_KEYFIELD( m_iszRespawnRoomName, FIELD_STRING, "respawnroomname" ),
	DEFINE_KEYFIELD( m_bSolid, FIELD_BOOLEAN, "solid_to_enemies" ),
	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetSolid", InputSetSolid ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncRespawnRoomVisualizer, DT_FuncRespawnRoomVisualizer )
END_SEND_TABLE()

CFuncRespawnRoomVisualizer::CFuncRespawnRoomVisualizer()
{
	m_bSolid = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoomVisualizer::Spawn( void )
{
	BaseClass::Spawn();

	SetActive( true );

	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoomVisualizer::InputRoundActivate( inputdata_t &inputdata )
{
	if ( m_iszRespawnRoomName != NULL_STRING )
	{
		m_hRespawnRoom = dynamic_cast<CFuncRespawnRoom*>(gEntList.FindEntityByName( NULL, m_iszRespawnRoomName ));
		if ( m_hRespawnRoom )
		{
			m_hRespawnRoom->AddVisualizer( this );
			ChangeTeam( m_hRespawnRoom->GetTeamNumber() );
		}
		else
		{
			Warning("%s(%s) was unable to find func_respawnroomvisualizer named '%s'\n", GetClassname(), GetDebugName(), STRING(m_iszRespawnRoomName) );
		}
	}

	SetActive( m_bSolid );
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFuncRespawnRoomVisualizer::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"TeamNumber: %d", GetTeamNumber() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 teamcolor = g_aTeamColors[ GetTeamNumber() ];
		teamcolor.a = 0;

		if ( m_hRespawnRoom )
		{
			NDebugOverlay::Line( GetAbsOrigin(), m_hRespawnRoom->WorldSpaceCenter(), teamcolor.r, teamcolor.g, teamcolor.b, false, 0.1 );
		}
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoomVisualizer::InputSetSolid( inputdata_t &inputdata )
{
	m_bSolid = inputdata.value.Bool();

	SetActive( m_bSolid );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncRespawnRoomVisualizer::UpdateTransmitState()
{
	//return SetTransmitState( FL_EDICT_FULLCHECK );

	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncRespawnRoomVisualizer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( !m_hRespawnRoom || m_hRespawnRoom->GetActive() )
	{
		// Respawn rooms are open in win state
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			// Only transmit to enemy players
			CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
			if ( pRecipientEntity->GetTeamNumber() > LAST_SHARED_TEAM && !InSameTeam(pRecipientEntity) )
				return FL_EDICT_ALWAYS;
		}
	}

	return FL_EDICT_DONTSEND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncRespawnRoomVisualizer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Respawn rooms are open in win state
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return false;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			if ( !(contentsMask & CONTENTS_BLUETEAM) )
				return false;
			break;

		case TF_TEAM_RED:
			if ( !(contentsMask & CONTENTS_REDTEAM) )
				return false;
			break;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnRoomVisualizer::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Our ShouldCollide() will make
		// us non-solid to members of the team that spawns here.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );	
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );	
	}
}
