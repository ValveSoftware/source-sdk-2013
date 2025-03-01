//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_spellbook.h"
#include "decals.h"
#include "tf_gamerules.h"
#include "tf_pumpkin_bomb.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_basedoor.h"
	#include "c_tf_player.h"
	#include "IEffects.h"
	#include "bone_setup.h"
	#include "c_tf_gamestats.h"
	#include "iclientmode.h"
	#include <vgui_controls/AnimationController.h>
	#include "econ_notifications.h"
	#include "gc_clientsystem.h"
	#include "tf_logic_halloween_2014.h"
	#include "tf_hud_itemeffectmeter.h"
	#include "dlight.h"
	#include "iefx.h"
	extern void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
// Server specific.
#else
	#include "doors.h"
	#include "tf_player.h"
	#include "tf_ammo_pack.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "particle_parse.h"
	#include "tf_projectile_base.h"
	#include "tf_gamerules.h"
	#include "tf_fx.h"
	#include "takedamageinfo.h"
	#include "halloween/zombie/zombie.h"
	#include "halloween/eyeball_boss/eyeball_boss.h"
	#include "halloween/halloween_base_boss.h"
	#include "entity_healthkit.h"
	#include "eyeball_boss/teleport_vortex.h"
	#include "in_buttons.h"
	#include "halloween/merasmus/merasmus.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "tf_obj_dispenser.h"
	#include "tf_weapon_flamethrower.h"
#endif

ConVar tf_test_spellindex( "tf_test_spellindex", "-1", FCVAR_CHEAT | FCVAR_REPLICATED, "Set to index to always get a specific spell" );
#ifdef GAME_DLL
ConVar tf_halloween_kart_rocketspell_speed( "tf_halloween_kart_rocketspell_speed", "1500", FCVAR_CHEAT );
ConVar tf_halloween_kart_rocketspell_lifetime( "tf_halloween_kart_rocketspell_lifetime", "0.5f", FCVAR_CHEAT );
ConVar tf_halloween_kart_rocketspell_force( "tf_halloween_kart_rocketspell_force", "900.0f", FCVAR_CHEAT );
#endif

extern ConVar tf_eyeball_boss_hover_height;
extern ConVar tf_halloween_kart_normal_speed;
extern ConVar tf_halloween_kart_dash_speed;
//=============================================================================
//
// Weapon Tables
//

// SpellBook --
IMPLEMENT_NETWORKCLASS_ALIASED( TFSpellBook, DT_TFWeaponSpellBook )

	BEGIN_NETWORK_TABLE( CTFSpellBook, DT_TFWeaponSpellBook )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iSelectedSpellIndex ) ),
	RecvPropInt( RECVINFO( m_iSpellCharges ) ),
	RecvPropFloat( RECVINFO( m_flTimeNextSpell ) ),
	RecvPropBool( RECVINFO( m_bFiredAttack ) ),
#else
	SendPropInt( SENDINFO( m_iSelectedSpellIndex ) ),
	SendPropInt( SENDINFO( m_iSpellCharges ) ),
	SendPropFloat( SENDINFO( m_flTimeNextSpell ) ),
	SendPropBool( SENDINFO( m_bFiredAttack ) ),
#endif

	END_NETWORK_TABLE()

	BEGIN_PREDICTION_DATA( CTFSpellBook )
	END_PREDICTION_DATA()

	LINK_ENTITY_TO_CLASS( tf_weapon_spellbook, CTFSpellBook );
PRECACHE_WEAPON_REGISTER( tf_weapon_spellbook );
// -- SpellBook

#define SPELL_EMPTY		-1
#define SPELL_UNKNOWN	-2
#define SPELL_BOXING_GLOVE "models/props_halloween/hwn_spell_boxing_glove.mdl"

//=============================================================================
// Spell Data Structures
//=============================================================================
enum SpellType_t
{
	SPELL_ROCKET,
	SPELL_JAR,		// Explodes on Contact
	SPELL_SELF,
};

struct spell_data_t
{
	spell_data_t( 
		const char *pSpellUiName, 
		int iSpellCharges, 
		SpellType_t eSpelltype, 
		const char *pSpellEntityName,  
		bool (*pCastSpell)(CTFPlayer*), 
		const char *pszCastSound, 
		float flSpeedScale, 
		int iCastContext, 
		int iSpellContext,
		const char *pIconName,
		bool bAutoCast = false
	) {
		m_pSpellUiName = pSpellUiName;
		m_eSpellType = eSpelltype;
		m_pSpellEntityName = pSpellEntityName;
		m_iSpellCharges = iSpellCharges;
		m_pCastSpell = pCastSpell;
		m_pszCastSound = pszCastSound;
		m_flSpeedScale = flSpeedScale;
		m_iCastContext = iCastContext;
		m_iSpellContext = iSpellContext;
		m_pIconName = pIconName;
		m_bAutoCast = bAutoCast;
	}

	const char * m_pSpellUiName;
	SpellType_t m_eSpellType;
	const char *m_pSpellEntityName;
	int m_iSpellCharges;
	const char *m_pszCastSound;
	float m_flSpeedScale;
	int m_iCastContext;	// context for the spell caster
	int m_iSpellContext; // context for enemies who witness the spell

	bool (*m_pCastSpell)(CTFPlayer*);
	const char *m_pIconName;
	bool m_bAutoCast;
};

static const spell_data_t g_NormalSpellList[] =
{
	spell_data_t( "#TF_Spell_Fireball",			2,		SPELL_ROCKET,	"tf_projectile_spellfireball",			NULL,	"Halloween.spell_fireball_cast", 1.f,MP_CONCEPT_PLAYER_CAST_BOMB_HEAD_CURSE,	MP_CONCEPT_PLAYER_SPELL_BOMB_HEAD_CURSE,		"spellbook_fireball" ),
	spell_data_t( "#TF_Spell_Bats",				2,		SPELL_JAR,		"tf_projectile_spellbats",				NULL,	"Halloween.spell_bat_cast", 1.f,	MP_CONCEPT_PLAYER_CAST_MERASMUS_ZAP,		MP_CONCEPT_PLAYER_SPELL_MERASMUS_ZAP,	"spellbook_bats" ),
	spell_data_t( "#TF_Spell_OverHeal",			1,		SPELL_SELF,		NULL,	CTFSpellBook::CastSelfHeal,				"Halloween.spell_overheal", 1.f,	MP_CONCEPT_PLAYER_CAST_SELF_HEAL,			MP_CONCEPT_PLAYER_SPELL_SELF_HEAL,		"spellbook_overheal" ),
	spell_data_t( "#TF_Spell_MIRV",				1,		SPELL_JAR,		"tf_projectile_spellmirv",				NULL,	"Halloween.spell_mirv_cast", 1.f,	MP_CONCEPT_PLAYER_CAST_MIRV,				MP_CONCEPT_PLAYER_SPELL_MIRV,			"spellbook_mirv" ),																													
	spell_data_t( "#TF_Spell_BlastJump",		2,		SPELL_SELF,		NULL,	CTFSpellBook::CastRocketJump,			"Halloween.spell_blastjump", 1.f,	MP_CONCEPT_PLAYER_CAST_BLAST_JUMP,			MP_CONCEPT_PLAYER_SPELL_BLAST_JUMP,		"spellbook_blastjump"),
	spell_data_t( "#TF_Spell_Stealth",			1,		SPELL_SELF,		NULL,	CTFSpellBook::CastSelfStealth,			"Halloween.spell_stealth", 1.f,		MP_CONCEPT_PLAYER_CAST_STEALTH,				MP_CONCEPT_PLAYER_SPELL_STEALTH,		"spellbook_stealth"),
	spell_data_t( "#TF_Spell_Teleport",			2,		SPELL_JAR,		"tf_projectile_spelltransposeteleport",	NULL,	"Halloween.spell_teleport", 1.f,	MP_CONCEPT_PLAYER_CAST_TELEPORT,			MP_CONCEPT_PLAYER_SPELL_TELEPORT,		"spellbook_teleport"),
};

static const int g_NavMeshSpells = 2; // Number of spells in this list that require a navmesh, they must be at the end of this array
static const spell_data_t g_RareSpellList[] =
{
	spell_data_t( "#TF_Spell_LightningBall",	1,		SPELL_ROCKET,	"tf_projectile_lightningorb",			NULL,	"Halloween.spell_lightning_cast", 0.4f,	MP_CONCEPT_PLAYER_CAST_LIGHTNING_BALL,		MP_CONCEPT_PLAYER_SPELL_LIGHTNING_BALL,		"spellbook_lightning"),
	spell_data_t( "#TF_Spell_Athletic",			1,		SPELL_SELF,		NULL,	CTFSpellBook::CastSelfSpeedBoost,		"Halloween.spell_athletic", 1.f,	MP_CONCEPT_PLAYER_CAST_MOVEMENT_BUFF,			MP_CONCEPT_PLAYER_SPELL_MOVEMENT_BUFF,		"spellbook_athletic"),
	spell_data_t( "#TF_Spell_Meteor",			1,		SPELL_JAR,		"tf_projectile_spellmeteorshower",		NULL,	"Halloween.spell_meteor_cast", 1.f,	MP_CONCEPT_PLAYER_CAST_METEOR_SWARM,			MP_CONCEPT_PLAYER_SPELL_METEOR_SWARM,		"spellbook_meteor"),
	spell_data_t( "#TF_Spell_SpawnBoss",		1,		SPELL_JAR,		"tf_projectile_spellspawnboss",			NULL,	"Halloween.Merasmus_Spell", 1.f,	MP_CONCEPT_PLAYER_CAST_MONOCULOUS,				MP_CONCEPT_PLAYER_SPELL_MONOCULOUS,			"spellbook_boss"),
	spell_data_t( "#TF_Spell_SkeletonHorde",	1,		SPELL_JAR,		"tf_projectile_spellspawnhorde",		NULL,	"Halloween.spell_skeleton_horde_cast", 1.f,	MP_CONCEPT_PLAYER_CAST_SKELETON_HORDE,	MP_CONCEPT_PLAYER_SPELL_SKELETON_HORDE,		"spellbook_skeleton"),
};

static const spell_data_t g_KartSpellList[] =
{
	// Kart Spells
	spell_data_t( "#TF_Spell_Fireball",			1,		SPELL_ROCKET,	"tf_projectile_spellkartorb",			NULL, "Halloween.spell_fireball_cast", 1.f,		MP_CONCEPT_PLAYER_CAST_MERASMUS_ZAP,		MP_CONCEPT_PLAYER_SPELL_MERASMUS_ZAP, "../hud/Punchglove_icon" ),
	spell_data_t( "#TF_Spell_BlastJump",		1,		SPELL_SELF,		NULL,	CTFSpellBook::CastKartRocketJump,			"Halloween.spell_blastjump", 1.f,	MP_CONCEPT_PLAYER_CAST_BLAST_JUMP,			MP_CONCEPT_PLAYER_SPELL_BLAST_JUMP,		"../hud/Parachute_icon"),
	spell_data_t( "#TF_Spell_OverHeal",			1,		SPELL_SELF,		NULL,	CTFSpellBook::CastKartUber,				"Halloween.spell_overheal", 1.f,		MP_CONCEPT_PLAYER_CAST_SELF_HEAL,			MP_CONCEPT_PLAYER_SPELL_SELF_HEAL,		"spellbook_overheal" ),
	spell_data_t( "#TF_Spell_BombHead",			1,		SPELL_SELF,		NULL,	CTFSpellBook::CastKartBombHead,				"Halloween.spell_overheal", 1.f,	MP_CONCEPT_PLAYER_CAST_FIREBALL,		MP_CONCEPT_PLAYER_SPELL_FIREBALL,		"../hud/bombhead_icon" ),
};

// Do not allow all spells in doomsday
static const int g_doomsdayNormalSpellIndexList[] =
{
	0,	//Fireball
	0,	//Fireball x2
	2,	//overheal
	4,	//Jump
	5,	//Stealth
};

static const int g_doomsdayRareSpellIndexList[] =
{
	ARRAYSIZE( g_NormalSpellList ) + 0,		// Lightning
	ARRAYSIZE( g_NormalSpellList ) + 1,		// Mini
	ARRAYSIZE( g_NormalSpellList ) + 2,		// Meteor
	ARRAYSIZE( g_NormalSpellList ) + 0,		// Lightning
	ARRAYSIZE( g_NormalSpellList ) + 1,		// Mini
	ARRAYSIZE( g_NormalSpellList ) + 2,		// Meteor
	ARRAYSIZE( g_NormalSpellList ) + 3		// Boss / Monoculus.  Smaller chance
};

// Regular SpellList
// teleport and summons removed
static const int g_generalSpellIndexList[] =
{
	0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 
	ARRAYSIZE ( g_NormalSpellList ) + 0, 
	ARRAYSIZE ( g_NormalSpellList ) + 1, 
	ARRAYSIZE ( g_NormalSpellList ) + 2
};

int GetTotalSpellCount( CTFPlayer *pPlayer )
{
	int iSpellCount = ARRAYSIZE( g_NormalSpellList ) + ARRAYSIZE( g_RareSpellList );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		iSpellCount += ARRAYSIZE( g_KartSpellList );
	}
	return iSpellCount;
}

bool IsRareSpell( int iSpellIndex )
{
	if ( tf_test_spellindex.GetInt() > 0 )
	{
		iSpellIndex = tf_test_spellindex.GetInt();
	}

	return ( ( iSpellIndex >= ARRAYSIZE( g_NormalSpellList ) ) && ( iSpellIndex < ARRAYSIZE( g_NormalSpellList ) + ARRAYSIZE( g_RareSpellList ) ) );
}

const spell_data_t* GetSpellData( int iSpellIndex )
{
	if ( tf_test_spellindex.GetInt() > -1 )
	{
		iSpellIndex = tf_test_spellindex.GetInt();
	}

	if ( iSpellIndex < 0 )
		return NULL;

	const int nNormalSpellCount = ARRAYSIZE( g_NormalSpellList );
	if ( iSpellIndex < nNormalSpellCount )
		return &g_NormalSpellList[ iSpellIndex ];

	const int nRareSpellRange = nNormalSpellCount + ARRAYSIZE( g_RareSpellList );
	if ( iSpellIndex < nRareSpellRange )
		return &g_RareSpellList[ iSpellIndex - nNormalSpellCount ];

	const int nKartSpellRange = nRareSpellRange + ARRAYSIZE( g_KartSpellList );
	if ( iSpellIndex < nKartSpellRange )
		return &g_KartSpellList[ iSpellIndex - nRareSpellRange];

	return NULL;
}

int GetSpellIndexFromContext( int iContext )
{
	const int nNormalSpellCount = ARRAYSIZE( g_NormalSpellList );
	for ( int i=0; i<nNormalSpellCount; ++i )
	{
		if ( g_NormalSpellList[i].m_iSpellContext == iContext )
		{
			return i;
		}
	}

	const int nRareSpellCount = ARRAYSIZE( g_RareSpellList );
	for ( int i=0; i<nRareSpellCount; ++i )
	{
		if ( g_RareSpellList[i].m_iSpellContext == iContext )
		{
			return i + nNormalSpellCount;
		}
	}

	return -1;
}

