//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_VIEWMODEL_H
#define TF_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#include "c_baseanimating.h"
#define CTFViewModel C_TFViewModel
#endif

class CTFViewModel : public CBaseViewModel
{
	DECLARE_CLASS( CTFViewModel, CBaseViewModel );
public:

	DECLARE_NETWORKCLASS();

	CTFViewModel( void );
	virtual ~CTFViewModel( void );

	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );
	virtual void AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles );

#if defined( CLIENT_DLL )
	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual void StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	virtual void ProcessMuzzleFlashEvent( void );

	virtual int GetSkin();
	BobState_t	&GetBobState() { return m_BobState; }

	virtual int DrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo ) OVERRIDE;
	virtual bool OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	virtual const char* ModifyEventParticles( const char* token );
#endif

	bool m_bBodygroupsDirty;

private:
	void RecalculatePlayerBodygroups();

#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;
	BobState_t		m_BobState;		// view model head bob state

	CTFViewModel( const CTFViewModel & ); // not defined, not accessible

	QAngle m_vLoweredWeaponOffset;

#endif
};

#endif // TF_VIEWMODEL_H
