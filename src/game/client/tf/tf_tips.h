//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF implementation of the IPresence interface
//
//=============================================================================

#ifndef TF_TIPS_H
#define TF_TIPS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"


struct CaptainCanteenAsset_t
{
	char szImage[ MAX_PATH ];
	float flRarity;
};

//-----------------------------------------------------------------------------
// Purpose: helper class for TF tips
//-----------------------------------------------------------------------------
class CTFTips : public CAutoGameSystem
{
public:
	CTFTips();

	virtual bool Init();
	virtual char const *Name() { return "CTFTips"; }

	const wchar_t *GetRandomTip( int &iClassUsed ); // iClassUsed will be filled in with the class that was selected
	const wchar_t *GetNextClassTip( int iClass );
	const wchar_t *GetArenaTip( void );
	const wchar_t *GetAbuseReportTip( void );
	const wchar_t *GetRandomMvMTip( int &iClassUsed ); // iClassUsed will be filled in with the class that was selected
	void GetRandomCaptainCanteenImages( const char **ppchBody, const char **ppchMisc, const char **ppchHat );

private:
	const wchar_t *GetTip( int iClass, int iTip );
	const char *GetRandomCaptainCanteenAsset( CUtlVector< CaptainCanteenAsset_t > *pAssetBucket );

	int m_iTipCount[TF_LAST_NORMAL_CLASS+1];		// how many tips there are for each class
	int m_iTipCountAll;								// how many tips there are total
	int m_iCurrentClassTip;							// index of current per-class tip
	bool m_bInited;									// have we been initialized
	int	m_iArenaTipCount;
	int m_iMvMTipCount;

	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenBody;
	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenMisc;
	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenHat;
};

extern CTFTips g_TFTips;
#endif // TF_TIPS_H
