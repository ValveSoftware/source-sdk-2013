//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "mathlib/ssemath.h"
#include "mathlib/ssequaternion.h"

const fltx4 Four_PointFives={0.5,0.5,0.5,0.5};
#ifndef _X360
const fltx4 Four_Zeros={0.0,0.0,0.0,0.0};
const fltx4 Four_Ones={1.0,1.0,1.0,1.0};
#endif
const fltx4 Four_Twos={2.0,2.0,2.0,2.0};
const fltx4 Four_Threes={3.0,3.0,3.0,3.0};
const fltx4 Four_Fours={4.0,4.0,4.0,4.0};
const fltx4 Four_Origin={0,0,0,1};
const fltx4 Four_NegativeOnes={-1,-1,-1,-1};

const fltx4 Four_2ToThe21s={ (float) (1<<21), (float) (1<<21), (float) (1<<21), (float)(1<<21) };
const fltx4 Four_2ToThe22s={ (float) (1<<22), (float) (1<<22), (float) (1<<22), (float)(1<<22) };
const fltx4 Four_2ToThe23s={ (float) (1<<23), (float) (1<<23), (float) (1<<23), (float)(1<<23) };
const fltx4 Four_2ToThe24s={ (float) (1<<24), (float) (1<<24), (float) (1<<24), (float)(1<<24) };

const fltx4 Four_Point225s={ .225, .225, .225, .225 };
const fltx4 Four_Epsilons={FLT_EPSILON,FLT_EPSILON,FLT_EPSILON,FLT_EPSILON};

const fltx4 Four_FLT_MAX={FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
const fltx4 Four_Negative_FLT_MAX={-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX};
const fltx4 g_SIMD_0123 = { 0., 1., 2., 3. };

const fltx4 g_QuatMultRowSign[4] =
{
	{  1.0f,  1.0f, -1.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f, 1.0f },
	{ -1.0f, -1.0f, -1.0f, 1.0f }
};

const int32 ALIGN16 g_SIMD_clear_signmask[4] ALIGN16_POST = {0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff};
const int32 ALIGN16 g_SIMD_signmask[4] ALIGN16_POST = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
const int32 ALIGN16 g_SIMD_lsbmask[4] ALIGN16_POST = { 0xfffffffe, 0xfffffffe, 0xfffffffe, 0xfffffffe };
const int32 ALIGN16 g_SIMD_clear_wmask[4] ALIGN16_POST = { 0xffffffff, 0xffffffff, 0xffffffff, 0 };
const int32 ALIGN16 g_SIMD_AllOnesMask[4] ALIGN16_POST = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }; // ~0,~0,~0,~0
const int32 ALIGN16 g_SIMD_Low16BitsMask[4] ALIGN16_POST = { 0xffff, 0xffff, 0xffff, 0xffff }; // 0xffff x 4

const int32 ALIGN16 g_SIMD_ComponentMask[4][4] ALIGN16_POST =
{
	{ 0xFFFFFFFF, 0, 0, 0 }, { 0, 0xFFFFFFFF, 0, 0 }, { 0, 0, 0xFFFFFFFF, 0 }, { 0, 0, 0, 0xFFFFFFFF }
};

