//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Contains a bunch of information about editor types
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DMEEDITORTYPEDICTIONARY_H
#define DMEEDITORTYPEDICTIONARY_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"


//-----------------------------------------------------------------------------
// Contains all attributes related to a particular attribute on a particular editor type
//-----------------------------------------------------------------------------
class CDmeEditorAttributeInfo : public CDmElement
{
	DEFINE_ELEMENT( CDmeEditorAttributeInfo, CDmElement );

public:
	// Returns the attribute name this info is associated with
	const char *GetAttributeName() const;

	// Returns the widget name to create
	const char *GetWidgetName() const;

	// Sets the info for an entry in an attribute array
	void SetArrayInfo( CDmeEditorAttributeInfo *pInfo );

	// Returns the info for a entry in an attribute array, if this attribute is an array type
	CDmeEditorAttributeInfo *GetArrayInfo();

	// NOTE: The name field of the widget element is the widget type
	// The attributes of the widget element are data for the widget to initialize
	CDmaString m_Widget;
	CDmaVar< bool > m_bIsVisible;
	CDmaVar< bool > m_bIsReadOnly;
	CDmaVar< bool > m_bHideType;
	CDmaVar< bool > m_bHideValue;
	CDmaString m_Help;

private:
	// If this attribute is an array attribute, how do we edit the array entries?
	CDmaElement< CDmeEditorAttributeInfo > m_ArrayEntries;
};


//-----------------------------------------------------------------------------
// Base class for configuration for choices
//-----------------------------------------------------------------------------
class CDmeEditorChoicesInfo : public CDmeEditorAttributeInfo
{
	DEFINE_ELEMENT( CDmeEditorChoicesInfo, CDmeEditorAttributeInfo );

public:
	void SetChoiceType( const char *pChoiceType );

	// Gets the choices
	int GetChoiceCount() const;
	const char *GetChoiceString( int nIndex ) const;
	const char *GetChoiceType() const;
	bool HasChoiceType() const;

protected:
	CDmElement *CreateChoice( const char *pChoiceString );

	CDmaElementArray< CDmElement > m_Choices;
	CDmaString m_ChoiceType;
};


//-----------------------------------------------------------------------------
// A single editor type
//-----------------------------------------------------------------------------
class CDmeEditorType : public CDmElement
{
	DEFINE_ELEMENT( CDmeEditorType, CDmElement );

public:
	// Adds a editor type to be associated with a particular attribute
	void AddAttributeInfo( const char *pAttributeName, CDmeEditorAttributeInfo *pInfo );

	// Removes a editor type associated with a particular attribute
	void RemoveAttributeInfo( const char *pAttributeName );

	// Returns the editor info associated with an editor type
	CDmeEditorAttributeInfo *GetAttributeInfo( const char *pAttributeName );

	// Returns the editor info associated with a single entry in an attribute array
	CDmeEditorAttributeInfo *GetAttributeArrayInfo( const char *pAttributeName );

private:
	// Computes the actual attribute name stored in the type
	const char *GetActualAttributeName( const char *pAttributeName );
};


//-----------------------------------------------------------------------------
// A dictionary of all editor types
//-----------------------------------------------------------------------------
class CDmeEditorTypeDictionary : public CDmElement
{
	DEFINE_ELEMENT( CDmeEditorTypeDictionary, CDmElement );

public:
	void AddEditorTypesFromFile( const char *pFileName, const char *pPathID );
	void AddEditorType( CDmeEditorType *pEditorType );

	// Returns the editor info associated with an editor type
	CDmeEditorAttributeInfo *GetAttributeInfo( CDmElement *pElement, const char *pAttributeName );

	// Returns the editor info associated with a single entry in an attribute array
	CDmeEditorAttributeInfo *GetAttributeArrayInfo( CDmElement *pElement, const char *pAttributeName );

private:
	// Returns the editor type to use with an element
	CDmeEditorType *GetEditorType( CDmElement *pElement );
};


#endif // DMEEDITORTYPEDICTIONARY_H



