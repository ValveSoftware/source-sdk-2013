//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "BoundBox.h"
//#include "hammer_mathlib.h"
//#include "MapDefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


float V_rint(float f)
{
	if (f > 0.0f) {
		return (float) floor(f + 0.5f);
	} else if (f < 0.0f) {
		return (float) ceil(f - 0.5f);
	} else
		return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BoundBox::BoundBox(void)
{
	ResetBounds();
}

BoundBox::BoundBox(const Vector &mins, const Vector &maxs)
{
	bmins = mins;
	bmaxs = maxs;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the box to an uninitialized state, so that calls to UpdateBounds
//			will properly set the mins and maxs.
//-----------------------------------------------------------------------------
void BoundBox::ResetBounds(void)
{
	bmins[0] = bmins[1] = bmins[2] = COORD_NOTINIT;
	bmaxs[0] = bmaxs[1] = bmaxs[2] = -COORD_NOTINIT;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//-----------------------------------------------------------------------------
void BoundBox::UpdateBounds(const Vector& pt)
{
	if(pt[0] < bmins[0])
		bmins[0] = pt[0];
	if(pt[1] < bmins[1])
		bmins[1] = pt[1];
	if(pt[2] < bmins[2])
		bmins[2] = pt[2];

	if(pt[0] > bmaxs[0])
		bmaxs[0] = pt[0];
	if(pt[1] > bmaxs[1])
		bmaxs[1] = pt[1];
	if(pt[2] > bmaxs[2])
		bmaxs[2] = pt[2];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bmins - 
//			bmaxs - 
//-----------------------------------------------------------------------------
void BoundBox::UpdateBounds(const Vector& mins, const Vector& maxs)
{
	if(mins[0] < bmins[0])
		bmins[0] = mins[0];
	if(mins[1] < bmins[1])
		bmins[1] = mins[1];
	if(mins[2] < bmins[2])
		bmins[2] = mins[2];

	if(maxs[0] > bmaxs[0])
		bmaxs[0] = maxs[0];
	if(maxs[1] > bmaxs[1])
		bmaxs[1] = maxs[1];
	if(maxs[2] > bmaxs[2])
		bmaxs[2] = maxs[2];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pBox - 
//-----------------------------------------------------------------------------
void BoundBox::UpdateBounds(const BoundBox *pBox)
{
	UpdateBounds(pBox->bmins, pBox->bmaxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptdest - 
//-----------------------------------------------------------------------------
void BoundBox::GetBoundsCenter(Vector& ptdest)
{
	ptdest = (bmins + bmaxs)/2.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BoundBox::ContainsPoint(const Vector& pt) const
{
	for (int i = 0; i < 3; i++)
	{
		if (pt[i] < bmins[i] || pt[i] > bmaxs[i])
		{
			return(false);
		}
	}
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfMins - 
//			pfMaxs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BoundBox::IsIntersectingBox(const Vector& pfMins, const Vector& pfMaxs) const
{
	if ((bmins[0] >= pfMaxs[0]) || (bmaxs[0] <= pfMins[0]))
	{
		return(false);

	}
	if ((bmins[1] >= pfMaxs[1]) || (bmaxs[1] <= pfMins[1]))
	{
		return(false);
	}

	if ((bmins[2] >= pfMaxs[2]) || (bmaxs[2] <= pfMins[2]))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfMins - 
//			pfMaxs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BoundBox::IsInsideBox(const Vector& pfMins, const Vector& pfMaxs) const
{
	if ((bmins[0] < pfMins[0]) || (bmaxs[0] > pfMaxs[0]))
	{
		return(false);
	}

	if ((bmins[1] < pfMins[1]) || (bmaxs[1] > pfMaxs[1]))
	{
		return(false);
	}

	if ((bmins[2] < pfMins[2]) || (bmaxs[2] > pfMaxs[2]))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether this bounding box is valid, ie maxs >= mins.
//-----------------------------------------------------------------------------
bool BoundBox::IsValidBox(void) const
{
	for (int i = 0; i < 3; i++)
	{
		if (bmins[i] > bmaxs[i])
		{
			return(false);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : size - 
//-----------------------------------------------------------------------------
void BoundBox::GetBoundsSize(Vector& size)
{
	size[0] = bmaxs[0] - bmins[0];
	size[1] = bmaxs[1] - bmins[1];
	size[2] = bmaxs[2] - bmins[2];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iValue - 
//			iGridSize - 
// Output : 
//-----------------------------------------------------------------------------
static int Snap(/*int*/ float iValue, int iGridSize)
{
	return (int)(V_rint(iValue/iGridSize) * iGridSize);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iGridSize - 
//-----------------------------------------------------------------------------
void BoundBox::SnapToGrid(int iGridSize)
{
	// does not alter the size of the box .. snaps its minimal coordinates
	//  to the grid size specified in iGridSize
	Vector size;
	GetBoundsSize(size);

	for(int i = 0; i < 3; i++)
	{
		bmins[i] = (float)Snap(/* YWB (int)*/bmins[i], iGridSize);
		bmaxs[i] = bmins[i] + size[i];
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : axis - 
//-----------------------------------------------------------------------------
void BoundBox::Rotate90(int axis)
{
	int e1 = AXIS_X, e2 = AXIS_Y;

	// get bounds center first
	Vector center;
	GetBoundsCenter(center);

	switch(axis)
	{
	case AXIS_Z:
		e1 = AXIS_X;
		e2 = AXIS_Y;
		break;
	case AXIS_X:
		e1 = AXIS_Y;
		e2 = AXIS_Z;
		break;
	case AXIS_Y:
		e1 = AXIS_X;
		e2 = AXIS_Z;
		break;
	}

	float tmp1, tmp2;
	tmp1 = bmins[e1] - center[e1] + center[e2];
	tmp2 = bmaxs[e1] - center[e1] + center[e2];
	bmins[e1] = bmins[e2] - center[e2] + center[e1];
	bmaxs[e1] = bmaxs[e2] - center[e2] + center[e1];
	bmins[e2] = tmp1;
	bmaxs[e2] = tmp2;
}

