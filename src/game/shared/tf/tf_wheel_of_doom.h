//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WHEEL_OF_DOOM_H
#define TF_WHEEL_OF_DOOM_H
#pragma once

#include "GameEventListener.h"

// Client specific.
#ifdef CLIENT_DLL
#define CWheelOfDoom C_WheelOfDoom
#endif

enum eWheelOfDoomEffectSkinIndex
{
	EFFECT_WHAMMY = 1,
	EFFECT_JUMP_HEIGHT,
	EFFECT_SMALL_HEAD,
	EFFECT_SPEED,
	EFFECT_LOW_GRAVITY,
	EFFECT_BIG_HEAD,
	EFFECT_UBER,
	EFFECT_CRITS,
	EFFECT_DANCE,
	EFFECT_BASEBALLS,
	EFFECT_DECAPITATE,
	EFFECT_BARS,
	EFFECT_BELLS,
	EFFECT_SEVENS,
	EFFECT_CHERRIES,

	EFFECT_COUNT
};


#ifdef GAME_DLL

class CWheelOfDoomSpiral;

//=============================================================================
//
// Wheel of doom class
//
class CWheelOfDoom : public CBaseAnimating, public CGameEventListener
{
public:
	DECLARE_CLASS( CWheelOfDoom, CBaseAnimating );
	DECLARE_DATADESC();


	CWheelOfDoom( void );
	~CWheelOfDoom( void );

	// Initialization
	virtual void Spawn( void );
	virtual void Precache( void );

	// Thinks
	void IdleThink( void );
	void SpinThink( void );

	virtual void FireGameEvent( IGameEvent *gameEvent );

	void StartSpin( void );
	void Spin( inputdata_t& inputdata );
	void ClearAllEffects( inputdata_t& inputdata );
	bool IsDoneBoardcastingEffectSound() const;

	float GetDuration() const { return m_flDuration; }

	void DBG_ApplyEffectByName( const char* pszEffectName );

private:

	enum eEffectProperty
	{
		PROPERTY_WHAMMY,
		PROPERTY_DOES_NOT_REAPPLY_ON_SPAWN
	};

	struct EffectData_t
	{
		CUtlVector<CTFPlayer*> m_vecPlayers;
		CWheelOfDoom* m_pWheel;
	};

	struct EffectManager;
	class WOD_BaseEffect
	{
	public:
		WOD_BaseEffect();
		virtual ~WOD_BaseEffect() {}

		virtual void InitEffect( float flDefaultDuration );

		virtual void ActivateEffect( EffectData_t& data )	{};
		virtual void UpdateEffect( EffectData_t& data  )	{};
		virtual void DeactivateEffect( EffectData_t& data )	{};

		void SetListFlags( int iFlags );
		int GetListFlags() const			{ return m_iListFlags; }
		const char* GetName()				{ return m_pszName; }
		eWheelOfDoomEffectSkinIndex GetSkinIndex() const { return m_nSkin; }
	protected:

		eWheelOfDoomEffectSkinIndex m_nSkin;
		float m_flExpireTime;
		const char* m_pszEffectAnnouncementSound;
		int m_iListFlags;

		const char* m_pszName;

		friend struct EffectManager;
	};

	struct EffectManager
	{
	public:
		explicit EffectManager( CWheelOfDoom* pWheel ) { m_pWheel = pWheel; }
		~EffectManager();

		int AddEffect( WOD_BaseEffect* pEffect, float flDefaultDuration );
		void ApplyAllEffectsToPlayer( CTFPlayer* pPlayer );
		void ClearEffects();
		bool UpdateAndClearExpiredEffects();
		void Precache();

	private:
		CWheelOfDoom*	m_pWheel;
		CUtlVector<WOD_BaseEffect*> m_vecActiveEffects;
	} m_EffectManager;

	WOD_BaseEffect* GetRandomEffectWithFlags();
	void EndSpin();

	CUtlVector<WOD_BaseEffect*> m_vecEffects;

	void RegisterEffect( WOD_BaseEffect* pEffect, int nFlags = 0 );

	static void ApplyAttributeToAllPlayers( const char* pszAttribName, float flValue );
	static void ApplyAttributeToPlayer( CTFPlayer* pPlayer, const char* pszAttribName, float flValue );
	static void RemoveAttributeFromAllPlayers( const char* pszAttribName );
	static void RemoveAttributeFromPlayer( CTFPlayer* pPlayer, const char* pszAttribName );

	static void SpeakMagicConceptToAllPlayers( const char* pszEffect );


	WOD_BaseEffect* m_pChosenEffect;
	CWheelOfDoomSpiral* m_pSpiral;

	static const char* GetScreenModelName();
	float CalcNextTickTime() const;
	float CalcSpinCompletion() const;
	void SetSkin( int nSkin );
	void SetScale( float flScale );
	void PlaySound( const char* pszSound );

