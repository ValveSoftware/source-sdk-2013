//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "weapon_ifmbasecamera.h"

#ifdef CLIENT_DLL
#include "view_shared.h"
#include "iviewrender.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );

#endif

#define INSET_VIEW_FACTOR 0.3f

//-----------------------------------------------------------------------------
// CWeaponIFMBaseCamera tables.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponIFMBaseCamera, DT_WeaponIFMBaseCamera )
LINK_ENTITY_TO_CLASS( weapon_ifm_base_camera, CWeaponIFMBaseCamera );

BEGIN_NETWORK_TABLE( CWeaponIFMBaseCamera, DT_WeaponIFMBaseCamera )	
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flRenderAspectRatio ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flRenderFOV ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flRenderArmLength ), 0, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRenderPosition ), 0, SPROP_NOSCALE ),
	SendPropQAngles( SENDINFO( m_angRenderAngles ), 0, SPROP_NOSCALE ),
#else
	RecvPropFloat( RECVINFO( m_flRenderAspectRatio ) ),
	RecvPropFloat( RECVINFO( m_flRenderFOV ) ),
	RecvPropFloat( RECVINFO( m_flRenderArmLength ) ),
	RecvPropVector( RECVINFO( m_vecRenderPosition ) ),
	RecvPropQAngles( RECVINFO( m_angRenderAngles ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL

BEGIN_PREDICTION_DATA( CWeaponIFMBaseCamera ) 
	DEFINE_PRED_FIELD( m_flFOV, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flArmLength, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_vecRelativePosition, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_angRelativeAngles, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_bFullScreen, FIELD_BOOLEAN, 0 ),
END_PREDICTION_DATA()

#endif


#ifdef GAME_DLL

BEGIN_DATADESC( CWeaponIFMBaseCamera )
	DEFINE_FIELD( m_flRenderAspectRatio, FIELD_FLOAT ),
	DEFINE_FIELD( m_flRenderFOV, FIELD_FLOAT ),
	DEFINE_FIELD( m_flRenderArmLength, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecRenderPosition, FIELD_VECTOR ),
	DEFINE_FIELD( m_angRenderAngles, FIELD_VECTOR ),
END_DATADESC()

#endif


//-----------------------------------------------------------------------------
// CWeaponIFMBaseCamera implementation. 
//-----------------------------------------------------------------------------
CWeaponIFMBaseCamera::CWeaponIFMBaseCamera()
{
#ifdef CLIENT_DLL
	m_flFOV = 75.0f;
	m_flArmLength = 4;
	m_vecRelativePosition.Init();
	m_angRelativeAngles.Init();
	m_bFullScreen = false;
	m_nScreenWidth = 0;
	m_nScreenHeight = 0; 
#endif
}


//-----------------------------------------------------------------------------
//
// Specific methods on the server 
//
//-----------------------------------------------------------------------------
#ifdef GAME_DLL

void CWeaponIFMBaseCamera::SetRenderInfo( float flAspectRatio, float flFOV, float flArmLength, const Vector &vecPosition, const QAngle &angles )
{
	m_flRenderAspectRatio = flAspectRatio;
	m_flRenderFOV = flFOV;
	m_flRenderArmLength = flArmLength;
	m_vecRenderPosition = vecPosition;
	m_angRenderAngles = angles;
}

CON_COMMAND( ifm_basecamera_camerastate, "Set camera state" )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	if ( args.ArgC() != 10 )
		return;
	
	Vector vecPosition;
	QAngle angAngles;
	float flAspectRatio = atof( args[1] );
	float flFOV = atof( args[2] );
	float flArmLength = atof( args[3] );
	vecPosition.x = atof( args[4] );
	vecPosition.y = atof( args[5] );
	vecPosition.z = atof( args[6] );
	angAngles.x = atof( args[7] );
	angAngles.y = atof( args[8] );
	angAngles.z = atof( args[9] );

	int nCount = pPlayer->WeaponCount();
	for ( int i = 0; i < nCount; ++i )
	{
		CWeaponIFMBaseCamera *pCamera = dynamic_cast<CWeaponIFMBaseCamera*>( pPlayer->GetWeapon( i ) );
		if ( !pCamera )
			continue;

		pCamera->SetRenderInfo( flAspectRatio, flFOV, flArmLength, vecPosition, angAngles );
	}
}	

#endif   // GAME_DLL


//-----------------------------------------------------------------------------
//
// Specific methods on the client 
//
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
	

//-----------------------------------------------------------------------------
// Sets up the material to draw with
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if (updateType == DATA_UPDATE_CREATED)
	{
		m_FrustumMaterial.Init( "effects/steadycamfrustum", TEXTURE_GROUP_OTHER );
		m_FrustumWireframeMaterial.Init( "shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER );
	}
}


//-----------------------------------------------------------------------------
// Transmits render information
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::TransmitRenderInfo()
{
	float flAspectRatio = (m_nScreenHeight != 0) ? (float)m_nScreenWidth / (float)m_nScreenHeight : 1.0f;
	float flFOV = m_flFOV;

	Vector position;
	QAngle angles;
	ComputeAbsCameraTransform( position, angles );

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( position, angles, flFOV );

	char pBuf[256];
	Q_snprintf( pBuf, sizeof(pBuf), "ifm_basecamera_camerastate %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f", 
		flAspectRatio, flFOV, m_flArmLength, position.x, position.y, position.z,
		angles.x, angles.y, angles.z );

	engine->ClientCmd( pBuf );
}


//-----------------------------------------------------------------------------
// Purpose: In 3rd person, draws the cone of the steadycam
//-----------------------------------------------------------------------------
#define FRUSTUM_SIZE 1000

int CWeaponIFMBaseCamera::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );

	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer && !pPlayer->IsLocalPlayer() )
	{
		// Compute endpoints
		float flMaxD = 1.0f / tan( M_PI * m_flFOV / 360.0f );
		float ex = flMaxD * FRUSTUM_SIZE;
		float ey = flMaxD * FRUSTUM_SIZE / m_flRenderAspectRatio;

		// Compute basis
		Vector vecForward, vecUp, vecRight;
		AngleVectors( m_angRenderAngles, &vecForward, &vecRight, &vecUp );

		Vector vecCenter;
		VectorMA( m_vecRenderPosition, FRUSTUM_SIZE, vecForward, vecCenter );

		Vector vecEndPoint[4];
		VectorMA( vecCenter, ex, vecRight, vecEndPoint[0] );
		VectorMA( vecEndPoint[0], ey, vecUp, vecEndPoint[0] );
		VectorMA( vecEndPoint[0], -2.0f * ex, vecRight, vecEndPoint[1] );
		VectorMA( vecEndPoint[1], -2.0f * ey, vecUp, vecEndPoint[2] );
		VectorMA( vecEndPoint[2], 2.0f * ex, vecRight, vecEndPoint[3] );

		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( m_FrustumMaterial );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 4 );
		for ( int i = 0; i < 4; ++i )
		{
			meshBuilder.Position3fv( m_vecRenderPosition.Get().Base() );
			meshBuilder.Color4ub( 128, 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecEndPoint[i].Base() );
			meshBuilder.Color4ub( 128, 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecEndPoint[(i+1)%4].Base() );
			meshBuilder.Color4ub( 128, 0, 0, 255 );
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();

		pRenderContext->Bind( m_FrustumWireframeMaterial );
		pMesh = pRenderContext->GetDynamicMesh( true );
		meshBuilder.Begin( pMesh, MATERIAL_LINES, 8 );
		for ( int i = 0; i < 4; ++i )
		{
			meshBuilder.Position3fv( m_vecRenderPosition.Get().Base() );
			meshBuilder.Color4ub( 255, 255, 255, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecEndPoint[i].Base() );
			meshBuilder.Color4ub( 255, 255, 255, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecEndPoint[i].Base() );
			meshBuilder.Color4ub( 255, 255, 255, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecEndPoint[(i+1)%4].Base() );
			meshBuilder.Color4ub( 255, 255, 255, 255 );
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();
	}

	return nRetVal;
}


