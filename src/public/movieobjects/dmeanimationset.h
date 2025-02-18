//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMEANIMATIONSET_H
#define DMEANIMATIONSET_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "movieobjects/dmephonememapping.h"
#include "movieobjects/timeutils.h"
#include "movieobjects/proceduralpresets.h"

class CDmeBookmark;

//-----------------------------------------------------------------------------
// A preset is a list of values to be applied to named controls in the animation set
//-----------------------------------------------------------------------------
class CDmePreset : public CDmElement
{
	DEFINE_ELEMENT( CDmePreset, CDmElement );

public:
	CDmaElementArray< CDmElement > &GetControlValues();	
	const CDmaElementArray< CDmElement > &GetControlValues() const;

	CDmElement *FindControlValue( const char *pControlName );
	CDmElement *FindOrAddControlValue( const char *pControlName );
	void RemoveControlValue( const char *pControlName );
	bool IsReadOnly();
	void CopyControlValuesFrom( CDmePreset *pSource );

	// See the enumeration above
	void SetProceduralPresetType( int nType );
	bool IsProcedural() const;
	int GetProceduralPresetType() const;

private:
	int FindControlValueIndex( const char *pControlName );

	CDmaElementArray< CDmElement > m_ControlValues;
	CDmaVar< int > m_nProceduralType;
};


class CDmeProceduralPresetSettings : public CDmElement
{
	DEFINE_ELEMENT( CDmeProceduralPresetSettings, CDmElement );
public:
	
	CDmaVar< float > m_flJitterScale;
	CDmaVar< float > m_flSmoothScale;
	CDmaVar< float > m_flSharpenScale;
	CDmaVar< float > m_flSoftenScale;

	CDmaVar< int > m_nJitterIterations;
	CDmaVar< int > m_nSmoothIterations;
	CDmaVar< int > m_nSharpenIterations;
	CDmaVar< int > m_nSoftenIterations;

	CDmaVar< int > m_nStaggerInterval;
};

//-----------------------------------------------------------------------------
// A class used to copy preset values from one preset group to another
//-----------------------------------------------------------------------------
class CDmePresetRemap : public CDmElement
{
	DEFINE_ELEMENT( CDmePresetRemap, CDmElement );

public:
	CDmaString m_SourcePresetGroup;

	const char *FindSourcePreset( const char *pDestPresetName );
	int GetRemapCount();
	const char *GetRemapSource( int i );
	const char *GetRemapDest( int i );
	void AddRemap( const char *pSourcePresetName, const char *pDestPresetName );
	void RemoveAll();

private:
	CDmaStringArray m_SrcPresets;
	CDmaStringArray m_DestPresets;
};


class CDmeAnimationSet;
class CDmeCombinationOperator;

//-----------------------------------------------------------------------------
// A preset group is a list of presets, with shared visibility + readonly settings
//-----------------------------------------------------------------------------
class CDmePresetGroup : public CDmElement
{
	DEFINE_ELEMENT( CDmePresetGroup, CDmElement );

public:
	CDmaElementArray< CDmePreset > &GetPresets();			// raw access to the array
	const CDmaElementArray< CDmePreset > &GetPresets() const;
	CDmePreset *FindPreset( const char *pPresetName );
	CDmePreset *FindOrAddPreset( const char *pPresetName, int nProceduralType = PROCEDURAL_PRESET_NOT );
	bool RemovePreset( CDmePreset *pPreset );
	void MovePresetUp( CDmePreset *pPreset );
	void MovePresetDown( CDmePreset *pPreset );
	void MovePresetInFrontOf( CDmePreset *pPreset, CDmePreset *pInFrontOf );
	CDmePresetRemap *GetPresetRemap();
	CDmePresetRemap *GetOrAddPresetRemap();

	CDmaVar< bool > m_bIsVisible;
	CDmaVar< bool > m_bIsReadOnly;

	// Exports this preset group to a faceposer .txt expression file
	bool ExportToTXT( const char *pFilename, CDmeAnimationSet *pAnimationSet = NULL, CDmeCombinationOperator *pComboOp = NULL ) const;

	// Exports this preset group to a faceposer .vfe expression file
	bool ExportToVFE( const char *pFilename, CDmeAnimationSet *pAnimationSet = NULL, CDmeCombinationOperator *pComboOp = NULL ) const;

private:
	int FindPresetIndex( CDmePreset *pGroupName );

	CDmaElementArray< CDmePreset > m_Presets; // "presets"
};