//=============================================================================
#ifdef CLIENT_DLL
//=============================================================================
// Ui Hud
//=============================================================================
extern ConVar cl_hud_minmode;

DECLARE_HUDELEMENT_DEPTH( CHudSpellMenu, 2 );
CHudSpellMenu::CHudSpellMenu( const char *pElementName ) : CHudElement( pElementName ), BaseClass ( NULL, "HudSpellMenu" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	m_iNextRollTime = 0;
	m_flRollTickGap = 0.05f;
	m_bTickSoundA = false;

	m_bKillstreakMeterDrawing = false;

	m_pSpellIcon = new vgui::ImagePanel( this, "SpellIcon" );
	m_pKeyBinding = new CExLabel( this, "ActionText", "" );

	ListenForGameEvent( "inventory_updated" );
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "post_inventory_application" );
}

//-----------------------------------------------------------------------------
void CHudSpellMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;
	if ( m_bKillstreakMeterDrawing )
	{
		pConditions = new KeyValues( "conditions" );
		if ( pConditions )
		{
			AddSubKeyNamed( pConditions, "if_killstreak_visible" );
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudSpellSelection.res", NULL, NULL, pConditions );
	SetVisible( false );
	UpdateSpellText( -1, -1 );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}

//=============================================================================
void CHudSpellMenu::OnTick( void )
{
	bool bKillstreakMeterDrawing = false;
	CHudItemEffectMeter *pMeter = NULL;
	for ( int i = 0; i < IHudItemEffectMeterAutoList::AutoList().Count(); ++i )
	{
		pMeter = static_cast<CHudItemEffectMeter*>( IHudItemEffectMeterAutoList::AutoList()[i] );
		if ( pMeter->IsKillstreakMeter() ) // we found the killstreak meter
		{
			if ( pMeter->IsEnabled() )
			{
				bKillstreakMeterDrawing = true;
			}
			break;
		}
	}

	if ( m_bKillstreakMeterDrawing != bKillstreakMeterDrawing )
	{
		m_bKillstreakMeterDrawing = bKillstreakMeterDrawing;
		InvalidateLayout( false, true );
	}

	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//=============================================================================
void CHudSpellMenu::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( event->GetName(), "post_inventory_application" ) ||
		 FStrEq( event->GetName(), "localplayer_respawn" ) ||
		 FStrEq( event->GetName(), "localplayer_changeclass" ) ||
		 FStrEq( event->GetName(), "inventory_updated" ) )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 10 );
	}
}

//=============================================================================
bool CHudSpellMenu::ShouldDraw( void )
{
	if ( TFGameRules() && TFGameRules()->IsUsingSpells() )
	{
		if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() && ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) )
			return false;

		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->IsAlive() && !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			CTFSpellBook *pSpellBook = dynamic_cast<CTFSpellBook*>( pPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
			if ( pSpellBook )
			{
				UpdateSpellText( pSpellBook->m_iSelectedSpellIndex, pSpellBook->m_iSpellCharges );
				return CHudElement::ShouldDraw();
			}
		}
	}
	return false;
}
//=============================================================================
void CHudSpellMenu::UpdateSpellText( int iSpellIndex, int iChargeCount )
{
	if ( iSpellIndex == SPELL_EMPTY || ( iChargeCount <= 0 && iSpellIndex != SPELL_UNKNOWN ) )
	{
		SetDialogVariable( "counttext", "..." );
		//SetDialogVariable( "selectedspell", g_pVGuiLocalize->Find( pSpellData->m_pSpellUiName ) );
		m_pSpellIcon->SetImage( "spellbook_nospell" );
		m_flRollTickGap = 0.01f;
		m_iNextRollTime = 0;
		m_pKeyBinding->SetVisible( false );
		return;
	}

	m_pSpellIcon->SetVisible( true );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	static wchar_t wLabel[256];

	if ( iSpellIndex == SPELL_UNKNOWN )
	{
		if ( m_iNextRollTime > gpGlobals->curtime )
			return;
		m_iNextRollTime = gpGlobals->curtime + m_flRollTickGap;
		m_flRollTickGap += 0.015f;
		static int s_iRandSpell = 0;
		s_iRandSpell = ( s_iRandSpell + 1 ) % GetTotalSpellCount( pLocalPlayer );
		const spell_data_t *pSpellData = GetSpellData( s_iRandSpell );
		SetDialogVariable( "counttext", "?" );
		m_pSpellIcon->SetImage( pSpellData->m_pIconName );

		pLocalPlayer->EmitSound( m_bTickSoundA ? "Halloween.spelltick_a" : "Halloween.spelltick_b" );
		m_bTickSoundA = !m_bTickSoundA;
				
		m_iPrevSelectedSpell = SPELL_UNKNOWN;
		m_pKeyBinding->SetVisible( false );
	}
	else
	{
		m_flRollTickGap = 0.01f;
		m_iNextRollTime = 0;
		const spell_data_t *pSpellData = GetSpellData( iSpellIndex );
		if ( pSpellData )
		{
			SetDialogVariable( "counttext", iChargeCount );
			m_pSpellIcon->SetImage( pSpellData->m_pIconName );
			if ( m_iPrevSelectedSpell != iSpellIndex && iSpellIndex != SPELL_EMPTY )
			{
				pLocalPlayer->EmitSound( "Halloween.spelltick_set" );
			}
			m_iPrevSelectedSpell = iSpellIndex;
			m_pKeyBinding->SetVisible( !cl_hud_minmode.GetBool() );

			// Action Key Text
			wchar_t wKeyReplaced[256];
			UTIL_ReplaceKeyBindings( g_pVGuiLocalize->Find( "#TF_Spell_Action" ), 0, wKeyReplaced, sizeof( wKeyReplaced ) );
			SetDialogVariable( "actiontext", wKeyReplaced );
		}
	}
}

//-----------------------------------------------------------------------------
// CEquipSpellbookNotification
//-----------------------------------------------------------------------------
void CEquipSpellbookNotification::Accept()
{
	m_bHasTriggered = true;
		
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
	{
		MarkForDeletion();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		MarkForDeletion();
		return;
	}

	// try to equip non-stock-spellbook first
	static CSchemaItemDefHandle pItemDef_Spellbook( "Basic Spellbook" );
	static CSchemaItemDefHandle pItemDef_Diary( "Secret Diary" );
	static CSchemaItemDefHandle pItemDef_FancySpellbook( "Halloween Spellbook" );

	Assert( pItemDef_Spellbook );
	Assert( pItemDef_Diary );
	Assert( pItemDef_FancySpellbook );

	CEconItemView *pSpellBook = NULL;

	if ( pItemDef_Spellbook && pItemDef_Diary && pItemDef_FancySpellbook )
	{
		for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
		{
			CEconItemView *pItem = pLocalInv->GetItem( i );
			Assert( pItem );
			if ( pItem->GetItemDefinition() == pItemDef_Spellbook 
				|| pItem->GetItemDefinition() == pItemDef_Diary 
				|| pItem->GetItemDefinition() == pItemDef_FancySpellbook 
			) {
				pSpellBook = pItem;
				break;
			}
		}
	}

	// Default item becomes a spellbook in this mode
	itemid_t iItemId = INVALID_ITEM_ID;
	if ( pSpellBook )
	{
		iItemId = pSpellBook->GetItemID();
	}

	TFInventoryManager()->EquipItemInLoadout( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION, iItemId );
	
	// Tell the GC to tell server that we should respawn if we're in a respawn room

	MarkForDeletion();
}

//===========================================================================================
void CEquipSpellbookNotification::UpdateTick()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		CTFSpellBook *pSpellBook = dynamic_cast<CTFSpellBook*>( pLocalPlayer->Weapon_OwnsThisID( TF_WEAPON_SPELLBOOK ) );
		if ( pSpellBook )
		{
			MarkForDeletion();
		}
	}
}
#endif // CLIENT_DLL

//===========================================================================================
//
// CTFSpellBook
//
//===========================================================================================
CTFSpellBook::CTFSpellBook()
{
	m_iSelectedSpellIndex = -1;
	m_iSpellCharges = 0;
	m_flTimeNextSpell = 0;
	m_bFiredAttack = false;
#ifdef CLIENT_DLL
	m_flTimeNextErrorSound = 0;
	m_hHandEffect = NULL;
	m_hHandEffectWeapon = NULL;
#endif // CLIENT_DLL

#ifdef GAME_DLL
	m_pStoredLastWpn = NULL;
	m_iPreviouslyCastSpell = -1;
#endif // GAME_DLL
}

void CTFSpellBook::Precache()
{
	PrecacheScriptSound( "Halloween.Merasmus_Spell" );
	PrecacheScriptSound( "Weapon_SniperRailgun_Large.SingleCrit" );
	PrecacheScriptSound( "Halloween.spelltick_a" );
	PrecacheScriptSound( "Halloween.spelltick_b" );
	PrecacheScriptSound( "Halloween.spelltick_set" );

	PrecacheScriptSound( "Halloween.spell_athletic" );
	PrecacheScriptSound( "Halloween.spell_bat_cast" );
	PrecacheScriptSound( "Halloween.spell_bat_impact" );
	PrecacheScriptSound( "Halloween.spell_blastjump" );
	PrecacheScriptSound( "Halloween.spell_fireball_cast" );
	PrecacheScriptSound( "Halloween.spell_fireball_impact" );
	PrecacheScriptSound( "Halloween.spell_lightning_cast" );
	PrecacheScriptSound( "Halloween.spell_lightning_impact" );
	PrecacheScriptSound( "Halloween.spell_meteor_cast" );
	PrecacheScriptSound( "Halloween.spell_meteor_impact" );
	PrecacheScriptSound( "Halloween.spell_mirv_cast" );
	PrecacheScriptSound( "Halloween.spell_mirv_explode_primary" );
	PrecacheScriptSound( "Halloween.spell_mirv_explode_secondary" );
	PrecacheScriptSound( "Halloween.spell_skeleton_horde_cast" );
	PrecacheScriptSound( "Halloween.spell_skeleton_horde_rise" );
	PrecacheScriptSound( "Halloween.spell_spawn_boss" );
	PrecacheScriptSound( "Halloween.spell_stealth" );
	PrecacheScriptSound( "Halloween.spell_teleport" );
	PrecacheScriptSound( "Halloween.spell_overheal" );

	PrecacheParticleSystem( "merasmus_zap" );
	PrecacheParticleSystem( "spell_cast_wheel_red" );
	PrecacheParticleSystem( "spell_cast_wheel_blue" );
	PrecacheParticleSystem( "Explosion_bubbles" );
	PrecacheParticleSystem( "ExplosionCore_buildings" );
	PrecacheParticleSystem( "water_splash01" );
	PrecacheParticleSystem( "healshot_trail_blue" );
	PrecacheParticleSystem( "healshot_trail_red" );
	PrecacheParticleSystem( "xms_snowburst" );
	PrecacheParticleSystem( "bomibomicon_ring" );
	PrecacheParticleSystem( "bombinomicon_burningdebris" );
	PrecacheParticleSystem( "merasmus_tp_bits" );
	PrecacheParticleSystem( "spell_fireball_tendril_parent_red" );
	PrecacheParticleSystem( "spell_fireball_tendril_parent_blue" );
	PrecacheParticleSystem( "spell_fireball_small_blue" );
	PrecacheParticleSystem( "spell_fireball_small_red" );
	PrecacheParticleSystem( "spell_lightningball_parent_blue" );
	PrecacheParticleSystem( "spell_lightningball_parent_red" );
	PrecacheParticleSystem( "spell_lightningball_hit_blue" );
	PrecacheParticleSystem( "spell_lightningball_hit_red" );
	
	PrecacheParticleSystem( "eyeboss_tp_vortex" );
	PrecacheParticleSystem( "spell_overheal_red" );
	PrecacheParticleSystem( "spell_overheal_blue" );
	PrecacheParticleSystem( "spell_teleport_red" );
	PrecacheParticleSystem( "spell_teleport_blue" );
	PrecacheParticleSystem( "spell_batball_red" );
	PrecacheParticleSystem( "spell_batball_blue" );
	PrecacheParticleSystem( "spell_batball_throw_red" );
	PrecacheParticleSystem( "spell_batball_throw_blue" );
	PrecacheParticleSystem( "spell_batball_impact_red" );
	PrecacheParticleSystem( "spell_batball_impact_blue" );
	
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_red" );
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_blue" );
	PrecacheParticleSystem( "spell_skeleton_goop_green" );

	PrecacheParticleSystem( "spellbook_rainbow" );
	PrecacheParticleSystem( "spellbook_major_burning" );
	PrecacheParticleSystem( "spellbook_minor_burning" );
	PrecacheModel( "models/props_mvm/mvm_human_skull_collide.mdl" );
	PrecacheModel( "models/props_lakeside_event/bomb_temp_hat.mdl" );
	PrecacheModel( SPELL_BOXING_GLOVE );
	PrecacheModel( "models/props_halloween/bombonomicon.mdl" ); // bomb head spell
	PrecacheParticleSystem( "halloween_rockettrail" );
	PrecacheParticleSystem( "ExplosionCore_MidAir" );

#ifdef GAME_DLL
	CEyeballBoss::PrecacheEyeballBoss();
	CZombie::PrecacheZombie();
#endif // GAME_DLL

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
void CTFSpellBook::PrimaryAttack()
{
	// cast spell
	if ( m_flTimeNextSpell > gpGlobals->curtime )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	bool bCastSuccessful = false;
	
	bCastSuccessful = CanCastSpell( pPlayer );

	if ( bCastSuccessful ) 
	{
#ifdef GAME_DLL
		SpeakSpellConceptIfAllowed();
		
		// We need to do this before PrimaryAttack so we use the right spell index
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			CastKartSpell();
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_ACTION_SHOOT );
		}
		else
		{
			CastSpell( pPlayer, m_iSelectedSpellIndex );
			BaseClass::PrimaryAttack();
		}
#endif
		
#ifdef GAME_DLL
		// set a default time cast time if none added
		if ( m_flTimeNextSpell < gpGlobals->curtime )
		{
			m_flTimeNextSpell = gpGlobals->curtime + 0.5f;
		}
#endif // GAME_DLL
	}
