//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "hud.h"
#include "itextmessage.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "imovehelper.h"
#include "checksum_crc.h"
#include "decals.h"
#include "iefx.h"
#include "view_scene.h"
#include "filesystem.h"
#include "model_types.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "c_te_effect_dispatch.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "view.h"
#include "ixboxsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
																						
ConVar localplayer_visionflags( "localplayer_visionflags", "0", FCVAR_DEVELOPMENTONLY );
																						
//-----------------------------------------------------------------------------
// ConVars
//-----------------------------------------------------------------------------
#ifdef _DEBUG

ConVar r_FadeProps( "r_FadeProps", "1" );

#endif
bool g_MakingDevShots = false;
extern ConVar cl_leveloverview;

//-----------------------------------------------------------------------------
// Purpose: Performs a var args printf into a static return buffer
// Input  : *format - 
//			... - 
// Output : char
//-----------------------------------------------------------------------------
char *VarArgs( const char *format, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	Q_vsnprintf (string, sizeof( string ), format,argptr);
	va_end (argptr);

	return string;	
}
	
//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity index corresponds to a player slot 
// Input  : index - 
// Output : bool
//-----------------------------------------------------------------------------
bool IsPlayerIndex( int index )
{
	return ( index >= 1 && index <= gpGlobals->maxClients ) ? true : false;
}

int GetLocalPlayerIndex( void )
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->entindex();
	else
		return 0;	// game not started yet
}

int GetLocalPlayerVisionFilterFlags( bool bWeaponsCheck /*= false */ )
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->GetVisionFilterFlags( bWeaponsCheck );
	else
		return 0;
}

bool IsLocalPlayerUsingVisionFilterFlags( int nFlags, bool bWeaponsCheck /* = false */ )
{
	int nLocalPlayerFlags = GetLocalPlayerVisionFilterFlags( bWeaponsCheck );

	if ( !bWeaponsCheck )
	{
		// We can only modify the RJ flags with normal checks that won't take the forced kill cam flags that can happen in weapon checks
		int nRJShaderFlags = nLocalPlayerFlags;
		if ( nRJShaderFlags != 0 && GameRules() && !GameRules()->AllowMapVisionFilterShaders() )
		{
			nRJShaderFlags = 0;
		}

		if ( nRJShaderFlags != localplayer_visionflags.GetInt() )
		{
			localplayer_visionflags.SetValue( nRJShaderFlags );
		}
	}

	return ( nLocalPlayerFlags & nFlags ) == nFlags;
}

bool IsLocalPlayerSpectator( void )
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->IsObserver();
	else
		return false;	// game not started yet
}

int GetSpectatorMode( void )
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->GetObserverMode();
	else
		return OBS_MODE_NONE;	// game not started yet
}

int GetSpectatorTarget( void )
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
	{
		CBaseEntity * target = player->GetObserverTarget();

		if ( target )
			return target->entindex();
		else
			return 0;
	}
	else
	{
		return  0;	// game not started yet
	}
}

int GetLocalPlayerTeam( void ) 
{ 
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	
	if ( pPlayer )
		return pPlayer->GetTeamNumber(); 
	else
		return TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: Convert angles to -180 t 180 range
// Input  : angles - 
//-----------------------------------------------------------------------------
void NormalizeAngles( QAngle& angles )
{
	int i;
	
	// Normalize angles to -180 to 180 range
	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Interpolate Euler angles using quaternions to avoid singularities
// Input  : start - 
//			end - 
//			output - 
//			frac - 
//-----------------------------------------------------------------------------
void InterpolateAngles( const QAngle& start, const QAngle& end, QAngle& output, float frac )
{
	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion( start, src );
	AngleQuaternion( end, dest );

	Quaternion result;

	// Slerp
	QuaternionSlerp( src, dest, frac, result );

	// Convert to euler
	QuaternionAngles( result, output );
}

//-----------------------------------------------------------------------------
// Purpose: Simple linear interpolation
// Input  : frac - 
//			src - 
//			dest - 
//			output - 
//-----------------------------------------------------------------------------
void InterpolateVector( float frac, const Vector& src, const Vector& dest, Vector& output )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		output[ i ] = src[ i ] + frac * ( dest[ i ] - src[ i ] );
	}
}

