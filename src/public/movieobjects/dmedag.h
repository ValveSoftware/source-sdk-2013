//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a Dag (directed acyclic graph) node used for holding transforms, lights, cameras and shapes
//
//=============================================================================

#ifndef DMEDAG_H
#define DMEDAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstack.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "movieobjects/dmeshape.h"
#include "movieobjects/dmetransform.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeTransform;
class CDmeShape;
class CDmeDrawSettings;


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeDag : public CDmElement
{
	DEFINE_ELEMENT( CDmeDag, CDmElement );

public:
	// Accessors
	CDmeTransform *GetTransform();
	CDmeShape *GetShape();

	// Changes the shage
	void SetShape( CDmeShape *pShape );

	bool IsVisible() const;
	void SetVisible( bool bVisible = true );

	// child helpers
	const CUtlVector< DmElementHandle_t > &GetChildren() const;
	int GetChildCount() const;
	CDmeDag *GetChild( int i ) const;
	void AddChild( CDmeDag* pDag );
	void RemoveChild( int i );
	void RemoveChild( const CDmeDag *pChild, bool bRecurse = false );
	int FindChild( const CDmeDag *pChild ) const;
	int FindChild( CDmeDag *&pParent, const CDmeDag *pChild );
	int FindChild( const char *name ) const;
	CDmeDag *FindOrAddChild( const char *name );

	// Recursively render the Dag hierarchy
	virtual void Draw( CDmeDrawSettings *pDrawSettings = NULL );
	void GetBoundingSphere( Vector &center, float &radius ) const
	{
		matrix3x4_t identity;
		SetIdentityMatrix( identity );
		GetBoundingSphere( center, radius, identity );
	}

	void GetShapeToWorldTransform( matrix3x4_t &mat );

	void GetLocalMatrix( matrix3x4_t &mat );

	void GetWorldMatrix( matrix3x4_t &mat );

	void GetParentWorldMatrix( matrix3x4_t &mat );

	static void DrawUsingEngineCoordinates( bool bEnable );

	// Transform from DME to engine coordinates
	static void DmeToEngineMatrix( matrix3x4_t& dmeToEngine );
	static void EngineToDmeMatrix( matrix3x4_t& engineToDme );

protected:
	void GetBoundingSphere( Vector &center, float &radius, const matrix3x4_t &pMat ) const;

	void PushDagTransform();
	void PopDagTransform();
	CDmAttribute *GetVisibilityAttribute();

	CDmaVar< bool >					m_Visible;
	CDmaElement< CDmeTransform >	m_Transform;
	CDmaElement< CDmeShape >		m_Shape;
	CDmaElementArray< CDmeDag >		m_Children;

private:
	struct TransformInfo_t
	{
		CDmeTransform *m_pTransform;
		matrix3x4_t	m_DagToWorld;
		bool m_bComputedDagToWorld;
	};
	
	static CUtlStack<TransformInfo_t> s_TransformStack;
	static bool s_bDrawUsingEngineCoordinates;
};


#endif // DMEDAG_H