#ifdef CLIENT_DLL
	else
	{
		if ( m_flTimeNextErrorSound < gpGlobals->curtime )
		{
			m_flTimeNextErrorSound = gpGlobals->curtime + 0.5f;
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
void CTFSpellBook::ItemBusyFrame( void )
{
#ifdef CLIENT_DLL
	if ( m_hHandEffectWeapon && m_hHandEffect )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( IsFirstPersonView() )
	{
		m_hHandEffectWeapon = pPlayer->GetViewModel();
	}
	else
	{
		m_hHandEffectWeapon = pPlayer;
	}

	if ( !m_hHandEffectWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	C_BaseAnimating* pBase = (C_BaseAnimating*)m_hHandEffectWeapon.Get();
	int iAttachment = pBase->C_BaseAnimating::LookupAttachment( "effect_hand_R" );

	// Start the muzzle flash, if a system hasn't already been started.
	if ( iAttachment > 0 )
	{
		const char *pszEffectName = GetHandEffect( GetAttributeContainer()->GetItem(), m_iSelectedSpellIndex >= ARRAYSIZE( g_NormalSpellList ) );
		if ( pszEffectName )
		{
			m_hHandEffect = pBase->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, iAttachment );
		}
	}
	else
	{
		if ( m_hHandEffect )
		{
			m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
			m_hHandEffectWeapon = NULL;
			m_hHandEffect		= NULL;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
void CTFSpellBook::ItemHolsterFrame( void )
{
#ifdef CLIENT_DLL
	if ( !m_hHandEffectWeapon )
		return;

	// Stop the muzzle flash.
	if ( m_hHandEffect )
	{
		m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
		m_hHandEffectWeapon = NULL;
		m_hHandEffect		= NULL;
	}
#endif

#ifdef GAME_DLL
	m_bFiredAttack = false;
#endif
}

//-----------------------------------------------------------------------------
void CTFSpellBook::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL
	// attempt to attack then switch back
	if ( !m_bFiredAttack && m_iSpellCharges > 0 )
	{
		PrimaryAttack();

		if ( m_hHandEffect )
		{
			m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
			m_hHandEffectWeapon = NULL;
			m_hHandEffect		= NULL;
		}
	}
#endif

#ifdef GAME_DLL
	if ( tf_test_spellindex.GetInt() > -1 )
	{
		SetSelectedSpell( tf_test_spellindex.GetInt() );
	}

	// attempt to attack then switch back
	if ( !m_bFiredAttack && m_iSpellCharges > 0 )
	{
		PrimaryAttack();
		m_bFiredAttack = true;
	}
	else
	{
		if ( m_flTimeNextSpell > gpGlobals->curtime )
			return;

		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( !pPlayer )
			return;

		if ( pPlayer->Weapon_Switch( pPlayer->GetLastWeapon() ) )
		{
			if ( m_pStoredLastWpn != NULL && pPlayer->Weapon_CanSwitchTo( m_pStoredLastWpn.Get() ) )
			{
				pPlayer->Weapon_SetLast( m_pStoredLastWpn.Get() );
				m_pStoredLastWpn = NULL;
			}
			else
			{
				pPlayer->Weapon_SetLast( NULL );
			}
			m_bFiredAttack = false;
		}
	}
#endif //GAME_DLL
}

//-----------------------------------------------------------------------------
/* static */ const char* CTFSpellBook::GetHandEffect( CEconItemView *pItem, int iTier )
{
	// if fancy spellbook //1069
	int defIndex = pItem->GetItemDefIndex();
	if ( defIndex == 1069 )
	{
		if ( iTier > 0 )
		{
			return "spellbook_major_burning";
		}
		else
		{
			return "spellbook_minor_burning";
		}
	}		
	else if ( defIndex == 5605 ) // secret diary
	{
		return "spellbook_rainbow";
	}
	else   // else Basic SpellBook
	{
		if ( iTier > 0 )
		{
			return "spellbook_major_fire";
		}
		else
		{
			return "spellbook_minor_fire";
		}
	}
}

//-----------------------------------------------------------------------------
bool CTFSpellBook::HasASpellWithCharges() 
{ 
	return tf_test_spellindex.GetInt() > -1 || m_iSpellCharges > 0 || m_iSelectedSpellIndex == SPELL_UNKNOWN;
}

//-----------------------------------------------------------------------------
bool CTFSpellBook::CanCastSpell( CTFPlayer *pPlayer )
{
	if ( !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART) && !pPlayer->CanAttack() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		return false;

	if ( tf_test_spellindex.GetInt() > -1 && tf_test_spellindex.GetInt() < GetTotalSpellCount( pPlayer ) )
		return true;

	return m_iSpellCharges > 0 && m_iSelectedSpellIndex >= 0 && m_iSelectedSpellIndex < GetTotalSpellCount( pPlayer );
}

//-----------------------------------------------------------------------------
void CTFSpellBook::PaySpellCost( CTFPlayer *pPlayer )
{
	m_iSpellCharges--;
}


//-----------------------------------------------------------------------------
void CTFSpellBook::ClearSpell()
{
	m_iSpellCharges = 0;
#ifdef GAME_DLL
	// If rolling for a spell, clear that too
	m_iNextSpell = SPELL_EMPTY;
#endif // GAME_DLL
}
//-----------------------------------------------------------------------------
CBaseEntity *CTFSpellBook::FireJar( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		TossJarThink();
	}
	else
	{
		SetContextThink( &CTFJar::TossJarThink, gpGlobals->curtime + 0.01f, "TOSS_JAR_THINK" );
	}
#endif
	return NULL;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFSpellBook::TossJarThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Self casts
	const spell_data_t *pSpellData = GetSpellData( m_iPreviouslyCastSpell );
	if ( !pSpellData )
		return;

	if ( pSpellData->m_eSpellType == SPELL_SELF )
	{
		// Self casts
		if ( TFGameRules() )
		{
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( pSpellData->m_iSpellContext, ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
		}
		// Play a sound immediately for self-cast spells
		EmitSound( pSpellData->m_pszCastSound );
		pSpellData->m_pCastSpell( pPlayer );
		return;
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	float fRight = 8.f;
	if ( IsViewModelFlipped() )
	{
		fRight *= -1;
	}
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	// Make spell toss position at the hand
	vecSrc = vecSrc + (vecUp * -9.0f) + (vecRight * 7.0f) + (vecForward * 3.0f);

	Vector vecVelocity = GetVelocityVector( vecForward, vecRight, vecUp ) * pSpellData->m_flSpeedScale;
	QAngle angForward = pPlayer->EyeAngles();

	// Halloween Hack
	// Eye Angles slighty higher
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// Add More up for Jar
		angForward = pPlayer->GetAbsAngles();
		if ( pSpellData->m_eSpellType == SPELL_JAR )
		{
			angForward.x -= 10.0f;
		}

		AngleVectors( angForward, &vecForward, &vecRight, &vecUp );
		vecVelocity = vecForward * tf_halloween_kart_rocketspell_speed.GetFloat();
	}

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	// If we started in solid, don't let them fire at all
	if ( trace.startsolid )
		return;

	// Play a sound when we actually cast the projectile
	EmitSound( pSpellData->m_pszCastSound );

	switch ( pSpellData->m_eSpellType )
	{
	case SPELL_ROCKET :
		{
			//QAngle angForward;
			//GetProjectileFireSetup( pPlayer, Vector(0,0,0), &vecSrc, &angForward, false );
			CreateSpellRocket( trace.endpos, angForward, vecVelocity, GetAngularImpulse(), pPlayer, GetTFWpnData() );
		}
		break;
	case SPELL_JAR :
		{
			CreateSpellJar( trace.endpos, angForward, vecVelocity, GetAngularImpulse(), pPlayer, GetTFWpnData() );
		}
		break;
	case SPELL_SELF :
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSpellBook::CreateSpellRocket( const Vector &position, const QAngle &angles, const Vector &velocity, 
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	const spell_data_t* pSpellData = GetSpellData( m_iPreviouslyCastSpell );
	if ( !pSpellData )
	{
		return;
	}

	ASSERT( pSpellData->m_eSpellType == SPELL_ROCKET );
	CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CBaseEntity::CreateNoSpawn( pSpellData->m_pSpellEntityName, position, angles, pOwner ) );
	if ( pRocket )
	{
		pRocket->SetOwnerEntity( pOwner );
		pRocket->SetLauncher( this ); 

		Vector vForward;
		AngleVectors( angles, &vForward, NULL, NULL );
		pRocket->SetAbsVelocity( vForward * velocity.Length() );

		pRocket->SetDamage( weaponInfo.GetWeaponData(TF_WEAPON_PRIMARY_MODE).m_nDamage );
		pRocket->ChangeTeam( pOwner ? pOwner->GetTeamNumber() : TEAM_UNASSIGNED );

		IPhysicsObject *pPhysicsObject = pRocket->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &velocity, &angVelocity );
		}

		DispatchSpawn( pRocket );
	}
}
//-----------------------------------------------------------------------------
void CTFSpellBook::CreateSpellJar( const Vector &position, const QAngle &angles, const Vector &velocity, 
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	const spell_data_t* pSpellData = GetSpellData( m_iPreviouslyCastSpell );
	if ( !pSpellData )
	{
		return;
	}

	ASSERT( pSpellData->m_eSpellType == SPELL_JAR );
	CTFProjectile_Jar *pGrenade = static_cast<CTFProjectile_Jar*>( CBaseEntity::CreateNoSpawn( pSpellData->m_pSpellEntityName, position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		IPhysicsObject *pPhys = pGrenade->VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->SetMass( 5.0f );
		}

		pGrenade->InitGrenade( velocity, vec3_origin, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->ApplyLocalAngularVelocityImpulse( vec3_origin );		
	}
}

//-----------------------------------------------------------------------------
void CTFSpellBook::RollNewSpell( int iTier, bool bForceReroll /*= false*/ )
{
	// do not do anything if we already have a spell for low tier, always roll for high tier
	if ( m_iSpellCharges > 0 && iTier == 0 && !bForceReroll )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	int iNextSpell = SPELL_EMPTY;
	// Halloween 2014
	// This is dumb, make spell lists better somehow
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		iNextSpell = RandomInt( ARRAYSIZE( g_NormalSpellList ) + ARRAYSIZE( g_RareSpellList ), GetTotalSpellCount( pPlayer ) - 1 );
	}
	else if ( iTier == 0 )
	{
		// Doomsday has special spell list
		if ( TFGameRules() && TFGameRules()->GetHalloweenScenario( ) == CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY )
		{
			iNextSpell = g_doomsdayNormalSpellIndexList[ RandomInt( 0, ARRAYSIZE( g_doomsdayNormalSpellIndexList ) - 1 ) ];
		}
		// Helltower has normal spell list
		else if ( TFGameRules() && TFGameRules()->GetHalloweenScenario( ) == CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER )
		{
			iNextSpell = RandomInt( 0, ARRAYSIZE( g_NormalSpellList ) - 1 );
		}
		// everyone else uses special list
		else
		{
			iNextSpell = g_generalSpellIndexList[ RandomInt( 0, ARRAYSIZE( g_generalSpellIndexList ) - 1 ) ];
		}
	}
	else // rare spell should not be the else
	{	
		// Doomsday has special spell list
		if ( TFGameRules() && TFGameRules()->GetHalloweenScenario( ) == CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY )
		{
			iNextSpell = g_doomsdayRareSpellIndexList[ RandomInt( 0, ARRAYSIZE( g_doomsdayRareSpellIndexList ) - 1 ) ];
		}
		else
		{
			// g_NavMeshSpells
			// If there's no Nav mesh do not allow the upper range of spells (summons)
			int iIndexReduction = 1;
			if ( ( TheNavMesh == NULL ) || ( TheNavMesh->GetNavAreaCount() <= 0 ) )
			{
				iIndexReduction += g_NavMeshSpells;
			}
			iNextSpell = RandomInt( ARRAYSIZE( g_NormalSpellList ), GetTotalSpellCount( pPlayer ) - 1 );
		}
	}
	
	const float flRollTime = 2.f;

	m_iNextSpell = iNextSpell;
	SetSelectedSpell( SPELL_UNKNOWN );
	SetContextThink( &CTFSpellBook::RollNewSpellFinish, gpGlobals->curtime + flRollTime, "SpellRollFinish" );
}

//-----------------------------------------------------------------------------
void CTFSpellBook::RollNewSpellFinish( void )
{
	SetSelectedSpell( m_iNextSpell );

	if ( m_iNextSpell < 0 )
		return;

	// response rules
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
	{
		return;
	}
	int iConcept = MP_CONCEPT_NONE;
	if ( m_iNextSpell < ARRAYSIZE( g_NormalSpellList ) )
	{
		iConcept = MP_CONCEPT_PLAYER_SPELL_PICKUP_COMMON;
	}
	else
	{
		iConcept = MP_CONCEPT_PLAYER_SPELL_PICKUP_RARE;
	}

	if ( iConcept != MP_CONCEPT_NONE )
	{
		pPlayer->SpeakConceptIfAllowed( iConcept );
	}
}

//-----------------------------------------------------------------------------
void CTFSpellBook::SetSelectedSpell( int index )
{
	m_iSelectedSpellIndex = index;

	const spell_data_t *pSpellData = GetSpellData( m_iSelectedSpellIndex );
	m_iSpellCharges = pSpellData ? pSpellData->m_iSpellCharges : 0;

	if ( pSpellData && pSpellData->m_bAutoCast )
	{
		PrimaryAttack();
	}
}
//-----------------------------------------------------------------------------
void CTFSpellBook::SpeakSpellConceptIfAllowed()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer || m_iSpellCharges <= 0 )
		return;

	const spell_data_t *pSpellData = GetSpellData( m_iSelectedSpellIndex );
	if ( pSpellData )
	{
		pPlayer->SpeakConceptIfAllowed( pSpellData->m_iCastContext );
	}
}

//------------------------------------------------------------------------------------------------------------------------------------
// KART FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------------------
void CTFSpellBook::CastKartSpell()
{
#ifdef GAME_DLL
	// cast spell time
	if ( m_flTimeNextSpell > gpGlobals->curtime )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( m_iSpellCharges <= 0 )
	{
		if ( tf_test_spellindex.GetInt() < 0 || tf_test_spellindex.GetInt() > GetTotalSpellCount( pPlayer ) )
			return;
	}
	
	// Save off what we cast for jar think
	PaySpellCost( pPlayer );

	m_iPreviouslyCastSpell = m_iSelectedSpellIndex;
	FireProjectile( pPlayer );

	m_flTimeNextSpell = gpGlobals->curtime + 0.5f;

	// Create one off spell effect in front of the player
	Vector origin = pPlayer->GetAbsOrigin();
	CPVSFilter filter( origin );

	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		TE_TFParticleEffect( filter, 0.0, "spell_cast_wheel_red", origin + Vector( 0, 0, 100 ), vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
	}
	else
	{
		TE_TFParticleEffect( filter, 0.0, "spell_cast_wheel_blue", origin + Vector( 0, 0, 100 ), vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
	}

#endif
}

//-----------------------------------------------------------------------------
// Individual spells
//-----------------------------------------------------------------------------
bool CTFSpellBook::CastSpell( CTFPlayer *pPlayer, int iSpellIndex )
{
	if ( CanCastSpell( pPlayer ) )	
	{
		PaySpellCost( pPlayer );
		const spell_data_t *pSpellData = GetSpellData( m_iSelectedSpellIndex );
		if ( !pSpellData)
			return false;

		if ( IsRareSpell( iSpellIndex ) )
		{
			if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
			{
				pPlayer->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_RARE_SPELL );
			}
		}

		// Save off what we cast for jar think
		m_iPreviouslyCastSpell = m_iSelectedSpellIndex;

		// Create one off spell effect in front of the player
		Vector origin = pPlayer->GetAbsOrigin();
		CPVSFilter filter( origin );

		//const spell_data_t *pSpellData = GetSpellData( m_iSelectedSpellIndex );
		if ( pSpellData && !FStrEq( pSpellData->m_pSpellUiName, "#TF_Spell_Stealth" ) )	// do NOT create for Stealth
		{
			if ( GetTeamNumber() == TF_TEAM_RED )
			{
				TE_TFParticleEffect( filter, 0.0, "spell_cast_wheel_red", origin + Vector( 0, 0, 100 ), vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				TE_TFParticleEffect( filter, 0.0, "spell_cast_wheel_blue", origin + Vector( 0, 0, 100 ), vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
			}
		}
		return true;
	}
	return false;
}

#endif

//-----------------------------------------------------------------------------
bool CTFSpellBook::CastSelfHeal( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	Vector origin = pPlayer->GetAbsOrigin();
	CPVSFilter filter( origin );
	const char* pszEffectName = pPlayer->GetTeamNumber() == TF_TEAM_RED ? "spell_overheal_red" : "spell_overheal_blue";
	TE_TFParticleEffect( filter, 0.0, pszEffectName, origin, vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
	
	//pPlayer->EmitSound( "BaseExplosionEffect.Sound" );

	// Collect players and cause knockback to enemies
	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( pPlayer, COLLISION_GROUP_PROJECTILE );

	// Splash pee on everyone nearby.
	CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
	int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), origin, 250.0f, FL_CLIENT | FL_FAKECLIENT | FL_NPC );
	for ( int i = 0; i < iEntities; ++i )
	{
		CBaseCombatCharacter *pBaseTarget = NULL;
		CTFPlayer *pTarget = ToTFPlayer( pListOfEntities[i] );
		if ( !pTarget )
		{
			pBaseTarget = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
		}
		else
		{
			pBaseTarget = pTarget;
		}

		if ( !pBaseTarget || !pTarget || !pTarget->IsAlive() )
			continue;

		// Do a quick trace to see if there's any geometry in the way.
		trace_t trace;
		UTIL_TraceLine( origin, pBaseTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
		if ( trace.DidHitWorld() )
			continue;

		Vector vecDir = pBaseTarget->WorldSpaceCenter() - origin;
		VectorNormalize( vecDir );

		// help allies
		if ( pBaseTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
		{
			pBaseTarget->TakeHealth( 50, DMG_GENERIC );
			
			if ( pTarget )
			{
				pTarget->m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, 1, pPlayer );
				pTarget->m_Shared.AddCond( TF_COND_HALLOWEEN_QUICK_HEAL, 3, pPlayer );
			}
		}
		else // knockback enemies
		{
			if ( pTarget )
			{
				pTarget->ApplyGenericPushbackImpulse( vecDir * 300.0f, pPlayer );
			}
			else
			{
				pBaseTarget->ApplyAbsVelocityImpulse( vecDir * 300.0f );
			}
		}
	}

#endif
	return true;
}
//-----------------------------------------------------------------------------
bool CTFSpellBook::CastRocketJump( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	const float flBlastRadius = 100.f;

	// Set z to zero then add impulse
	// make this proper jumping later
	Vector vel = pPlayer->GetAbsVelocity();
	if ( vel.z < 0 )
	{
		vel.z = 0;
	}
	pPlayer->SetAbsVelocity( vel );

	Vector vForward( 0, 0, 800 );
	pPlayer->ApplyAbsVelocityImpulse( vForward );
		
	const Vector& origin = pPlayer->GetAbsOrigin();
	CPVSFilter filter( origin );
	TE_TFParticleEffect( filter, 0.0, "bombinomicon_burningdebris", origin, vec3_angle );
	TE_TFParticleEffect( filter, 0.0, "heavy_ring_of_fire", origin, vec3_angle );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pPlayer, "foot_L" );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pPlayer, "foot_R" );

	// Give a little health to compensate for fall damage
	pPlayer->TakeHealth( 25, DMG_GENERIC );

	// Collect players and cause knockback to enemies
	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( pPlayer, COLLISION_GROUP_PROJECTILE );

	// Splash pee on everyone nearby.
	CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
	int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), origin, flBlastRadius, FL_CLIENT | FL_FAKECLIENT | FL_NPC );
	for ( int i = 0; i < iEntities; ++i )
	{
		CBaseCombatCharacter *pBaseTarget = NULL;
		CTFPlayer *pTarget = ToTFPlayer( pListOfEntities[i] );
		if ( !pTarget )
		{
			pBaseTarget = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
		}
		else
		{
			pBaseTarget = pTarget;
		}

		if ( !pBaseTarget || !pTarget || !pTarget->IsAlive() || pBaseTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
			continue;

		// Do a quick trace to see if there's any geometry in the way.
		trace_t trace;
		UTIL_TraceLine( origin, pBaseTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
		if ( trace.DidHitWorld() )
			continue;

		Vector vecDir = pBaseTarget->WorldSpaceCenter() - origin;
		VectorNormalize( vecDir );

		pBaseTarget->RemoveFlag( FL_ONGROUND );
		
		if ( pTarget )
		{
			pTarget->ApplyGenericPushbackImpulse( vecDir * 800.0f, pPlayer );
		}
		else
		{
			pBaseTarget->ApplyAbsVelocityImpulse( vecDir * 800.0f );
		}
	}

	CTakeDamageInfo info;
	info.SetAttacker( pPlayer );
	info.SetInflictor( pPlayer ); 
	info.SetDamage( 20.f );
	info.SetDamageCustom( TF_DMG_CUSTOM_SPELL_BLASTJUMP );
	info.SetDamagePosition( origin );
	info.SetDamageType( DMG_BLAST );

	CTFRadiusDamageInfo radiusinfo( &info, origin, flBlastRadius, pPlayer );
	TFGameRules()->RadiusDamage( radiusinfo );

#endif
	return true;
}

//-----------------------------------------------------------------------------
bool CTFSpellBook::CastSelfSpeedBoost( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	// Give a little health
	pPlayer->TakeHealth( 100, DMG_GENERIC );

	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_TINY, 20, pPlayer );
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_SPEED_BOOST, 20, pPlayer );
#endif
	return true;
}

