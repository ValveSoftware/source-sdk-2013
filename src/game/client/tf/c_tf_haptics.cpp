//========= Copyright Valve Corporation, All rights reserved. ============//
// NVNT haptic manager for tf2
#include "cbase.h"
#include "c_tf_haptics.h"
#include "c_tf_player.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// for full cloak effect
extern ConVar tf_teammate_max_invis;

C_TFHaptics::C_TFHaptics()
{
	memset(this, 0, sizeof(C_TFHaptics));
}

void C_TFHaptics::Revert() {
	if ( haptics )
	{
		if(wasBeingHealed)
		{
			haptics->ProcessHapticEvent(2,"Game","being_healed_stop");
		}
		if(wasHealing)
		{
			haptics->ProcessHapticEvent(2,"Game","healing_stop");
		}
		if(wasUber)
		{
			haptics->ProcessHapticEvent(2, "Game", "uber_stop");
		}
		if(wasFullyCloaked)
		{
			haptics->ProcessHapticEvent(2, "Game", "cloak_full_stop");
		}
		if(wasCloaked)
		{
			haptics->ProcessHapticEvent(2, "Game", "cloak_stop");
		}
		if(wasBurning)
		{
			haptics->ProcessHapticEvent(2, "Game", "burning_stop");
		}
	}
	memset(this, 0, sizeof(C_TFHaptics));
	if ( haptics )
	{
		haptics->LocalPlayerReset();
		haptics->SetNavigationClass("on_foot");
	}
}

void C_TFHaptics::HapticsThink(C_TFPlayer *player)
{
	if ( !haptics )
		return;
	Assert(player!=C_TFPlayer::GetLocalPlayer());

	{// being healed check
		C_TFPlayer *pHealer = NULL;
		float uberCharge = 0.0f;
		player->GetHealer(&pHealer,&uberCharge);
		if(pHealer)
		{
			if(!wasBeingHealedMedic && !healingDispenserCount)
				isBeingHealed = true;
		}
		else
		{
			if(wasBeingHealedMedic && !healingDispenserCount)
				isBeingHealed = false;
		}
		wasBeingHealedMedic = pHealer!=NULL;
		if(isBeingHealed&&!wasBeingHealed&&haptics)
			haptics->ProcessHapticEvent(2,"Game","being_healed_start");
		else if(!isBeingHealed&&wasBeingHealed&&haptics)
			haptics->ProcessHapticEvent(2,"Game","being_healed_stop");

		wasBeingHealed = isBeingHealed;
	}
	{// healing check
		C_BaseEntity *pHealTarget = player->MedicGetHealTarget();
		if(pHealTarget) 
		{
			if(!wasHealing&&haptics)
				haptics->ProcessHapticEvent(2,"Game","healing_start");
		}
		else
		{
			if(wasHealing&&haptics)
				haptics->ProcessHapticEvent(2,"Game","healing_stop");
		}
		wasHealing = pHealTarget!=NULL;
	}
	//uber
	if(player->m_Shared.InCond( TF_COND_INVULNERABLE ) && !player->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
	{
		if(!wasUber&&haptics)
		{
			haptics->ProcessHapticEvent(2, "Game", "uber_start");
		}
	}else{
		if(wasUber&&haptics)
		{
			haptics->ProcessHapticEvent(2, "Game", "uber_stop");
		}
	}
	//burning
	if(player->m_Shared.InCond( TF_COND_BURNING ) )
	{
		if(!wasBurning&&haptics)
		{
			haptics->ProcessHapticEvent(2, "Game", "burning_start");
			wasBurning = true;
		}
	}else{
		if(wasBurning)
		{
			haptics->ProcessHapticEvent(2, "Game", "burning_stop");
			wasBurning = false;
		}
	}
	//cloak
	// note: theres some weird stuff going on here.
	float cloakLevel = player->GetPercentInvisible();
	if(readyForCloak)
	{
		if(cloakLevel>0.0f)
		{
			if(!wasCloaked)
			{
				haptics->ProcessHapticEvent(2, "Game", "cloak_start");
				wasCloaked = true;
			}
			if(!wasFullyCloaked)
			{
				if(cloakLevel >= tf_teammate_max_invis.GetFloat())
				{
					haptics->ProcessHapticEvent(2, "Game", "cloak_full_start");
					wasFullyCloaked = true;
				}
			}
			else
			{
				if(cloakLevel < tf_teammate_max_invis.GetFloat())
				{
					haptics->ProcessHapticEvent(2, "Game", "cloak_full_stop");
					wasFullyCloaked = false;
				}
			}
		}else{
			if(wasFullyCloaked)
			{
				haptics->ProcessHapticEvent(2, "Game", "cloak_full_stop");
				wasFullyCloaked = false;
			}
			if(wasCloaked)
			{
				haptics->ProcessHapticEvent(2, "Game", "cloak_stop");
				wasCloaked = false;
			}
		}
	}else{
		if(skippedFirstCloak) 
		{
			if(cloakLevel==0.0f)
				readyForCloak = true;
		}else{
			if(cloakLevel!=0.0f)
			{
				skippedFirstCloak = true;
			}
		}
	}
}

class C_TFHapticsInternal : public C_TFHaptics
{
public:
	C_TFHapticsInternal() : C_TFHaptics() {};
};

static C_TFHapticsInternal tfInternalHaptics;

C_TFHaptics &tfHaptics = *((C_TFHaptics*)&tfInternalHaptics);
