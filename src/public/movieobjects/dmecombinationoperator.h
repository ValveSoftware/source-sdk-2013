//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Operators that generate combinations
//
//=============================================================================

#ifndef DMECOMBINATIONOPERATOR_H
#define DMECOMBINATIONOPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "movieobjects/dmeoperator.h"
#include "movieobjects/dmeexpressionoperator.h"
#include "datamodel/dmehandle.h"


//-----------------------------------------------------------------------------
// Expression operator
//-----------------------------------------------------------------------------
class CDmeChannel;
class CDmeDag;
class CDmElement;
class CDmeChannelsClip;
class CDmeShape;


//-----------------------------------------------------------------------------
// Control handles
//-----------------------------------------------------------------------------
typedef int ControlIndex_t;


//-----------------------------------------------------------------------------
// Basic version..
//-----------------------------------------------------------------------------
class CDmeCombinationInputControl : public CDmElement
{
	DEFINE_ELEMENT( CDmeCombinationInputControl, CDmElement );

public:
	virtual void OnElementUnserialized();

	// Adds a control, returns the control index, 
	// returns true if remapped control lists need updating
	bool AddRawControl( const char *pRawControlName );

	// Removes controls
	// returns true if remapped control lists need updating
	bool RemoveRawControl( const char *pRawControlName );
	void RemoveAllRawControls();

	// Iterates remapped controls
	int RawControlCount() const;
	const char *RawControlName( int nIndex ) const;

	// Do we have a raw control?
	bool HasRawControl( const char *pRawControlName ) const;

	// Reordering controls
	void MoveRawControlUp( const char *pRawControlName );
	void MoveRawControlDown( const char *pRawControlName );

	// Is this control a stereo control?
	bool IsStereo() const;
	void SetStereo( bool bStereo );

	// Is this control an eyelid control?
	bool IsEyelid() const;
	void SetEyelid( bool bEyelid );

	// Returns the name of the eyeball
	const char *GetEyesUpDownFlexName() const;

	// Returns the wrinkle scale for a particular control
	float WrinkleScale( const char *pRawControlName );
	float WrinkleScale( int nIndex );
	void SetWrinkleScale( const char *pRawControlName, float flWrinkleScale );

	float GetDefaultValue() const;
	float GetBaseValue() const;

private:
	int FindRawControl( const char *pRawControlName );

	CDmaStringArray m_RawControlNames;
	CDmaVar<bool> m_bIsStereo;
	CDmaVar< bool > m_bIsEyelid;

	// FIXME! Remove soon! Used to autogenerate wrinkle deltas
	CDmaArray< float > m_WrinkleScales;
};


//-----------------------------------------------------------------------------
// Basic version..
//-----------------------------------------------------------------------------
class CDmeCombinationDominationRule : public CDmElement
{
	DEFINE_ELEMENT( CDmeCombinationDominationRule, CDmElement );

public:
	// Methods of IDmElement
	virtual void	OnAttributeChanged( CDmAttribute *pAttribute );

	// Adds a dominating control
	void AddDominator( const char *pDominatorControl );

	// Add a suppressed control
	void AddSuppressed( const char *pSuppressedControl );

	// Remove all dominatior + suppressed controls
	void RemoveAllDominators();
	void RemoveAllSuppressed();

	// Iteration
	int DominatorCount() const;
	const char *GetDominator( int i ) const;

	int SuppressedCount() const;
	const char *GetSuppressed( int i ) const;

	// Search
	bool HasDominatorControl( const char *pDominatorControl ) const;
	bool HasSuppressedControl( const char *pSuppressedControl ) const;

private:
	bool HasString( const char *pString, const CDmaStringArray& attr );

	CDmaStringArray m_Dominators;
	CDmaStringArray m_Suppressed;
};


//-----------------------------------------------------------------------------
// Basic version.. needs channels to copy the data out of its output attributes
//-----------------------------------------------------------------------------
enum CombinationControlType_t
{
	COMBO_CONTROL_FIRST = 0,

	COMBO_CONTROL_NORMAL = 0,
	COMBO_CONTROL_LAGGED,

	COMBO_CONTROL_TYPE_COUNT,
};

class CDmeCombinationOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeCombinationOperator, CDmeOperator );