client_textmessage_t *TextMessageGet( const char *pName )
{ 
	return engine->TextMessageGet( pName );
}

//-----------------------------------------------------------------------------
// Purpose: ScreenHeight returns the height of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenHeight( void )
{
	int w, h;
	GetHudSize( w, h );
	return h;
}

//-----------------------------------------------------------------------------
// Purpose: ScreenWidth returns the width of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenWidth( void )
{
	int w, h;
	GetHudSize( w, h );
	return w;
}

//-----------------------------------------------------------------------------
// Purpose: Return the difference between two angles
// Input  : destAngle - 
//			srcAngle - 
// Output : float
//-----------------------------------------------------------------------------
float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	float delta;

	delta = destAngle - srcAngle;
	if ( destAngle > srcAngle )
	{
		while ( delta >= 180 )
			delta -= 360;
	}
	else
	{
		while ( delta <= -180 )
			delta += 360;
	}
	return delta;
}


float UTIL_WaterLevel( const Vector &position, float minz, float maxz )
{
	Vector midUp = position;
	midUp.z = minz;

	if ( !(UTIL_PointContents(midUp) & MASK_WATER) )
		return minz;

	midUp.z = maxz;
	if ( UTIL_PointContents(midUp) & MASK_WATER )
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff/2.0;
		if ( UTIL_PointContents(midUp) & MASK_WATER )
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}

void UTIL_Bubbles( const Vector& mins, const Vector& maxs, int count )
{
	Vector mid =  (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel( mid,  mid.z, mid.z + 1024 );
	flHeight = flHeight - mins.z;

	CPASFilter filter( mid );

	int bubbles = modelinfo->GetModelIndex( "sprites/bubble.vmt" );

	te->Bubbles( filter, 0.0,
		&mins, &maxs, flHeight, bubbles, count, 8.0 );
}

void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake )
{
	// Nothing for now
}

char TEXTURETYPE_Find( trace_t *ptr )
{
	surfacedata_t *psurfaceData = physprops->GetSurfaceData( ptr->surface.surfaceProps );

	return psurfaceData->game.material;
}

//-----------------------------------------------------------------------------
// Purpose: Make a tracer effect
//-----------------------------------------------------------------------------
void UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex, int iAttachment, float flVelocity, bool bWhiz, char *pCustomTracerName )
{
	CEffectData data;
	data.m_vStart = vecStart;
	data.m_vOrigin = vecEnd;
	data.m_hEntity = ClientEntityList().EntIndexToHandle( iEntIndex );
	data.m_flScale = flVelocity;

	// Flags
	if ( bWhiz )
	{
		data.m_fFlags |= TRACER_FLAG_WHIZ;
	}
	if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
	{
		data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
		// Stomp the start, since it's not going to be used anyway
		data.m_vStart[0] = iAttachment;
	}

	// Fire it off
	if ( pCustomTracerName )
	{
		DispatchEffect( pCustomTracerName, data );
	}
	else
	{
		DispatchEffect( "Tracer", data );
	}
}


