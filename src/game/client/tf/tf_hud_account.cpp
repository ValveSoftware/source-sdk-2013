//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "view_scene.h"
#include "view.h"
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "tf_weapon_invis.h"
#include <vgui_controls/AnimationController.h>

#include "c_tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ISoundEmitterSystemBase *soundemitterbase;

// Floating delta text items, float off the top of the frame to 
// show changes to the metal account value
typedef struct 
{
	enum eAccountDeltaType_t
	{
		ACCOUNT_DELTA_INVALID,
		ACCOUNT_DELTA_HEALING,
		ACCOUNT_DELTA_DAMAGE,
		ACCOUNT_DELTA_BONUS_POINTS,
		ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_RED,
		ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_BLUE,
	};



	// amount of delta
	int m_iAmount;
	
	bool m_bLargeFont;		// display larger font
	eAccountDeltaType_t m_eDataType;

	// die time
	float m_flDieTime;

	// position
	int m_nX;				// X Pos in screen space & world space
	int m_nXEnd;			// Ending X Pos in screen space and world space
	int m_nHStart;			// Starting Y Pos in screen space, Z pos in world space
	int m_nHEnd;			// Ending Y Pos in screen space, Z pos in world space
	int m_nY;				// Y Coord in world space, not used in screen space
	bool m_bWorldSpace;
	float m_flBatchWindow;
	int m_nSourceID;		// Can be entindex, etc
	Color m_color;
	bool m_bShadows;

	// append a bit of extra text to the end
	wchar_t m_wzText[8];

} account_delta_t;

#define NUM_ACCOUNT_DELTA_ITEMS 10