//-----------------------------------------------------------------------------
// The main controlbox for controlling animation 
//-----------------------------------------------------------------------------
class CDmeAnimationSet : public CDmElement
{
	DEFINE_ELEMENT( CDmeAnimationSet, CDmElement );

public:
	CDmaElementArray< CDmElement > &GetControls();			// raw access to the array
	CDmaElementArray< CDmElement > &GetSelectionGroups();	// raw access to the array
	CDmaElementArray< CDmePresetGroup > &GetPresetGroups();	// raw access to the array
	CDmaElementArray< CDmePhonemeMapping > &GetPhonemeMap();		// raw access to the array
	CDmaElementArray< CDmeOperator > &GetOperators();		// raw access to the array

	void RestoreDefaultPhonemeMap();

	CDmePhonemeMapping *FindMapping( const char *pRawPhoneme );
	CDmElement *FindControl( const char *pControlName );
	CDmElement *FindOrAddControl( const char *pControlName );

	// Methods pertaining to preset groups
	CDmePresetGroup *FindPresetGroup( const char *pGroupName );
	CDmePresetGroup *FindOrAddPresetGroup( const char *pGroupName );
	bool RemovePresetGroup( CDmePresetGroup *pPresetGroup );
	void MovePresetGroupUp( CDmePresetGroup *pPresetGroup );
	void MovePresetGroupDown( CDmePresetGroup *pPresetGroup );
	void MovePresetGroupInFrontOf( CDmePresetGroup *pPresetGroup, CDmePresetGroup *pInFrontOf );

	CDmePreset *FindOrAddPreset( const char *pGroupName, const char *pPresetName, int nProceduralType = PROCEDURAL_PRESET_NOT );
	bool RemovePreset( CDmePreset *pPreset );

	const CDmaElementArray< CDmeBookmark > &GetBookmarks() const;
	CDmaElementArray< CDmeBookmark > &GetBookmarks();

	CDmElement *FindSelectionGroup( const char *pSelectionGroupName );
	CDmElement *FindOrAddSelectionGroup( const char *pSelectionGroupName );

	virtual void OnElementUnserialized();

	void CollectOperators( CUtlVector< DmElementHandle_t > &operators );
	void AddOperator( CDmeOperator *pOperator );
	void RemoveOperator( CDmeOperator *pOperator );

	void EnsureProceduralPresets();

private:
	int FindPresetGroupIndex( CDmePresetGroup *pGroup );
	int FindPresetGroupIndex( const char *pGroupName );

	CDmaElementArray< CDmElement >			m_Controls;			// "controls"
	CDmaElementArray< CDmElement >			m_SelectionGroups;	// "selectionGroups"
	CDmaElementArray< CDmePresetGroup >		m_PresetGroups;		// "presetGroups"
	CDmaElementArray< CDmePhonemeMapping >	m_PhonemeMap;		// "phonememap"
	CDmaElementArray< CDmeOperator >		m_Operators;		// "operators"
	CDmaElementArray< CDmeBookmark >		m_Bookmarks;		// "bookmarks"

	friend class CModelPresetGroupManager;
};


//-----------------------------------------------------------------------------
// Utility methods to convert between L/R and V/B
//-----------------------------------------------------------------------------
inline void ValueBalanceToLeftRight( float *pLeft, float *pRight, float flValue, float flBalance )
{
	*pLeft = ( flBalance <= 0.5f ) ? 1.0f : ( ( 1.0f - flBalance ) / 0.5f );
	*pLeft *= flValue;
	*pRight = ( flBalance >= 0.5f ) ? 1.0f : ( flBalance / 0.5f );
	*pRight *= flValue;
}

inline void LeftRightToValueBalance( float *pValue, float *pBalance, float flLeft, float flRight, float flDefaultBalance = 0.5f )
{
	*pValue = max( flRight, flLeft );
	if ( *pValue <= 1e-6 )
	{
		// Leave target balance at input value if target == 0 and on the dest side of blending
		*pBalance = flDefaultBalance;
		return;
	}

	if ( flRight < flLeft )
	{
		*pBalance = 0.5f * flRight / flLeft;
	}
	else
	{
		*pBalance = 1.0f - ( 0.5f * flLeft / flRight );
	}
}


//-----------------------------------------------------------------------------
// A cache of preset groups to be associated with specific models
//-----------------------------------------------------------------------------
abstract_class IModelPresetGroupManager
{
public:
	virtual void AssociatePresetsWithFile( DmFileId_t fileId ) = 0;
	virtual void ApplyModelPresets( const char *pModelName, CDmeAnimationSet *pAnimationSet ) = 0;
};


extern IModelPresetGroupManager *g_pModelPresetGroupMgr;


#endif // DMEANIMATIONSET_H
