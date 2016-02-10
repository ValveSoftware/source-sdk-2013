//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "cs_weapon_parse.h"
//#include "cs_shareddefs.h"
#include "weapon_csbase.h"
#include "icvar.h"
//#include "cs_gamerules.h"
//#include "cs_blackmarket.h"


//--------------------------------------------------------------------------------------------------------
struct WeaponTypeInfo
{
    CSWeaponType type;
    const char * name;
};


//--------------------------------------------------------------------------------------------------------
WeaponTypeInfo s_weaponTypeInfo[] =
{
    { WEAPONTYPE_KNIFE, "Knife" },
    { WEAPONTYPE_PISTOL, "Pistol" },
    { WEAPONTYPE_SUBMACHINEGUN, "Submachine Gun" },	// First match is printable
    { WEAPONTYPE_SUBMACHINEGUN, "submachinegun" },
    { WEAPONTYPE_SUBMACHINEGUN, "smg" },
    { WEAPONTYPE_RIFLE, "Rifle" },
    { WEAPONTYPE_SHOTGUN, "Shotgun" },
    { WEAPONTYPE_SNIPER_RIFLE, "Sniper" },
    { WEAPONTYPE_MACHINEGUN, "Machine Gun" },		// First match is printable
    { WEAPONTYPE_MACHINEGUN, "machinegun" },
    { WEAPONTYPE_MACHINEGUN, "mg" },
    { WEAPONTYPE_C4, "C4" },
    { WEAPONTYPE_GRENADE, "Grenade" },
    { WEAPONTYPE_UNKNOWN, NULL },
};

//--------------------------------------------------------------------------------------------------------------
static const char *WeaponNames[WEAPON_MAX] =
{
    "weapon_none",

    "weapon_p228",
    "weapon_glock",
    "weapon_scout",
    "weapon_hegrenade",
    "weapon_xm1014",
    "weapon_c4",
    "weapon_mac10",
    "weapon_aug",
    "weapon_smokegrenade",
    "weapon_elite",
    "weapon_fiveseven",
    "weapon_ump45",
    "weapon_sg550",

    "weapon_galil",
    "weapon_famas",
    "weapon_usp",
    "weapon_awp",
    "weapon_mp5navy",
    "weapon_m249",
    "weapon_m3",
    "weapon_m4a1",
    "weapon_tmp",
    "weapon_g3sg1",
    "weapon_flashbang",
    "weapon_deagle",
    "weapon_sg552",
    "weapon_ak47",
    "weapon_knife",
    "weapon_p90",

    "weapon_shieldgun",
};


CCSWeaponInfo g_EquipmentInfo[MAX_EQUIPMENT];

void PrepareEquipmentInfo(void)
{
    /*memset( g_EquipmentInfo, 0, ARRAYSIZE( g_EquipmentInfo ) );

    g_EquipmentInfo[2].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_KEVLAR ) );
    g_EquipmentInfo[2].SetDefaultPrice( KEVLAR_PRICE );
    g_EquipmentInfo[2].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_KEVLAR ) );
    g_EquipmentInfo[2].m_iTeam = TEAM_UNASSIGNED;
    Q_strcpy( g_EquipmentInfo[2].szClassName, "weapon_vest" );

    #ifdef CLIENT_DLL
    g_EquipmentInfo[2].iconActive = new CHudTexture;
    g_EquipmentInfo[2].iconActive->cCharacterInFont = 't';
    #endif

    g_EquipmentInfo[1].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_ASSAULTSUIT ) );
    g_EquipmentInfo[1].SetDefaultPrice( ASSAULTSUIT_PRICE );
    g_EquipmentInfo[1].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_ASSAULTSUIT ) );
    g_EquipmentInfo[1].m_iTeam = TEAM_UNASSIGNED;
    Q_strcpy( g_EquipmentInfo[1].szClassName, "weapon_vesthelm" );

    #ifdef CLIENT_DLL
    g_EquipmentInfo[1].iconActive = new CHudTexture;
    g_EquipmentInfo[1].iconActive->cCharacterInFont = 'u';
    #endif

    g_EquipmentInfo[0].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_NVG ) );
    g_EquipmentInfo[0].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_NVG ) );
    g_EquipmentInfo[0].SetDefaultPrice( NVG_PRICE );
    g_EquipmentInfo[0].m_iTeam = TEAM_UNASSIGNED;
    Q_strcpy( g_EquipmentInfo[0].szClassName, "weapon_nvgs" );

    #ifdef CLIENT_DLL
    g_EquipmentInfo[0].iconActive = new CHudTexture;
    g_EquipmentInfo[0].iconActive->cCharacterInFont = 's';
    #endif
    */
}

