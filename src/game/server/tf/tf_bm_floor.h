//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_BM_FLOOR_H
#define TF_BM_FLOOR_H

#ifdef SOURCESDK

class CTFBMFloor : public CBaseEntity
{
	DECLARE_CLASS( CTFBMFloor, CBaseEntity );

public:
	virtual void Spawn( void );

	static CTFBMFloor *CreateForArena( const Vector &vecCenter, float flWidth, float flDepth, float flTopZ );
	static void RemoveAllFloors( void );
};

#endif // SOURCESDK

#endif // TF_BM_FLOOR_H
