//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "basemodel_panel.h"
#include "activitylist.h"
#include "animation.h"
#include "vgui/IInput.h"
#include "matsys_controls/manipulator.h"
#include "bone_setup.h"

using namespace vgui;
extern float GetAutoPlayTime( void );
DECLARE_BUILD_FACTORY( CBaseModelPanel );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseModelPanel::CBaseModelPanel( vgui::Panel *pParent, const char *pName )
	: BaseClass( pParent, pName )
	, m_nActiveSequence( ACT_INVALID )
	, m_flActiveSequenceDuration( 0.f )
{
	m_bForcePos = false;
	m_bMousePressed = false;
	m_bAllowRotation = false;
	m_bAllowPitch = false;
	m_bAllowFullManipulation = false;
	m_bApplyManipulators = false;
	m_bForcedCameraPosition = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseModelPanel::~CBaseModelPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: Load in the model portion of the panel's resource file.
//-----------------------------------------------------------------------------
void CBaseModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Set whether we render to texture
	m_bRenderToTexture = inResourceData->GetBool( "render_texture", true );
	m_bUseParticle = inResourceData->GetBool( "use_particle", false );

	// Grab and set the camera FOV.
	float flFOV = GetCameraFOV();
	m_BMPResData.m_flFOV = inResourceData->GetInt( "fov", flFOV );
	SetCameraFOV( m_BMPResData.m_flFOV );

	// Do we allow rotation on these panels.
	m_bAllowRotation = inResourceData->GetBool( "allow_rot", false );
	m_bAllowPitch = inResourceData->GetBool( "allow_pitch", false );

	// Do we allow full manipulation on these panels.
	m_bAllowFullManipulation = inResourceData->GetBool( "allow_manip", false );

	// Continued velocity after the user releases the mouse after a manipulation
	m_bUseVelocity = inResourceData->GetBool( "continued_velocity", true );
	// Don't use velocity if full manipulation is on.  It breaks.
	m_bUseVelocity &= !m_bAllowFullManipulation;
	m_flYawVelocityDecay  = inResourceData->GetFloat( "yaw_velocity_decay", 12.f );
	m_flPitchVelocityDecay  = inResourceData->GetFloat( "pitch_velocity_decay", 12.f );

	// Parse our resource file and apply all necessary updates to the MDL.
 	for ( KeyValues *pData = inResourceData->GetFirstSubKey() ; pData != NULL ; pData = pData->GetNextKey() )
 	{
 		if ( !Q_stricmp( pData->GetName(), "model" ) )
 		{
 			ParseModelResInfo( pData );
 		}
 	}

	SetMouseInputEnabled( m_bAllowFullManipulation || m_bAllowRotation || m_bAllowPitch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::ParseModelResInfo( KeyValues *inResourceData )
{
	m_bForcePos = ( inResourceData->GetInt( "force_pos", 0 ) == 1 );
	m_BMPResData.m_pszModelName = ReadAndAllocStringValue( inResourceData, "modelname" );
	m_BMPResData.m_pszModelName_HWM = ReadAndAllocStringValue( inResourceData, "modelname_hwm" );
	m_BMPResData.m_pszVCD = ReadAndAllocStringValue( inResourceData, "vcd" );
	m_BMPResData.m_angModelPoseRot.Init( inResourceData->GetFloat( "angles_x", 0.0f ), inResourceData->GetFloat( "angles_y", 0.0f ), inResourceData->GetFloat( "angles_z", 0.0f ) );
	m_BMPResData.m_vecOriginOffset.Init( inResourceData->GetFloat( "origin_x", 110.0 ), inResourceData->GetFloat( "origin_y", 5.0 ), inResourceData->GetFloat( "origin_z", 5.0 ) );
	m_BMPResData.m_vecFramedOriginOffset.Init( inResourceData->GetFloat( "frame_origin_x", 110.0 ), inResourceData->GetFloat( "frame_origin_y", 5.0 ), inResourceData->GetFloat( "frame_origin_z", 5.0 ) );
	m_BMPResData.m_vecViewportOffset.Init();
	m_BMPResData.m_nSkin = inResourceData->GetInt( "skin", -1 );
	m_BMPResData.m_bUseSpotlight = ( inResourceData->GetInt( "spotlight", 0 ) == 1 );

	m_angPlayer = m_BMPResData.m_angModelPoseRot;
	m_vecPlayerPos = m_BMPResData.m_vecOriginOffset;

	for ( KeyValues *pData = inResourceData->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		if ( !Q_stricmp( pData->GetName(), "animation" ) )
		{
			ParseModelAnimInfo( pData );
		}
		else if ( !Q_stricmp( pData->GetName(), "attached_model" ) )
		{
			ParseModelAttachInfo( pData );
		}
	}

	SetupModelDefaults();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::ParseModelAnimInfo( KeyValues *inResourceData )
{
	if ( !inResourceData )
		return;

	int iAnim = m_BMPResData.m_aAnimations.AddToTail();
	if ( iAnim == m_BMPResData.m_aAnimations.InvalidIndex() )
		return;

	m_BMPResData.m_aAnimations[iAnim].m_pszName = ReadAndAllocStringValue( inResourceData, "name" );
	m_BMPResData.m_aAnimations[iAnim].m_pszSequence = ReadAndAllocStringValue( inResourceData, "sequence" );
	m_BMPResData.m_aAnimations[iAnim].m_pszActivity = ReadAndAllocStringValue( inResourceData, "activity" );
	m_BMPResData.m_aAnimations[iAnim].m_bDefault = inResourceData->GetBool( "default" );

	for ( KeyValues *pAnimData = inResourceData->GetFirstSubKey(); pAnimData != NULL; pAnimData = pAnimData->GetNextKey() )
	{
		if ( !Q_stricmp( pAnimData->GetName(), "pose_parameters" ) )
		{
			m_BMPResData.m_aAnimations[iAnim].m_pPoseParameters = pAnimData->MakeCopy();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::ParseModelAttachInfo( KeyValues *inResourceData )
{
	if ( !inResourceData )
		return;

	int iAttach = m_BMPResData.m_aAttachModels.AddToTail();
	if ( iAttach == m_BMPResData.m_aAttachModels.InvalidIndex() )
		return;

	m_BMPResData.m_aAttachModels[iAttach].m_pszModelName = ReadAndAllocStringValue( inResourceData, "modelname" );
	m_BMPResData.m_aAttachModels[iAttach].m_nSkin = inResourceData->GetInt( "skin", -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetupModelDefaults( void )
{
	SetupModelAnimDefaults();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetupModelAnimDefaults( void )
{
	// Set the move_x parameter so the run activity works
	SetPoseParameterByName( "move_x", 1.0f );

	// Verify that we have animations for this model.
	int nAnimCount = m_BMPResData.m_aAnimations.Count();
	if ( nAnimCount == 0 )
		return;

	// Find the default animation if one exists.
	int iIndex = FindDefaultAnim();
	if ( iIndex == -1 )
		return;

	SetModelAnim( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseModelPanel::FindDefaultAnim( void )
{
	int iIndex = -1;

	int nAnimCount = m_BMPResData.m_aAnimations.Count();
	for ( int iAnim = 0; iAnim < nAnimCount; ++iAnim )
	{
		if ( m_BMPResData.m_aAnimations[iAnim].m_bDefault )
			return iAnim;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseModelPanel::FindAnimByName( const char *pszName )
{
	int iIndex = -1;
	if ( !pszName )
		return iIndex;
	
	int nAnimCount = m_BMPResData.m_aAnimations.Count();
	for ( int iAnim = 0; iAnim < nAnimCount; ++iAnim )
	{
		if ( !Q_stricmp( m_BMPResData.m_aAnimations[iAnim].m_pszName, pszName ) )
			return iAnim;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseModelPanel::FindSequenceFromActivity( CStudioHdr *pStudioHdr, const char *pszActivity )
{
	if ( !pStudioHdr )
		return -1;

	for ( int iSeq = 0; iSeq < pStudioHdr->GetNumSeq(); ++iSeq )
	{
		mstudioseqdesc_t &seqDesc = pStudioHdr->pSeqdesc( iSeq );
		if ( !V_stricmp( seqDesc.pszActivityName(), pszActivity ) )
		{
			return iSeq;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetModelAnim( int iAnim )
{
	int nAnimCount = m_BMPResData.m_aAnimations.Count();
	if ( nAnimCount == 0 || !m_BMPResData.m_aAnimations.IsValidIndex( iAnim ) )
		return;

	MDLCACHE_CRITICAL_SECTION();

	// Get the studio header of the root model.
	studiohdr_t *pStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( !pStudioHdr )
		return;

	CStudioHdr studioHdr( pStudioHdr, g_pMDLCache );

	// Do we have an activity or a sequence?
	int iSequence = ACT_INVALID;
	if ( m_BMPResData.m_aAnimations[iAnim].m_pszActivity && m_BMPResData.m_aAnimations[iAnim].m_pszActivity[0] )
	{
		iSequence = FindSequenceFromActivity( &studioHdr, m_BMPResData.m_aAnimations[iAnim].m_pszActivity );
	}
	else if ( m_BMPResData.m_aAnimations[iAnim].m_pszSequence && m_BMPResData.m_aAnimations[iAnim].m_pszSequence[0] )
	{
		iSequence = LookupSequence( &studioHdr, m_BMPResData.m_aAnimations[iAnim].m_pszSequence );
	}

	if ( iSequence != ACT_INVALID )
	{
		SetSequence( iSequence, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetMDL( MDLHandle_t handle, void *pProxyData )
{
	MDLCACHE_CRITICAL_SECTION();
	studiohdr_t *pHdr = g_pMDLCache->GetStudioHdr( handle );

	if ( pHdr )
	{
		// SetMDL will cause the base CMdl code to set our localtoglobal indices if they aren't set.
		// We set them up here so that they're left alone by that code.
		CStudioHdr studioHdr( pHdr, g_pMDLCache );
		if (studioHdr.numflexcontrollers() > 0 && studioHdr.pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1)
		{
			for (LocalFlexController_t i = LocalFlexController_t(0); i < studioHdr.numflexcontrollers(); i++)
			{
				int j = C_BaseFlex::AddGlobalFlexController( studioHdr.pFlexcontroller( i )->pszName() );
				studioHdr.pFlexcontroller( i )->localToGlobal = j;
			}
		}
	}
	else 
	{
		handle = MDLHANDLE_INVALID;
	}

	// Clear our current sequence
	SetSequence( ACT_IDLE );

	BaseClass::SetMDL( handle, pProxyData );

	SetupModelDefaults();

	// Need to invalidate the layout so the panel will adjust is LookAt for the new model.
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetModelAnglesAndPosition( const QAngle &angRot, const Vector &vecPos )
{
	BaseClass::SetModelAnglesAndPosition( angRot, vecPos );

	// Cache
	m_vecPlayerPos = vecPos;
	m_angPlayer = angRot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::SetMDL( const char *pMDLName, void *pProxyData )
{
	BaseClass::SetMDL( pMDLName, pProxyData );

	// Need to invalidate the layout so the panel will adjust is LookAt for the new model.
//	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_bForcedCameraPosition )
	{
		return;
	}

	if ( m_bAllowFullManipulation )
	{
		// Set this to true if you want to keep the current rotation when changing models or poses
		const bool bPreserveManipulation = false;

		// Need to look at the target so we can rotate around it
		const Vector kVecFocalPoint( 0.0f, 0.0f, 60.0f );
		ResetCameraPivot();
		SetCameraOffset( -(m_vecPlayerPos + kVecFocalPoint) );
		SetCameraPositionAndAngles( kVecFocalPoint, vec3_angle, !bPreserveManipulation );

		// We want to move the player to the origin and facing the correct way,
		// but don't clobber m_angPlayer and m_vecPlayerPos, so use BaseClass.
		BaseClass::SetModelAnglesAndPosition( m_angPlayer, vec3_origin );

		// Once a manual transform has been done we want to apply it
		if ( m_bApplyManipulators )
		{
			ApplyManipulation();
		}
		else
		{
			SyncManipulation();
		}
		return;
	}

	if ( m_bForcePos )
	{
		ResetCameraPivot();
		SetCameraOffset( Vector( 0.0f, 0.0f, 0.0f ) );
		SetCameraPositionAndAngles( vec3_origin, vec3_angle );
		SetModelAnglesAndPosition( m_angPlayer, m_vecPlayerPos );
	}

	// Center and fill the frame with the model?
	if ( m_bStartFramed )
	{
		Vector vecBoundsMin, vecBoundsMax;
		if ( GetBoundingBox( vecBoundsMin, vecBoundsMax ) )
		{
			LookAtBounds( vecBoundsMin, vecBoundsMax );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnTick()
{
	// Cycle stuff gets handled in mdlpanel::OnTick, so we want to fix up
	// what our sequence is before it gets called.

	// Check if we have a active sequence, and if it's expired and we need
	// to run our default
	if ( m_nActiveSequence != ACT_INVALID )
	{
		float flElapsedTime = GetAutoPlayTime() - m_RootMDL.m_flCycleStartTime;
		if ( flElapsedTime >= m_flActiveSequenceDuration )
		{
			m_nActiveSequence = ACT_INVALID;
			m_flActiveSequenceDuration = 0.f;

			SetupModelDefaults();
		}
	}

	BaseClass::OnTick();
}

void CBaseModelPanel::OnThink()
{
	BaseClass::OnThink();

	float flDt = 0.f;
	if ( m_flLastThink != 0.f )
	{
		flDt = Plat_FloatTime() - m_flLastThink;
	}
	m_flLastThink = Plat_FloatTime();

	if ( !m_bMousePressed && m_bUseVelocity )
	{
		RotateYaw( m_flYawVelocity );
		RotatePitch( m_flPitchVelocity );

		// Decay
		m_flYawVelocity *= 1.f - Clamp( m_flYawVelocityDecay * flDt, 0.f, 1.f );
		m_flPitchVelocity *= 1.f - Clamp( m_flPitchVelocityDecay * flDt, 0.f, 1.f );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnKeyCodePressed ( vgui::KeyCode code )
{
	if ( m_bAllowFullManipulation )
	{
		BaseClass::OnKeyCodePressed( code );
		return;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnKeyCodeReleased( vgui::KeyCode code )
{
	if ( m_bAllowFullManipulation )
	{
		BaseClass::OnKeyCodeReleased( code );
		return;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnMousePressed ( vgui::MouseCode code )
{
	if ( m_bAllowFullManipulation )
	{
		BaseClass::OnMousePressed( code );
		return;
	}

	if ( !m_bAllowRotation && !m_bAllowPitch )
		return;

	RequestFocus();

	EnableMouseCapture( true, code );

	// Save where they clicked
	input()->GetCursorPosition( m_nClickStartX, m_nClickStartY );

	// Warp the mouse to the center of the screen
	int width, height;
	GetSize( width, height );
	int x = width / 2;
	int y = height / 2;

	int xpos = x;
	int ypos = y;
	LocalToScreen( xpos, ypos );
	input()->SetCursorPos( xpos, ypos );

	m_nManipStartX = xpos;
	m_nManipStartY = ypos;

	m_bMousePressed = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnMouseReleased( vgui::MouseCode code )
{
	if ( m_bAllowFullManipulation )
	{
		BaseClass::OnMouseReleased( code );
		return;
	}

	if ( !m_bAllowRotation && !m_bAllowPitch )
		return;

	EnableMouseCapture( false );
	m_bMousePressed = false;

	// Restore the cursor to where the clicked
	input()->SetCursorPos( m_nClickStartX, m_nClickStartY );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnCursorMoved( int x, int y )
{
	if ( m_bAllowFullManipulation )
	{
		if ( m_pCurrentManip )
		{
			m_bApplyManipulators = true;
		}
		BaseClass::OnCursorMoved( x, y );
		return;
	}

	if ( !m_bAllowRotation && !m_bAllowPitch )
		return;

	if ( m_bMousePressed )
	{
		WarpMouse( x, y );
		int xpos, ypos;
		input()->GetCursorPos( xpos, ypos );

		if ( m_bAllowRotation )
		{
			// Only want the x delta.
			float flDelta = xpos - m_nManipStartX;


			// Apply the delta and rotate the player.
			RotateYaw( flDelta );
			m_flYawVelocity = flDelta;
		}

		if ( m_bAllowPitch )
		{
			// Only want the y delta.
			float flDelta = ypos - m_nManipStartY;


			// Apply the delta and rotate the player.
			RotatePitch( flDelta );
			m_flPitchVelocity = flDelta;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::RotateYaw( float flDelta )
{
	m_angPlayer.y += flDelta;
	if ( m_angPlayer.y > 360.0f )
	{
		m_angPlayer.y = m_angPlayer.y - 360.0f;
	}
	else if ( m_angPlayer.y < -360.0f )
	{
		m_angPlayer.y = m_angPlayer.y + 360.0f;
	}

	SetModelAnglesAndPosition( m_angPlayer, m_vecPlayerPos );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::RotatePitch( float flDelta )
{
	m_angPlayer.x += flDelta;
	if ( m_angPlayer.x > m_flMaxPitch )
	{
		m_angPlayer.x = m_flMaxPitch;
	}
	else if ( m_angPlayer.x < -m_flMaxPitch )
	{
		m_angPlayer.x = -m_flMaxPitch;
	}

	SetModelAnglesAndPosition( m_angPlayer, m_vecPlayerPos );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CBaseModelPanel::GetPlayerPos() const
{
	return m_vecPlayerPos;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
QAngle CBaseModelPanel::GetPlayerAngles() const
{
	return m_angPlayer;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::PlaySequence( const char *pszSequenceName )
{
	CStudioHdr studioHDR( GetStudioHdr(), g_pMDLCache );
	int iSeq = ::LookupSequence( &studioHDR, pszSequenceName );
	if ( iSeq != ACT_INVALID )
	{
		m_nActiveSequence = iSeq;
		m_flActiveSequenceDuration = Studio_Duration( &studioHDR, iSeq, NULL );
		SetSequence( m_nActiveSequence, true );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseModelPanel::OnMouseWheeled( int delta )
{
	if ( m_bAllowFullManipulation )
	{
		BaseClass::OnMouseWheeled( delta );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the camera to a distance that allows the object to fill the model panel.
//-----------------------------------------------------------------------------
void CBaseModelPanel::LookAtBounds( const Vector &vecBoundsMin, const Vector &vecBoundsMax )
{
	// Get the model space render bounds.
	Vector vecMin = vecBoundsMin;
	Vector vecMax = vecBoundsMax;
	Vector vecCenter = ( vecMax + vecMin ) * 0.5f;
	vecMin -= vecCenter;
	vecMax -= vecCenter;

	// Get the bounds points and transform them by the desired model panel rotation.
	Vector aBoundsPoints[8];
	aBoundsPoints[0].Init( vecMax.x, vecMax.y, vecMax.z ); 
	aBoundsPoints[1].Init( vecMin.x, vecMax.y, vecMax.z ); 
	aBoundsPoints[2].Init( vecMax.x, vecMin.y, vecMax.z ); 
	aBoundsPoints[3].Init( vecMin.x, vecMin.y, vecMax.z ); 
	aBoundsPoints[4].Init( vecMax.x, vecMax.y, vecMin.z ); 
	aBoundsPoints[5].Init( vecMin.x, vecMax.y, vecMin.z ); 
	aBoundsPoints[6].Init( vecMax.x, vecMin.y, vecMin.z ); 
	aBoundsPoints[7].Init( vecMin.x, vecMin.y, vecMin.z ); 

	// Translated center point (offset from camera center).
	Vector vecTranslateCenter = -vecCenter;

	// Build the rotation matrix.
	matrix3x4_t matRotation;
	AngleMatrix( m_BMPResData.m_angModelPoseRot, matRotation );

	Vector aXFormPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		VectorTransform( aBoundsPoints[iPoint], matRotation, aXFormPoints[iPoint] );
	}

	Vector vecXFormCenter;
	VectorTransform( -vecTranslateCenter, matRotation, vecXFormCenter );

	int w, h;
	GetSize( w, h );
	float flW = (float)w;
	float flH = (float)h;

	float flFOVx = DEG2RAD( m_BMPResData.m_flFOV * 0.5f );
	float flFOVy = CalcFovY( ( m_BMPResData.m_flFOV * 0.5f ), flW/flH );
	flFOVy = DEG2RAD( flFOVy );

	float flTanFOVx = tan( flFOVx );
	float flTanFOVy = tan( flFOVy );

	// Find the max value of x, y, or z
	Vector2D dist[8];
	float flDist = 0.0f;
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		float flDistY = fabs( aXFormPoints[iPoint].y / flTanFOVx ) - aXFormPoints[iPoint].x;
		float flDistZ = fabs( aXFormPoints[iPoint].z / flTanFOVy ) - aXFormPoints[iPoint].x;
		dist[iPoint].x = flDistY;
		dist[iPoint].y = flDistZ;
		float flTestDist = MAX( flDistZ, flDistY );
		flDist = MAX( flDist, flTestDist );
	}

	// Screen space points.
	Vector2D aScreenPoints[8];
	Vector aCameraPoints[8];
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		aCameraPoints[iPoint] = aXFormPoints[iPoint];
		aCameraPoints[iPoint].x += flDist;

		aScreenPoints[iPoint].x = aCameraPoints[iPoint].y / ( flTanFOVx * aCameraPoints[iPoint].x );
		aScreenPoints[iPoint].y = aCameraPoints[iPoint].z / ( flTanFOVy * aCameraPoints[iPoint].x );

		aScreenPoints[iPoint].x = ( aScreenPoints[iPoint].x * 0.5f + 0.5f ) * flW;
		aScreenPoints[iPoint].y = ( aScreenPoints[iPoint].y * 0.5f + 0.5f ) * flH;
	}

	// Find the min/max and center of the 2D bounding box of the object.
	Vector2D vecScreenMin( 99999.0f, 99999.0f ), vecScreenMax( -99999.0f, -99999.0f );
	for ( int iPoint = 0; iPoint < 8; ++iPoint )
	{
		vecScreenMin.x = MIN( vecScreenMin.x, aScreenPoints[iPoint].x );
		vecScreenMin.y = MIN( vecScreenMin.y, aScreenPoints[iPoint].y );
		vecScreenMax.x = MAX( vecScreenMax.x, aScreenPoints[iPoint].x );
		vecScreenMax.y = MAX( vecScreenMax.y, aScreenPoints[iPoint].y );
	}

	// Offset the model to the be the correct distance away from the camera.
	Vector vecModelPos;
	vecModelPos.x = flDist - vecXFormCenter.x;
	vecModelPos.y = -vecXFormCenter.y;
	vecModelPos.z = -vecXFormCenter.z;
	SetModelAnglesAndPosition( m_BMPResData.m_angModelPoseRot, vecModelPos );
	m_vecPlayerPos = vecModelPos;

	// Back project to figure out the camera offset to center the model.
	Vector2D vecPanelCenter( ( flW * 0.5f ), ( flH * 0.5f ) );
	Vector2D vecScreenCenter = ( vecScreenMax + vecScreenMin ) * 0.5f;

	Vector2D vecPanelCenterCamera, vecScreenCenterCamera;
	vecPanelCenterCamera.x = ( ( vecPanelCenter.x / flW ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.y = ( ( vecPanelCenter.y / flH ) * 2.0f ) - 0.5f;
	vecPanelCenterCamera.x *= ( flTanFOVx * flDist );
	vecPanelCenterCamera.y *= ( flTanFOVy * flDist );
	vecScreenCenterCamera.x = ( ( vecScreenCenter.x / flW ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.y = ( ( vecScreenCenter.y / flH ) * 2.0f ) - 0.5f;
	vecScreenCenterCamera.x *= ( flTanFOVx * flDist );
	vecScreenCenterCamera.y *= ( flTanFOVy * flDist );

	Vector2D vecCameraOffset( 0.0f, 0.0f );
	vecCameraOffset.x = vecPanelCenterCamera.x - vecScreenCenterCamera.x;
	vecCameraOffset.y = vecPanelCenterCamera.y - vecScreenCenterCamera.y;

	// Clear the camera pivot and set position matrix.
	ResetCameraPivot();
	if (m_bAllowRotation || m_bAllowPitch )
	{
		vecCameraOffset.x = 0.0f;
	}
	SetCameraOffset( Vector( 0.0f, -vecCameraOffset.x, -vecCameraOffset.y ) );
	UpdateCameraTransform();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseModelPanel::particle_data_t::~particle_data_t()
{
	if ( m_pParticleSystem )
	{
		delete m_pParticleSystem;
		m_pParticleSystem = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Allocate particle data
//-----------------------------------------------------------------------------
void CBaseModelPanel::particle_data_t::UpdateControlPoints( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix, const CUtlVector< int >& vecAttachments, int iDefaultBone /*= 0*/, const Vector& vecParticleOffset /*= vec3_origin*/ )
{
	if ( m_pParticleSystem )
	{
		// Update control points which is updating the position of the particles
		matrix3x4_t matAttachToWorld;
		Vector vecPosition, vecForward, vecRight, vecUp;
		if ( vecAttachments.Count() )
		{
			for ( int i = 0; i < vecAttachments.Count(); ++i )
			{
				const mstudioattachment_t& attach = pStudioHdr->pAttachment( vecAttachments[i] ); 
				MatrixMultiply( pWorldMatrix[ attach.localbone ], attach.local, matAttachToWorld );

				MatrixVectors( matAttachToWorld, &vecForward, &vecRight, &vecUp );
				MatrixPosition( matAttachToWorld, vecPosition );

				m_pParticleSystem->SetControlPointOrientation( i, vecForward, vecRight, vecUp );
				m_pParticleSystem->SetControlPoint( i, vecPosition + vecParticleOffset );
			}
		}
		else
		{
			matAttachToWorld = pWorldMatrix[iDefaultBone];
			MatrixVectors( matAttachToWorld, &vecForward, &vecRight, &vecUp );
			MatrixPosition( matAttachToWorld, vecPosition );
			
			m_pParticleSystem->SetControlPointOrientation( 0, vecForward, vecRight, vecUp );
			m_pParticleSystem->SetControlPoint( 0, vecPosition + vecParticleOffset );
		}
	}

	m_bIsUpdateToDate = true;
}


//-----------------------------------------------------------------------------
// Purpose: Allocate particle data
//-----------------------------------------------------------------------------
CBaseModelPanel::particle_data_t *CBaseModelPanel::CreateParticleData( const char *pszParticleName )
{
	Assert( m_bUseParticle );
	if ( !m_bUseParticle )
		return NULL;

	CParticleCollection *pParticle = g_pParticleSystemMgr->CreateParticleCollection( pszParticleName );
	if ( !pParticle )
		return NULL;

	particle_data_t *pData = new particle_data_t;
	pData->m_bIsUpdateToDate = false;
	pData->m_pParticleSystem = pParticle;

	m_particleList.AddToTail( pData );

	return pData;
}


//-----------------------------------------------------------------------------
// Purpose: remove and delete particle data
//-----------------------------------------------------------------------------
bool CBaseModelPanel::SafeDeleteParticleData( particle_data_t **pData )
{
	if ( !m_bUseParticle )
		return false;

	if ( *pData )
	{
		FOR_EACH_VEC( m_particleList, i )
		{
			if ( *pData == m_particleList[i] )
			{
				delete *pData;
				*pData = NULL;
				m_particleList.FastRemove( i );
				return true;
			}
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::PrePaint3D( IMatRenderContext *pRenderContext )
{
	if ( !m_bUseParticle )
		return;

	// mark all effects need to be updated
	FOR_EACH_VEC( m_particleList, i )
	{
		m_particleList[i]->m_bIsUpdateToDate = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModelPanel::PostPaint3D( IMatRenderContext *pRenderContext )
{
	if ( !m_bUseParticle )
		return;

	// This needs calling to reset various counters.
	g_pParticleSystemMgr->SetLastSimulationTime( gpGlobals->curtime );

	// Render Particles
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity( );

	FOR_EACH_VEC( m_particleList, i )
	{
		if ( m_particleList[i]->m_bIsUpdateToDate )
		{
			m_particleList[i]->m_pParticleSystem->Simulate( gpGlobals->frametime, false );
			m_particleList[i]->m_pParticleSystem->Render( pRenderContext );
			m_particleList[i]->m_bIsUpdateToDate = false;
		}
	}

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();
}

