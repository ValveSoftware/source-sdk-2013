//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The morph operator class - sets morph target weights on meshes
//
//=============================================================================

#ifndef DMEPACKOPERATORS_H
#define DMEPACKOPERATORS_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"


//-----------------------------------------------------------------------------
// CDmePackColorOperator - combines floats into a color
//-----------------------------------------------------------------------------
class CDmePackColorOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackColorOperator, CDmeOperator );

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
// CDmePackVector2Operator - combines floats into a vector2
//-----------------------------------------------------------------------------
class CDmePackVector2Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackVector2Operator, CDmeOperator );

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
// CDmePackVector3Operator - combines floats into a vector
//-----------------------------------------------------------------------------
class CDmePackVector3Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackVector3Operator, CDmeOperator );

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
// CDmePackVector4Operator - combines floats into a vector4
//-----------------------------------------------------------------------------
class CDmePackVector4Operator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackVector4Operator, CDmeOperator );

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
// CDmePackQAngleOperator - combines floats into a qangle
//-----------------------------------------------------------------------------
class CDmePackQAngleOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackQAngleOperator, CDmeOperator );

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
// CDmePackQuaternionOperator - combines floats into a quaternion
//-----------------------------------------------------------------------------
class CDmePackQuaternionOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackQuaternionOperator, CDmeOperator );

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
// CDmePackVMatrixOperator - combines floats into a VMatrix
//-----------------------------------------------------------------------------
class CDmePackVMatrixOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmePackVMatrixOperator, CDmeOperator );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< VMatrix > m_vmatrix;
	CDmaVar< float > m_cells[ 16 ];
};


#endif // DMEPACKOPERATORS_H