public:
	// Methods of IDmElement
	virtual void	OnAttributeChanged( CDmAttribute *pAttribute );

	virtual void OnElementUnserialized();

	// Adds a control, returns the control index. Also adds a raw control with the same name to this control
	ControlIndex_t FindOrCreateControl( const char *pControlName, bool bStereo, bool bAutoAddRawControl = false );

	// Finds the index of the control with the specified name
	ControlIndex_t FindControlIndex( const char *pControlName );

	// Changes a control's name
	void SetControlName( ControlIndex_t nControl, const char *pControlName );

	// Removes a control
	void RemoveControl( const char *pControlName );
	void RemoveAllControls();

	// Adds a remapped control to a input control
	void AddRawControl( ControlIndex_t nControl, const char *pRawControlName );

	// Removes an remapped control from a control
	void RemoveRawControl( ControlIndex_t nControl, const char *pRawControlName );
	void RemoveAllRawControls( ControlIndex_t nControl );

	// Iterates output controls associated with an input control
	int GetRawControlCount( ControlIndex_t nControl ) const;
	const char *GetRawControlName( ControlIndex_t nControl, int nIndex ) const;
	float GetRawControlWrinkleScale( ControlIndex_t nControl, int nIndex ) const;
	float GetRawControlWrinkleScale( ControlIndex_t nControl, const char *pRawControlName ) const;

	// Iterates a global list of output controls
	int GetRawControlCount( ) const;
	const char *GetRawControlName( int nIndex ) const;
	float GetRawControlWrinkleScale( int nIndex ) const;
	bool IsStereoRawControl( int nIndex ) const;
	bool IsEyelidRawControl( int nIndex ) const;

	// Gets Input Control Default & Base Values
	float GetControlDefaultValue( ControlIndex_t nControl ) const;
	float GetControlBaseValue( ControlIndex_t nControl ) const;

	// Do we have a raw control?
	bool HasRawControl( const char *pRawControlName ) const;

	// Sets the wrinkle scale for a particular raw control
	void SetWrinkleScale( ControlIndex_t nControlIndex, const char *pRawControlName, float flWrinkleScale );

	// Sets the value of a control
	void SetControlValue( ControlIndex_t nControlIndex, float flValue, CombinationControlType_t type = COMBO_CONTROL_NORMAL );

	// Sets the value of a stereo control
	void SetControlValue( ControlIndex_t nControlIndex, float flLevel, float flBalance, CombinationControlType_t type = COMBO_CONTROL_NORMAL );
	void SetControlValue( ControlIndex_t nControlIndex, const Vector2D& vec, CombinationControlType_t type = COMBO_CONTROL_NORMAL );

	// Returns true if a control is a stereo control
	void SetStereoControl( ControlIndex_t nControlIndex, bool bIsStereo );
	bool IsStereoControl( ControlIndex_t nControlIndex ) const;

	// Sets the level of a control (only used by controls w/ 3 or more remappings)
	void SetMultiControlLevel( ControlIndex_t nControlIndex, float flLevel, CombinationControlType_t type = COMBO_CONTROL_NORMAL );
	float GetMultiControlLevel( ControlIndex_t nControlIndex, CombinationControlType_t type = COMBO_CONTROL_NORMAL ) const;

	// Reordering controls
	void MoveControlUp( const char *pControlName );
	void MoveControlDown( const char *pControlName );
	void MoveControlBefore( const char *pDragControlName, const char *pDropControlName );
	void MoveControlAfter( const char *pDragControlName, const char *pDropControlName );

	void MoveRawControlUp( ControlIndex_t nControlIndex, const char *pRawControlName );
	void MoveRawControlDown( ControlIndex_t nControlIndex, const char *pRawControlName );

	// Returns true if a control is a multi control (a control w/ 3 or more remappings)
	bool IsMultiControl( ControlIndex_t nControlIndex ) const;

	// Returns true if a control is a multi-control which controls eyelids
	void SetEyelidControl( ControlIndex_t nControlIndex, bool bIsEyelid );
	bool IsEyelidControl( ControlIndex_t nControlIndex ) const;
	const char *GetEyesUpDownFlexName( ControlIndex_t nControlIndex ) const;

	// Sets the value of a control
	float GetControlValue( ControlIndex_t nControlIndex, CombinationControlType_t type = COMBO_CONTROL_NORMAL ) const;
	const Vector2D& GetStereoControlValue( ControlIndex_t nControlIndex, CombinationControlType_t type = COMBO_CONTROL_NORMAL ) const;

	// Iterates controls
	int GetControlCount() const;
	const char *GetControlName( ControlIndex_t i ) const;

	// Attaches a channel to an input
	void AttachChannelToControlValue( ControlIndex_t nControlIndex, CombinationControlType_t type, CDmeChannel *pChannel );

	// Adds a domination rule. Domination rules are specified using raw control names
	CDmeCombinationDominationRule *AddDominationRule( );
	CDmeCombinationDominationRule *AddDominationRule( int nDominatorCount, const char **ppDominatorOutputControlNames, int nSuppressedCount, const char **ppSuppressedOutputControlNames );
	CDmeCombinationDominationRule *AddDominationRule( const CUtlVector< const char * > dominators, const CUtlVector< const char * > suppressed );
	CDmeCombinationDominationRule *AddDominationRule( CDmeCombinationDominationRule *pSrcRule );

	// Removes a domination rule
	void RemoveDominationRule( int nIndex );
	void RemoveDominationRule( CDmeCombinationDominationRule *pRule );
	void RemoveAllDominationRules();

	// Iteration
	int DominationRuleCount() const;
	CDmeCombinationDominationRule *GetDominationRule( int i );

	// Rule reordering
	void MoveDominationRuleUp( CDmeCombinationDominationRule* pRule );
	void MoveDominationRuleDown( CDmeCombinationDominationRule* pRule );

	// Indicates we're using lagged control values
	void UsingLaggedData( bool bEnable );
	bool IsUsingLaggedData() const;

	// Adds a target model/arbitrary element to perform the combinations on
	// The arbitrary element must have two attributes
	// "deltaStates", which is an array of elements
	// "deltaStateWeights", which is an array of floats
	// In the case of the model, it will look for all shapes in the dag hierarchy
	// and attempt to add that shape as a target
	// NOTE: Targets are not saved
	void AddTarget( CDmeDag *pDag );
	void AddTarget( CDmElement *pElement );
	void RemoveAllTargets();

	// Used by studiomdl to discover the various combination rules
	int GetOperationTargetCount() const;
	CDmElement *GetOperationTarget( int nTargetIndex );
	int GetOperationCount( int nTargetIndex ) const;
	CDmElement *GetOperationDeltaState( int nTargetIndex, int nOpIndex );
	const CUtlVector< int > &GetOperationControls( int nTargetIndex, int nOpIndex ) const;
	int GetOperationDominatorCount( int nTargetIndex, int nOpIndex ) const;
	const CUtlVector< int > &GetOperationDominator( int nTargetIndex, int nOpIndex, int nDominatorIndex ) const;

	// Does one of the targets we refer to contain a particular delta state?
	bool DoesTargetContainDeltaState( const char *pDeltaStateName );

	virtual void Operate();

	virtual void Resolve();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	// Would a particular delta state attached to this combination operator end up stero?
	bool IsDeltaStateStereo( const char *pDeltaStateName );

	void CopyControls( CDmeCombinationOperator *pSrc );

	// FIXME: Remove soon!
	// This is a very short-term solution to the problem of autogenerating
	// wrinkle data; when we have real editors we can remove it
	void GenerateWrinkleDeltas( bool bOverwrite = true );

	void SetToDefault();

	// The base values are different from the default values see CDmeCombinationInputControl
	void SetToBase();

	// Remove all controls and domination rules which are not referring to anything
	void Purge();

