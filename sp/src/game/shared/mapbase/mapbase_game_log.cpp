//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: A special system designed to record game information for map testing.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include "utlbuffer.h"
#ifdef CLIENT_DLL
#else
#include "ammodef.h"
#include "ai_basenpc.h"
#include "ai_squad.h"
#include "fmtstr.h"
#include "GameEventListener.h"
#include "saverestore_utlvector.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

class CMapbaseGameLogger : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CMapbaseGameLogger, CLogicalEntity );

	CMapbaseGameLogger()
	{
		pGameLoggerEnt = this;
	}

	void Activate()
	{
		BaseClass::Activate();

		ListenForGameEvent("skill_changed");
	}

	void FireGameEvent( IGameEvent *event )
	{
		if (FStrEq(event->GetName(), "skill_changed"))
		{
			m_ListSkillChanged.AddToTail(event->GetInt("skill_level"));
			m_ListSkillChangedTime.AddToTail(gpGlobals->curtime);
		}
	}

	float m_flLastLogTime;
	int m_iSaveID;

	CUtlVector<int> m_ListSkillChanged;
	CUtlVector<float> m_ListSkillChangedTime;

	static CMapbaseGameLogger *GetGameLoggerEnt()
	{
		if (!pGameLoggerEnt)
			pGameLoggerEnt = static_cast<CMapbaseGameLogger*>(CBaseEntity::Create("mapbase_game_logger", vec3_origin, vec3_angle));

		return pGameLoggerEnt;
	}

private:
	static CHandle<CMapbaseGameLogger> pGameLoggerEnt;
};

LINK_ENTITY_TO_CLASS( mapbase_game_logger, CMapbaseGameLogger );

BEGIN_DATADESC( CMapbaseGameLogger )

	DEFINE_FIELD( m_flLastLogTime, FIELD_TIME ),
	DEFINE_FIELD( m_iSaveID, FIELD_INTEGER ),

	DEFINE_UTLVECTOR( m_ListSkillChanged, FIELD_INTEGER ),
	DEFINE_UTLVECTOR( m_ListSkillChangedTime, FIELD_TIME ),

END_DATADESC()

CHandle<CMapbaseGameLogger> CMapbaseGameLogger::pGameLoggerEnt;

void MapbaseGameLog_CVarToggle( IConVar *var, const char *pOldString, float flOldValue );
ConVar mapbase_game_log_on_autosave( "mapbase_game_log_on_autosave", "0", FCVAR_NONE, "Logs information to %mapname%_log_%number%.txt on each autosave", MapbaseGameLog_CVarToggle );

void MapbaseGameLog_Init()
{
	if (mapbase_game_log_on_autosave.GetBool())
	{
		// Create the game logger ent
		CMapbaseGameLogger::GetGameLoggerEnt();
	}
}