ConVar hud_combattext( "hud_combattext", "1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_healing( "hud_combattext_healing", "1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Shows health restored per-second over heal targets." );
ConVar hud_combattext_batching( "hud_combattext_batching", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "If set to 1, numbers that are too close together are merged." );
ConVar hud_combattext_batching_window( "hud_combattext_batching_window", "0.2", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Maximum delay between damage events in order to batch numbers.", true, 0.1, true, 2.0 );
ConVar hud_combattext_doesnt_block_overhead_text( "hud_combattext_doesnt_block_overhead_text", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "If set to 1, allow text like \"CRIT\" to still show over a victim's head." );
ConVar hud_combattext_red( "hud_combattext_red", "255", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_green( "hud_combattext_green", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_blue( "hud_combattext_blue", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

ConVar tf_dingalingaling( "tf_dingalingaling", "0", FCVAR_ARCHIVE, "If set to 1, play a sound everytime you injure an enemy. The sound can be customized by replacing the 'tf/sound/ui/hitsound.wav' file." );
ConVar tf_dingaling_volume( "tf_dingaling_volume", "0.75", FCVAR_ARCHIVE, "Desired volume of the hit sound.", true, 0.0, true, 1.0 );
ConVar tf_dingaling_pitchmindmg( "tf_dingaling_pitchmindmg", "100", FCVAR_ARCHIVE, "Desired pitch of the hit sound when a minimal damage hit (<= 10 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_pitchmaxdmg( "tf_dingaling_pitchmaxdmg", "100", FCVAR_ARCHIVE, "Desired pitch of the hit sound when a maximum damage hit (>= 150 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_pitch_override( "tf_dingaling_pitch_override", "-1", FCVAR_NONE, "If set, pitch for all hit sounds." );

ConVar tf_dingalingaling_lasthit( "tf_dingalingaling_lasthit", "0", FCVAR_ARCHIVE, "If set to 1, play a sound whenever one of your attacks kills an enemy. The sound can be customized by replacing the 'tf/sound/ui/killsound.wav' file." );
ConVar tf_dingaling_lasthit_volume( "tf_dingaling_lasthit_volume", "0.75", FCVAR_ARCHIVE, "Desired volume of the last hit sound.", true, 0.0, true, 1.0 );
ConVar tf_dingaling_lasthit_pitchmindmg( "tf_dingaling_lasthit_pitchmindmg", "100", FCVAR_ARCHIVE, "Desired pitch of the last hit sound when a minimal damage hit (<= 10 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_lasthit_pitchmaxdmg( "tf_dingaling_lasthit_pitchmaxdmg", "100", FCVAR_ARCHIVE, "Desired pitch of the last hit sound when a maximum damage hit (>= 150 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_lasthit_pitch_override( "tf_dingaling_lasthit_pitch_override", "-1", FCVAR_NONE, "If set, pitch for last hit sounds." );

ConVar tf_dingalingaling_repeat_delay( "tf_dingalingaling_repeat_delay", "0.0", FCVAR_ARCHIVE, "Desired repeat delay of the hit sound.  Set to 0 to play a sound for every instance of damage dealt.", true, 0.f, false, 0.f );

ConVar hud_damagemeter( "hud_damagemeter", "0", FCVAR_CHEAT, "Display damage-per-second information in the lower right corner of the screen." );
ConVar hud_damagemeter_period( "hud_damagemeter_period", "0", FCVAR_NONE, "When set to zero, average damage-per-second across all recent damage events, otherwise average damage across defined period (number of seconds)." );
ConVar hud_damagemeter_ooctimer( "hud_damagemeter_ooctimer", "1", FCVAR_NONE, "How many seconds after the last damage event before we consider the player out of combat." );
ConVar hud_damagemeter_report( "hud_damagemeter_report", "1", FCVAR_NONE, "Display end-of-combat DPS result (from first damage even to last before OOC timer hit)." );

struct hitsound_params_t
{
	hitsound_params_t( const char * pszName, int minpitch, int maxpitch )
	{
		m_pszName = pszName;
		m_iMinPitch = minpitch;
		m_iMaxPitch = maxpitch;
	}

	float GetPitchMin( bool bLastHit ) const
	{
		return bLastHit ? tf_dingaling_lasthit_pitchmindmg.GetInt() : tf_dingaling_pitchmindmg.GetInt();
		//return RemapValClamped( tf_dingaling_pitchmindmg.GetInt(), 0, 100, m_iMinPitch, m_iMaxPitch );
	}

	float GetPitchMax( bool bLastHit ) const
	{
		return bLastHit ? tf_dingaling_lasthit_pitchmaxdmg.GetInt() : tf_dingaling_pitchmaxdmg.GetInt();
		//return RemapValClamped( tf_dingaling_pitchmaxdmg.GetInt(), 0, 100, m_iMinPitch, m_iMaxPitch );
	}

	float GetPitchFromDamage( int damage, bool bLastHit ) const
	{
		if ( bLastHit && tf_dingaling_lasthit_pitch_override.GetInt() > 0 )
		{
			return tf_dingaling_lasthit_pitch_override.GetFloat();
		}
		else if ( tf_dingaling_pitch_override.GetInt() > 0 )
		{
			return tf_dingaling_pitch_override.GetFloat();
		}

		return RemapValClamped( damage, 10, 150, GetPitchMin( bLastHit ), GetPitchMax( bLastHit ) );
	}

	const char *m_pszName;
	int m_iMinPitch;
	int m_iMaxPitch;
};

static const hitsound_params_t g_HitSounds[] =
{
	hitsound_params_t( "Player.HitSoundDefaultDing",		1,			255 ),
	hitsound_params_t( "Player.HitSoundElectro",			1,			255 ),
	hitsound_params_t( "Player.HitSoundNotes",				1,			255 ),
	hitsound_params_t( "Player.HitSoundPercussion",			1,			255 ),
	hitsound_params_t( "Player.HitSoundRetro",				1,			255 ),
	hitsound_params_t( "Player.HitSoundSpace",				1,			255 ),
	hitsound_params_t( "Player.HitSoundBeepo",				1,			255 ),
	hitsound_params_t( "Player.HitSoundVortex",				1,			255 ),
	hitsound_params_t( "Player.HitSoundSquasher",			1,			255 ),
};

static const hitsound_params_t g_LastHitSounds[] =
{
	hitsound_params_t( "Player.KillSoundDefaultDing", 1, 255 ),
	hitsound_params_t( "Player.KillSoundElectro", 1, 255 ),
	hitsound_params_t( "Player.KillSoundNotes", 1, 255 ),
	hitsound_params_t( "Player.KillSoundPercussion", 1, 255 ),
	hitsound_params_t( "Player.KillSoundRetro", 1, 255 ),
	hitsound_params_t( "Player.KillSoundSpace", 1, 255 ),
	hitsound_params_t( "Player.KillSoundBeepo", 1, 255 ),
	hitsound_params_t( "Player.KillSoundVortex", 1, 255 ),
	hitsound_params_t( "Player.KillSoundSquasher", 1, 255 ),
};

ConVar tf_dingalingaling_effect( "tf_dingalingaling_effect", "0", FCVAR_ARCHIVE, "Which Dingalingaling sound is used", true, 0, true, ARRAYSIZE( g_HitSounds )-1 );
ConVar tf_dingalingaling_last_effect( "tf_dingalingaling_last_effect", "0", FCVAR_ARCHIVE, "Which final hit sound to play when the target expires.", true, 0, true, ARRAYSIZE( g_LastHitSounds )-1 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAccountPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAccountPanel, EditablePanel );

public:
	CAccountPanel( Panel *parent, const char *name )
		: EditablePanel( parent, name )
	{
		m_nBGTexture = -1;
		m_bNegativeFlipDir = false;
		SetDialogVariable( "metal", 0 );
	}

	virtual void	ApplySchemeSettings( IScheme *scheme ) OVERRIDE;
	virtual void	ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void	Paint( void ) OVERRIDE;

	virtual account_delta_t *OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type );

	virtual const char *GetResFileName( void ) { return "resource/UI/HudAccountPanel.res"; }

protected:
	virtual Color GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue = 0 );

	CUtlVector <account_delta_t> m_AccountDeltaItems;

	int m_nBGTexture;
	bool m_bNegativeFlipDir;

	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemXEndPos, "delta_item_end_x", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBGImageX, "bg_image_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageY, "bg_image_y", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageWide, "bg_image_wide", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageTall, "bg_image_tall", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );
	CPanelAnimationVar( Color, m_DeltaEventColor, "EventColor", "255 0 255 255" );
	CPanelAnimationVar( Color, m_DeltaRedRobotScoreColor, "RedRobotScoreColor", "255 0 0 255" );
	CPanelAnimationVar( Color, m_DeltaBlueRobotScoreColor, "BlueRobotScoreColor", "0 166 255 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFontBig, "delta_item_font_big", "Default" );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudAccountPanel : public CHudElement, public CAccountPanel
{
	DECLARE_CLASS_SIMPLE( CHudAccountPanel, CAccountPanel );

public:
	CHudAccountPanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, CAccountPanel( NULL, pElementName )
	{
		Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_METAL );
		ListenForGameEvent( "player_account_changed" );
	}

	virtual void LevelInit( void ) OVERRIDE
	{
		CHudElement::LevelInit();
	}

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		const char * type = event->GetName();

		if ( Q_strcmp(type, "player_account_changed") == 0 )
		{
			int iOldValue = event->GetInt( "old_account" );
			int iNewValue = event->GetInt( "new_account" );
			account_delta_t::eAccountDeltaType_t deltaType = ( iNewValue - iOldValue >= 0 ) ? account_delta_t::ACCOUNT_DELTA_HEALING
																							: account_delta_t::ACCOUNT_DELTA_DAMAGE;

			OnAccountValueChanged( iOldValue, iNewValue, deltaType );
		}
		else
		{
			CHudElement::FireGameEvent( event );
		}
	}

	virtual bool ShouldDraw( void ) OVERRIDE
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() || !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			m_AccountDeltaItems.RemoveAll();
			return false;
		}

		CTFPlayer *pTFPlayer = CTFPlayer::GetLocalTFPlayer();
		if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;
		
		if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
			return false;

		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return false;

		return CHudElement::ShouldDraw();
	}
};

DECLARE_HUDELEMENT( CHudAccountPanel );

// Derived account panels
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHealthAccountPanel : public CHudAccountPanel
{
	DECLARE_CLASS_SIMPLE( CHealthAccountPanel, CHudAccountPanel );
public:
	CHealthAccountPanel( const char *pElementName ) : CHudAccountPanel(pElementName)
	{
		ListenForGameEvent( "player_healonhit" );
		ListenForGameEvent( "building_healed" );
	}

	virtual const char *GetResFileName( void ) { return "resource/UI/HudHealthAccount.res"; }

	void FireGameEvent( IGameEvent *event )
	{
		const char * type = event->GetName();

		if ( Q_strcmp(type, "player_healonhit") == 0 )
		{
			int iAmount = event->GetInt( "amount" );
			int iPlayer = event->GetInt( "entindex" );
			CTFPlayer *pEventPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayer ) );
			if ( pEventPlayer && !pEventPlayer->IsDormant() )
			{
				if ( pEventPlayer == C_TFPlayer::GetLocalTFPlayer() )
				{
					OnAccountValueChanged( 0, iAmount, account_delta_t::ACCOUNT_DELTA_HEALING );
				}
				else
				{
					CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
					if ( pLocalPlayer )
					{
						bool bEnemySpy = !pLocalPlayer->InSameTeam( pEventPlayer ) && pEventPlayer->IsPlayerClass( TF_CLASS_SPY );
						bool bSneakyEnemySpy = bEnemySpy && ( pEventPlayer->m_Shared.IsStealthed() || pLocalPlayer->m_Shared.IsSpyDisguisedAsMyTeam( pEventPlayer ) );
						bool bShouldSpawnRedParticle = ( pEventPlayer->GetTeamNumber() == TF_TEAM_RED );
						if ( bSneakyEnemySpy )
						{
							bShouldSpawnRedParticle = ( GetLocalPlayerTeam() == TF_TEAM_RED );
						}

						const char *pEffectName;
						if ( iAmount < 0 )
						{
							pEffectName = bShouldSpawnRedParticle ? "healthlost_red" : "healthlost_blu";
						}
						else if ( iAmount >= 100 )
						{
							if ( pEventPlayer->IsMiniBoss() )
							{
								pEffectName = bShouldSpawnRedParticle ? "healthgained_red_giant" : "healthgained_blu_giant";

							}
							else
							{
								pEffectName = bShouldSpawnRedParticle ? "healthgained_red_large" : "healthgained_blu_large";
							}
						}
						else
						{
							pEffectName = bShouldSpawnRedParticle ? "healthgained_red" : "healthgained_blu";
						}

						pEventPlayer->ParticleProp()->Create( pEffectName, PATTACH_POINT, "head" );
					}
				}
			}
		}
		else if ( FStrEq( event->GetName(), "building_healed" ) )
		{
			CBaseEntity *pBuilding = ClientEntityList().GetEnt( event->GetInt( "building" ) );
			if ( pBuilding )
			{
				bool bRedParticle = ( pBuilding->GetTeamNumber() == TF_TEAM_RED );
				const char *pszEffectName = ( bRedParticle ) ? "healthgained_red_large" : "healthgained_blu_large";
				pBuilding->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN, INVALID_PARTICLE_ATTACHMENT, Vector( 0, 0, 32 ) );
			}
		}
		else
		{
			CHudElement::FireGameEvent( event );
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool ShouldDraw( void )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			m_AccountDeltaItems.RemoveAll();
		}

		if ( !m_AccountDeltaItems.Count() )
			return false;

		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
			return false;

		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return false;

		return CHudElement::ShouldDraw();
	}
};
DECLARE_HUDELEMENT( CHealthAccountPanel );

class CScoreAccountPanel : public CAccountPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CScoreAccountPanel, CAccountPanel );
public:
	CScoreAccountPanel( Panel *parent, const char *name )
		: CAccountPanel( parent, name )
		, m_nTeam( TF_TEAM_COUNT )
	{}

	virtual const char *GetResFileName( void ) { return "resource/UI/HudScoreAccount.res"; }

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		const char * pszEventName = event->GetName();

		if ( Q_strcmp(pszEventName, m_pszEventName) == 0 )
		{
			CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
				return;

			int nTeam = event->GetInt( "team" );
			if ( m_nTeam != nTeam )
				return;

			const int nPoints = event->GetInt( "points" );
			if ( !nPoints )
				return;

			account_delta_t* pNewAccount = OnAccountValueChanged( 0, nPoints, account_delta_t::ACCOUNT_DELTA_BONUS_POINTS );
			if ( pNewAccount )
			{
				pNewAccount->m_bShadows = true;
				pNewAccount->m_flBatchWindow = pNewAccount->m_flDieTime;
				pNewAccount->m_nSourceID = (( nPoints > 0 ) ? 0 : 1 ) + (nTeam * 2);

				if ( ( GetLocalPlayerTeam() == nTeam && nPoints > 0 ) || ( GetLocalPlayerTeam() != nTeam && nPoints < 0 ) )
				{
					pNewAccount->m_color = m_DeltaPositiveColor;
				}
				else
				{
					pNewAccount->m_color = m_DeltaNegativeColor;
				}
			}
		}
	}

	void ApplySettings( KeyValues *inResourceData )
	{
		BaseClass::ApplySettings( inResourceData );

		Q_strncpy( m_pszEventName, inResourceData->GetString( "event" ), sizeof( m_pszEventName ) );
		if ( *m_pszEventName )
		{
			ListenForGameEvent( m_pszEventName );
		}

		const char *pszTeam = inResourceData->GetString( "team" );
		if ( Q_stricmp( pszTeam, "red" ) == 0 )
		{
			m_nTeam = TF_TEAM_RED;
		}
		else
		{
			m_nTeam = TF_TEAM_BLUE;
		}
	}

