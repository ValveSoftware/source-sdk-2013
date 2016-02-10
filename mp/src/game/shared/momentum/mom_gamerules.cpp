#include "cbase.h"
#include "mom_gamerules.h"
#include "cs_ammodef.h"
#include "weapon_csbase.h"
#include "voice_gamemgr.h"
#include "mom_shareddefs.h"

#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS(CMomentum);



// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

static CAmmoDef ammoDef;

CAmmoDef* GetAmmoDef()
{
    static bool bInitted = false;

    if (!bInitted)
    {
        bInitted = true;

        ammoDef.AddAmmoType(BULLET_PLAYER_50AE, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max", 2400 * BULLET_IMPULSE_EXAGGERATION, 0, 10, 14);
        ammoDef.AddAmmoType(BULLET_PLAYER_762MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_762mm_max", 2400 * BULLET_IMPULSE_EXAGGERATION, 0, 10, 14);
        ammoDef.AddAmmoType(BULLET_PLAYER_556MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_556mm_max", 2400 * BULLET_IMPULSE_EXAGGERATION, 0, 10, 14);
        ammoDef.AddAmmoType(BULLET_PLAYER_556MM_BOX, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_556mm_box_max", 2400 * BULLET_IMPULSE_EXAGGERATION, 0, 10, 14);
        ammoDef.AddAmmoType(BULLET_PLAYER_338MAG, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_338mag_max", 2800 * BULLET_IMPULSE_EXAGGERATION, 0, 12, 16);
        ammoDef.AddAmmoType(BULLET_PLAYER_9MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_9mm_max", 2000 * BULLET_IMPULSE_EXAGGERATION, 0, 5, 10);
        ammoDef.AddAmmoType(BULLET_PLAYER_BUCKSHOT, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_buckshot_max", 600 * BULLET_IMPULSE_EXAGGERATION, 0, 3, 6);
        ammoDef.AddAmmoType(BULLET_PLAYER_45ACP, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_45acp_max", 2100 * BULLET_IMPULSE_EXAGGERATION, 0, 6, 10);
        ammoDef.AddAmmoType(BULLET_PLAYER_357SIG, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_357sig_max", 2000 * BULLET_IMPULSE_EXAGGERATION, 0, 4, 8);
        ammoDef.AddAmmoType(BULLET_PLAYER_57MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_57mm_max", 2000 * BULLET_IMPULSE_EXAGGERATION, 0, 4, 8);
        ammoDef.AddAmmoType(AMMO_TYPE_HEGRENADE, DMG_BLAST, TRACER_LINE, 0, 0, 1/*max carry*/, 1, 0);
        ammoDef.AddAmmoType(AMMO_TYPE_FLASHBANG, 0, TRACER_LINE, 0, 0, 2/*max carry*/, 1, 0);
        ammoDef.AddAmmoType(AMMO_TYPE_SMOKEGRENADE, 0, TRACER_LINE, 0, 0, 1/*max carry*/, 1, 0);
    }

    return &ammoDef;
}

/*
ConVar ammo_50AE_max( "ammo_50AE_max", "35", FCVAR_REPLICATED );
ConVar ammo_762mm_max( "ammo_762mm_max", "90", FCVAR_REPLICATED );
ConVar ammo_556mm_max( "ammo_556mm_max", "90", FCVAR_REPLICATED );
ConVar ammo_556mm_box_max( "ammo_556mm_box_max", "200", FCVAR_REPLICATED );
ConVar ammo_338mag_max( "ammo_338mag_max", "30", FCVAR_REPLICATED );
ConVar ammo_9mm_max( "ammo_9mm_max", "120", FCVAR_REPLICATED );
ConVar ammo_buckshot_max( "ammo_buckshot_max", "32", FCVAR_REPLICATED );
ConVar ammo_45acp_max( "ammo_45acp_max", "100", FCVAR_REPLICATED );
ConVar ammo_357sig_max( "ammo_357sig_max", "52", FCVAR_REPLICATED );
ConVar ammo_57mm_max( "ammo_57mm_max", "100", FCVAR_REPLICATED );
ConVar ammo_hegrenade_max( "ammo_hegrenade_max", "1", FCVAR_REPLICATED );
ConVar ammo_flashbang_max( "ammo_flashbang_max", "2", FCVAR_REPLICATED );
ConVar ammo_smokegrenade_max( "ammo_smokegrenade_max", "1", FCVAR_REPLICATED );
*/

ConVar ammo_50AE_max("ammo_50AE_max", "-2", FCVAR_REPLICATED);
ConVar ammo_762mm_max("ammo_762mm_max", "-2", FCVAR_REPLICATED);
ConVar ammo_556mm_max("ammo_556mm_max", "-2", FCVAR_REPLICATED);
ConVar ammo_556mm_box_max("ammo_556mm_box_max", "-2", FCVAR_REPLICATED);
ConVar ammo_338mag_max("ammo_338mag_max", "-2", FCVAR_REPLICATED);
ConVar ammo_9mm_max("ammo_9mm_max", "-2", FCVAR_REPLICATED);
ConVar ammo_buckshot_max("ammo_buckshot_max", "-2", FCVAR_REPLICATED);
ConVar ammo_45acp_max("ammo_45acp_max", "-2", FCVAR_REPLICATED);
ConVar ammo_357sig_max("ammo_357sig_max", "-2", FCVAR_REPLICATED);
ConVar ammo_57mm_max("ammo_57mm_max", "-2", FCVAR_REPLICATED);
ConVar ammo_hegrenade_max("ammo_hegrenade_max", "1", FCVAR_REPLICATED);
ConVar ammo_flashbang_max("ammo_flashbang_max", "2", FCVAR_REPLICATED);
ConVar ammo_smokegrenade_max("ammo_smokegrenade_max", "1", FCVAR_REPLICATED);

CMomentum::CMomentum()
{
    //m_iGameMode = 0;
}

CMomentum::~CMomentum()
{
}

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(info_player_terrorist, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_counterterrorist, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_logo, CPointEntity);



Vector CMomentum::DropToGround(
    CBaseEntity *pMainEnt,
    const Vector &vPos,
    const Vector &vMins,
    const Vector &vMaxs)
{
    trace_t trace;
    UTIL_TraceHull(vPos, vPos + Vector(0, 0, -500), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace);
    return trace.endpos;
}

CBaseEntity *CMomentum::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
    // gat valid spwan point
    if (pPlayer)
    {
        CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
        if (pSpawnSpot)
        {
            // drop down to ground
            Vector GroundPos = DropToGround(pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX);

            // Move the player to the place it said.
            pPlayer->Teleport(&pSpawnSpot->GetAbsOrigin(), &pSpawnSpot->GetLocalAngles(), &vec3_origin);
            pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
            return pSpawnSpot;
        }
    }
    return NULL;
}

// checks if the spot is clear of players
bool CMomentum::IsSpawnPointValid(CBaseEntity *pSpot, CBasePlayer *pPlayer)
{
    if (!pSpot->IsTriggered(pPlayer))
    {
        return false;
    }

    Vector mins = GetViewVectors()->m_vHullMin;
    Vector maxs = GetViewVectors()->m_vHullMax;

    Vector vTestMins = pSpot->GetAbsOrigin() + mins;
    Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;

    // First test the starting origin.
    return UTIL_IsSpaceEmpty(pPlayer, vTestMins, vTestMaxs);
}

bool CMomentum::ClientCommand(CBaseEntity *pEdict, const CCommand &args)
{
    if (BaseClass::ClientCommand(pEdict, args))
        return true;

    CMomentumPlayer *pPlayer = (CMomentumPlayer *) pEdict;

    return pPlayer->ClientCommand(args);
}

static void OnGamemodeChanged(IConVar *var, const char* pOldValue, float fOldValue)
{
    int toCheck = ((ConVar*) var)->GetInt();
    if (toCheck == fOldValue) return;
    if (toCheck < 0)
    {
        // This will never happen. but better be safe than sorry, right?
        Warning("Cannot set a game mode under 0!\n");
        var->SetValue(((ConVar*) var)->GetDefault());
        return;
    }
    bool result = TickSet::SetTickrate(toCheck);
    if (result)
    {
        Msg("Successfully changed the tickrate to %f!\n", TickSet::GetTickrate());
        gpGlobals->interval_per_tick = TickSet::GetTickrate();
    }
    else Warning("Failed to change interval per tick, cannot set tick rate!\n");
}

static ConVar gamemode("mom_gamemode", "0", FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_HIDDEN, "", true, 0, false, 0,OnGamemodeChanged);

static ConVar allow_custom("mom_allow_custom_maps", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Allow loading custom maps that aren't of an official gametype.", true ,0, true ,1);

static ConVar give_weapon("mom_spawn_with_weapon", "1", FCVAR_NONE, "Spawn the player with a weapon?", true, 0, true, 1);

void CMomentum::PlayerSpawn(CBasePlayer* pPlayer)
{
    if (gamemode.GetInt() == 0 && !allow_custom.GetBool())
    {
        engine->ServerCommand("disconnect\n"); 
        Warning("\n\nBeware, beware!\nYou have been disconnected from the map because custom maps are not allowed if %s is 0.\nPlease set it to 1 in order to play custom maps.\n\n", allow_custom.GetName());
    }

    ConVarRef map("host_map");
    const char *pMapName = map.GetString();

    if (gpGlobals->eLoadType == MapLoad_Background || !Q_strcmp(pMapName, "credits.bsp"))
    {
        //Hide timer/speedometer on background maps
        pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
    }
    else
    {
        // Turn them back on
        pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
    }

    
    //MOM_TODO: could this change to gamemode != ALLOWED ?
    if (give_weapon.GetBool() && !Q_strcmp(pMapName, "credits.bsp") && !(Q_strnicmp(pMapName, "background", Q_strlen("background"))))
        pPlayer->Weapon_Create("weapon_momentum_gun");
    //MOM_TODO: keep track of holstering (convar?)
}


class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
    virtual bool		CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity)
    {
        return true;
    }
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;


//-----------------------------------------------------------------------------
// Purpose: MULTIPLAYER BODY QUE HANDLING
//-----------------------------------------------------------------------------
class CCorpse : public CBaseAnimating
{
public:
    DECLARE_CLASS(CCorpse, CBaseAnimating);
    DECLARE_SERVERCLASS();

    virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }

public:
    CNetworkVar(int, m_nReferencePlayer);
};

IMPLEMENT_SERVERCLASS_ST(CCorpse, DT_Corpse)
SendPropInt(SENDINFO(m_nReferencePlayer), 10, SPROP_UNSIGNED)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(bodyque, CCorpse);


CCorpse		*g_pBodyQueueHead;

void InitBodyQue(void)
{
    CCorpse *pEntity = (CCorpse *) CreateEntityByName("bodyque");
    pEntity->AddEFlags(EFL_KEEP_ON_RECREATE_ENTITIES);
    g_pBodyQueueHead = pEntity;
    CCorpse *p = g_pBodyQueueHead;

    // Reserve 3 more slots for dead bodies
    for (int i = 0; i < 3; i++)
    {
        CCorpse *next = (CCorpse *) CreateEntityByName("bodyque");
        next->AddEFlags(EFL_KEEP_ON_RECREATE_ENTITIES);
        p->SetOwnerEntity(next);
        p = next;
    }

    p->SetOwnerEntity(g_pBodyQueueHead);
}

//-----------------------------------------------------------------------------
// Purpose: make a body que entry for the given ent so the ent can be respawned elsewhere
// GLOBALS ASSUMED SET:  g_eoBodyQueueHead
//-----------------------------------------------------------------------------
void CopyToBodyQue(CBaseAnimating *pCorpse)
{
    if (pCorpse->IsEffectActive(EF_NODRAW))
        return;

    CCorpse *pHead = g_pBodyQueueHead;

    pHead->CopyAnimationDataFrom(pCorpse);

    pHead->SetMoveType(MOVETYPE_FLYGRAVITY);
    pHead->SetAbsVelocity(pCorpse->GetAbsVelocity());
    pHead->ClearFlags();
    pHead->m_nReferencePlayer = ENTINDEX(pCorpse);

    pHead->SetLocalAngles(pCorpse->GetAbsAngles());
    UTIL_SetOrigin(pHead, pCorpse->GetAbsOrigin());

    UTIL_SetSize(pHead, pCorpse->WorldAlignMins(), pCorpse->WorldAlignMaxs());
    g_pBodyQueueHead = (CCorpse *) pHead->GetOwnerEntity();
}

#endif