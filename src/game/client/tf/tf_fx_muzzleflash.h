//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_FX_MUZZLEFLASH_H
#define TF_FX_MUZZLEFLASH_H

#include "particles_simple.h"
#include "particles_localspace.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "c_baseanimating.h"

class CMuzzleFlashEmitter_1stPerson : public CLocalSpaceEmitter
{
public:
	DECLARE_CLASS( CMuzzleFlashEmitter_1stPerson, CLocalSpaceEmitter );

	static CSmartPtr<CMuzzleFlashEmitter_1stPerson> Create( const char *pDebugName, int entIndex, int nAttachment, int fFlags = 0 );

	virtual void Update( float t ) { BaseClass::Update(t); }

protected:
	CMuzzleFlashEmitter_1stPerson( const char *pDebugName );

private:
	CMuzzleFlashEmitter_1stPerson( const CMuzzleFlashEmitter_1stPerson & );

	int m_iMuzzleFlashType;
};

// Model versions of muzzle flashes
class C_MuzzleFlashModel : public C_BaseAnimating
{
	DECLARE_CLASS( C_MuzzleFlashModel, C_BaseAnimating );
public:
	static C_MuzzleFlashModel *CreateMuzzleFlashModel( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime = 0.2 );
	bool	InitializeMuzzleFlash( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime );
	void	ClientThink( void );

	void	SetLifetime( float flLifetime );

	// Recording
	virtual void GetToolRecordingState( KeyValues *msg );
	virtual bool SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );

	void	SetIs3rdPersonFlash( bool bEnable );

private:
	float	m_flExpiresAt;
	float	m_flRotateAt;
	bool	m_bIs3rdPersonFlash;
};

#endif //TF_FX_MUZZLEFLASH_H
