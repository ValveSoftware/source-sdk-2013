//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PLAYERMODELPANEL_H
#define TF_PLAYERMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"
#include "ichoreoeventcallback.h"

class CChoreoScene;

extern CMouthInfo g_ClientUIMouth;

// A model panel that knows how to imitate a TF2 player, including wielding/wearing unlockable items.
class CTFPlayerModelPanel : public CBaseModelPanel, public IChoreoEventCallback, public IHasLocalToGlobalFlexSettings, public IModelLoadCallback
{
	DECLARE_CLASS_SIMPLE( CTFPlayerModelPanel, CBaseModelPanel );
public:
	CTFPlayerModelPanel( vgui::Panel *pParent, const char *pName );
	~CTFPlayerModelPanel( void );

	void	ApplySettings( KeyValues *inResourceData );

	void	SetToPlayerClass( int iClass, bool bForceRefresh = false, const char *pszPlayerModelOverride = NULL );
	bool	HoldItemInSlot( int iSlot );
	bool	HoldItem( int iItemNumber );
	void	SwitchHeldItemTo( CEconItemView *pItem );
	void	EquipRequiredLoadoutSlot( int iRequiredLoadoutSlot );
	CEconItemView	*GetHeldItem() { return m_pHeldItem; }

	int		AddCarriedItem( CEconItemView *pItem );
	void	ClearCarriedItems( void );

	void	PlayVCD( const char *pszVCD, const char *pszWeaponEntityRequired = NULL, bool bLoopVCD = true, bool bFileNameOnly = true );

	// Handle animation events
	virtual void FireEvent( const char *pszEventName, const char *pszEventOptions );

	const CUtlVector<CEconItemView*> &GetCarriedItems() { return m_ItemsToCarry; }
	int		GetNumCarriedItems() const { return m_ItemsToCarry.Count(); }
	int		GetPlayerClass() const	{ return m_iCurrentClassIndex; }
	Vector	GetZoomOffset();

	void	ToggleZoom();
	bool	IsZoomed()		{ return m_bZoomedToHead; }

	void	SetTeam( int iTeam );
	int		GetTeam( void ) { return m_iTeam; }

	void	UpdatePreviewVisuals( void );

	// From IChoreoEventCallback
	virtual void	StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool	CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	SetupFlexWeights( void );

	// IHasLocalToGlobalFlexSettings
	virtual void	EnsureTranslations( const flexsettinghdr_t *pSettinghdr );
	int				FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );

	// IModelLoadCallback
	virtual void	OnModelLoadComplete( const model_t *pModel );

	void			SetEyeGlowEffect ( const char *pEffectName, Vector vColor1, Vector vColor2, bool bForceUpdate, bool bPlaySparks );

	void	InvalidateParticleEffects();

protected:
	// From CBaseModelPanel
	virtual void	PrePaint3D( IMatRenderContext *pRenderContext ) OVERRIDE;
	virtual void	PostPaint3D( IMatRenderContext *pRenderContext ) OVERRIDE;
	virtual void	RenderingRootModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix );
	virtual void	RenderingMergedModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix );
	virtual IMaterial* GetOverrideMaterial( MDLHandle_t mdlHandle ) OVERRIDE;

