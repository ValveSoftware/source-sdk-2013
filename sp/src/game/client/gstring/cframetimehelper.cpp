
#include "cbase.h"
#include "Gstring/cFrametimeHelper.h"
#include "vgui_controls/controls.h"
#include "vgui/isystem.h"

static CFrameTimeHelper g_FrameTimeHelper;

CFrameTimeHelper::CFrameTimeHelper() : CAutoGameSystemPerFrame( "AutoFrameTimeAccess" )
{
	m_flFrameTime = 0.0f;
	m_flFrameTimeLast = 0.0f;
}

void CFrameTimeHelper::Update( float frametime )
{
	double curframetime = vgui::system()->GetCurrentTime();
	m_flFrameTime = min( 1.0f, curframetime - m_flFrameTimeLast );
	m_flFrameTimeLast = curframetime;
}

double CFrameTimeHelper::GetFrameTime()
{
	return g_FrameTimeHelper.m_flFrameTime;
}

double CFrameTimeHelper::GetCurrentTime()
{
	return vgui::system()->GetCurrentTime();
}

void CFrameTimeHelper::RandomStart()
{
	int count = fmod( GetCurrentTime() * 10000.0, 1.0 ) * 10;
	for ( int i = 0; i < count; i++ )
		RandomInt( 0, 1 );
}