//-----------------------------------------------------------------------------
// Gets the size of the overlay to draw
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::GetViewportSize( int &w, int &h )
{
	if ( !m_bFullScreen )
	{
		w = m_nScreenWidth * INSET_VIEW_FACTOR;
		h = m_nScreenHeight * INSET_VIEW_FACTOR;
	}
	else
	{
		w = m_nScreenWidth;
		h = m_nScreenHeight;
	}
}


//-----------------------------------------------------------------------------
// Gets the abs orientation of the camera
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::ComputeAbsCameraTransform( Vector &vecAbsOrigin, QAngle &angAbsRotation )
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
	{
		vecAbsOrigin.Init();
		angAbsRotation.Init();
		return;
	}

	float flFOV = m_flFOV;

	float flZNear = view->GetZNear();
	float flZFar = view->GetZFar();
	Vector viewOrigin;
	QAngle viewAngles;
	pPlayer->CalcView( viewOrigin, viewAngles, flZNear, flZFar, flFOV );

	// Offset the view along the forward direction vector by the arm length
	Vector vecForward;
	AngleVectors( viewAngles, &vecForward );
	VectorMA( viewOrigin, m_flArmLength, vecForward, viewOrigin );  
	
	// Use player roll
	QAngle angles = m_angRelativeAngles;
	angles.z = viewAngles.z;

	// Compute the actual orientation of the view
	matrix3x4_t cameraToWorld, overlayToCamera, overlayToWorld;
	AngleMatrix( vec3_angle, viewOrigin, cameraToWorld );
	AngleMatrix( angles, m_vecRelativePosition, overlayToCamera );
	ConcatTransforms( cameraToWorld, overlayToCamera, overlayToWorld );
	MatrixAngles( overlayToWorld, angAbsRotation, vecAbsOrigin );

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( vecAbsOrigin, angAbsRotation, flFOV );
}


