//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_emerge.h
// The Halloween Boss emerging from the ground
// Michael Booth, October 2010

#ifndef HEADLESS_HATMAN_EMERGE_H
#define HEADLESS_HATMAN_EMERGE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CHeadlessHatmanEmerge : public Action< CHeadlessHatman >
{
public:
	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );
	virtual const char *GetName( void ) const	{ return "Emerge"; }		// return name of this action

private:
	CountdownTimer m_riseTimer;
	CountdownTimer m_rumbleTimer;
	Vector m_emergePos;
	float m_height;
};


#endif // HEADLESS_HATMAN_EMERGE_H
