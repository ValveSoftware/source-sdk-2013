#ifndef TIER0_CACHE_HINTS_HDR
#define TIER0_CACHE_HINTS_HDR

#if   defined(WIN32)
// NOTE: In every case I've tested so far using this prefetch on PC is actually slower.  Changing it actually 
// prefetch 128-bytes (tested on a PC with 64-byte cache lines) makes it even slower
// It is much more difficult to improve performance with prefetch on the PC.  I suggest trying to make your data
// linear and let the hardware prefetch do the work for you.  Otherwise you can prefetch in 64-byte chunks with this:
#define PREFETCH_64_PC(POINTER,OFFSET)	{ _mm_prefetch((const char*)(POINTER) + (OFFSET), _MM_HINT_T0); }

// leave this empty because we can't improve perf of any existing cases by defining it
#define PREFETCH_128(POINTER,OFFSET)	{ /* Nothing to do here */ }

// The concept of zeroing the cache does not exist the same way on PC. Nevertheless, simulate the same behavior. 
#define PREZERO_128(POINTER,OFFSET)											\
	{																		\
		intptr_t __tempPtr__ = (intptr_t)((char *)(POINTER) + (OFFSET));	\
		__tempPtr__ &= -128;												\
		memset((void*)__tempPtr__, 0, 128);									\
	}
#else
// Same for other platforms.
#define PREFETCH_128(POINTER,OFFSET)	{ /* Nothing to do here */ }
#define PREZERO_128(POINTER,OFFSET)											\
	{																		\
		intptr_t __tempPtr__ = (intptr_t)((char *)(POINTER) + (OFFSET));	\
		__tempPtr__ &= -128;												\
		memset((void*)__tempPtr__, 0, 128);									\
	}
#endif

// This exists for backward compatibility until a massive search and replace is done
#define PREFETCH_CACHE_LINE PREFETCH_128
// Indicate that the cache line is 128. It is not correct on PC, but this will have no side effects related to the macros above.
#define CACHE_LINE_SIZE	128

#ifdef IVP_VECTOR_INCLUDED
template<class T>
inline void UnsafePrefetchLastElementOf(IVP_U_Vector<T>&array)
{
	PREFETCH_128(array.element_at(array.len()-1),0);
}
template<class T>
inline void PrefetchLastElementOf(IVP_U_Vector<T>&array)
{
	if(array.len() > 0)
		PREFETCH_128(array.element_at(array.len()-1),0);
}
#endif

#endif 
