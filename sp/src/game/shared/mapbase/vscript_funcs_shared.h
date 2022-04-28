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
public:
	float			GetFriction() const				{ return physics.friction; }
	float			GetThickness() const			{ return physics.thickness; }

	float			GetJumpFactor() const			{ return game.jumpFactor; }
	char			GetMaterialChar() const			{ return game.material; }

	const char*		GetSoundStepLeft() const		{ return physprops->GetString( sounds.stepleft ); }
	const char*		GetSoundStepRight() const		{ return physprops->GetString( sounds.stepright ); }
	const char*		GetSoundImpactSoft() const		{ return physprops->GetString( sounds.impactSoft ); }
	const char*		GetSoundImpactHard() const		{ return physprops->GetString( sounds.impactHard ); }
	const char*		GetSoundScrapeSmooth() const	{ return physprops->GetString( sounds.scrapeSmooth ); }
	const char*		GetSoundScrapeRough() const		{ return physprops->GetString( sounds.scrapeRough ); }
	const char*		GetSoundBulletImpact() const	{ return physprops->GetString( sounds.bulletImpact ); }
	const char*		GetSoundRolling() const			{ return physprops->GetString( sounds.rolling ); }
	const char*		GetSoundBreak() const			{ return physprops->GetString( sounds.breakSound ); }
	const char*		GetSoundStrain() const			{ return physprops->GetString( sounds.strainSound ); }
};

//-----------------------------------------------------------------------------
// Exposes csurface_t to VScript
//-----------------------------------------------------------------------------
class CSurfaceScriptHelper
{
public:
	// This class is owned by CScriptGameTrace, and cannot be accessed without being initialised in CScriptGameTrace::RegisterSurface()
	//CSurfaceScriptHelper() : m_pSurface(NULL), m_hSurfaceData(NULL) {}

	~CSurfaceScriptHelper()
	{
		g_pScriptVM->RemoveInstance( m_hSurfaceData );
	}

	void Init( csurface_t *surf )
	{
		m_pSurface = surf;
		m_hSurfaceData = g_pScriptVM->RegisterInstance(
			reinterpret_cast< scriptsurfacedata_t* >( physprops->GetSurfaceData( m_pSurface->surfaceProps ) ) );
	}

	const char* Name() const { return m_pSurface->name; }
	HSCRIPT SurfaceProps() const { return m_hSurfaceData; }

private:
	csurface_t *m_pSurface;
	HSCRIPT m_hSurfaceData;
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
class CScriptGameTrace : public CGameTrace
{
public:
	CScriptGameTrace() : m_surfaceAccessor(NULL), m_planeAccessor(NULL)
	{
		m_hScriptInstance = g_pScriptVM->RegisterInstance( this );
	}

	~CScriptGameTrace()
	{
		if ( m_hScriptInstance )
		{
			g_pScriptVM->RemoveInstance( m_hScriptInstance );
		}

		if ( m_surfaceAccessor )
		{
			g_pScriptVM->RemoveInstance( m_surfaceAccessor );
		}

		if ( m_planeAccessor )
		{
			g_pScriptVM->RemoveInstance( m_planeAccessor );
		}
	}

	void RegisterSurface()
	{
		m_surfaceHelper.Init( &surface );
		m_surfaceAccessor = g_pScriptVM->RegisterInstance( &m_surfaceHelper );
	}

	void RegisterPlane()
	{
		m_planeAccessor = g_pScriptVM->RegisterInstance( &plane );
	}

	HSCRIPT GetScriptInstance() const
	{
		return m_hScriptInstance;
	}

public:
	float FractionLeftSolid() const		{ return fractionleftsolid; }
	int HitGroup() const				{ return hitgroup; }
	int PhysicsBone() const				{ return physicsbone; }

	HSCRIPT Entity() const				{ return ToHScript( m_pEnt ); }
	int HitBox() const					{ return hitbox; }

	const Vector& StartPos() const		{ return startpos; }
	const Vector& EndPos() const		{ return endpos; }

	float Fraction() const				{ return fraction; }

	int Contents() const				{ return contents; }
	int DispFlags() const				{ return dispFlags; }

	bool AllSolid() const				{ return allsolid; }
	bool StartSolid() const				{ return startsolid; }

	HSCRIPT Surface() const				{ return m_surfaceAccessor; }
	HSCRIPT Plane() const				{ return m_planeAccessor; }

	void Destroy()						{ delete this; }

private:
	HSCRIPT m_surfaceAccessor;
	HSCRIPT m_planeAccessor;
	HSCRIPT m_hScriptInstance;

	CSurfaceScriptHelper m_surfaceHelper;

	CScriptGameTrace( const CScriptGameTrace& v );
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
