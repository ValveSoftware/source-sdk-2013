//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The morph operator class - sets morph target weights on meshes
//
//=============================================================================

#ifndef DMEUNPACKOPERATORS_H
#define DMEUNPACKOPERATORS_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"


//-----------------------------------------------------------------------------
// CDmeUnpackColorOperator - extracts floats from a color
//-----------------------------------------------------------------------------
class CDmeUnpackColorOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackColorOperator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Color > m_color;
	CDmaVar< float> m_red;
	CDmaVar< float> m_green;
	CDmaVar< float> m_blue;
	CDmaVar< float> m_alpha;
};

//-----------------------------------------------------------------------------
// CDmeUnpackVector2Operator - extracts floats from a vector2
//-----------------------------------------------------------------------------
class CDmeUnpackVector2Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackVector2Operator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Vector2D > m_vector;
	CDmaVar< float> m_x;
	CDmaVar< float> m_y;
};

//-----------------------------------------------------------------------------
// CDmeUnpackVector3Operator - extracts floats from a vector
//-----------------------------------------------------------------------------
class CDmeUnpackVector3Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackVector3Operator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Vector > m_vector;
	CDmaVar< float> m_x;
	CDmaVar< float> m_y;
	CDmaVar< float> m_z;
};

//-----------------------------------------------------------------------------
// CDmeUnpackVector4Operator - extracts floats from a vector4
//-----------------------------------------------------------------------------
class CDmeUnpackVector4Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackVector4Operator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Vector4D > m_vector;
	CDmaVar< float> m_x;
	CDmaVar< float> m_y;
	CDmaVar< float> m_z;
	CDmaVar< float> m_w;
};

//-----------------------------------------------------------------------------
// CDmeUnpackQAngleOperator - extracts floats from a qangle
//-----------------------------------------------------------------------------
class CDmeUnpackQAngleOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackQAngleOperator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< QAngle > m_qangle;
	CDmaVar< float> m_x;
	CDmaVar< float> m_y;
	CDmaVar< float> m_z;
};

//-----------------------------------------------------------------------------
// CDmeUnpackQuaternionOperator - extracts floats from a quaternion
//-----------------------------------------------------------------------------
class CDmeUnpackQuaternionOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackQuaternionOperator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Quaternion > m_quaternion;
	CDmaVar< float> m_x;
	CDmaVar< float> m_y;
	CDmaVar< float> m_z;
	CDmaVar< float> m_w;
};

//-----------------------------------------------------------------------------
// CDmeUnpackVMatrixOperator - extracts floats from a VMatrix
//-----------------------------------------------------------------------------
class CDmeUnpackVMatrixOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeUnpackVMatrixOperator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< VMatrix > m_vmatrix;
	CDmaVar< float > m_cells[ 16 ];
};


#endif // DMEUNPACKOPERATORS_H