//-----------------------------------------------------------------------------
bool CTFSpellBook::CastSelfStealth( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	// Grant a small amount of health
	pPlayer->TakeHealth( 40, DMG_GENERIC );
	pPlayer->m_Shared.AddCond( TF_COND_STEALTHED_USER_BUFF, 8, pPlayer );
#endif
	return true;
}

//********************************************************************************************************************************
//-----------------------------------------------------------------------------
// Kart Self Spells
//-----------------------------------------------------------------------------
bool CTFSpellBook::CastKartRocketJump( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	const float flBlastRadius = 250.f;

	// Set z to zero then add impulse
	// make this proper jumping later
	Vector vel = pPlayer->GetAbsVelocity();
	if ( vel.z < 0 )
	{
		vel.z = 0;
	}
	pPlayer->SetAbsVelocity( vel );

	Vector vForward( 0, 0, 1200 );
	pPlayer->ApplyAbsVelocityImpulse( vForward );

	const Vector& origin = pPlayer->GetAbsOrigin();
	CPVSFilter filter( origin );
	TE_TFParticleEffect( filter, 0.0, "bombinomicon_burningdebris", origin, vec3_angle );
	TE_TFParticleEffect( filter, 0.0, "heavy_ring_of_fire", origin, vec3_angle );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pPlayer, "foot_L" );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pPlayer, "foot_R" );

	// Give a little health to compensate for fall damage
	//pPlayer->TakeHealth( 25, DMG_GENERIC );
	pPlayer->RemoveFlag( FL_ONGROUND );
	pPlayer->m_Shared.AddCond( TF_COND_PARACHUTE_ACTIVE );

	// Collect players and cause knockback to enemies
	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( pPlayer, COLLISION_GROUP_PROJECTILE );

	// Trace entity radius
	CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
	int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), origin, flBlastRadius, FL_CLIENT | FL_FAKECLIENT | FL_NPC );
	for ( int i = 0; i < iEntities; ++i )
	{
		CTFPlayer *pTarget = ToTFPlayer( pListOfEntities[i] );
		
		if ( !pTarget || !pTarget->IsAlive() || pTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
			continue;

		// Do a quick trace to see if there's any geometry in the way.
		trace_t trace;
		UTIL_TraceLine( origin, pTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
		if ( trace.DidHitWorld() )
			continue;

		Vector vecDir = pTarget->WorldSpaceCenter() - origin;
		vecDir.NormalizeInPlace();
		vecDir.z += 0.5f;
		pTarget->AddHalloweenKartPushEvent( pPlayer, pPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ), pPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ), vecDir * tf_halloween_kart_normal_speed.GetFloat(), 30.0f );
	}
#endif
	return true;
}

bool CTFSpellBook::CastKartUber( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	Vector origin = pPlayer->GetAbsOrigin();
	CPVSFilter filter( origin );
	const char* pszEffectName = pPlayer->GetTeamNumber() == TF_TEAM_RED ? "spell_overheal_red" : "spell_overheal_blue";
	TE_TFParticleEffect( filter, 0.0, pszEffectName, origin, vec3_angle, pPlayer, PATTACH_ABSORIGIN_FOLLOW );

	//pPlayer->EmitSound( "BaseExplosionEffect.Sound" );

	// Collect players and cause knockback to enemies
	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( pPlayer, COLLISION_GROUP_PROJECTILE );

	pPlayer->m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, 7, pPlayer );
	pPlayer->AddKartDamage( -50 ); //Heal
#endif
	return true;
}


bool CTFSpellBook::CastKartBombHead( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_BOMB_HEAD, 10, pPlayer );
#endif
	return true;
}

//************************************************************************************************************************
// Spell Projectiles
//************************************************************************************************************************
class CTFProjectile_SpellFireball : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_SpellFireball, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SPELLBOOK_PROJECTILE; }
	virtual float		GetDamageRadius() const				{ return 200.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE	{ return m_bIsMeteor ? TF_DMG_CUSTOM_SPELL_METEOR : TF_DMG_CUSTOM_SPELL_FIREBALL; }
	virtual bool		IsDeflectable() OVERRIDE { return false; }

	void				SetMeteor( bool bIsMeteor ) { m_bIsMeteor = bIsMeteor; }

	CTFProjectile_SpellFireball()
	{
		m_bIsMeteor = false;
#ifdef GAME_DLL
		//m_pszExplodeParticleName = "ExplosionCore_buildings";
		m_pszExplodeParticleName = "bombinomicon_burningdebris";
#endif // GAME_DLL
	}
	
#ifdef GAME_DLL
	virtual void Spawn() OVERRIDE
	{
		SetModelScale( GetFireballScale() );
		BaseClass::Spawn();
	}
	virtual int UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_PVSCHECK ); }

	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE
	{
		Assert( pOther );
		if ( !pOther || !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) || pOther->IsFuncLOD() )
			return;

		if ( pOther->GetParent() == GetOwnerEntity() )
			return;

		// Handle hitting skybox (disappear).
		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

		if( pTrace->surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		// pass through ladders
		if( pTrace->surface.flags & CONTENTS_LADDER )
			return;

		if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
			return;

		Explode( pTrace );

		UTIL_Remove( this );
	}

	virtual void Explode( const trace_t *pTrace )
	{
		SetModelName( NULL_STRING );//invisible
		AddSolidFlags( FSOLID_NOT_SOLID );

		m_takedamage = DAMAGE_NO;

		// Pull out of the wall a bit.
		if ( pTrace->fraction != 1.0 )
		{
			SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
		}

		CTFPlayer *pThrower = ToTFPlayer( GetOwnerEntity() );
		if ( pThrower )
		{
			const Vector &vecOrigin = GetAbsOrigin();

			// Any effects from the initial explosion
			if ( InitialExplodeEffects( pThrower, pTrace ) )
			{
				// Particle
				if ( GetExplodeEffectParticle() )
				{	
					CPVSFilter filter( vecOrigin );
					TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );
				}

				// Sounds
				if ( GetExplodeEffectSound() )
				{
					EmitSound( GetExplodeEffectSound() );
				}

				// Treat this trace exactly like radius damage
				CTraceFilterIgnorePlayers traceFilter( pThrower, COLLISION_GROUP_PROJECTILE );

				// Splash pee on everyone nearby.
				CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
				int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), vecOrigin, GetDamageRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC );
				for ( int i = 0; i < iEntities; ++i )
				{
					CBaseCombatCharacter *pBasePlayer = NULL;
					CTFPlayer *pPlayer = ToTFPlayer( pListOfEntities[i] );
					if ( !pPlayer )
					{
						pBasePlayer = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
					}
					else
					{
						pBasePlayer = pPlayer;
					}

					if ( !pBasePlayer || !pPlayer || !pPlayer->IsAlive() )
						continue;

					// Do a quick trace to see if there's any geometry in the way.
					trace_t trace;
					UTIL_TraceLine( vecOrigin, pPlayer->WorldSpaceCenter(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
					//debugoverlay->AddLineOverlay( vecOrigin, pPlayer->WorldSpaceCenter(), 255, 0, 0, false, 10 );
					if ( trace.DidHitWorld() )
						continue;

					// Effects on the individual players
					ExplodeEffectOnTarget( pThrower, pPlayer, pBasePlayer );
				}

				if ( TFGameRules() )
				{
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_FIREBALL, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
				}

				CTakeDamageInfo info;
				info.SetAttacker( pThrower );
				info.SetInflictor( this ); 
				info.SetWeapon( GetLauncher() );
				info.SetDamage( 10.f );
				info.SetDamageCustom( GetCustomDamageType() );
				info.SetDamagePosition( vecOrigin );
				info.SetDamageType( DMG_BLAST );

				CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, GetDamageRadius(), pThrower );
				TFGameRules()->RadiusDamage( radiusinfo );
			}
			else
			{
				pThrower->EmitSound( "Player.DenyWeaponSelection" );
			}
		}

		// Grenade remove
		//SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );

		// Remove the rocket.
		UTIL_Remove( this );
		
		SetTouch( NULL );
		AddEffects( EF_NODRAW );
		SetAbsVelocity( vec3_origin );
	}	

	virtual const char *GetProjectileModelName( void ) { return ""; } // We dont have a model by default, and that's OK
	
	virtual bool		InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) { return true; }
	virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )
	{
		if ( pBaseTarget->GetTeamNumber() == GetTeamNumber() )
			return;

		if ( pTarget )
		{
			if ( pTarget->m_Shared.IsInvulnerable() )
				return;

			if ( pTarget->m_Shared.InCond( TF_COND_PHASE ) || pTarget->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
				return;

			pTarget->m_Shared.SelfBurn( 5.0f );
		}

		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

		CBaseEntity *pInflictor = GetLauncher();
		CTakeDamageInfo info;
		info.SetAttacker( pThrower );
		info.SetInflictor( this ); 
		info.SetWeapon( pInflictor );
		info.SetDamage( 100.f );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_BURN );

		// Hurt 'em.
		Vector dir;
		AngleVectors( GetAbsAngles(), &dir );
		pBaseTarget->DispatchTraceAttack( info, dir, pNewTrace );
		ApplyMultiDamage();


		Vector vecDir = pBaseTarget->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDir );
		vecDir.z = 0.1f;

		if ( pTarget )
		{
			pTarget->ApplyGenericPushbackImpulse( vecDir * 5, pThrower );
		}
	}

	virtual const char *GetExplodeEffectParticle() const	{ return m_pszExplodeParticleName; }
	void SetExplodeParticleName( const char *pszName )		{ m_pszExplodeParticleName = pszName; }
	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_fireball_impact"; }
