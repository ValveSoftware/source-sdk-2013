//========= Copyright Valve Corporation, All rights reserved. ============//
#if !defined(POSIX)
#ifndef min
	#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
	#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#endif
