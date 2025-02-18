//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ECON_ENTITY_H
#define ECON_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "ihasattributes.h"
#include "ihasowner.h"
#include "attribute_manager.h"
#include "econ_item_view.h"

#if defined( CLIENT_DLL )
#define CEconEntity				C_EconEntity
#define CBaseAttributableItem	C_BaseAttributableItem

// Additional attachments.
struct AttachedModelData_t
{
	const model_t *m_pModel;
	int m_iModelDisplayFlags;
};

#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconEntity : public CBaseAnimating, public IHasAttributes
{
	DECLARE_CLASS( CEconEntity, CBaseAnimating );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
#ifdef GAME_DLL
	DECLARE_ENT_SCRIPTDESC();
#endif
	CEconEntity();
	~CEconEntity();

	void					InitializeAttributes( void );
	void					DebugDescribe( void );
	Activity				TranslateViewmodelHandActivity( Activity actBase );
	virtual void			UpdateOnRemove( void );

	virtual CStudioHdr *	OnNewModel();

#if !defined( CLIENT_DLL )
	virtual void 			GiveTo( CBaseEntity *pOther ) {}
	void					OnOwnerClassChange( void );
	void					UpdateModelToClass( void );
	void					PlayAnimForPlaybackEvent( wearableanimplayback_t iPlayback );
	virtual int				CalculateVisibleClassFor( CBaseCombatCharacter *pPlayer );

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	void					MarkAttachedEntityAsValidated() { m_bValidatedAttachedEntity = true; }
#endif // TF_DLL || TF_CLIENT_DLL

#else
	enum ParticleSystemState_t
	{
		PARTICLE_SYSTEM_STATE_NOT_VISIBLE, 
		PARTICLE_SYSTEM_STATE_VISIBLE,
		PARTICLE_SYSTEM_STATE_VISIBLE_VM
	};

	virtual void			Release();
	virtual void			SetDormant( bool bDormant );
	virtual void			OnPreDataChanged( DataUpdateType_t type );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual bool			ShouldShowToolTip( void	) { return true; }
	virtual bool			InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );
	virtual bool			OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual IMaterial		*GetEconWeaponMaterialOverride( int iTeam ) OVERRIDE;
	virtual void			FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual bool			OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	bool					InternalFireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	// Custom flex controllers
	virtual bool			UsesFlexDelayedWeights( void );
	virtual	void			SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	float					m_flFlexDelayTime;
	float *					m_flFlexDelayedWeight;
	int						m_cFlexDelayedWeight;

	// Custom particle attachments
	bool					HasCustomParticleSystems( void ) const;
	void					UpdateParticleSystems( void );
	virtual bool			ShouldDrawParticleSystems( void );
	void					SetParticleSystemsVisible( ParticleSystemState_t bVisible );
	void					UpdateSingleParticleSystem( bool bVisible, const attachedparticlesystem_t *pSystem );
	virtual void			UpdateAttachmentModels( void );
	virtual bool			AttachmentModelsShouldBeVisible( void ) { return true; }
	void					GetEconParticleSystems( CUtlVector<const attachedparticlesystem_t *> *out_pvecParticleSystems ) const;

	// Model swaping
	bool					ShouldDraw( void );
	bool					ShouldHideForVisionFilterFlags( void );

	virtual bool			IsTransparent( void ) OVERRIDE;

	// Viewmodel overriding
	virtual bool			ViewModel_IsTransparent( void );
	virtual bool			ViewModel_IsUsingFBTexture( void );
	virtual bool			IsOverridingViewmodel( void );
	virtual int				DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );

	// Attachments
	bool					WantsToOverrideViewmodelAttachments( void ) { return (m_hViewmodelAttachment != NULL); }
	virtual int				LookupAttachment( const char *pAttachmentName );
	virtual bool			GetAttachment( const char *szName, Vector &absOrigin ) { return BaseClass::GetAttachment(szName,absOrigin); }
	virtual bool			GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles ) { return BaseClass::GetAttachment(szName,absOrigin,absAngles); }
	virtual bool			GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool			GetAttachment( int number, Vector &origin );
	virtual	bool			GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool			GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );

	C_BaseAnimating			*GetViewmodelAttachment( void ) { return m_hViewmodelAttachment.Get(); }
	virtual void			ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask ) {}

	void					SetWaitingToLoad( bool bWaiting );

	virtual bool			ValidateEntityAttachedToPlayer( bool &bShouldRetry );

	virtual void			SetMaterialOverride( int team, const char *pszMaterial );
	virtual void			SetMaterialOverride( int team, CMaterialReference &ref );

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

