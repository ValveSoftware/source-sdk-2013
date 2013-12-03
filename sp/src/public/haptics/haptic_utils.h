#ifndef HAPTIC_UTILS_H
#define HAPTIC_UTILS_H


#ifdef CLIENT_DLL

#include "haptics/ihaptics.h"

// forward decl.
class C_BaseEntity;
class C_BaseCombatCharacter;
class C_BasePlayer;
class bf_read;

// use client side versions.
#ifndef CBasePlayer
#define CBasePlayer C_BasePlayer;
#endif
#ifndef CBaseCombatWeapon
#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

// stubbed version of haptics interface. Used when haptics is not available.
class CHapticsStubbed : public IHaptics
{
public:
public: // Initialization.
	virtual bool Initialize(IVEngineClient* newengine, 
		IViewRender *newview, 
		vgui::IInputInternal* newinput, 
		CGlobalVarsBase* newgpGlobals,
		CreateInterfaceFn newengineFactory, 
		void *IMEWindow,
		IFileSystem* filesystem, 
		IEngineVGui* newvgui,
		ActivityList_IndexForName_t actIndexForName,
		ActivityList_NameForIndex_t actNameForIndex)
		{return false;};

public: // Device methods
	virtual bool HasDevice(){return false;};
	virtual void ShutdownHaptics(){};

public: // Game input handling
	virtual void CalculateMove(float &forward_move, float &side_move, float delta){};
	virtual void OnPlayerChanged(){}
	virtual void SetNavigationClass(const char *defaultNavigationName){};
	virtual const char *GetNavigationClass(){ return 0; };
	virtual void GameProcess(){}
	virtual void MenuProcess(){}

public: // Effect methods
	virtual void ProcessHapticEvent(int numArgs, ...){}
	virtual void ProcessHapticWeaponActivity(const char *weapon, int activity){}
	virtual void HapticsPunch(float strength, const QAngle &angle){}
	virtual void ApplyDamageEffect(float damage, int damagetype, const Vector &angle){}
	virtual void UpdateAvatarVelocity(const Vector &vel){}
	virtual void RemoveAvatarEffect(){}
	virtual void SetConstantForce(const Vector &force){}
	virtual Vector GetConstantForce(){return Vector(0,0,0);}
	virtual void SetDrag(float amount){}
	virtual void SetShake(float scalar, float currentamount){}
	virtual void SetHeld(float amount){}
	virtual void SetMoveSurface(HapticSurfaceType_t surface){}
	virtual HapticSurfaceType_t GetMoveSurface(){ return HST_NONE; }
	virtual void SetDangling(float amount){};

public: // Notify methods
	virtual void LocalPlayerReset(){};
	virtual void UpdatePlayerFOV(float fov){};
	virtual void WorldPrecache() {};
};
#else
// forward decl.
class CBasePlayer;
class CBaseCombatWeapon;
class CTakeDamageInfo;

#endif // CLIENT_DLL

void HapticSendWeaponAnim(class CBaseCombatWeapon* weapon, int iActivity);
void HapticSetConstantForce(class CBasePlayer* pPlayer,Vector force);
void HapticSetDrag(class CBasePlayer* pPlayer, float drag);

// note: does nothing on server.
void HapticProcessSound(const char* soundname, int entIndex);

#ifdef CLIENT_DLL
	void ConnectHaptics(CreateInterfaceFn appFactory);
	void DisconnectHaptics();

	void UpdateAvatarEffect(void);
	void HapticsExitedVehicle(C_BaseEntity* vehicle, C_BaseCombatCharacter *pPassenger );
	void HapticsEnteredVehicle(C_BaseEntity* vehicle, C_BaseCombatCharacter *pPassenger );
	
	//bool value true if user is using a haptic device.
	extern ConVar hap_HasDevice;
#else
	void HapticsDamage(CBasePlayer* pPlayer, const CTakeDamageInfo &info);
	void HapticPunch(CBasePlayer* pPlayer, float amount, float x, float y);
	void HapticMeleeContact(CBasePlayer* pPlayer);
#endif

#endif