void MapbaseGameLog_Record( const char *szContext )
{
	CMapbaseGameLogger *pGameLoggerEnt = CMapbaseGameLogger::GetGameLoggerEnt();
	if (!pGameLoggerEnt)
	{
		Warning("Failed to get game logger ent\n");
		return;
	}

	KeyValues *pKV = new KeyValues( "Log" );

	KeyValues *pKVLogInfo = pKV->FindKey( "logging_info", true );
	if ( pKVLogInfo )
	{
		pKVLogInfo->SetString("context", szContext);
		pKVLogInfo->SetFloat("last_log", pGameLoggerEnt->m_flLastLogTime > 0.0f ? gpGlobals->curtime - pGameLoggerEnt->m_flLastLogTime : -1.0f);
	}

	KeyValues *pKVGameInfo = pKV->FindKey( "game_info", true );
	if ( pKVGameInfo )
	{
		pKVGameInfo->SetInt("skill", g_pGameRules->GetSkillLevel());

		if (pGameLoggerEnt->m_ListSkillChanged.Count() > 0)
		{
			KeyValues *pKVSkill = pKVGameInfo->FindKey("skill_changes", true);
			for (int i = 0; i < pGameLoggerEnt->m_ListSkillChanged.Count(); i++)
			{
				float flTime = pGameLoggerEnt->m_ListSkillChangedTime[i];
				switch (pGameLoggerEnt->m_ListSkillChanged[i])
				{
					case SKILL_EASY:	pKVSkill->SetString(CNumStr(flTime), "easy"); break;
					case SKILL_MEDIUM:	pKVSkill->SetString(CNumStr(flTime), "normal"); break;
					case SKILL_HARD:	pKVSkill->SetString(CNumStr(flTime), "hard"); break;
				}
			}
		}
	}

	KeyValues *pKVPlayer = pKV->FindKey( "player", true );
	if ( pKVPlayer )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		if ( pPlayer )
		{
			pKVPlayer->SetInt("health", pPlayer->GetHealth());
			pKVPlayer->SetInt("armor", pPlayer->ArmorValue());

			pKVPlayer->SetString("position", CFmtStrN<128>("[%f %f %f]", pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, pPlayer->GetAbsOrigin().z));
			pKVPlayer->SetString("angles", CFmtStrN<128>("[%f %f %f]", pPlayer->EyeAngles().x, pPlayer->EyeAngles().y, pPlayer->EyeAngles().z));

			KeyValues *pKVWeapons = pKVPlayer->FindKey( "weapons", true );
			if ( pKVWeapons )
			{
				// Cycle through all of the player's weapons
				for ( int i = 0; i < pPlayer->WeaponCount(); i++ )
				{
					CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
					if ( !pWeapon )
						continue;

					if ( pPlayer->GetActiveWeapon() == pWeapon )
						pKVWeapons->SetString(pWeapon->GetClassname(), CFmtStrN<32>("%i; %i (active)", pWeapon->m_iClip1.Get(), pWeapon->m_iClip2.Get()));
					else
						pKVWeapons->SetString(pWeapon->GetClassname(), CFmtStrN<32>("%i; %i", pWeapon->m_iClip1.Get(), pWeapon->m_iClip2.Get()));
				}
			}

			KeyValues *pKVAmmo = pKVPlayer->FindKey( "ammo", true );
			if ( pKVAmmo )
			{
				// Cycle through all of the player's ammo
				for ( int i = 0; i < GetAmmoDef()->m_nAmmoIndex; i++ )
				{
					int iAmmo = pPlayer->GetAmmoCount( i );
					if ( iAmmo > 0 )
						pKVAmmo->SetInt( GetAmmoDef()->m_AmmoType[i].pName, iAmmo );
				}
			}
		}
	}

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for (int i = 0; i < nAIs; i++)
	{
		CAI_BaseNPC *pNPC = ppAIs[i];

		if (!pNPC->IsAlive() || pNPC->GetSleepState() != AISS_AWAKE)
			continue;

		KeyValues *pKVNPC = pKV->FindKey( CNumStr( pNPC->entindex() ), true );
		if (pKVNPC)
		{
			pKVNPC->SetString("classname", pNPC->GetClassname());
			pKVNPC->SetString("name", STRING(pNPC->GetEntityName()));

			pKVNPC->SetString("position", CFmtStrN<128>("[%f %f %f]", pNPC->GetAbsOrigin().x, pNPC->GetAbsOrigin().y, pNPC->GetAbsOrigin().z));

			pKVNPC->SetInt("health", pNPC->GetHealth());

			if (pNPC->GetActiveWeapon())
				pKVNPC->SetString("weapon", pNPC->GetActiveWeapon()->GetClassname());

			if (pNPC->GetSquad())
				pKVNPC->SetString("squad", pNPC->GetSquad()->GetName());
		}
	}

	CFmtStrN<MAX_PATH> pathfmt("map_logs/%s_log_%i.txt", STRING(gpGlobals->mapname), pGameLoggerEnt->m_iSaveID);

	pGameLoggerEnt->m_flLastLogTime = gpGlobals->curtime;
	pGameLoggerEnt->m_iSaveID++;

	// Create the folder first, since "map_logs" is not standard and is unlikely to exist
	g_pFullFileSystem->CreateDirHierarchy( "map_logs", "MOD" );

	if (pKV->SaveToFile( g_pFullFileSystem, pathfmt, "MOD" ))
	{
		Msg("Saved game log file to \"%s\"\n", pathfmt.Get());
	}

	pKV->deleteThis();
}

static void CC_Mapbase_GameLogRecord( const CCommand& args )
{
	MapbaseGameLog_Record( "command" );
}

static ConCommand mapbase_game_log_record("mapbase_game_log_record", CC_Mapbase_GameLogRecord, "Records game data to %mapname%_log_%number%." );

void MapbaseGameLog_CVarToggle( IConVar *var, const char *pOldString, float flOldValue )
{
	if (mapbase_game_log_on_autosave.GetBool())
	{
		// Create the game logger ent
		CMapbaseGameLogger::GetGameLoggerEnt();
	}
}
#endif
