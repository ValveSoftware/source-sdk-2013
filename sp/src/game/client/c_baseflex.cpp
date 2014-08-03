//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "filesystem.h"
#include "sentence.h"
#include "hud_closecaption.h"
#include "engine/ivmodelinfo.h"
#include "engine/ivdebugoverlay.h"
#include "bone_setup.h"
#include "soundinfo.h"
#include "tools/bonelist.h"
#include "KeyValues.h"
#include "tier0/vprof.h"
#include "toolframework/itoolframework.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "choreoactor.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool UseHWMorphVCDs();

ConVar g_CV_PhonemeDelay("phonemedelay", "0", 0, "Phoneme delay to account for sound system latency." );
ConVar g_CV_PhonemeFilter("phonemefilter", "0.08", 0, "Time duration of box filter to pass over phonemes." );
ConVar g_CV_FlexRules("flex_rules", "1", 0, "Allow flex animation rules to run." );
ConVar g_CV_BlinkDuration("blink_duration", "0.2", 0, "How many seconds an eye blink will last." );
ConVar g_CV_FlexSmooth("flex_smooth", "1", 0, "Applies smoothing/decay curve to flex animation controller changes." );

#if defined( CBaseFlex )
#undef CBaseFlex
#endif

IMPLEMENT_CLIENTCLASS_DT(C_BaseFlex, DT_BaseFlex, CBaseFlex)
	RecvPropArray3( RECVINFO_ARRAY(m_flexWeight), RecvPropFloat(RECVINFO(m_flexWeight[0]))),
	RecvPropInt(RECVINFO(m_blinktoggle)),
	RecvPropVector(RECVINFO(m_viewtarget)),

#ifdef HL2_CLIENT_DLL
	RecvPropFloat( RECVINFO(m_vecViewOffset[0]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[1]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[2]) ),

	RecvPropVector(RECVINFO(m_vecLean)),
	RecvPropVector(RECVINFO(m_vecShift)),
#endif

END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseFlex )

/*
	// DEFINE_FIELD( C_BaseFlex, m_viewtarget, FIELD_VECTOR ),
	// DEFINE_ARRAY( C_BaseFlex, m_flexWeight, FIELD_FLOAT, 64 ),
	// DEFINE_FIELD( C_BaseFlex, m_blinktoggle, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_blinktime, FIELD_FLOAT ),
	// DEFINE_FIELD( C_BaseFlex, m_prevviewtarget, FIELD_VECTOR ),
	// DEFINE_ARRAY( C_BaseFlex, m_prevflexWeight, FIELD_FLOAT, 64 ),
	// DEFINE_FIELD( C_BaseFlex, m_prevblinktoggle, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iBlink, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iEyeUpdown, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iEyeRightleft, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_FileList, CUtlVector < CFlexSceneFile * > ),
*/

END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool GetHWMExpressionFileName( const char *pFilename, char *pHWMFilename )
{
	// Are we even using hardware morph?
	if ( !UseHWMorphVCDs() )
		return false;

	// Do we have a valid filename?
	if ( !( pFilename && pFilename[0] ) )
		return false;

	// Check to see if we already have an player/hwm/* filename.
	if ( ( V_strstr( pFilename, "player/hwm" ) != NULL ) || ( V_strstr( pFilename, "player\\hwm" ) != NULL ) )
	{
		V_strcpy( pHWMFilename, pFilename );
		return true;
	}

	// Find the hardware morph scene name and pass that along as well.
	char szExpression[MAX_PATH];
	V_strcpy( szExpression, pFilename );

	char szExpressionHWM[MAX_PATH];
	szExpressionHWM[0] = '\0';

	char *pszToken = strtok( szExpression, "/\\" );
	while ( pszToken != NULL )
	{
		V_strcat( szExpressionHWM, pszToken, sizeof( szExpressionHWM ) );
		if ( !V_stricmp( pszToken, "player" ) )
		{
			V_strcat( szExpressionHWM, "\\hwm", sizeof( szExpressionHWM ) );
		}

		pszToken = strtok( NULL, "/\\" );
		if ( pszToken != NULL )
		{
			V_strcat( szExpressionHWM, "\\", sizeof( szExpressionHWM ) );
		}
	}

	V_strcpy( pHWMFilename, szExpressionHWM );
	return true;
}

C_BaseFlex::C_BaseFlex() : 
	m_iv_viewtarget( "C_BaseFlex::m_iv_viewtarget" ), 
	m_iv_flexWeight("C_BaseFlex:m_iv_flexWeight" ),
#ifdef HL2_CLIENT_DLL
	m_iv_vecLean("C_BaseFlex:m_iv_vecLean" ),
	m_iv_vecShift("C_BaseFlex:m_iv_vecShift" ),
#endif
	m_LocalToGlobal( 0, 0, FlexSettingLessFunc )
{
#ifdef _DEBUG
	((Vector&)m_viewtarget).Init();
#endif

	AddVar( &m_viewtarget, &m_iv_viewtarget, LATCH_ANIMATION_VAR | INTERPOLATE_LINEAR_ONLY );
	AddVar( m_flexWeight, &m_iv_flexWeight, LATCH_ANIMATION_VAR );

	// Fill in phoneme class lookup
	SetupMappings( "phonemes" );

	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;

	/// Make sure size is correct
	Assert( PHONEME_CLASS_STRONG + 1 == NUM_PHONEME_CLASSES );

#ifdef HL2_CLIENT_DLL
	// Get general lean vector
	AddVar( &m_vecLean, &m_iv_vecLean, LATCH_ANIMATION_VAR );
	AddVar( &m_vecShift, &m_iv_vecShift, LATCH_ANIMATION_VAR );
#endif
}

C_BaseFlex::~C_BaseFlex()
{
	delete[] m_flFlexDelayedWeight;
	m_SceneEvents.RemoveAll();
	m_LocalToGlobal.RemoveAll();
}


void C_BaseFlex::Spawn()
{
	BaseClass::Spawn();

	InitPhonemeMappings();
}

// TF Player overrides all of these with class specific files
void C_BaseFlex::InitPhonemeMappings()
{
	SetupMappings( "phonemes" );
}

void C_BaseFlex::SetupMappings( char const *pchFileRoot )
{
	// Fill in phoneme class lookup
	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

	Emphasized_Phoneme *normal = &m_PhonemeClasses[ PHONEME_CLASS_NORMAL ];
	Q_snprintf( normal->classname, sizeof( normal->classname ), "%s", pchFileRoot );
	normal->required = true;

	Emphasized_Phoneme *weak = &m_PhonemeClasses[ PHONEME_CLASS_WEAK ];
	Q_snprintf( weak->classname, sizeof( weak->classname ), "%s_weak", pchFileRoot );
	Emphasized_Phoneme *strong = &m_PhonemeClasses[ PHONEME_CLASS_STRONG ];
	Q_snprintf( strong->classname, sizeof( strong->classname ), "%s_strong", pchFileRoot );
}

