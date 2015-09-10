//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef MDLPANEL_H
#define MDLPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "vgui_controls/Panel.h"
#include "datacache/imdlcache.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "matsys_controls/potterywheelpanel.h"
#include "tier3/mdlutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class IScheme;
}

// 
struct MDLAnimEventState_t
{
	int		m_nEventSequence;
	float	m_flPrevEventCycle;
};

//-----------------------------------------------------------------------------
// MDL Viewer Panel
//-----------------------------------------------------------------------------
class CMDLPanel : public CPotteryWheelPanel
{
	DECLARE_CLASS_SIMPLE( CMDLPanel, CPotteryWheelPanel );

public:
	// constructor, destructor
	CMDLPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CMDLPanel();

	// Overriden methods of vgui::Panel
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnTick();

	virtual void Paint();

	// Sets the current mdl
	virtual void SetMDL( MDLHandle_t handle, void *pProxyData = NULL );
	virtual void SetMDL( const char *pMDLName, void *pProxyData = NULL );

	// Sets the camera to look at the model
	void LookAtMDL( );

	// Sets the current LOD
	void SetLOD( int nLOD );

	// Sets the current sequence
	void SetSequence( int nSequence, bool bResetSequence = false );

	// Set the pose parameters
	void SetPoseParameters( const float *pPoseParameters, int nCount );
	bool SetPoseParameterByName( const char *pszName, float fValue );

	// Set the overlay sequence layers
	void SetSequenceLayers( const MDLSquenceLayer_t *pSequenceLayers, int nCount );

	void SetCollsionModel( bool bVisible );
	void SetGroundGrid( bool bVisible );
	void SetWireFrame( bool bVisible );
	void SetLockView( bool bLocked );
	void SetSkin( int nSkin );
	void SetLookAtCamera( bool bLookAtCamera );
	void SetIgnoreDoubleClick( bool bState );
	void SetThumbnailSafeZone( bool bVisible );

	// Bounds.
	bool GetBoundingBox( Vector &vecBoundsMin, Vector &vecBoundsMax );
	bool GetBoundingSphere( Vector &vecCenter, float &flRadius );

	virtual void SetModelAnglesAndPosition( const QAngle &angRot, const Vector &vecPos );

	// Attached models.
	void	SetMergeMDL( MDLHandle_t handle, void *pProxyData = NULL, int nSkin = -1 );
	MDLHandle_t SetMergeMDL( const char *pMDLName, void *pProxyData = NULL, int nSkin = -1 );
	int		GetMergeMDLIndex( void *pProxyData );
	int		GetMergeMDLIndex( MDLHandle_t handle );
	CMDL	*GetMergeMDL(MDLHandle_t handle );
	void	ClearMergeMDLs( void );

	virtual void	SetupFlexWeights( void ) { return; }

	// Events
	void DoAnimationEvents();
	void DoAnimationEvents( CStudioHdr *pStudioHdr, int nSeqNum, float flTime, bool bNoLoop, MDLAnimEventState_t *pEventState );
	virtual void FireEvent( const char *pszEventName, const char *pszEventOptions ) { }
	void ResetAnimationEventState( MDLAnimEventState_t *pEventState );

protected:

	virtual void SetupRenderState( int nDisplayWidth, int nDisplayHeight ) OVERRIDE;

	struct MDLData_t
	{
		CMDL		m_MDL;
		matrix3x4_t	m_MDLToWorld;
		bool		m_bDisabled;
		float		m_flCycleStartTime;
	};

	MDLData_t				m_RootMDL;
	CUtlVector<MDLData_t>	m_aMergeMDLs;

	static const int MAX_SEQUENCE_LAYERS = 8;
	int					m_nNumSequenceLayers;
	MDLSquenceLayer_t	m_SequenceLayers[ MAX_SEQUENCE_LAYERS ];

	MDLAnimEventState_t	m_EventState;
	MDLAnimEventState_t	m_SequenceLayerEventState[ MAX_SEQUENCE_LAYERS ];

private:
	// paint it!
	virtual void OnPaint3D();
	virtual void PrePaint3D( IMatRenderContext *pRenderContext ) { };
	virtual void PostPaint3D( IMatRenderContext *pRenderContext ) { };
	virtual void RenderingRootModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix ) { };
	virtual void RenderingMergedModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix ) { };

	void OnMouseDoublePressed( vgui::MouseCode code );

	void DrawCollisionModel();
	void UpdateStudioRenderConfig( void );

	CTextureReference m_DefaultEnvCubemap;
	CTextureReference m_DefaultHDREnvCubemap;

	bool	m_bDrawCollisionModel : 1;
	bool	m_bGroundGrid : 1;
	bool	m_bLockView : 1;
	bool	m_bWireFrame : 1;
	bool	m_bLookAtCamera : 1;
	bool	m_bIgnoreDoubleClick : 1;
	bool	m_bThumbnailSafeZone : 1;

	float	m_PoseParameters[ MAXSTUDIOPOSEPARAM ];
};


#endif // MDLPANEL_H