#endif

public:
	// IHasAttributes
	CAttributeManager			*GetAttributeManager( void ) { return &m_AttributeManager; }
	CAttributeContainer			*GetAttributeContainer( void ) { return &m_AttributeManager; }
	const CAttributeContainer	*GetAttributeContainer( void ) const { return &m_AttributeManager; }
	CBaseEntity					*GetAttributeOwner( void ) { return GetOwnerEntity(); }
	CAttributeList				*GetAttributeList( void ) { return m_AttributeManager.GetItem()->GetAttributeList(); }
	virtual void				ReapplyProvision( void );
	float						ScriptGetAttribute( const char *pName, float flFallbackValue );

	void AddAttribute( const char *pszAttributeName, float flVal, float flDuration )
	{
		const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttributeName );
		if ( !pDef )
			return;

		GetAttributeList()->SetRuntimeAttributeValue( pDef, flVal );
		GetAttributeManager()->OnAttributeValuesChanged();
	}

	void RemoveAttribute( const char* pszAttribName )
	{
		const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );
		if ( !pDef )
			return;

		GetAttributeList()->RemoveAttribute( pDef );
		GetAttributeManager()->OnAttributeValuesChanged();
	}

	virtual bool			UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState );

protected:
	virtual Activity		TranslateViewmodelHandActivityInternal( Activity actBase ) { return actBase; }
	
protected:
	CNetworkVarEmbedded(	CAttributeContainer, m_AttributeManager );

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	CNetworkVar( bool, m_bValidatedAttachedEntity );
#endif // TF_DLL || TF_CLIENT_DLL

#ifdef CLIENT_DLL
	bool					m_bClientside;
	ParticleSystemState_t		m_nParticleSystemsCreated;
	CMaterialReference		m_MaterialOverrides[TEAM_VISUAL_SECTIONS];
	CHandle<C_BaseAnimating>	m_hViewmodelAttachment;
	int						m_iOldTeam;
	bool					m_bAttachmentDirty;
	int						m_nUnloadedModelIndex;
	int						m_iNumOwnerValidationRetries;
#endif

	bool					m_bHasParticleSystems;
	EHANDLE					m_hOldProvidee;

#ifdef GAME_DLL
	int						m_iOldOwnerClass; // Used to detect class changes on items that have per-class models
#endif

protected:
#ifdef CLIENT_DLL

public:

	CUtlVector<AttachedModelData_t> m_vecAttachedModels;

#endif // CLIENT_DLL
};

#define ITEM_PICKUP_BOX_BLOAT		24

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseAttributableItem : public CEconEntity
{
	DECLARE_CLASS( CBaseAttributableItem, CEconEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CBaseAttributableItem();
};

#if defined( CLIENT_DLL )
#ifndef DOTA_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_ViewmodelAttachmentModel : public C_BaseAnimating, public IHasOwner
{
	DECLARE_CLASS( C_ViewmodelAttachmentModel, C_BaseAnimating );
public:
	void SetOuter( CEconEntity *pOuter );
	CHandle<CEconEntity> GetOuter( void ) { return m_hOuter; }
	bool InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );
	int  InternalDrawModel( int flags );
	bool OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	
	virtual CBaseEntity	*GetOwnerViaInterface( void ) { return GetOuter()->GetAttributeOwner(); }

	virtual void FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld );

	virtual int GetSkin( void );

private:
	CHandle<CEconEntity>  m_hOuter;
	bool							m_bAlwaysFlip;
};
#endif // !defined( DOTA_DLL )
#endif // defined( CLIENT_DLL )

#endif // ECON_ENTITY_H