	bool	m_bAnnounced;
	bool	m_bHasSpiral;
	float	m_flDuration;
	float	m_flNextTickTime;
	float	m_flNextAnnounceTime;
	float	m_flStopSpinTime;
	CUtlVector<CWheelOfDoom*> m_vecOtherWODs;

	float	m_flFinishBroadcastingEffectTime;

	COutputEvent m_EffectApplied;
	COutputEvent m_EffectExpired;


	//-----------------------------------------------------------------------
	// Effects

	class WOD_CritsEffect : public WOD_BaseEffect
	{
	public:
		WOD_CritsEffect()
		{
			m_pszName = "Crits";
			m_nSkin = EFFECT_CRITS;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelCrits";
		};

		void ActivateEffect( EffectData_t& data );
	};

	class WOD_UberEffect : public WOD_BaseEffect
	{
	public:
		WOD_UberEffect()
		{
			m_pszName = "Uber";
			m_nSkin = EFFECT_UBER;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelUber";
		};

		void InitEffect( float flDefaultExpireTime );
		void ActivateEffect( EffectData_t& data );
	};

	class WOD_SuperSpeedEffect : public WOD_BaseEffect
	{
	public:
		WOD_SuperSpeedEffect()
		{
			m_pszName = "Super Speed";
			m_nSkin = EFFECT_SPEED;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelSuperSpeed";
		};

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_SuperJumpEffect : public WOD_BaseEffect
	{
	public:
		WOD_SuperJumpEffect()
		{
			m_pszName = "Super Jump";
			m_nSkin = EFFECT_JUMP_HEIGHT;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelSuperJump";
		}

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_BigHeadEffect : public WOD_BaseEffect
	{
	public:
		WOD_BigHeadEffect()
		{
			m_pszName = "Big Head";
			m_nSkin = EFFECT_BIG_HEAD;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelBigHead";
		}

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_SmallHeadEffect : public WOD_BaseEffect
	{
	public:
		WOD_SmallHeadEffect()
		{
			m_pszName = "Small Head";
			m_nSkin = EFFECT_SMALL_HEAD;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelShrunkHead";
		}

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_LowGravityEffect : public WOD_BaseEffect
	{
	public:
		WOD_LowGravityEffect()
		{
			m_pszName = "Low Gravity";
			m_nSkin = EFFECT_LOW_GRAVITY;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelGravity";
		}

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_Pee : public WOD_BaseEffect
	{
	public:
		WOD_Pee()
		{
			m_pszName = "Pee";
			m_nSkin = EFFECT_WHAMMY;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelJarate";
		}

		void ActivateEffect( EffectData_t& data );
		void UpdateEffect( EffectData_t& data );

	private:

		float m_flNextPeeTime;
		CUtlVector<CBaseEntity*> m_vecClouds;
	};

	class WOD_Burn : public WOD_BaseEffect
	{
	public:
		WOD_Burn()
		{
			m_pszName = "Burn";
			m_nSkin = EFFECT_WHAMMY;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelBurn";
		}

		void InitEffect( float flDefaultDuration );
		void ActivateEffect( EffectData_t& data );
	};

	class WOD_Ghosts : public WOD_BaseEffect
	{
	public:
		WOD_Ghosts()
		{
			m_pszName = "Ghosts";
			m_nSkin = EFFECT_WHAMMY;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelGhosts";
		}

		void ActivateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );
	};

	class WOD_Dance : public WOD_BaseEffect
	{
	public:
		WOD_Dance()
		{
			m_pszName = "Dance";
			m_nSkin = EFFECT_DANCE;
			m_pszEffectAnnouncementSound = "Halloween.MerasmusWheelDance";
			m_flNextDanceTime = 0.f;
		}

		void InitEffect( float flExpireTime );

		void UpdateEffect( EffectData_t& data );
		void DeactivateEffect( EffectData_t& data );

	private:
		int GetNumOFTeamDancing( int nTeam ) const;
		void SlamPosAndAngles( CTFPlayer* pPlayer, const Vector& vPos, const QAngle& vAng );
		
		struct Dancer_t
		{
			Vector m_vecPos;
			QAngle m_vecAngles;
			CHandle<CTFPlayer> m_hPlayer;
		};

		CUtlVector<Dancer_t*> m_vecDancers;

		struct MerasmusCreateInfo_t
		{
			MerasmusCreateInfo_t( const Vector& p, const QAngle& ang ) : m_vecPos( p ), m_vecAngles( ang ) {}
			Vector m_vecPos;
			QAngle m_vecAngles;
		};
		CUtlVector< MerasmusCreateInfo_t > m_vecMerasmusDancerCreateInfos;
		int m_iCurrentMerasmusCreateInfo;
		CHandle<CMerasmusDancer> m_hMerasmusDancer;

		float m_flNextDanceTime;
	};
};

#endif // GAME_DLL

#endif // TF_WHEEL_OF_DOOM_H
