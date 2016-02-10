//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Laser Rifle & Shield combo
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "weapon_csbase.h"
#include "ammodef.h"
#include "mom_player_shared.h"

#if defined( CLIENT_DLL )

#include "vgui/ISurface.h"
//#include "vgui_controls/controls.h"
#include "hud_crosshair.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"

extern IVModelInfoClient* modelinfo;

#else

#include "te_effect_dispatch.h"
#include "KeyValues.h"
#include "cs_ammodef.h"

extern IVModelInfo* modelinfo;

#endif


// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

//--------------------------------------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] =
{
    "none",		// WEAPON_NONE
    "p228",		// WEAPON_P228
    "glock",	// WEAPON_GLOCK				// old glock
    "scout",	// WEAPON_SCOUT
    "hegren",	// WEAPON_HEGRENADE
    "xm1014",	// WEAPON_XM1014			// auto shotgun
    "c4",		// WEAPON_C4
    "mac10",	// WEAPON_MAC10				// T only
    "aug",		// WEAPON_AUG
    "sgren",	// WEAPON_SMOKEGRENADE
    "elite",	// WEAPON_ELITE
    "fiveseven",// WEAPON_FIVESEVEN
    "ump45",	// WEAPON_UMP45
    "sg550",	// WEAPON_SG550				// auto-sniper
    "galil",	// WEAPON_GALIL
    "famas",	// WEAPON_FAMAS				// CT cheap m4a1
    "usp",		// WEAPON_USP
    "awp",		// WEAPON_AWP
    "mp5navy",	// WEAPON_MP5N 
    "m249",		// WEAPON_M249				// big machinegun
    "m3",		// WEAPON_M3 				// cheap shotgun
    "m4a1",		// WEAPON_M4A1
    "tmp",		// WEAPON_TMP
    "g3sg1",	// WEAPON_G3SG1				// T auto-sniper
    "flash",	// WEAPON_FLASHBANG
    "deagle",	// WEAPON_DEAGLE
    "sg552",	// WEAPON_SG552				// T aug equivalent
    "ak47",		// WEAPON_AK47
    "knife",	// WEAPON_KNIFE
    "p90",		// WEAPON_P90
    "shield",	// WEAPON_SHIELDGUN 
    "kevlar",
    "assaultsuit",
    "nightvision",
    NULL,		// WEAPON_NONE
};

struct WeaponAliasTranslationInfoStruct
{
    char *m_alias;
    char *m_translatedAlias;
};

static const WeaponAliasTranslationInfoStruct s_WeaponAliasTranslationInfo[] =
{
    { "cv47", "ak47" },
    { "defender", "galil" },
    { "krieg552", "sg552" },
    { "magnum", "awp" },
    { "d3au1", "g3sg1" },
    { "clarion", "famas" },
    { "bullpup", "aug" },
    { "krieg550", "sg550" },
    { "9x19mm", "glock" },
    { "km45", "usp" },
    { "228compact", "p228" },
    { "nighthawk", "deagle" },
    { "elites", "elite" },
    { "fn57", "fiveseven" },
    { "12gauge", "m3" },
    { "autoshotgun", "xm1014" },
    { "mp", "tmp" },
    { "smg", "mp5navy" },
    { "mp5", "mp5navy" },
    { "c90", "p90" },
    { "vest", "kevlar" },
    { "vesthelm", "assaultsuit" },
    { "smokegrenade", "sgren" },
    { "smokegrenade", "sgren" },
    { "nvgs", "nightvision" },

    { "", "" } // this needs to be last
};

