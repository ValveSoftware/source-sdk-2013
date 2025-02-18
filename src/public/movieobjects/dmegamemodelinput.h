//========= Copyright Valve Corporation, All rights reserved. ============//
//
// game model input - gets its values from an MDL within the game
//
//=============================================================================

#ifndef DMEGAMEMODELINPUT_H
#define DMEGAMEMODELINPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "movieobjects/dmeinput.h"


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CDmeGameModelInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeGameModelInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

public:
	void AddBone( const Vector& pos, const Quaternion& rot );
	void SetBone( uint index, const Vector& pos, const Quaternion& rot );
	void SetRootBone( const Vector& pos, const Quaternion& rot );
	uint NumBones() const;

	void SetFlexWeights( uint nFlexWeights, const float* flexWeights );
	uint NumFlexWeights() const;

	const Vector& GetViewTarget() const;
	void SetViewTarget( const Vector &viewTarget );

	void SetFlags( int nFlags );

public:
	CDmaVar< int >					m_skin;
	CDmaVar< int >					m_body;
	CDmaVar< int >					m_sequence;
	CDmaVar< bool >					m_visible;

protected:
	CDmaVar< int >					m_flags;
	CDmaArray< float >				m_flexWeights;
	CDmaVar< Vector >				m_viewTarget;
	CDmaArray< Vector >				m_bonePositions;
	CDmaArray< Quaternion >			m_boneRotations;
	CDmaVar< Vector >				m_position;
	CDmaVar< Quaternion >			m_rotation;
};


class CDmeGameSpriteInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeGameSpriteInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

public:
	void SetState( bool bVisible, float nFrame, int nRenderMode, int nRenderFX, float flRenderScale, float flProxyRadius, const Vector &pos, const Quaternion &rot, const Color &color );

protected:

	CDmaVar< bool >			m_visible;
	CDmaVar< float >		m_frame;
	CDmaVar< int >			m_rendermode;
	CDmaVar< int >			m_renderfx;
	CDmaVar< float >		m_renderscale;
	CDmaVar< float >		m_proxyRadius;
	CDmaVar< Vector >		m_position;
	CDmaVar< Quaternion >	m_rotation;
	CDmaVar< Color >		m_color;
};


class CDmeGameCameraInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeGameCameraInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

public:
	void SetPosition( const Vector& pos );
	void SetOrientation( const Quaternion& rot );
	void SetFOV( float fov );

protected:
	CDmaVar< Vector >		m_position;
	CDmaVar< Quaternion >	m_rotation;
	CDmaVar< float >		m_fov;
};


#endif // DMEGAMEMODELINPUT_H
