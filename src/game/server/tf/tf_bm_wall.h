//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_BM_WALL_H
#define TF_BM_WALL_H

#ifdef SOURCESDK

class CTFBMWall : public CBaseAnimating
{
	DECLARE_CLASS( CTFBMWall, CBaseAnimating );

public:
	CTFBMWall();

	virtual void Spawn( void );
	virtual void Precache( void );

	static CTFBMWall *CreateAtCell( int iCellX, int iCellY );
	static void RemoveAllWalls( void );

	int m_iCellX;
	int m_iCellY;
};

#endif // SOURCESDK

#endif // TF_BM_WALL_H
