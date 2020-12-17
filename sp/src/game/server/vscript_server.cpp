//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_server.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "eventqueue.h"
#include "characterset.h"
#include "sceneentity.h"		// for exposing scene precache function
#include "isaverestore.h"
#include "gamerules.h"
#include "vscript_server.nut"
#ifdef MAPBASE_VSCRIPT
#include "world.h"
#endif

extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptEntityIterator
{
public:
#ifdef MAPBASE_VSCRIPT
	HSCRIPT GetLocalPlayer()
	{
		return ToHScript( UTIL_GetLocalPlayerOrListenServerHost() );
	}
#endif
	HSCRIPT First() { return Next(NULL); }

	HSCRIPT Next( HSCRIPT hStartEntity )
	{
		return ToHScript( gEntList.NextEnt( ToEnt( hStartEntity ) ) );
	}

	HSCRIPT CreateByClassname( const char *className )
	{
		return ToHScript( CreateEntityByName( className ) );
	}

	HSCRIPT FindByClassname( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByClassname( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByName( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByName( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindInSphere( HSCRIPT hStartEntity, const Vector &vecCenter, float flRadius )
	{
		return ToHScript( gEntList.FindEntityInSphere( ToEnt( hStartEntity ), vecCenter, flRadius ) );
	}

	HSCRIPT FindByTarget( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByTarget( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByModel( HSCRIPT hStartEntity, const char *szModelName )
	{
		return ToHScript( gEntList.FindEntityByModel( ToEnt( hStartEntity ), szModelName ) );
	}

	HSCRIPT FindByNameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByNameWithin( HSCRIPT hStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameWithin( HSCRIPT hStartEntity , const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}
#ifdef MAPBASE_VSCRIPT
	HSCRIPT FindByClassnameWithinBox( HSCRIPT hStartEntity , const char *szName, const Vector &vecMins, const Vector &vecMaxs )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecMins, vecMaxs ) );
	}
#endif
private:
} g_ScriptEntityIterator;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptEntityIterator, "CEntities", SCRIPT_SINGLETON "The global list of entities" )
#ifdef MAPBASE_VSCRIPT
	DEFINE_SCRIPTFUNC( GetLocalPlayer, "Get local player or listen server host" )
#endif
	DEFINE_SCRIPTFUNC( First, "Begin an iteration over the list of entities" )
	DEFINE_SCRIPTFUNC( Next, "Continue an iteration over the list of entities, providing reference to a previously found entity" )
	DEFINE_SCRIPTFUNC( CreateByClassname, "Creates an entity by classname" )
	DEFINE_SCRIPTFUNC( FindByClassname, "Find entities by class name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByName, "Find entities by name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindInSphere, "Find entities within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByTarget, "Find entities by targetname. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByModel, "Find entities by model name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByNameNearest, "Find entities by name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByNameWithin, "Find entities by name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassnameNearest, "Find entities by class name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByClassnameWithin, "Find entities by class name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
#ifdef MAPBASE_VSCRIPT
	DEFINE_SCRIPTFUNC( FindByClassnameWithinBox, "Find entities by class name within an AABB. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
#endif
END_SCRIPTDESC();

#ifndef MAPBASE_VSCRIPT // Mapbase adds this to the base library so that CScriptKeyValues can be accessed anywhere, like VBSP.
// ----------------------------------------------------------------------------
// KeyValues access - CBaseEntity::ScriptGetKeyFromModel returns root KeyValues
// ----------------------------------------------------------------------------

BEGIN_SCRIPTDESC_ROOT( CScriptKeyValues, "Wrapper class over KeyValues instance" )
	DEFINE_SCRIPT_CONSTRUCTOR()	
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindKey, "FindKey", "Given a KeyValues object and a key name, find a KeyValues object associated with the key name" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstSubKey, "GetFirstSubKey", "Given a KeyValues object, return the first sub key object" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNextKey, "GetNextKey", "Given a KeyValues object, return the next key object in a sub key group" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueInt, "GetKeyInt", "Given a KeyValues object and a key name, return associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueFloat, "GetKeyFloat", "Given a KeyValues object and a key name, return associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueBool, "GetKeyBool", "Given a KeyValues object and a key name, return associated bool value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueString, "GetKeyString", "Given a KeyValues object and a key name, return associated string value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsKeyValueEmpty, "IsKeyEmpty", "Given a KeyValues object and a key name, return true if key name has no value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptReleaseKeyValues, "ReleaseKeyValues", "Given a root KeyValues object, release its contents" );
END_SCRIPTDESC();

HSCRIPT CScriptKeyValues::ScriptFindKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetFirstSubKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetFirstSubKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetNextKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetNextKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

int CScriptKeyValues::ScriptGetKeyValueInt( const char *pszName )
{
	int i = m_pKeyValues->GetInt( pszName );
	return i;
}

float CScriptKeyValues::ScriptGetKeyValueFloat( const char *pszName )
{
	float f = m_pKeyValues->GetFloat( pszName );
	return f;
}

const char *CScriptKeyValues::ScriptGetKeyValueString( const char *pszName )
{
	const char *psz = m_pKeyValues->GetString( pszName );
	return psz;
}

bool CScriptKeyValues::ScriptIsKeyValueEmpty( const char *pszName )
{
	bool b = m_pKeyValues->IsEmpty( pszName );
	return b;
}

bool CScriptKeyValues::ScriptGetKeyValueBool( const char *pszName )
{
	bool b = m_pKeyValues->GetBool( pszName );
	return b;
}

void CScriptKeyValues::ScriptReleaseKeyValues( )
{
	m_pKeyValues->deleteThis();
	m_pKeyValues = NULL;
}


// constructors
CScriptKeyValues::CScriptKeyValues( KeyValues *pKeyValues = NULL )
{
	m_pKeyValues = pKeyValues;
}

// destructor
CScriptKeyValues::~CScriptKeyValues( )
{
	if (m_pKeyValues)
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = NULL;
}
#endif

#ifdef MAPBASE_VSCRIPT
#define RETURN_IF_CANNOT_DRAW_OVERLAY\
	if (engine->IsPaused())\
	{\
		CGWarning( 1, CON_GROUP_VSCRIPT, "debugoverlay: cannot draw while the game is paused!\n");\
		return;\
	}
class CDebugOverlayScriptHelper
{
public:

	void Box(const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddBoxOverlay(origin, mins, maxs, vec3_angle, r, g, b, a, flDuration);
		}
	}
	void BoxDirection(const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &forward, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		QAngle f_angles = vec3_angle;
		f_angles.y = UTIL_VecToYaw(forward);

		if (debugoverlay)
		{
			debugoverlay->AddBoxOverlay(origin, mins, maxs, f_angles, r, g, b, a, flDuration);
		}
	}
	void BoxAngles(const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	void SweptBox(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle & angles, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddSweptBoxOverlay(start, end, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	void EntityBounds(HSCRIPT pEntity, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		CBaseEntity *pEnt = ToEnt(pEntity);
		if (!pEnt)
			return;

		const CCollisionProperty *pCollide = pEnt->CollisionProp();
		if (debugoverlay)
		{
			debugoverlay->AddBoxOverlay(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), pCollide->GetCollisionAngles(), r, g, b, a, flDuration);
		}
	}
	void Line(const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddLineOverlay(origin, target, r, g, b, noDepthTest, flDuration);
		}
	}
	void Triangle(const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepthTest, duration);
		}
	}
	void EntityText(int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddEntityTextOverlay(entityID, text_offset, flDuration,
				(int)clamp(r * 255.f, 0.f, 255.f), (int)clamp(g * 255.f, 0.f, 255.f), (int)clamp(b * 255.f, 0.f, 255.f),
				(int)clamp(a * 255.f, 0.f, 255.f), text);
		}
	}
	void EntityTextAtPosition(const Vector &origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddTextOverlayRGB(origin, text_offset, flDuration, r, g, b, a, "%s", text);
		}
	}
	void Grid(const Vector &vPosition)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddGridOverlay(vPosition);
		}
	}
	void Text(const Vector &origin, const char *text, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddTextOverlay(origin, flDuration, "%s", text);
		}
	}
	void ScreenText(float fXpos, float fYpos, const char *text, int r, int g, int b, int a, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		if (debugoverlay)
		{
			debugoverlay->AddScreenTextOverlay(fXpos, fYpos, flDuration, r, g, b, a, text);
		}
	}
	void Cross3D(const Vector &position, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Line( position + Vector(size,0,0), position - Vector(size,0,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,size,0), position - Vector(0,size,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,0,size), position - Vector(0,0,size), r, g, b, noDepthTest, flDuration );
	}
	void Cross3DOriented(const Vector &position, const QAngle &angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward, right, up;
		AngleVectors( angles, &forward, &right, &up );

		forward *= size;
		right *= size;
		up *= size;

		Line( position + right, position - right, r, g, b, noDepthTest, flDuration );
		Line( position + forward, position - forward, r, g, b, noDepthTest, flDuration );
		Line( position + up, position - up, r, g, b, noDepthTest, flDuration );
	}
	void DrawTickMarkedLine(const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir = (endPos - startPos);
		float	lineDist = VectorNormalize(lineDir);
		int		numTicks = lineDist / tickDist;

		Vector  upVec = Vector(0,0,4);
		Vector	sideDir;
		Vector	tickPos = startPos;
		int		tickTextCnt = 0;

		CrossProduct(lineDir, upVec, sideDir);

		Line(startPos, endPos, r, g, b, noDepthTest, flDuration);

		for (int i = 0; i<numTicks + 1; i++)
		{
			Vector tickLeft = tickPos - sideDir;
			Vector tickRight = tickPos + sideDir;

			if (tickTextCnt == tickTextDist)
			{
				char text[25];
				Q_snprintf(text, sizeof(text), "%i", i);
				Vector textPos = tickLeft + Vector(0, 0, 8);
				Line(tickLeft, tickRight, 255, 255, 255, noDepthTest, flDuration);
				Text(textPos, text, flDuration);
				tickTextCnt = 0;
			}
			else
			{
				Line(tickLeft, tickRight, r, g, b, noDepthTest, flDuration);
			}

			tickTextCnt++;

			tickPos = tickPos + (tickDist * lineDir);
		}
	}
	void HorzArrow(const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

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

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void YawArrow(const Vector &startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward = UTIL_YawToVector( yaw );
		HorzArrow( startPos, startPos + forward * length, width, r, g, b, a, noDepthTest, flDuration );
	}
	void VertArrow(const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

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

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void Axis(const Vector &position, const QAngle &angles, float size, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector xvec, yvec, zvec;
		AngleVectors( angles, &xvec, &yvec, &zvec );

		xvec = position + (size * xvec);
		yvec = position - (size * yvec);
		zvec = position + (size * zvec);

		Line( position, xvec, 255, 0, 0, noDepthTest, flDuration );
		Line( position, yvec, 0, 255, 0, noDepthTest, flDuration );
		Line( position, zvec, 0, 0, 255, noDepthTest, flDuration );
	}
	void Sphere(const Vector &center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

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
	void CircleOriented(const Vector &position, const QAngle &angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		matrix3x4_t xform;
		AngleMatrix(angles, position, xform);
		Vector xAxis, yAxis;
		MatrixGetColumn(xform, 2, xAxis);
		MatrixGetColumn(xform, 1, yAxis);
		Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	void Circle(const Vector &position, const Vector &xAxis, const Vector &yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		const unsigned int nSegments = 16;
		const float flRadStep = (M_PI*2.0f) / (float) nSegments;

		Vector vecLastPosition;
		Vector vecStart = position + xAxis * radius;
		Vector vecPosition = vecStart;

		for ( int i = 1; i <= nSegments; i++ )
		{
			vecLastPosition = vecPosition;

			float flSin, flCos;
			SinCos( flRadStep*i, &flSin, &flCos );
			vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);

			Line( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration );

			if ( a && i > 1 )
			{		
				debugoverlay->AddTriangleOverlay( vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration );
			}
		}
	}
	void SetDebugBits(HSCRIPT hEntity, int bit) // DebugOverlayBits_t
	{
		CBaseEntity *pEnt = ToEnt(hEntity);
		if (!pEnt)
			return;

		if (pEnt->m_debugOverlays & bit)
		{
			pEnt->m_debugOverlays &= ~bit;
		}
		else
		{
			pEnt->m_debugOverlays |= bit;

#ifdef AI_MONITOR_FOR_OSCILLATION
			if (pEnt->IsNPC())
			{
				pEnt->MyNPCPointer()->m_ScheduleHistory.RemoveAll();
			}
#endif//AI_MONITOR_FOR_OSCILLATION
		}
	}
	void ClearAllOverlays()
	{
		// Clear all entities of their debug overlays
		for (CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity; pEntity = gEntList.NextEnt(pEntity))
		{
			pEntity->m_debugOverlays = 0;
		}

		if (debugoverlay)
		{
			debugoverlay->ClearAllOverlays();
		}
	}

private:
} g_ScriptDebugOverlay;

BEGIN_SCRIPTDESC_ROOT(CDebugOverlayScriptHelper, SCRIPT_SINGLETON "CDebugOverlayScriptHelper")
	DEFINE_SCRIPTFUNC( Box, "Draws a world-space axis-aligned box. Specify bounds in world space." )
	DEFINE_SCRIPTFUNC( BoxDirection, "Draw box oriented to a Vector direction" )
	DEFINE_SCRIPTFUNC( BoxAngles, "Draws an oriented box at the origin. Specify bounds in local space." )
	DEFINE_SCRIPTFUNC( SweptBox, "Draws a swept box. Specify endpoints in world space and the bounds in local space." )
	DEFINE_SCRIPTFUNC( EntityBounds, "Draws bounds of an entity" )
	DEFINE_SCRIPTFUNC( Line, "Draws a line between two points" )
	DEFINE_SCRIPTFUNC( Triangle, "Draws a filled triangle. Specify vertices in world space." )
	DEFINE_SCRIPTFUNC( EntityText, "Draws text on an entity" )
	DEFINE_SCRIPTFUNC( EntityTextAtPosition, "Draw entity text overlay at a specific position" )
	DEFINE_SCRIPTFUNC( Grid, "Add grid overlay" )
	DEFINE_SCRIPTFUNC( Text, "Draws 2D text. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( ScreenText, "Draws 2D text. Specify coordinates in screen space." )
	DEFINE_SCRIPTFUNC( Cross3D, "Draws a world-aligned cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( Cross3DOriented, "Draws an oriented cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( DrawTickMarkedLine, "Draws a dashed line. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( HorzArrow, "Draws a horizontal arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( YawArrow, "Draws a arrow associated with a specific yaw. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( VertArrow, "Draws a vertical arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( Axis, "Draws an axis. Specify origin + orientation in world space." )
	DEFINE_SCRIPTFUNC( Sphere, "Draws a wireframe sphere. Specify center in world space." )
	DEFINE_SCRIPTFUNC( CircleOriented, "Draws a circle oriented. Specify center in world space." )
	DEFINE_SCRIPTFUNC( Circle, "Draws a circle. Specify center in world space." )
	DEFINE_SCRIPTFUNC( SetDebugBits, "Set debug bits on entity" )
	DEFINE_SCRIPTFUNC( ClearAllOverlays, "Clear all debug overlays at once" )
END_SCRIPTDESC();
#endif // MAPBASE_VSCRIPT


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float Time()
{
	return gpGlobals->curtime;
}

static float FrameTime()
{
	return gpGlobals->frametime;
}

#ifdef MAPBASE_VSCRIPT
static int MaxPlayers()
{
	return gpGlobals->maxClients;
}

static float IntervalPerTick()
{
	return gpGlobals->interval_per_tick;
}

static int GetLoadType()
{
	return gpGlobals->eLoadType;
}
#endif

static void SendToConsole( const char *pszCommand )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
	{
#ifdef MAPBASE
		CGMsg( 1, CON_GROUP_VSCRIPT, "Cannot execute \"%s\", no player\n", pszCommand );
#else
		DevMsg ("Cannot execute \"%s\", no player\n", pszCommand );
#endif
		return;
	}

	engine->ClientCommand( pPlayer->edict(), pszCommand );
}

static void SendToConsoleServer( const char *pszCommand )
{
	// TODO: whitelist for multiplayer
	engine->ServerCommand( UTIL_VarArgs("%s\n", pszCommand) );
}

static const char *GetMapName()
{
	return STRING( gpGlobals->mapname );
}

static const char *DoUniqueString( const char *pszBase )
{
	static char szBuf[512];
	g_pScriptVM->GenerateUniqueKey( pszBase, szBuf, ARRAYSIZE(szBuf) );
	return szBuf;
}

#ifdef MAPBASE_VSCRIPT
static int  DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#else
static void DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#endif
{
	const char *target = "", *action = "Use";
	variant_t value;

	target = STRING( AllocPooledString( pszTarget ) );

	// Don't allow them to run anything on a point_servercommand unless they're the host player. Otherwise they can ent_fire
	// and run any command on the server. Admittedly, they can only do the ent_fire if sv_cheats is on, but 
	// people complained about users resetting the rcon password if the server briefly turned on cheats like this:
	//    give point_servercommand
	//    ent_fire point_servercommand command "rcon_password mynewpassword"
	if ( gpGlobals->maxClients > 1 && V_stricmp( target, "point_servercommand" ) == 0 )
	{
		return 0;
	}

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

#ifdef MAPBASE_VSCRIPT
	return
#endif
	g_EventQueue.AddEvent( target, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}


bool DoIncludeScript( const char *pszScript, HSCRIPT hScope )
{
	if ( !VScriptRunScript( pszScript, hScope, true ) )
	{
		g_pScriptVM->RaiseException( CFmtStr( "Failed to include script \"%s\"", ( pszScript ) ? pszScript : "unknown" ) );
		return false;
	}
	return true;
}

HSCRIPT CreateProp( const char *pszEntityName, const Vector &vOrigin, const char *pszModelName, int iAnim )
{
	CBaseAnimating *pBaseEntity = (CBaseAnimating *)CreateEntityByName( pszEntityName );
	pBaseEntity->SetAbsOrigin( vOrigin );
	pBaseEntity->SetModel( pszModelName );
	pBaseEntity->SetPlaybackRate( 1.0f );

	int iSequence = pBaseEntity->SelectWeightedSequence( (Activity)iAnim );

	if ( iSequence != -1 )
	{
		pBaseEntity->SetSequence( iSequence );
	}

	return ToHScript( pBaseEntity );
}

//--------------------------------------------------------------------------------------------------
// Use an entity's script instance to add an entity IO event (used for firing events on unnamed entities from vscript)
//--------------------------------------------------------------------------------------------------
#ifdef MAPBASE_VSCRIPT
static int  DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#else
static void DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#endif
{
	const char *action = "Use";
	variant_t value;

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	CBaseEntity* pTarget = ToEnt(hTarget);

	if ( !pTarget )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "VScript error: DoEntFire was passed an invalid entity instance.\n" );
#ifdef MAPBASE_VSCRIPT
		return 0;
#else
		return;
#endif
	}

#ifdef MAPBASE_VSCRIPT
	return
#endif
	g_EventQueue.AddEvent( pTarget, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}

static float ScriptTraceLine( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	// UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, ptr );
	trace_t tr;
	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, MASK_NPCWORLDSTATIC, pLooker, COLLISION_GROUP_NONE, &tr);
	if (tr.fractionleftsolid && tr.startsolid)
	{
		return 1.0 - tr.fractionleftsolid;
	}
	else
	{
		return tr.fraction;
	}
}

#ifdef MAPBASE_VSCRIPT
static bool CancelEntityIOEvent( int event )
{
	return g_EventQueue.RemoveEvent(event);
}

static float GetEntityIOEventTimeLeft( int event )
{
	return g_EventQueue.GetTimeLeft(event);
}

// vscript_server.nut adds this to the base CConvars class
static const char *ScriptGetClientConvarValue( const char *pszConVar, int entindex )
{
	return engine->GetClientConVarValue( entindex, pszConVar );
}
#endif // MAPBASE_VSCRIPT

bool VScriptServerInit()
{
	VMPROF_START

	if( scriptmanager != NULL )
	{
		ScriptLanguage_t scriptLanguage = SL_DEFAULT;

		char const *pszScriptLanguage;
#ifdef MAPBASE_VSCRIPT
		if (GetWorldEntity()->GetScriptLanguage() != SL_NONE)
		{
			// Allow world entity to override script language
			scriptLanguage = GetWorldEntity()->GetScriptLanguage();

			// Less than SL_NONE means the script language should literally be none
			if (scriptLanguage < SL_NONE)
				scriptLanguage = SL_NONE;
		}
		else
#endif
		if ( CommandLine()->CheckParm( "-scriptlang", &pszScriptLanguage ) )
		{
			if( !Q_stricmp(pszScriptLanguage, "gamemonkey") )
			{
				scriptLanguage = SL_GAMEMONKEY;
			}
			else if( !Q_stricmp(pszScriptLanguage, "squirrel") )
			{
				scriptLanguage = SL_SQUIRREL;
			}
			else if( !Q_stricmp(pszScriptLanguage, "python") )
			{
				scriptLanguage = SL_PYTHON;
			}
#ifdef MAPBASE_VSCRIPT
			else if( !Q_stricmp(pszScriptLanguage, "lua") )
			{
				scriptLanguage = SL_LUA;
			}
#endif
			else
			{
				CGWarning( 1, CON_GROUP_VSCRIPT, "-server_script does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}

		}
		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if( g_pScriptVM )
			{
#ifdef MAPBASE_VSCRIPT
				CGMsg( 0, CON_GROUP_VSCRIPT, "VSCRIPT SERVER: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
#else
				Log( "VSCRIPT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
#endif

#ifdef MAPBASE_VSCRIPT
				// MULTIPLAYER
				// ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_PlayerByIndex, "GetPlayerByIndex", "PlayerInstanceFromIndex" );
				// ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_PlayerByUserId, "GetPlayerByUserId", "GetPlayerFromUserID" );
				// ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_PlayerByName, "GetPlayerByName", "" );
				// ScriptRegisterFunctionNamed( g_pScriptVM, ScriptGetPlayerByNetworkID, "GetPlayerByNetworkID", "" );

				ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_ShowMessageAll, "ShowMessage", "Print a hud message on all clients" );
#else
				ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_ShowMessageAll, "ShowMessage", "Print a hud message on all clients" );
#endif

				ScriptRegisterFunction( g_pScriptVM, SendToConsole, "Send a string to the console as a command" );
				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLine, "TraceLine", "given 2 points & ent to ignore, return fraction along line that hits world or models" );

				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, FrameTime, "Get the time spent on the server in the last frame" );
#ifdef MAPBASE_VSCRIPT
				ScriptRegisterFunction( g_pScriptVM, SendToConsoleServer, "Send a string to the server console as a command" );
				ScriptRegisterFunction( g_pScriptVM, MaxPlayers, "Get the maximum number of players allowed on this server" );
				ScriptRegisterFunction( g_pScriptVM, IntervalPerTick, "Get the interval used between each tick" );
				ScriptRegisterFunction( g_pScriptVM, GetLoadType, "Get the way the current game was loaded (corresponds to the MapLoad enum)" );
				ScriptRegisterFunction( g_pScriptVM, DoEntFire, SCRIPT_ALIAS( "EntFire", "Generate an entity i/o event" ) );
				ScriptRegisterFunction( g_pScriptVM, DoEntFireByInstanceHandle, SCRIPT_ALIAS( "EntFireByHandle", "Generate an entity i/o event. First parameter is an entity instance." ) );
				// ScriptRegisterFunction( g_pScriptVM, IsValidEntity, "Returns true if the entity is valid." );

				ScriptRegisterFunction( g_pScriptVM, CancelEntityIOEvent, "Remove entity I/O event." );
				ScriptRegisterFunction( g_pScriptVM, GetEntityIOEventTimeLeft, "Get time left on entity I/O event." );
				ScriptRegisterFunction( g_pScriptVM, ScriptGetClientConvarValue, SCRIPT_HIDE );
#else
				ScriptRegisterFunction( g_pScriptVM, DoEntFire, SCRIPT_ALIAS( "EntFire", "Generate and entity i/o event" ) );
				ScriptRegisterFunctionNamed( g_pScriptVM, DoEntFireByInstanceHandle, "EntFireByHandle", "Generate and entity i/o event. First parameter is an entity instance." );
#endif
				ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string. Useful for adding data to tables when not sure what keys are already in use in that table." ) );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCreateSceneEntity, "CreateSceneEntity", "Create a scene entity to play the specified scene." );
#ifndef MAPBASE_VSCRIPT
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Box, "DebugDrawBox", "Draw a debug overlay box" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Line, "DebugDrawLine", "Draw a debug overlay box" );
#endif
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
				ScriptRegisterFunction( g_pScriptVM, CreateProp, "Create a physics prop" );

				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

				g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );
#ifdef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterInstance( &g_ScriptDebugOverlay, "debugoverlay" );
#endif // MAPBASE_VSCRIPT

#ifdef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterAllClasses();
				g_pScriptVM->RegisterAllEnums();

				IGameSystem::RegisterVScriptAllSystems();

				RegisterSharedScriptConstants();
				RegisterSharedScriptFunctions();
#endif

				if (scriptLanguage == SL_SQUIRREL)
				{
					g_pScriptVM->Run( g_Script_vscript_server );
				}

				VScriptRunScript( "mapspawn", false );

#ifdef MAPBASE_VSCRIPT
				// Since the world entity spawns before VScript is initted, RunVScripts() is called before the VM has started, so no scripts are run.
				// This gets around that by calling the same function right after the VM is initted.
				GetWorldEntity()->RunVScripts();
#endif

				VMPROF_SHOW( pszScriptLanguage, "virtual machine startup" );

				return true;
			}
			else
			{
				CGWarning( 1, CON_GROUP_VSCRIPT, "VM Did not start!\n" );
			}
		}
	}
	else
	{
		CGMsg( 0, CON_GROUP_VSCRIPT, "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptServerTerm()
{
	if( g_pScriptVM != NULL )
	{
		if( g_pScriptVM )
		{
			scriptmanager->DestroyVM( g_pScriptVM );
			g_pScriptVM = NULL;
		}
	}
}


bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	HSCRIPT hReplaceClosuresFunc = g_pScriptVM->LookupFunction( "__ReplaceClosures" );
	if ( !hReplaceClosuresFunc )
	{
		return false;
	}
	HSCRIPT hNewScript =  VScriptCompileScript( pszScriptName, bWarnMissing );
	if ( !hNewScript )
	{
		return false;
	}

	g_pScriptVM->Call( hReplaceClosuresFunc, NULL, true, NULL, hNewScript, hScope );
	return true;
}

CON_COMMAND( script_reload_code, "Execute a vscript file, replacing existing functions with the functions in the run script" )
{
	if ( !*args[1] )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}

	VScriptServerReplaceClosures( args[1], NULL, true );
}

CON_COMMAND( script_reload_entity_code, "Execute all of this entity's VScripts, replacing existing functions with the functions in the run scripts" )
{
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszVScripts != NULL_STRING )
		{
			char szScriptsList[255];
			Q_strcpy( szScriptsList, STRING(pEntity->m_iszVScripts) );
			CUtlStringList szScripts;
			V_SplitString( szScriptsList, " ", szScripts);

			for( int i = 0 ; i < szScripts.Count() ; i++ )
			{
				VScriptServerReplaceClosures( szScripts[i], pEntity->m_ScriptScope, true );
			}
		}
	}
}

