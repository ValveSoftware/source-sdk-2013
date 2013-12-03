//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GDVAR_H
#define GDVAR_H
#pragma once

#include <utlvector.h>
#include <TokenReader.h> // dvs: for MAX_STRING. Fix.


class MDkeyvalue;


enum GDIV_TYPE
{
	ivBadType = -1,
	ivAngle,
	ivTargetDest,
	ivTargetNameOrClass,
	ivTargetSrc,
	ivInteger,
	ivString,
	ivChoices,
	ivFlags,
	ivDecal,
	ivColor255,		// components are 0-255
	ivColor1,		// components are 0-1
	ivStudioModel,
	ivSprite,
	ivSound,
	ivVector,
	ivNPCClass,
	ivFilterClass,
	ivFloat,
	ivMaterial,
	ivScene,
	ivSide,			// One brush face ID.
	ivSideList,		// One or more brush face IDs, space delimited.
	ivOrigin,		// The origin of an entity, in the form "x y z".
	ivVecLine,		// An origin that draws a line back to the parent entity.
	ivAxis,			// The axis of rotation for a rotating entity, in the form "x0 y0 z0, x1 y1 z1".
	ivPointEntityClass,
	ivNodeDest,
	ivInstanceFile,			// used for hammer to know this field should display a browse button to find map files
	ivAngleNegativePitch,	// used for instance rotating when just a pitch value is present
	ivInstanceVariable,		// used for instance variables for easy hammer editing
	ivInstanceParm,			// used for instance parameter declaration

	ivMax					// count of types
};


//-----------------------------------------------------------------------------
// Defines an element in a choices/flags list. Choices values are strings;
// flags values are integers, hence the iValue and szValue members.
//-----------------------------------------------------------------------------
typedef struct
{
	unsigned long iValue;		// Bitflag value for ivFlags
	char szValue[MAX_STRING];	// String value for ivChoices
	char szCaption[MAX_STRING];	// Name of this choice
	BOOL bDefault;				// Flag set by default?
} GDIVITEM;


class GDinputvariable
{
	public:

		GDinputvariable();
		GDinputvariable( const char *szType, const char *szName );
		~GDinputvariable();

		BOOL InitFromTokens(TokenReader& tr);
		
		// functions:
		inline const char *GetName() { return m_szName; }
		inline const char *GetLongName(void) { return m_szLongName; }
		inline const char *GetDescription(void);

		inline int GetFlagCount() { return m_Items.Count(); }
		inline int GetFlagMask(int nFlag);
		inline const char *GetFlagCaption(int nFlag);
		
		inline int GetChoiceCount() { return m_Items.Count(); }
		inline const char *GetChoiceCaption(int nChoice);

		inline GDIV_TYPE GetType() { return m_eType; }
		const char *GetTypeText(void);
		
		inline void GetDefault(int *pnStore)
		{ 
			pnStore[0] = m_nDefault; 
		}

		inline void GetDefault(char *pszStore)
		{ 
			strcpy(pszStore, m_szDefault); 
		}
		
		GDIV_TYPE GetTypeFromToken(const char *pszToken);
		trtoken_t GetStoreAsFromType(GDIV_TYPE eType);

		const char *ItemStringForValue(const char *szValue);
		const char *ItemValueForString(const char *szString);

		BOOL IsFlagSet(unsigned int);
		void SetFlag(unsigned int, BOOL bSet);

		void ResetDefaults();

		void ToKeyValue(MDkeyvalue* pkv);
		void FromKeyValue(MDkeyvalue* pkv);

		inline bool IsReportable(void);
		inline bool IsReadOnly(void);

		GDinputvariable &operator =(GDinputvariable &Other);
		void Merge(GDinputvariable &Other);

		static const char *GetVarTypeName( GDIV_TYPE eType );

	private:

		// for choices/flags:
		CUtlVector<GDIVITEM> m_Items;

		static char *m_pszEmpty;

		char m_szName[MAX_IDENT];
		char m_szLongName[MAX_STRING];
		char *m_pszDescription;

		GDIV_TYPE m_eType;

		int m_nDefault;
		char m_szDefault[MAX_STRING];

		int m_nValue;
		char m_szValue[MAX_STRING];

		bool m_bReportable;
		bool m_bReadOnly;

		friend class GDclass;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GDinputvariable::GetDescription(void)
{
	if (m_pszDescription != NULL)
	{	
		return(m_pszDescription);
	}

	return(m_pszEmpty);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this variable is read only. Read only variables
//			cannot be edited in the Entity Properties dialog.
//-----------------------------------------------------------------------------
bool GDinputvariable::IsReadOnly(void)
{
	return(m_bReadOnly);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this variable should be displayed in the Entity
//			Report dialog.
//-----------------------------------------------------------------------------
bool GDinputvariable::IsReportable(void)
{
	return(m_bReportable);
}


//-----------------------------------------------------------------------------
// Returns the flag mask (eg 4096) for the flag at the given index. The
// array is packed, so it isn't just 1 >> nFlag.
//-----------------------------------------------------------------------------
int GDinputvariable::GetFlagMask(int nFlag)
{
	Assert(m_eType == ivFlags);
	return m_Items.Element(nFlag).iValue;
}


//-----------------------------------------------------------------------------
// Returns the caption text (eg "Only break on trigger") for the flag at the given index.
//-----------------------------------------------------------------------------
const char *GDinputvariable::GetFlagCaption(int nFlag)
{
	Assert(m_eType == ivFlags);
	return m_Items.Element(nFlag).szCaption;
}


//-----------------------------------------------------------------------------
// Returns the caption text (eg "Yes") for the choice at the given index.
//-----------------------------------------------------------------------------
const char *GDinputvariable::GetChoiceCaption(int nChoice)
{
	Assert(m_eType == ivChoices);
	return m_Items.Element(nChoice).szCaption;
}


#endif // GDVAR_H