#endif

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName( void )
	{ 
		return GetTeamNumber() == TF_TEAM_BLUE ? "spell_fireball_small_blue" : "spell_fireball_small_red";
	}
#endif

protected:
	virtual float GetFireballScale() const { return 0.01f; }

private:
	bool m_bIsMeteor;

#ifdef GAME_DLL
	const char *m_pszExplodeParticleName;
#endif // GAME_DLL
};

// Fireball
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellFireball, DT_TFProjectile_SpellFireball )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellFireball, DT_TFProjectile_SpellFireball )
	END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_spellfireball, CTFProjectile_SpellFireball );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellfireball);


// *************************************************************************************************************************
class CTFProjectile_SpellBats : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_SpellBats, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SPELLBOOK_PROJECTILE; }
	virtual float		GetDamageRadius() const				{ return 250.0f; }
	virtual float		GetModelScale() const				{ return 0.01f; }
	virtual int			GetCustomDamageType() const OVERRIDE	{ return TF_DMG_CUSTOM_SPELL_BATS; }
	virtual bool		IsDeflectable() OVERRIDE { return false; }

#ifdef GAME_DLL
	virtual void Spawn( void )
	{
		SetModelScale( GetModelScale() );
		BaseClass::Spawn();
	}

	//-----------------------------------------------------------------------------
	// Lightning Ball / Base
	//-----------------------------------------------------------------------------
	virtual void Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE
	{
		SetModelName( NULL_STRING );//invisible
		AddSolidFlags( FSOLID_NOT_SOLID );

		m_takedamage = DAMAGE_NO;

		// Pull out of the wall a bit.
		if ( pTrace->fraction != 1.0 )
		{
			SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
		}

		CTFPlayer *pThrower = ToTFPlayer( GetThrower() );

		if ( pThrower )
		{
			const Vector& vecOrigin = GetAbsOrigin();

			// Any effects from the initial explosion
			if ( InitialExplodeEffects( pThrower, pTrace ) )
			{
				// Particle
				if ( GetExplodeEffectParticle() )
				{	
					CPVSFilter filter( vecOrigin );
					TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );
				}

				// Sounds
				if ( GetExplodeEffectSound() )
				{
					EmitSound( GetExplodeEffectSound() );
				}

				// Treat this trace exactly like radius damage
				CTraceFilterIgnorePlayers traceFilter( pThrower, COLLISION_GROUP_PROJECTILE );

				// Splash pee on everyone nearby.
				CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
				int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), vecOrigin, GetDamageRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC );
				for ( int i = 0; i < iEntities; ++i )
				{
					CBaseCombatCharacter *pBasePlayer = NULL;
					CTFPlayer *pPlayer = ToTFPlayer( pListOfEntities[i] );
					if ( !pPlayer )
					{
						pBasePlayer = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
					}
					else
					{
						pBasePlayer = pPlayer;
					}

					if ( !pBasePlayer || !pBasePlayer->IsAlive() )
						continue;

					// Do a quick trace to see if there's any geometry in the way.
					trace_t trace;
					UTIL_TraceLine( GetAbsOrigin(), pBasePlayer->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
					if ( trace.DidHitWorld() )
						continue;

					// Effects on the individual players
					ExplodeEffectOnTarget( pThrower, pPlayer, pBasePlayer );
				}

				if ( TFGameRules() )
				{
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MERASMUS_ZAP, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
				}

				ApplyBlastDamage( pThrower, vecOrigin );
			}
			else
			{
				pThrower->EmitSound( "Player.DenyWeaponSelection" );
			}
		}

		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
		SetTouch( NULL );

		AddEffects( EF_NODRAW );
		SetAbsVelocity( vec3_origin );
	}

	virtual void ApplyBlastDamage( CTFPlayer *pThrower, Vector vecOrigin )
	{
		CTakeDamageInfo info;
		info.SetAttacker( pThrower );
		info.SetInflictor( this ); 
		info.SetWeapon( GetLauncher() );
		info.SetDamage( 10.f );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( vecOrigin );
		info.SetDamageType( DMG_BLAST );

		CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, GetDamageRadius(), pThrower );
		TFGameRules()->RadiusDamage( radiusinfo );
	}

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace )
	{
		// Added Particle
		Vector vecOrigin = GetAbsOrigin();
		// Particle
		CPVSFilter filter( vecOrigin );
		TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, TF_WEAPON_GRENADE_PIPEBOMB, kInvalidEHandleExplosion, -1, SPECIAL1, INVALID_STRING_INDEX );

		return true;
	}

	virtual void ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )
	{
		if ( pBaseTarget->GetTeamNumber() == GetTeamNumber() )
			return;

		if ( pTarget )
		{
			if ( pTarget->m_Shared.IsInvulnerable() )
				return;

			if ( pTarget->m_Shared.InCond( TF_COND_PHASE ) || pTarget->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
				return;

			// Stun the target
			pTarget->m_Shared.StunPlayer( 0.5, 0.5, TF_STUN_MOVEMENT, pThrower );
		}

		Vector vecDir = pBaseTarget->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDir );

		if ( pTarget )
		{
			pTarget->ApplyGenericPushbackImpulse( vecDir * 200.0f + Vector(0, 0, 800 ), pThrower );
			const char* pszEffectName = GetTeamNumber() == TF_TEAM_RED ? "spell_batball_red" : "spell_batball_blue";
			DispatchParticleEffect( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, pTarget );

			CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
			if ( pSpellBook )
			{
				pTarget->m_Shared.MakeBleed( pThrower, pSpellBook, 3.0f );
			}
		}
		else
		{
			pBaseTarget->ApplyAbsVelocityImpulse( vecDir * 1000.0f );
		}

		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

		CBaseEntity *pInflictor = GetLauncher();
		CTakeDamageInfo info;
		info.SetAttacker( pThrower );
		info.SetInflictor( this ); 
		info.SetWeapon( pInflictor );
		info.SetDamage( 40 );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_BURN );

		// Hurt 'em.
		Vector dir;
		AngleVectors( GetAbsAngles(), &dir );
		pBaseTarget->DispatchTraceAttack( info, dir, pNewTrace );
		ApplyMultiDamage();
	}

	virtual const char *GetExplodeEffectParticle() const	{ return GetTeamNumber() == TF_TEAM_RED ? "spell_batball_impact_red" : "spell_batball_impact_blue"; }
	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_bat_impact"; }
#endif

#ifdef CLIENT_DLL
	virtual const char*	GetTrailParticleName( void ) { return GetTeamNumber() == TF_TEAM_RED ? "spell_batball_throw_red" : "spell_batball_throw_blue"; }
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellBats, DT_TFProjectile_SpellBats )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellBats, DT_TFProjectile_SpellBats )
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellbats, CTFProjectile_SpellBats );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellbats );

// *************************************************************************************************************************
class CTFProjectile_SpellSpawnZombie : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellSpawnZombie, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	CTFProjectile_SpellSpawnZombie()
	{
#ifdef GAME_DLL
		m_skeletonType = 0;
#endif // GAME_DLL
	}

	virtual float		GetDamageRadius()	const			{ return 1.0f; }
	virtual void		SetCustomPipebombModel()			{ SetModel( "models/props_mvm/mvm_human_skull_collide.mdl" ); }
	virtual float		GetModelScale() const				{ return 1.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE { return TF_DMG_CUSTOM_SPELL_SKELETON; }

#ifdef GAME_DLL

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) { }
	virtual void PipebombTouch( CBaseEntity *pOther ) { }

	virtual void Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE
	{
		// no owner? spawn skeletons anyways
		if ( !GetThrower() )
		{
			InitialExplodeEffects( NULL, pTrace );
			
			// Particle
			if ( GetExplodeEffectParticle() )
			{	
				CPVSFilter filter( GetAbsOrigin() );
				TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), GetAbsOrigin(), vec3_angle );
			}

			// Sounds
			if ( GetExplodeEffectSound() )
			{
				EmitSound( GetExplodeEffectSound() );
			}

			SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
			SetTouch( NULL );

			AddEffects( EF_NODRAW );
			SetAbsVelocity( vec3_origin );

			return;
		}

		BaseClass::Explode( pTrace, bitsDamageType );
	}

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		// Pull in a little
		Vector vSpawnPoint = ( pTrace->endpos + ( pTrace->plane.normal * 2.0f ) );
		CZombie::SpawnAtPos( vSpawnPoint, 30.0f, GetTeamNumber(), pThrower, (CZombie::SkeletonType_t)m_skeletonType );
		return true;
	}
	virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) { }
	virtual const char *GetExplodeEffectParticle() const
	{
		if ( GetTeamNumber() == TF_TEAM_HALLOWEEN )
		{
			return "spell_skeleton_goop_green";
		}

		return GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue";
	}
	virtual const char *GetExplodeEffectSound()	const	{ return "Cleaver.ImpactFlesh"; }

	void SetSkeletonType ( int iType ) { m_skeletonType = iType; }

	int m_skeletonType;
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return "unusual_bubbles_green_fumes"; }
#endif
};

// Spawn Zombie
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellSpawnZombie, DT_TFProjectile_SpellSpawnZombie )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellSpawnZombie, DT_TFProjectile_SpellSpawnZombie )
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellspawnzombie, CTFProjectile_SpellSpawnZombie );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellspawnzombie );

#ifdef GAME_DLL

CBaseEntity* CreateSpellSpawnZombie( CBaseCombatCharacter *pCaster, const Vector& vSpawnPosition, int nSkeletonType )
{
	Vector offset = RandomVector( -32, 32 );
	offset.z = 16;
	CTFProjectile_SpellSpawnZombie *pGrenade = static_cast<CTFProjectile_SpellSpawnZombie*>( CBaseEntity::CreateNoSpawn( "tf_projectile_spellspawnzombie", vSpawnPosition + offset, RandomAngle( 0, 360 ), pCaster ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		Vector vecImpulse = RandomVector( -1, 1 );
		VectorNormalize( vecImpulse );
		vecImpulse.z = RandomFloat( 1.0f, 1.6f );
		Vector vecVelocity = vecImpulse * RandomFloat( 250.0f, 300.0f );

		AngularImpulse angVelocity = AngularImpulse( 300, 300, 100 );
		pGrenade->InitGrenade( vecVelocity, angVelocity, pCaster, 0, 0 );
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetDetonateTimerLength( RandomFloat( 2.f, 2.5f ) );

		pGrenade->SetSkeletonType( nSkeletonType );
	}

	return pGrenade;
}

#endif

// *************************************************************************************************************************
class CTFProjectile_SpellSpawnHorde : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellSpawnHorde, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	virtual float		GetDamageRadius()	const			{ return 1.0f; }
	virtual void		SetCustomPipebombModel()			{ SetModel( "models/props_mvm/mvm_human_skull_collide.mdl" ); }
	virtual float		GetModelScale() const				{ return 1.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE { return TF_DMG_CUSTOM_SPELL_SKELETON; }

#ifdef GAME_DLL
	//virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) { }
	//virtual void PipebombTouch( CBaseEntity *pOther ) { }

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		// Spawn a tonne of extra skelatone grenades (mirv style)

		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( !pSpellBook )
			return false;

		for ( int i = 0; i < 3; i++ )
		{
			Vector offset = RandomVector( -32, 32 );
			offset.z = 16;
			CreateSpellSpawnZombie( pThrower, GetAbsOrigin(), 0 );
		}

		if ( TFGameRules() )
		{
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_SKELETON_HORDE, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
		}

		return true;
	}
	virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) { }
	virtual const char *GetExplodeEffectParticle() const	{ return GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue"; }
	virtual const char *GetExplodeEffectSound()	const	{ return "Halloween.spell_skeleton_horde_rise"; }
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return "unusual_bubbles_green_fumes"; }
#endif
};

// Spawn Horde of Skels
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellSpawnHorde, DT_TFProjectile_SpellSpawnHorde )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellSpawnHorde, DT_TFProjectile_SpellSpawnHorde )
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellspawnhorde, CTFProjectile_SpellSpawnHorde );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellspawnhorde);


// *************************************************************************************************************************
class CTFProjectile_SpellPumpkin : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellPumpkin, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	CTFProjectile_SpellPumpkin ()
	{
#ifdef GAME_DLL
		m_flImpactTime = gpGlobals->curtime + 1.0f;
#endif
	}
	virtual float		GetDamageRadius()	const			{ return 1.0f; }
	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/w_models/w_cannonball.mdl" ); }
	virtual float		GetModelScale() const				{ return 0.75f; }
	virtual int			GetCustomDamageType() const OVERRIDE	{ return TF_DMG_CUSTOM_SPELL_MIRV; }
#ifdef GAME_DLL
	// ignore collisions early in its lifetime
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) 
	{
		if ( gpGlobals->curtime < m_flImpactTime )
			return;
		BaseClass::VPhysicsCollision( index, pEvent );
	}
	virtual void PipebombTouch( CBaseEntity *pOther )
	{
		if ( gpGlobals->curtime < m_flImpactTime )
			return;
		BaseClass::PipebombTouch( pOther );
	}

	virtual void ApplyBlastDamage ( CTFPlayer *pThrower, Vector vecOrigin ) { }

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		// Spawn a pumkin bomb here
		// Set the angles to what I want
		QAngle angle(0, RandomFloat( 0, 360 ) ,0);
		CTFPumpkinBomb *pGrenade = static_cast<CTFPumpkinBomb*>( CBaseEntity::CreateNoSpawn( "tf_pumpkin_bomb", GetAbsOrigin(), angle, NULL ) );
		if ( pGrenade )
		{
			pGrenade->SetInitParams( 0.60, 80.0f, 200.0f, GetTeamNumber(), 40.0f + RandomFloat(0 , 1.0f) );
			DispatchSpawn( pGrenade );
			pGrenade->SetSpell( true );
		}

		if ( TFGameRules() )
		{
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MIRV, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
		}

		return true;
	}
	virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) { }
	virtual const char *GetExplodeEffectParticle() const	{ return GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue"; }
	virtual const char *GetExplodeEffectSound()	const	{ return "Halloween.spell_mirv_explode_secondary"; }

	float m_flImpactTime;
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return "unusual_bubbles_green_fumes"; }
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellPumpkin, DT_TFProjectile_SpellPumpkin )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellPumpkin, DT_TFProjectile_SpellPumpkin)
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellpumpkin, CTFProjectile_SpellPumpkin );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellpumpkin);