protected:
	void ComputeCombinationInfo( int nIndex );

private:
	typedef int RawControlIndex_t;

	struct DominatorInfo_t
	{
		CUtlVector< RawControlIndex_t > m_DominantIndices;
		CUtlVector< RawControlIndex_t > m_SuppressedIndices;
	};

	struct CombinationOperation_t
	{
		int m_nDeltaStateIndex;
		CUtlVector< RawControlIndex_t > m_ControlIndices;
		CUtlVector< RawControlIndex_t > m_DominatorIndices;
	};

	struct CombinationInfo_t
	{
		DmAttributeHandle_t m_hDestAttribute[COMBO_CONTROL_TYPE_COUNT];
		CUtlVector< CombinationOperation_t > m_Outputs;
	};

	struct RawControlInfo_t
	{
		CUtlString m_Name;
		bool m_bIsDefaultControl;
		ControlIndex_t m_InputControl;
		float m_flWrinkleScale;
		bool m_bLowerEyelid;
		Vector4D m_FilterRamp;	// [0] = point at which ramp starts going up
								// [1] = point at which ramp hits 1.0
								// [2] = point at which ramp stops holding at 1.0
								// [3] = point at which ramp starts going down
	};

	void ComputeCombinationInfo();
	void CleanUpCombinationInfo( int nIndex );
	void CleanUpCombinationInfo();

	// Is a particular remapped control stereo?
	bool IsRawControlStereo( const char *pRawControlName );

	// Determines the weighting of input controls based on the deltaState name
	int FindDeltaStateIndex( CDmAttribute *pDeltaArray, const char *pDeltaStateName );

	// Determines which combination to use based on the deltaState name
	int ParseDeltaName( const char *pDeltaStateName, int *pControlIndices );

	// Finds dominators
	void FindDominators( CombinationOperation_t& op );

	// Computes lists of dominators and suppressors
	void RebuildDominatorInfo();

	// Computes list of all remapped controls
	void RebuildRawControlList();

	// Remaps non-stereo -> stereo, stereo ->left/right
	void ComputeInternalControlValue( RawControlIndex_t nRawControlIndex, CombinationControlType_t type, Vector2D &value );

	// Computes lagged input values from non-lagged input
	void ComputeLaggedInputValues();

	// Finds the index of the remapped control with the specified name
	RawControlIndex_t FindRawControlIndex( const char *pControlName, bool bIgnoreDefaultControls = false ) const;

	// Updates the default value associated with a control
	void UpdateDefaultValue( ControlIndex_t nControlIndex );

	// Finds a domination rule
	int FindDominationRule( CDmeCombinationDominationRule *pRule );

	// Generates wrinkle deltas for a dag hierarchy
	void GenerateWrinkleDeltas( CDmeShape *pShape, bool bOverwrite );

	CDmaElementArray< CDmeCombinationInputControl > m_InputControls;
	CDmaArray< Vector > m_ControlValues[COMBO_CONTROL_TYPE_COUNT];
	CDmaVar< bool > m_bSpecifyingLaggedData;

	CDmaElementArray< CDmeCombinationDominationRule > m_Dominators;

	CDmaElementArray< CDmElement > m_Targets;

	CUtlVector< bool > m_IsDefaultValue;	// one per control value
	CUtlVector< RawControlInfo_t > m_RawControlInfo;
	CUtlVector< CombinationInfo_t > m_CombinationInfo;
	CUtlVector< DominatorInfo_t > m_DominatorInfo;

	float m_flLastLaggedComputationTime;
};


