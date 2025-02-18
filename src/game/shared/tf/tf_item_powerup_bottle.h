//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_POWERUP_BOTTLE_H
#define TF_POWERUP_BOTTLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_item_wearable.h"

#if defined( CLIENT_DLL )
#define CTFPowerupBottle C_TFPowerupBottle
#include "econ_notifications.h"
#endif


class CTFPowerupBottle : public CTFWearable
{
	DECLARE_CLASS( CTFPowerupBottle, CTFWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFPowerupBottle();
	virtual ~CTFPowerupBottle() { }

	PowerupBottleType_t GetPowerupType( void ) const;

	virtual void Precache( void );

	// reset the bottle to its initial state
	void Reset( void );

	// Unequips the item as usual, but also removes any effect it may have been granting
	virtual void UnEquip( CBasePlayer* pOwner );

	// Overridden so that this item can apply the effect only when it is active
	virtual void ReapplyProvision();

	// @return true if the effect was applied and a charge was consumed, false otherwise
	bool Use();

	// Remove the effect applied by the item
	void RemoveEffect();

	// set the number of charges availabe on this item
	// @param usNumCharges
	void SetNumCharges( uint8 usNumCharges );

	// @return the number of charges the item has
	uint8 GetNumCharges() const;

	// @return the maximum number of charges this item can hold
	uint8 GetMaxNumCharges() const;

	bool AllowedToUse();

	const char* GetEffectLabelText( void );
	const char* GetEffectIconName( void );
	float GetProgress( void ) { return 0.0f; }

	virtual int		GetSkin();
	bool IsBasePowerUpBottle( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return (iMode == 1); };

protected:

	// Used internally to remove the effect after a tunable amount of time
	void StatusThink();

	CNetworkVar( bool, m_bActive );
	CNetworkVar( uint8, m_usNumCharges );

private:

#ifdef TF_CLIENT_DLL
	virtual void FireGameEvent( IGameEvent *event );
	virtual int GetWorldModelIndex( void );
#endif 

	float m_flLastSpawnTime;
};

#ifdef CLIENT_DLL

// ******************************************************************************************
// CEquipMvMCanteenNotification - Client notification to equip a canteen
// ******************************************************************************************
class CEquipMvMCanteenNotification : public CEconNotification
{
public:
	CEquipMvMCanteenNotification() : CEconNotification()
	{
		m_bHasTriggered = false;
	}

	~CEquipMvMCanteenNotification()
	{
		if ( !m_bHasTriggered )
		{
			m_bHasTriggered = true;
		}
	}

	virtual void MarkForDeletion()
	{
		m_bHasTriggered = true;
		CEconNotification::MarkForDeletion();
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }
	virtual bool BShowInGameElements() const { return true; }

	virtual void Accept();
	virtual void Trigger() { Accept(); }
	virtual void Decline() { MarkForDeletion(); }
	virtual void UpdateTick();

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast<CEquipMvMCanteenNotification *>( pNotification ) != NULL; }

private:
	bool m_bHasTriggered;
};

#endif // client

#endif // TF_POWERUP_BOTTLE_H