// *************************************************************************************************************************
class CTFProjectile_SpellMirv : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellMirv, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	virtual float		GetDamageRadius()	const			{ return 1.0f; }
	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/w_models/w_cannonball.mdl" ); }
	virtual float		GetModelScale() const				{ return 0.9f; }
	virtual int			GetCustomDamageType() const OVERRIDE	{ return TF_DMG_CUSTOM_SPELL_MIRV; }

#ifdef GAME_DLL
	//virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) { }
	//virtual void PipebombTouch( CBaseEntity *pOther ) { }

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		// Spawn a tonne of extra grenades (mirv style)
		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( !pSpellBook )
			return false;

		// Create bomblets
		Vector offset = Vector( 0, -100, 400 );

		for ( int i = 0; i < 6; i++ )
		{
			AngularImpulse angVelocity = AngularImpulse( 0, 0, RandomFloat( 100, 300) );

			switch ( i )
			{
				case 0: offset = Vector(  75,  110, 400 ); break;
				case 1: offset = Vector(  75, -110, 400 ); break;
				case 2: offset = Vector( -75,  110, 400 ); break;
				case 3: offset = Vector( -75, -110, 400 ); break;
				case 4: offset = Vector( 135,  0, 400 ); break;
				case 5: offset = Vector( -135, 0, 400 ); break;
			}

			CTFProjectile_SpellPumpkin *pGrenade = static_cast<CTFProjectile_SpellPumpkin*>( CBaseEntity::CreateNoSpawn( "tf_projectile_spellpumpkin", GetAbsOrigin(), pThrower->EyeAngles(), pThrower ) );
			if ( pGrenade )
			{
				// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
				pGrenade->SetPipebombMode();
				DispatchSpawn( pGrenade );
				pGrenade->InitGrenade( offset, angVelocity, pThrower, pSpellBook->GetTFWpnData() );
				pGrenade->m_flFullDamage = 0;
				pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
				pGrenade->SetDetonateTimerLength( 2.0f + 0.05f * i );
			}
		}

		if ( TFGameRules() )
		{
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MIRV, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
		}

		return true;
	}
	virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) { }
	virtual const char *GetExplodeEffectParticle() const	{ return GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue"; }
	virtual const char *GetExplodeEffectSound()	const	{ return "Halloween.spell_mirv_explode_primary"; }
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return "unusual_bubbles_green_fumes"; }
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellMirv, DT_TFProjectile_SpellMirv )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellMirv, DT_TFProjectile_SpellMirv)
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellmirv, CTFProjectile_SpellMirv );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellmirv);

// *************************************************************************************************************************
class CTFProjectile_SpellSpawnBoss : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellSpawnBoss, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	virtual float		GetDamageRadius()	const			{ return 1.0f; }
	virtual void		SetCustomPipebombModel()			{ SetModel( "models/props_mvm/mvm_human_skull_collide.mdl" ); }
	virtual float		GetModelScale() const				{ return 1.5f; }
	virtual int			GetCustomDamageType() const OVERRIDE { return TF_DMG_CUSTOM_SPELL_MONOCULUS; }

#ifdef GAME_DLL

		//virtual void		Explode( trace_t *pTrace, int bitsDamageType );	
		virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
		{
			const Vector &vContactPoint = pTrace->endpos;
			CHalloweenBaseBoss *pBoss = CHalloweenBaseBoss::SpawnBossAtPos( HALLOWEEN_BOSS_MONOCULUS, vContactPoint, pThrower->GetTeamNumber(), pThrower );
			if ( pBoss )
			{
				float flDesiredHeight = tf_eyeball_boss_hover_height.GetFloat();

				const Vector &vMins = pBoss->WorldAlignMins();
				const Vector &vMaxs = pBoss->WorldAlignMaxs();
				Vector vSize = vMaxs - vMins;

				float flBossHeight = vSize.z;
				float flBossHalfX = 0.5f * vSize.x;
				float flBossHalfY = 0.5f * vSize.y;

				static Vector vTest[] =
				{
					Vector( 0, 0, flBossHeight ),
					Vector( flBossHalfX, flBossHalfY, flBossHeight ),
					Vector( -flBossHalfX, -flBossHalfY, flBossHeight ),
					Vector( flBossHalfX, -flBossHalfY, flBossHeight ),
					Vector( -flBossHalfX, flBossHalfY, flBossHeight )
				};

				bool bFoundValidSpawnPos = false;
				for ( int i=0; i<ARRAYSIZE( vTest ); ++i )
				{
					trace_t result;
					float bloat = 5.0f;
					Vector vStart = vContactPoint + vTest[i] + 30.f * pTrace->plane.normal;
					Vector vEnd = vStart + Vector( 0, 0, flDesiredHeight );

					CTraceFilterNoNPCsOrPlayer filter( pBoss, COLLISION_GROUP_NONE );
					UTIL_TraceHull( vStart, vEnd, vMins - Vector( bloat, bloat, 0 ), vMaxs + Vector( bloat, bloat, bloat ), MASK_SOLID | CONTENTS_PLAYERCLIP, &filter, &result );
					if ( !result.startsolid )
					{
						pBoss->SetAbsOrigin( result.endpos );
						bFoundValidSpawnPos = true;
						//NDebugOverlay::SweptBox( vStart, vEnd, vMins - Vector( bloat, bloat, 0 ), vMaxs + Vector( bloat, bloat, bloat ), vec3_angle, 0, 255, 0, 0, 5.f );
						//NDebugOverlay::Sphere( result.endpos, 10.f, 0, 255, 0, true, 5.f );
						
						break;
					}
					else
					{
						// Maybe we should play fail sound here?
						//NDebugOverlay::SweptBox( vStart, vEnd, vMins - Vector( bloat, bloat, 0 ), vMaxs + Vector( bloat, bloat, bloat ), vec3_angle, 255, 0, 0, 0, 5.f );
						//NDebugOverlay::Sphere( result.endpos, 10.f, 255, 0, 0, true, 5.f );
					}
				}

				// couldn't find any valid position
				if ( !bFoundValidSpawnPos )
				{
					UTIL_Remove( pBoss );
					pBoss = NULL;
				}
			}

			// refund the player the spell
			if ( !pBoss )
			{
				CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
				if ( pSpellBook )
				{
					pSpellBook->SetSelectedSpell( GetSpellIndexFromContext( MP_CONCEPT_PLAYER_SPELL_MONOCULOUS ) );
				}

				return false;
			}

			if ( TFGameRules() )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MONOCULOUS, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
			}

			return true;
		}
		virtual void		ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) { }
		virtual const char *GetExplodeEffectParticle() const	{ return "eyeboss_death"; }
		virtual const char *GetExplodeEffectSound()	const	{ return "Halloween.spell_spawn_boss"; }
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return "unusual_bubbles_green_fumes"; }
#endif
};

// Spawn Boss
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellSpawnBoss, DT_TFProjectile_SpellSpawnBoss )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellSpawnBoss, DT_TFProjectile_SpellSpawnBoss )
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spellspawnboss, CTFProjectile_SpellSpawnBoss );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellspawnboss );


// *************************************************************************************************************************
#ifdef GAME_DLL
class CTFSpell_MeteorShowerSpawner : public CBaseEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFSpell_MeteorShowerSpawner, CBaseEntity );

	virtual void Spawn() OVERRIDE
	{
		m_flFinishTime = gpGlobals->curtime + 4.f;
		SetContextThink( &CTFSpell_MeteorShowerSpawner::MeteorShowerThink, gpGlobals->curtime, "MeteorShowerThink" );
	}

	void MeteorShowerThink( void )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pPlayer && m_flFinishTime > gpGlobals->curtime )
		{
			// the owner changed team? remove this
			if ( pPlayer->GetTeamNumber() != GetTeamNumber() )
			{
				UTIL_Remove( this );
				return;
			}

			// Determine our "height" offset range based on surface normal
			Vector vecDir = Vector( 0.f, 0.f, 1.f );
			float flOffsetMin = 400.f;
			float flOffsetMax = 500.f;
			if ( m_vecImpactNormal.z <= -0.6f )	// Ceiling?
			{
				flOffsetMin = 45.f;
				flOffsetMax = 60.f;
				vecDir.z = -1.f;
			}
			const float flRange = 200.f;
			const float flRandomAngleOffset = 75.f;

			const int nNumToSpawn = random->RandomInt( 1, 2 );
			for ( int i = 0; i < nNumToSpawn; ++i )
			{
				// Vary start point away from surface center
				Vector vecOnPlane = Vector( RandomFloat( -flRange, flRange ), RandomFloat( -flRange, flRange ), 0.f ).Normalized();
				Vector vecPointOnPlane = GetAbsOrigin() + random->RandomFloat( -flRange, flRange ) * vecOnPlane;
				const float flOffsetFromPlane = random->RandomFloat( flOffsetMin, flOffsetMax );
				Vector vecEmit = vecPointOnPlane + flOffsetFromPlane * vecDir;

				// debugoverlay->AddLineOverlay( GetAbsOrigin(), vecEmit, 255, 0, 0, false, 10 );

				Vector vecVelocity = Vector( RandomFloat( -flRandomAngleOffset, flRandomAngleOffset ), RandomFloat( -flRandomAngleOffset, flRandomAngleOffset ), -700.f );

				// Check for a spot
				trace_t trace;
				UTIL_TraceLine( GetAbsOrigin(), vecEmit, ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), NULL, COLLISION_GROUP_NONE, &trace );
				if ( !trace.DidHit() )
				{
					SpawnMeteor( pPlayer, trace.endpos, vec3_angle, vecVelocity );
				}
				else
				{
					// Pull back and try again
					vecEmit = trace.endpos + ( trace.plane.normal * 1.0f );
					SpawnMeteor( pPlayer, vecEmit, vec3_angle, vecVelocity );
				}
			}

			SetContextThink( &CTFSpell_MeteorShowerSpawner::MeteorShowerThink, gpGlobals->curtime + 0.2f, "MeteorShowerThink" );
			return;
		}

		UTIL_Remove( this );
	}

	void SpawnMeteor( CTFPlayer *pOwner, const Vector &origin, const QAngle &angles, const Vector &velocity )
	{
		CTFProjectile_SpellFireball *pRocket = static_cast< CTFProjectile_SpellFireball* >( CBaseEntity::CreateNoSpawn( "tf_projectile_spellfireball", origin, angles, pOwner ) );
		if ( pRocket )
		{
			pRocket->SetOwnerEntity( pOwner );
			pRocket->SetLauncher( pOwner ); 
			pRocket->SetAbsVelocity( velocity );
			pRocket->SetDamage( 50.f );
			pRocket->SetMeteor( true );
			pRocket->ChangeTeam( GetTeamNumber() );
			const char *pszParticle = GetTeamNumber() == TF_TEAM_BLUE ? "spell_fireball_tendril_parent_blue" : "spell_fireball_tendril_parent_red";
			pRocket->SetExplodeParticleName( pszParticle );

			IPhysicsObject *pPhysicsObject = pRocket->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->AddVelocity( &velocity, NULL );
			}

			DispatchSpawn( pRocket );
		}
	}

	void SetImpaceNormal( Vector &vecNormal ) { m_vecImpactNormal = vecNormal; }

private:
	float m_flFinishTime;
	Vector m_vecImpactNormal;
};

// Meteor Shower
LINK_ENTITY_TO_CLASS( tf_spell_meteorshowerspawner, CTFSpell_MeteorShowerSpawner );

BEGIN_DATADESC( CTFSpell_MeteorShowerSpawner )
END_DATADESC()
#endif // GAME_DLL

// *************************************************************************************************************************
class CTFProjectile_SpellMeteorShower : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellMeteorShower, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	virtual float		GetModelScale() const				{ return 0.01f; }

#ifdef GAME_DLL
	virtual void Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE
	{
		m_takedamage = DAMAGE_NO;

		// Pull out of the wall a bit.
		if ( pTrace->fraction != 1.0 )
		{
			SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
		}

		Vector vecOrigin = GetAbsOrigin();

		// Particle
		if ( GetExplodeEffectParticle() )
		{	
			CPVSFilter filter( vecOrigin );
			TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );
		}

		// Sounds
		if ( GetExplodeEffectSound() )
		{
			EmitSound( GetExplodeEffectSound() );
		}

		CTFSpell_MeteorShowerSpawner *pSpawner = static_cast< CTFSpell_MeteorShowerSpawner* >( CBaseEntity::CreateNoSpawn( "tf_spell_meteorshowerspawner", vecOrigin, vec3_angle, GetThrower() ) );
		if ( pSpawner )
		{
			pSpawner->SetImpaceNormal( pTrace->plane.normal );
			pSpawner->ChangeTeam( GetTeamNumber() );
			DispatchSpawn( pSpawner );
		}

		if ( TFGameRules() )
		{
			CTFPlayer *pThrower = ToTFPlayer( GetOwnerEntity() );
			if ( pThrower )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_METEOR_SWARM, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
			}
		}

		SetModelName( NULL_STRING );
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetTouch( NULL );
		AddEffects( EF_NODRAW );
		SetAbsVelocity( vec3_origin );

		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	}

	virtual const char *GetExplodeEffectParticle() const	{ return "bomibomicon_ring"; }
	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_meteor_impact"; }
#endif // GAME_DLL

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName( void )
	{ 
		return GetTeamNumber() == TF_TEAM_BLUE ? "spell_fireball_small_blue" : "spell_fireball_small_red";
	}
#endif
};

// Meteor Shower
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellMeteorShower, DT_TFProjectile_SpellMeteorShower )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellMeteorShower, DT_TFProjectile_SpellMeteorShower)
	END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_spellmeteorshower, CTFProjectile_SpellMeteorShower );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellmeteorshower );


