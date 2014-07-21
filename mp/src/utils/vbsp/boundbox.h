//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An axis aligned bounding box class.
//
// $NoKeywords: $
//=============================================================================//

#ifndef BOUNDBOX_H
#define BOUNDBOX_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"

#define COORD_NOTINIT	((float)(99999.0))

enum
{
	AXIS_X = 0,
	AXIS_Y,
	AXIS_Z
};

class BoundBox
{
	public:

		BoundBox(void);
		BoundBox(const Vector &mins, const Vector &maxs);
		
		void ResetBounds(void);
		inline void SetBounds(const Vector &mins, const Vector &maxs);

		void UpdateBounds(const Vector& bmins, const Vector& bmaxs);
		void UpdateBounds(const Vector& pt);
		void UpdateBounds(const BoundBox *pBox);
		void GetBoundsCenter(Vector& ptdest);
		inline void GetBounds(Vector& Mins, Vector& Maxs);

		virtual bool IsIntersectingBox(const Vector& pfMins, const Vector& pfMaxs) const;
		bool IsInsideBox(const Vector& pfMins, const Vector& pfMaxs) const;
		bool ContainsPoint(const Vector& pt) const;
		bool IsValidBox(void) const;
		void GetBoundsSize(Vector& size);
		void SnapToGrid(int iGridSize);
		void Rotate90(int axis);

		Vector bmins;
		Vector bmaxs;
}; 


//-----------------------------------------------------------------------------
// Purpose: Gets the bounding box as two vectors, a min and a max.
// Input  : Mins - Receives the box's minima.
//			Maxs - Receives the box's maxima.
//-----------------------------------------------------------------------------
void BoundBox::GetBounds(Vector &Mins, Vector &Maxs)
{
	Mins = bmins;
	Maxs = bmaxs;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the box outright, equivalent to ResetBounds + UpdateBounds.
// Input  : mins - Minima to set.
//			maxs - Maxima to set.
//-----------------------------------------------------------------------------
void BoundBox::SetBounds(const Vector &mins, const Vector &maxs)
{
	bmins = mins;
	bmaxs = maxs;
}


#endif // BOUNDBOX_H
