//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Rich Presence support
//
//=====================================================================================//

#include "cbase.h"
#include "tf_tips.h"
#include "tier3/tier3.h"
#include <vgui/ILocalize.h>
#include "cdll_util.h"
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CTFTips::CTFTips() : CAutoGameSystem( "CTFTips" )
{
	Q_memset( m_iTipCount, 0, sizeof( m_iTipCount ) );
	m_iTipCountAll = 0;
	m_iCurrentClassTip = 0;
	m_bInited = false;
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CTFTips::Init()
{
	if ( !m_bInited )
	{
		// count how many tips there are for each class and in total
		m_iTipCountAll = 0;
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
		{
			// tip count per class is stored in resource file
			wchar_t *wzTipCount = g_pVGuiLocalize->Find( CFmtStr( "Tip_%d_Count", iClass ) );
			int iClassTipCount = wzTipCount ? _wtoi( wzTipCount ) : 0;
			m_iTipCount[iClass] = iClassTipCount;
			m_iTipCountAll += iClassTipCount;

			m_iArenaTipCount = g_pVGuiLocalize->Find( "Tip_arena_Count" ) ? _wtoi( g_pVGuiLocalize->Find( "Tip_arena_Count" ) ) : 0;	
		}

		// Captain Canteen mascot parts!
		m_CaptainCanteenBody.RemoveAll();
		m_CaptainCanteenMisc.RemoveAll();
		m_CaptainCanteenHat.RemoveAll();

		KeyValues *kv = new KeyValues( "cpncntn" );
		KeyValues::AutoDelete autodelete_key( kv );

		if ( kv->LoadFromFile( g_pFullFileSystem, "scripts/cpncntn.txt", "MOD" ) )
		{
			for ( KeyValues *pSubKey = kv->GetFirstTrueSubKey(); pSubKey; pSubKey = pSubKey->GetNextTrueSubKey() )
			{
				// Figure out which bucket it goes in
				CUtlVector< CaptainCanteenAsset_t > *pAssetBucket = NULL;

				if ( V_stricmp( pSubKey->GetName(), "body" ) == 0 )
				{
					pAssetBucket = &m_CaptainCanteenBody;
				}
				else if ( V_stricmp( pSubKey->GetName(), "misc" ) == 0 )
				{
					pAssetBucket = &m_CaptainCanteenMisc;
				}
				else if ( V_stricmp( pSubKey->GetName(), "hat" ) == 0 )
				{
					pAssetBucket = &m_CaptainCanteenHat;
				}

				if ( pAssetBucket )
				{
					// Added more assets to that bucket
					for ( KeyValues *pAssetKey = pSubKey->GetFirstSubKey(); pAssetKey; pAssetKey = pAssetKey->GetNextKey() )
					{
						int nNewAsset = pAssetBucket->AddToTail();
						V_strncpy( (*pAssetBucket)[ nNewAsset ].szImage, pAssetKey->GetName(), sizeof( (*pAssetBucket)[ nNewAsset ].szImage ) );
						(*pAssetBucket)[ nNewAsset ].flRarity = ( 1.0f / pAssetKey->GetFloat() );
					}
				}
			}
		}

		m_bInited = true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a random tip, selected from tips for all classes, 
//          fills in iClassUsed with the class the tip is for
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetRandomTip( int &iClassUsed )
{
	Init();

	// Chance of reminding players about the Abuse Reporter.
	// The chance is very high for the first 20 hours of play since newbies get picked on a lot.
	int abuseHintChance = 3;

	if ( CTFStatPanel::GetTotalHoursPlayed() < 20.0f )
	{
		abuseHintChance = 33;
	}

	if ( RandomInt( 1, 100 ) <= abuseHintChance )
	{
		iClassUsed = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
		return GetAbuseReportTip();
	}

	// pick a random tip
	int iTip = RandomInt( 0, m_iTipCountAll-1 );
	// walk through each class until we find the class this tip lands in
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		Assert( iTip >= 0 );
		int iClassTipCount = m_iTipCount[iClass]; 
		if ( iTip < iClassTipCount )
		{
			iClassUsed = iClass; 

			// return the tip
			return GetTip( iClass, iTip+1 );
		}
		iTip -= iClassTipCount;
	}
	Assert( false );	// shouldn't hit this

	iClassUsed = TF_CLASS_UNDEFINED;
	return L"";
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next tip for specified class
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetNextClassTip( int iClass )
{
	int iTipClass = TF_CLASS_UNDEFINED;

	// OK to call this function with TF_CLASS_UNDEFINED or TF_CLASS_RANDOM, just return a random tip for any class in that case
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_LAST_NORMAL_CLASS )
		return GetRandomTip( iTipClass );

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
	{
		return GetArenaTip();
	}
	
	int iClassTipCount = m_iTipCount[iClass];
	Assert( 0 != iClassTipCount );
	if ( 0 == iClassTipCount )
		return L"";
	// wrap the tip index to the valid range for this class
	if ( m_iCurrentClassTip >= iClassTipCount )
	{
		m_iCurrentClassTip %= iClassTipCount;
	}

	// return the tip
	const wchar_t *wzTip = GetTip( iClass, m_iCurrentClassTip+1 );
	m_iCurrentClassTip++;

	return wzTip;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specified tip index for arena
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetArenaTip( void )
{
	int iClassTipCount = m_iArenaTipCount;
	Assert( 0 != iClassTipCount );

	if ( 0 == iClassTipCount )
		return L"";

	// wrap the tip index to the valid range for this class
	if ( m_iCurrentClassTip >= iClassTipCount )
	{
		m_iCurrentClassTip %= iClassTipCount;
	}

	// return the tip
	const wchar_t *wzFmt = g_pVGuiLocalize->Find( CFmtStr( "#Tip_arena_%d", m_iCurrentClassTip+1 ) );
	static wchar_t wzTip[512] = L"";

	// replace any commands with their bound keys
	UTIL_ReplaceKeyBindings( wzFmt, 0, wzTip, sizeof( wzTip ) );

	m_iCurrentClassTip++;

	return wzTip;

}

//-----------------------------------------------------------------------------
// Purpose: Returns specified tip index for specified class
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetTip( int iClass, int iTip )
{
	static wchar_t wzTip[512] = L"";

	// get the tip
	const wchar_t *wzFmt = g_pVGuiLocalize->Find( CFmtStr( "#Tip_%d_%d", iClass, iTip ) );
	// replace any commands with their bound keys
	UTIL_ReplaceKeyBindings( wzFmt, 0, wzTip, sizeof( wzTip ) );

	return wzTip;
}

//-----------------------------------------------------------------------------
// Purpose: Returns tip about using the Abuse Reporter
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetAbuseReportTip( void )
{
	static wchar_t wzAbuseTip[512] = L"";

	// replace any commands with their bound keys
	const wchar_t *wzFmt = g_pVGuiLocalize->Find( "Tip_Abuse_Report" );
	UTIL_ReplaceKeyBindings( wzFmt, 0, wzAbuseTip, sizeof( wzAbuseTip ) );

	return wzAbuseTip;
}

//-----------------------------------------------------------------------------
// Purpose: Returns tip related to MvM
//-----------------------------------------------------------------------------
const wchar_t *CTFTips::GetRandomMvMTip( int &iClassUsed )
{
	Init();

	static wchar_t wzMvMTip[512] = L"";
	static int iPrevMvMClass = -1;

	iClassUsed = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );

	if ( iClassUsed == iPrevMvMClass )
	{
		iClassUsed = ( iClassUsed + 1 ) % TF_LAST_NORMAL_CLASS;
	}
	iPrevMvMClass = iClassUsed;

	// Get Tip Count
	CFmtStr fmtTipCount("Tip_MvM_%d_Count", iClassUsed );
	int iMvMTipCount = g_pVGuiLocalize->Find( fmtTipCount ) ? _wtoi( g_pVGuiLocalize->Find( fmtTipCount ) ) : 1;

	int iTip = RandomInt( 1, iMvMTipCount );

	// replace any commands with their bound keys
	const wchar_t *wzFmt = g_pVGuiLocalize->Find( CFmtStr( "#Tip_MvM_%d_%d", iClassUsed, iTip ) );
	UTIL_ReplaceKeyBindings( wzFmt, 0, wzMvMTip, sizeof( wzMvMTip ) );

	iPrevMvMClass = iClassUsed;
	return wzMvMTip;
}

void CTFTips::GetRandomCaptainCanteenImages( const char **ppchBody, const char **ppchMisc, const char **ppchHat )
{
	// Select and copy over all 3 parts
	*ppchBody = GetRandomCaptainCanteenAsset( &m_CaptainCanteenBody );
	*ppchMisc = GetRandomCaptainCanteenAsset( &m_CaptainCanteenMisc );
	*ppchHat = GetRandomCaptainCanteenAsset( &m_CaptainCanteenHat );
}

const char *CTFTips::GetRandomCaptainCanteenAsset( CUtlVector< CaptainCanteenAsset_t > *pAssetBucket )
{
	// If there's nothing in the bucket, bail out
	if ( pAssetBucket->Count() <= 0 )
	{
		return "";
	}

	if ( pAssetBucket->Count() == 1 )
	{
		// Easy out if there's only 1 choice
		return (*pAssetBucket)[ 0 ].szImage;
	}

	// Get the total scale of selection possibilities
	float flSelectionTotal = 0.0f;

	for ( int i = 0; i < pAssetBucket->Count(); ++i )
	{
		flSelectionTotal += (*pAssetBucket)[ i ].flRarity;
	}

	if ( flSelectionTotal > 0.0f )
	{
		// Pick a number from 0 to 1
		float flRand = RandomFloat();

		// Loop through to figure out which asset we've chosen
		float flCurrentPosition = 0.0f;

		for ( int i = 0; i < pAssetBucket->Count(); ++i )
		{
			flCurrentPosition += (*pAssetBucket)[ i ].flRarity / flSelectionTotal;

			if ( flRand <= flCurrentPosition )
			{
				// We landed on this random asset
				return (*pAssetBucket)[ i ].szImage;
			}
		}
	}

	// Something went wrong... just return the first choice
	return (*pAssetBucket)[ 0 ].szImage;
}


// global instance
CTFTips g_TFTips;