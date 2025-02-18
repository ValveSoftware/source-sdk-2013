//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_terrify.h
// The Halloween Boss leans over and yells "Boo!", terrifying nearby victims
// Michael Booth, October 2010

#ifndef HEADLESS_HATMAN_TERRIFY_H
#define HEADLESS_HATMAN_TERRIFY_H

//---------------------------------------------------------------------------------------------
class CHeadlessHatmanTerrify : public Action< CHeadlessHatman >
{
public:
	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );
	virtual const char *GetName( void ) const	{ return "Terrify"; }		// return name of this action

private:
	CountdownTimer m_booTimer;
	CountdownTimer m_scareTimer;
	CountdownTimer m_timer;

	bool IsWearingPumpkinHeadOrSaxtonMask( CTFPlayer *player );
};


#endif // HEADLESS_HATMAN_TERRIFY_H