private:
	char m_pszEventName[32]; // max length of event names
	int	 m_nTeam;
};

DECLARE_BUILD_FACTORY( CScoreAccountPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDamageAccountPanel : public CHudAccountPanel
{
	DECLARE_CLASS_SIMPLE( CDamageAccountPanel, CHudAccountPanel );
public:
	CDamageAccountPanel( const char *pElementName ) : CHudAccountPanel(pElementName)
	{
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "npc_hurt" );
		ListenForGameEvent( "player_healed" );
		ListenForGameEvent( "player_bonuspoints" );
		ListenForGameEvent( "building_healed" );

		ResetDamageVars();
		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	}

	virtual void OnTick( void );
	virtual const char *GetResFileName( void ) { return "resource/UI/HudDamageAccount.res"; }
	virtual void Paint( void );


	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool ShouldDrawDPSMeter( void )
	{
		return hud_damagemeter.GetBool();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void DisplayDamageFeedback( CTFPlayer *pAttacker, CBaseCombatCharacter *pVictim, int iDamage, int iHealth, bool bIsCrit )
	{
		if ( iDamage <= 0 ) // zero value (invuln?)
			return;

		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
			return;

		if ( !pAttacker || !pVictim )
			return;

		// Show the attacker, or when healing the player that is
		if ( ( pAttacker == pLocalPlayer )  ||
			 ( pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && ( pLocalPlayer->MedicGetHealTarget() == pAttacker ) ) )
		{
			bool bDeadRingerSpy = false;
			C_TFPlayer *pVictimPlayer = ToTFPlayer( pVictim );
			if ( pVictimPlayer )
			{
				// Player hurt self
				if ( pAttacker == pVictimPlayer )
					return;

				// Don't show damage on stealthed and/or disguised enemy spies
				if ( pVictimPlayer->IsPlayerClass( TF_CLASS_SPY ) && pVictimPlayer->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
				{
					CTFWeaponInvis *pWpn = (CTFWeaponInvis *)pVictimPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
					if ( pWpn && pWpn->HasFeignDeath() )
					{
						if ( pVictimPlayer->m_Shared.IsFeignDeathReady() )
						{
							bDeadRingerSpy = true;
						}
					}

					if ( !bDeadRingerSpy )
					{
						if ( pVictimPlayer->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() || pVictimPlayer->m_Shared.IsStealthed() )
							return;
					}
				}
			}

			if ( pAttacker == pLocalPlayer )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "DamagedPlayer" );

				bool bHitEnabled = ( tf_dingalingaling.GetBool() );
				bool bLastHitEnabled = ( tf_dingalingaling_lasthit.GetBool() );
				bool bLastHit = ( iHealth <= 0 ) || bDeadRingerSpy;
				if ( bLastHitEnabled && bLastHit )
				{
					// Always allow the last hit sound
					m_flLastDingTime = 0.f;
				}
				
				// Play hitbeeps 
				if ( ( bHitEnabled || bLastHitEnabled ) && 
					 ( gpGlobals->curtime > ( m_flLastDingTime + tf_dingalingaling_repeat_delay.GetFloat() ) || tf_dingalingaling_repeat_delay.GetFloat() == 0.f ) )
				{
					m_flLastDingTime = gpGlobals->curtime;

					CSoundParameters params;
					CLocalPlayerFilter filter;
					const char *pszSound = NULL;
					const hitsound_params_t *pHitSound = NULL;

					if ( bLastHit && bLastHitEnabled )
					{
						pszSound = g_LastHitSounds[tf_dingalingaling_last_effect.GetInt()].m_pszName;
						pHitSound = &g_LastHitSounds[tf_dingalingaling_last_effect.GetInt()];
						if ( pszSound && pHitSound && CBaseEntity::GetParametersForSound( pszSound, params, NULL ) )
						{
							EmitSound_t es( params );
							es.m_nPitch = pHitSound->GetPitchFromDamage( iDamage, bLastHit );
							es.m_flVolume = tf_dingaling_lasthit_volume.GetFloat();
							pLocalPlayer->EmitSound( filter, pLocalPlayer->entindex(), es );
						}
					}
					else if ( bHitEnabled )
					{
						pszSound = g_HitSounds[tf_dingalingaling_effect.GetInt()].m_pszName;
						pHitSound = &g_HitSounds[tf_dingalingaling_effect.GetInt()];
						if ( pszSound && pHitSound && CBaseEntity::GetParametersForSound( pszSound, params, NULL ) )
						{
							EmitSound_t es( params );
							es.m_nPitch = pHitSound->GetPitchFromDamage( iDamage, false );
							es.m_flVolume = tf_dingaling_volume.GetFloat();
							pLocalPlayer->EmitSound( filter, pLocalPlayer->entindex(), es );
						}
					}
				}
			}

			if ( hud_combattext.GetBool() )
			{
				// Ignore damage events on targets that we can't see, so it's not a cheat
				trace_t	tr;
				UTIL_TraceLine( pVictim->WorldSpaceCenter(), MainViewOrigin(), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction >= 1.f )
				{
					account_delta_t *pNewAccount = OnAccountValueChanged( 0, -iDamage, account_delta_t::ACCOUNT_DELTA_DAMAGE );
					if ( pNewAccount )
					{
						Vector vecPos = pVictim->GetAbsOrigin();
						Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
						int nHeightoffset = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 16 );
						vecPos.z += (VEC_HULL_MAX_SCALED( pVictim ).z + nHeightoffset);
						pNewAccount->m_nX = vecPos.x;
						pNewAccount->m_nXEnd = pNewAccount->m_nX;
						pNewAccount->m_nY = vecPos.y;
						pNewAccount->m_nHStart = vecPos.z;
						pNewAccount->m_nHEnd = pNewAccount->m_nHStart + 32;	// How many units to float up
						pNewAccount->m_bWorldSpace = true;
						pNewAccount->m_nSourceID = pVictim->entindex();
						pNewAccount->m_flBatchWindow = hud_combattext_batching.GetBool() ? hud_combattext_batching_window.GetFloat() : 0.f;
						pNewAccount->m_bLargeFont = bIsCrit;
						//	V_swprintf_safe( pNewAccount->m_wzText, L" (%d)", m_nQueuedDamageEvents );
					}
				}
			}

			m_flLastDamageEventTime = gpGlobals->curtime;

			// Damage meter tracking
			if ( hud_damagemeter_period.GetFloat() > 0.f )
			{
				// Store events and average across a sliding window
				DamageHistory_t damage = { (float)iDamage, m_flLastDamageEventTime };
				m_DamageHistory.AddToTail( damage );
			}
			else
			{
				// Running tally until we hit the out-of-combat timer
				if ( m_flFirstDamageEventTime == 0.f )
				{
					m_flFirstDamageEventTime = m_flLastDamageEventTime;
					m_flDamagePerSecond = 0.f;
					m_flDamageMeterTotal = 0.f;
				}
				
				m_flDamageMeterTotal += iDamage;
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void FireGameEvent( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_hurt" ) )
		{
			const int iDamage = event->GetInt( "damageamount" );
			const int iHealth = event->GetInt( "health" );

			const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			C_TFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( iAttacker ) );

			const int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			C_TFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictim ) );

			EAttackBonusEffects_t eBonusEffect = (EAttackBonusEffects_t)event->GetInt( "bonuseffect", (int)kBonusEffect_None );
			bool bLargeText = g_BonusEffects[ eBonusEffect ].m_bLargeCombatText;
			if ( eBonusEffect == kBonusEffect_None )
			{
				bLargeText |= event->GetBool( "crit", false );
				bLargeText |= event->GetBool( "minicrit", false );
			}

			DisplayDamageFeedback( pAttacker, pVictim, iDamage, iHealth, bLargeText );
		}
		else if ( FStrEq( event->GetName(), "npc_hurt" ) )
		{
			const int iDamage = event->GetInt( "damageamount" );
			const int iHealth = event->GetInt( "health" );

			const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker_player" ) );
			C_TFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( iAttacker ) );

			C_BaseCombatCharacter *pVictim = (C_BaseCombatCharacter *)ClientEntityList().GetClientEntity( event->GetInt( "entindex" ) );
			
			DisplayDamageFeedback( pAttacker, pVictim, iDamage, iHealth, event->GetBool( "crit", 0 ) );
		}
		else if ( FStrEq( event->GetName(), "player_healed" ) )
		{
			if ( hud_combattext.GetBool() && hud_combattext_healing.GetBool() )
			{
				CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
					return;

				const int iHealer = engine->GetPlayerForUserID( event->GetInt( "healer" ) );
				CBasePlayer *pHealer = UTIL_PlayerByIndex( iHealer );
				if ( pHealer && pHealer == pLocalPlayer )
				{
					const int iPatient = engine->GetPlayerForUserID( event->GetInt( "patient" ) );
					CBasePlayer *pPatient = UTIL_PlayerByIndex( iPatient );

					if ( pPatient )
					{
						const int iHealedAmt = event->GetInt( "amount" );

						account_delta_t *pNewAccount = OnAccountValueChanged( 0, iHealedAmt, account_delta_t::ACCOUNT_DELTA_HEALING );
						if ( pNewAccount )
						{
							Vector vecPos = pPatient->GetAbsOrigin();
							Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
							int nHeightoffset = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 16 );
							vecPos.z += ( VEC_HULL_MAX_SCALED( pPatient ).z + nHeightoffset );
							pNewAccount->m_nX = vecPos.x;
							pNewAccount->m_nXEnd = pNewAccount->m_nX;
							pNewAccount->m_nY = vecPos.y;
							pNewAccount->m_nHStart = vecPos.z;
							pNewAccount->m_nHEnd = pNewAccount->m_nHStart + 32;		// Float 32 units up in worldspace
							pNewAccount->m_bWorldSpace = true;
						}
					}
				}
			}
		}
		else if ( FStrEq( event->GetName(), "player_bonuspoints" ) )
		{
			if ( hud_combattext.GetBool() )
			{
				CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
					return;

				const int nPoints = ( event->GetInt( "points" ) / 10 );
				if ( !nPoints )
					return;

				const int iPlayer = event->GetInt( "player_entindex" );
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayer );
				if ( pPlayer && pPlayer == pLocalPlayer )
				{
					const int iSource = event->GetInt( "source_entindex" );
					C_BaseEntity *pSource = ClientEntityList().GetBaseEntity( iSource );
					if ( !pSource )
						return;

					account_delta_t *pNewAccount = OnAccountValueChanged( 0, nPoints, account_delta_t::ACCOUNT_DELTA_BONUS_POINTS );
					if ( pNewAccount )
					{
						Vector vecPos = pSource->GetAbsOrigin();
						Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
						int nHeightoffset = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 16 );
						vecPos.z += ( pSource->IsPlayer() ) ? (VEC_HULL_MAX_SCALED( pSource->GetBaseAnimating() ).z + nHeightoffset) : 0;
						pNewAccount->m_nX = vecPos.x;
						pNewAccount->m_nXEnd = pNewAccount->m_nX;
						pNewAccount->m_nY = vecPos.y;
						pNewAccount->m_nHStart = vecPos.z;
						pNewAccount->m_nHEnd = pNewAccount->m_nHStart + 16;
						pNewAccount->m_bWorldSpace = true;
					}
				}
			}
		}
		else if ( FStrEq( event->GetName(), "building_healed" ) )
		{
			if ( !hud_combattext.GetBool() || !hud_combattext_healing.GetBool() )
				return;

			CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
				return;

			CBaseEntity *pHealer = ClientEntityList().GetEnt( event->GetInt( "healer" ) );
			if ( pHealer && pHealer == pLocalPlayer )
			{
				CBaseEntity *pBuilding = ClientEntityList().GetEnt( event->GetInt( "building" ) );
				if ( !pBuilding )
					return;

				const int iHealedAmt = event->GetInt( "amount" );

				account_delta_t *pNewAccount = OnAccountValueChanged( 0, iHealedAmt, account_delta_t::ACCOUNT_DELTA_HEALING );
				if ( pNewAccount )
				{
					Vector vecPos = pBuilding->GetAbsOrigin();
					Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
					int nHeightoffset = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 16 );
					vecPos.z += ( 64 + nHeightoffset );
					pNewAccount->m_nX = vecPos.x;
					pNewAccount->m_nXEnd = pNewAccount->m_nX;
					pNewAccount->m_nY = vecPos.y;
					pNewAccount->m_nHStart = vecPos.z;
					pNewAccount->m_nHEnd = pNewAccount->m_nHStart + 32;		// Float 32 units up in worldspace
					pNewAccount->m_bWorldSpace = true;
				}
				
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool ShouldDraw( void )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			m_AccountDeltaItems.RemoveAll();
		}

		if ( ShouldDrawDPSMeter() )
			return true;

		if ( !m_AccountDeltaItems.Count() )
			return false;

		return CHudElement::ShouldDraw();
	}

	//-----------------------------------------------------------------------------
	// Purpose: called whenever a new level is starting
	//-----------------------------------------------------------------------------
	virtual void LevelInit( void ) OVERRIDE
	{
		ResetDamageVars();

		BaseClass::LevelInit();
	}