const int32 ALIGN16 g_SIMD_SkipTailMask[4][4] ALIGN16_POST =
{
	{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
	{ 0xffffffff, 0x00000000, 0x00000000, 0x00000000 },
	{ 0xffffffff, 0xffffffff, 0x00000000, 0x00000000 },
	{ 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000 },
};


	// FUNCTIONS
	// NOTE: WHY YOU **DO NOT** WANT TO PUT FUNCTIONS HERE
// Generally speaking, you want to make sure SIMD math functions
// are inlined, because that gives the compiler much more latitude
// in instruction scheduling. It's not that the overhead of calling
// the function is particularly great; rather, many of the SIMD 
// opcodes have long latencies, and if you have a sequence of 
// several dependent ones inside a function call, the latencies 
// stack up to create a big penalty. If the function is inlined,
// the compiler can interleave its operations with ones from the
// caller to better hide those latencies. Finally, on the 360,
// putting parameters or return values on the stack, and then 
// reading them back within the next forty cycles, is a very 
// severe penalty. So, as much as possible, you want to leave your
// data on the registers.

// That said, there are certain occasions where it is appropriate
// to call into functions -- particularly for very large blocks
// of code that will spill most of the registers anyway. Unless your
// function is more than one screen long, yours is probably not one
// of those occasions.



/// You can use this to rotate a long array of FourVectors all by the same
/// matrix. The first parameter is the head of the array. The second is the
/// number of vectors to rotate. The third is the matrix.
void FourVectors::RotateManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix )
{
	Assert(numVectors > 0);
	if ( numVectors == 0 )
		return;

	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02,
		matSplat10, matSplat11, matSplat12,
		matSplat20, matSplat21, matSplat22;

	{
		// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
		// often unaligned. The w components will be the tranpose row of
		// the matrix, but we don't really care about that.
		fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
		fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
		fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

		matSplat00 = SplatXSIMD(matCol0);
		matSplat01 = SplatYSIMD(matCol0);
		matSplat02 = SplatZSIMD(matCol0);

		matSplat10 = SplatXSIMD(matCol1);
		matSplat11 = SplatYSIMD(matCol1);
		matSplat12 = SplatZSIMD(matCol1);

		matSplat20 = SplatXSIMD(matCol2);
		matSplat21 = SplatYSIMD(matCol2);
		matSplat22 = SplatZSIMD(matCol2);
	}

#ifdef _X360
	// Same algorithm as above, but the loop is unrolled to eliminate data hazard latencies
	// and simplify prefetching. Named variables are deliberately used instead of arrays to
	// ensure that the variables live on the registers instead of the stack (stack load/store
	// is a serious penalty on 360).  Nb: for prefetching to be most efficient here, the
	// loop should be unrolled to 8 FourVectors per iteration; because each FourVectors is 
	// 48 bytes long, 48 * 8 = 384, its least common multiple with the 128-byte cache line. 
	// That way you can fetch the next 3 cache lines while you work on these three. 
	// If you do go this route, be sure to dissassemble and make sure it doesn't spill 
	// registers to stack as you do this; the cost of that will be excessive. Unroll the loop
	// a little and just live with the fact that you'll be doing a couple of redundant dbcts
	// (they don't cost you anything). Be aware that all three cores share L2 and it can only
	// have eight cache lines fetching at a time.
	fltx4 outX0, outY0, outZ0; // bank one of outputs
	fltx4 outX1, outY1, outZ1; // bank two of outputs


	// Because of instruction latencies and scheduling, it's actually faster to use adds and muls
	// rather than madds. (Empirically determined by timing.)
	const FourVectors * stop = pVectors + numVectors;
	FourVectors * RESTRICT pVectNext;
	// prime the pump.
	if (numVectors & 0x01)
	{
		// odd number of vectors to process
		// prime the 1 group of registers
		pVectNext = pVectors++;
		outX1 = AddSIMD( AddSIMD( MulSIMD( pVectNext->x, matSplat00 ), MulSIMD( pVectNext->y, matSplat01 ) ), MulSIMD( pVectNext->z, matSplat02 ) );
		outY1 = AddSIMD( AddSIMD( MulSIMD( pVectNext->x, matSplat10 ), MulSIMD( pVectNext->y, matSplat11 ) ), MulSIMD( pVectNext->z, matSplat12 ) );
		outZ1 = AddSIMD( AddSIMD( MulSIMD( pVectNext->x, matSplat20 ), MulSIMD( pVectNext->y, matSplat21 ) ), MulSIMD( pVectNext->z, matSplat22 ) );
	}
	else
	{
		// even number of total vectors to process; 
		// prime the zero group and jump into the middle of the loop
		outX0 = AddSIMD( AddSIMD( MulSIMD( pVectors->x, matSplat00 ), MulSIMD( pVectors->y, matSplat01 ) ), MulSIMD( pVectors->z, matSplat02 ) );
		outY0 = AddSIMD( AddSIMD( MulSIMD( pVectors->x, matSplat10 ), MulSIMD( pVectors->y, matSplat11 ) ), MulSIMD( pVectors->z, matSplat12 ) );
		outZ0 = AddSIMD( AddSIMD( MulSIMD( pVectors->x, matSplat20 ), MulSIMD( pVectors->y, matSplat21 ) ), MulSIMD( pVectors->z, matSplat22 ) );
		goto EVEN_CASE;
	}

	// perform an even number of iterations through this loop.
	while (pVectors < stop)
	{
		outX0 = MaddSIMD( pVectors->z, matSplat02, AddSIMD( MulSIMD( pVectors->x, matSplat00 ), MulSIMD( pVectors->y, matSplat01 ) ) );
		outY0 = MaddSIMD( pVectors->z, matSplat12, AddSIMD( MulSIMD( pVectors->x, matSplat10 ), MulSIMD( pVectors->y, matSplat11 ) ) );
		outZ0 = MaddSIMD( pVectors->z, matSplat22, AddSIMD( MulSIMD( pVectors->x, matSplat20 ), MulSIMD( pVectors->y, matSplat21 ) ) );

		pVectNext->x = outX1;
		pVectNext->y = outY1;
		pVectNext->z = outZ1;

EVEN_CASE:
		pVectNext = pVectors+1;

		outX1 = MaddSIMD( pVectNext->z, matSplat02, AddSIMD( MulSIMD( pVectNext->x, matSplat00 ), MulSIMD( pVectNext->y, matSplat01 ) ) );
		outY1 = MaddSIMD( pVectNext->z, matSplat12, AddSIMD( MulSIMD( pVectNext->x, matSplat10 ), MulSIMD( pVectNext->y, matSplat11 ) ) );
		outZ1 = MaddSIMD( pVectNext->z, matSplat22, AddSIMD( MulSIMD( pVectNext->x, matSplat20 ), MulSIMD( pVectNext->y, matSplat21 ) ) );

		pVectors->x = outX0;
		pVectors->y = outY0;
		pVectors->z = outZ0;

		pVectors += 2;
	}

	// flush the last round of output
	pVectNext->x = outX1;
	pVectNext->y = outY1;
	pVectNext->z = outZ1;
#else
	// PC does not benefit from the unroll/scheduling above
	fltx4 outX0, outY0, outZ0; // bank one of outputs


	// Because of instruction latencies and scheduling, it's actually faster to use adds and muls
	// rather than madds. (Empirically determined by timing.)
	const FourVectors * stop = pVectors + numVectors;

	// perform an even number of iterations through this loop.
	while (pVectors < stop)
	{
		outX0 = MaddSIMD( pVectors->z, matSplat02, AddSIMD( MulSIMD( pVectors->x, matSplat00 ), MulSIMD( pVectors->y, matSplat01 ) ) );
		outY0 = MaddSIMD( pVectors->z, matSplat12, AddSIMD( MulSIMD( pVectors->x, matSplat10 ), MulSIMD( pVectors->y, matSplat11 ) ) );
		outZ0 = MaddSIMD( pVectors->z, matSplat22, AddSIMD( MulSIMD( pVectors->x, matSplat20 ), MulSIMD( pVectors->y, matSplat21 ) ) );

		pVectors->x = outX0;
		pVectors->y = outY0;
		pVectors->z = outZ0;
		pVectors++;
	}
#endif
}

