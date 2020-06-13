//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SHARED_H
#define VSCRIPT_SHARED_H

#include "vscript/ivscript.h"

#if defined( _WIN32 )
#pragma once
#endif

extern IScriptVM * g_pScriptVM;

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing = false );
bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );
inline bool VScriptRunScript( const char *pszScriptName, bool bWarnMissing = false ) { return VScriptRunScript( pszScriptName, NULL, bWarnMissing ); }

#define DECLARE_ENT_SCRIPTDESC()													ALLOW_SCRIPT_ACCESS(); virtual ScriptClassDesc_t *GetScriptDesc()

#define BEGIN_ENT_SCRIPTDESC( className, baseClass, description )					_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC( className, baseClass, description )
#define BEGIN_ENT_SCRIPTDESC_ROOT( className, description )							_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_ROOT( className, description )
#define BEGIN_ENT_SCRIPTDESC_NAMED( className, baseClass, scriptName, description )	_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_NAMED( className, baseClass, scriptName, description )
#define BEGIN_ENT_SCRIPTDESC_ROOT_NAMED( className, scriptName, description )		_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_ROOT_NAMED( className, scriptName, description )

#define _IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className )					template <> ScriptClassDesc_t * GetScriptDesc<className>( className * ); ScriptClassDesc_t *className::GetScriptDesc()  { return ::GetScriptDesc( this ); }		

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

#ifdef MAPBASE_VSCRIPT
void RegisterSharedScriptFunctions();

// 
// Exposes FireBulletsInfo_t to VScript
// 
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

HSCRIPT ScriptCreateMatrixInstance( matrix3x4_t &matrix );
#endif

#endif // VSCRIPT_SHARED_H
