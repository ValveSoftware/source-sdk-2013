//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: See vscript_funcs_shared.cpp
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_FUNCS_SHARED
#define VSCRIPT_FUNCS_SHARED
#ifdef _WIN32
#pragma once
#endif

#include "npcevent.h"
#ifdef GAME_DLL
#include "ai_memory.h"
#endif

//-----------------------------------------------------------------------------
// Exposes surfacedata_t to VScript
//-----------------------------------------------------------------------------
struct scriptsurfacedata_t : public surfacedata_t
{
	float			GetFriction() { return physics.friction; }
	float			GetThickness() { return physics.thickness; }

	float			GetJumpFactor() { return game.jumpFactor; }
	char			GetMaterialChar() { return game.material; }

	const char*		GetSoundStepLeft();
	const char*		GetSoundStepRight();
	const char*		GetSoundImpactSoft();
	const char*		GetSoundImpactHard();
	const char*		GetSoundScrapeSmooth();
	const char*		GetSoundScrapeRough();
	const char*		GetSoundBulletImpact();
	const char*		GetSoundRolling();
	const char*		GetSoundBreak();
	const char*		GetSoundStrain();
};

//-----------------------------------------------------------------------------
// Exposes csurface_t to VScript
//-----------------------------------------------------------------------------
class CSurfaceScriptAccessor
{
public:
	CSurfaceScriptAccessor( csurface_t &surf ) { m_surf = &surf; m_surfaceData = g_pScriptVM->RegisterInstance( reinterpret_cast<scriptsurfacedata_t*>(physprops->GetSurfaceData( m_surf->surfaceProps )) ); }
	~CSurfaceScriptAccessor() { delete m_surfaceData; }

	// cplane_t stuff
	const char* Name() const { return m_surf->name; }
	HSCRIPT SurfaceProps() const { return m_surfaceData; }

	void Destroy() { delete this; }

private:
	csurface_t *m_surf;
	HSCRIPT m_surfaceData;
};

//-----------------------------------------------------------------------------
// Exposes cplane_t to VScript
//-----------------------------------------------------------------------------
class CPlaneTInstanceHelper : public IScriptInstanceHelper
{
	bool Get( void *p, const char *pszKey, ScriptVariant_t &variant )
	{
		cplane_t *pi = ((cplane_t *)p);
		if (FStrEq(pszKey, "normal"))
			variant = pi->normal;
		else if (FStrEq(pszKey, "dist"))
			variant = pi->dist;
		else
			return false;

		return true;
	}

	//bool Set( void *p, const char *pszKey, ScriptVariant_t &variant );
};

//-----------------------------------------------------------------------------
// Exposes trace_t to VScript
//-----------------------------------------------------------------------------
class CTraceInfoAccessor
{
public:
	~CTraceInfoAccessor()
	{
		if (m_surfaceAccessor)
		{
			CSurfaceScriptAccessor *pScriptSurface = HScriptToClass<CSurfaceScriptAccessor>( m_surfaceAccessor );
			//g_pScriptVM->RemoveInstance( m_surfaceAccessor );
			delete pScriptSurface;
		}

		//if (m_planeAccessor)
		//{
		//	g_pScriptVM->RemoveInstance( m_planeAccessor );
		//}
	}

	// CGrameTrace stuff
	bool DidHitWorld() const { return m_tr.DidHitWorld(); }
	bool DidHitNonWorldEntity() const { return m_tr.DidHitNonWorldEntity(); }
	int GetEntityIndex() const { return m_tr.GetEntityIndex(); }
	bool DidHit() const { return m_tr.DidHit(); }

	float FractionLeftSolid() const { return m_tr.fractionleftsolid; }
	int HitGroup() const { return m_tr.hitgroup; }
	int PhysicsBone() const { return m_tr.physicsbone; }

	HSCRIPT Entity() const { return ToHScript(m_tr.m_pEnt); }

	int HitBox() const { return m_tr.hitbox; }