#ifdef _X360
// Loop-scheduled code to process FourVectors in groups of eight quite efficiently.
void FourVectors_TransformManyGroupsOfEightBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix, FourVectors * RESTRICT pOut )
{
	Assert(numVectors > 0);
	if ( numVectors == 0 )
		return;

	AssertMsg( (pOut < pVectors && pOut+numVectors <= pVectors) ||
			   (pOut > pVectors && pVectors+numVectors <= pOut), "FourVectors::TransformManyBy called with overlapping buffer pointers." );

	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02, matSplat03,	// TWELVE REGISTERS
		  matSplat10, matSplat11, matSplat12, matSplat13,
		  matSplat20, matSplat21, matSplat22, matSplat23;

	{
		// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
		// often unaligned. The w components will be the tranpose row of
		// the matrix.
		fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
		fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
		fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

		matSplat00 = SplatXSIMD(matCol0);
		matSplat01 = SplatYSIMD(matCol0);
		matSplat02 = SplatZSIMD(matCol0);
		matSplat03 = SplatWSIMD(matCol0);

		matSplat10 = SplatXSIMD(matCol1);
		matSplat11 = SplatYSIMD(matCol1);
		matSplat12 = SplatZSIMD(matCol1);
		matSplat13 = SplatWSIMD(matCol1);

		matSplat20 = SplatXSIMD(matCol2);
		matSplat21 = SplatYSIMD(matCol2);
		matSplat22 = SplatZSIMD(matCol2);
		matSplat23 = SplatWSIMD(matCol2);
	}

	// this macro defines how to compute a specific row from an input and certain splat columns
#define COMPUTE(res, invec, xterm, yterm, zterm, transterm) res = AddSIMD( AddSIMD( MulSIMD((invec)->z, zterm), AddSIMD( MulSIMD( (invec)->x, xterm ), MulSIMD( (invec)->y, yterm ) ) ), transterm )
#define WRITE(term, reg, toptr) toptr->term = reg

	// define result groups (we're going to have an eight-way unroll)
	
	fltx4 res0X, res0Y, res0Z, res0XTemp, res0YTemp, res0ZTemp;	// 48 REGISTERS
	fltx4 res1X, res1Y, res1Z, res1XTemp, res1YTemp, res1ZTemp;
	fltx4 res2X, res2Y, res2Z, res2XTemp, res2YTemp, res2ZTemp;
	fltx4 res3X, res3Y, res3Z, res3XTemp, res3YTemp, res3ZTemp;
	fltx4 res4X, res4Y, res4Z, res4XTemp, res4YTemp, res4ZTemp;
	fltx4 res5X, res5Y, res5Z, res5XTemp, res5YTemp, res5ZTemp;
	fltx4 res6X, res6Y, res6Z, res6XTemp, res6YTemp, res6ZTemp;
	fltx4 res7X, res7Y, res7Z, res7XTemp, res7YTemp, res7ZTemp;
	

// #define FROZ(out,in,offset) COMPUTE((out+offset)->x, (in + offset), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE((out + offset )->y, (in + offset), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE((out + offset)->z, (in + offset), matSplat20, matSplat21, matSplat22, matSplat23)
#define COMPUTE_GROUP(resgroup,dataptr) COMPUTE(resgroup ## X, (dataptr), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE(resgroup ## Y, (dataptr), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE(resgroup ## Z, (dataptr), matSplat20, matSplat21, matSplat22, matSplat23)
#define WRITE_GROUP(ptr, resgroup) (ptr)->x = resgroup ## X; (ptr)->y = resgroup ## Y; (ptr)->z = resgroup ## Z

	/*
	// stage 1 -- 6 ops for xyz, each w 12 cycle latency
	res0X = MulSIMD( (invec)->y, matSplat01 );
	res0Temp = MaddSIMD((invec)->z, matSplat02, matSplat03);
	// stage 2 -- 3 clocks for xyz
	res0X = MaddSIMD( (invec)->x, matSplat00, res0X );
	// stage 3 -- 3 clocks for xyz
	res0X = AddSIMD(res0X, res0Temp);
	*/
#define COMPUTE_STAGE1_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MulSIMD( (invec)->y, ysplat ); tempvar = MaddSIMD((invec)->z, zsplat, transplat)
#define COMPUTE_STAGE2_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MaddSIMD( (invec)->x, xsplat, res )
#define COMPUTE_STAGE3_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = AddSIMD(res, tempvar)  // frees up the tempvar

#define COMPUTE_STAGE1_GROUP(resgroup, invec) COMPUTE_STAGE1_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
										COMPUTE_STAGE1_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
										COMPUTE_STAGE1_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE2_GROUP(resgroup, invec) COMPUTE_STAGE2_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
										COMPUTE_STAGE2_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
										COMPUTE_STAGE2_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE3_GROUP(resgroup, invec) COMPUTE_STAGE3_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
										COMPUTE_STAGE3_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
										COMPUTE_STAGE3_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

	FourVectors * RESTRICT inData = pVectors;
	FourVectors * RESTRICT outData = pOut;
	const FourVectors * const RESTRICT STOP = pVectors + numVectors;

	// Use techniques of loop scheduling to eliminate data hazards; process
	// eight groups simultaneously so that we never have any operations stalling
	// waiting for data.
	// Note: this loop, while pretty fast, could be faster still -- you'll notice
	// that it does all of its loads, then all computation, then writes everything
	// out. If made truly cyclic, such that every line interleaved a stage 1, stage 2,
	// stage 3, and write, then throughput could be higher (probably by about 50%). 
	while (inData < STOP)
	{
		// start prefetching the three cache lines
		// we'll hit two iterations from now
		__dcbt( sizeof(FourVectors) * 16,       inData );
		__dcbt( sizeof(FourVectors) * 16 + 128, inData );
		__dcbt( sizeof(FourVectors) * 16 + 256, inData );

		// synchro
		COMPUTE_STAGE1_GROUP(res0, inData + 0);
		COMPUTE_STAGE1_GROUP(res1, inData + 1);
		COMPUTE_STAGE1_GROUP(res2, inData + 2);
		COMPUTE_STAGE1_GROUP(res3, inData + 3);

			COMPUTE_STAGE2_GROUP(res0, inData + 0);
		COMPUTE_STAGE1_GROUP(res4, inData + 4);
			COMPUTE_STAGE2_GROUP(res1, inData + 1);
		COMPUTE_STAGE1_GROUP(res5, inData + 5);
			COMPUTE_STAGE2_GROUP(res2, inData + 2);
		COMPUTE_STAGE1_GROUP(res6, inData + 6);
			COMPUTE_STAGE2_GROUP(res3, inData + 3);
		COMPUTE_STAGE1_GROUP(res7, inData + 7);

				COMPUTE_STAGE3_GROUP(res0, inData + 0);
			COMPUTE_STAGE2_GROUP(res4, inData + 4);
				COMPUTE_STAGE3_GROUP(res1, inData + 1);
			COMPUTE_STAGE2_GROUP(res5, inData + 5);
				COMPUTE_STAGE3_GROUP(res2, inData + 2);
			COMPUTE_STAGE2_GROUP(res6, inData + 6);
				COMPUTE_STAGE3_GROUP(res3, inData + 3);
			COMPUTE_STAGE2_GROUP(res7, inData + 7);

				COMPUTE_STAGE3_GROUP(res4, inData + 4);
					WRITE_GROUP( outData + 0, res0 );
				COMPUTE_STAGE3_GROUP(res5, inData + 5);
					WRITE_GROUP( outData + 1, res1 );
				COMPUTE_STAGE3_GROUP(res6, inData + 6);
					WRITE_GROUP( outData + 2, res2 );
				COMPUTE_STAGE3_GROUP(res7, inData + 7);
					WRITE_GROUP( outData + 3, res3 );
		

					WRITE_GROUP( outData + 4, res4 );
					WRITE_GROUP( outData + 5, res5 );
					WRITE_GROUP( outData + 6, res6 );
					WRITE_GROUP( outData + 7, res7 );
		
		inData += 8;
		outData += 8;
	}


#undef COMPUTE
#undef WRITE
#undef COMPUTE_STAGE1_ROW
#undef COMPUTE_STAGE2_ROW
#undef COMPUTE_STAGE3_ROW
#undef COMPUTE_STAGE1_GROUP
#undef COMPUTE_STAGE2_GROUP
#undef COMPUTE_STAGE3_GROUP
#undef COMPUTE_GROUP
#undef WRITE_GROUP
}

