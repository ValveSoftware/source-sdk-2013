//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Load item upgrade data from KeyValues
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_UPGRADES_H
#define TF_UPGRADES_H

#include "bot/tf_bot.h"
#include "networkvar.h"
#include "triggers.h"
#include "tf_population_manager.h"

struct UpgradeAttribBlock_t
{
	char szName[MAX_ATTRIBUTE_DESCRIPTION_LENGTH];
	float flValue;
	loadout_positions_t iSlot;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUpgrades : public CBaseTrigger, public CGameEventListener
{
public:
	DECLARE_CLASS( CUpgrades, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void	Spawn( void );

	virtual void	FireGameEvent( IGameEvent *gameEvent );

	void			UpgradeTouch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pEntity );

	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputReset( inputdata_t &inputdata );

	void			GrantOrRemoveAllUpgrades( CTFPlayer *pTFPlayer, bool bRemove = false, bool bRefund = true );
	bool			PlayerPurchasingUpgrade( CTFPlayer *pTFPlayer, int iItemSlot, int iUpgrade, bool bDowngrade, bool bFree = false, bool bRespec = false );

	attrib_definition_index_t ApplyUpgradeToItem( CTFPlayer *pTFPlayer, CEconItemView *pView, int iUpgrade, int nCost, bool bDowngrade = false, bool bIsFresh = false );		// needed for checkpoint restore

	const char *	GetUpgradeAttributeName( int iUpgrade ) const;

private:
	void			NotifyItemOnUpgrade( CTFPlayer *pTFPlayer, attrib_definition_index_t nAttrDefIndex, bool bDowngrade = false );
	void			ReportUpgrade ( CTFPlayer *pTFPlayer, int nItemDef, int nAttributeDef, int nQuality, int nCost, bool bDowngrade, bool bIsFresh, bool bIsBottle = false );
	void			RestoreItemAttributeToBaseValue( CEconItemAttributeDefinition *pAttrib, CEconItemView *pItem );
	void			RestorePlayerAttributeToBaseValue( CEconItemAttributeDefinition *pAttrib, CTFPlayer *pTFPlayer );

	void			ApplyUpgradeAttributeBlock( UpgradeAttribBlock_t *upgradeBlock, int upgradeCount, CTFPlayer *pPlayer, bool bDowngrade );

	int m_nStartDisabled;

	COutputEvent m_onPeriodicSpawn;

	bool m_bIsEnabled;
};

extern CHandle<CUpgrades>	g_hUpgradeEntity;

#endif // TF_UPGRADES_H