//-----------------------------------------------------------------------------
// Indicates we're using lagged control values
//-----------------------------------------------------------------------------
inline void CDmeCombinationOperator::UsingLaggedData( bool bEnable )
{
	m_bSpecifyingLaggedData = bEnable;
}

inline bool CDmeCombinationOperator::IsUsingLaggedData() const
{
	return m_bSpecifyingLaggedData;
}


//-----------------------------------------------------------------------------
// Helper method to create a lagged version of channel data from source data
//-----------------------------------------------------------------------------
void CreateLaggedVertexAnimation( CDmeChannelsClip *pClip, int nSamplesPerSec );


//-----------------------------------------------------------------------------
//
// A class used to edit combination operators in Maya.. doesn't connect to targets
//
//-----------------------------------------------------------------------------
class CDmeMayaCombinationOperator : public CDmeCombinationOperator
{
	DEFINE_ELEMENT( CDmeMayaCombinationOperator, CDmeCombinationOperator );

public:
	void AddDeltaState( const char *pDeltaStateName );
	void RemoveDeltaState( const char *pDeltaStateName );
	void RemoveAllDeltaStates();

	int FindDeltaState( const char *pDeltaStateName );

	int DeltaStateCount() const;
	const char *GetDeltaState( int nIndex ) const;
	const Vector2D& GetDeltaStateWeight( int nIndex, CombinationControlType_t type ) const;

private:
	CDmaElementArray< CDmElement > m_DeltaStates;
	CDmaArray< Vector2D > m_DeltaStateWeights[COMBO_CONTROL_TYPE_COUNT];
};


#endif // DMECOMBINATIONOPERATOR_H