	// CBaseTrace stuff
	bool IsDispSurface() { return m_tr.IsDispSurface(); }
	bool IsDispSurfaceWalkable() { return m_tr.IsDispSurfaceWalkable(); }
	bool IsDispSurfaceBuildable() { return m_tr.IsDispSurfaceBuildable(); }
	bool IsDispSurfaceProp1() { return m_tr.IsDispSurfaceProp1(); }
	bool IsDispSurfaceProp2() { return m_tr.IsDispSurfaceProp2(); }

	const Vector& StartPos() const { return m_tr.startpos; }
	const Vector& EndPos() const { return m_tr.endpos; }

	float Fraction() const { return m_tr.fraction; }

	int Contents() const { return m_tr.contents; }
	int DispFlags() const { return m_tr.dispFlags; }

	bool AllSolid() const { return m_tr.allsolid; }
	bool StartSolid() const { return m_tr.startsolid; }

	HSCRIPT Surface() { return m_surfaceAccessor; }
	void SetSurface( HSCRIPT hSurfAccessor ) { m_surfaceAccessor = hSurfAccessor; }

	HSCRIPT Plane() { return m_planeAccessor; }
	void SetPlane( HSCRIPT hPlaneAccessor ) { m_planeAccessor = hPlaneAccessor; }

	trace_t &GetTrace() { return m_tr; }
	void Destroy() { delete this; }

private:
	trace_t m_tr;

	HSCRIPT m_surfaceAccessor;
	HSCRIPT m_planeAccessor;
};

//-----------------------------------------------------------------------------
// Exposes animevent_t to VScript
//-----------------------------------------------------------------------------
struct scriptanimevent_t : public animevent_t
{
	int GetEvent() { return event; }
	void SetEvent( int nEvent ) { event = nEvent; }

	const char *GetOptions() { return options; }
	void SetOptions( const char *pOptions ) { options = pOptions; }

	float GetCycle() { return cycle; }
	void SetCycle( float flCycle ) { cycle = flCycle; }

	float GetEventTime() { return eventtime; }
	void SetEventTime( float flEventTime ) { eventtime = flEventTime; }

	int GetType() { return type; }
	void SetType( int nType ) { eventtime = type; }

	HSCRIPT GetSource() { return ToHScript( pSource ); }
	void SetSource( HSCRIPT hSource )
	{
		CBaseEntity *pEnt = ToEnt( hSource );
		if (pEnt)
			pSource = pEnt->GetBaseAnimating();
	}
};

class CAnimEventTInstanceHelper : public IScriptInstanceHelper
{
	bool Get( void *p, const char *pszKey, ScriptVariant_t &variant );
	bool Set( void *p, const char *pszKey, ScriptVariant_t &variant );
};

//-----------------------------------------------------------------------------
// Exposes EmitSound_t to VScript
//-----------------------------------------------------------------------------
struct ScriptEmitSound_t : public EmitSound_t
{
	int GetChannel() { return m_nChannel; }
	void SetChannel( int nChannel ) { m_nChannel = nChannel; }

	const char *GetSoundName() { return m_pSoundName; }
	void SetSoundName( const char *pSoundName ) { m_pSoundName = pSoundName; }

	float GetVolume() { return m_flVolume; }
	void SetVolume( float flVolume ) { m_flVolume = flVolume; }

	int GetSoundLevel() { return m_SoundLevel; }
	void SetSoundLevel( int iSoundLevel ) { m_SoundLevel = (soundlevel_t)iSoundLevel; }

	int GetFlags() { return m_nFlags; }
	void SetFlags( int nFlags ) { m_nFlags = nFlags; }

	int GetSpecialDSP() { return m_nSpecialDSP; }
	void SetSpecialDSP( int nSpecialDSP ) { m_nSpecialDSP = nSpecialDSP; }

	bool HasOrigin() { return m_pOrigin ? true : false; }
	const Vector &GetOrigin() { return m_pOrigin ? *m_pOrigin : vec3_origin; }
	void SetOrigin( const Vector &origin ) { static Vector tempOrigin; tempOrigin = origin; m_pOrigin = &tempOrigin; }
	void ClearOrigin() { m_pOrigin = NULL; }

	float GetSoundTime() { return m_flSoundTime; }
	void SetSoundTime( float flSoundTime ) { m_flSoundTime = flSoundTime; }

