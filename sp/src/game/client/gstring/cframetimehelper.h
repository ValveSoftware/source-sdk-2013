#ifndef FRAME_TIME_HELPER_H
#define FRAME_TIME_HELPER_H

#include "cbase.h"

class CFrameTimeHelper : public CAutoGameSystemPerFrame
{
public:
	CFrameTimeHelper();

	void Update( float frametime );

	static double GetFrameTime();
	static double GetCurrentTime();

	static void RandomStart();

private:
	double m_flFrameTime;
	double m_flFrameTimeLast;
};




#endif