private:

	enum modelpanel_particle_system_t
	{
		SYSTEM_HEAD = 0,
		SYSTEM_MISC1,
		SYSTEM_MISC2,
		SYSTEM_WEAPON, // there can only be one weapon equipped
		SYSTEM_ACTIONSLOT,
		SYSTEM_EYEGLOW_LEFT,
		SYSTEM_EYEGLOW_RIGHT,
		SYSTEM_EYESPARK_LEFT,
		SYSTEM_EYESPARK_RIGHT,
		SYSTEM_TAUNT,
		SYSTEM_COUNT,
	};

	// Choreo Scene handling
	void	ClearScene( void );
	void	ProcessSequence( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessExpression( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	void	AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr );
	void	ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event );
	void	SetFlexWeight( LocalFlexController_t index, float value );
	float	GetFlexWeight( LocalFlexController_t index );

	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexDescFacs( int iFlexDesc );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );
	const char *GetFlexControllerType( LocalFlexController_t iFlexController );
	LocalFlexController_t		FindFlexController( const char *szName );

	// Mouth processing
	CMouthInfo&	MouthInfo() { return g_ClientUIMouth; }
	void	ProcessVisemes( Emphasized_Phoneme *classes );
	void	AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted );
	void	AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression );
	bool	SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme );
	void	ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity );
	void	InitPhonemeMappings( void );
	void	SetupMappings( char const *pchFileRoot );

	void	HoldFirstValidItem( void );
	void	EquipAllWearables( CEconItemView *pHeldItem );
	void	EquipItem( CEconItemView *pItem );
	bool	UpdateHeldItem( int iDesiredSlot );
	void	UpdateWeaponBodygroups( bool bModifyDeployedOnlyBodygroups );
	void	UpdateHiddenBodyGroups( CEconItemView* pItem );
	CEconItemView *GetItemInSlot( int iSlot );
	CEconItemView *GetPreviewItem( CEconItemView *pMatchItem );

	// Use this instead of SetMergeModel() - handles dynamic asset allocation
	void	LoadAndAttachAdditionalModel( const char *pMDLName, CEconItemView *pItem );
	bool	FinishAttachAdditionalModel( const model_t *pModel );
	void	RemoveAdditionalModels( void );

	bool	UpdateCosmeticParticles( 
		IMatRenderContext				*pRenderContext, 
		CStudioHdr						*pStudioHdr, 
		MDLHandle_t						mdlHandle, 
		matrix3x4_t						*pWorldMatrix, 
		modelpanel_particle_system_t	iSystem,
		CEconItemView					*pEconItem
	);

	void	UpdateEyeGlows( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix, bool bIsRightEye );
	void	UpdateActionSlotEffects( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix );
	void	UpdateTauntEffects( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix );

	int				m_iCurrentClassIndex;
	int				m_iCurrentSlotIndex;
	CUtlVector<CEconItemView*>	m_ItemsToCarry;		// Items that our player should be seen carrying
	QAngle			m_angPlayerOrg;

	int				m_nBody;
	int				m_iTeam;
	bool			m_bZoomedToHead;

	MDLHandle_t		m_MergeMDL;

	CEconItemView	*m_pHeldItem;

	const char		*m_pszVCD;
	const char		*m_pszWeaponEntityRequired;
	bool			m_bLoopVCD;
	bool			m_bVCDFileNameOnly;

	CChoreoScene	*m_pScene;
	float			m_flSceneTime;
	float			m_flSceneEndTime;
	float			m_flLastTickTime;
	bool			m_bLoopScene;

	CUtlVector< CRefCountedModelIndex > m_vecDynamicAssetsLoaded;
	CUtlVector< CEconItemView* > m_vecItemsLoaded;

	struct CustomClassData_t
	{
		float m_flFOV;
		Vector m_vPosition;
		QAngle m_vAngles;
	};
	CUtlVector< CustomClassData_t > m_customClassData;

	// Choreo scenes
	bool				m_bShouldRunFlexEvents;
	float				m_flexWeight[ MAXSTUDIOFLEXCTRL ];
	Emphasized_Phoneme	m_PhonemeClasses[ NUM_PHONEME_CLASSES ];
	CUtlRBTree< FS_LocalToGlobal_t, unsigned short > m_LocalToGlobal;

	// The particle system to draw
	particle_data_t *m_aParticleSystems[ SYSTEM_COUNT ];

	bool m_bUpdateEyeGlows;
	bool m_bPlaySparks;

	char m_pszEyeGlowParticleName[MAX_PATH];
	Vector m_vEyeGlowColor1;
	Vector m_vEyeGlowColor2;
	bool m_bDrawActionSlotEffects;
	bool m_bDrawTauntParticles;

	float m_flTauntParticleRefireTime;
	float m_flTauntParticleRefireRate;

	CUtlString m_strPlayerModelOverride;

	CPanelAnimationVar( bool, m_bDisableSpeakEvent, "disable_speak_event", "0" );

	CEconItemView			*GetLoadoutItemFromMDLHandle( loadout_positions_t iPosition, MDLHandle_t mdlHandle );
	bool					RenderStatTrack( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix );
	MDLData_t				m_StatTrackModel;
	float					m_flStatTrackScale;
};

#endif // TF_PLAYERMODELPANEL_H