bool IsAmmoType(int iAmmoType, const char *pAmmoName)
{
    return GetAmmoDef()->Index(pAmmoName) == iAmmoType;
}

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the translated alias.
//
const char * GetTranslatedWeaponAlias(const char *alias)
{
    int i = 0;
    const WeaponAliasTranslationInfoStruct *info = &(s_WeaponAliasTranslationInfo[i]);

    while (info->m_alias[0] != 0)
    {
        if (Q_stricmp(alias, info->m_alias) == 0)
        {
            return info->m_translatedAlias;
        }
        info = &(s_WeaponAliasTranslationInfo[++i]);
    }

    return alias;
}

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the associated weapon ID
//
int AliasToWeaponID(const char *alias)
{
    if (alias)
    {
        for (int i = 0; s_WeaponAliasInfo[i] != NULL; ++i)
            if (!Q_stricmp(s_WeaponAliasInfo[i], alias))
                return i;
    }

    return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a classname, return the associated weapon ID
//
int ClassnameToWeaponID(const char *classname)
{
    if (classname)
    {
        for (int i = 0; s_WeaponAliasInfo[i] != NULL; ++i)
            if (Q_stristr(classname, s_WeaponAliasInfo[i]))
                return i;
    }

    return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a weapon ID, return its alias
//
const char *WeaponIDToAlias(int id)
{
    if ((id >= WEAPON_MAX) || (id < 0))
        return NULL;

    return s_WeaponAliasInfo[id];
}

//--------------------------------------------------------------------------------------------------------
//
// Return true if given weapon ID is a primary weapon
//
bool IsPrimaryWeapon(int id)
{
    switch (id)
    {
    case WEAPON_SCOUT:
    case WEAPON_XM1014:
    case WEAPON_MAC10:
    case WEAPON_AUG:
    case WEAPON_UMP45:
    case WEAPON_SG550:
    case WEAPON_GALIL:
    case WEAPON_FAMAS:
    case WEAPON_AWP:
    case WEAPON_MP5NAVY:
    case WEAPON_M249:
    case WEAPON_M3:
    case WEAPON_M4A1:
    case WEAPON_TMP:
    case WEAPON_G3SG1:
    case WEAPON_SG552:
    case WEAPON_AK47:
    case WEAPON_P90:
    case WEAPON_SHIELDGUN:
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------------
//
// Return true if given weapon ID is a secondary weapon
//
bool IsSecondaryWeapon(int id)
{
    switch (id)
    {
    case WEAPON_USP:
    case WEAPON_GLOCK:
    case WEAPON_DEAGLE:
    case WEAPON_ELITE:
    case WEAPON_P228:
    case WEAPON_FIVESEVEN:
        return true;
    }

    return false;
}

#ifdef CLIENT_DLL
int GetShellForAmmoType(const char *ammoname)
{
    if (!Q_strcmp(BULLET_PLAYER_762MM, ammoname))
        return CS_SHELL_762NATO;

    if (!Q_strcmp(BULLET_PLAYER_556MM, ammoname))
        return CS_SHELL_556;

    if (!Q_strcmp(BULLET_PLAYER_338MAG, ammoname))
        return CS_SHELL_338MAG;

    if (!Q_strcmp(BULLET_PLAYER_BUCKSHOT, ammoname))
        return CS_SHELL_12GAUGE;

    if (!Q_strcmp(BULLET_PLAYER_57MM, ammoname))
        return CS_SHELL_57;

    // default 9 mm
    return CS_SHELL_9MM;
}
#endif


// ----------------------------------------------------------------------------- //
// CWeaponCSBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponCSBase, DT_WeaponCSBase)

BEGIN_NETWORK_TABLE(CWeaponCSBase, DT_WeaponCSBase)
#ifdef CLIENT_DLL

#else
// world weapon models have no aminations
SendPropExclude("DT_AnimTimeMustBeFirst", "m_flAnimTime"),
SendPropExclude("DT_BaseAnimating", "m_nSequence"),
//	SendPropExclude( "DT_LocalActiveWeaponData", "m_flTimeWeaponIdle" ),
#endif


END_NETWORK_TABLE()

#if defined CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponCSBase)
DEFINE_PRED_FIELD(m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_cs_base, CWeaponCSBase);


#ifdef GAME_DLL

BEGIN_DATADESC(CWeaponCSBase)

//DEFINE_FUNCTION( DefaultTouch ),
DEFINE_FUNCTION(FallThink)

END_DATADESC()

#endif

#ifdef CLIENT_DLL
ConVar cl_crosshaircolor("cl_crosshaircolor", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_dynamiccrosshair("cl_dynamiccrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_scalecrosshair("cl_scalecrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_crosshairscale("cl_crosshairscale", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_crosshairalpha("cl_crosshairalpha", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_crosshairusealpha("cl_crosshairusealpha", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
#endif


// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponCSBase::CWeaponCSBase()
{
    SetPredictionEligible(true);
    m_bDelayFire = true;
    m_nextPrevOwnerTouchTime = 0.0;
    m_prevOwner = NULL;
    AddSolidFlags(FSOLID_TRIGGER); // Nothing collides with these but it gets touches.

#ifdef CLIENT_DLL
    m_iCrosshairTextureID = 0;

    m_bInReloadAnimation = false;
#else
    m_iDefaultExtraAmmo = 0;
#endif
}


#ifndef CLIENT_DLL
bool CWeaponCSBase::KeyValue(const char *szKeyName, const char *szValue)
{
    if (!BaseClass::KeyValue(szKeyName, szValue))
    {
        if (FStrEq(szKeyName, "ammo"))
        {
            int bullets = atoi(szValue);
            if (bullets < 0)
                return false;

            m_iDefaultExtraAmmo = bullets;

            return true;
        }
    }

    return false;
}
#endif


bool CWeaponCSBase::IsPredicted() const
{
    return true;
}


bool CWeaponCSBase::IsPistol() const
{
    return GetCSWpnData().m_WeaponType == WEAPONTYPE_PISTOL;
}


bool CWeaponCSBase::IsAwp() const
{
    return false;
}


bool CWeaponCSBase::PlayEmptySound()
{
    //MIKETODO: certain weapons should override this to make it empty:
    //	C4
    //	Flashbang
    //	HE Grenade
    //	Smoke grenade				

    CPASAttenuationFilter filter(this);
    filter.UsePredictionRules();

    if (IsPistol())
    {
        EmitSound(filter, entindex(), "Default.ClipEmpty_Pistol");
    }
    else
    {
        EmitSound(filter, entindex(), "Default.ClipEmpty_Rifle");
    }

    return 0;
}

CMomentumPlayer* CWeaponCSBase::GetPlayerOwner() const
{
    return dynamic_cast<CMomentumPlayer*>(GetOwner());
}

bool CWeaponCSBase::SendWeaponAnim(int iActivity)
{
#ifdef CS_SHIELD_ENABLED
    CCSPlayer *pPlayer = GetPlayerOwner();

    if ( pPlayer && pPlayer->HasShield() )
    {
        CBaseViewModel *vm = pPlayer->GetViewModel( 1 );

        if ( vm == NULL )
            return false;

        vm->SetWeaponModel( SHIELD_VIEW_MODEL, this );

        int	idealSequence = vm->SelectWeightedSequence( (Activity)iActivity );

        if ( idealSequence >= 0 )
        {
            vm->SendViewModelMatchingSequence( idealSequence );
        }
    } 
#endif

    return BaseClass::SendWeaponAnim(iActivity);
}


void CWeaponCSBase::ItemPostFrame()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (!pPlayer)
        return;

    UpdateShieldState();

    if ((m_bInReload) && (pPlayer->m_flNextAttack <= gpGlobals->curtime))
    {
        // complete the reload. 
        int j = min(GetMaxClip1() - m_iClip1, pPlayer->GetAmmoCount(m_iPrimaryAmmoType));

        // Add them to the clip
        m_iClip1 += j;
        pPlayer->RemoveAmmo(j, m_iPrimaryAmmoType);

        m_bInReload = false;
    }

    if ((pPlayer->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
    {
        if (m_iClip2 != -1 && !pPlayer->GetAmmoCount(GetSecondaryAmmoType()))
        {
            m_bFireOnEmpty = TRUE;
        }

        SecondaryAttack();

        pPlayer->m_nButtons &= ~IN_ATTACK2;
    }
    else if ((pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
    {
        if ((m_iClip1 == 0/* && pszAmmo1()*/) || (GetMaxClip1() == -1 && !pPlayer->GetAmmoCount(GetPrimaryAmmoType())))
        {
            m_bFireOnEmpty = TRUE;
        }

        // Can't shoot during the freeze period
        // Ken: Always allow firing in single player
        //---
        /*if ( !CSGameRules()->IsFreezePeriod() &&
            !pPlayer->m_bIsDefusing &&
            pPlayer->State_Get() == STATE_ACTIVE && !pPlayer->IsShieldDrawn()
            )*/
        {
#ifndef CLIENT_DLL
            // allow the bots to react to the gunfire
            if (GetCSWpnData().m_WeaponType != WEAPONTYPE_GRENADE)
            {
                IGameEvent * event = gameeventmanager->CreateEvent((HasAmmo()) ? "weapon_fire" : "weapon_fire_on_empty");
                if (event)
                {
                    const char *weaponName = STRING(m_iClassname);
                    if (strncmp(weaponName, "weapon_", 7) == 0)
                    {
                        weaponName += 7;
                    }

                    event->SetInt("userid", pPlayer->GetUserID());
                    event->SetString("weapon", weaponName);
                    gameeventmanager->FireEvent(event);
                }
            }
#endif
            PrimaryAttack();
        }
        //---
    }
    else if (pPlayer->m_nButtons & IN_RELOAD && GetMaxClip1() != WEAPON_NOCLIP && !m_bInReload && m_flNextPrimaryAttack < gpGlobals->curtime)
    {
        // reload when reload is pressed, or if no buttons are down and weapon is empty.

        //MIKETODO: add code for shields...
        //if ( !FBitSet( m_iWeaponState, WPNSTATE_SHIELD_DRAWN ) )

        //if ( !pPlayer->IsShieldDrawn() )
        {
#ifndef CLIENT_DLL
            // allow the bots to react to the reload
            //IGameEvent * event = gameeventmanager->CreateEvent( "weapon_reload" );
            //if( event )
            //{
            //	event->SetInt( "userid", pPlayer->GetUserID() );
            //	gameeventmanager->FireEvent( event );
            //}
#endif

            Reload();
        }
    }
    else if (!(pPlayer->m_nButtons & (IN_ATTACK | IN_ATTACK2)))
    {
        // no fire buttons down

        // The following code prevents the player from tapping the firebutton repeatedly 
        // to simulate full auto and retaining the single shot accuracy of single fire
        if (m_bDelayFire)
        {
            m_bDelayFire = false;

            if (pPlayer->m_iShotsFired > 15)
                pPlayer->m_iShotsFired = 15;

            m_flDecreaseShotsFired = gpGlobals->curtime + 0.4;
        }

        m_bFireOnEmpty = FALSE;

        // if it's a pistol then set the shots fired to 0 after the player releases a button
        if (IsPistol())
        {
            pPlayer->m_iShotsFired = 0;
        }
        else
        {
            if ((pPlayer->m_iShotsFired > 0) && (m_flDecreaseShotsFired < gpGlobals->curtime))
            {
                m_flDecreaseShotsFired = gpGlobals->curtime + 0.0225;
                pPlayer->m_iShotsFired--;
            }
        }

        if ((!IsUseable() && m_flNextPrimaryAttack < gpGlobals->curtime)
#ifdef CLIENT_DLL
            || (m_bInReloadAnimation)
#endif
            )
        {
            // Intentionally blank -- used to switch weapons here
        }
        else
        {
            // weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
            if (m_iClip1 == 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime)
            {
                Reload();
                return;
            }
        }

        WeaponIdle();
        return;
    }
}


float CWeaponCSBase::GetMaxSpeed() const
{
    // The weapon should have set this in its constructor.
    float flRet = GetCSWpnData().m_flMaxSpeed;
    Assert(flRet > 1);
    return flRet;
}


const CCSWeaponInfo &CWeaponCSBase::GetCSWpnData() const
{
    const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
    const CCSWeaponInfo *pCSInfo;

#ifdef _DEBUG
    pCSInfo = dynamic_cast< const CCSWeaponInfo* >( pWeaponInfo );
    Assert( pCSInfo );
#else
    pCSInfo = static_cast<const CCSWeaponInfo*>(pWeaponInfo);
#endif

    return *pCSInfo;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponCSBase::GetViewModel(int /*viewmodelindex = 0 -- this is ignored in the base class here*/) const
{
    CMomentumPlayer *pOwner = GetPlayerOwner();

    if (pOwner == NULL)
    {
        return BaseClass::GetViewModel();
    }

    return GetWpnData().szViewModel;

    //return BaseClass::GetViewModel();
}

void CWeaponCSBase::Precache(void)
{
    BaseClass::Precache();

#ifdef CS_SHIELD_ENABLED
    if ( GetCSWpnData().m_bCanUseWithShield )
    {
        PrecacheModel( GetCSWpnData().m_szShieldViewModel );
    }
#endif

    PrecacheScriptSound("Default.ClipEmpty_Pistol");
    PrecacheScriptSound("Default.ClipEmpty_Rifle");

    PrecacheScriptSound("Default.Zoom");
}

Activity CWeaponCSBase::GetDeployActivity(void)
{
    return ACT_VM_DRAW;
}

bool CWeaponCSBase::DefaultDeploy(char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt)
{
    // Msg( "deploy %s at %f\n", GetClassname(), gpGlobals->curtime );
    CMomentumPlayer *pOwner = GetPlayerOwner();
    if (!pOwner)
    {
        return false;
    }

    pOwner->SetAnimationExtension(szAnimExt);

    SetViewModel();
    SendWeaponAnim(GetDeployActivity());

    pOwner->SetNextAttack(gpGlobals->curtime + SequenceDuration());
    m_flNextPrimaryAttack = gpGlobals->curtime;
    m_flNextSecondaryAttack = gpGlobals->curtime;

    SetWeaponVisible(true);
    //pOwner->SetShieldDrawnState( false );

    //if ( pOwner->HasShield() == true )
    //	 SetWeaponModelIndex( SHIELD_WORLD_MODEL);
    //else
    SetWeaponModelIndex(szWeaponModel);

    return true;
}

void CWeaponCSBase::UpdateShieldState(void)
{
    //empty by default.
    CMomentumPlayer *pOwner = GetPlayerOwner();

    if (pOwner == NULL)
        return;

    //ADRIANTODO
    //Make the hitbox set switches here!!!
    //if ( pOwner->HasShield() == false )
    //{

    //	pOwner->SetShieldDrawnState( false );
    //pOwner->SetHitBoxSet( 0 );
    //	return;
    //}
    //else
    {
        //pOwner->SetHitBoxSet( 1 );
    }
}

void CWeaponCSBase::SetWeaponModelIndex(const char *pName)
{
    m_iWorldModelIndex = modelinfo->GetModelIndex(pName);
}

bool CWeaponCSBase::CanBeSelected(void)
{
    if (!VisibleInWeaponSelection())
        return false;

    return true;
}

bool CWeaponCSBase::CanDeploy(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    //if ( pPlayer->HasShield() && GetCSWpnData().m_bCanUseWithShield == false )
    //	 return false;

    return BaseClass::CanDeploy();
}

bool CWeaponCSBase::Holster(CBaseCombatWeapon *pSwitchingTo)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

#ifndef CLIENT_DLL
    if (pPlayer)
        pPlayer->SetFOV(pPlayer, 0); // reset the default FOV.
#endif

    //if ( pPlayer )
    //	pPlayer->SetShieldDrawnState( false );

    return BaseClass::Holster(pSwitchingTo);
}

bool CWeaponCSBase::Deploy()
{
#ifdef CLIENT_DLL
    m_iAlpha = 80;
#else

    m_flDecreaseShotsFired = gpGlobals->curtime;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer)
    {
        pPlayer->m_iShotsFired = 0;
        pPlayer->m_bResumeZoom = false;
        pPlayer->m_iLastZoom = 0;
        pPlayer->SetFOV(pPlayer, 0);
    }
#endif

    return BaseClass::Deploy();
}

#ifndef CLIENT_DLL
bool CWeaponCSBase::IsRemoveable()
{
    if (BaseClass::IsRemoveable() == true)
    {
        if (m_nextPrevOwnerTouchTime > gpGlobals->curtime)
        {
            return false;
        }
    }

    return BaseClass::IsRemoveable();
}
#endif

void CWeaponCSBase::Drop(const Vector &vecVelocity)
{

#ifdef CLIENT_DLL
    BaseClass::Drop(vecVelocity);
    return;
#else

    // Once somebody drops a gun, it's fair game for removal when/if
    // a game_weapon_manager does a cleanup on surplus weapons in the
    // world.
    SetRemoveable(true);

    StopAnimation();
    StopFollowingEntity();
    SetMoveType(MOVETYPE_FLYGRAVITY);
    // clear follow stuff, setup for collision
    SetGravity(1.0);
    m_iState = WEAPON_NOT_CARRIED;
    RemoveEffects(EF_NODRAW);
    FallInit();
    SetGroundEntity(NULL);

    m_bInReload = false; // stop reloading 

    SetThink(NULL);
    m_nextPrevOwnerTouchTime = gpGlobals->curtime + 0.8f;
    m_prevOwner = GetPlayerOwner();

    SetTouch(&CWeaponCSBase::DefaultTouch);

    IPhysicsObject *pObj = VPhysicsGetObject();
    if (pObj != NULL)
    {
        AngularImpulse	angImp(200, 200, 200);
        pObj->AddVelocity(&vecVelocity, &angImp);
    }
    else
    {
        SetAbsVelocity(vecVelocity);
    }

    SetNextThink(gpGlobals->curtime);

    SetOwnerEntity(NULL);
    SetOwner(NULL);
#endif
}

// whats going on here is that if the player drops this weapon, they shouldn't take it back themselves
// for a little while.  But if they throw it at someone else, the other player should get it immediately.
void CWeaponCSBase::DefaultTouch(CBaseEntity *pOther)
{
    if ((m_prevOwner != NULL) && (pOther == m_prevOwner) && (gpGlobals->curtime < m_nextPrevOwnerTouchTime))
    {
        return;
    }

    BaseClass::DefaultTouch(pOther);
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Draw the weapon's crosshair
//-----------------------------------------------------------------------------
void CWeaponCSBase::DrawCrosshair()
{
    if (!crosshair.GetInt())
        return;

    CHudCrosshair *pCrosshair = GET_HUDELEMENT(CHudCrosshair);

    if (!pCrosshair)
        return;

    // clear crosshair
    pCrosshair->SetCrosshair(0, Color(255, 255, 255, 255));

    CMomentumPlayer* pPlayer = (CMomentumPlayer*) C_BasePlayer::GetLocalPlayer();

    if (!pPlayer)
        return;

    // localplayer must be owner if not in Spec mode
    Assert((pPlayer == GetPlayerOwner()) || (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE));

    // Draw the targeting zone around the pCrosshair
    if (pPlayer->IsInVGuiInputMode())
        return;

    //if ( pPlayer->HasShield() && pPlayer->IsShieldDrawn() == true )
    //	 return;

    // no crosshair for sniper rifles
    if (GetCSWpnData().m_WeaponType == WEAPONTYPE_SNIPER_RIFLE)
        return;

    int iDistance = GetCSWpnData().m_iCrosshairMinDistance; // The minimum distance the crosshair can achieve...

    int iDeltaDistance = GetCSWpnData().m_iCrosshairDeltaDistance; // Distance at which the crosshair shrinks at each step

    if (cl_dynamiccrosshair.GetBool())
    {
        if (!(pPlayer->GetFlags() & FL_ONGROUND))
            iDistance *= 2.0f;
        else if (pPlayer->GetFlags() & FL_DUCKING)
            iDistance *= 0.5f;
        else if (pPlayer->GetAbsVelocity().Length() > 100)
            iDistance *= 1.5f;
    }

    if (pPlayer->m_iShotsFired > m_iAmmoLastCheck)
    {
        m_flCrosshairDistance = min(15, m_flCrosshairDistance + iDeltaDistance);
    }
    else if (m_flCrosshairDistance > iDistance)
    {
        m_flCrosshairDistance -= 0.1f + m_flCrosshairDistance * 0.013;
    }

    m_iAmmoLastCheck = pPlayer->m_iShotsFired;

    if (m_flCrosshairDistance < iDistance)
        m_flCrosshairDistance = iDistance;

    //scale bar size to the resolution
    int crosshairScale = cl_crosshairscale.GetInt();
    if (crosshairScale < 1)
    {
        if (ScreenHeight() <= 600)
        {
            crosshairScale = 600;
        }
        else if (ScreenHeight() <= 768)
        {
            crosshairScale = 768;
        }
        else
        {
            crosshairScale = 1200;
        }
    }

    float scale;
    if (cl_scalecrosshair.GetBool() == false)
    {
        scale = 1.0f;
    }
    else
    {
        scale = (float) ScreenHeight() / (float) crosshairScale;
    }

    int iCrosshairDistance = (int) ceil(m_flCrosshairDistance * scale);

    int iBarSize = XRES(5) + (iCrosshairDistance - iDistance) / 2;

    iBarSize = max(1, (int) ((float) iBarSize * scale));

    int iBarThickness = max(1, (int) floor(scale + 0.5f));

    int	r, g, b;

    switch (cl_crosshaircolor.GetInt())
    {
    case 0:	r = 50;		g = 250;	b = 50;		break;
    case 1:	r = 250;	g = 50;		b = 50;		break;
    case 2:	r = 50;		g = 50;		b = 250;	break;
    case 3:	r = 250;	g = 250;	b = 50;		break;
    case 4:	r = 50;		g = 250;	b = 250;	break;
    default:	r = 50;		g = 250;	b = 50;		break;
    }

    // if user is using nightvision, make the crosshair red.
    //if (pPlayer->m_bNightVisionOn)
    //{
    //	r = 250;
    //	g = 50;
    //	b = 50;
    //}

    int alpha = clamp(cl_crosshairalpha.GetInt(), 0, 255);
    vgui::surface()->DrawSetColor(r, g, b, alpha);

    if (!m_iCrosshairTextureID)
    {
        CHudTexture *pTexture = gHUD.GetIcon("whiteAdditive");
        if (pTexture)
        {
            m_iCrosshairTextureID = pTexture->textureId;
        }
    }

    if (!cl_crosshairusealpha.GetBool())
    {
        vgui::surface()->DrawSetColor(r, g, b, 200);
        vgui::surface()->DrawSetTexture(m_iCrosshairTextureID);
    }

    int iHalfScreenWidth = ScreenWidth() / 2;
    int iHalfScreenHeight = ScreenHeight() / 2;

    int iLeft = iHalfScreenWidth - (iCrosshairDistance + iBarSize);
    int iRight = iHalfScreenWidth + iCrosshairDistance + iBarThickness;
    int iFarLeft = iLeft + iBarSize;
    int iFarRight = iRight + iBarSize;

    if (!cl_crosshairusealpha.GetBool())
    {
        // Additive crosshair
        vgui::surface()->DrawTexturedRect(iLeft, iHalfScreenHeight, iFarLeft, iHalfScreenHeight + iBarThickness);
        vgui::surface()->DrawTexturedRect(iRight, iHalfScreenHeight, iFarRight, iHalfScreenHeight + iBarThickness);
    }
    else
    {
        // Alpha-blended crosshair
        vgui::surface()->DrawFilledRect(iLeft, iHalfScreenHeight, iFarLeft, iHalfScreenHeight + iBarThickness);
        vgui::surface()->DrawFilledRect(iRight, iHalfScreenHeight, iFarRight, iHalfScreenHeight + iBarThickness);
    }

    int iTop = iHalfScreenHeight - (iCrosshairDistance + iBarSize);
    int iBottom = iHalfScreenHeight + iCrosshairDistance + iBarThickness;
    int iFarTop = iTop + iBarSize;
    int iFarBottom = iBottom + iBarSize;

    if (!cl_crosshairusealpha.GetBool())
    {
        // Additive crosshair
        vgui::surface()->DrawTexturedRect(iHalfScreenWidth, iTop, iHalfScreenWidth + iBarThickness, iFarTop);
        vgui::surface()->DrawTexturedRect(iHalfScreenWidth, iBottom, iHalfScreenWidth + iBarThickness, iFarBottom);
    }
    else
    {
        // Alpha-blended crosshair
        vgui::surface()->DrawFilledRect(iHalfScreenWidth, iTop, iHalfScreenWidth + iBarThickness, iFarTop);
        vgui::surface()->DrawFilledRect(iHalfScreenWidth, iBottom, iHalfScreenWidth + iBarThickness, iFarBottom);
    }
}


void CWeaponCSBase::OnDataChanged(DataUpdateType_t type)
{
    BaseClass::OnDataChanged(type);

    if (GetPredictable() && !ShouldPredict())
        ShutdownPredictable();
}


bool CWeaponCSBase::ShouldPredict()
{
    if (GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer())
        return true;

    return BaseClass::ShouldPredict();
}

void CWeaponCSBase::ProcessMuzzleFlashEvent()
{
    // This is handled from the player's animstate, so it can match up to the beginning of the fire animation
}

bool CWeaponCSBase::OnFireEvent(C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options)
{
    if (event == 5001)
    {
        CMomentumPlayer *pPlayer = GetPlayerOwner();
        if (pPlayer && pPlayer->GetFOV() < pPlayer->GetDefaultFOV() && HideViewModelWhenZoomed())
            return true;

        CEffectData data;
        data.m_fFlags = 0;
        data.m_hEntity = pViewModel->GetRefEHandle();
        data.m_nAttachmentIndex = 1;
        data.m_flScale = GetCSWpnData().m_flMuzzleScale;

        switch (GetMuzzleFlashStyle())
        {
        case CS_MUZZLEFLASH_NONE:
            break;

        case CS_MUZZLEFLASH_X:
            DispatchEffect("CS_MuzzleFlash_X", data);
            break;

        case CS_MUZZLEFLASH_NORM:
        default:
            DispatchEffect("CS_MuzzleFlash", data);
            break;
        }

        return true;
    }

    return BaseClass::OnFireEvent(pViewModel, origin, angles, event, options);
}

int CWeaponCSBase::GetMuzzleFlashStyle(void)
{
    return GetCSWpnData().m_iMuzzleFlashStyle;
}

int CWeaponCSBase::GetMuzzleAttachment(void)
{
    return LookupAttachment("muzzle_flash");
}

#else		

//-----------------------------------------------------------------------------
// Purpose: Get the accuracy derived from weapon and player, and return it
//-----------------------------------------------------------------------------
const Vector& CWeaponCSBase::GetBulletSpread()
{
    static Vector cone = VECTOR_CONE_8DEGREES;
    return cone;
}

//-----------------------------------------------------------------------------
// Purpose: Match the anim speed to the weapon speed while crouching
//-----------------------------------------------------------------------------
float CWeaponCSBase::GetDefaultAnimSpeed()
{
    return 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the laser rifle effect
//-----------------------------------------------------------------------------
void CWeaponCSBase::BulletWasFired(const Vector &vecStart, const Vector &vecEnd)
{
}


bool CWeaponCSBase::ShouldRemoveOnRoundRestart()
{
    if (GetPlayerOwner())
        return false;
    else
        return true;
}


//=========================================================
// Materialize - make a CWeaponCSBase visible and tangible
//=========================================================
void CWeaponCSBase::Materialize()
{
    if (IsEffectActive(EF_NODRAW))
    {
        // changing from invisible state to visible.
        RemoveEffects(EF_NODRAW);
        DoMuzzleFlash();
    }

    AddSolidFlags(FSOLID_TRIGGER);

    //SetTouch( &CWeaponCSBase::DefaultTouch );

    SetThink(NULL);

}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CWeaponCSBase::AttemptToMaterialize()
{
    float time = g_pGameRules->FlWeaponTryRespawn(this);

    if (time == 0)
    {
        Materialize();
        return;
    }

    SetNextThink(gpGlobals->curtime + time);
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CWeaponCSBase::CheckRespawn()
{
    //GOOSEMAN : Do not respawn weapons!
    return;
}


//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CWeaponCSBase::Respawn()
{
    // make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
    // will decide when to make the weapon visible and touchable.
    CBaseEntity *pNewWeapon = CBaseEntity::Create(GetClassname(), g_pGameRules->VecWeaponRespawnSpot(this), GetAbsAngles(), GetOwner());

    if (pNewWeapon)
    {
        pNewWeapon->AddEffects(EF_NODRAW);// invisible for now
        pNewWeapon->SetTouch(NULL);// no touch
        pNewWeapon->SetThink(&CWeaponCSBase::AttemptToMaterialize);

        UTIL_DropToFloor(this, MASK_SOLID);

        // not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
        // but when it should respawn is based on conditions belonging to the weapon that was taken.
        pNewWeapon->SetNextThink(gpGlobals->curtime + g_pGameRules->FlWeaponRespawnTime(this));
    }
    else
    {
        Msg("Respawn failed to create %s!\n", GetClassname());
    }

    return pNewWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCSBase::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
    CBasePlayer *pPlayer = ToBasePlayer(pActivator);

    if (pPlayer)
    {
        pPlayer->Weapon_Equip(this);
    }
}

bool CWeaponCSBase::Reload()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    pPlayer->m_iShotsFired = 0;

    bool retval = BaseClass::Reload();

#ifdef CLIENT_DLL
    if (retval)
    {
        m_bInReloadAnimation = true;
    }
#endif

    return retval;
}

void CWeaponCSBase::Spawn()
{
    BaseClass::Spawn();

    // Override the bloat that our base class sets as it's a little bit bigger than we want.
    // If it's too big, you drop a weapon and its box is so big that you're still touching it
    // when it falls and you pick it up again right away.
    CollisionProp()->UseTriggerBounds(true, 30);

    // Set this here to allow players to shoot dropped weapons
    SetCollisionGroup(COLLISION_GROUP_WEAPON);

    SetExtraAmmoCount(m_iDefaultExtraAmmo);	//Start with no additional ammo

    m_nextPrevOwnerTouchTime = 0.0;
    m_prevOwner = NULL;

#ifdef CLIENT_DLL
    m_bInReloadAnimation = false;
#endif
}

bool CWeaponCSBase::DefaultReload(int iClipSize1, int iClipSize2, int iActivity)
{
    if (BaseClass::DefaultReload(iClipSize1, iClipSize2, iActivity))
    {
        //SendReloadEvents(); MOM_TODO: will this be needed? (it sends it to other players)
        return true;
    }
    else
    {
        return false;
    }
}

void CWeaponCSBase::SendReloadEvents()
{
    //CMomentumPlayer *pPlayer = GetPlayerOwner();
    //if ( !pPlayer )
    //    return;

    // Make the player play his reload animation.
    //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

#endif


bool CWeaponCSBase::DefaultPistolReload()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0)
        return true;

    if (!DefaultReload(GetCSWpnData().iDefaultClip1, 0, ACT_VM_RELOAD))
        return false;

    pPlayer->m_iShotsFired = 0;

#ifdef CLIENT_DLL
    m_bInReloadAnimation = true;
#endif

    return true;
}

bool CWeaponCSBase::IsUseable()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (Clip1() <= 0)
    {
        if (pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0 && GetMaxClip1() != -1)
        {
            // clip is empty (or nonexistant) and the player has no more ammo of this type. 
            return false;
        }
    }

    return true;
}


#if defined( CLIENT_DLL )

float	g_lateralBob = 0;
float	g_verticalBob = 0;

static ConVar	cl_bobcycle("cl_bobcycle", "0.8", FCVAR_CHEAT);
static ConVar	cl_bob("cl_bob", "0.002", FCVAR_CHEAT);
static ConVar	cl_bobup("cl_bobup", "0.5", FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CWeaponCSBase::CalcViewmodelBob(void)
{
    static	float bobtime;
    static	float lastbobtime;
    static  float lastspeed;
    float	cycle;

    CBasePlayer *player = ToBasePlayer(GetOwner());
    //Assert( player );

    //NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

    if ((!gpGlobals->frametime) ||
        (player == NULL) ||
        (cl_bobcycle.GetFloat() <= 0.0f) ||
        (cl_bobup.GetFloat() <= 0.0f) ||
        (cl_bobup.GetFloat() >= 1.0f))
    {
        //NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
        return 0.0f;// just use old value
    }

    //Find the speed of the player
    float speed = player->GetLocalVelocity().Length2D();
    float flmaxSpeedDelta = max(0, (gpGlobals->curtime - lastbobtime) * 320.0f);

    // don't allow too big speed changes
    speed = clamp(speed, lastspeed - flmaxSpeedDelta, lastspeed + flmaxSpeedDelta);
    speed = clamp(speed, -320, 320);

    lastspeed = speed;

    //FIXME: This maximum speed value must come from the server.
    //		 MaxSpeed() is not sufficient for dealing with sprinting - jdw



    float bob_offset = RemapVal(speed, 0, 320, 0.0f, 1.0f);

    bobtime += (gpGlobals->curtime - lastbobtime) * bob_offset;
    lastbobtime = gpGlobals->curtime;

    //Calculate the vertical bob
    cycle = bobtime - (int) (bobtime / cl_bobcycle.GetFloat())*cl_bobcycle.GetFloat();
    cycle /= cl_bobcycle.GetFloat();

    if (cycle < cl_bobup.GetFloat())
    {
        cycle = M_PI * cycle / cl_bobup.GetFloat();
    }
    else
    {
        cycle = M_PI + M_PI*(cycle - cl_bobup.GetFloat()) / (1.0 - cl_bobup.GetFloat());
    }

    g_verticalBob = speed*0.005f;
    g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

    g_verticalBob = clamp(g_verticalBob, -7.0f, 4.0f);

    //Calculate the lateral bob
    cycle = bobtime - (int) (bobtime / cl_bobcycle.GetFloat() * 2)*cl_bobcycle.GetFloat() * 2;
    cycle /= cl_bobcycle.GetFloat() * 2;

    if (cycle < cl_bobup.GetFloat())
    {
        cycle = M_PI * cycle / cl_bobup.GetFloat();
    }
    else
    {
        cycle = M_PI + M_PI*(cycle - cl_bobup.GetFloat()) / (1.0 - cl_bobup.GetFloat());
    }

    g_lateralBob = speed*0.005f;
    g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
    g_lateralBob = clamp(g_lateralBob, -7.0f, 4.0f);

    //NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
    return 0.0f;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponCSBase::AddViewmodelBob(CBaseViewModel *viewmodel, Vector &origin, QAngle &angles)
{
    Vector	forward, right;
    AngleVectors(angles, &forward, &right, NULL);

    CalcViewmodelBob();

    // Apply bob, but scaled down to 40%
    VectorMA(origin, g_verticalBob * 0.4f, forward, origin);

    // Z bob a bit more
    origin[2] += g_verticalBob * 0.1f;

    // bob the angles
    angles[ROLL] += g_verticalBob * 0.5f;
    angles[PITCH] -= g_verticalBob * 0.4f;

    angles[YAW] -= g_lateralBob  * 0.3f;

    //	VectorMA( origin, g_lateralBob * 0.2f, right, origin );
}

#else

void CWeaponCSBase::AddViewmodelBob(CBaseViewModel *viewmodel, Vector &origin, QAngle &angles)
{

}

float CWeaponCSBase::CalcViewmodelBob(void)
{
    return 0.0f;
}

#endif

#ifndef CLIENT_DLL
bool CWeaponCSBase::PhysicsSplash(const Vector &centerPoint, const Vector &normal, float rawSpeed, float scaledSpeed)
{
    if (rawSpeed > 20)
    {

        float size = 4.0f;
        if (!IsPistol())
            size += 2.0f;

        // adjust splash size based on speed
        size += RemapValClamped(rawSpeed, 0, 400, 0, 3);

        CEffectData	data;
        data.m_vOrigin = centerPoint;
        data.m_vNormal = normal;
        data.m_flScale = random->RandomFloat(size, size + 1.0f);

        if (GetWaterType() & CONTENTS_SLIME)
        {
            data.m_fFlags |= FX_WATER_IN_SLIME;
        }

        DispatchEffect("gunshotsplash", data);

        return true;
    }

    return false;
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CWeaponCSBase::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
#if !defined( CLIENT_DLL )
    RemoveEffects(EF_ITEM_BLINK);

    if (pNewOwner->IsPlayer() && pNewOwner->IsAlive())
    {
        // Play the pickup sound for 1st-person observers
        CRecipientFilter filter;
        for (int i = 0; i < gpGlobals->maxClients; ++i)
        {
            CBasePlayer *player = UTIL_PlayerByIndex(i);
            if (player && !player->IsAlive() && player->GetObserverMode() == OBS_MODE_IN_EYE)
            {
                filter.AddRecipient(player);
            }
        }
        if (filter.GetRecipientCount())
        {
            CBaseEntity::EmitSound(filter, pNewOwner->entindex(), "Player.PickupWeapon");
        }

        // Robin: We don't want to delete weapons the player has picked up, so 
        // clear the name of the weapon. This prevents wildcards that are meant 
        // to find NPCs finding weapons dropped by the NPCs as well.
        SetName(NULL_STRING);
    }

    // Someone picked me up, so make it so that I can't be removed.
    SetRemoveable(false);
#endif
}
