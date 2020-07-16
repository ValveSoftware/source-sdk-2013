//========= Copyright Valve Corporation, All rights reserved. =================
//
// Purpose: Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
//			some of the code normally available in games like L4D2 or Valve's original VScript DLL.
//			Instead, that code is recreated here, shared between server and client.
//
//			It also contains other functions unique to Mapbase.
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_FUNCS_SHARED
#define VSCRIPT_FUNCS_SHARED
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Exposes trace_t to VScript
//-----------------------------------------------------------------------------
class CTraceInfoAccessor
{
public:

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

	Vector StartPos() const { return m_tr.startpos; }
	Vector EndPos() const { return m_tr.endpos; }

	float Fraction() const { return m_tr.fraction; }

	int Contents() const { return m_tr.contents; }
	int DispFlags() const { return m_tr.dispFlags; }

	bool AllSolid() const { return m_tr.allsolid; }
	bool StartSolid() const { return m_tr.startsolid; }

	trace_t &GetTrace() { return m_tr; }
	void Destroy() { delete this; }

private:
	trace_t m_tr;

};

//-----------------------------------------------------------------------------
// Exposes FireBulletsInfo_t to VScript
//-----------------------------------------------------------------------------
class CFireBulletsInfoAccessor
{
public:
	CFireBulletsInfoAccessor( FireBulletsInfo_t *info ) { m_info = info; }

	int GetShots() { return m_info->m_iShots; }
	void SetShots( int value ) { m_info->m_iShots = value; }

	Vector GetSource() { return m_info->m_vecSrc; }
	void SetSource( Vector value ) { m_info->m_vecSrc = value; }
	Vector GetDirShooting() { return m_info->m_vecDirShooting; }
	void SetDirShooting( Vector value ) { m_info->m_vecDirShooting = value; }
	Vector GetSpread() { return m_info->m_vecSpread; }
	void SetSpread( Vector value ) { m_info->m_vecSpread = value; }

	float GetDistance() { return m_info->m_flDistance; }
	void SetDistance( float value ) { m_info->m_flDistance = value; }

	int GetAmmoType() { return m_info->m_iAmmoType; }
	void SetAmmoType( int value ) { m_info->m_iAmmoType = value; }

	int GetTracerFreq() { return m_info->m_iTracerFreq; }
	void SetTracerFreq( int value ) { m_info->m_iTracerFreq = value; }

	float GetDamage() { return m_info->m_flDamage; }
	void SetDamage( float value ) { m_info->m_flDamage = value; }
	int GetPlayerDamage() { return m_info->m_iPlayerDamage; }
	void SetPlayerDamage( float value ) { m_info->m_iPlayerDamage = value; }

	int GetFlags() { return m_info->m_nFlags; }
	void SetFlags( float value ) { m_info->m_nFlags = value; }

	float GetDamageForceScale() { return m_info->m_flDamageForceScale; }
	void SetDamageForceScale( float value ) { m_info->m_flDamageForceScale = value; }

	HSCRIPT GetAttacker();
	void SetAttacker( HSCRIPT value );
	HSCRIPT GetAdditionalIgnoreEnt();
	void SetAdditionalIgnoreEnt( HSCRIPT value );

	bool GetPrimaryAttack() { return m_info->m_bPrimaryAttack; }
	void SetPrimaryAttack( bool value ) { m_info->m_bPrimaryAttack = value; }

	FireBulletsInfo_t *GetInfo() { return m_info; }

	void Destroy() { delete m_info; delete this; }

private:
	FireBulletsInfo_t *m_info;
};

//-----------------------------------------------------------------------------
// Exposes CUserCmd to VScript
//-----------------------------------------------------------------------------
class CUserCmdAccessor
{
public:
	CUserCmdAccessor( CUserCmd *cmd ) { m_cmd = cmd; }

	int GetCommandNumber() { return m_cmd->command_number; }

	int ScriptGetTickCount() { return m_cmd->tick_count; }

	const QAngle& GetViewAngles() { return m_cmd->viewangles; }
	void SetViewAngles( const QAngle& val ) { m_cmd->viewangles = val; }

	float GetForwardMove() { return m_cmd->forwardmove; }
	void SetForwardMove( float val ) { m_cmd->forwardmove = val; }
	float GetSideMove() { return m_cmd->sidemove; }
	void SetSideMove( float val ) { m_cmd->sidemove = val; }
	float GetUpMove() { return m_cmd->upmove; }
	void SetUpMove( float val ) { m_cmd->upmove = val; }

	int GetButtons() { return m_cmd->buttons; }
	void SetButtons( int val ) { m_cmd->buttons = val; }
	int GetImpulse() { return m_cmd->impulse; }
	void SetImpulse( int val ) { m_cmd->impulse = val; }

	int GetWeaponSelect() { return m_cmd->weaponselect; }
	void SetWeaponSelect( int val ) { m_cmd->weaponselect = val; }
	int GetWeaponSubtype() { return m_cmd->weaponsubtype; }
	void SetWeaponSubtype( int val ) { m_cmd->weaponsubtype = val; }

	int GetRandomSeed() { return m_cmd->random_seed; }

	int GetMouseX() { return m_cmd->mousedx; }
	void SetMouseX( int val ) { m_cmd->mousedx = val; }
	int GetMouseY() { return m_cmd->mousedy; }
	void SetMouseY( int val ) { m_cmd->mousedy = val; }

private:
	CUserCmd *m_cmd;
};

#endif
