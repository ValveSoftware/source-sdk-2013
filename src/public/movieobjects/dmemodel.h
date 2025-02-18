//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Dme version of a skeletal model (gets compiled into a MDL)
//
//===========================================================================//

#ifndef DMEMODEL_H
#define DMEMODEL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstack.h"
#include "movieobjects/dmejoint.h"
#include "movieobjects/dmetransformlist.h"


class CDmeDrawSettings;

//-----------------------------------------------------------------------------
// A class representing a skeletal model
//-----------------------------------------------------------------------------
class CDmeModel : public CDmeDag
{
	DEFINE_ELEMENT( CDmeModel, CDmeDag );

public:
	// Add joint
	CDmeJoint *AddJoint( const char *pJointName, CDmeDag *pParent = NULL );
	int AddJoint( CDmeDag *pJoint );

	// Returns the number of joint transforms we know about
	int GetJointTransformCount() const;

	// Determines joint transform index	given a joint name
	int GetJointTransformIndex( CDmeTransform *pTransform ) const;

	// Determines joint transform index	given a joint
	int GetJointTransformIndex( CDmeDag *pJoint ) const;

	// Determines joint transform index	given a joint name
	CDmeTransform *GetJointTransform( int nIndex );
	const CDmeTransform *GetJointTransform( int nIndex ) const;

	// Captures the current joint transforms into a base state
	void CaptureJointsToBaseState( const char *pBaseStateName );

	// Finds a base state by name, returns NULL if not found
	CDmeTransformList *FindBaseState( const char *pBaseStateName );

	// Recursively render the Dag hierarchy
	virtual void Draw( CDmeDrawSettings *pDrawSettings = NULL );

	// Set if Z is the up axis of the model
	void ZUp( bool bYUp );

	// Returns true if the DmeModel is Z Up.
	bool IsZUp() const;

protected:
	// The order in which the joint transform names appear in this list
	// indicates the joint index for each dag
	CDmaElementArray<CDmeTransform> m_JointTransforms;

	// Stores a list of base poses for all the joint transforms
	CDmaElementArray<CDmeTransformList> m_BaseStates;

private:
	enum SetupBoneRetval_t
	{
		NO_SKIN_DATA = 0,
		TOO_MANY_BONES,
		BONES_SET_UP
	};

	// Sets up the render state for the model
	SetupBoneRetval_t SetupBoneMatrixState( const matrix3x4_t& shapeToWorld, bool bForceSoftwareSkin );

	// Loads up joint transforms for this model
	void LoadJointTransform( CDmeDag *pJoint, CDmeTransformList *pBindPose, const matrix3x4_t &parentToWorld, const matrix3x4_t &parentToBindPose, bool bSetHardwareState );

	// Sets up the render state for the model
	static matrix3x4_t *SetupModelRenderState( const matrix3x4_t& shapeToWorld, bool bHasSkinningData, bool bForceSoftwareSkin );
	static void CleanupModelRenderState();

	// Stack of DmeModels currently being rendered. Used to set up render state
	static CUtlStack< CDmeModel * > s_ModelStack;

	friend class CDmeMesh;
};


#endif // DMEMODEL_H