	float GetEmitCloseCaption() { return m_bEmitCloseCaption; }
	void SetEmitCloseCaption( bool bEmitCloseCaption ) { m_bEmitCloseCaption = bEmitCloseCaption; }

	float GetWarnOnMissingCloseCaption() { return m_bWarnOnMissingCloseCaption; }
	void SetWarnOnMissingCloseCaption( bool bWarnOnMissingCloseCaption ) { m_bWarnOnMissingCloseCaption = bWarnOnMissingCloseCaption; }

	float GetWarnOnDirectWaveReference() { return m_bWarnOnDirectWaveReference; }
	void SetWarnOnDirectWaveReference( bool bWarnOnDirectWaveReference ) { m_bWarnOnDirectWaveReference = bWarnOnDirectWaveReference; }

	int GetSpeakerEntity() { return m_nSpeakerEntity; }
	void SetSpeakerEntity( int nSpeakerEntity ) { m_nSpeakerEntity = nSpeakerEntity; }

	int GetSoundScriptHandle() { return m_hSoundScriptHandle; }
	void SetSoundScriptHandle( int hSoundScriptHandle ) { m_hSoundScriptHandle = hSoundScriptHandle; }
};

//-----------------------------------------------------------------------------
// Exposes CUserCmd to VScript
//-----------------------------------------------------------------------------
class CScriptUserCmd : public CUserCmd
{
public:
	int GetCommandNumber() { return command_number; }

	int ScriptGetTickCount() { return tick_count; }

	const QAngle& GetViewAngles() { return viewangles; }
	void SetViewAngles( const QAngle& val ) { viewangles = val; }

	float GetForwardMove() { return forwardmove; }
	void SetForwardMove( float val ) { forwardmove = val; }
	float GetSideMove() { return sidemove; }
	void SetSideMove( float val ) { sidemove = val; }
	float GetUpMove() { return upmove; }
	void SetUpMove( float val ) { upmove = val; }

	int GetButtons() { return buttons; }
	void SetButtons( int val ) { buttons = val; }
	int GetImpulse() { return impulse; }
	void SetImpulse( int val ) { impulse = val; }

	int GetWeaponSelect() { return weaponselect; }
	void SetWeaponSelect( int val ) { weaponselect = val; }
	int GetWeaponSubtype() { return weaponsubtype; }
	void SetWeaponSubtype( int val ) { weaponsubtype = val; }

	int GetRandomSeed() { return random_seed; }

	int GetMouseX() { return mousedx; }
	void SetMouseX( int val ) { mousedx = val; }
	int GetMouseY() { return mousedy; }
	void SetMouseY( int val ) { mousedy = val; }
};

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Exposes AI_EnemyInfo_t to VScript
//-----------------------------------------------------------------------------
struct Script_AI_EnemyInfo_t : public AI_EnemyInfo_t
{
	#define ENEMY_INFO_SCRIPT_FUNCS(type, name, var) \
	type			Get##name() { return var; } \
	void			Set##name( type v ) { var = v; }

	HSCRIPT			Enemy() { return ToHScript(hEnemy); }
	void			SetEnemy( HSCRIPT ent ) { hEnemy = ToEnt(ent); }

	ENEMY_INFO_SCRIPT_FUNCS( Vector, LastKnownLocation, vLastKnownLocation );
	ENEMY_INFO_SCRIPT_FUNCS( Vector, LastSeenLocation, vLastSeenLocation );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeLastSeen, timeLastSeen );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeFirstSeen, timeFirstSeen );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeLastReacquired, timeLastReacquired );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeValidEnemy, timeValidEnemy );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeLastReceivedDamageFrom, timeLastReceivedDamageFrom );
	ENEMY_INFO_SCRIPT_FUNCS( float, TimeAtFirstHand, timeAtFirstHand );
	ENEMY_INFO_SCRIPT_FUNCS( bool, DangerMemory, bDangerMemory );
	ENEMY_INFO_SCRIPT_FUNCS( bool, EludedMe, bEludedMe );
	ENEMY_INFO_SCRIPT_FUNCS( bool, Unforgettable, bUnforgettable );
	ENEMY_INFO_SCRIPT_FUNCS( bool, MobbedMe, bMobbedMe );
};
#endif

#endif
