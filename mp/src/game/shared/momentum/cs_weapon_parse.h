//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CS_WEAPON_PARSE_H
#define CS_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_parse.h"
#include "networkvar.h"


//--------------------------------------------------------------------------------------------------------
enum CSWeaponType
{

	WEAPONTYPE_KNIFE=0,	
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN

};


//--------------------------------------------------------------------------------------------------------
enum CSWeaponID
{
	WEAPON_NONE = 0,

	WEAPON_P228,
	WEAPON_GLOCK,
	WEAPON_SCOUT,
	WEAPON_HEGRENADE,
	WEAPON_XM1014,
	WEAPON_C4,
	WEAPON_MAC10,
	WEAPON_AUG,
	WEAPON_SMOKEGRENADE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_UMP45,
	WEAPON_SG550,

	WEAPON_GALIL,
	WEAPON_FAMAS,
	WEAPON_USP,
	WEAPON_AWP,
	WEAPON_MP5NAVY,
	WEAPON_M249,
	WEAPON_M3,
	WEAPON_M4A1,
	WEAPON_TMP,
	WEAPON_G3SG1,
	WEAPON_FLASHBANG,
	WEAPON_DEAGLE,
	WEAPON_SG552,
	WEAPON_AK47,
	WEAPON_KNIFE,
	WEAPON_P90,

	WEAPON_SHIELDGUN,	// BOTPORT: Is this still needed?

	WEAPON_KEVLAR,
	WEAPON_ASSAULTSUIT,
	WEAPON_NVG,

	WEAPON_MAX,		// number of weapons weapon index
};


#define MAX_EQUIPMENT (WEAPON_MAX - WEAPON_KEVLAR)

void PrepareEquipmentInfo( void );

//--------------------------------------------------------------------------------------------------------
const char * WeaponClassAsString( CSWeaponType weaponType );


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromString( const char * weaponType );

//--------------------------------------------------------------------------------------------------------
enum CSWeaponID;


//--------------------------------------------------------------------------------------------------------
const char * WeaponClassAsString( CSWeaponType weaponType );


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromString( const char * weaponType );


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromWeaponID( CSWeaponID weaponID );


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromWeaponID( CSWeaponID weaponID );


//--------------------------------------------------------------------------------------------------------
class CCSWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CCSWeaponInfo, FileWeaponInfo_t );
	
	CCSWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	int GetRealWeaponPrice( void ) { return m_iWeaponPrice; }


public:

	float m_flMaxSpeed;			// How fast the player can run while this is his primary weapon.

	CSWeaponType m_WeaponType;

	int m_iTeam;				// Which team can have this weapon. TEAM_UNASSIGNED if both can have it.
	float m_flBotAudibleRange;	// How far away a bot can hear this weapon.
	float m_flArmorRatio;

	int	  m_iCrosshairMinDistance;
	int	  m_iCrosshairDeltaDistance;
	
	bool  m_bCanUseWithShield;
	
	char m_WrongTeamMsg[32];	// Reference to a string describing the error if someone tries to buy
								// this weapon but they're on the wrong team to have it.
								// Zero-length if no specific message for this weapon.

	char m_szAnimExtension[16];
	char m_szShieldViewModel[64];

	char m_szAddonModel[MAX_WEAPON_STRING];		// If this is set, it is used as the addon model. Otherwise, szWorldModel is used.
	char m_szDroppedModel[MAX_WEAPON_STRING];	// Alternate dropped model, if different from the szWorldModel the player holds
	char m_szSilencerModel[MAX_WEAPON_STRING];	// Alternate model with silencer attached

	int	  m_iMuzzleFlashStyle;
	float m_flMuzzleScale;
	
	// Parameters for FX_FireBullets:
	int		m_iPenetration;
	int		m_iDamage;
	float	m_flRange;
	float	m_flRangeModifier;
	int		m_iBullets;
	float	m_flCycleTime;

	// Variables that control how fast the weapon's accuracy changes as it is fired.
	bool	m_bAccuracyQuadratic;
	float	m_flAccuracyDivisor;
	float	m_flAccuracyOffset;
	float	m_flMaxInaccuracy;

	// Delay until the next idle animation after shooting.
	float	m_flTimeToIdleAfterFire;
	float	m_flIdleInterval;
   
	int		GetWeaponPrice( void );
	int		GetDefaultPrice( void );
	int		GetPrevousPrice( void );
	void	SetWeaponPrice( int iPrice ) { m_iWeaponPrice = iPrice; }
	void	SetDefaultPrice( int iPrice ) { m_iDefaultPrice = iPrice; }
	void	SetPreviousPrice( int iPrice ) { m_iPreviousPrice = iPrice; }
    
private:

	int m_iWeaponPrice;
	int m_iDefaultPrice;
	int m_iPreviousPrice;

};


#endif // CS_WEAPON_PARSE_H