#ifdef _X360
// Loop-scheduled code to process FourVectors in groups of eight quite efficiently. This is the version
// to call when starting on a 128-byte-aligned address.
void FourVectors_TransformManyGroupsOfEightBy_128byteAligned(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix, FourVectors * RESTRICT pOut )
{
	/* If this has changed, you will need to change all the prefetches, *
	 * and groups of eight are no longer the ideal unit for iterating   *
	 * on many vectors.													*/
	COMPILE_TIME_ASSERT( sizeof(FourVectors) == 48 ) ;

	Assert(numVectors > 0);
	if ( numVectors == 0 )
		return;

	AssertMsg((numVectors & 0x07) == 0, "FourVectors_TransformManyGroupsOfEight called with numVectors % 8 != 0!");

	// Assert alignment
	AssertMsg( ( ( reinterpret_cast<uint32>( pVectors )  & 127 ) == 0) && 
			   ( ( reinterpret_cast<uint32>(pOut) & 127 ) == 0),
			   "FourVectors_Transform..aligned called with non-128-byte-aligned buffers." );

	// Assert non overlap
	AssertMsg( (pOut < pVectors && pOut+numVectors <= pVectors) ||
		(pOut > pVectors && pVectors+numVectors <= pOut), "FourVectors::TransformManyBy called with overlapping buffer pointers." );

		// Here's the plan. 8 four-vecs = 3 cache lines exactly. It takes about 400 cycles to process a group
		// of eight, and cache latency is 600 cycles, so we try to prefetch two iterations ahead (eg fetch
		// iteration 3 while working on iteration 1). In the case of the output, we can simply zero-flush 
		// the cache lines since we are sure to write into them. Because we're reading and fetching two ahead,
		// we want to stop two away from the last iteration.

		// No matter what, we will need to prefetch the first two groups of eight of input (that's the 
		// first six cache lines)
	__dcbt( 0, pVectors );
	__dcbt( 128, pVectors );
	__dcbt( 256, pVectors );
	__dcbt( 384, pVectors );
	__dcbt( 512, pVectors );
	__dcbt( 640, pVectors );


	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02, matSplat03,	// TWELVE REGISTERS
		matSplat10, matSplat11, matSplat12, matSplat13,
		matSplat20, matSplat21, matSplat22, matSplat23;

	{
		// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
		// often unaligned. The w components will be the tranpose row of
		// the matrix.
		fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
		fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
		fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

		matSplat00 = SplatXSIMD(matCol0);
		matSplat01 = SplatYSIMD(matCol0);
		matSplat02 = SplatZSIMD(matCol0);
		matSplat03 = SplatWSIMD(matCol0);

		matSplat10 = SplatXSIMD(matCol1);
		matSplat11 = SplatYSIMD(matCol1);
		matSplat12 = SplatZSIMD(matCol1);
		matSplat13 = SplatWSIMD(matCol1);

		matSplat20 = SplatXSIMD(matCol2);
		matSplat21 = SplatYSIMD(matCol2);
		matSplat22 = SplatZSIMD(matCol2);
		matSplat23 = SplatWSIMD(matCol2);
	}

	// this macro defines how to compute a specific row from an input and certain splat columns
#define COMPUTE(res, invec, xterm, yterm, zterm, transterm) res = AddSIMD( AddSIMD( MulSIMD((invec)->z, zterm), AddSIMD( MulSIMD( (invec)->x, xterm ), MulSIMD( (invec)->y, yterm ) ) ), transterm )
#define WRITE(term, reg, toptr) toptr->term = reg

	// define result groups (we're going to have an eight-way unroll)

	fltx4 res0X, res0Y, res0Z, res0XTemp, res0YTemp, res0ZTemp;	// 48 REGISTERS
	fltx4 res1X, res1Y, res1Z, res1XTemp, res1YTemp, res1ZTemp;
	fltx4 res2X, res2Y, res2Z, res2XTemp, res2YTemp, res2ZTemp;
	fltx4 res3X, res3Y, res3Z, res3XTemp, res3YTemp, res3ZTemp;
	fltx4 res4X, res4Y, res4Z, res4XTemp, res4YTemp, res4ZTemp;
	fltx4 res5X, res5Y, res5Z, res5XTemp, res5YTemp, res5ZTemp;
	fltx4 res6X, res6Y, res6Z, res6XTemp, res6YTemp, res6ZTemp;
	fltx4 res7X, res7Y, res7Z, res7XTemp, res7YTemp, res7ZTemp;


	// #define FROZ(out,in,offset) COMPUTE((out+offset)->x, (in + offset), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE((out + offset )->y, (in + offset), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE((out + offset)->z, (in + offset), matSplat20, matSplat21, matSplat22, matSplat23)
#define COMPUTE_GROUP(resgroup,dataptr) COMPUTE(resgroup ## X, (dataptr), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE(resgroup ## Y, (dataptr), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE(resgroup ## Z, (dataptr), matSplat20, matSplat21, matSplat22, matSplat23)
#define WRITE_GROUP(ptr, resgroup) (ptr)->x = resgroup ## X; (ptr)->y = resgroup ## Y; (ptr)->z = resgroup ## Z

	/*
	// stage 1 -- 6 ops for xyz, each w 12 cycle latency
	res0X = MulSIMD( (invec)->y, matSplat01 );
	res0Temp = MaddSIMD((invec)->z, matSplat02, matSplat03);
	// stage 2 -- 3 clocks for xyz
	res0X = MaddSIMD( (invec)->x, matSplat00, res0X );
	// stage 3 -- 3 clocks for xyz
	res0X = AddSIMD(res0X, res0Temp);
	*/
#define COMPUTE_STAGE1_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MulSIMD( (invec)->y, ysplat ); tempvar = MaddSIMD((invec)->z, zsplat, transplat)
#define COMPUTE_STAGE2_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MaddSIMD( (invec)->x, xsplat, res )
#define COMPUTE_STAGE3_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = AddSIMD(res, tempvar)  // frees up the tempvar

#define COMPUTE_STAGE1_GROUP(resgroup, invec) COMPUTE_STAGE1_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE1_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE1_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE2_GROUP(resgroup, invec) COMPUTE_STAGE2_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE2_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE2_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE3_GROUP(resgroup, invec) COMPUTE_STAGE3_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE3_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE3_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)


		// Okay. First do all but the last two turns of the crank; we don't want to overshoot with the flush-to-zero.
	FourVectors * RESTRICT inData = pVectors;
	FourVectors * RESTRICT outData = pOut;
	const FourVectors * RESTRICT STOP;
	if (numVectors > 16)
	{
		STOP = pVectors + numVectors - 16;
		// flush the first two blocks we'll write into 
		__dcbz128( 0, outData );
		__dcbz128( 128, outData );
		__dcbz128( 256, outData );

		while (inData < STOP)
		{
			// start prefetching the three cache lines
			// we'll hit two iterations from now
			__dcbt( sizeof(FourVectors) * 16,       inData );
			__dcbt( sizeof(FourVectors) * 16 + 128, inData );
			__dcbt( sizeof(FourVectors) * 16 + 256, inData );

			// synchro
			COMPUTE_STAGE1_GROUP(res0, inData + 0);
			COMPUTE_STAGE1_GROUP(res1, inData + 1);
			COMPUTE_STAGE1_GROUP(res2, inData + 2);
			COMPUTE_STAGE1_GROUP(res3, inData + 3);

			// pre-zero the three cache lines we'll overwrite
			// in the next iteration
			__dcbz128( 384, outData );
			__dcbz128( 512, outData );
			__dcbz128( 640, outData );


			COMPUTE_STAGE2_GROUP(res0, inData + 0);
			COMPUTE_STAGE1_GROUP(res4, inData + 4);
			COMPUTE_STAGE2_GROUP(res1, inData + 1);
			COMPUTE_STAGE1_GROUP(res5, inData + 5);
			COMPUTE_STAGE2_GROUP(res2, inData + 2);
			COMPUTE_STAGE1_GROUP(res6, inData + 6);
			COMPUTE_STAGE2_GROUP(res3, inData + 3);
			COMPUTE_STAGE1_GROUP(res7, inData + 7);

			COMPUTE_STAGE3_GROUP(res0, inData + 0);
			COMPUTE_STAGE2_GROUP(res4, inData + 4);
			COMPUTE_STAGE3_GROUP(res1, inData + 1);
			COMPUTE_STAGE2_GROUP(res5, inData + 5);
			COMPUTE_STAGE3_GROUP(res2, inData + 2);
			COMPUTE_STAGE2_GROUP(res6, inData + 6);
			COMPUTE_STAGE3_GROUP(res3, inData + 3);
			COMPUTE_STAGE2_GROUP(res7, inData + 7);

			COMPUTE_STAGE3_GROUP(res4, inData + 4);
			WRITE_GROUP( outData + 0, res0 );
			COMPUTE_STAGE3_GROUP(res5, inData + 5);
			WRITE_GROUP( outData + 1, res1 );
			COMPUTE_STAGE3_GROUP(res6, inData + 6);
			WRITE_GROUP( outData + 2, res2 );
			COMPUTE_STAGE3_GROUP(res7, inData + 7);
			WRITE_GROUP( outData + 3, res3 );


			WRITE_GROUP( outData + 4, res4 );
			WRITE_GROUP( outData + 5, res5 );
			WRITE_GROUP( outData + 6, res6 );
			WRITE_GROUP( outData + 7, res7 );

			inData += 8;
			outData += 8;
		}
	}
	else if (numVectors == 16)
	{
		// zero out the exactly six cache lines we will write into
		__dcbz128( 0, outData );
		__dcbz128( 128, outData );
		__dcbz128( 256, outData );
		__dcbz128( 384, outData );
		__dcbz128( 512, outData );
		__dcbz128( 640, outData );
	}
	else if (numVectors == 8)
	{
		// zero out the exactly three cache lines we will write into
		__dcbz128( 0, outData );
		__dcbz128( 128, outData );
		__dcbz128( 256, outData );
	}
	else
	{
		AssertMsg(false, "Can't happen!");
	}
	
	// deal with the ultimate two groups (or, if we were fed
	// less than 16 groups, the whole shebang)
	STOP = pVectors + numVectors - 16;
	

	// Use techniques of loop scheduling to eliminate data hazards; process
	// eight groups simultaneously so that we never have any operations stalling
	// waiting for data.
	// Note: this loop, while pretty fast, could be faster still -- you'll notice
	// that it does all of its loads, then all computation, then writes everything
	// out. If made truly cyclic, such that every line interleaved a stage 1, stage 2,
	// stage 3, and write, then throughput could be higher (probably by about 50%). 
	while (inData < STOP)
	{
		// synchro
		COMPUTE_STAGE1_GROUP(res0, inData + 0);
		COMPUTE_STAGE1_GROUP(res1, inData + 1);
		COMPUTE_STAGE1_GROUP(res2, inData + 2);
		COMPUTE_STAGE1_GROUP(res3, inData + 3);

		COMPUTE_STAGE2_GROUP(res0, inData + 0);
		COMPUTE_STAGE1_GROUP(res4, inData + 4);
		COMPUTE_STAGE2_GROUP(res1, inData + 1);
		COMPUTE_STAGE1_GROUP(res5, inData + 5);
		COMPUTE_STAGE2_GROUP(res2, inData + 2);
		COMPUTE_STAGE1_GROUP(res6, inData + 6);
		COMPUTE_STAGE2_GROUP(res3, inData + 3);
		COMPUTE_STAGE1_GROUP(res7, inData + 7);

		COMPUTE_STAGE3_GROUP(res0, inData + 0);
		COMPUTE_STAGE2_GROUP(res4, inData + 4);
		COMPUTE_STAGE3_GROUP(res1, inData + 1);
		COMPUTE_STAGE2_GROUP(res5, inData + 5);
		COMPUTE_STAGE3_GROUP(res2, inData + 2);
		COMPUTE_STAGE2_GROUP(res6, inData + 6);
		COMPUTE_STAGE3_GROUP(res3, inData + 3);
		COMPUTE_STAGE2_GROUP(res7, inData + 7);

		COMPUTE_STAGE3_GROUP(res4, inData + 4);
		WRITE_GROUP( outData + 0, res0 );
		COMPUTE_STAGE3_GROUP(res5, inData + 5);
		WRITE_GROUP( outData + 1, res1 );
		COMPUTE_STAGE3_GROUP(res6, inData + 6);
		WRITE_GROUP( outData + 2, res2 );
		COMPUTE_STAGE3_GROUP(res7, inData + 7);
		WRITE_GROUP( outData + 3, res3 );


		WRITE_GROUP( outData + 4, res4 );
		WRITE_GROUP( outData + 5, res5 );
		WRITE_GROUP( outData + 6, res6 );
		WRITE_GROUP( outData + 7, res7 );

		inData += 8;
		outData += 8;
	}


#undef COMPUTE
#undef WRITE
#undef COMPUTE_STAGE1_ROW
#undef COMPUTE_STAGE2_ROW
#undef COMPUTE_STAGE3_ROW
#undef COMPUTE_STAGE1_GROUP
#undef COMPUTE_STAGE2_GROUP
#undef COMPUTE_STAGE3_GROUP
#undef COMPUTE_GROUP
#undef WRITE_GROUP
}
#endif

