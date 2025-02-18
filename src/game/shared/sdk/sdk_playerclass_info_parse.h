//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_PLAYERCLASS_INFO_PARSE_H
#define SDK_PLAYERCLASS_INFO_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "playerclass_info_parse.h"
#include "networkvar.h"

#if defined ( SDK_USE_PLAYERCLASSES )
//--------------------------------------------------------------------------------------------------------
class CSDKPlayerClassInfo final : public FilePlayerClassInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CSDKPlayerClassInfo, FilePlayerClassInfo_t );
	
	CSDKPlayerClassInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	int m_iTeam;		//which team. 2 == team 1, 3 == team 2

	int m_iPrimaryWeapon;
	int m_iSecondaryWeapon;
	int m_iMeleeWeapon;

	int m_iNumGrensType1;
	int m_iGrenType1;

	int m_iNumGrensType2;
	int m_iGrenType2;

	char m_szLimitCvar[64];	//which cvar controls the class limit for this class

	char m_szClassImage[SDK_PLAYERCLASS_IMAGE_LENGTH];
	char m_szClassImageBG[SDK_PLAYERCLASS_IMAGE_LENGTH];

	float m_flRunSpeed;
	float m_flSprintSpeed;
	float m_flProneSpeed;

	int m_iArmor;
};
#endif // SDK_USE_PLAYERCLASSES

#endif // DOD_PLAYERCLASS_INFO_PARSE_H