// *************************************************************************************************************************
class CTFProjectile_SpellTransposeTeleport : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellTransposeTeleport, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

	virtual void Spawn( void )
	{
		SetModelScale( 0.01f );
		BaseClass::Spawn();
		SetCollisionGroup( COLLISION_GROUP_PLAYER_MOVEMENT );
#ifdef GAME_DLL
		SetContextThink( &CTFProjectile_SpellTransposeTeleport::RecordPosThink, gpGlobals->curtime + 0.05f, "RecordThink" );
#endif
	}

	// FIX
	virtual int			GetWeaponID( void ) const			{ return TF_PROJECTILE_SPELL; }
	virtual float		GetDamageRadius()	const			{ return 5.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE { return TF_DMG_CUSTOM_SPELL_TELEPORT; }

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const
	{
		return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_PLAYERCLIP;
	}

#ifdef GAME_DLL

	void RecordPosThink( void )
	{
		m_vecTrailingPos.AddToTail( GetAbsOrigin() );

		// Only retain 5 positions
		if ( m_vecTrailingPos.Count() > 5 )
		{
			m_vecTrailingPos.Remove( 0 );
		}

		SetContextThink( &CTFProjectile_SpellTransposeTeleport::RecordPosThink, gpGlobals->curtime + 0.05f, "RecordThink" );
	}

	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		if ( !pThrower->IsAlive() )
			return false;

		// Grant a small amount of health
		pThrower->TakeHealth( 30, DMG_GENERIC );

		trace_t result;
		CTraceFilterIgnoreTeammates traceFilter( this, COLLISION_GROUP_PLAYER_MOVEMENT, GetTeamNumber() );
		unsigned int nMask = pThrower->GetTeamNumber() == TF_TEAM_RED ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
		nMask |= MASK_PLAYERSOLID;

		m_vecTrailingPos.AddToTail( pTrace->endpos + ( pTrace->plane.normal * 50.f ) );

		// Try a few spots
		FOR_EACH_VEC_BACK( m_vecTrailingPos, i )
		{
			// Try positions starting with the current, and moving back in time a bit
			Vector vecStart = m_vecTrailingPos[i];
			UTIL_TraceHull( vecStart, vecStart, VEC_HULL_MIN, VEC_HULL_MAX, nMask, &traceFilter, &result );

			if( !result.DidHit() )
			{
				// Place a teleport effect where they came from
				const Vector& vecOrigin = pThrower->GetAbsOrigin();
				CPVSFilter pvsFilter( vecOrigin );
				TE_TFParticleEffect( pvsFilter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );

				// Move 'em!
				pThrower->Teleport( &vecStart, &pThrower->GetAbsAngles(), NULL );

				// Do a zoom effect
				pThrower->SetFOV( pThrower, 0, 0.3f, 120 );

				// Screen flash
				color32 fadeColor = {255,255,255,100};
				UTIL_ScreenFade( pThrower, fadeColor, 0.25, 0.4, FFADE_IN );

				if ( TFGameRules() )
				{
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_TELEPORT, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
				}
				
				// Success!
				return true;
			}
		}

		return false;
	}
	virtual void ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )
	{
		// ...
	}
	virtual const char *GetExplodeEffectParticle() const	{ return "eyeboss_tp_player"; }
	virtual const char *GetExplodeEffectSound()	const	{ return "Building_Teleporter.Ready"; }
#endif

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void ) { return GetTeamNumber() == TF_TEAM_RED ? "spell_teleport_red" : "spell_teleport_blue"; }
#endif

private:
#ifdef GAME_DLL
	CUtlVector< Vector > m_vecTrailingPos;
#endif
};

// Spawn Boss
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellTransposeTeleport, SpellTransposeTeleport )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellTransposeTeleport, SpellTransposeTeleport )
	END_NETWORK_TABLE()

	LINK_ENTITY_TO_CLASS( tf_projectile_spelltransposeteleport, CTFProjectile_SpellTransposeTeleport );
PRECACHE_WEAPON_REGISTER( tf_projectile_spelltransposeteleport );

#ifdef GAME_DLL
void RemoveAll2013HalloweenTeleportSpellsInMidFlight( void )
{
	CBaseEntity *pTeleport = NULL;
	while ( ( pTeleport = gEntList.FindEntityByClassname( pTeleport, "tf_projectile_spelltransposeteleport" ) ) != NULL )
	{
		UTIL_Remove( pTeleport );
	}
}
#endif

// *************************************************************************************************************************
class CTFProjectile_SpellLightningOrb : public CTFProjectile_SpellFireball
{
public:
	DECLARE_CLASS( CTFProjectile_SpellLightningOrb, CTFProjectile_SpellFireball );
	DECLARE_NETWORKCLASS();

	~CTFProjectile_SpellLightningOrb()
	{
#ifdef CLIENT_DLL
		if ( m_pTrailParticle )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pTrailParticle );
			m_pTrailParticle = NULL;
		}
#endif // CLIENT_DLL
	}

#ifdef GAME_DLL

	virtual void Spawn() OVERRIDE
	{
		BaseClass::Spawn();

		// We dont want to collide with anything but the world
		SetSolid( SOLID_NONE );

		SetExplodeParticleName( GetTeamNumber() == TF_TEAM_BLUE ? "drg_cow_explosioncore_charged_blue" : "drg_cow_explosioncore_charged" );
		SetContextThink( &CTFProjectile_SpellLightningOrb::ZapThink, gpGlobals->curtime + 0.25f, "ZapThink" );
		SetContextThink( &CTFProjectile_SpellLightningOrb::VortexThink, gpGlobals->curtime + 0.2f, "VortexThink" );
		SetContextThink( &CTFProjectile_SpellLightningOrb::ExplodeAndRemove, gpGlobals->curtime + 5.f, "ExplodeAndRemoveThink" );
		SetDamage( 20.f );
	}

	virtual const char *GetProjectileModelName( void ) { return ""; } // We dont have a model by default, and that's OK

	virtual float		GetDamageRadius()	const			{ return 200.f; }
	virtual int			GetCustomDamageType() const OVERRIDE { return TF_DMG_CUSTOM_SPELL_LIGHTNING; }

	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE
	{
		Assert( pOther );
		if ( !pOther || !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) || pOther->IsFuncLOD() )
			return;

		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		// Bounce off the world
		if ( pOther->IsWorld() )
		{
			Vector vIntoSurface = pTrace->plane.normal * pTrace->plane.normal.Dot( GetAbsVelocity() );
			SetAbsVelocity( GetAbsVelocity() + ( -1.5f * vIntoSurface ) );
			return;
		}

		// Handle hitting skybox (disappear).
		if ( pTrace->surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		// pass through ladders
		if( pTrace->surface.flags & CONTENTS_LADDER )
			return;

		if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
			return;

		if ( pOther->IsPlayer() )
			return;

		// Spell ends when we run into something
		ExplodeAndRemove();
		return;
	}
	
	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
	{
		Zap( 16 );

		return true;
	}

	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_lightning_impact"; }
	virtual void ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget ) OVERRIDE
	{}

	void ExplodeAndRemove()
	{
		// Particle
		if ( GetExplodeEffectParticle() )
		{	
			CPVSFilter filter( GetAbsOrigin() );
			TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), GetAbsOrigin(), vec3_angle );

			EmitSound( filter, entindex(), GetExplodeEffectSound() );
		}

		// Go out with a bang
		Zap( 16 );
			
		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
		return;
	}

	void ZapThink()
	{
		Zap( 2 );
		SetContextThink( &CTFProjectile_SpellLightningOrb::ZapThink, gpGlobals->curtime + RandomFloat( 0.25f, 0.35f ), "ZapThink" );
	}

	void Zap( int nNumToZap )
	{
		CBaseEntity *pOwner = GetOwnerEntity();

		if ( !pOwner )
			return;
		
		CTakeDamageInfo info;
		info.SetAttacker( pOwner );
		info.SetInflictor( this ); 
		info.SetWeapon( GetLauncher() );
		info.SetDamage( GetDamage() );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_BURN );

		CBaseEntity *pListOfEntities[5];
		int iEntities = UTIL_EntitiesInSphere( pListOfEntities, 5, GetAbsOrigin(), GetDamageRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC );

		// Shuffle the list
		for( int i = iEntities - 1; i > 0; --i )
		{
			V_swap( pListOfEntities[i], pListOfEntities[ RandomInt( 0, i ) ] );
		}

		// Zap as many targets as we're told to, if we can
		int nHits = 0;
		for ( int i = 0; i < iEntities && nHits < nNumToZap; ++i )
		{
			CBaseEntity* pTarget = pListOfEntities[i];

			if ( !pTarget )
				continue;

			if ( !pTarget->IsAlive() )
				continue;

			if ( pOwner->InSameTeam( pTarget ) )
				continue;

			if ( !FVisible( pTarget, MASK_OPAQUE ) )
				continue;

			CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
			if ( pTFPlayer )
			{
				if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
					continue;

				if ( pTFPlayer->m_Shared.IsInvulnerable() )
					continue;
			}

			CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
			trace_t trace;
			UTIL_TraceLine( GetAbsOrigin(), pTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );
			if ( trace.DidHitWorld() )
				continue;

			// Shoot a beam at them
			CPVSFilter filter( pTarget->WorldSpaceCenter() );
			Vector vStart = WorldSpaceCenter();
			Vector vEnd = pTarget->EyePosition();
			const char *pszHitEffect = GetTeamNumber() == TF_TEAM_BLUE ? "spell_lightningball_hit_blue" : "spell_lightningball_hit_red";
			te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
			TE_TFParticleEffectComplex( filter, 0.0f, pszHitEffect, vStart, QAngle( 0, 0, 0 ), NULL, &controlPoint, pTFPlayer, PATTACH_CUSTOMORIGIN );

			// Hurt 'em.
			Vector dir;
			AngleVectors( GetAbsAngles(), &dir );
			pTarget->DispatchTraceAttack( info, dir, &trace );
			ApplyMultiDamage();

			++nHits;
		}

		// We zapped someone.  Play a sound
		if ( nHits > 0 )
		{
			pOwner->EmitSound( "TFPlayer.MedicChargedDeath" );

			if ( TFGameRules() )
			{
				CTFPlayer *pThrower = ToTFPlayer( GetOwnerEntity() );
				if ( pThrower )
				{
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_LIGHTNING_BALL, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
				}
			}
		}
	}

	void VortexThink( void )
	{
		const int nMaxEnts = 32;

		Vector vecPos = GetAbsOrigin();
		CBaseEntity	*pObjects[ nMaxEnts ];
		int nCount = UTIL_EntitiesInSphere( pObjects, nMaxEnts, vecPos, GetDamageRadius(), FL_CLIENT | FL_NPC );

		// NDebugOverlay::Sphere( vecPos, GetDamageRadius(), 0, 255, 0, false, 0.35f );

		// Iterate through sphere's contents
		for ( int i = 0; i < nCount; i++ )
		{
			CBaseCombatCharacter *pEntity = pObjects[i]->MyCombatCharacterPointer();
			if ( !pEntity )
				continue;

			if ( InSameTeam( pEntity ) )
				continue;

			if ( !FVisible( pEntity, MASK_OPAQUE ) )
				continue;

 			// Draw player toward us
			Vector vecSourcePos = pEntity->GetAbsOrigin();
			Vector vecTargetPos = GetAbsOrigin();
			Vector vecVelocity = ( vecTargetPos - vecSourcePos ) * 2.f;
			vecVelocity.z += 50.f;

			if ( pEntity->GetFlags() & FL_ONGROUND )
			{
				vecVelocity.z += 150.f;
				pEntity->SetGroundEntity( NULL );
				pEntity->SetGroundChangeTime( gpGlobals->curtime + 0.5f );
			}

			pEntity->Teleport( NULL, NULL, &vecVelocity );
		}

		SetContextThink( &CTFProjectile_SpellLightningOrb::VortexThink, gpGlobals->curtime + 0.2f, "VortexThink" );
		return;
	}
#endif

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName( void )		{ return NULL; } // CRUTGUN!
	virtual void CreateTrails( void )
	{
		BaseClass::CreateTrails();

		if ( !m_pTrailParticle )
		{
			m_pTrailParticle = ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_BLUE ? "spell_lightningball_parent_blue" : "spell_lightningball_parent_red" ), PATTACH_ABSORIGIN_FOLLOW );
		}
	}

private:
	CNewParticleEffect *m_pTrailParticle;
#endif // CLIENT_DLL
};

// Lightning ball
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellLightningOrb, DT_TFProjectile_SpellLightningOrb )
	BEGIN_NETWORK_TABLE( CTFProjectile_SpellLightningOrb, DT_TFProjectile_SpellLightningOrb )
	END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_lightningorb, CTFProjectile_SpellLightningOrb );
PRECACHE_WEAPON_REGISTER( tf_projectile_lightningorb);


#ifdef CLIENT_DLL
	#define CTFHellZap C_TFHellZap
#endif

#ifdef GAME_DLL
	#include "tf_obj_dispenser.h"
	#include "particle_parse.h"
	#include "tf_fx.h"
#endif

class CTFHellZap : public CBaseEntity 
{
	DECLARE_CLASS( CTFHellZap, CBaseEntity )
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
public:
	CTFHellZap()
#ifdef GAME_DLL
		: m_eType( ZAP_ON_TOUCH )
#endif
	{}
#ifdef GAME_DLL

	enum EZapperType
	{
		ZAP_ON_TOUCH,
		ZAP_ON_TEST,
	};

	virtual void Spawn()
	{
		m_bEnabled = true;

		if ( m_iszCustomTouchTrigger != NULL_STRING )
		{
			m_hTouchTrigger = dynamic_cast<CDispenserTouchTrigger *> ( gEntList.FindEntityByName( NULL, m_iszCustomTouchTrigger ) );

			if ( m_hTouchTrigger.Get() != NULL )
			{
				Assert( m_hTouchTrigger->GetOwnerEntity() == NULL );
				m_hTouchTrigger->SetOwnerEntity( this );	//owned
			}
		}
	}

	void ZapAllTouching()
	{
		FOR_EACH_VEC_BACK( m_vecZapTargets, i )
		{
			CBaseEntity* pZapTarget = m_vecZapTargets[i].Get();
			// Remove targets that have disappeared
			if ( !pZapTarget )
			{
				m_vecZapTargets.Remove( i );
				continue;
			}

			// Shoot a beam at them
			CPVSFilter filter( pZapTarget->WorldSpaceCenter() );
			Vector vStart = WorldSpaceCenter();
			Vector vEnd = pZapTarget->WorldSpaceCenter();
			te_tf_particle_effects_control_point_t controlPoint = { PATTACH_CUSTOMORIGIN, vEnd };
			TE_TFParticleEffectComplex( filter, 0.0f, m_iszParticleName.ToCStr(), vStart, QAngle( 0, 0, 0 ), NULL, &controlPoint, this, PATTACH_CUSTOMORIGIN );
		}
	}

	void ZapThink()
	{
		ZapAllTouching();

		// Keep zapping if we have targets
		if ( m_vecZapTargets.Count() )
		{
			SetContextThink( &CTFHellZap::ZapThink, gpGlobals->curtime + 0.25f, "ZapThink" );
		}
	}