// Transform a long array of FourVectors by a given matrix. 
void FourVectors::TransformManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix, FourVectors * RESTRICT pOut )
{
	Assert(numVectors > 0);

	AssertMsg( (pOut < pVectors && pOut+numVectors <= pVectors) ||
		(pOut > pVectors && pVectors+numVectors <= pOut), "FourVectors::TransformManyBy called with overlapping buffer pointers." );

#ifdef _X360
	// The really fast version of this function likes to operate on blocks of eight. So, chug through
	// groups of eight, then deal with any leftovers.
	int numVectorsRoundedToNearestEight = numVectors & (~0x07);
	if (numVectors >= 8)
	{
		// aligned?
		if ((reinterpret_cast<unsigned int>(pVectors) & 127) == 0 && (reinterpret_cast<unsigned int>(pOut) & 127) == 0)
		{
			FourVectors_TransformManyGroupsOfEightBy_128byteAligned(pVectors, numVectorsRoundedToNearestEight, rotationMatrix, pOut);
		}
		else
		{
			FourVectors_TransformManyGroupsOfEightBy(pVectors, numVectorsRoundedToNearestEight, rotationMatrix, pOut);
		}
		numVectors -= numVectorsRoundedToNearestEight;
		pVectors += numVectorsRoundedToNearestEight;
		pOut += numVectorsRoundedToNearestEight;
	}
#endif

	// any left over?
	if (numVectors > 0)
	{

		// Splat out each of the entries in the matrix to a fltx4. Do this
		// in the order that we will need them, to hide latency. I'm
		// avoiding making an array of them, so that they'll remain in 
		// registers.
		fltx4 matSplat00, matSplat01, matSplat02, matSplat03,	// TWELVE REGISTERS
			matSplat10, matSplat11, matSplat12, matSplat13,
			matSplat20, matSplat21, matSplat22, matSplat23;

		{
			// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
			// often unaligned. The w components will be the transpose row of
			// the matrix.
			fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
			fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
			fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

			matSplat00 = SplatXSIMD(matCol0);
			matSplat01 = SplatYSIMD(matCol0);
			matSplat02 = SplatZSIMD(matCol0);
			matSplat03 = SplatWSIMD(matCol0);

			matSplat10 = SplatXSIMD(matCol1);
			matSplat11 = SplatYSIMD(matCol1);
			matSplat12 = SplatZSIMD(matCol1);
			matSplat13 = SplatWSIMD(matCol1);

			matSplat20 = SplatXSIMD(matCol2);
			matSplat21 = SplatYSIMD(matCol2);
			matSplat22 = SplatZSIMD(matCol2);
			matSplat23 = SplatWSIMD(matCol2);
		}

		do 
		{
			// Trust in the compiler to schedule these operations correctly:
			pOut->x = MaddSIMD(pVectors->z, matSplat02, MaddSIMD(pVectors->y, matSplat01, MaddSIMD(pVectors->x, matSplat00, matSplat03)));
			pOut->y = MaddSIMD(pVectors->z, matSplat12, MaddSIMD(pVectors->y, matSplat11, MaddSIMD(pVectors->x, matSplat00, matSplat13)));
			pOut->z = MaddSIMD(pVectors->z, matSplat22, MaddSIMD(pVectors->y, matSplat21, MaddSIMD(pVectors->x, matSplat00, matSplat23)));

			++pOut;
			++pVectors;
			--numVectors;
		} while(numVectors > 0);
	}
}