//-----------------------------------------------------------------------------
// Purpose: initialize fast lookups when model changes
//-----------------------------------------------------------------------------

CStudioHdr *C_BaseFlex::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	
	// init to invalid setting
	m_iBlink = -1;
	m_iEyeUpdown = LocalFlexController_t(-1);
	m_iEyeRightleft = LocalFlexController_t(-1);
	m_bSearchedForEyeFlexes = false;
	m_iMouthAttachment = 0;

	delete[] m_flFlexDelayedWeight;
	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;

	if (hdr)
	{
		if (hdr->numflexdesc())
		{
			m_cFlexDelayedWeight = hdr->numflexdesc();
			m_flFlexDelayedWeight = new float[ m_cFlexDelayedWeight ];
			memset( m_flFlexDelayedWeight, 0, sizeof( float ) * m_cFlexDelayedWeight );
		}

		m_iv_flexWeight.SetMaxCount( hdr->numflexcontrollers() );

		m_iMouthAttachment = LookupAttachment( "mouth" );

		LinkToGlobalFlexControllers( hdr );
	}

	return hdr;
}


void C_BaseFlex::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

#ifdef HL2_CLIENT_DLL
	// shift pelvis, rotate body
	if (hdr->GetNumIKChains() != 0 && (m_vecShift.x != 0.0 || m_vecShift.y != 0.0))
	{
		//CIKContext auto_ik;
		//auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), currentTime, gpGlobals->framecount, boneMask );
		//auto_ik.AddAllLocks( pos, q );

		matrix3x4_t rootxform;
		AngleMatrix( GetRenderAngles(), GetRenderOrigin(), rootxform );

		Vector localShift;
		VectorIRotate( m_vecShift, rootxform, localShift );
		Vector localLean;
		VectorIRotate( m_vecLean, rootxform, localLean );

		Vector p0 = pos[0];
		float length = VectorNormalize( p0 );

		// shift the root bone, but keep the height off the origin the same
		Vector shiftPos = pos[0] + localShift;
		VectorNormalize( shiftPos );
		Vector leanPos = pos[0] + localLean;
		VectorNormalize( leanPos );
		pos[0] = shiftPos * length;

		// rotate the root bone based on how much it was "leaned"
		Vector p1;
		CrossProduct( p0, leanPos, p1 );
		float sinAngle = VectorNormalize( p1 );
		float cosAngle = DotProduct( p0, leanPos );
		float angle = atan2( sinAngle, cosAngle ) * 180 / M_PI;
		Quaternion q1;
		angle = clamp( angle, -45, 45 );
		AxisAngleQuaternion( p1, angle, q1 );
		QuaternionMult( q1, q[0], q[0] );
		QuaternionNormalize( q[0] );

		// DevMsgRT( "   (%.2f) %.2f %.2f %.2f\n", angle, p1.x, p1.y, p1.z );
		// auto_ik.SolveAllLocks( pos, q );
	}
#endif
}



//-----------------------------------------------------------------------------
// Purpose: place "voice" sounds on mouth
//-----------------------------------------------------------------------------

bool C_BaseFlex::GetSoundSpatialization( SpatializationInfo_t& info )
{
	bool bret = BaseClass::GetSoundSpatialization( info );
	// Default things it's audible, put it at a better spot?
	if ( bret )
	{
		if ((info.info.nChannel == CHAN_VOICE || info.info.nChannel == CHAN_VOICE2) && m_iMouthAttachment > 0)
		{
			Vector origin;
			QAngle angles;
			
			C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false );

			if (GetAttachment( m_iMouthAttachment, origin, angles ))
			{
				if (info.pOrigin)
				{
					*info.pOrigin = origin;
				}

				if (info.pAngles)
				{
					*info.pAngles = angles;
				}
			}
		}
	}

	return bret;
}


