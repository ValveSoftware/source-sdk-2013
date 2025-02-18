//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEVIEWMODEL_SHARED_H
#define BASEVIEWMODEL_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "econ/ihasowner.h"

class CBaseCombatWeapon;
class CBaseCombatCharacter;
class CVGuiScreen;

#if defined( CLIENT_DLL )
#define CBaseViewModel C_BaseViewModel
#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

#define VIEWMODEL_INDEX_BITS 1

class CBaseViewModel : public CBaseAnimating, public IHasOwner
{
	DECLARE_CLASS( CBaseViewModel, CBaseAnimating );
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

							CBaseViewModel( void );
							~CBaseViewModel( void );


	bool IsViewable(void) { return false; }

	virtual void					UpdateOnRemove( void );

	// Weapon client handling
	virtual void			SendViewModelMatchingSequence( int sequence );
	virtual void			SetWeaponModel( const char *pszModelname, CBaseCombatWeapon *weapon );

	virtual void			CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void			CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, 
								const QAngle& eyeAngles );
	virtual void			AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles ) {};

	// Initializes the viewmodel for use							
	void					SetOwner( CBaseEntity *pEntity );
	void					SetIndex( int nIndex );
	// Returns which viewmodel it is
	int						ViewModelIndex( ) const;

	virtual void			Precache( void );

	virtual void			Spawn( void );

	virtual CBaseEntity *GetOwner( void ) { return m_hOwner; };

	virtual void			AddEffects( int nEffects );
	virtual void			RemoveEffects( int nEffects );

	void					SpawnControlPanels();
	void					DestroyControlPanels();
	void					SetControlPanelsActive( bool bState );
	void					ShowControlPanells( bool show );

	virtual CBaseCombatWeapon *GetOwningWeapon( void );
	
	virtual CBaseEntity	*GetOwnerViaInterface( void ) { return GetOwner(); }

	virtual bool			IsSelfAnimating()
	{
		return true;
	}

	Vector					m_vecLastFacing;

	// Only support prediction in TF2 for now
#if defined( INVASION_DLL ) || defined( INVASION_CLIENT_DLL )
	// All predicted weapons need to implement and return true
	virtual bool			IsPredicted( void ) const
	{ 
		return true;
	}
#endif

#if !defined( CLIENT_DLL )
	virtual int				UpdateTransmitState( void );
	virtual int				ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void			SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
#else

	virtual RenderGroup_t	GetRenderGroup();

// Only supported in TF2 right now
#if defined( INVASION_CLIENT_DLL )

	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

#endif


	virtual void			FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual void			PostDataUpdate( DataUpdateType_t updateType );

	virtual bool			Interpolate( float currentTime );

	virtual bool			ShouldFlipViewModel() OVERRIDE;
	void					UpdateAnimationParity( void );

	virtual void			ApplyBoneMatrixTransform( matrix3x4_t& transform );

	virtual bool			ShouldDraw();
	virtual int				DrawModel( int flags );
	virtual int				InternalDrawModel( int flags );
	int						DrawOverriddenViewmodel( int flags );
	virtual int				GetFxBlend( void );
	virtual bool			IsTransparent( void );
	virtual bool			UsesPowerOfTwoFrameBufferTexture( void );
	
	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }

	// Should this object receive shadows?
	virtual bool			ShouldReceiveProjectedTextures( int flags )
	{
		return false;
	}

	// Add entity to visible view models list?
	virtual void			AddEntity( void );

	virtual void			GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);

	// See C_StudioModel's definition of this.
	virtual void			UncorrectViewModelAttachment( Vector &vOrigin );

	// (inherited from C_BaseAnimating)
	virtual void			FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld );
	virtual bool			IsViewModel() const;
	
	CBaseCombatWeapon		*GetWeapon() const { return m_hWeapon.Get(); }

#ifdef CLIENT_DLL
	virtual bool			ShouldResetSequenceOnNewModel( void ) { return false; }

	// Attachments
	virtual int				LookupAttachment( const char *pAttachmentName );
	virtual bool			GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool			GetAttachment( int number, Vector &origin );
	virtual	bool			GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool			GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );
#endif

private:
	CBaseViewModel( const CBaseViewModel & ); // not defined, not accessible

#endif

private:
	CNetworkVar( int, m_nViewModelIndex );		// Which viewmodel is it?
	CNetworkHandle( CBaseEntity, m_hOwner );				// Player or AI carrying this weapon

	// soonest time Update will call WeaponIdle
	float					m_flTimeWeaponIdle;							

	Activity				m_Activity;

	// Used to force restart on client, only needs a few bits
	CNetworkVar( int, m_nAnimationParity );

	// Weapon art
	string_t				m_sVMName;			// View model of this weapon
	string_t				m_sAnimationPrefix;		// Prefix of the animations that should be used by the player carrying this weapon

#if defined( CLIENT_DLL )
	int						m_nOldAnimationParity;
#endif


	typedef CHandle< CBaseCombatWeapon > CBaseCombatWeaponHandle;
	CNetworkVar( CBaseCombatWeaponHandle, m_hWeapon );

	// Control panel
	typedef CHandle<CVGuiScreen>	ScreenHandle_t;
	CUtlVector<ScreenHandle_t>	m_hScreens;
};

#endif // BASEVIEWMODEL_SHARED_H
