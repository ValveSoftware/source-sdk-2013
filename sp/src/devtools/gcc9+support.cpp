// compatibility with old ABI
#if __GNUC__ >= 9
#include <cmath>

extern "C" double __pow_finite(double a, double b)
{
	return pow(a, b);
}

extern "C" double __log_finite(double a)
{
	return log(a);
}

extern "C" double __exp_finite(double a)
{
	return exp(a);
}

extern "C" double __atan2_finite(double a, double b)
{
	return atan2(a, b);
}

extern "C" double __atan2f_finite(double a, double b)
{
	return atan2f(a, b);
}

extern "C" float __powf_finite(float a, float b)
{
	return powf(a, b);
}

extern "C" float __logf_finite(float a)
{
	return logf(a);
}

extern "C" float __expf_finite(float a)
{
	return expf(a);
}

extern "C" float __acosf_finite(float a)
{
	return acosf(a);
}

extern "C" double __asin_finite(double a)
{
	return asin(a);
}

extern "C" double __acos_finite(double a)
{
	return acos(a);
}
#endif
