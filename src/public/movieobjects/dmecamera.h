//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a camera
//
//=============================================================================

#ifndef DMECAMERA_H
#define DMECAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmedag.h"
#include "movieobjects/timeutils.h"


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeCamera : public CDmeDag
{
	DEFINE_ELEMENT( CDmeCamera, CDmeDag );

public:
	// Sets up render state in the material system for rendering
	// This includes the view matrix and the projection matrix
	void SetupRenderState( int nDisplayWidth, int nDisplayHeight, bool bUseEngineCoordinateSystem = false );

	// accessors for generated matrices
	void GetViewMatrix( VMatrix &view, bool bUseEngineCoordinateSystem = false );
	void GetProjectionMatrix( VMatrix &proj, int width, int height );
	void GetViewProjectionInverse( VMatrix &viewprojinv, int width, int height );
	void ComputeScreenSpacePosition( const Vector &vecWorldPosition, int width, int height, Vector2D *pScreenPosition );

	// Returns the x FOV (the full angle)
	float GetFOVx() const;
	void SetFOVx( float fov );

	// Returns the focal distance in inches
	float GetFocalDistance() const;

	// Sets the focal distance in inches
	void SetFocalDistance( const float &fFocalDistance );

	// Returns the aperture size in inches
	float GetAperture() const;

	// Returns the shutter speed in seconds
	float GetShutterSpeed() const;

	// Returns the tone map scale
	float GetToneMapScale() const;

	// Returns the tone map scale
	float GetBloomScale() const;

	// Returns the view direction
	void GetViewDirection( Vector *pDirection );

	// Returns Depth of Field quality level
	int GetDepthOfFieldQuality() const;

	// Returns the Motion Blur quality level
	int GetMotionBlurQuality() const;

private:
	// Loads the material system view matrix based on the transform
	void LoadViewMatrix( bool bUseEngineCoordinateSystem );

	// Loads the material system projection matrix based on the fov, etc.
	void LoadProjectionMatrix( int nDisplayWidth, int nDisplayHeight );

	// Sets the studiorender state
	void LoadStudioRenderCameraState();

	CDmaVar< float > m_fieldOfView;
	CDmaVar< float > m_zNear;
	CDmaVar< float > m_zFar;

	CDmaVar< float > m_fFocalDistance;
	CDmaVar< float > m_fAperture;
	CDmaVar< float > m_fShutterSpeed;
	CDmaVar< float > m_fToneMapScale;
	CDmaVar< float > m_fBloomScale;

	CDmaVar< int > m_nDoFQuality;
	CDmaVar< int > m_nMotionBlurQuality;
};


#endif // DMECAMERA_H