CON_COMMAND( script_reload_think, "Execute an activation script, replacing existing functions with the functions in the run script" )
{
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszScriptThinkFunction != NULL_STRING )
		{
			VScriptServerReplaceClosures( STRING(pEntity->m_iszScriptThinkFunction), pEntity->m_ScriptScope, true );
		}
	}
}

class CVScriptGameSystem : public CAutoGameSystemPerFrame
{
public:
	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void )
	{
		m_bAllowEntityCreationInScripts = true;
		VScriptServerInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
	}

	virtual void LevelShutdownPostEntity( void )
	{
		VScriptServerTerm();
	}

	virtual void FrameUpdatePostEntityThink() 
	{ 
		if ( g_pScriptVM )
			g_pScriptVM->Frame( gpGlobals->frametime );
	}

	bool m_bAllowEntityCreationInScripts;
};

CVScriptGameSystem g_VScriptGameSystem;

#ifdef MAPBASE_VSCRIPT
ConVar script_allow_entity_creation_midgame( "script_allow_entity_creation_midgame", "1", FCVAR_NOT_CONNECTED, "Allows VScript files to create entities mid-game, as opposed to only creating entities on startup." );
#endif

bool IsEntityCreationAllowedInScripts( void )
{
#ifdef MAPBASE_VSCRIPT
	if (script_allow_entity_creation_midgame.GetBool())
		return true;
#endif

	return g_VScriptGameSystem.m_bAllowEntityCreationInScripts;
}