//-----------------------------------------------------------------------------
// Purpose: run the interpreted FAC's expressions, converting global flex_controller 
//			values into FAC weights
//-----------------------------------------------------------------------------
void C_BaseFlex::RunFlexRules( CStudioHdr *hdr, float *dest )
{
	if ( !g_CV_FlexRules.GetInt() )
		return;

	if ( !hdr )
		return;

/*
	// 0 means run them all
	int nFlexRulesToRun = 0;

	const char *pszExpression = flex_expression.GetString();
	if ( pszExpression )
	{
		nFlexRulesToRun = atoi(pszExpression); // 0 will be returned if not a numeric string
	}
//*/

	hdr->RunFlexRules( g_flexweight, dest );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFlexSceneFileManager::Init()
{
	// Trakcer 16692:  Preload these at startup to avoid hitch first time we try to load them during actual gameplay
	FindSceneFile( NULL, "phonemes", true );
	FindSceneFile( NULL, "phonemes_weak", true );
	FindSceneFile(NULL,  "phonemes_strong", true );

#if defined( HL2_CLIENT_DLL )
	FindSceneFile( NULL, "random", true );
	FindSceneFile( NULL, "randomAlert", true );
#endif

#if defined( TF_CLIENT_DLL )
	// HACK TO ALL TF TO HAVE PER CLASS OVERRIDES
	char const *pTFClasses[] = 
	{
		"scout",
		"sniper",
		"soldier",
		"demo",
		"medic",
		"heavy",
		"pyro",
		"spy",
		"engineer",
	};

	char fn[ MAX_PATH ];
	for ( int i = 0; i < ARRAYSIZE( pTFClasses ); ++i )
	{
		Q_snprintf( fn, sizeof( fn ), "player/%s/phonemes/phonemes", pTFClasses[i] );
		FindSceneFile( NULL, fn, true );
		Q_snprintf( fn, sizeof( fn ), "player/%s/phonemes/phonemes_weak", pTFClasses[i] );
		FindSceneFile( NULL, fn, true );
		Q_snprintf( fn, sizeof( fn ), "player/%s/phonemes/phonemes_strong", pTFClasses[i] );
		FindSceneFile( NULL, fn, true );

		if ( !IsX360() )
		{
			Q_snprintf( fn, sizeof( fn ), "player/hwm/%s/phonemes/phonemes", pTFClasses[i] );
			FindSceneFile( NULL, fn, true );
			Q_snprintf( fn, sizeof( fn ), "player/hwm/%s/phonemes/phonemes_weak", pTFClasses[i] );
			FindSceneFile( NULL, fn, true );
			Q_snprintf( fn, sizeof( fn ), "player/hwm/%s/phonemes/phonemes_strong", pTFClasses[i] );
			FindSceneFile( NULL, fn, true );
		}

		Q_snprintf( fn, sizeof( fn ), "player/%s/emotion/emotion", pTFClasses[i] );
		FindSceneFile( NULL, fn, true );
		if ( !IsX360() )
		{
			Q_snprintf( fn, sizeof( fn ), "player/hwm/%s/emotion/emotion", pTFClasses[i] );
			FindSceneFile( NULL, fn, true );
		}
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Tracker 14992:  We used to load 18K of .vfes for every C_BaseFlex who lipsynced, but now we only load those files once globally.
// Note, we could wipe these between levels, but they don't ever load more than the weak/normal/strong phoneme classes that I can tell
//  so I'll just leave them loaded forever for now
//-----------------------------------------------------------------------------
void CFlexSceneFileManager::Shutdown()
{
	DeleteSceneFiles();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up translations
// Input  : *instance - 
//			*pSettinghdr - 
// Output : 	void
//-----------------------------------------------------------------------------
void CFlexSceneFileManager::EnsureTranslations( IHasLocalToGlobalFlexSettings *instance, const flexsettinghdr_t *pSettinghdr )
{
	// The only time instance is NULL is in Init() above, where we're just loading the .vfe files off of the hard disk.
	if ( instance )
	{
		instance->EnsureTranslations( pSettinghdr );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void *CFlexSceneFileManager::FindSceneFile( IHasLocalToGlobalFlexSettings *instance, const char *filename, bool allowBlockingIO )
{
	char szFilename[MAX_PATH];
	Assert( V_strlen( filename ) < MAX_PATH );
	V_strcpy( szFilename, filename );
	
#if defined( TF_CLIENT_DLL )	
	char szHWMFilename[MAX_PATH];
	if ( GetHWMExpressionFileName( szFilename, szHWMFilename ) )
	{
		V_strcpy( szFilename, szHWMFilename );
	}
#endif

	Q_FixSlashes( szFilename );

	// See if it's already loaded
	int i;
	for ( i = 0; i < m_FileList.Count(); i++ )
	{
		CFlexSceneFile *file = m_FileList[ i ];
		if ( file && !Q_stricmp( file->filename, szFilename ) )
		{
			// Make sure translations (local to global flex controller) are set up for this instance
			EnsureTranslations( instance, ( const flexsettinghdr_t * )file->buffer );
			return file->buffer;
		}
	}
	
	if ( !allowBlockingIO )
	{
		return NULL;
	}

	// Load file into memory
	void *buffer = NULL;
	int len = filesystem->ReadFileEx( VarArgs( "expressions/%s.vfe", szFilename ), "GAME", &buffer );

	if ( !len )
		return NULL;

	// Create scene entry
	CFlexSceneFile *pfile = new CFlexSceneFile;
	// Remember filename
	Q_strncpy( pfile->filename, szFilename, sizeof( pfile->filename ) );
	// Remember data pointer
	pfile->buffer = buffer;
	// Add to list
	m_FileList.AddToTail( pfile );

	// Swap the entire file
	if ( IsX360() )
	{
		CByteswap swap;
		swap.ActivateByteSwapping( true );
		byte *pData = (byte*)buffer;
		flexsettinghdr_t *pHdr = (flexsettinghdr_t*)pData;
		swap.SwapFieldsToTargetEndian( pHdr );

		// Flex Settings
		flexsetting_t *pFlexSetting = (flexsetting_t*)((byte*)pHdr + pHdr->flexsettingindex);
		for ( int i = 0; i < pHdr->numflexsettings; ++i, ++pFlexSetting )
		{
			swap.SwapFieldsToTargetEndian( pFlexSetting );
			
			flexweight_t *pWeight = (flexweight_t*)(((byte*)pFlexSetting) + pFlexSetting->settingindex );
			for ( int j = 0; j < pFlexSetting->numsettings; ++j, ++pWeight )
			{
				swap.SwapFieldsToTargetEndian( pWeight );
			}
		}

		// indexes
		pData = (byte*)pHdr + pHdr->indexindex;
		swap.SwapBufferToTargetEndian( (int*)pData, (int*)pData, pHdr->numindexes );

		// keymappings
		pData  = (byte*)pHdr + pHdr->keymappingindex;
		swap.SwapBufferToTargetEndian( (int*)pData, (int*)pData, pHdr->numkeys );

		// keyname indices
		pData = (byte*)pHdr + pHdr->keynameindex;
		swap.SwapBufferToTargetEndian( (int*)pData, (int*)pData, pHdr->numkeys );
	}

	// Fill in translation table
	EnsureTranslations( instance, ( const flexsettinghdr_t * )pfile->buffer );

	// Return data
	return pfile->buffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexSceneFileManager::DeleteSceneFiles()
{
	while ( m_FileList.Count() > 0 )
	{
		CFlexSceneFile *file = m_FileList[ 0 ];
		m_FileList.Remove( 0 );
		free( file->buffer );
		delete file;
	}
}

CFlexSceneFileManager g_FlexSceneFileManager;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void *C_BaseFlex::FindSceneFile( const char *filename )
{
	return g_FlexSceneFileManager.FindSceneFile( this, filename, false );
}

//-----------------------------------------------------------------------------
// Purpose: make sure the eyes are within 30 degrees of forward
//-----------------------------------------------------------------------------
Vector C_BaseFlex::SetViewTarget( CStudioHdr *pStudioHdr )
{
  	if ( !pStudioHdr )
  		return Vector( 0, 0, 0);

	// aim the eyes
	Vector tmp = m_viewtarget;

	if ( !m_bSearchedForEyeFlexes )
	{
		m_bSearchedForEyeFlexes = true;

		m_iEyeUpdown = FindFlexController( "eyes_updown" );
		m_iEyeRightleft = FindFlexController( "eyes_rightleft" );

		if ( m_iEyeUpdown != -1 )
		{
			pStudioHdr->pFlexcontroller( m_iEyeUpdown )->localToGlobal = AddGlobalFlexController( "eyes_updown" );
		}
		if ( m_iEyeRightleft != -1 )
		{
			pStudioHdr->pFlexcontroller( m_iEyeRightleft )->localToGlobal = AddGlobalFlexController( "eyes_rightleft" );
		}
	}

	if (m_iEyeAttachment > 0)
	{
		matrix3x4_t attToWorld;
		if (!GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			return Vector( 0, 0, 0);
		}

		Vector local;
		VectorITransform( tmp, attToWorld, local );
		
		// FIXME: clamp distance to something based on eyeball distance
		if (local.x < 6)
		{
			local.x = 6;
		}
		float flDist = local.Length();
		VectorNormalize( local );

		// calculate animated eye deflection
		Vector eyeDeflect;
		QAngle eyeAng( 0, 0, 0 );
		if ( m_iEyeUpdown != -1 )
		{
			mstudioflexcontroller_t *pflex = pStudioHdr->pFlexcontroller( m_iEyeUpdown );
			eyeAng.x = g_flexweight[ pflex->localToGlobal ];
		}
		
		if ( m_iEyeRightleft != -1 )
		{
			mstudioflexcontroller_t *pflex = pStudioHdr->pFlexcontroller( m_iEyeRightleft );
			eyeAng.y = g_flexweight[ pflex->localToGlobal ];
		}

		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%5.3f %5.3f", eyeAng.x, eyeAng.y );

		AngleVectors( eyeAng, &eyeDeflect );
		eyeDeflect.x = 0;

		// reduce deflection the more the eye is off center
		// FIXME: this angles make no damn sense
		eyeDeflect = eyeDeflect * (local.x * local.x);
		local = local + eyeDeflect;
		VectorNormalize( local );

		// check to see if the eye is aiming outside the max eye deflection
		float flMaxEyeDeflection = pStudioHdr->MaxEyeDeflection();
		if ( local.x < flMaxEyeDeflection )
		{
			// if so, clamp it to 30 degrees offset
			// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 1, 0, "%5.3f %5.3f %5.3f", local.x, local.y, local.z );
			local.x = 0;
			float d = local.LengthSqr();
			if ( d > 0.0f )
			{
				d = sqrtf( ( 1.0f - flMaxEyeDeflection * flMaxEyeDeflection ) / ( local.y*local.y + local.z*local.z ) );
				local.x = flMaxEyeDeflection;
				local.y = local.y * d;
				local.z = local.z * d;
			}
			else
			{
				local.x = 1.0;
			}
		}
		local = local * flDist;
		VectorTransform( local, attToWorld, tmp );
	}

	modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );

	/*
	debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f  : %.2f %.2f %.2f", 
		m_viewtarget.x, m_viewtarget.y, m_viewtarget.z, 
		m_prevviewtarget.x, m_prevviewtarget.y, m_prevviewtarget.z );
	*/

	return tmp;
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void C_BaseFlex::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which blends are available for the current phoneme
	bool has_weak	= classes[ PHONEME_CLASS_WEAK ].valid;
	bool has_strong = classes[ PHONEME_CLASS_STRONG ].valid;

	// Better have phonemes in general
	Assert( classes[ PHONEME_CLASS_NORMAL ].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (frac) * 2.0f * STRONG_CROSSFADE_START;
			classes[ PHONEME_CLASS_STRONG ].amount = 1.0f - frac; 
		}
		else
		{
			emphasis_intensity = MIN( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (1.0f - frac) * 2.0f * WEAK_CROSSFADE_START;
			classes[ PHONEME_CLASS_WEAK ].amount = frac; 
		}
		else
		{
			emphasis_intensity = MAX( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );
	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}
		
	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[ type ];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		const flexsettinghdr_t *actual_flexsetting_header = info->base;
		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if (!pSetting)
		{
			continue;
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for ( int i = 0; i < truecount; i++)
			{
				// Translate to global controller number
				int j = FlexControllerLocalToGlobal( actual_flexsetting_header, pWeights->key );
				// Add scaled weighting in
				g_flexweight[j] += info->amount * scale * pWeights->weight;
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseFlex::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[ i ];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)FindSceneFile( info->classname );
		}
		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ('V' << 16) + ('F' << 8) + ('E') );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}
	}
	
	return skip;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			*sentence - 
//			t - 
//			dt - 
//			juststarted - 
//-----------------------------------------------------------------------------
ConVar g_CV_PhonemeSnap("phonemesnap", "2", 0, "Lod at level at which visemes stops always considering two phonemes, regardless of duration." );
void C_BaseFlex::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	int pcount = sentence->GetRuntimePhonemeCount();
	for ( int k = 0; k < pcount; k++ )
	{
		const CBasePhonemeTag *phoneme = sentence->GetRuntimePhoneme( k );

		if (t > phoneme->GetStartTime() && t < phoneme->GetEndTime())
		{
			bool bCrossfade = true;
			if ((hdr->flags() & STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE) == 0)
			{
				if (m_iAccumulatedBoneMask & BONE_USED_BY_VERTEX_LOD0)
				{
					bCrossfade = (g_CV_PhonemeSnap.GetInt() > 0);
				}
				else if (m_iAccumulatedBoneMask & BONE_USED_BY_VERTEX_LOD1)
				{
					bCrossfade = (g_CV_PhonemeSnap.GetInt() > 1);
				}
				else if (m_iAccumulatedBoneMask & BONE_USED_BY_VERTEX_LOD2)
				{
					bCrossfade = (g_CV_PhonemeSnap.GetInt() > 2);
				}
				else if (m_iAccumulatedBoneMask & BONE_USED_BY_VERTEX_LOD3)
				{
					bCrossfade = (g_CV_PhonemeSnap.GetInt() > 3);
				}
				else
				{
					bCrossfade = false;
				}
			}

			if (bCrossfade)
			{
				if (k < pcount-1)
				{
					const CBasePhonemeTag *next = sentence->GetRuntimePhoneme( k + 1 );
					// if I have a neighbor
					if ( next )
					{
						//  and they're touching
						if (next->GetStartTime() == phoneme->GetEndTime() )
						{
							// no gap, so increase the blend length to the end of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetEndTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
						else
						{
							// dead space, so increase the blend length to the start of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetStartTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
					}
					else
					{
						// last phoneme in list, increase the blend length to the length of the current phoneme
						dt = MAX( dt, phoneme->GetEndTime() - phoneme->GetStartTime() );
					}
				}
			}
		}

		float t1 = ( phoneme->GetStartTime() - t) / dt;
		float t2 = ( phoneme->GetEndTime() - t) / dt;

		if (t1 < 1.0 && t2 > 0)
		{
			float scale;

			// clamp
			if (t2 > 1)
				t2 = 1;
			if (t1 < 0)
				t1 = 0;

			// FIXME: simple box filter.  Should use something fancier
			scale = (t2 - t1);

			AddViseme( classes, emphasis_intensity, phoneme->GetPhonemeCode(), scale, juststarted );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//-----------------------------------------------------------------------------
void C_BaseFlex::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	if ( !MouthInfo().IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0 ; source < MouthInfo().GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = MouthInfo().GetVoiceSource( source );
		if ( !vd || vd->ShouldIgnorePhonemes() )
			continue;

		CSentence *sentence = engine->GetSentence( vd->GetSource() );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->GetSource() );
		float	timesincestart = vd->GetElapsedTime();

		// This sound should be done...why hasn't it been removed yet???
		if ( timesincestart >= ( sentence_length + 2.0f ) )
			continue;

		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();

		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Streaming sounds get an additional delay...
		/*
		// Tracker 20534:  Probably not needed any more with the async sound stuff that
		//  we now have (we don't have a disk i/o hitch on startup which might have been
		//  messing up the startup timing a bit )
		bool streaming = engine->IsStreaming( vd->m_pAudioSource );
		if ( streaming )
		{
			t -= g_CV_PhonemeDelayStreaming.GetFloat();
		}
		*/

		// Assume sound has been playing for a while...
		bool juststarted = false;

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );
	}
}

//-----------------------------------------------------------------------------
// Purpose: fill keyvalues message with flex state
// Input  :
//-----------------------------------------------------------------------------
void C_BaseFlex::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_BaseFlex::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	BaseClass::GetToolRecordingState( msg );

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	memset( g_flexweight, 0, sizeof( g_flexweight ) );

	if ( hdr->numflexcontrollers() == 0 )
		return;

	LocalFlexController_t i;

	ProcessSceneEvents( true );

	// FIXME: shouldn't this happen at runtime?
	// initialize the models local to global flex controller mappings
	if (hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1)
	{
		for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
		{
			int j = AddGlobalFlexController( hdr->pFlexcontroller( i )->pszName() );
			hdr->pFlexcontroller( i )->localToGlobal = j;
		}
	}

	// blend weights from server
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		g_flexweight[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		g_flexweight[pflex->localToGlobal] = g_flexweight[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	ProcessSceneEvents( false );

	// check for blinking
	if (m_blinktoggle != m_prevblinktoggle)
	{
		m_prevblinktoggle = m_blinktoggle;
		m_blinktime = gpGlobals->curtime + g_CV_BlinkDuration.GetFloat();
	}

	if (m_iBlink == -1)
		m_iBlink = AddGlobalFlexController( "blink" );
	g_flexweight[m_iBlink] = 0;

	// FIXME: this needs a better algorithm
	// blink the eyes
	float t = (m_blinktime - gpGlobals->curtime) * M_PI * 0.5 * (1.0/g_CV_BlinkDuration.GetFloat());
	if (t > 0)
	{
		// do eyeblink falloff curve
		t = cos(t);
		if (t > 0)
		{
			g_flexweight[m_iBlink] = sqrtf( t ) * 2;
			if (g_flexweight[m_iBlink] > 1)
				g_flexweight[m_iBlink] = 2.0 - g_flexweight[m_iBlink];
		}
	}

	// Drive the mouth from .wav file playback...
	ProcessVisemes( m_PhonemeClasses );

	// Necessary???
	SetViewTarget( hdr );

	Vector viewtarget = m_viewtarget; // Use the unfiltered value

	// HACK HACK: Unmap eyes right/left amounts
	if (m_iEyeUpdown != -1 && m_iEyeRightleft != -1)
	{
		mstudioflexcontroller_t *flexupdown = hdr->pFlexcontroller( m_iEyeUpdown );
		mstudioflexcontroller_t *flexrightleft = hdr->pFlexcontroller( m_iEyeRightleft );

		if ( flexupdown->localToGlobal != -1 && flexrightleft->localToGlobal != -1 )
		{
			float updown = g_flexweight[ flexupdown->localToGlobal ];
			float rightleft = g_flexweight[ flexrightleft->localToGlobal ];

			if ( flexupdown->min != flexupdown->max )
			{
				updown = RemapVal( updown, flexupdown->min, flexupdown->max, 0.0f, 1.0f );
			}
			if ( flexrightleft->min != flexrightleft->max )
			{
				rightleft = RemapVal( rightleft, flexrightleft->min, flexrightleft->max, 0.0f, 1.0f );
			}
	
			g_flexweight[ flexupdown->localToGlobal ] = updown;
			g_flexweight[ flexrightleft->localToGlobal ] = rightleft;
		}
	}

	// Convert back to normalized weights
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		// rescale
		if ( pflex->max != pflex->min )
		{
			g_flexweight[pflex->localToGlobal] = ( g_flexweight[pflex->localToGlobal] - pflex->min ) / ( pflex->max - pflex->min );
		}
	}

	static BaseFlexRecordingState_t state;
	state.m_nFlexCount = MAXSTUDIOFLEXCTRL;
	state.m_pDestWeight = g_flexweight;
	state.m_vecViewTarget = viewtarget;
	msg->SetPtr( "baseflex", &state );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::OnThreadedDrawSetup()
{
	if (m_iEyeAttachment < 0)
		return;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}
	CalcAttachments();
}


//-----------------------------------------------------------------------------
// Should we use delayed flex weights?
//-----------------------------------------------------------------------------
bool C_BaseFlex::UsesFlexDelayedWeights()
{
	return ( m_flFlexDelayedWeight && g_CV_FlexSmooth.GetBool() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::LinkToGlobalFlexControllers( CStudioHdr *hdr )
{
	if ( hdr && hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1 )
	{
		for (LocalFlexController_t i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
		{
			int j = AddGlobalFlexController( hdr->pFlexcontroller( i )->pszName() );
			hdr->pFlexcontroller( i )->localToGlobal = j;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Rendering callback to allow the client to set up all the model specific flex weights
//-----------------------------------------------------------------------------
void C_BaseFlex::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// hack in an initialization
	LinkToGlobalFlexControllers( GetModelPtr() );
	m_iBlink = AddGlobalFlexController( "UH" );

	if ( SetupGlobalWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights ) )
	{
		SetupLocalWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Use the local bone positions to set flex control weights
//          via boneflexdrivers specified in the model
//-----------------------------------------------------------------------------
void C_BaseFlex::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	const int nBoneFlexDriverCount = pStudioHdr->BoneFlexDriverCount();

	for ( int i = 0; i < nBoneFlexDriverCount; ++i )
	{
		const mstudioboneflexdriver_t *pBoneFlexDriver = pStudioHdr->BoneFlexDriver( i );
		const Vector &position = pos[ pBoneFlexDriver->m_nBoneIndex ];

		const int nControllerCount = pBoneFlexDriver->m_nControlCount;
		for ( int j = 0; j < nControllerCount; ++j )
		{
			const mstudioboneflexdrivercontrol_t *pController = pBoneFlexDriver->pBoneFlexDriverControl( j );
			Assert( pController->m_nFlexControllerIndex >= 0 && pController->m_nFlexControllerIndex < pStudioHdr->numflexcontrollers() );
			Assert( pController->m_nBoneComponent >= 0 && pController->m_nBoneComponent <= 2 );
			SetFlexWeight( static_cast< LocalFlexController_t >( pController->m_nFlexControllerIndex ), RemapValClamped( position[pController->m_nBoneComponent], pController->m_flMin, pController->m_flMax, 0.0f, 1.0f ) );
		}
	}

	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );
}


//-----------------------------------------------------------------------------
// Purpose: process the entities networked state, vcd playback, wav file visemes, and blinks into a global shared flex controller array
//-----------------------------------------------------------------------------
bool C_BaseFlex::SetupGlobalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return false;

	memset( g_flexweight, 0, sizeof(g_flexweight) );

	// FIXME: this should assert then, it's too complex a class for the model
	if ( hdr->numflexcontrollers() == 0 )
	{
		int nSizeInBytes = nFlexWeightCount * sizeof( float );
		memset( pFlexWeights, 0, nSizeInBytes );
		if ( pFlexDelayedWeights )
		{
			memset( pFlexDelayedWeights, 0, nSizeInBytes );
		}
		return false;
	}

	LocalFlexController_t i;

	ProcessSceneEvents( true );

	Assert( hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal != -1 );

	// get the networked flexweights and convert them from 0..1 to real dynamic range
	for (i = LocalFlexController_t(0); i < hdr->numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		g_flexweight[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		g_flexweight[pflex->localToGlobal] = g_flexweight[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	ProcessSceneEvents( false );

	// check for blinking
	if (m_blinktoggle != m_prevblinktoggle)
	{
		m_prevblinktoggle = m_blinktoggle;
		m_blinktime = gpGlobals->curtime + g_CV_BlinkDuration.GetFloat();
	}

	if (m_iBlink == -1)
	{
		m_iBlink = AddGlobalFlexController( "blink" );
	}

	// FIXME: this needs a better algorithm
	// blink the eyes
	float flBlinkDuration = g_CV_BlinkDuration.GetFloat();
	float flOOBlinkDuration = ( flBlinkDuration > 0 ) ? 1.0f / flBlinkDuration : 0.0f;
	float t = ( m_blinktime - gpGlobals->curtime ) * M_PI * 0.5 * flOOBlinkDuration;
	if (t > 0)
	{
		// do eyeblink falloff curve
		t = cos(t);
		if (t > 0.0f && t < 1.0f)
		{
			t = sqrtf( t ) * 2.0f;
			if (t > 1.0f)
				t = 2.0f - t;
			t = clamp( t, 0.0f, 1.0f );
			// add it to whatever the blink track is doing
			g_flexweight[m_iBlink] = clamp( g_flexweight[m_iBlink] + t, 0.0f, 1.0f );
		}
	}

	// Drive the mouth from .wav file playback...
	ProcessVisemes( m_PhonemeClasses );

	return true;
}

void C_BaseFlex::RunFlexDelay( int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights, float &flFlexDelayTime )
{
	// process the delayed version of the flexweights
	if ( flFlexDelayTime > 0.0f && flFlexDelayTime < gpGlobals->curtime )
	{
		float d = clamp( gpGlobals->curtime - flFlexDelayTime, 0.0, gpGlobals->frametime );
		d = ExponentialDecay( 0.8, 0.033, d );

		for ( int i = 0; i < nFlexWeightCount; i++)
		{
			pFlexDelayedWeights[i] = pFlexDelayedWeights[i] * d + pFlexWeights[i] * (1.0f - d);
		}
	}
	flFlexDelayTime = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: convert the global flex controllers into model specific flex weights
//-----------------------------------------------------------------------------
void C_BaseFlex::SetupLocalWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	// BUGBUG: We have a bug with SetCustomModel that causes a disagreement between the studio header here and the one used in l_studio.cpp CModelRender::DrawModelExecute
	// So when we hit that case, let's not do any work because otherwise we'd crash since the array sizes (m_flFlexDelayedWeight vs pFlexWeights) don't match.
	// Note that this check is duplicated in CEconEntity::SetupWeights.
	AssertMsg( nFlexWeightCount == m_cFlexDelayedWeight, "Disagreement between the number of flex weights. Do the studio headers match?" );
	if ( nFlexWeightCount != m_cFlexDelayedWeight )
	{
		return;
	}

	// convert the flex controllers into actual flex values
	RunFlexRules( hdr, pFlexWeights );

	// aim the eyes
	SetViewTarget( hdr );

	AssertOnce( hdr->pFlexcontroller( LocalFlexController_t(0) )->localToGlobal != -1 );

	if ( pFlexDelayedWeights )
	{
		RunFlexDelay( nFlexWeightCount, pFlexWeights, m_flFlexDelayedWeight, m_flFlexDelayTime );
		memcpy( pFlexDelayedWeights, m_flFlexDelayedWeight, sizeof( float ) * nFlexWeightCount );
	}

	/*
	LocalFlexController_t i;

	for (i = 0; i < hdr->numflexdesc; i++)
	{
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i-hdr->numflexcontrollers, 0, "%2d:%s : %3.2f", i, hdr->pFlexdesc( i )->pszFACS(), pFlexWeights[i] );
	}
	*/

	/*
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		int j = hdr->pFlexcontroller( i )->link;
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -i, 0, "%s %3.2f", g_flexcontroller[i], g_flexweight[j] );
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Unified set of flex controller entries that all systems can talk to
//-----------------------------------------------------------------------------

int C_BaseFlex::g_numflexcontrollers;
char * C_BaseFlex::g_flexcontroller[MAXSTUDIOFLEXCTRL*4];
float C_BaseFlex::g_flexweight[MAXSTUDIOFLEXDESC];

int C_BaseFlex::AddGlobalFlexController( const char *szName )
{
	int i;
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		if (Q_stricmp( g_flexcontroller[i], szName ) == 0)
		{
			return i;
		}
	}

	if ( g_numflexcontrollers < MAXSTUDIOFLEXCTRL * 4 )
	{
		g_flexcontroller[g_numflexcontrollers++] = strdup( szName );
	}
	else
	{
		// FIXME: missing runtime error condition
	}
	return i;
}

char const *C_BaseFlex::GetGlobalFlexControllerName( int idx )
{
	if ( idx < 0 || idx >= g_numflexcontrollers )
	{
		return "";
	}

	return g_flexcontroller[ idx ];
}

const flexsetting_t *C_BaseFlex::FindNamedSetting( const flexsettinghdr_t *pSettinghdr, const char *expr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return NULL;
	}

	return pSetting;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::StartChoreoScene( CChoreoScene *scene )
{
	if ( m_ActiveChoreoScenes.Find( scene ) != m_ActiveChoreoScenes.InvalidIndex() )
	{
		return;
	}

	m_ActiveChoreoScenes.AddToTail( scene );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::RemoveChoreoScene( CChoreoScene *scene )
{
	// Assert( m_ActiveChoreoScenes.Find( scene ) != m_ActiveChoreoScenes.InvalidIndex() );

	m_ActiveChoreoScenes.FindAndRemove( scene );
}

//-----------------------------------------------------------------------------
// Purpose: Remove all active SceneEvents
//-----------------------------------------------------------------------------
void C_BaseFlex::ClearSceneEvents( CChoreoScene *scene, bool canceled )
{
	if ( !scene )
	{
		m_SceneEvents.RemoveAll();
		return;
	}

	for ( int i = m_SceneEvents.Count() - 1; i >= 0; i-- )
	{
		CSceneEventInfo *info = &m_SceneEvents[ i ];

		Assert( info );
		Assert( info->m_pScene );
		Assert( info->m_pEvent );

		if ( info->m_pScene != scene )
			continue;

		if ( !ClearSceneEvent( info, false, canceled ))
		{
			// unknown expression to clear!!
			Assert( 0 );
		}

		// Free this slot
		info->m_pEvent		= NULL;
		info->m_pScene		= NULL;
		info->m_bStarted	= false;

		m_SceneEvents.Remove( i );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Stop specifics of expression
//-----------------------------------------------------------------------------

bool C_BaseFlex::ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled )
{
	Assert( info );
	Assert( info->m_pScene );
	Assert( info->m_pEvent );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Add string indexed scene/expression/duration to list of active SceneEvents
// Input  : scenefile - 
//			expression - 
//			duration - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddSceneEvent( CChoreoScene *scene, CChoreoEvent *event, CBaseEntity *pTarget, bool bClientSide )
{
	if ( !scene || !event )
	{
		Msg( "C_BaseFlex::AddSceneEvent:  scene or event was NULL!!!\n" );
		return;
	}

	CChoreoActor *actor = event->GetActor();
	if ( !actor )
	{
		Msg( "C_BaseFlex::AddSceneEvent:  event->GetActor() was NULL!!!\n" );
		return;
	}


	CSceneEventInfo info;

	memset( (void *)&info, 0, sizeof( info ) );

	info.m_pEvent		= event;
	info.m_pScene		= scene;
	info.m_hTarget		= pTarget;
	info.m_bStarted		= false;
	info.m_bClientSide	= bClientSide;

	if (StartSceneEvent( &info, scene, event, actor, pTarget ))
	{
		m_SceneEvents.AddToTail( info );
	}
	else
	{
		scene->SceneMsg( "C_BaseFlex::AddSceneEvent:  event failed\n" );
		// Assert( 0 ); // expression failed to start
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseFlex::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	default:
		break;

	case CChoreoEvent::FLEXANIMATION:
		info->InitWeight( this );
		return true;

	case CChoreoEvent::EXPRESSION:
		return true;
		
	case CChoreoEvent::SEQUENCE: 
		if ( info->m_bClientSide )
		{
			return RequestStartSequenceSceneEvent( info, scene, event, actor, pTarget );
		}
		break;

	case CChoreoEvent::SPEAK:
		if ( info->m_bClientSide )
		{
			return true;
		}
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseFlex::RequestStartSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	info->m_nSequence = LookupSequence( event->GetParameters() );

	// make sure sequence exists
	if ( info->m_nSequence < 0 )
		return false;

	info->m_pActor = actor;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Remove expression
// Input  : scenefile - 
//			expression - 
//-----------------------------------------------------------------------------
void C_BaseFlex::RemoveSceneEvent( CChoreoScene *scene, CChoreoEvent *event, bool fastKill )
{
	Assert( event );

	for ( int i = 0 ; i < m_SceneEvents.Count(); i++ )
	{
		CSceneEventInfo *info = &m_SceneEvents[ i ];

		Assert( info );
		Assert( info->m_pEvent );

		if ( info->m_pScene != scene )
			continue;

		if ( info->m_pEvent != event)
			continue;

		if (ClearSceneEvent( info, fastKill, false ))
		{
			// Free this slot
			info->m_pEvent		= NULL;
			info->m_pScene		= NULL;
			info->m_bStarted	= false;

			m_SceneEvents.Remove( i );
			return;
		}
	}

	// many events refuse to start due to bogus parameters
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the event should be considered "completed"
//-----------------------------------------------------------------------------
bool C_BaseFlex::CheckSceneEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	for ( int i = 0 ; i < m_SceneEvents.Count(); i++ )
	{
		CSceneEventInfo *info = &m_SceneEvents[ i ];

		Assert( info );
		Assert( info->m_pEvent );

		if ( info->m_pScene != scene )
			continue;

		if ( info->m_pEvent != event)
			continue;
		
		return CheckSceneEventCompletion( info, currenttime, scene, event );
	}
	return true;
}



bool C_BaseFlex::CheckSceneEventCompletion( CSceneEventInfo *info, float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

void C_BaseFlex::SetFlexWeight( LocalFlexController_t index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0f, 1.0f );
		}

		m_flexWeight[ index ] = value;
	}
}

float C_BaseFlex::GetFlexWeight( LocalFlexController_t index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return 0;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return m_flexWeight[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}
				
		return m_flexWeight[index];
	}
	return 0.0;
}

LocalFlexController_t C_BaseFlex::FindFlexController( const char *szName )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t(-1);
}

//-----------------------------------------------------------------------------
// Purpose: Default implementation
//-----------------------------------------------------------------------------
void C_BaseFlex::ProcessSceneEvents( bool bFlexEvents )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// slowly decay to netural expression

	if ( bFlexEvents )
	{
		for ( LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
		{
			SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
		}
	}

	// Iterate SceneEvents and look for active slots
	for ( int i = 0; i < m_SceneEvents.Count(); i++ )
	{
		CSceneEventInfo *info = &m_SceneEvents[ i ];
		Assert( info );

		// FIXME:  Need a safe handle to m_pEvent in case of memory deletion?
		CChoreoEvent *event = info->m_pEvent;
		Assert( event );

		CChoreoScene *scene = info->m_pScene;
		Assert( scene );

		if ( ProcessSceneEvent( bFlexEvents, info, scene, event ) )
		{
			info->m_bStarted = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Various methods to process facial SceneEvents: 
//-----------------------------------------------------------------------------
bool C_BaseFlex::ProcessFlexAnimationSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->HasEndTime() );
	if ( event->HasEndTime() )
	{
		AddFlexAnimation( info );
	}
	return true;
}

bool C_BaseFlex::ProcessFlexSettingSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	// Flexanimations have to have an end time!!!
	if ( !event->HasEndTime() )
		return true;

	VPROF( "C_BaseFlex::ProcessFlexSettingSceneEvent" );

	// Look up the actual strings
	const char *scenefile	= event->GetParameters();
	const char *name		= event->GetParameters2();
	
	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = ( const flexsettinghdr_t * )g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();
			
			float scale = event->GetIntensity( scenetime );
			
			// Add the named expression
			AddFlexSetting( name, scale, pExpHdr, !info->m_bStarted );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Each CBaseFlex maintains a UtlRBTree of mappings, one for each loaded flex scene file it uses.  This is used to
//  sort the entries in the RBTree
// Input  : lhs - 
//			rhs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FlexSettingLessFunc( const FS_LocalToGlobal_t& lhs, const FS_LocalToGlobal_t& rhs )
{
	return lhs.m_Key < rhs.m_Key;
}

//-----------------------------------------------------------------------------
// Purpose: Since everyone shared a pSettinghdr now, we need to set up the localtoglobal mapping per entity, but 
//  we just do this in memory with an array of integers (could be shorts, I suppose)
// Input  : *pSettinghdr - 
//-----------------------------------------------------------------------------
void C_BaseFlex::EnsureTranslations( const flexsettinghdr_t *pSettinghdr )
{
	Assert( pSettinghdr );

	FS_LocalToGlobal_t entry( pSettinghdr );

	unsigned short idx = m_LocalToGlobal.Find( entry );
	if ( idx != m_LocalToGlobal.InvalidIndex() )
		return;

	entry.SetCount( pSettinghdr->numkeys );

	for ( int i = 0; i < pSettinghdr->numkeys; ++i )
	{
		entry.m_Mapping[ i ] = AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	m_LocalToGlobal.Insert( entry );
}

//-----------------------------------------------------------------------------
// Purpose: Look up instance specific mapping
// Input  : *pSettinghdr - 
//			key - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseFlex::FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key )
{
	FS_LocalToGlobal_t entry( pSettinghdr );

	int idx = m_LocalToGlobal.Find( entry );
	if ( idx == m_LocalToGlobal.InvalidIndex() )
	{
		// This should never happen!!!
		Assert( 0 );
		Warning( "Unable to find mapping for flexcontroller %i, settings %p on %i/%s\n", key, pSettinghdr, entindex(), GetClassname() );
		EnsureTranslations( pSettinghdr );
		idx = m_LocalToGlobal.Find( entry );
		if ( idx == m_LocalToGlobal.InvalidIndex() )
		{
			Error( "CBaseFlex::FlexControllerLocalToGlobal failed!\n" );
		}
	}

	FS_LocalToGlobal_t& result = m_LocalToGlobal[ idx ];
	// Validate lookup
	Assert( result.m_nCount != 0 && key < result.m_nCount );
	int index = result.m_Mapping[ key ];
	return index;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//			newexpression - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddFlexSetting( const char *expr, float scale, 
	const flexsettinghdr_t *pSettinghdr, bool newexpression )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !V_stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return;
	}

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for (i = 0; i < truecount; i++, pWeights++)
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

		// blend scaled weighting in to total (post networking g_flexweight!!!!)
		float s = clamp( scale * pWeights->influence, 0.0f, 1.0f );
		g_flexweight[index] = g_flexweight[index] * (1.0f - s) + pWeights->weight * s;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseFlex::ProcessSceneEvent( bool bFlexEvents, CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
	default:
		break;
	case CChoreoEvent::FLEXANIMATION:
		if ( bFlexEvents )
		{
			return ProcessFlexAnimationSceneEvent( info, scene, event );
		}
		return true;

	case CChoreoEvent::EXPRESSION:
		if ( !bFlexEvents )
		{
            return ProcessFlexSettingSceneEvent( info, scene, event );
		}
		return true;

	case CChoreoEvent::SEQUENCE:
		if ( info->m_bClientSide )
		{
			if ( !bFlexEvents )
			{
				return ProcessSequenceSceneEvent( info, scene, event );
			}
			return true;
		}
		break;

	case CChoreoEvent::SPEAK:
		if ( info->m_bClientSide )
		{
			return true;
		}
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
bool C_BaseFlex::ProcessSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !info  || !event || !scene )
		return false;

	SetSequence( info->m_nSequence );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddFlexAnimation( CSceneEventInfo *info )
{	
	if ( !info )
		return;

	CChoreoEvent *event = info->m_pEvent;
	if ( !event )
		return;

	CChoreoScene *scene = info->m_pScene;
	if ( !scene )
		return;

	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[ 512 ];
				Q_strncpy( name, "right_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 0 );

				Q_strncpy( name, "left_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 1 );
			}
			else
			{
				track->SetFlexControllerIndex( MAX( FindFlexController( (char *)track->GetFlexControllerName() ), LocalFlexController_t(0)), 0 );
			}
		}

		event->SetTrackLookupSet( true );
	}

	if ( !scene_clientflex.GetBool() )
		return;

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// decay if this is a background scene and there's other flex animations playing
	weight = weight * info->UpdateWeight( this );

	// Compute intensity for each track in animation and apply
	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				LocalFlexController_t controller = track->GetRawFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= LocalFlexController_t(0) )
				{
					float orig = GetFlexWeight( controller );
					float value = orig * (1 - weight) + flIntensity * weight;
					SetFlexWeight( controller, value );
				}
			}
		}
		else
		{
			LocalFlexController_t controller = track->GetRawFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= LocalFlexController_t(0) )
			{
				float orig = GetFlexWeight( controller );
				float value = orig * (1 - weight) + flIntensity * weight;
				SetFlexWeight( controller, value );
			}
		}
	}

	info->m_bStarted = true;
}

void CSceneEventInfo::InitWeight( C_BaseFlex *pActor )
{
	m_flWeight = 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: update weight for background events.  Only call once per think
//-----------------------------------------------------------------------------

float CSceneEventInfo::UpdateWeight( C_BaseFlex *pActor )
{
	m_flWeight = MIN( m_flWeight + 0.1, 1.0 );
	return m_flWeight;
}

BEGIN_BYTESWAP_DATADESC( flexsettinghdr_t )
	DEFINE_FIELD( id, FIELD_INTEGER ),
	DEFINE_FIELD( version, FIELD_INTEGER ),
	DEFINE_ARRAY( name, FIELD_CHARACTER, 64 ),
	DEFINE_FIELD( length, FIELD_INTEGER ),
	DEFINE_FIELD( numflexsettings, FIELD_INTEGER ),
	DEFINE_FIELD( flexsettingindex, FIELD_INTEGER ),
	DEFINE_FIELD( nameindex, FIELD_INTEGER ),
	DEFINE_FIELD( numindexes, FIELD_INTEGER ),
	DEFINE_FIELD( indexindex, FIELD_INTEGER ),
	DEFINE_FIELD( numkeys, FIELD_INTEGER ),
	DEFINE_FIELD( keynameindex, FIELD_INTEGER ),
	DEFINE_FIELD( keymappingindex, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( flexsetting_t )
	DEFINE_FIELD( nameindex, FIELD_INTEGER ),
	DEFINE_FIELD( obsolete1, FIELD_INTEGER ),
	DEFINE_FIELD( numsettings, FIELD_INTEGER ),
	DEFINE_FIELD( index, FIELD_INTEGER ),
	DEFINE_FIELD( obsolete2, FIELD_INTEGER ),
	DEFINE_FIELD( settingindex, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( flexweight_t )
	DEFINE_FIELD( key, FIELD_INTEGER ),
	DEFINE_FIELD( weight, FIELD_FLOAT ),
	DEFINE_FIELD( influence, FIELD_FLOAT ),
END_BYTESWAP_DATADESC()