#ifdef _X360
// Loop-scheduled code to process FourVectors in groups of eight quite efficiently.
static void FourVectors_TransformManyGroupsOfEightBy_InPlace(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix )
{
	Assert(numVectors > 0);
	if ( numVectors == 0 )
		return;

	// Prefetch line 1 and 2
	__dcbt(0,pVectors);
	__dcbt(128,pVectors);

	// Splat out each of the entries in the matrix to a fltx4. Do this
	// in the order that we will need them, to hide latency. I'm
	// avoiding making an array of them, so that they'll remain in 
	// registers.
	fltx4 matSplat00, matSplat01, matSplat02, matSplat03,	// TWELVE REGISTERS
		matSplat10, matSplat11, matSplat12, matSplat13,
		matSplat20, matSplat21, matSplat22, matSplat23;

	{
		// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
		// often unaligned. The w components will be the tranpose row of
		// the matrix.
		fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
		fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
		fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

		matSplat00 = SplatXSIMD(matCol0);
		matSplat01 = SplatYSIMD(matCol0);
		matSplat02 = SplatZSIMD(matCol0);
		matSplat03 = SplatWSIMD(matCol0);

		matSplat10 = SplatXSIMD(matCol1);
		matSplat11 = SplatYSIMD(matCol1);
		matSplat12 = SplatZSIMD(matCol1);
		matSplat13 = SplatWSIMD(matCol1);

		matSplat20 = SplatXSIMD(matCol2);
		matSplat21 = SplatYSIMD(matCol2);
		matSplat22 = SplatZSIMD(matCol2);
		matSplat23 = SplatWSIMD(matCol2);
	}

	// this macro defines how to compute a specific row from an input and certain splat columns
#define COMPUTE(res, invec, xterm, yterm, zterm, transterm) res = AddSIMD( AddSIMD( MulSIMD((invec)->z, zterm), AddSIMD( MulSIMD( (invec)->x, xterm ), MulSIMD( (invec)->y, yterm ) ) ), transterm )
#define WRITE(term, reg, toptr) toptr->term = reg

	// define result groups (we're going to have an eight-way unroll)

	fltx4 res0X, res0Y, res0Z, res0XTemp, res0YTemp, res0ZTemp;	// 48 REGISTERS
	fltx4 res1X, res1Y, res1Z, res1XTemp, res1YTemp, res1ZTemp;
	fltx4 res2X, res2Y, res2Z, res2XTemp, res2YTemp, res2ZTemp;
	fltx4 res3X, res3Y, res3Z, res3XTemp, res3YTemp, res3ZTemp;
	fltx4 res4X, res4Y, res4Z, res4XTemp, res4YTemp, res4ZTemp;
	fltx4 res5X, res5Y, res5Z, res5XTemp, res5YTemp, res5ZTemp;
	fltx4 res6X, res6Y, res6Z, res6XTemp, res6YTemp, res6ZTemp;
	fltx4 res7X, res7Y, res7Z, res7XTemp, res7YTemp, res7ZTemp;


	// #define FROZ(out,in,offset) COMPUTE((out+offset)->x, (in + offset), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE((out + offset )->y, (in + offset), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE((out + offset)->z, (in + offset), matSplat20, matSplat21, matSplat22, matSplat23)
#define COMPUTE_GROUP(resgroup,dataptr) COMPUTE(resgroup ## X, (dataptr), matSplat00, matSplat01, matSplat02, matSplat03); COMPUTE(resgroup ## Y, (dataptr), matSplat10, matSplat11, matSplat12, matSplat13); COMPUTE(resgroup ## Z, (dataptr), matSplat20, matSplat21, matSplat22, matSplat23)
#define WRITE_GROUP(ptr, resgroup) (ptr)->x = resgroup ## X; (ptr)->y = resgroup ## Y; (ptr)->z = resgroup ## Z

	/*
	// stage 1 -- 6 ops for xyz, each w 12 cycle latency
	res0X = MulSIMD( (invec)->y, matSplat01 );
	res0Temp = MaddSIMD((invec)->z, matSplat02, matSplat03);
	// stage 2 -- 3 clocks for xyz
	res0X = MaddSIMD( (invec)->x, matSplat00, res0X );
	// stage 3 -- 3 clocks for xyz
	res0X = AddSIMD(res0X, res0Temp);
	*/
#define COMPUTE_STAGE1_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MulSIMD( (invec)->y, ysplat ); tempvar = MaddSIMD((invec)->z, zsplat, transplat)
#define COMPUTE_STAGE2_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = MaddSIMD( (invec)->x, xsplat, res )
#define COMPUTE_STAGE3_ROW(res, tempvar, invec, xsplat, ysplat, zsplat, transplat) res = AddSIMD(res, tempvar)  // frees up the tempvar

#define COMPUTE_STAGE1_GROUP(resgroup, invec) COMPUTE_STAGE1_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE1_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE1_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE2_GROUP(resgroup, invec) COMPUTE_STAGE2_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE2_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE2_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

#define COMPUTE_STAGE3_GROUP(resgroup, invec) COMPUTE_STAGE3_ROW(resgroup ## X, resgroup ## X ## Temp, invec, matSplat00, matSplat01, matSplat02, matSplat03);\
	COMPUTE_STAGE3_ROW(resgroup ## Y, resgroup ## Y ## Temp, invec, matSplat10, matSplat11, matSplat12, matSplat13);\
	COMPUTE_STAGE3_ROW(resgroup ## Z, resgroup ## Z ## Temp, invec, matSplat20, matSplat21, matSplat22, matSplat23)

	const FourVectors * const RESTRICT STOP = pVectors + numVectors;

	// Use techniques of loop scheduling to eliminate data hazards; process
	// eight groups simultaneously so that we never have any operations stalling
	// waiting for data.
	// Note: this loop, while pretty fast, could be faster still -- you'll notice
	// that it does all of its loads, then all computation, then writes everything
	// out. If made truly cyclic, such that every line interleaved a stage 1, stage 2,
	// stage 3, and write, then throughput could be higher (probably by about 50%). 
	while (pVectors < STOP)
	{
		// start prefetching the three cache lines
		// we'll hit two iterations from now
		__dcbt( sizeof(FourVectors) * 16,       pVectors );
		__dcbt( sizeof(FourVectors) * 16 + 128, pVectors );
		__dcbt( sizeof(FourVectors) * 16 + 256, pVectors );

		// synchro
		COMPUTE_STAGE1_GROUP(res0, pVectors + 0);
		COMPUTE_STAGE1_GROUP(res1, pVectors + 1);
		COMPUTE_STAGE1_GROUP(res2, pVectors + 2);
		COMPUTE_STAGE1_GROUP(res3, pVectors + 3);

		COMPUTE_STAGE2_GROUP(res0, pVectors + 0);
		COMPUTE_STAGE1_GROUP(res4, pVectors + 4);
		COMPUTE_STAGE2_GROUP(res1, pVectors + 1);
		COMPUTE_STAGE1_GROUP(res5, pVectors + 5);
		COMPUTE_STAGE2_GROUP(res2, pVectors + 2);
		COMPUTE_STAGE1_GROUP(res6, pVectors + 6);
		COMPUTE_STAGE2_GROUP(res3, pVectors + 3);
		COMPUTE_STAGE1_GROUP(res7, pVectors + 7);

		COMPUTE_STAGE3_GROUP(res0, pVectors + 0);
		COMPUTE_STAGE2_GROUP(res4, pVectors + 4);
		COMPUTE_STAGE3_GROUP(res1, pVectors + 1);
		COMPUTE_STAGE2_GROUP(res5, pVectors + 5);
		COMPUTE_STAGE3_GROUP(res2, pVectors + 2);
		COMPUTE_STAGE2_GROUP(res6, pVectors + 6);
		COMPUTE_STAGE3_GROUP(res3, pVectors + 3);
		COMPUTE_STAGE2_GROUP(res7, pVectors + 7);

		COMPUTE_STAGE3_GROUP(res4, pVectors + 4);
		WRITE_GROUP( pVectors + 0, res0 );
		COMPUTE_STAGE3_GROUP(res5, pVectors + 5);
		WRITE_GROUP( pVectors + 1, res1 );
		COMPUTE_STAGE3_GROUP(res6, pVectors + 6);
		WRITE_GROUP( pVectors + 2, res2 );
		COMPUTE_STAGE3_GROUP(res7, pVectors + 7);
		WRITE_GROUP( pVectors + 3, res3 );

		WRITE_GROUP( pVectors + 4, res4 );
		WRITE_GROUP( pVectors + 5, res5 );
		WRITE_GROUP( pVectors + 6, res6 );
		WRITE_GROUP( pVectors + 7, res7 );

		pVectors += 8;
	}


#undef COMPUTE
#undef WRITE
#undef COMPUTE_STAGE1_ROW
#undef COMPUTE_STAGE2_ROW
#undef COMPUTE_STAGE3_ROW
#undef COMPUTE_STAGE1_GROUP
#undef COMPUTE_STAGE2_GROUP
#undef COMPUTE_STAGE3_GROUP
#undef COMPUTE_GROUP
#undef WRITE_GROUP
}
#endif

// In-place version of above. It's necessary to have this, rather than just allowing pOut and pVectors
// to equal each other, because of the semantics of RESTRICT: pVectors and pOut must not be allowed 
// to alias. (Simply un-restricting the pointers results in very poor scheduling.)
void FourVectors::TransformManyBy(FourVectors * RESTRICT pVectors, unsigned int numVectors, const matrix3x4_t& rotationMatrix )
{
	Assert(numVectors > 0);

#ifdef _X360
	// The really fast version of this function likes to operate on blocks of eight. So, chug through
	// groups of eight, then deal with any leftovers.
	int numVectorsRoundedToNearestEight = numVectors & (~0x07);
	if (numVectors >= 8)
	{
		FourVectors_TransformManyGroupsOfEightBy_InPlace(pVectors, numVectorsRoundedToNearestEight, rotationMatrix);
		numVectors -= numVectorsRoundedToNearestEight;
		pVectors += numVectorsRoundedToNearestEight;
	}
#endif

	// any left over?
	if (numVectors > 0)
	{

		// Splat out each of the entries in the matrix to a fltx4. Do this
		// in the order that we will need them, to hide latency. I'm
		// avoiding making an array of them, so that they'll remain in 
		// registers.
		fltx4 matSplat00, matSplat01, matSplat02, matSplat03,	// TWELVE REGISTERS
			matSplat10, matSplat11, matSplat12, matSplat13,
			matSplat20, matSplat21, matSplat22, matSplat23;

		{
			// Load the matrix into local vectors. Sadly, matrix3x4_ts are 
			// often unaligned. The w components will be the transpose row of
			// the matrix.
			fltx4 matCol0 = LoadUnalignedSIMD(rotationMatrix[0]);
			fltx4 matCol1 = LoadUnalignedSIMD(rotationMatrix[1]);
			fltx4 matCol2 = LoadUnalignedSIMD(rotationMatrix[2]);

			matSplat00 = SplatXSIMD(matCol0);
			matSplat01 = SplatYSIMD(matCol0);
			matSplat02 = SplatZSIMD(matCol0);
			matSplat03 = SplatWSIMD(matCol0);

			matSplat10 = SplatXSIMD(matCol1);
			matSplat11 = SplatYSIMD(matCol1);
			matSplat12 = SplatZSIMD(matCol1);
			matSplat13 = SplatWSIMD(matCol1);

			matSplat20 = SplatXSIMD(matCol2);
			matSplat21 = SplatYSIMD(matCol2);
			matSplat22 = SplatZSIMD(matCol2);
			matSplat23 = SplatWSIMD(matCol2);
		}

		do 
		{
			fltx4 resultX, resultY, resultZ;
			// Trust in the compiler to schedule these operations correctly:
			resultX = MaddSIMD(pVectors->z, matSplat02, MaddSIMD(pVectors->y, matSplat01, MaddSIMD(pVectors->x, matSplat00, matSplat03)));
			resultY = MaddSIMD(pVectors->z, matSplat12, MaddSIMD(pVectors->y, matSplat11, MaddSIMD(pVectors->x, matSplat00, matSplat13)));
			resultZ = MaddSIMD(pVectors->z, matSplat22, MaddSIMD(pVectors->y, matSplat21, MaddSIMD(pVectors->x, matSplat00, matSplat23)));

			pVectors->x = resultX;
			pVectors->y = resultY;
			pVectors->z = resultZ;

			++pVectors;
			--numVectors;
		} while(numVectors > 0);
	}
}


#endif

// Transform many (horizontal) points in-place by a 3x4 matrix,
// here already loaded onto three fltx4 registers but not transposed. 
// The points must be stored as 16-byte aligned. They are points
// and not vectors because we assume the w-component to be 1. 
#ifdef _X360
void TransformManyPointsBy(VectorAligned * RESTRICT pVectors, unsigned int numVectors, FLTX4 mRow0, FLTX4 mRow1, FLTX4 mRow2)
{
	/**************************************************
	 *  Here is an elaborate and carefully scheduled  *
	 *  algorithm nicked from xboxmath.inl and hacked *
	 *  up for 3x4 matrices.                          *
	 **************************************************/

	COMPILE_TIME_ASSERT(sizeof(VectorAligned) == sizeof(XMFLOAT4)); // VectorAligned's need to be 16 bytes

	XMVECTOR R0[8], R1[8], R2[8];
	XMVECTOR vIn[8];

	//    C_ASSERT(UnrollCount == 8);
	//    C_ASSERT(sizeof(XMFLOAT4) == 16);
	Assert(pVectors);
	Assert(((UINT_PTR)pVectors & 3) == 0); // assert alignment 

	UINT GroupIndex;

	VectorAligned * RESTRICT vCurrent = pVectors;
	// sentinel pointers
	VectorAligned * vStreamEnd, *vStreamGroupBase, *vStreamGroupEnd;

	{
		// cook up the pointers from integer math. Necessary because otherwise we LHS all over
		// the place. (Odd that this doesn't happen to the xbox math.) 

		UINT_PTR InputVector = (UINT_PTR)pVectors;
		UINT_PTR InputStreamEnd = InputVector + numVectors * sizeof(XMFLOAT4);
		// compute start and end points on 128-byte alignment
		UINT_PTR InputStreamCGroupBase = XMMin(InputVector + (XM_CACHE_LINE_SIZE - 1), InputStreamEnd) & ~(XM_CACHE_LINE_SIZE - 1);
		UINT_PTR InputStreamCGroupEnd = InputStreamCGroupBase + ((InputStreamEnd - InputStreamCGroupBase) & ~(4 * XM_CACHE_LINE_SIZE - 1));

		vStreamEnd = (VectorAligned *)InputStreamEnd;
		vStreamGroupBase = (VectorAligned *)InputStreamCGroupBase;
		vStreamGroupEnd = (VectorAligned *)InputStreamCGroupEnd;
	}


	__dcbt(0,                      vStreamGroupBase);
	__dcbt(XM_CACHE_LINE_SIZE,     vStreamGroupBase);
	__dcbt(XM_CACHE_LINE_SIZE * 2, vStreamGroupBase);
	__dcbt(XM_CACHE_LINE_SIZE * 3, vStreamGroupBase);

	while (vCurrent < vStreamGroupBase)
	{
		fltx4 vec = __lvx(vCurrent->Base(), 0);

		R0[0] = __vmsum4fp(vec, mRow0);
		R1[0] = __vmsum4fp(vec, mRow1);
		R2[0] = __vmsum4fp(vec, mRow2);

		__stvewx(R0[0], vCurrent->Base(), 0);
		__stvewx(R1[0], vCurrent->Base(), 4);
		__stvewx(R2[0], vCurrent->Base(), 8);

		vCurrent++; 
	}

	while (vCurrent < vStreamGroupEnd)
	{
		__dcbt(XM_CACHE_LINE_SIZE * 4, vCurrent);
		__dcbt(XM_CACHE_LINE_SIZE * 5, vCurrent);
		__dcbt(XM_CACHE_LINE_SIZE * 6, vCurrent);
		__dcbt(XM_CACHE_LINE_SIZE * 7, vCurrent);

		for (GroupIndex = 0; GroupIndex < 4; GroupIndex++)
		{
			// all kinds of LHS on this pointer. Why?
			VectorAligned* OutputVector = vCurrent;

			vIn[0] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[1] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[2] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[3] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[4] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[5] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[6] = __lvx(vCurrent->Base(), 0);
			vCurrent++;
			vIn[7] = __lvx(vCurrent->Base(), 0);
			vCurrent++;

			R0[0] = __vmsum4fp(vIn[0], mRow0);
			R1[0] = __vmsum4fp(vIn[0], mRow1);
			R2[0] = __vmsum4fp(vIn[0], mRow2);

			R0[1] = __vmsum4fp(vIn[1], mRow0);
			R1[1] = __vmsum4fp(vIn[1], mRow1);
			R2[1] = __vmsum4fp(vIn[1], mRow2);

			R0[2] = __vmsum4fp(vIn[2], mRow0);
			R1[2] = __vmsum4fp(vIn[2], mRow1);
			R2[2] = __vmsum4fp(vIn[2], mRow2);

			R0[3] = __vmsum4fp(vIn[3], mRow0);
			R1[3] = __vmsum4fp(vIn[3], mRow1);
			R2[3] = __vmsum4fp(vIn[3], mRow2);

			R0[4] = __vmsum4fp(vIn[4], mRow0);
			R1[4] = __vmsum4fp(vIn[4], mRow1);
			R2[4] = __vmsum4fp(vIn[4], mRow2);

			R0[5] = __vmsum4fp(vIn[5], mRow0);
			R1[5] = __vmsum4fp(vIn[5], mRow1);
			R2[5] = __vmsum4fp(vIn[5], mRow2);

			R0[6] = __vmsum4fp(vIn[6], mRow0);
			R1[6] = __vmsum4fp(vIn[6], mRow1);
			R2[6] = __vmsum4fp(vIn[6], mRow2);

			R0[7] = __vmsum4fp(vIn[7], mRow0);
			R1[7] = __vmsum4fp(vIn[7], mRow1);
			R2[7] = __vmsum4fp(vIn[7], mRow2);

			__stvewx(R0[0], OutputVector, 0);
			__stvewx(R1[0], OutputVector, 4);
			__stvewx(R2[0], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[1], OutputVector, 0);
			__stvewx(R1[1], OutputVector, 4);
			__stvewx(R2[1], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[2], OutputVector, 0);
			__stvewx(R1[2], OutputVector, 4);
			__stvewx(R2[2], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[3], OutputVector, 0);
			__stvewx(R1[3], OutputVector, 4);
			__stvewx(R2[3], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[4], OutputVector, 0);
			__stvewx(R1[4], OutputVector, 4);
			__stvewx(R2[4], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[5], OutputVector, 0);
			__stvewx(R1[5], OutputVector, 4);
			__stvewx(R2[5], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[6], OutputVector, 0);
			__stvewx(R1[6], OutputVector, 4);
			__stvewx(R2[6], OutputVector, 8);
			OutputVector++;

			__stvewx(R0[7], OutputVector, 0);
			__stvewx(R1[7], OutputVector, 4);
			__stvewx(R2[7], OutputVector, 8);
			OutputVector++;
		}
	}

	while (vCurrent < vStreamEnd)
	{
		vIn[0] = __lvx(vCurrent->Base(), 0);

		R0[0] = __vmsum4fp(vIn[0], mRow0);
		R1[0] = __vmsum4fp(vIn[0], mRow1);
		R2[0] = __vmsum4fp(vIn[0], mRow2);

		__stvewx(R0[0], vCurrent->Base(), 0);
		__stvewx(R1[0], vCurrent->Base(), 4);
		__stvewx(R2[0], vCurrent->Base(), 8);

		vCurrent++;
	}
	

}
#endif