//------------------------------------------------------------------------------
// Purpose : Creates both an decal and any associated impact effects (such
//			 as flecks) for the given iDamageType and the trace's end position
// Input   :
// Output  :
//------------------------------------------------------------------------------
void UTIL_ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	C_BaseEntity *pEntity = pTrace->m_pEnt;

	// Is the entity valid, is the surface sky?
	if ( !pEntity || (pTrace->surface.flags & SURF_SKY) )
		return;

	if (pTrace->fraction == 1.0)
		return;

	// don't decal nodraw surfaces
	if ( pTrace->surface.flags & SURF_NODRAW )
		return;

	pEntity->ImpactTrace( pTrace, iDamageType, pCustomImpactName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UTIL_PrecacheDecal( const char *name, bool preload )
{
	return effects->Draw_DecalIndexFromName( (char*)name );
}

extern short g_sModelIndexSmoke;

void UTIL_Smoke( const Vector &origin, const float scale, const float framerate )
{
	CPVSFilter filter( origin );
	te->Smoke( filter, 0.0f, &origin, g_sModelIndexSmoke, scale, framerate );
}

void UTIL_SetOrigin( C_BaseEntity *entity, const Vector &vecOrigin )
{
	entity->SetLocalOrigin( vecOrigin );
}

//#define PRECACHE_OTHER_ONCE
// UNDONE: Do we need this to avoid doing too much of this?  Measure startup times and see
#if PRECACHE_OTHER_ONCE

#include "utlsymbol.h"
class CPrecacheOtherList : public CAutoServerSystem
{
public:
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();

	bool AddOrMarkPrecached( const char *pClassname );

private:
	CUtlSymbolTable		m_list;
};

void CPrecacheOtherList::LevelInitPreEntity()
{
	m_list.RemoveAll();
}

void CPrecacheOtherList::LevelShutdownPostEntity()
{
	m_list.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: mark or add
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPrecacheOtherList::AddOrMarkPrecached( const char *pClassname )
{
	CUtlSymbol sym = m_list.Find( pClassname );
	if ( sym.IsValid() )
		return false;

	m_list.AddString( pClassname );
	return true;
}

CPrecacheOtherList g_PrecacheOtherList;
#endif

void UTIL_PrecacheOther( const char *szClassname )
{
#if PRECACHE_OTHER_ONCE
	// already done this one?, if not, mark as done
	if ( !g_PrecacheOtherList.AddOrMarkPrecached( szClassname ) )
		return;
#endif

	// Client should only do this once entities are coming down from server!!!
	// Assert( engine->IsConnected() );

	C_BaseEntity	*pEntity = CreateEntityByName( szClassname );
	if ( !pEntity )
	{
		Warning( "NULL Ent in UTIL_PrecacheOther\n" );
		return;
	}
	
	if (pEntity)
	{
		pEntity->Precache( );
	}

	// Bye bye
	pEntity->Release();
}

static csurface_t	g_NullSurface = { "**empty**", 0 };
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UTIL_SetTrace(trace_t& trace, const Ray_t& ray, C_BaseEntity *ent, float fraction, int hitgroup, unsigned int contents, const Vector& normal, float intercept )
{
	trace.startsolid = (fraction == 0.0f);
	trace.fraction = fraction;
	VectorCopy( ray.m_Start, trace.startpos );
	VectorMA( ray.m_Start, fraction, ray.m_Delta, trace.endpos );
	VectorCopy( normal, trace.plane.normal );
	trace.plane.dist = intercept;
	trace.m_pEnt = C_BaseEntity::Instance( ent );
	trace.hitgroup = hitgroup;
	trace.surface =	g_NullSurface;
	trace.contents = contents;
}

//-----------------------------------------------------------------------------
// Purpose: Get the x & y positions of a world position in screenspace
//			Returns true if it's onscreen
//-----------------------------------------------------------------------------
bool GetVectorInScreenSpace( Vector pos, int& iX, int& iY, Vector *vecOffset )
{
	Vector screen;

	// Apply the offset, if one was specified
	if ( vecOffset != NULL )
		pos += *vecOffset;

	// Transform to screen space
	int iFacing = ScreenTransform( pos, screen );
	iX = 0.5f * ( 1.0f + screen[0] ) * ScreenWidth();
	iY = 0.5f * ( 1.0f - screen[1] ) * ScreenHeight();

	// Make sure the player's facing it
	if ( iFacing )
	{
		// We're actually facing away from the Target. Stomp the screen position.
		iX = -640;
		iY = -640;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get the x & y positions of a world position in HUD space
//			Returns true if it's onscreen
//-----------------------------------------------------------------------------
bool GetVectorInHudSpace( Vector pos, int& iX, int& iY, Vector *vecOffset )
{
	Vector screen;

	// Apply the offset, if one was specified
	if ( vecOffset != NULL )
		pos += *vecOffset;

	// Transform to HUD space
	int iFacing = HudTransform( pos, screen );
	iX = 0.5f * ( 1.0f + screen[0] ) * ScreenWidth();
	iY = 0.5f * ( 1.0f - screen[1] ) * ScreenHeight();

	// Make sure the player's facing it
	if ( iFacing )
	{
		// We're actually facing away from the Target. Stomp the screen position.
		iX = -640;
		iY = -640;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get the x & y positions of an entity in screenspace
//			Returns true if it's onscreen
//-----------------------------------------------------------------------------
bool GetTargetInScreenSpace( C_BaseEntity *pTargetEntity, int& iX, int& iY, Vector *vecOffset )
{
	return GetVectorInScreenSpace( pTargetEntity->WorldSpaceCenter(), iX, iY, vecOffset );
}

//-----------------------------------------------------------------------------
// Purpose: Get the x & y positions of an entity in Vgui space
//			Returns true if it's onscreen
//-----------------------------------------------------------------------------
bool GetTargetInHudSpace( C_BaseEntity *pTargetEntity, int& iX, int& iY, Vector *vecOffset )
{
	return GetVectorInHudSpace( pTargetEntity->WorldSpaceCenter(), iX, iY, vecOffset );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			msg_dest - 
//			*msg_name - 
//			*param1 - 
//			*param2 - 
//			*param3 - 
//			*param4 - 
//-----------------------------------------------------------------------------
void ClientPrint( C_BasePlayer *player, int msg_dest, const char *msg_name, const char *param1 /*= NULL*/, const char *param2 /*= NULL*/, const char *param3 /*= NULL*/, const char *param4 /*= NULL*/ )
{
}

//-----------------------------------------------------------------------------
// class CFlaggedEntitiesEnum
//-----------------------------------------------------------------------------
// enumerate entities that match a set of edict flags into a static array
class CFlaggedEntitiesEnum : public IPartitionEnumerator
{
public:
	CFlaggedEntitiesEnum( C_BaseEntity **pList, int listMax, int flagMask );
	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
	
	int GetCount() { return m_count; }
	bool AddToList( C_BaseEntity *pEntity );
	
private:
	C_BaseEntity		**m_pList;
	int				m_listMax;
	int				m_flagMask;
	int				m_count;
};

CFlaggedEntitiesEnum::CFlaggedEntitiesEnum( C_BaseEntity **pList, int listMax, int flagMask )
{
	m_pList = pList;
	m_listMax = listMax;
	m_flagMask = flagMask;
	m_count = 0;
}

bool CFlaggedEntitiesEnum::AddToList( C_BaseEntity *pEntity )
{
	if ( m_count >= m_listMax )
		return false;
	m_pList[m_count] = pEntity;
	m_count++;
	return true;
}

IterationRetval_t CFlaggedEntitiesEnum::EnumElement( IHandleEntity *pHandleEntity )
{
	IClientEntity *pClientEntity = cl_entitylist->GetClientEntityFromHandle( pHandleEntity->GetRefEHandle() );
	C_BaseEntity *pEntity = pClientEntity ? pClientEntity->GetBaseEntity() : NULL;
	if ( pEntity )
	{
		if ( m_flagMask && !(pEntity->GetFlags() & m_flagMask) )	// Does it meet the criteria?
			return ITERATION_CONTINUE;

		if ( !AddToList( pEntity ) )
			return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: Pass in an array of pointers and an array size, it fills the array and returns the number inserted
// Input  : **pList - 
//			listMax - 
//			&mins - 
//			&maxs - 
//			flagMask - 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_EntitiesInBox( C_BaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask, int partitionMask )
{
	CFlaggedEntitiesEnum boxEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInBox( partitionMask, mins, maxs, false, &boxEnum );
	
	return boxEnum.GetCount();

}

//-----------------------------------------------------------------------------
// Purpose: Pass in an array of pointers and an array size, it fills the array and returns the number inserted
// Input  : **pList - 
//			listMax - 
//			&center - 
//			radius - 
//			flagMask - 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_EntitiesInSphere( C_BaseEntity **pList, int listMax, const Vector &center, float radius, int flagMask, int partitionMask )
{
	CFlaggedEntitiesEnum sphereEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInSphere( partitionMask, center, radius, false, &sphereEnum );

	return sphereEnum.GetCount();

}

//-----------------------------------------------------------------------------
// Purpose: Pass in an array of pointers and an array size, it fills the array and returns the number inserted
// Input  : **pList - 
//			listMax - 
//			&ray - 
//			flagMask - 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_EntitiesAlongRay( C_BaseEntity **pList, int listMax, const Ray_t &ray, int flagMask, int partitionMask )
{
	CFlaggedEntitiesEnum rayEnum( pList, listMax, flagMask );
	partition->EnumerateElementsAlongRay( partitionMask, ray, false, &rayEnum );

	return rayEnum.GetCount();
}

CEntitySphereQuery::CEntitySphereQuery( const Vector &center, float radius, int flagMask, int partitionMask )
{
	m_listIndex = 0;
	m_listCount = UTIL_EntitiesInSphere( m_pList, ARRAYSIZE(m_pList), center, radius, flagMask, partitionMask );
}

CBaseEntity *CEntitySphereQuery::GetCurrentEntity()
{
	if ( m_listIndex < m_listCount )
		return m_pList[m_listIndex];
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
const char *nexttoken(char *token, const char *str, char sep)
{
	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return(NULL);
	}

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0'))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
//			*str - 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_ComputeStringWidth( vgui::HFont& font, const char *str )
{
	float pixels = 0;
	const char *p = str;
	const char *pAfter = p + 1;
	const char *pBefore = "\0";
	while ( *p )
	{
#if USE_GETKERNEDCHARWIDTH
		float wide, abcA;
		vgui::surface()->GetKernedCharWidth( font, *p, *pBefore, *pAfter, wide, abcA );
		pixels += wide;
#else
		pixels += vgui::surface()->GetCharacterWidth( font, *p );
#endif
		pBefore = p;
		p++;
		if ( *p )
			pAfter = p + 1; 
		else
			pAfter = "\0";
	}
	return (int)ceil(pixels);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
//			*str - 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_ComputeStringWidth( vgui::HFont& font, const wchar_t *str )
{
	float pixels = 0;
	const wchar_t *p = str;
	const wchar_t *pAfter = p + 1;
	const wchar_t *pBefore = L"\0";
	while ( *p )
	{
#if USE_GETKERNEDCHARWIDTH
		float wide, abcA;
		vgui::surface()->GetKernedCharWidth( font, *p, *pBefore, *pAfter, wide, abcA );
		pixels += wide;
#else
		pixels += vgui::surface()->GetCharacterWidth( font, *p );
#endif
		pBefore = p;
		p++;
		if ( *p )
			pAfter = p + 1; 
		else
			pAfter = L"\0";
	}
	return (int)ceil(pixels);
}

//-----------------------------------------------------------------------------
// Purpose: Scans player names
//Passes the player name to be checked in a KeyValues pointer
//with the keyname "name"
// - replaces '&' with '&&' so they will draw in the scoreboard
// - replaces '#' at the start of the name with '*'
//-----------------------------------------------------------------------------

void UTIL_MakeSafeName( const char *oldName, char *newName, int newNameBufSize )
{
	Assert( newNameBufSize >= sizeof(newName[0]) );

	int newpos = 0;

	for( const char *p=oldName; *p != 0 && newpos < newNameBufSize-1; p++ )
	{
		//check for a '#' char at the beginning
		if( p == oldName && *p == '#' )
		{
			newName[newpos] = '*';
			newpos++;
		}
		else if( *p == '%' )
		{
			// remove % chars
			newName[newpos] = '*';
			newpos++;
		}
		else if( *p == '&' )
		{
			//insert another & after this one
			if ( newpos+2 < newNameBufSize )
			{
				newName[newpos] = '&';
				newName[newpos+1] = '&';
				newpos+=2;
			}
		}
		else
		{
			newName[newpos] = *p;
			newpos++;
		}
	}
	newName[newpos] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Scans player names and replaces characters that vgui won't
//          display properly
// Input  : *oldName - player name to be fixed up
// Output : *char - static buffer with the safe name
//-----------------------------------------------------------------------------

const char * UTIL_SafeName( const char *oldName )
{
	static char safeName[ MAX_PLAYER_NAME_LENGTH * 2 + 1 ];
	UTIL_MakeSafeName( oldName, safeName, sizeof( safeName ) );

	return safeName;
}


//-----------------------------------------------------------------------------
// Purpose: Looks up key bindings for commands and replaces them in string.
//			%<commandname>% will get replaced with its bound control, e.g. %attack2%
//			Input buffer sizes are in bytes rather than unicode character count
//			for consistency with other APIs.  If inbufsizebytes is 0 a NULL-terminated
//			input buffer is assumed, or you can pass the size of the input buffer if
//			not NULL-terminated.
//-----------------------------------------------------------------------------
void UTIL_ReplaceKeyBindings( const wchar_t *inbuf, int inbufsizebytes, OUT_Z_BYTECAP(outbufsizebytes) wchar_t *outbuf, int outbufsizebytes )
{
	Assert( outbufsizebytes >= sizeof(outbuf[0]) );
	// copy to a new buf if there are vars
	outbuf[0]=0;

	if ( !inbuf || !inbuf[0] )
		return;

	int pos = 0;
	const wchar_t *inbufend = NULL;
	if ( inbufsizebytes > 0 )
	{
		inbufend = inbuf + ( inbufsizebytes / 2 );
	}

	while( inbuf != inbufend && *inbuf != 0 )
	{
		// check for variables
		if ( *inbuf == '%' )
		{
			++inbuf;

			const wchar_t *end = wcschr( inbuf, '%' );
			if ( end && ( end != inbuf ) ) // make sure we handle %% in the string, which should be treated in the output as %
			{
				wchar_t token[64];
				wcsncpy( token, inbuf, end - inbuf );
				token[end - inbuf] = 0;

				inbuf += end - inbuf;

				// lookup key names
				char binding[64];
				g_pVGuiLocalize->ConvertUnicodeToANSI( token, binding, sizeof(binding) );

				const char *key = engine->Key_LookupBinding( *binding == '+' ? binding + 1 : binding );
				if ( !key )
				{
					key = IsX360() ? "" : "< not bound >";
				}

				//!! change some key names into better names
				char friendlyName[64];
				bool bAddBrackets = false;
				if ( IsX360() )
				{
					if ( !key || !key[0] )
					{
						Q_snprintf( friendlyName, sizeof(friendlyName), "#GameUI_None" );
						bAddBrackets = true;
					}
					else
					{
						Q_snprintf( friendlyName, sizeof(friendlyName), "#GameUI_KeyNames_%s", key );
					}
				}
				else
				{
					Q_snprintf( friendlyName, sizeof(friendlyName), "%s", key );
				}
				Q_strupr( friendlyName );

				wchar_t *locName = g_pVGuiLocalize->Find( friendlyName );
				if ( !locName || wcslen(locName) <= 0)
				{
					g_pVGuiLocalize->ConvertANSIToUnicode( friendlyName, token, sizeof(token) );

					outbuf[pos] = '\0';
					wcscat( outbuf, token );
					pos += wcslen(token);
				}
				else
				{
					outbuf[pos] = '\0';
					if ( bAddBrackets )
					{
						wcscat( outbuf, L"[" );
						pos += 1;
					}
					wcscat( outbuf, locName );
					pos += wcslen(locName);
					if ( bAddBrackets )
					{
						wcscat( outbuf, L"]" );
						pos += 1;
					}
				}
			}
			else
			{
				outbuf[pos] = *inbuf;
				++pos;
			}
		}
		else
		{
			outbuf[pos] = *inbuf;
			++pos;
		}

		++inbuf;
	}

	outbuf[pos] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//			*pLength - 
// Output : byte
//-----------------------------------------------------------------------------
byte *UTIL_LoadFileForMe( const char *filename, int *pLength )
{
	byte *buffer;

	FileHandle_t file;
	file = filesystem->Open( filename, "rb", "GAME" );
	if ( FILESYSTEM_INVALID_HANDLE == file )
	{
		if ( pLength ) *pLength = 0;
		return NULL;
	}

	int size = filesystem->Size( file );
	buffer = new byte[ size + 1 ];
	if ( !buffer )
	{
		Warning( "UTIL_LoadFileForMe:  Couldn't allocate buffer of size %i for file %s\n", size + 1, filename );
		filesystem->Close( file );
		return NULL;
	}
	filesystem->Read( buffer, size, file );
	filesystem->Close( file );

	// Ensure null terminator
	buffer[ size ] =0;

	if ( pLength )
	{
		*pLength = size;
	}

	return buffer;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
//-----------------------------------------------------------------------------
void UTIL_FreeFile( byte *buffer )
{
	delete[] buffer;
}


//-----------------------------------------------------------------------------
// Compute distance fade
//-----------------------------------------------------------------------------
static unsigned char ComputeDistanceFade( C_BaseEntity *pEntity, float flMinDist, float flMaxDist )
{
	if ((flMinDist <= 0) && (flMaxDist <= 0))
		return 255;

	if( flMinDist > flMaxDist )
	{
		::V_swap( flMinDist, flMaxDist );
	}

	// If a negative value is provided for the min fade distance, then base it off the max.
	if( flMinDist < 0 )
	{
		flMinDist = flMaxDist - 400;
		if( flMinDist < 0 )
		{
			flMinDist = 0;
		}
	}

	flMinDist *= flMinDist;
	flMaxDist *= flMaxDist;

	float flCurrentDistanceSq = CurrentViewOrigin().DistToSqr( pEntity->WorldSpaceCenter() );
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( pLocal )
	{
		float flDistFactor = pLocal->GetFOVDistanceAdjustFactor();
		flCurrentDistanceSq *= flDistFactor * flDistFactor;
	}

	// If I'm inside the minimum range than don't resort to alpha trickery
	if ( flCurrentDistanceSq <= flMinDist )
		return 255;

	if ( flCurrentDistanceSq >= flMaxDist )
		return 0;

	// NOTE: Because of the if-checks above, flMinDist != flMinDist here
	float flFalloffFactor = 255.0f / (flMaxDist - flMinDist);
	int nAlpha = flFalloffFactor * (flMaxDist - flCurrentDistanceSq);
	return clamp( nAlpha, 0, 255 );
}


//-----------------------------------------------------------------------------
// Compute fade amount
//-----------------------------------------------------------------------------
unsigned char UTIL_ComputeEntityFade( C_BaseEntity *pEntity, float flMinDist, float flMaxDist, float flFadeScale )
{
	unsigned char nAlpha = 255;

	// If we're taking devshots, don't fade props at all
	if ( g_MakingDevShots || cl_leveloverview.GetFloat() > 0 )
		return 255;

#ifdef _DEBUG
	if ( r_FadeProps.GetBool() )
#endif
	{
		nAlpha = ComputeDistanceFade( pEntity, flMinDist, flMaxDist );

		// NOTE: This computation for the center + radius is invalid!
		// The center of the sphere is at the center of the OBB, which is not necessarily
		// at the render origin. But it should be close enough.
		Vector vecMins, vecMaxs;
		pEntity->GetRenderBounds( vecMins, vecMaxs );
		float flRadius = vecMins.DistTo( vecMaxs ) * 0.5f;

		Vector vecAbsCenter;
		if ( modelinfo->GetModelType( pEntity->GetModel() ) == mod_brush )
		{
			Vector vecRenderMins, vecRenderMaxs;
			pEntity->GetRenderBoundsWorldspace( vecRenderMins, vecRenderMaxs );
			VectorAdd( vecRenderMins, vecRenderMaxs, vecAbsCenter );
			vecAbsCenter *= 0.5f;
		}
		else
		{
			vecAbsCenter = pEntity->GetRenderOrigin();
		}

		unsigned char nGlobalAlpha = IsXbox() ? 255 : modelinfo->ComputeLevelScreenFade( vecAbsCenter, flRadius, flFadeScale );
		unsigned char nDistAlpha;

		if ( !engine->IsLevelMainMenuBackground() )
		{
			nDistAlpha = modelinfo->ComputeViewScreenFade( vecAbsCenter, flRadius, flFadeScale );
		}
		else
		{
			nDistAlpha = 255;
		}

		if ( nDistAlpha < nGlobalAlpha )
		{
			nGlobalAlpha = nDistAlpha;
		}

		if ( nGlobalAlpha < nAlpha )
		{
			nAlpha = nGlobalAlpha;
		}
	}

	return nAlpha;
}


//-----------------------------------------------------------------------------
// Purpose: Given a vector, clamps the scalar axes to MAX_COORD_FLOAT ranges from worldsize.h
// Input  : *pVecPos - 
//-----------------------------------------------------------------------------
void UTIL_BoundToWorldSize( Vector *pVecPos )
{
	Assert( pVecPos );
	for ( int i = 0; i < 3; ++i )
	{
		(*pVecPos)[ i ] = clamp( (*pVecPos)[ i ], MIN_COORD_FLOAT, MAX_COORD_FLOAT );
	}
}

#ifdef _X360
#define MAP_KEY_FILE_DIR	"cfg"
#else
#define MAP_KEY_FILE_DIR	"media"
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns the filename to count map loads in
//-----------------------------------------------------------------------------
bool UTIL_GetMapLoadCountFileName( const char *pszFilePrependName, char *pszBuffer, int iBuflen )
{
	if ( IsX360() )
	{
#ifdef _X360
		if ( XBX_GetStorageDeviceId() == XBX_INVALID_STORAGE_ID || XBX_GetStorageDeviceId() == XBX_STORAGE_DECLINED )
			return false;
#endif
	}

	if ( IsX360() )
	{
		Q_snprintf( pszBuffer, iBuflen, "%s:/%s", MAP_KEY_FILE_DIR, pszFilePrependName );
	}
	else
	{
		Q_snprintf( pszBuffer, iBuflen, "%s/%s", MAP_KEY_FILE_DIR, pszFilePrependName );
	}

	return true;
}

#ifdef TF_CLIENT_DLL
#define MAP_KEY_FILE "viewed.res"
#else
#define MAP_KEY_FILE "mapkeys.res"
#endif	

void UTIL_IncrementMapKey( const char *pszCustomKey )
{
	if ( !pszCustomKey )
		return;

	char szFilename[ _MAX_PATH ];
	if ( !UTIL_GetMapLoadCountFileName( MAP_KEY_FILE, szFilename, _MAX_PATH ) )
		return;

	int iCount = 1;

	KeyValues *kvMapLoadFile = new KeyValues( MAP_KEY_FILE );
	if ( kvMapLoadFile )
	{
		kvMapLoadFile->LoadFromFile( g_pFullFileSystem, szFilename, "MOD" );

		char mapname[MAX_MAP_NAME];
		Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname) );
		Q_strlower( mapname );

		// Increment existing, or add a new one
		KeyValues *pMapKey = kvMapLoadFile->FindKey( mapname );
		if ( pMapKey )
		{
			iCount = pMapKey->GetInt( pszCustomKey, 0 ) + 1;
			pMapKey->SetInt( pszCustomKey, iCount );
		}
		else 
		{
			KeyValues *pNewKey = new KeyValues( mapname );
			if ( pNewKey )
			{
				pNewKey->SetString( pszCustomKey, "1" );
				kvMapLoadFile->AddSubKey( pNewKey );
			}
		}

		// Write it out

		// force create this directory incase it doesn't exist
		filesystem->CreateDirHierarchy( MAP_KEY_FILE_DIR, "MOD");

		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		kvMapLoadFile->RecursiveSaveToFile( buf, 0 );
		g_pFullFileSystem->WriteFile( szFilename, "MOD", buf );

		kvMapLoadFile->deleteThis();
	}

	if ( IsX360() )
	{
#ifdef _X360
		xboxsystem->FinishContainerWrites();
#endif
	}
}

int UTIL_GetMapKeyCount( const char *pszCustomKey )
{
	if ( !pszCustomKey )
		return 0;

	char szFilename[ _MAX_PATH ];
	if ( !UTIL_GetMapLoadCountFileName( MAP_KEY_FILE, szFilename, _MAX_PATH ) )
		return 0;

	int iCount = 0;

	KeyValues *kvMapLoadFile = new KeyValues( MAP_KEY_FILE );
	if ( kvMapLoadFile )
	{
		// create an empty file if none exists
		if ( !g_pFullFileSystem->FileExists( szFilename, "MOD" ) )
		{
			// force create this directory incase it doesn't exist
			filesystem->CreateDirHierarchy( MAP_KEY_FILE_DIR, "MOD");

			CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
			g_pFullFileSystem->WriteFile( szFilename, "MOD", buf );
		}

		kvMapLoadFile->LoadFromFile( g_pFullFileSystem, szFilename, "MOD" );

		char mapname[MAX_MAP_NAME];
		Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname) );
		Q_strlower( mapname );

		KeyValues *pMapKey = kvMapLoadFile->FindKey( mapname );
		if ( pMapKey )
		{
			iCount = pMapKey->GetInt( pszCustomKey );
		}

		kvMapLoadFile->deleteThis();
	}

	return iCount;
}

bool UTIL_HasLoadedAnyMap()
{
	char szFilename[ _MAX_PATH ];
	if ( !UTIL_GetMapLoadCountFileName( MAP_KEY_FILE, szFilename, _MAX_PATH ) )
		return false;

	return g_pFullFileSystem->FileExists( szFilename, "MOD" );
}
