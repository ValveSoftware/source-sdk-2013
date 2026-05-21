//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_BM_BOMB_H
#define TF_BM_BOMB_H

#ifdef SOURCESDK

class CTFBMBomb : public CBaseAnimating
{
	DECLARE_CLASS( CTFBMBomb, CBaseAnimating );

public:
	CTFBMBomb();

	virtual void Spawn( void );
	virtual void Precache( void );
	void BombThink( void );

	static CTFBMBomb *PlaceAtCell( CTFPlayer *pOwner, int iCellX, int iCellY );
	static CTFBMBomb *GetBombAtCell( int iCellX, int iCellY );

	void Detonate( void );

	int m_iCellX;
	int m_iCellY;
	float m_flDetonateTime;
	int m_iBlastRange;
	EHANDLE m_hOwnerPlayer;
	bool m_bDetonating;
};

#endif // SOURCESDK

#endif // TF_BM_BOMB_H
