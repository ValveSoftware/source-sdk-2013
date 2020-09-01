//-----------------------------------------------------------------------
// see copyright notice in squirrel.h
//
// Purpose : Squirrel time library cropped out from 
//           the system library as a safe include.
//
//-----------------------------------------------------------------------

#include "squirrel.h"
#include "time.h"

static SQInteger _system_clock(HSQUIRRELVM v)
{
	sq_pushfloat(v, ((SQFloat)clock()) / (SQFloat)CLOCKS_PER_SEC);
	return 1;
}

static SQInteger _system_time(HSQUIRRELVM v)
{
	SQInteger t = (SQInteger)time(NULL);
	sq_pushinteger(v, t);
	return 1;
}

static void _set_integer_slot(HSQUIRRELVM v, const SQChar *name, SQInteger val)
{
	sq_pushstring(v, name, -1);
	sq_pushinteger(v, val);
	sq_rawset(v, -3);
}

static SQInteger _system_date(HSQUIRRELVM v)
{
	time_t t;
	SQInteger it;
	SQInteger format = 'l';
	if (sq_gettop(v) > 1) {
		sq_getinteger(v, 2, &it);
		t = it;
		if (sq_gettop(v) > 2) {
			sq_getinteger(v, 3, (SQInteger*)&format);
		}
	}
	else {
		time(&t);
	}
	tm *date;
	if (format == 'u')
		date = gmtime(&t);
	else
		date = localtime(&t);
	if (!date)
		return sq_throwerror(v, _SC("crt api failure"));
	sq_newtable(v);
	_set_integer_slot(v, _SC("sec"), date->tm_sec);
	_set_integer_slot(v, _SC("min"), date->tm_min);
	_set_integer_slot(v, _SC("hour"), date->tm_hour);
	_set_integer_slot(v, _SC("day"), date->tm_mday);
	_set_integer_slot(v, _SC("month"), date->tm_mon);
	_set_integer_slot(v, _SC("year"), date->tm_year + 1900);
	_set_integer_slot(v, _SC("wday"), date->tm_wday);
	_set_integer_slot(v, _SC("yday"), date->tm_yday);
	return 1;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const SQRegFunction timelib_funcs[] = {
	_DECL_FUNC(clock, 0, NULL),
	_DECL_FUNC(time, 1, NULL),
	_DECL_FUNC(date, -1, _SC(".nn")),
	{ NULL, (SQFUNCTION)0, 0, NULL }
};
#undef _DECL_FUNC

SQInteger sqstd_register_timelib(HSQUIRRELVM v)
{
	SQInteger i = 0;
	while (timelib_funcs[i].name != 0)
	{
		sq_pushstring(v, timelib_funcs[i].name, -1);
		sq_newclosure(v, timelib_funcs[i].f, 0);
		sq_setparamscheck(v, timelib_funcs[i].nparamscheck, timelib_funcs[i].typemask);
		sq_setnativeclosurename(v, -1, timelib_funcs[i].name);
		sq_newslot(v, -3, SQFalse);
		i++;
	}
	return 1;
}