	virtual void StartTouch( CBaseEntity *pEntity )
	{
		m_vecZapTargets.AddToTail( pEntity );

		if ( m_eType == ZAP_ON_TOUCH )
		{
			SetContextThink( &CTFHellZap::ZapThink, gpGlobals->curtime, "ZapThink" );
		}
	}

	virtual void EndTouch( CBaseEntity *pEntity )
	{
		int nIndex = m_vecZapTargets.Find( pEntity );
		if( nIndex != m_vecZapTargets.InvalidIndex() )
		{
			m_vecZapTargets.Remove( nIndex );
		}

		// No more targets.  Stop thinking!
		if ( m_vecZapTargets.Count() == 0 && m_eType == ZAP_ON_TOUCH )
		{
			SetContextThink( NULL, 0, "ZapThink" );
		}
	}

	void InputEnable( inputdata_t &inputdata )
	{
		m_bEnabled = true;

		if ( m_vecZapTargets.Count() > 0 && m_eType == ZAP_ON_TOUCH )
		{
			SetContextThink( &CTFHellZap::ZapThink, gpGlobals->curtime, "ZapThink" );
		}
	}

	void InputDisable( inputdata_t &inputdata )
	{
		m_bEnabled = false;
		SetContextThink( NULL, 0, "ZapThink" );
	}

	void InputZapAllTouching( inputdata_t &inputdata )
	{
		ZapAllTouching();
	}
private:

	EZapperType m_eType;
	bool	m_bEnabled;
	EHANDLE m_hTouchTrigger;
	string_t m_iszCustomTouchTrigger;
	string_t m_iszParticleName;
	CUtlVector< EHANDLE > m_vecZapTargets;
#endif
};


BEGIN_DATADESC( CTFHellZap )
#ifdef GAME_DLL
 	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ZapTouching", InputZapAllTouching ),

	DEFINE_KEYFIELD( m_iszCustomTouchTrigger, FIELD_STRING, "touch_trigger" ),
	DEFINE_KEYFIELD( m_iszParticleName, FIELD_STRING, "ParticleEffect" ),
	DEFINE_KEYFIELD( m_eType, FIELD_INTEGER, "ZapperType" ),
#endif
END_DATADESC()

LINK_ENTITY_TO_CLASS( halloween_zapper, CTFHellZap );
IMPLEMENT_NETWORKCLASS_ALIASED( TFHellZap, DT_TFHellZap )

BEGIN_NETWORK_TABLE( CTFHellZap, DT_TFHellZap )
END_NETWORK_TABLE()


//*******************************************************************************************************************************************************
// Kart Spells
//*******************************************************************************************************************************************************
class CTFProjectile_SpellKartOrb: public CTFProjectile_SpellFireball
{
public:
	DECLARE_CLASS( CTFProjectile_SpellKartOrb, CTFProjectile_SpellFireball );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	virtual void Spawn() OVERRIDE
	{
		BaseClass::Spawn();
		SetContextThink( &CTFProjectile_SpellKartOrb::ExplodeAndRemove, gpGlobals->curtime + tf_halloween_kart_rocketspell_lifetime.GetFloat(), "ExplodeAndRemoveThink" );
		SetContextThink( &CTFProjectile_SpellKartOrb::MoveChecking, gpGlobals->curtime + 0.05f, "MoveCheckingThink" );

		SetModel( SPELL_BOXING_GLOVE );
		SetModelScale( 2.5f );

		Vector mins( -20, -20, 0 );
		Vector maxs( 20, 20, 20 );
		UTIL_SetSize( this, mins, maxs );
	}

	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE
	{
		Assert( pOther );
		if ( !pOther || !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) || pOther->IsFuncLOD() )
			return;

		// Handle hitting skybox (disappear).
		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		if ( pTrace->surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		// Bounce off the world
		if ( pOther->IsWorld() )
		{
			Vector vOld = GetAbsVelocity();
			//float flSpeed = vOld.Length();
			Vector vNew = ( -2.0f * pTrace->plane.normal.Dot( vOld ) * pTrace->plane.normal + vOld );
			vNew.NormalizeInPlace();
			vNew *= tf_halloween_kart_rocketspell_speed.GetFloat();
			SetAbsVelocity( vNew );
			return;
		}

		// pass through ladders
		if ( pTrace->surface.flags & CONTENTS_LADDER )
			return;

		if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
			return;

		if ( pOther->IsPlayer() )
			ExplodeAndRemove();

		// Spell ends when we run into something
		//ExplodeAndRemove();
		return;
	}

	void MoveChecking ()
	{
		// do a short trace down, if nothing is there, add a bit of downward velocity
		trace_t pTrace;
		Vector vecSpot = GetAbsOrigin() ;
		UTIL_TraceLine( vecSpot, vecSpot - Vector(0, 0, 32), MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

		if ( pTrace.fraction >= 1.0  )
		{
			// Start moving down
			SetAbsVelocity( GetAbsVelocity() - Vector( 0, 0, 128 ) );
		}
		
		SetContextThink( &CTFProjectile_SpellKartOrb::MoveChecking, gpGlobals->curtime + 0.05f, "MoveCheckingThink" );
	}

	void ExplodeAndRemove()
	{
		// Handle hitting skybox (disappear).
		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

		if ( pTrace->surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		// pass through ladders
		if ( pTrace->surface.flags & CONTENTS_LADDER )
			return;

		Explode( pTrace );

		UTIL_Remove( this );
	}

	// We dont deal actual damage, just Car damage
	virtual void Explode( const trace_t *pTrace )
	{
		SetModelName( NULL_STRING );//invisible
		AddSolidFlags( FSOLID_NOT_SOLID );

		m_takedamage = DAMAGE_NO;

		// Pull out of the wall a bit.
		if ( pTrace->fraction != 1.0 )
		{
			SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
		}

		CTFPlayer *pThrower = ToTFPlayer( GetOwnerEntity() );
		if ( pThrower )
		{
			const Vector &vecOrigin = GetAbsOrigin();

			// Any effects from the initial explosion
			if ( InitialExplodeEffects( pThrower, pTrace ) )
			{
				// Particle
				if ( GetExplodeEffectParticle() )
				{
					CPVSFilter filter( vecOrigin );
					TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );
				}

				// Sounds
				if ( GetExplodeEffectSound() )
				{
					EmitSound( GetExplodeEffectSound() );
				}

				// Treat this trace exactly like radius damage
				CTraceFilterIgnorePlayers traceFilter( pThrower, COLLISION_GROUP_PROJECTILE );

				// Splash pee on everyone nearby.
				CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
				int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), vecOrigin, GetDamageRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC );
				for ( int i = 0; i < iEntities; ++i )
				{
					CBaseCombatCharacter *pBasePlayer = NULL;
					CTFPlayer *pPlayer = ToTFPlayer( pListOfEntities[i] );
					if ( !pPlayer )
					{
						pBasePlayer = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
					}
					else
					{
						pBasePlayer = pPlayer;
					}

					if ( !pBasePlayer || !pPlayer || !pPlayer->IsAlive() || InSameTeam(pPlayer) )
						continue;

					// Do a quick trace to see if there's any geometry in the way.
					trace_t trace;
					UTIL_TraceLine( vecOrigin, pPlayer->WorldSpaceCenter(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
					//debugoverlay->AddLineOverlay( vecOrigin, pPlayer->WorldSpaceCenter(), 255, 0, 0, false, 10 );
					if ( trace.DidHitWorld() )
						continue;

					// Effects on the individual players
					//ExplodeEffectOnTarget( pThrower, pPlayer, pBasePlayer );

					// Apply Car Damage and a force
					Vector vecDir = pPlayer->WorldSpaceCenter() - GetAbsOrigin();
					vecDir.NormalizeInPlace();
					Vector vecForward, vecRight, vecUp;
					AngleVectors( pPlayer->GetAnimRenderAngles(), &vecForward, &vecRight, &vecUp );
					vecDir += ( vecUp * 0.5f );
					pPlayer->AddHalloweenKartPushEvent( pThrower, this, pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ), vecDir * tf_halloween_kart_rocketspell_force.GetFloat(), 50.0f );
				}
			}
			else
			{
				pThrower->EmitSound( "Player.DenyWeaponSelection" );
			}
		}
	}

	virtual const char *GetExplodeEffectParticle() const	{ return "ExplosionCore_MidAir"; }
#endif

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName( void )
	{
		return "halloween_rockettrail";
	}
#endif
};

// Kart Spell Orbs
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellKartOrb, DT_TFProjectile_SpellKartOrb )
BEGIN_NETWORK_TABLE( CTFProjectile_SpellKartOrb, DT_TFProjectile_SpellKartOrb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_spellkartorb, CTFProjectile_SpellKartOrb );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellkartorb );

class CTFProjectile_SpellKartBats : public CTFProjectile_SpellBats
{
public:
	DECLARE_CLASS( CTFProjectile_SpellKartBats, CTFProjectile_SpellBats );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	virtual void ApplyBlastDamage( CTFPlayer *pThrower, Vector vecOrigin )
	{
		
	}

	virtual void ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )
	{
		if ( pBaseTarget->GetTeamNumber() == GetTeamNumber() )
			return;

		if ( !pTarget )
			return;

		if ( pTarget->m_Shared.IsInvulnerable() )
			return;

		if ( pTarget->m_Shared.InCond( TF_COND_PHASE ) || pTarget->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
			return;

		// Stun the target
		pTarget->m_Shared.StunPlayer( 0.5, 0.5, TF_STUN_MOVEMENT, pThrower );

		Vector vecDir = pBaseTarget->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDir );
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pTarget->GetAnimRenderAngles(), &vecForward, &vecRight, &vecUp );
		vecDir += ( vecUp * 0.5f );

		if ( pTarget )
		{
			pTarget->AddHalloweenKartPushEvent( pThrower, this, pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ), vecDir * tf_halloween_kart_normal_speed.GetFloat() * 1.10f, 45.0f );

			const char* pszEffectName = GetTeamNumber() == TF_TEAM_RED ? "spell_batball_red" : "spell_batball_blue";
			DispatchParticleEffect( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, pTarget );
		}
	}
#endif
};


IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellKartBats, DT_TFProjectile_SpellKartBats )
BEGIN_NETWORK_TABLE( CTFProjectile_SpellKartBats, DT_TFProjectile_SpellKartBats )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_spellkartbats, CTFProjectile_SpellKartBats );
PRECACHE_WEAPON_REGISTER( tf_projectile_spellkartbats );

//// *************************************************************************************************************************
//class CTFProjectile_SpellKartPumpkin : public CTFProjectile_SpellPumpkin
//{
//public:
//	DECLARE_CLASS( CTFProjectile_SpellKartPumpkin, CTFProjectile_SpellPumpkin );
//	DECLARE_NETWORKCLASS();
//
//#ifdef GAME_DLL
//	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
//	{
//		// Spawn a pumkin bomb here
//		// Set the angles to what I want
//		QAngle angle( 0, RandomFloat( 0, 360 ), 0 );
//		CTFPumpkinBomb *pGrenade = static_cast<CTFPumpkinBomb*>( CBaseEntity::CreateNoSpawn( "tf_pumpkin_bomb", GetAbsOrigin(), angle, NULL ) );
//		if ( pGrenade )
//		{
//			pGrenade->SetInitParams( 0.60, 80.0f, 200.0f, GetTeamNumber(), 40.0f + RandomFloat( 0, 1.0f ) );
//			DispatchSpawn( pGrenade );
//			pGrenade->SetSpell( true );
//			pGrenade->TakeDamage( CTakeDamageInfo( pThrower, pThrower, 10.f, DMG_CRUSH ) );
//		}
//
//		if ( TFGameRules() )
//		{
//			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MIRV, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
//		}
//
//		return true;
//	}
//#endif
//};
//
//IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellKartPumpkin, DT_TFProjectile_SpellKartPumpkin )
//BEGIN_NETWORK_TABLE( CTFProjectile_SpellKartPumpkin, DT_TFProjectile_SpellKartPumpkin )
//END_NETWORK_TABLE()
//
//LINK_ENTITY_TO_CLASS( tf_projectile_spellkartpumpkin, CTFProjectile_SpellKartPumpkin );
//PRECACHE_WEAPON_REGISTER( tf_projectile_spellkartpumpkin );
//
////*************
//// *************************************************************************************************************************
//class CTFProjectile_SpellKartMirv : public CTFProjectile_SpellMirv
//{
//public:
//	DECLARE_CLASS( CTFProjectile_SpellKartMirv, CTFProjectile_SpellMirv );
//	DECLARE_NETWORKCLASS();
//
//#ifdef GAME_DLL
//		
//	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace ) OVERRIDE
//	{
//		// Spawn a tonne of extra grenades (mirv style)
//		CTFSpellBook *pSpellBook = dynamic_cast<CTFSpellBook*>( pThrower->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
//		if ( !pSpellBook )
//			return false;
//
//		// Create bomblets
//		Vector offset = Vector( 0, -100, 400 );
//
//		for ( int i = 0; i < 6; i++ )
//		{
//			AngularImpulse angVelocity = AngularImpulse( 0, 0, RandomFloat( 100, 300 ) );
//
//			switch ( i )
//			{
//			case 0: offset = Vector( 75, 110, 400 ); break;
//			case 1: offset = Vector( 75, -110, 400 ); break;
//			case 2: offset = Vector( -75, 110, 400 ); break;
//			case 3: offset = Vector( -75, -110, 400 ); break;
//			case 4: offset = Vector( 135, 0, 400 ); break;
//			case 5: offset = Vector( -135, 0, 400 ); break;
//			}
//
//			CTFProjectile_SpellPumpkin *pGrenade = static_cast<CTFProjectile_SpellPumpkin*>( CBaseEntity::CreateNoSpawn( "tf_projectile_spellkartpumpkin", GetAbsOrigin(), pThrower->EyeAngles(), pThrower ) );
//			if ( pGrenade )
//			{
//				// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
//				pGrenade->SetPipebombMode();
//				DispatchSpawn( pGrenade );
//				pGrenade->InitGrenade( offset, angVelocity, pThrower, pSpellBook->GetTFWpnData() );
//				pGrenade->m_flFullDamage = 0;
//				pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
//				pGrenade->SetDetonateTimerLength( 2.0f + 0.05f * i );
//			}
//		}
//
//		if ( TFGameRules() )
//		{
//			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SPELL_MIRV, ( pThrower->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
//		}
//
//		return true;
//	}
//#endif
//};
//
//IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SpellKartMirv, DT_TFProjectile_SpellKartMirv )
//BEGIN_NETWORK_TABLE( CTFProjectile_SpellKartMirv, DT_TFProjectile_SpellKartMirv )
//END_NETWORK_TABLE()
//
//LINK_ENTITY_TO_CLASS( tf_projectile_spellkartmirv, CTFProjectile_SpellKartMirv );
//PRECACHE_WEAPON_REGISTER( tf_projectile_spellkartmirv );
