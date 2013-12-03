//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the interface to a game class as defined by the game data
//			file (FGD). Each class is type of entity that can be placed in
//			the editor and has attributes such as: keys that may be set,
//			the default size and color of the entity, inputs and outputs,
//			and any default models that are created when the entity is placed.
//
//			The game classes support multiple inheritence through aggregation
//			of properties.
//
//=============================================================================//

#ifndef GDCLASS_H
#define GDCLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "HelperInfo.h"
#include "TokenReader.h"
#include "GDVar.h"
#include "InputOutput.h"
#include "mathlib/vector.h"

class CHelperInfo;
class GameData;
class GDinputvariable;

const int GD_MAX_VARIABLES = 128;


class GDclass
{
	public:

		GDclass(void);
		~GDclass(void);

		//
		// Interface to class information:
		//
		inline const char *GetName(void) { return(m_szName); }
		inline const char *GetDescription(void);

		//
		// Reading a class from the game data file:
		//
		BOOL InitFromTokens(TokenReader& tr, GameData*);

		//
		// Interface to variable information (keys):
		//
		inline int GetVariableCount(void) { return(m_nVariables); }
		GDinputvariable *GetVariableAt(int iIndex);
		void GetHelperForGDVar( GDinputvariable *pVar, CUtlVector<const char *> *helperName );
		GDinputvariable *VarForName(const char *pszName, int *piIndex = NULL);
		BOOL AddVariable(GDinputvariable *pVar, GDclass *pBase, int iBaseIndex, int iVarIndex);
		void AddBase(GDclass *pBase);

		//
		// Interface to input information:
		//
		inline void AddInput(CClassInput *pInput);
		CClassInput *FindInput(const char *szName);
		inline int GetInputCount(void) { return(m_Inputs.Count()); }
		CClassInput *GetInput(int nIndex);

		//
		// Interface to output information:
		//
		inline void AddOutput(CClassOutput *pOutput);
		CClassOutput *FindOutput(const char *szName);
		inline int GetOutputCount(void) { return(m_Outputs.Count()); }
		CClassOutput *GetOutput(int nIndex);

		GameData *Parent;

		//
		// Interface to class attributes:
		//
		inline bool IsClass(const char *pszClass);
		inline bool IsSolidClass(void) { return(m_bSolid); }
		inline bool IsBaseClass(void) { return(m_bBase); }
		inline bool IsMoveClass(void) { return(m_bMove); }
		inline bool IsKeyFrameClass(void) { return(m_bKeyFrame); }
		inline bool IsPointClass(void) { return(m_bPoint); }
		inline bool IsNPCClass(void) { return(m_bNPC); }
		inline bool IsFilterClass(void) { return(m_bFilter); }
		inline bool IsNodeClass(void);
		static inline bool IsNodeClass(const char *pszClassName);

		inline bool ShouldSnapToHalfGrid() { return m_bHalfGridSnap; }

		inline void SetNPCClass(bool bNPC) { m_bNPC = bNPC; }
		inline void SetFilterClass(bool bFilter) { m_bFilter = bFilter; }
		inline void SetPointClass(bool bPoint) { m_bPoint = bPoint; }
		inline void SetSolidClass(bool bSolid) { m_bSolid = bSolid; }
		inline void SetBaseClass(bool bBase) { m_bBase = bBase; }
		inline void SetMoveClass(bool bMove) { m_bMove = bMove; }
		inline void SetKeyFrameClass(bool bKeyFrame) { m_bKeyFrame = bKeyFrame; }

		inline const Vector &GetMins(void) { return(m_bmins); }
		inline const Vector &GetMaxs(void) { return(m_bmaxs); }
		
		BOOL GetBoundBox(Vector& pfMins, Vector& pfMaxs);
		bool HasBoundBox() const { return m_bGotSize; }

		inline color32 GetColor(void);

		//
		// Interface to helper information:
		//
		inline void AddHelper(CHelperInfo *pHelper);
		inline int GetHelperCount(void) { return(m_Helpers.Count()); }
		CHelperInfo *GetHelper(int nIndex);

