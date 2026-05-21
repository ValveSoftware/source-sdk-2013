//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef OW_HERO_REGISTRY_H
#define OW_HERO_REGISTRY_H

#include "ow_shareddefs.h"

struct OWHeroDefinition_t
{
	int		m_iHeroId;
	char	m_szName[64];
	char	m_szInternalName[32];
	int		m_iTFClass;
	int		m_iMaxHealth;
	float	m_flMoveSpeedScale;
	char	m_szPrimaryWeapon[64];
	char	m_szSecondaryWeapon[64];
	char	m_szMeleeWeapon[64];
	float	m_flAbilityCooldown[OW_MAX_ABILITIES];
	int		m_iAbilityType[OW_MAX_ABILITIES];
	int		m_iAbilityCond[OW_MAX_ABILITIES];
	float	m_flAbilityDuration[OW_MAX_ABILITIES];
	float	m_flUltDuration;
	int		m_iUltType;
	float	m_flUltChargeRate;
};

class COWHeroRegistry
{
public:
	void		Reload( void );
	void		Clear( void );
	int			GetHeroCount( void ) const { return m_nHeroCount; }
	const OWHeroDefinition_t *GetHeroById( int iHeroId ) const;
	const OWHeroDefinition_t *GetHeroByTFClass( int iTFClass ) const;
	const OWHeroDefinition_t *GetHeroByIndex( int iIndex ) const;
	int			GetTFClassForHeroId( int iHeroId ) const;
	bool		IsTFClassEnabled( int iTFClass ) const;
	void		PrintRoster( void ) const;

	static COWHeroRegistry &Instance( void );

private:
	OWHeroDefinition_t	m_Heroes[OW_MAX_HEROES];
	int					m_nHeroCount;
};

#ifdef GAME_DLL
void OW_LoadModeConfig( void );
OWGameModeType_t OW_GetModeForMap( const char *pszMapName );
#endif

#endif // OW_HERO_REGISTRY_H