private:

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void ResetDamageVars( void )
	{
		m_flFirstDamageEventTime = 0.f;
		m_flLastDamageEventTime = 0.f;
		m_flDamagePerSecond = 0.f;
		m_flDamageMeterTotal = 0.f;
		m_flLastDingTime = 0.f;
	}

private:

	// DamageMeter
	float				m_flFirstDamageEventTime;
	float				m_flLastDamageEventTime;
	float				m_flDamagePerSecond;
	float				m_flDamageMeterTotal;
	struct DamageHistory_t
	{
		float flDamage;
		float flDamageTime;
	};
	CUtlVector< DamageHistory_t >	m_DamageHistory;

	// Dings
	float				m_flLastDingTime;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDamageAccountPanel::OnTick( void )
{
	if ( ShouldDrawDPSMeter() )
	{
		// We're out of combat - nuke everything
		if ( m_flFirstDamageEventTime != 0.f && m_flLastDamageEventTime < gpGlobals->curtime - hud_damagemeter_ooctimer.GetFloat() )
		{
			if ( hud_damagemeter_report.GetBool() )
			{
				DevMsg( "-------\n" );
				DevMsg( "%3.2f DPS over %3.2f seconds\n" , m_flDamagePerSecond, ( m_flLastDamageEventTime - m_flFirstDamageEventTime ) );
				DevMsg( "-------\n" );
			}
			m_DamageHistory.RemoveAll();
			m_flFirstDamageEventTime = 0.f;
		}
		else
		{
			float flPeriod = hud_damagemeter_period.GetFloat();
			
			// Period-based calculation (averaged across a defined range)
			if ( flPeriod > 0.f )
			{
				m_flDamageMeterTotal = 0.f;

				FOR_EACH_VEC_BACK( m_DamageHistory, i )
				{
					if ( flPeriod > 0.f )
					{
						// This method averages across a fixed period, so nuke entires outside the period
						if ( gpGlobals->curtime - flPeriod > m_DamageHistory[i].flDamageTime )
						{
							m_DamageHistory.Remove( i );
							continue;
						}
					}

					// What's left is within the period (sliding window)
					m_flDamageMeterTotal += m_DamageHistory[i].flDamage;
				}
				
				m_flDamagePerSecond = m_flDamageMeterTotal / flPeriod;
			}
			// Event-based calculation (absolute dps)
			else if ( m_flFirstDamageEventTime > 0.f )
			{
				flPeriod = Max( m_flLastDamageEventTime - m_flFirstDamageEventTime, 0.01f );
				m_flDamagePerSecond = m_flDamageMeterTotal / flPeriod;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDamageAccountPanel::Paint( void )
{
	BaseClass::Paint();

	if ( ShouldDrawDPSMeter() )
	{
		int iScreenWide, iScreenTall;
		GetHudSize( iScreenWide, iScreenTall );
		int nX = iScreenWide / 1.15;
		int nY = iScreenTall / 1.20;

		int r = 255, g = 255, b = 255;
		if ( m_flLastDamageEventTime < gpGlobals->curtime - hud_damagemeter_ooctimer.GetFloat() )
		{
			r = 255, g = 0, b = 0;
		}
		Color cDPS( r, g, b, 255 );

		vgui::surface()->DrawSetTextFont( m_hDeltaItemFontBig );
		vgui::surface()->DrawSetTextColor( cDPS );
		vgui::surface()->DrawSetTextPos( nX, nY );

		wchar_t wDPSBuf[20];
		V_swprintf_safe( wDPSBuf, L"%d DPS", (int)m_flDamagePerSecond );
		vgui::surface()->DrawPrintText( wDPSBuf, wcslen( wDPSBuf ), FONT_DRAW_NONADDITIVE );
	}
}

DECLARE_HUDELEMENT( CDamageAccountPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAccountPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFileName() );

	BaseClass::ApplySchemeSettings( pScheme );
}

void CAccountPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Backwards compatibility.  If we DONT find "delta_item_end_x" specified in the keyvalues,
	// then just take the starting x-pos as the ending x-pos.
	if ( inResourceData->FindKey( "delta_item_end_x" ) == NULL )
	{
		m_flDeltaItemXEndPos = m_flDeltaItemX;
	}

	m_bNegativeFlipDir = inResourceData->FindKey( "negative_flip_dir", false );

	const char *pszBGTextureName = inResourceData->GetString( "bg_texture", NULL );
	if ( m_nBGTexture == -1 && pszBGTextureName && pszBGTextureName[0] )
	{
		m_nBGTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nBGTexture , pszBGTextureName, true, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
account_delta_t *CAccountPanel::OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type )
{
	// update the account value
	SetDialogVariable( "metal", iNewValue ); 

	int iDelta = iNewValue - iOldValue;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( iDelta != 0 && pPlayer && pPlayer->IsAlive() )
	{
		int index = m_AccountDeltaItems.AddToTail();
		account_delta_t *pNewDeltaItem = &m_AccountDeltaItems[index];

		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_iAmount = iDelta;
		pNewDeltaItem->m_nX = m_flDeltaItemX;
		pNewDeltaItem->m_nXEnd = m_flDeltaItemXEndPos;
		pNewDeltaItem->m_nHStart = m_flDeltaItemStartPos;
		pNewDeltaItem->m_nHEnd = m_flDeltaItemEndPos;
		pNewDeltaItem->m_bWorldSpace = false;
		pNewDeltaItem->m_nSourceID = -1;
		pNewDeltaItem->m_flBatchWindow = 0.f;
		pNewDeltaItem->m_bLargeFont = false;
		pNewDeltaItem->m_eDataType = type;
		pNewDeltaItem->m_wzText[0] = NULL;
		pNewDeltaItem->m_color = GetColor( type, iDelta );
		pNewDeltaItem->m_bShadows = false;
		return &m_AccountDeltaItems[index];
	}

	return NULL;
}

Color CAccountPanel::GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue )
{
	if ( type == account_delta_t::ACCOUNT_DELTA_BONUS_POINTS )
	{
		return m_DeltaEventColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_HEALING )
	{
		return ( iDeltaValue < 0 ) ? m_DeltaNegativeColor : m_DeltaPositiveColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_DAMAGE )
	{
		return Color( hud_combattext_red.GetInt(), hud_combattext_green.GetInt(), hud_combattext_blue.GetInt() );
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_BLUE )
	{
		return m_DeltaBlueRobotScoreColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_RED )
	{
		return m_DeltaRedRobotScoreColor;
	}
	
	return Color( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CAccountPanel::Paint( void )
{
	BaseClass::Paint();

	FOR_EACH_VEC_BACK( m_AccountDeltaItems, i )
	{
		// Reduce lifetime when count grows too high
		float flTimeMod = m_AccountDeltaItems.Count() > NUM_ACCOUNT_DELTA_ITEMS ? RemapValClamped( m_AccountDeltaItems.Count(), 10.f, 15.f, 0.5f, 1.5f ) : 0.f;

		// update all the valid delta items
		if ( ( m_AccountDeltaItems[i].m_flDieTime - flTimeMod ) > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			Color c = m_AccountDeltaItems[i].m_color;

			float flLifetimePercent = ( m_flDeltaLifetime - ( m_AccountDeltaItems[i].m_flDieTime - gpGlobals->curtime ) ) / m_flDeltaLifetime;
			// fade out after half our lifetime
			int nAlpha = flLifetimePercent > 0.5 ? (int)( 255.0f * ( ( 0.5f - flLifetimePercent ) / 0.5f ) ) : 255;
			c[3] = nAlpha;
			


			// Some items want to be batched together as they're super frequent (i.e. damage events from a flamethrower, or minigun)
			if ( m_AccountDeltaItems[i].m_flBatchWindow > 0.f && m_AccountDeltaItems[i].m_nSourceID != -1 && m_AccountDeltaItems.IsValidIndex( i - 1 ) )
			{
				// If next item is from the same source and too close, merge
				float flDelay = m_AccountDeltaItems[i].m_flBatchWindow;
				if ( m_AccountDeltaItems[i].m_flDieTime - m_AccountDeltaItems[i-1].m_flDieTime <= flDelay &&
					 m_AccountDeltaItems[i-1].m_nSourceID == m_AccountDeltaItems[i].m_nSourceID )
				{
					m_AccountDeltaItems[i].m_iAmount += m_AccountDeltaItems[i-1].m_iAmount;
					m_AccountDeltaItems.Remove( i - 1 );
					continue;
				}
			}

			float flHeight = m_AccountDeltaItems[i].m_nHEnd - m_AccountDeltaItems[i].m_nHStart;
			float flWidth = m_AccountDeltaItems[i].m_nXEnd - m_AccountDeltaItems[i].m_nX;

			// We can be told to go the opposite direction if we're negative
			if ( m_bNegativeFlipDir && m_AccountDeltaItems[i].m_iAmount < 0 )
			{
				flHeight = -flHeight;
				flWidth = -flWidth;
			}

			float flYPos = m_AccountDeltaItems[i].m_nHStart + ( flLifetimePercent * flHeight );
			float flXPos = m_AccountDeltaItems[i].m_nX + ( flLifetimePercent * flWidth );
			if ( m_AccountDeltaItems[i].m_bWorldSpace )
			{
				Vector vecWorld( m_AccountDeltaItems[i].m_nX, m_AccountDeltaItems[i].m_nY, flYPos );
				int iX,iY;
				if ( !GetVectorInHudSpace( vecWorld, iX, iY ) )				// Tested - NOT GetVectorInScreenSpace
					continue;

				flXPos = iX;
				flYPos = iY;
			}

			// If we have a background texture, then draw it!
			if ( m_nBGTexture != -1 )
			{
				vgui::surface()->DrawSetColor(255,255,255,nAlpha);
				vgui::surface()->DrawSetTexture(m_nBGTexture);
				vgui::surface()->DrawTexturedRect( flXPos + m_flBGImageX, flYPos + m_flBGImageY, flXPos + m_flBGImageX + m_flBGImageWide, flYPos + m_flBGImageY + m_flBGImageTall );
			}

			wchar_t wBuf[20];

			if ( m_AccountDeltaItems[i].m_iAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d", m_AccountDeltaItems[i].m_iAmount );
			}
			else
			{
				V_swprintf_safe( wBuf, L"%d", m_AccountDeltaItems[i].m_iAmount );
			}

			// Append?
			if ( m_AccountDeltaItems[i].m_wzText[0] )
			{
				wchar_t wAppend[8] = { 0 };
				V_swprintf_safe( wAppend, L"%ls", m_AccountDeltaItems[i].m_wzText );
				V_wcscat_safe( wBuf, wAppend );
			}


			if ( m_AccountDeltaItems[i].m_bLargeFont )
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFontBig );
			}
			else
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			}

			// If we're supposed to have shadows, then draw the text as black and offset a bit first.
			// Things get ugly as we approach 0 alpha, so stop drawing the shadow a bit early.
			if ( m_AccountDeltaItems[i].m_bShadows && c[3] > 10 )
			{
				vgui::surface()->DrawSetTextPos( (int)flXPos + XRES(1), (int)flYPos + YRES(1) );
				vgui::surface()->DrawSetTextColor( COLOR_BLACK );
				vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
			}

			vgui::surface()->DrawSetTextPos( (int)flXPos, (int)flYPos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
		}
		else
		{
			m_AccountDeltaItems.Remove( i );
		}
	}
}