	protected:

		//
		// Parsing the game data file:
		//
		bool ParseInput(TokenReader &tr);
		bool ParseInputOutput(TokenReader &tr, CClassInputOutputBase *pInputOutput);
		bool ParseOutput(TokenReader &tr);
		bool ParseVariable(TokenReader &tr);

	private:

		bool ParseBase(TokenReader &tr);
		bool ParseColor(TokenReader &tr);
		bool ParseHelper(TokenReader &tr, char *pszHelperName);
		bool ParseSize(TokenReader &tr);
		bool ParseSpecifiers(TokenReader &tr);
		bool ParseVariables(TokenReader &tr);

		color32 m_rgbColor;					// Color of entity.

		bool m_bBase;						// Base only - not available to user.
		bool m_bSolid;						// Tied to solids only.
		bool m_bModel;						// Properties of a single model.
		bool m_bMove;						// Animatable
		bool m_bKeyFrame;					// Animation keyframe
		bool m_bPoint;						// Point class, not tied to solids.
		bool m_bNPC;						// NPC class - used for populating lists of NPC classes.
		bool m_bFilter;						// filter class - used for populating lists of filters.
		bool m_bHalfGridSnap;				// Snaps to a 1/2 grid so it can be centered on any geometry. Used for hinges, etc.

		bool m_bGotSize;					// Just for loading.
		bool m_bGotColor;

		char m_szName[MAX_IDENT];			// Name of this class.
		char *m_pszDescription;				// Description of this class, dynamically allocated.

		CUtlVector<GDinputvariable *> m_Variables;		// Variables for this class.
		int m_nVariables;								// Count of base & local variables combined.
		CUtlVector<GDclass *> m_Bases;					// List of base classes this class inherits from.

		CClassInputList m_Inputs;
		CClassOutputList m_Outputs;

		CHelperInfoList m_Helpers;			// Helpers for this class.

		//
		//	[0] = base number from Bases, or -1 if not in a base.
		//	[1] = index into base's variables
		//
		signed short m_VariableMap[GD_MAX_VARIABLES][2];

		Vector m_bmins;		// 3D minima of object (pointclass).
		Vector m_bmaxs;		// 3D maxima of object (pointclass).
};


void GDclass::AddInput(CClassInput *pInput)
{
	Assert(pInput != NULL);
	if (pInput != NULL)
	{
		m_Inputs.AddToTail(pInput);
	}
}


inline void GDclass::AddOutput(CClassOutput *pOutput)
{
	Assert(pOutput != NULL);
	if (pOutput != NULL)
	{
		m_Outputs.AddToTail(pOutput);
	}
}


inline void GDclass::AddHelper(CHelperInfo *pHelper)
{
	Assert(pHelper != NULL);
	if (pHelper != NULL)
	{
		m_Helpers.AddToTail(pHelper);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the render color of this entity class.
//-----------------------------------------------------------------------------
color32 GDclass::GetColor(void)
{
	return m_rgbColor;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a description of this entity class, or the entity class name
//			if no description exists.
//-----------------------------------------------------------------------------
const char *GDclass::GetDescription(void)
{
	if (m_pszDescription == NULL)
	{
		return(m_szName);
	}

	return(m_pszDescription);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszClass - 
//-----------------------------------------------------------------------------
bool GDclass::IsClass(const char *pszClass)
{
	Assert(pszClass != NULL);
	return(!stricmp(pszClass, m_szName));
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given classname represents an AI node class, false if not.
//-----------------------------------------------------------------------------
bool GDclass::IsNodeClass(const char *pszClassName)
{
	return((strnicmp(pszClassName, "info_node", 9) == 0) && (stricmp(pszClassName, "info_node_link") != 0));
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this is an AI node class, false if not.
//
// HACK: if this is necessary, we should have a new @NodeClass FGD specifier (or something)
//-----------------------------------------------------------------------------
bool GDclass::IsNodeClass(void)
{
	return(IsNodeClass(m_szName));
}


#endif // GDCLASS_H
