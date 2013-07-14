#define __builtin_apply(x,y,z) ((void*)0)
#define __builtin_nan(x) (0.0)
#define __builtin_nanf(x) (0.0f)
#define __builtin_nanl(x) (0.0l)
#define __builtin_huge_val(x) (0.0)
#define __builtin_huge_valf(x) (0.0f)
#define __builtin_huge_vall(x) (0.0l)
#define __builtin_apply_args(x) ((void*)0)
#define __builtin_types_compatible_p(x,y) 0
#define __builtin_choose_expr(x,y,z) int
#define __builtin_constant_p(x) 0
void* __builtin_memchr(void const*, int, unsigned int);
void __builtin_return (void *RESULT);
void * __builtin_return_address (unsigned int LEVEL);
void * __builtin_frame_address (unsigned int LEVEL);
long __builtin_expect (long EXP, long C);
void __builtin_prefetch (const void *ADDR, ...);
double __builtin_inf (void);
float __builtin_inff (void);
long double __builtin_infl (void);
double __builtin_nans (const char *str);
float __builtin_nansf (const char *str);
long double __builtin_nansl (const char *str);
double      __builtin_acos(double);
float       __builtin_acosf(float);
long double __builtin_acosl(long double);
double      __builtin_asin(double);
float       __builtin_asinf(float);
long double __builtin_asinl(long double);
double      __builtin_atan(double);
double      __builtin_atan2(double, double);
float       __builtin_atan2f(float, float);
long double __builtin_atan2l(long double, long double);
float       __builtin_atanf(float);
long double __builtin_atanl(long double);
double      __builtin_ceil(double);
float       __builtin_ceilf(float);
long double __builtin_ceill(long double);
double      __builtin_cos(double);
float       __builtin_cosf(float);
double      __builtin_cosh(double);
float       __builtin_coshf(float);
long double __builtin_coshl(long double);
long double __builtin_cosl(long double);
double      __builtin_exp(double);
float       __builtin_expf(float);
long double __builtin_expl(long double);
double      __builtin_fabs(double);
float       __builtin_fabsf(float);
long double __builtin_fabsl(long double);
double      __builtin_floor(double);
float       __builtin_floorf(float);
long double __builtin_floorl(long double);
float       __builtin_fmodf(float, float);
long double __builtin_fmodl(long double, long double);
double      __builtin_frexp(double, int*);
float       __builtin_frexpf(float, int*);
long double __builtin_frexpl(long double, int*);
double      __builtin_ldexp(double, int);
float       __builtin_ldexpf(float, int);
long double __builtin_ldexpl(long double, int);
double      __builtin_log(double);
double      __builtin_log10(double);
float       __builtin_log10f(float);
long double __builtin_log10l(long double);
float       __builtin_logf(float);
long double __builtin_logl(long double);
float       __builtin_modff(float, float*);
long double __builtin_modfl(long double, long double*);
float       __builtin_powf(float, float);
long double __builtin_powl(long double, long double);
double      __builtin_powi(double, int);
float       __builtin_powif(float, int);
long double __builtin_powil(long double, int);
double      __builtin_sin(double);
float       __builtin_sinf(float);
double      __builtin_sinh(double);
float       __builtin_sinhf(float);
long double __builtin_sinhl(long double);
long double __builtin_sinl(long double);
double      __builtin_sqrt(double);
float       __builtin_sqrtf(float);
long double __builtin_sqrtl(long double);
double      __builtin_tan(double);
float       __builtin_tanf(float);
double      __builtin_tanh(double);
float       __builtin_tanhf(float);
long double __builtin_tanhl(long double);
long double __builtin_tanl(long double);
float       __builtin_cabsf(float __complex__);
double      __builtin_cabs(double __complex__);
long double __builtin_cabsl(long double __complex__);
float       __builtin_cargf(float __complex__);
double      __builtin_carg(double __complex__);
long double __builtin_cargl(long double __complex__);
int         __builtin_ctz(int);
int         __builtin_ctzl(long);
int         __builtin_ctzll(long long);
int         __builtin_popcount(int);
int         __builtin_popcountl(long);
int         __builtin_popcountll(long long);
float       __complex__ __builtin_ccosf(float __complex__);
double      __complex__ __builtin_ccos(double __complex__);
long double __complex__ __builtin_ccosl(long double __complex__);
float       __complex__ __builtin_ccoshf(float __complex__);
double      __complex__ __builtin_ccosh(double __complex__);
long double __complex__ __builtin_ccoshl(long double __complex__);
float       __complex__ __builtin_cexpf(float __complex__);
double      __complex__ __builtin_cexp(double __complex__);
long double __complex__ __builtin_cexpl(long double __complex__);
float       __complex__ __builtin_clogf(float __complex__);
double      __complex__ __builtin_clog(double __complex__);
long double __complex__ __builtin_clogl(long double __complex__);
float       __complex__ __builtin_csinf(float __complex__);
double      __complex__ __builtin_csin(double __complex__);
long double __complex__ __builtin_csinl(long double __complex__);
float       __complex__ __builtin_csinhf(float __complex__);
double      __complex__ __builtin_csinh(double __complex__);
long double __complex__ __builtin_csinhl(long double __complex__);
float       __complex__ __builtin_csqrtf(float __complex__);
double      __complex__ __builtin_csqrt(double __complex__);
long double __complex__ __builtin_csqrtl(long double __complex__);
float       __complex__ __builtin_ctanf(float __complex__);
double      __complex__ __builtin_ctan(double __complex__);
long double __complex__ __builtin_ctanl(long double __complex__);
float       __complex__ __builtin_ctanhf(float __complex__);
double      __complex__ __builtin_ctanh(double __complex__);
long double __complex__ __builtin_ctanhl(long double __complex__);
float       __complex__ __builtin_cpowf(float __complex__, float __complex__);
double      __complex__ __builtin_cpow(double __complex__, double __complex__);
long double __complex__ __builtin_cpowl(long double __complex__, long double __complex__);

/* The GCC 4.5 parser hard-codes handling of these, so they do not
   have real signatures.  */
bool __builtin_fpclassify(...);
bool __builtin_isfinite(...);
bool __builtin_isgreater(...);
bool __builtin_isgreaterequal(...);
bool __builtin_isinf(...);
bool __builtin_isinf_sign(...);
bool __builtin_isless(...);
bool __builtin_islessequal(...);
bool __builtin_islessgreater(...);
bool __builtin_isnan(...);
bool __builtin_isnormal(...);
bool __builtin_isunordered(...);
bool __builtin_va_arg_pack(...);
int  __builtin_va_arg_pack_len(...);

/* We fake some constant expressions from GCC 4.5 parser.  */
#define __is_empty(x) false
#define __is_pod(x) false
#define __is_trivial(x) false
#define __has_trivial_destructor(x) false
#define __has_trivial_constructor(x) false

extern unsigned int  __builtin_bswap32(unsigned int _data);
extern unsigned long __builtin_bswap64(unsigned long _data);
