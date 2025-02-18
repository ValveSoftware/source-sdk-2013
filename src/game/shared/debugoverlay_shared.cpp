//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Utility functions for using debug overlays to visualize information
//			in the world.  Uses the IVDebugOverlay interface.
//
//=============================================================================//

#include "cbase.h"
#include "debugoverlay_shared.h"
#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	MAX_OVERLAY_DIST_SQR	90000000

//-----------------------------------------------------------------------------
// Purpose: Local player on the server or client
//-----------------------------------------------------------------------------
CBasePlayer *GetLocalPlayer( void )
{
#if defined( CLIENT_DLL)
	return C_BasePlayer::GetLocalPlayer();
#else
	return UTIL_GetListenServerHost();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Debug player by index
//-----------------------------------------------------------------------------
CBasePlayer *GetDebugPlayer( void )
{
#if defined( CLIENT_DLL )
	//NOTENOTE: This doesn't necessarily make sense on the client
	return GetLocalPlayer();
#else
	return UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Draw a box with no orientation
//-----------------------------------------------------------------------------
void NDebugOverlay::Box(const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration)
{
	BoxAngles( origin, mins, maxs, vec3_angle, r, g, b, a, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw box oriented to a Vector direction
//-----------------------------------------------------------------------------
void NDebugOverlay::BoxDirection(const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &orientation, int r, int g, int b, int a, float duration)
{
	// convert forward vector to angles
	QAngle f_angles = vec3_angle;
	f_angles.y = UTIL_VecToYaw( orientation );

	BoxAngles( origin, mins, maxs, f_angles, r, g, b, a, duration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw box oriented to a QAngle direction
//-----------------------------------------------------------------------------
void NDebugOverlay::BoxAngles(const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float duration)
{
	if ( debugoverlay )
	{
		debugoverlay->AddBoxOverlay( origin, mins, maxs, angles, r, g, b, a, duration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws a swept box
//-----------------------------------------------------------------------------
void NDebugOverlay::SweptBox( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle & angles, int r, int g, int b, int a, float flDuration)
{
	if ( debugoverlay )
	{
		debugoverlay->AddSweptBoxOverlay( start, end, mins, maxs, angles, r, g, b, a, flDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws a box around an entity
//-----------------------------------------------------------------------------
void NDebugOverlay::EntityBounds( const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration )
{
	const CCollisionProperty *pCollide = pEntity->CollisionProp();
	BoxAngles( pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), pCollide->GetCollisionAngles(), r, g, b, a, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draws a line from one position to another
//-----------------------------------------------------------------------------
void NDebugOverlay::Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float duration )
{
	// --------------------------------------------------------------
	// Clip the line before sending so we 
	// don't overflow the client message buffer
	// --------------------------------------------------------------
	CBasePlayer *player = GetLocalPlayer();

	if ( player == NULL )
		return;

	// Clip line that is far away
	if (((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
		((player->GetAbsOrigin() - target).LengthSqr() > MAX_OVERLAY_DIST_SQR) ) 
		return;

	// Clip line that is behind the client 
	Vector clientForward;
	player->EyeVectors( &clientForward );

	Vector toOrigin		= origin - player->GetAbsOrigin();
	Vector toTarget		= target - player->GetAbsOrigin();
 	float  dotOrigin	= DotProduct(clientForward,toOrigin);
 	float  dotTarget	= DotProduct(clientForward,toTarget);
	
	if (dotOrigin < 0 && dotTarget < 0) 
		return;

	if ( debugoverlay )
	{
		debugoverlay->AddLineOverlay( origin, target, r, g, b, noDepthTest, duration );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw triangle
//-----------------------------------------------------------------------------
void NDebugOverlay::Triangle( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration )
{
	CBasePlayer *player = GetLocalPlayer();
	if ( !player )
		return;

	// Clip triangles that are far away
	Vector to1 = p1 - player->GetAbsOrigin();
	Vector to2 = p2 - player->GetAbsOrigin();
	Vector to3 = p3 - player->GetAbsOrigin();

	if ((to1.LengthSqr() > MAX_OVERLAY_DIST_SQR) && 
		(to2.LengthSqr() > MAX_OVERLAY_DIST_SQR) && 
		(to3.LengthSqr() > MAX_OVERLAY_DIST_SQR))
	{
		return;
	}

	// Clip triangles that are behind the client 
	Vector clientForward;
	player->EyeVectors( &clientForward );
	
 	float  dot1 = DotProduct(clientForward, to1);
 	float  dot2 = DotProduct(clientForward, to2);
 	float  dot3 = DotProduct(clientForward, to3);

	if (dot1 < 0 && dot2 < 0 && dot3 < 0) 
		return;

	if ( debugoverlay )
	{
		debugoverlay->AddTriangleOverlay( p1, p2, p3, r, g, b, a, noDepthTest, duration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw entity text overlay
//-----------------------------------------------------------------------------
void NDebugOverlay::EntityText( int entityID, int text_offset, const char *text, float duration, int r, int g, int b, int a )
{
	if ( debugoverlay )
	{
		debugoverlay->AddEntityTextOverlay( entityID, text_offset, duration, 
			(int)clamp(r * 255.f,0.f,255.f), (int)clamp(g * 255.f,0.f,255.f), (int)clamp(b * 255.f,0.f,255.f), 
			(int)clamp(a * 255.f,0.f,255.f), text );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw entity text overlay at a specific position
//-----------------------------------------------------------------------------
void NDebugOverlay::EntityTextAtPosition( const Vector &origin, int text_offset, const char *text, float duration, int r, int g, int b, int a )
{
	if ( debugoverlay )
	{
		debugoverlay->AddTextOverlayRGB( origin, text_offset, duration, r, g, b, a, "%s", text );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add grid overlay 
//-----------------------------------------------------------------------------
void NDebugOverlay::Grid( const Vector &vPosition )
{
	if ( debugoverlay )
	{
		debugoverlay->AddGridOverlay( vPosition );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw debug text at a position
//-----------------------------------------------------------------------------
void NDebugOverlay::Text( const Vector &origin, const char *text, bool bViewCheck, float duration )
{
	CBasePlayer *player = GetLocalPlayer();
	
	if ( !player )
		return;

	// Clip text that is far away
	if ( ( player->GetAbsOrigin() - origin ).LengthSqr() > MAX_OVERLAY_DIST_SQR ) 
		return;

	// Clip text that is behind the client 
	Vector clientForward;
	player->EyeVectors( &clientForward );

	Vector toText	= origin - player->GetAbsOrigin();
 	float  dotPr	= DotProduct(clientForward,toText);
	
	if (dotPr < 0) 
		return;

	// Clip text that is obscured
	if (bViewCheck)
	{
		trace_t tr;
		UTIL_TraceLine(player->GetAbsOrigin(), origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);
		
		if ((tr.endpos - origin).Length() > 10)
			return;
	}

	if ( debugoverlay )
	{
		debugoverlay->AddTextOverlay( origin, duration, "%s", text );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Add debug overlay text with screen position
//-----------------------------------------------------------------------------
void NDebugOverlay::ScreenText( float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float duration )
{
	if ( debugoverlay )
	{
		debugoverlay->AddScreenTextOverlay( flXpos, flYpos, duration, r, g, b, a, text );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add debug overlay text with screen position and line offset
//-----------------------------------------------------------------------------
void NDebugOverlay::ScreenTextLine( float flXpos, float flYpos, int iLine, const char *text, int r, int g, int b, int a, float duration )
{
    if ( debugoverlay )
    {
        debugoverlay->AddScreenTextOverlay( flXpos, flYpos, iLine, duration, r, g, b, a, text );
    }
}

//-----------------------------------------------------------------------------
// Purpose: Draw a colored 3D cross of the given hull size at the given position
//-----------------------------------------------------------------------------
void NDebugOverlay::Cross3D(const Vector &position, const Vector &mins, const Vector &maxs, int r, int g, int b, bool noDepthTest, float fDuration )
{
	Vector start = mins + position;
	Vector end   = maxs + position;
	Line(start,end, r, g, b, noDepthTest,fDuration);

	start.x += (maxs.x - mins.x);
	end.x	-= (maxs.x - mins.x);
	Line(start,end, r, g, b, noDepthTest,fDuration);

	start.y += (maxs.y - mins.y);
	end.y	-= (maxs.y - mins.y);
	Line(start,end, r, g, b, noDepthTest,fDuration);

	start.x -= (maxs.x - mins.x);
	end.x	+= (maxs.x - mins.x);
	Line(start,end, r, g, b, noDepthTest,fDuration);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a colored 3D cross of the given size at the given position
//-----------------------------------------------------------------------------
void NDebugOverlay::Cross3D(const Vector &position, float size, int r, int g, int b, bool noDepthTest, float flDuration )
{
	Line( position + Vector(size,0,0), position - Vector(size,0,0), r, g, b, noDepthTest, flDuration );
	Line( position + Vector(0,size,0), position - Vector(0,size,0), r, g, b, noDepthTest, flDuration );
	Line( position + Vector(0,0,size), position - Vector(0,0,size), r, g, b, noDepthTest, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw an oriented, colored 3D cross of the given size at the given position (via a vector)
//-----------------------------------------------------------------------------
void NDebugOverlay::Cross3DOriented( const Vector &position, const QAngle &angles, float size, int r, int g, int b, bool noDepthTest, float flDuration )
{
	Vector forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	forward *= size;
	right *= size;
	up *= size;

	Line( position + right, position - right, r, g, b, noDepthTest, flDuration );
	Line( position + forward, position - forward, r, g, b, noDepthTest, flDuration );
	Line( position + up, position - up, r, g, b, noDepthTest, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw an oriented, colored 3D cross of the given size at the given position (via a matrix)
//-----------------------------------------------------------------------------
void NDebugOverlay::Cross3DOriented( const matrix3x4_t &m, float size, int c, bool noDepthTest, float flDuration )
{
	Vector forward, left, up, position;

	MatrixGetColumn( m, 0, forward );
	MatrixGetColumn( m, 1, left );
	MatrixGetColumn( m, 2, up );
	MatrixGetColumn( m, 3, position );

	forward *= size;
	left *= size;
	up *= size;

	Line( position + left, position - left, 0, c, 0, noDepthTest, flDuration );
	Line( position + forward, position - forward, c, 0, 0, noDepthTest, flDuration );
	Line( position + up, position - up, 0, 0, c, noDepthTest, flDuration );
}

//--------------------------------------------------------------------------------
// Purpose : Draw tick marks between start and end position of the given distance
//			 with text every tickTextDist steps apart. 
//--------------------------------------------------------------------------------
void NDebugOverlay::DrawTickMarkedLine(const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float duration )
{
	CBasePlayer* pPlayer = GetDebugPlayer();

	if ( !pPlayer ) 
		return;
	
	Vector	lineDir		= (endPos - startPos);
	float	lineDist	= VectorNormalize( lineDir );
	int		numTicks	= lineDist/tickDist;
	Vector	vBodyDir;
	
#if defined( CLIENT_DLL )
	AngleVectors( pPlayer->LocalEyeAngles(), &vBodyDir );
#else
	vBodyDir = pPlayer->BodyDirection2D( );
#endif

	Vector  upVec		= 4*vBodyDir;
	Vector	sideDir;
	Vector	tickPos		= startPos;
	int		tickTextCnt = 0;

	CrossProduct(lineDir, upVec, sideDir);

	// First draw the line
	Line(startPos, endPos, r,g,b,noDepthTest,duration);

	// Now draw the ticks
	for (int i=0;i<numTicks+1;i++)
	{
		// Draw tick mark
		Vector tickLeft	 = tickPos - sideDir;
		Vector tickRight = tickPos + sideDir;
		
		// Draw tick mark text
		if (tickTextCnt == tickTextDist)
		{
			char text[25];
			Q_snprintf(text,sizeof(text),"%i",i);
			Vector textPos = tickLeft + Vector(0,0,8);
			Line(tickLeft, tickRight, 255,255,255,noDepthTest,duration);
			Text( textPos, text, true, 0 );
			tickTextCnt = 0;
		}
		else
		{
			Line(tickLeft, tickRight, r,g,b,noDepthTest,duration);
		}
		
		tickTextCnt++;

		tickPos = tickPos + (tickDist * lineDir);
	}
}

//------------------------------------------------------------------------------
// Purpose : Draw crosshair on ground where player is looking
//------------------------------------------------------------------------------
void NDebugOverlay::DrawGroundCrossHairOverlay( void )
{
	CBasePlayer* pPlayer = GetDebugPlayer();

	if ( !pPlayer ) 
		return;

	// Trace a line to where player is looking
	Vector vForward;
	Vector vSource = pPlayer->EyePosition();
	pPlayer->EyeVectors( &vForward );

	trace_t tr;
	UTIL_TraceLine ( vSource, vSource + vForward * 2048, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	float dotPr = DotProduct(Vector(0,0,1),tr.plane.normal);
	if (tr.fraction != 1.0 &&  dotPr > 0.5)
	{
		tr.endpos.z += 1;
		float	scale	 = 6;
		Vector	startPos = tr.endpos + Vector (-scale,0,0);
		Vector	endPos	 = tr.endpos + Vector ( scale,0,0);
		Line(startPos, endPos, 255, 0, 0,false,0);

		startPos = tr.endpos + Vector (0,-scale,0);
		endPos	 = tr.endpos + Vector (0, scale,0);
		Line(startPos, endPos, 255, 0, 0,false,0);
	}
}

//--------------------------------------------------------------------------------
// Purpose : Draw a horizontal arrow pointing in the specified direction
//--------------------------------------------------------------------------------
void NDebugOverlay::HorzArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
{
	Vector	lineDir		= (endPos - startPos);
	VectorNormalize( lineDir );
	Vector  upVec		= Vector( 0, 0, 1 );
	Vector	sideDir;
	float   radius		= width / 2.0;

	CrossProduct(lineDir, upVec, sideDir);

	Vector p1 =	startPos - sideDir * radius;
	Vector p2 = endPos - lineDir * width - sideDir * radius;
	Vector p3 = endPos - lineDir * width - sideDir * width;
	Vector p4 = endPos;
	Vector p5 = endPos - lineDir * width + sideDir * width;
	Vector p6 = endPos - lineDir * width + sideDir * radius;
	Vector p7 =	startPos + sideDir * radius;

	// Outline the arrow
	Line(p1, p2, r,g,b,noDepthTest,flDuration);
	Line(p2, p3, r,g,b,noDepthTest,flDuration);
	Line(p3, p4, r,g,b,noDepthTest,flDuration);
	Line(p4, p5, r,g,b,noDepthTest,flDuration);
	Line(p5, p6, r,g,b,noDepthTest,flDuration);
	Line(p6, p7, r,g,b,noDepthTest,flDuration);

	if ( a > 0 )
	{
		// Fill us in with triangles
		Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration ); // Tip
		Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration ); // Shaft
		Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

		// And backfaces
		Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration ); // Tip
		Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration ); // Shaft
		Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose : Draw a horizontal arrow pointing in the specified direction by yaw value
//-----------------------------------------------------------------------------
void NDebugOverlay::YawArrow( const Vector &startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
{
	Vector forward = UTIL_YawToVector( yaw );

	HorzArrow( startPos, startPos + forward * length, width, r, g, b, a, noDepthTest, flDuration );
}

//--------------------------------------------------------------------------------
// Purpose : Draw a vertical arrow at a position
//--------------------------------------------------------------------------------
void NDebugOverlay::VertArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
{
	Vector	lineDir		= (endPos - startPos);
	VectorNormalize( lineDir );
	Vector  upVec;
	Vector	sideDir;
	float   radius		= width / 2.0;

	VectorVectors( lineDir, sideDir, upVec );

	Vector p1 =	startPos - upVec * radius;
	Vector p2 = endPos - lineDir * width - upVec * radius;
	Vector p3 = endPos - lineDir * width - upVec * width;
	Vector p4 = endPos;
	Vector p5 = endPos - lineDir * width + upVec * width;
	Vector p6 = endPos - lineDir * width + upVec * radius;
	Vector p7 =	startPos + upVec * radius;

	// Outline the arrow
	Line(p1, p2, r,g,b,noDepthTest,flDuration);
	Line(p2, p3, r,g,b,noDepthTest,flDuration);
	Line(p3, p4, r,g,b,noDepthTest,flDuration);
	Line(p4, p5, r,g,b,noDepthTest,flDuration);
	Line(p5, p6, r,g,b,noDepthTest,flDuration);
	Line(p6, p7, r,g,b,noDepthTest,flDuration);

	if ( a > 0 )
	{
		// Fill us in with triangles
		Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration ); // Tip
		Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration ); // Shaft
		Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

		// And backfaces
		Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration ); // Tip
		Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration ); // Shaft
		Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw an axis
//-----------------------------------------------------------------------------
void NDebugOverlay::Axis( const Vector &position, const QAngle &angles, float size, bool noDepthTest, float flDuration )
{
	Vector xvec, yvec, zvec;
	AngleVectors( angles, &xvec, &yvec, &zvec );
	
	xvec = position + (size * xvec);
	yvec = position - (size * yvec); // Left is positive
	zvec = position + (size * zvec);

	Line( position, xvec, 255, 0, 0, noDepthTest, flDuration );
	Line( position, yvec, 0, 255, 0, noDepthTest, flDuration );
	Line( position, zvec, 0, 0, 255, noDepthTest, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw circles to suggest a sphere
//-----------------------------------------------------------------------------
void NDebugOverlay::Sphere( const Vector &center, float radius, int r, int g, int b, bool noDepthTest, float flDuration )
{
	Vector edge, lastEdge;

	float axisSize = radius;
	Line( center + Vector( 0, 0, -axisSize ), center + Vector( 0, 0, axisSize ), r, g, b, noDepthTest, flDuration );
	Line( center + Vector( 0, -axisSize, 0 ), center + Vector( 0, axisSize, 0 ), r, g, b, noDepthTest, flDuration );
	Line( center + Vector( -axisSize, 0, 0 ), center + Vector( axisSize, 0, 0 ), r, g, b, noDepthTest, flDuration );

	lastEdge = Vector( radius + center.x, center.y, center.z );
	float angle;
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
		edge.y = center.y;
		edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

		Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

		lastEdge = edge;
	}

	lastEdge = Vector( center.x, radius + center.y, center.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = center.x;
		edge.y = radius * cosf( angle / 180.0f * M_PI ) + center.y;
		edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

		Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

		lastEdge = edge;
	}

	lastEdge = Vector( center.x, radius + center.y, center.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
		edge.y = radius * sinf( angle / 180.0f * M_PI ) + center.y;
		edge.z = center.z;

		Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

		lastEdge = edge;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw a circle whose center is at a position, facing the camera
//-----------------------------------------------------------------------------
void NDebugOverlay::Circle( const Vector &position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
{
	CBasePlayer *player = GetLocalPlayer();
	if ( player == NULL )
		return;

	Vector clientForward;
	player->EyeVectors( &clientForward );

	QAngle vecAngles;
	VectorAngles( clientForward, vecAngles );
	
	Circle( position, vecAngles, radius, r, g, b, a, bNoDepthTest, flDuration );
}


//-----------------------------------------------------------------------------
// Purpose: Draw a circle whose center is at a position and is facing a specified direction
//-----------------------------------------------------------------------------
void NDebugOverlay::Circle( const Vector &position, const QAngle &angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
{
	// Setup our transform matrix
	matrix3x4_t xform;
	AngleMatrix( angles, position, xform );
	Vector xAxis, yAxis;
	// default draws circle in the y/z plane
	MatrixGetColumn( xform, 2, xAxis );
	MatrixGetColumn( xform, 1, yAxis );
	Circle( position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration );
}

void NDebugOverlay::Circle( const Vector &position, const Vector &xAxis, const Vector &yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
{
	const unsigned int nSegments = 16;
	const float flRadStep = (M_PI*2.0f) / (float) nSegments;

	Vector vecLastPosition;
	
	// Find our first position
	// Retained for triangle fanning
	Vector vecStart = position + xAxis * radius;
	Vector vecPosition = vecStart;

	// Draw out each segment (fanning triangles if we have an alpha amount)
	for ( int i = 1; i <= nSegments; i++ )
	{
		// Store off our last position
		vecLastPosition = vecPosition;

		// Calculate the new one
		float flSin, flCos;
		SinCos( flRadStep*i, &flSin, &flCos );
		vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);

		// Draw the line
		Line( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration );

		// If we have an alpha value, then draw the fan
		if ( a && i > 1 )
		{		
			if ( debugoverlay )
			{
				debugoverlay->AddTriangleOverlay( vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration );
			}
		}
	}
}

void NDebugOverlay::Sphere( const Vector &position, const QAngle &angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
{
	// Setup our transform matrix
	matrix3x4_t xform;
	AngleMatrix( angles, position, xform );
	Vector xAxis, yAxis, zAxis;
	// default draws circle in the y/z plane
	MatrixGetColumn( xform, 0, xAxis );
	MatrixGetColumn( xform, 1, yAxis );
	MatrixGetColumn( xform, 2, zAxis );
	Circle( position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// xy plane
	Circle( position, yAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// yz plane
	Circle( position, xAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// xz plane
}
