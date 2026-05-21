//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_BM_CRATE_H
#define TF_BM_CRATE_H

#ifdef SOURCESDK

class CTFBMCrate : public CBaseAnimating
{
	DECLARE_CLASS( CTFBMCrate, CBaseAnimating );

public:
	CTFBMCrate();

	virtual void Spawn( void );
	virtual void Precache( void );

	static CTFBMCrate *CreateAtCell( int iCellX, int iCellY );
	static CTFBMCrate *GetCrateAtCell( int iCellX, int iCellY );
	static void RemoveAllCrates( void );

	int m_iCellX;
	int m_iCellY;
};

#endif // SOURCESDK

#endif // TF_BM_CRATE_H
