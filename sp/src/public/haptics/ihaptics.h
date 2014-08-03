#ifndef HAPTICS_INTERFACE_H
#define HAPTICS_INTERFACE_H

#ifdef GAME_DLL
#pragma warning("IHaptics.h is only for client ussage");
#endif

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"

#define HAPTICS_INTERFACE_VERSION "HapticsInterface001"
#define HAPTICS_DLL_NAME "haptics"

// systems forward decl.
class IVEngineClient;
class IViewRender;
class IInputInternal;
class CGlobalVarsBase;
class IFileSystem;
class IEngineVGui;

// vgui forward decl
namespace vgui{
    class IInputInternal;
}

// math types forward decl
class QAngle;
class Vector;

typedef enum {
	HST_NONE = 0,
	HST_ROPE,
} HapticSurfaceType_t;

typedef int (*ActivityList_IndexForName_t)( const char *pszActivityName );
typedef const char *(*ActivityList_NameForIndex_t)( int iActivityIndex );
// NVNT haptic system interface declaration
abstract_class IHaptics
{
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
		ActivityList_NameForIndex_t actNameForIndex) = 0;

public: // Device methods

	// returns true if there is at least one device connected.
	virtual bool HasDevice() = 0;

	// closes all haptic devices and effect processing
	virtual void ShutdownHaptics() = 0;

public: // Game input handling
	
	// computes view angles and adjusts forward_move and side_move
	virtual void CalculateMove(float &forward_move, float &side_move, float delta) = 0;

	virtual void OnPlayerChanged()=0;

	// Sets the internal navigation class.	
	virtual void SetNavigationClass(const char *defaultNavigationName) = 0;

	// Turns the internal navigation off. ( clears navigation class )
	inline  void ClearNavigationClass();

	// Returns the active navigation class ( if none returns NULL ) 
	virtual const char *GetNavigationClass() = 0;

	// Should be called by the game input class after CalculateMove (when not in menu)
	virtual void GameProcess() = 0;

	// Should be called by the game input class when in a menu
	virtual void MenuProcess() = 0;


public: // Effect methods
	
	// process a haptic event.
	virtual void ProcessHapticEvent(int numArgs, ...) = 0;
	virtual void ProcessHapticWeaponActivity(const char *weapon, int activity) = 0;
	
	// send a haptic punch effect
	virtual void HapticsPunch(float strength, const QAngle &angle) = 0;

	// trigger a damage effect
	virtual void ApplyDamageEffect(float damage, int damagetype, const Vector &angle) = 0;
	
	// update the avatar ( acceleration ) effect by a velocity sample
	virtual void UpdateAvatarVelocity(const Vector &velocity) = 0;

	// stop processing any running avatar effects
	virtual void RemoveAvatarEffect() = 0;

	// sets the device's constant force effect to force vector
	virtual void SetConstantForce(const Vector &force) = 0;

	// returns the last sent constant force
	virtual Vector GetConstantForce() = 0;

	// set the amount of drag (viscosity) on the haptic devices
	virtual void SetDrag(float amount) = 0;

	// set the values to the screen shake effect
	virtual void SetShake(float scalar, float currentamount) = 0;

	// enable/disable device position lock.
	virtual void SetHeld(float amount) = 0;

	// set anchor weight mass scaler.
	virtual void SetMoveSurface(HapticSurfaceType_t surface) = 0;

	virtual HapticSurfaceType_t GetMoveSurface() = 0;

	// set dangling ( being hung, holding onto ledges )
	virtual void SetDangling(float amount) = 0;

public: // Notify methods
	
	// notify the haptics system that we have been respawned.
	virtual void LocalPlayerReset()=0;

	// notify the haptics system of the player's field of view angle
	virtual void UpdatePlayerFOV(float fov)=0;

	virtual void WorldPrecache() = 0;

};

inline void IHaptics::ClearNavigationClass( void )
{
	 SetNavigationClass(0);
}

extern IHaptics* haptics;
	
#endif// HAPTICS_INTERFACE_H