//-----------------------------------------------------------------------------
// Gets the bounds of the overlay to draw
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::GetOverlayBounds( int &x, int &y, int &w, int &h )
{
	const CViewSetup *pViewSetup = view->GetViewSetup();
	if ( !m_bFullScreen )
	{
		w = pViewSetup->width * INSET_VIEW_FACTOR;
		h = pViewSetup->height * INSET_VIEW_FACTOR;
		x = pViewSetup->x + ( pViewSetup->width - w ) / 2;
		y = pViewSetup->height - h;
	}
	else
	{
		w = pViewSetup->width;
		h = pViewSetup->height;
		x = pViewSetup->x;
		y = pViewSetup->y;
	}
}


//-----------------------------------------------------------------------------
// When drawing the model, if drawing the viewmodel, draw an overlay of what's being rendered
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::ViewModelDrawn( CBaseViewModel *pBaseViewModel )
{
	// NOTE: This is not recursively called because we do not draw viewmodels in the overlay
	CViewSetup overlayView = *view->GetViewSetup();

	m_nScreenWidth = overlayView.width;
	m_nScreenHeight = overlayView.height;

	GetOverlayBounds( overlayView.x, overlayView.y, overlayView.width, overlayView.height );
	overlayView.m_bRenderToSubrectOfLargerScreen = true;
	overlayView.fov = m_flFOV;

	// Compute the location of the camera
	ComputeAbsCameraTransform( overlayView.origin, overlayView.angles );

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( overlayView.origin, overlayView.angles, overlayView.fov );

	view->QueueOverlayRenderView( overlayView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, RENDERVIEW_UNSPECIFIED );
}


//-----------------------------------------------------------------------------
// Purpose: Draw the weapon's crosshair
//-----------------------------------------------------------------------------
void CWeaponIFMBaseCamera::DrawCrosshair( void )
{
	BaseClass::DrawCrosshair();

	int x, y, w, h;
	GetOverlayBounds( x, y, w, h );

	// Draw the targeting zone around the crosshair
	int r, g, b, a;
	gHUD.m_clrYellowish.GetColor( r, g, b, a );

	Color light( r, g, b, 160 );

	int nBorderSize = 4;
	vgui::surface()->DrawSetColor( light );
	vgui::surface()->DrawFilledRect( x-nBorderSize, y-nBorderSize, x+w+nBorderSize, y );
	vgui::surface()->DrawFilledRect( x-nBorderSize, y+h, x+w+nBorderSize, y+h+nBorderSize );
	vgui::surface()->DrawFilledRect( x-nBorderSize, y, x, y+h );
	vgui::surface()->DrawFilledRect( x+w, y, x+w+nBorderSize, y+h );
}

#endif // CLIENT_DLL


