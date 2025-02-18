//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectSentrygun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_dispenser.h"

// NVNT haptics system interface
#include "c_tf_haptics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_HealingList(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_ObjectDispenser *pDispenser = (C_ObjectDispenser*)pStruct;

	CBaseHandle *pHandle = (CBaseHandle*)(&(pDispenser->m_hHealingTargets[pData->m_iElement])); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );

	// update the heal beams
	pDispenser->m_bUpdateHealingTargets = true;
}

void RecvProxyArrayLength_HealingArray( void *pStruct, int objectID, int currentArrayLength )
{
	C_ObjectDispenser *pDispenser = (C_ObjectDispenser*)pStruct;

	if ( pDispenser->m_hHealingTargets.Size() != currentArrayLength )
		pDispenser->m_hHealingTargets.SetSize( currentArrayLength );

	// update the heal beams
	pDispenser->m_bUpdateHealingTargets = true;
}

//-----------------------------------------------------------------------------
// Purpose: Dispenser object
//-----------------------------------------------------------------------------

IMPLEMENT_CLIENTCLASS_DT(C_ObjectDispenser, DT_ObjectDispenser, CObjectDispenser)
	RecvPropInt( RECVINFO( m_iState ) ),
	RecvPropInt( RECVINFO( m_iAmmoMetal ) ),
	RecvPropInt( RECVINFO( m_iMiniBombCounter ) ),

	RecvPropArray2( 
		RecvProxyArrayLength_HealingArray,
		RecvPropInt( "healing_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_HealingList ), 
		MAX_PLAYERS, 
		0, 
		"healing_array"
		)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectDispenser::C_ObjectDispenser()
{
	m_bUpdateHealingTargets = false;
	m_bPlayingSound = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectDispenser::~C_ObjectDispenser()
{
	StopSound( "Building_Dispenser.Heal" );
	// NVNT see if local player is in the list of targets
	// temp. fix if dispener is destroyed will stop all healers.
	if(m_bPlayingSound)
	{
		if(tfHaptics.healingDispenserCount>0) {
			tfHaptics.healingDispenserCount --;
			if(tfHaptics.healingDispenserCount==0 && !tfHaptics.wasBeingHealedMedic)
				tfHaptics.isBeingHealed = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_ObjectDispenser::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );


	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectDispenser::ClientThink()
{
	BaseClass::ClientThink();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectDispenser::SetInvisibilityLevel( float flValue )
{
	if ( IsEnteringOrExitingFullyInvisible( flValue ) )
	{
		UpdateEffects();
	}

	BaseClass::SetInvisibilityLevel( flValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_ObjectDispenser::UpdateEffects( void )
{
	C_TFPlayer *pOwner = GetOwner();

	if ( GetInvisibilityLevel() == 1.f || ( pOwner && pOwner->m_Shared.IsFullyInvisible() ) )
	{
		StopEffects( true );
		return;
	}

	StopEffects();

	// Now add any new targets
	for ( int i = 0; i < m_hHealingTargets.Count(); i++ )
	{
		C_BaseEntity *pTarget = m_hHealingTargets[i].Get();

		// Loops through the healing targets, and make sure we have an effect for each of them
		if ( pTarget )
		{
			// don't want to show this effect for stealthed spies
			C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer * >( pTarget );
			if ( pPlayer && ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) )
				continue;

			bool bHaveEffect = false;
			for ( int targets = 0; targets < m_hHealingTargetEffects.Count(); targets++ )
			{
				if ( m_hHealingTargetEffects[targets].pTarget == pTarget )
				{
					bHaveEffect = true;
					break;
				}
			}

			if ( bHaveEffect )
				continue;
			// NVNT if the dispenser has started to heal the local player
			//   notify the haptics system
			if(pTarget==C_BasePlayer::GetLocalPlayer())
			{
				tfHaptics.healingDispenserCount++;
				if(!tfHaptics.wasBeingHealedMedic) {
					tfHaptics.isBeingHealed = true;
				}
			}

			const char *pszEffectName;
			if ( GetTeamNumber() == TF_TEAM_RED )
			{
				pszEffectName = "dispenser_heal_red";
			}
			else
			{
				pszEffectName = "dispenser_heal_blue";
			}

			CNewParticleEffect *pEffect;

			// if we don't have a model, attach at the origin, otherwise use attachment 'heal_origin'
			if ( FBitSet( GetObjectFlags(), OF_DOESNT_HAVE_A_MODEL ) )
			{
				// offset the origin to player's chest
				if ( FBitSet( GetObjectFlags(), OF_PLAYER_DESTRUCTION ) )
				{
					pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 50 ) );
				}
				else
				{
					pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
				}
			}
			else
			{
				pEffect = ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "heal_origin" );
			}

			ParticleProp()->AddControlPoint( pEffect, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );
			
			int iIndex = m_hHealingTargetEffects.AddToTail();
			m_hHealingTargetEffects[iIndex].pTarget = pTarget;
			m_hHealingTargetEffects[iIndex].pEffect = pEffect;

			// Start the sound over again every time we start a new beam
			StopSound( "Building_Dispenser.Heal" );

			CLocalPlayerFilter filter;
			EmitSound( filter, entindex(), "Building_Dispenser.Heal" );

			m_bPlayingSound = true;
		}
	}

	// Stop the sound if we're not healing anyone
	if ( m_bPlayingSound && m_hHealingTargets.Count() == 0 )
	{
		m_bPlayingSound = false;

		// stop the sound
		StopSound( "Building_Dispenser.Heal" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_ObjectDispenser::StopEffects( bool bRemoveAll /* = false */ )
{
	// Find all the targets we've stopped healing
	bool bStillHealing[MAX_PLAYERS_ARRAY_SAFE] = { 0 };
	for ( int i = 0; i < m_hHealingTargetEffects.Count(); i++ )
	{
		bStillHealing[i] = false;

		// Are we still healing this target?
		if ( !bRemoveAll )
		{
			for ( int target = 0; target < m_hHealingTargets.Count(); target++ )
			{
				if ( m_hHealingTargets[target] && m_hHealingTargets[target] == m_hHealingTargetEffects[i].pTarget )
				{
					bStillHealing[i] = true;
					break;
				}
			}
		}
	}

	// Now remove all the dead effects
	for ( int i = m_hHealingTargetEffects.Count()-1; i >= 0; i-- )
	{
		if ( !bStillHealing[i] )
		{

			// NVNT if the healing target of this dispenser is the local player.
			//   inform the haptics system interface we are no longer healing.
			if(m_hHealingTargetEffects[i].pTarget==C_BasePlayer::GetLocalPlayer())
			{
				if(tfHaptics.healingDispenserCount>0) {
					tfHaptics.healingDispenserCount --;
					if(tfHaptics.healingDispenserCount==0 && !tfHaptics.wasBeingHealedMedic)
						tfHaptics.isBeingHealed = false;
				}
			}

			ParticleProp()->StopEmission( m_hHealingTargetEffects[i].pEffect );
			m_hHealingTargetEffects.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectDispenser::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_hDamageEffects )
	{
		m_hDamageEffects->StopEmission( false, false );
		m_hDamageEffects = NULL;
	}

	const char *pszEffect = "";

	switch( damageLevel )
	{
	case BUILDING_DAMAGE_LEVEL_LIGHT:
		pszEffect = "dispenserdamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "dispenserdamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "dispenserdamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "dispenserdamage_4";
		break;

	default:
		break;
	}

	if ( Q_strlen(pszEffect) > 0 )
	{
		m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int C_ObjectDispenser::GetMaxMetal( void )
{
	return DISPENSER_MAX_METAL_AMMO;
}

//-----------------------------------------------------------------------------
// Control screen 
//-----------------------------------------------------------------------------

DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel, "screen_obj_dispenser_blue" );
DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel_Red, "screen_obj_dispenser_red" );

//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CDispenserControlPanel::CDispenserControlPanel( vgui::Panel *parent, const char *panelName )
: BaseClass( parent, "CDispenserControlPanel" ) 
{
	m_pAmmoProgress = new RotatingProgressBar( this, "MeterArrow" );
}

//-----------------------------------------------------------------------------
// Deactivates buttons we can't afford
//-----------------------------------------------------------------------------
void CDispenserControlPanel::OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer )
{
	BaseClass::OnTickActive( pObj, pLocalPlayer );

	Assert( dynamic_cast< C_ObjectDispenser* >( pObj ) );
	m_hDispenser = static_cast< C_ObjectDispenser* >( pObj );

	float flProgress = m_hDispenser ? m_hDispenser->GetMetalAmmoCount() / (float)m_hDispenser->GetMaxMetal() : 0.f;

	m_pAmmoProgress->SetProgress( flProgress );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispenserControlPanel::IsVisible( void )
{
	if ( m_hDispenser )
	{

		if ( m_hDispenser->GetInvisibilityLevel() == 1.f )
			return false;
	}

	return BaseClass::IsVisible();
}

IMPLEMENT_CLIENTCLASS_DT(C_ObjectCartDispenser, DT_ObjectCartDispenser, CObjectCartDispenser)
END_RECV_TABLE()