static short VSCRIPT_SERVER_SAVE_RESTORE_VERSION = 2;


//-----------------------------------------------------------------------------

class CVScriptSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	CVScriptSaveRestoreBlockHandler() :
		m_InstanceMap( DefLessFunc(const char *) )
	{
	}
	const char *GetBlockName()
	{
		return "VScriptServer";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->StartBlock();

		int temp = g_pScriptVM != NULL;
		pSave->WriteInt( &temp );
		if ( g_pScriptVM )
		{
			temp = g_pScriptVM->GetLanguage();
			pSave->WriteInt( &temp );
			CUtlBuffer buffer;
			g_pScriptVM->WriteState( &buffer );
			temp = buffer.TellPut();
			pSave->WriteInt( &temp );
			if ( temp > 0 )
			{
				pSave->WriteData( (const char *)buffer.Base(), temp );
			}
		}

		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( !m_fDoLoad && g_pScriptVM )
		{
			return;
		}
		CBaseEntity *pEnt = gEntList.FirstEnt();
		while ( pEnt )
		{
			if ( pEnt->m_iszScriptId != NULL_STRING )
			{
#ifndef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterClass( pEnt->GetScriptDesc() );
#endif
				m_InstanceMap.Insert( STRING( pEnt->m_iszScriptId ), pEnt );
			}
			pEnt = gEntList.NextEnt( pEnt );
		}

		pRestore->StartBlock();
		if ( pRestore->ReadInt() && pRestore->ReadInt() == g_pScriptVM->GetLanguage() )
		{
			int nBytes = pRestore->ReadInt();
			if ( nBytes > 0 )
			{
				CUtlBuffer buffer;
				buffer.EnsureCapacity( nBytes );
				pRestore->ReadData( (char *)buffer.AccessForDirectRead( nBytes ), nBytes, 0 );
				g_pScriptVM->ReadState( &buffer );
			}
		}
		pRestore->EndBlock();
	}

	void PostRestore( void )
	{
		for ( int i = m_InstanceMap.FirstInorder(); i != m_InstanceMap.InvalidIndex(); i = m_InstanceMap.NextInorder( i ) )
		{
			CBaseEntity *pEnt = m_InstanceMap[i];
			if ( pEnt->m_hScriptInstance )
			{
				ScriptVariant_t variant;
				if ( g_pScriptVM->GetValue( STRING(pEnt->m_iszScriptId), &variant ) && variant.m_type == FIELD_HSCRIPT )
				{
					pEnt->m_ScriptScope.Init( variant.m_hScript, false );
					pEnt->RunPrecacheScripts();
				}
			}
			else
			{
				// Script system probably has no internal references
				pEnt->m_iszScriptId = NULL_STRING;
			}
		}
		m_InstanceMap.Purge();
	}


	CUtlMap<const char *, CBaseEntity *> m_InstanceMap;

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CVScriptSaveRestoreBlockHandler g_VScriptSaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler()
{
	return &g_VScriptSaveRestoreBlockHandler;
}

//-----------------------------------------------------------------------------

bool CBaseEntityScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CBaseEntity *pEntity = (CBaseEntity *)p;
	if ( pEntity->GetEntityName() != NULL_STRING )
	{
		V_snprintf( pBuf, bufSize, "([%d] %s: %s)", pEntity->entindex(), STRING(pEntity->m_iClassname), STRING( pEntity->GetEntityName() ) );
	}
	else
	{
		V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), STRING(pEntity->m_iClassname) );
	}
	return true; 
}

void *CBaseEntityScriptInstanceHelper::BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
{
	int iEntity = g_VScriptSaveRestoreBlockHandler.m_InstanceMap.Find( pszId );
	if ( iEntity != g_VScriptSaveRestoreBlockHandler.m_InstanceMap.InvalidIndex() )
	{
		CBaseEntity *pEnt = g_VScriptSaveRestoreBlockHandler.m_InstanceMap[iEntity];
		pEnt->m_hScriptInstance = hInstance;
		return pEnt;
	}
	return NULL;
}


CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;