//--------------------------------------------------------------------------------------------------------------
CCSWeaponInfo * GetWeaponInfo(CSWeaponID weaponID)
{
    if (weaponID == WEAPON_NONE)
        return NULL;

    if (weaponID >= WEAPON_KEVLAR)
    {
        int iIndex = (WEAPON_MAX - weaponID) - 1;

        return &g_EquipmentInfo[iIndex];

    }

    const char *weaponName = WeaponNames[weaponID];
    WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot(weaponName);
    if (hWpnInfo == GetInvalidWeaponInfoHandle())
    {
        return NULL;
    }

    CCSWeaponInfo *pWeaponInfo = dynamic_cast<CCSWeaponInfo*>(GetFileWeaponInfoFromHandle(hWpnInfo));

    return pWeaponInfo;
}

//--------------------------------------------------------------------------------------------------------
const char * WeaponClassAsString(CSWeaponType weaponType)
{
    WeaponTypeInfo *info = s_weaponTypeInfo;
    while (info->name != NULL)
    {
        if (info->type == weaponType)
        {
            return info->name;
        }
        ++info;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromString(const char * weaponType)
{
    WeaponTypeInfo *info = s_weaponTypeInfo;
    while (info->name != NULL)
    {
        if (!Q_stricmp(info->name, weaponType))
        {
            return info->type;
        }
        ++info;
    }

    return WEAPONTYPE_UNKNOWN;
}


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromWeaponID(CSWeaponID weaponID)
{
    const char *weaponStr = WeaponIDToAlias(weaponID);
    const char *translatedAlias = GetTranslatedWeaponAlias(weaponStr);

    char wpnName[128];
    Q_snprintf(wpnName, sizeof(wpnName), "weapon_%s", translatedAlias);
    WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot(wpnName);
    if (hWpnInfo != GetInvalidWeaponInfoHandle())
    {
        CCSWeaponInfo *pWeaponInfo = dynamic_cast<CCSWeaponInfo*>(GetFileWeaponInfoFromHandle(hWpnInfo));
        if (pWeaponInfo)
        {
            return pWeaponInfo->m_WeaponType;
        }
    }

    return WEAPONTYPE_UNKNOWN;
}


//--------------------------------------------------------------------------------------------------------
void ParseVector(KeyValues *keyValues, const char *keyName, Vector& vec)
{
    vec.x = vec.y = vec.z = 0.0f;

    if (!keyValues || !keyName)
        return;

    const char *vecString = keyValues->GetString(keyName, "0 0 0");
    if (vecString && *vecString)
    {
        float x, y, z;
        if (3 == sscanf(vecString, "%f %f %f", &x, &y, &z))
        {
            vec.x = x;
            vec.y = y;
            vec.z = z;
        }
    }
}

FileWeaponInfo_t* CreateWeaponInfo()
{
    return new CCSWeaponInfo;
}

CCSWeaponInfo::CCSWeaponInfo()
{
    m_flMaxSpeed = 1; // This should always be set in the script.
    m_szAddonModel[0] = 0;
}

int	CCSWeaponInfo::GetWeaponPrice(void)
{
    return m_iWeaponPrice;
}

int	CCSWeaponInfo::GetDefaultPrice(void)
{
    return m_iDefaultPrice;
}

int	CCSWeaponInfo::GetPrevousPrice(void)
{
    return m_iPreviousPrice;
}


void CCSWeaponInfo::Parse(KeyValues *pKeyValuesData, const char *szWeaponName)
{
    BaseClass::Parse(pKeyValuesData, szWeaponName);

    m_flMaxSpeed = (float) pKeyValuesData->GetInt("MaxPlayerSpeed", 1);

    m_iDefaultPrice = m_iWeaponPrice = pKeyValuesData->GetInt("WeaponPrice", -1);
    if (m_iWeaponPrice == -1)
    {
        // This weapon should have the price in its script.
        Assert(false);
    }

    /*if ( CSGameRules()->IsBlackMarket() )
    {
    int iWeaponID = ClassnameToWeaponID( GetTranslatedWeaponAlias ( szWeaponName ) );

    m_iDefaultPrice = m_iWeaponPrice;
    m_iPreviousPrice = CSGameRules()->GetBlackMarketPreviousPriceForWeapon( iWeaponID );
    m_iWeaponPrice = CSGameRules()->GetBlackMarketPriceForWeapon( iWeaponID );
    }*/

    m_flArmorRatio = pKeyValuesData->GetFloat("WeaponArmorRatio", 1);
    m_iCrosshairMinDistance = pKeyValuesData->GetInt("CrosshairMinDistance", 4);
    m_iCrosshairDeltaDistance = pKeyValuesData->GetInt("CrosshairDeltaDistance", 3);
    m_bCanUseWithShield = !!pKeyValuesData->GetInt("CanEquipWithShield", false);
    m_flMuzzleScale = pKeyValuesData->GetFloat("MuzzleFlashScale", 1);

    const char *pMuzzleFlashStyle = pKeyValuesData->GetString("MuzzleFlashStyle", "CS_MUZZLEFLASH_NORM");

    if (pMuzzleFlashStyle)
    {
        if (Q_stricmp(pMuzzleFlashStyle, "CS_MUZZLEFLASH_X") == 0)
        {
            m_iMuzzleFlashStyle = CS_MUZZLEFLASH_X;
        }
        else if (Q_stricmp(pMuzzleFlashStyle, "CS_MUZZLEFLASH_NONE") == 0)
        {
            m_iMuzzleFlashStyle = CS_MUZZLEFLASH_NONE;
        }
        else
        {
            m_iMuzzleFlashStyle = CS_MUZZLEFLASH_NORM;
        }
    }
    else
    {
        Assert(false);
    }

    m_iPenetration = pKeyValuesData->GetInt("Penetration", 1);
    m_iDamage = pKeyValuesData->GetInt("Damage", 42); // Douglas Adams 1952 - 2001
    m_flRange = pKeyValuesData->GetFloat("Range", 8192.0f);
    m_flRangeModifier = pKeyValuesData->GetFloat("RangeModifier", 0.98f);
    m_iBullets = pKeyValuesData->GetInt("Bullets", 1);
    m_flCycleTime = pKeyValuesData->GetFloat("CycleTime", 0.15);
    m_bAccuracyQuadratic = pKeyValuesData->GetBool("AccuracyQuadratic", false);
    m_flAccuracyDivisor = pKeyValuesData->GetFloat("AccuracyDivisor", -1); // -1 = off
    m_flAccuracyOffset = pKeyValuesData->GetFloat("AccuracyOffset", 0);
    m_flMaxInaccuracy = pKeyValuesData->GetFloat("MaxInaccuracy", 0);

    m_flTimeToIdleAfterFire = pKeyValuesData->GetFloat("TimeToIdle", 2);
    m_flIdleInterval = pKeyValuesData->GetFloat("IdleInterval", 20);

    // Figure out what team can have this weapon.
    m_iTeam = TEAM_UNASSIGNED;
    /*const char *pTeam = pKeyValuesData->GetString( "Team", NULL );
    if ( pTeam )
    {
    if ( Q_stricmp( pTeam, "CT" ) == 0 )
    {
    m_iTeam = TEAM_CT;
    }
    else if ( Q_stricmp( pTeam, "TERRORIST" ) == 0 )
    {
    m_iTeam = TEAM_TERRORIST;
    }
    else if ( Q_stricmp( pTeam, "ANY" ) == 0 )
    {
    m_iTeam = TEAM_UNASSIGNED;
    }
    else
    {
    Assert( false );
    }
    }
    else
    {
    Assert( false );
    }*/


    const char *pWrongTeamMsg = pKeyValuesData->GetString("WrongTeamMsg", "");
    Q_strncpy(m_WrongTeamMsg, pWrongTeamMsg, sizeof(m_WrongTeamMsg));

    const char *pShieldViewModel = pKeyValuesData->GetString("shieldviewmodel", "");
    Q_strncpy(m_szShieldViewModel, pShieldViewModel, sizeof(m_szShieldViewModel));

    const char *pAnimEx = pKeyValuesData->GetString("PlayerAnimationExtension", "m4");
    Q_strncpy(m_szAnimExtension, pAnimEx, sizeof(m_szAnimExtension));

    // Default is 2000.
    m_flBotAudibleRange = pKeyValuesData->GetFloat("BotAudibleRange", 2000.0f);

    const char *pTypeString = pKeyValuesData->GetString("WeaponType", NULL);

    m_WeaponType = WEAPONTYPE_UNKNOWN;
    if (!pTypeString)
    {
        Assert(false);
    }
    else if (Q_stricmp(pTypeString, "Knife") == 0)
    {
        m_WeaponType = WEAPONTYPE_KNIFE;
    }
    else if (Q_stricmp(pTypeString, "Pistol") == 0)
    {
        m_WeaponType = WEAPONTYPE_PISTOL;
    }
    else if (Q_stricmp(pTypeString, "Rifle") == 0)
    {
        m_WeaponType = WEAPONTYPE_RIFLE;
    }
    else if (Q_stricmp(pTypeString, "Shotgun") == 0)
    {
        m_WeaponType = WEAPONTYPE_SHOTGUN;
    }
    else if (Q_stricmp(pTypeString, "SniperRifle") == 0)
    {
        m_WeaponType = WEAPONTYPE_SNIPER_RIFLE;
    }
    else if (Q_stricmp(pTypeString, "SubMachinegun") == 0)
    {
        m_WeaponType = WEAPONTYPE_SUBMACHINEGUN;
    }
    else if (Q_stricmp(pTypeString, "Machinegun") == 0)
    {
        m_WeaponType = WEAPONTYPE_MACHINEGUN;
    }
    else if (Q_stricmp(pTypeString, "C4") == 0)
    {
        m_WeaponType = WEAPONTYPE_C4;
    }
    else if (Q_stricmp(pTypeString, "Grenade") == 0)
    {
        m_WeaponType = WEAPONTYPE_GRENADE;
    }
    else
    {
        Assert(false);
    }

    // Read the addon model.
    Q_strncpy(m_szAddonModel, pKeyValuesData->GetString("AddonModel"), sizeof(m_szAddonModel));

    // Read the dropped model.
    Q_strncpy(m_szDroppedModel, pKeyValuesData->GetString("DroppedModel"), sizeof(m_szDroppedModel));

    // Read the silencer model.
    Q_strncpy(m_szSilencerModel, pKeyValuesData->GetString("SilencerModel"), sizeof(m_szSilencerModel));

#ifndef CLIENT_DLL
    // Enforce consistency for the weapon here, since that way we don't need to save off the model bounds
    // for all time.
    //engine->ForceExactFile( UTIL_VarArgs("scripts/%s.ctx", szWeaponName ) );

    // Model bounds are rounded to the nearest integer, then extended by 1
    engine->ForceModelBounds(szWorldModel, Vector(-15, -12, -18), Vector(44, 16, 19));
    if (m_szAddonModel[0])
    {
        engine->ForceModelBounds(m_szAddonModel, Vector(-5, -5, -6), Vector(13, 5, 7));
    }
    if (m_szSilencerModel[0])
    {
        engine->ForceModelBounds(m_szSilencerModel, Vector(-15, -12, -18), Vector(44, 16, 19));
    }
#endif // !CLIENT_DLL
}


