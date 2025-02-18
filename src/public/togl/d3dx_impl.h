#ifndef D3DX_IMPL_H
#define D3DX_IMPL_H

#include "togl/dx_iunknown.h"

#include "tier0/dynfunction.h"
#include "tier0/vprof_telemetry.h"
#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier0/vprof.h"
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlvector.h"
#include "mathlib/vmatrix.h"

// ------------------------------------------------------------------------------------------------------------------------------ //
// D3DX
// ------------------------------------------------------------------------------------------------------------------------------ //

class D3DXMATRIX;

D3DXMATRIX* D3DXMatrixMultiply( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );

class D3DXMATRIX : public D3DMATRIX
{
public:
	D3DXMATRIX operator*( const D3DXMATRIX &o ) const
	{
		D3DXMATRIX result;
	
		D3DXMatrixMultiply( &result, this, &o );	// this = lhs    o = rhs    result = this * o

		return result;
	}

	operator FLOAT* ()
	{
		return (float*)this;
	}

	float& operator()( int row, int column )
	{
		return m[row][column];
	}

	const float& operator()( int row, int column ) const
	{
		return m[row][column];
	}

	bool operator != ( CONST D3DXMATRIX& src ) const 
	{
 		return V_memcmp( (void*)this, (void*)&src, sizeof(this) ) != 0;
	}
};

typedef struct D3DXPLANE
{
	float& operator[]( int i )
	{
		return ((float*)this)[i];
	}

	bool operator==( const D3DXPLANE &o )
	{
		return a == o.a && b == o.b && c == o.c && d == o.d;
	}

	bool operator!=( const D3DXPLANE &o )
	{
		return !( *this == o );
	}

	operator float*()
	{
		return (float*)this;
	}

	operator const float*() const
	{
		return (const float*)this;
	}

	float a, b, c, d;
} D3DXPLANE;

class D3DXVECTOR2
{
public:
	operator FLOAT* ()
	{
		return (float*)this;
	}

	operator CONST FLOAT* () const
	{
		return (const float*)this;
	}

	float x, y;
};

class D3DXVECTOR3 : public D3DVECTOR
{
public:
	D3DXVECTOR3() {}
	D3DXVECTOR3( float a, float b, float c )
	{
		x = a;
		y = b;
		z = c;
	}

	operator FLOAT* ()
	{
		return (float*)this;
	}

	operator CONST FLOAT* () const
	{
		return (const float*)this;
	}
};

typedef enum _D3DXINCLUDE_TYPE
{
	D3DXINC_LOCAL,

	// force 32-bit size enum
	D3DXINC_FORCE_DWORD = 0x7fffffff

} D3DXINCLUDE_TYPE;

class D3DXVECTOR4
{
public:
	D3DXVECTOR4() {}
	D3DXVECTOR4( float a, float b, float c, float d )
	{
		x = a;
		y = b;
		z = c;
		w = d;
	}

	float x,y,z,w;
};

//----------------------------------------------------------------------------
// D3DXMACRO:
// ----------
// Preprocessor macro definition.  The application pass in a NULL-terminated
// array of this structure to various D3DX APIs.  This enables the application
// to #define tokens at runtime, before the file is parsed.
//----------------------------------------------------------------------------

typedef struct _D3DXMACRO
{
    LPCSTR Name;
    LPCSTR Definition;

} D3DXMACRO, *LPD3DXMACRO;

#define D3DXINLINE inline

#define D3DXSHADER_DEBUG                    (1 << 0)
#define D3DXSHADER_AVOID_FLOW_CONTROL       (1 << 9)

typedef class CUtlMemory<D3DMATRIX> CD3DMATRIXAllocator;
typedef class CUtlVector<D3DMATRIX, CD3DMATRIXAllocator> CD3DMATRIXStack;

inline void D3DXMatrixIdentity(D3DXMATRIX* mat)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			mat->m[i][j] = (i == j) ? 1.0f : 0.0f;	// 1's on the diagonal.
		}
	}
}

struct ID3DXMatrixStack //: public CUnknown
{
	int	m_refcount[2];
	CD3DMATRIXStack	m_stack;
	int						m_stackTop;	// top of stack is at the highest index, this is that index.  push increases, pop decreases.

	ID3DXMatrixStack( void )
	{
		m_refcount[0] = 1;
		m_refcount[1] = 0;
	};
		
	inline void AddRef( int which = 0, char *comment = NULL )
	{
		m_refcount[which]++;
	};
		
	inline ULONG Release( int which = 0 , char *comment = NULL )
	{
		bool deleting = false;
			
		m_refcount[which]--;
		if ( (!m_refcount[0]) && (!m_refcount[1]) )
		{
			deleting = true;
		}
			
		if (deleting)
		{
			delete this;
			return 0;
		}
		else
		{
			return m_refcount[0];
		}
	};


	inline HRESULT Create()
	{
		m_stack.EnsureCapacity( 16 );	// 1KB ish
		m_stack.AddToTail();
		m_stackTop = 0;				// top of stack is at index 0 currently
	
		LoadIdentity();
	
		return S_OK;
	}

	inline D3DXMATRIX* GetTop()
	{
		return (D3DXMATRIX*)&m_stack[ m_stackTop ];
	}

	inline void Push()
	{
		D3DMATRIX temp = m_stack[ m_stackTop ];
		m_stack.AddToTail( temp );
		m_stackTop ++;
	}

	inline void Pop()
	{
		int elem = m_stackTop--;
		m_stack.Remove( elem );
	}

	inline void LoadIdentity()
	{
		D3DXMATRIX *mat = GetTop();

		D3DXMatrixIdentity( mat );
	}

	inline void LoadMatrix( const D3DXMATRIX *pMat )
	{
		*(GetTop()) = *pMat;
	}


	inline void MultMatrix( const D3DXMATRIX *pMat )
	{

		// http://msdn.microsoft.com/en-us/library/bb174057(VS.85).aspx
		//	This method right-multiplies the given matrix to the current matrix
		//	(transformation is about the current world origin).
		//		m_pstack[m_currentPos] = m_pstack[m_currentPos] * (*pMat);
		//	This method does not add an item to the stack, it replaces the current
		//  matrix with the product of the current matrix and the given matrix.


		DXABSTRACT_BREAK_ON_ERROR();
	}

	inline void MultMatrixLocal( const D3DXMATRIX *pMat )
	{
		//	http://msdn.microsoft.com/en-us/library/bb174058(VS.85).aspx
		//	This method left-multiplies the given matrix to the current matrix
		//	(transformation is about the local origin of the object).
		//		m_pstack[m_currentPos] = (*pMat) * m_pstack[m_currentPos];
		//	This method does not add an item to the stack, it replaces the current
		//	matrix with the product of the given matrix and the current matrix.


		DXABSTRACT_BREAK_ON_ERROR();
	}

	inline HRESULT ScaleLocal(FLOAT x, FLOAT y, FLOAT z)
	{
		//	http://msdn.microsoft.com/en-us/library/bb174066(VS.85).aspx
		//	Scale the current matrix about the object origin.
		//	This method left-multiplies the current matrix with the computed
		//	scale matrix. The transformation is about the local origin of the object.
		//
		//	D3DXMATRIX tmp;
		//	D3DXMatrixScaling(&tmp, x, y, z);
		//	m_stack[m_currentPos] = tmp * m_stack[m_currentPos];

		DXABSTRACT_BREAK_ON_ERROR();
		return S_OK;
	}


	inline HRESULT RotateAxisLocal(CONST D3DXVECTOR3* pV, FLOAT Angle)
	{
		//	http://msdn.microsoft.com/en-us/library/bb174062(VS.85).aspx
		//	Left multiply the current matrix with the computed rotation
		//	matrix, counterclockwise about the given axis with the given angle.
		//	(rotation is about the local origin of the object)

		//	D3DXMATRIX tmp;
		//	D3DXMatrixRotationAxis( &tmp, pV, angle );
		//	m_stack[m_currentPos] = tmp * m_stack[m_currentPos];
		//	Because the rotation is left-multiplied to the matrix stack, the rotation
		//	is relative to the object's local coordinate space.
	
		DXABSTRACT_BREAK_ON_ERROR();
		return S_OK;
	}

	inline HRESULT TranslateLocal(FLOAT x, FLOAT y, FLOAT z)
	{
		//	http://msdn.microsoft.com/en-us/library/bb174068(VS.85).aspx
		//	Left multiply the current matrix with the computed translation
		//	matrix. (transformation is about the local origin of the object)

		//	D3DXMATRIX tmp;
		//	D3DXMatrixTranslation( &tmp, x, y, z );
		//	m_stack[m_currentPos] = tmp * m_stack[m_currentPos];

		DXABSTRACT_BREAK_ON_ERROR();
		return S_OK;
	}
};

typedef ID3DXMatrixStack* LPD3DXMATRIXSTACK;

#if _WIN32
DECLARE_INTERFACE( ID3DXInclude )
{
	STDMETHOD( Open )( THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes ) PURE;
	STDMETHOD( Close )( THIS_ LPCVOID pData ) PURE;
};
typedef ID3DXInclude* LPD3DXINCLUDE;


DECLARE_INTERFACE_( ID3DXBuffer, IUnknown )
{
	// IUnknown
	STDMETHOD( QueryInterface )( THIS_ REFIID iid, LPVOID * ppv ) PURE;
	STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
	STDMETHOD_( ULONG, Release )( THIS ) PURE;

	// ID3DXBuffer
	STDMETHOD_( LPVOID, GetBufferPointer )( THIS ) PURE;
	STDMETHOD_( DWORD, GetBufferSize )( THIS ) PURE;
};
typedef ID3DXBuffer* LPD3DXBUFFER;
#else
struct ID3DXInclude
{
	virtual HRESULT Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
	virtual HRESULT Close(LPCVOID pData);
};
typedef ID3DXInclude* LPD3DXINCLUDE;


struct ID3DXBuffer : public CUnknown
{
	inline void* GetBufferPointer()
	{
		DXABSTRACT_BREAK_ON_ERROR();
		return NULL;
	}

	inline DWORD GetBufferSize()
	{
		DXABSTRACT_BREAK_ON_ERROR();
		return 0;
	}
};

typedef ID3DXBuffer* LPD3DXBUFFER;
#endif

class ID3DXConstantTable : public CUnknown
{
};
typedef ID3DXConstantTable* LPD3DXCONSTANTTABLE;

inline const char* D3DXGetPixelShaderProfile(IDirect3DDevice9* pDevice)
{
	DXABSTRACT_BREAK_ON_ERROR();
	return "";
}

inline D3DXMATRIX* D3DXMatrixMultiply( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 )
{
	D3DXMATRIX temp;
	
	for( int i=0; i<4; i++)
	{
		for( int j=0; j<4; j++)
		{
			temp.m[i][j]	=	(pM1->m[ i ][ 0 ] * pM2->m[ 0 ][ j ])
							+	(pM1->m[ i ][ 1 ] * pM2->m[ 1 ][ j ])
							+	(pM1->m[ i ][ 2 ] * pM2->m[ 2 ][ j ])
							+	(pM1->m[ i ][ 3 ] * pM2->m[ 3 ][ j ]);
		}
	}
	*pOut = temp;
	return pOut;
}

// Transform a 3D vector by a given matrix, projecting the result back into w = 1
// http://msdn.microsoft.com/en-us/library/ee417622(VS.85).aspx
inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM)
{
	D3DXVECTOR3 vOut;

	vOut.x = vOut.y = vOut.z = 0.0f;
	float norm = (pM->m[0][3] * pV->x) + (pM->m[1][3] * pV->y) + (pM->m[2][3] *pV->z) + pM->m[3][3];
	if ( norm )
	{
		vOut.x = (pM->m[0][0] * pV->x + pM->m[1][0] * pV->y + pM->m[2][0] * pV->z + pM->m[3][0]) / norm;
		vOut.y = (pM->m[0][1] * pV->x + pM->m[1][1] * pV->y + pM->m[2][1] * pV->z + pM->m[3][1]) / norm;
		vOut.z = (pM->m[0][2] * pV->x + pM->m[1][2] * pV->y + pM->m[2][2] * pV->z + pM->m[3][2]) / norm;
	}

	*pOut = vOut;

	return pOut;
}

inline HRESULT D3DXCreateMatrixStack( DWORD Flags, LPD3DXMATRIXSTACK* ppStack)
{
	
	*ppStack = new ID3DXMatrixStack;
	
	(*ppStack)->Create();
	
	return S_OK;
}

inline D3DXVECTOR3* D3DXVec3Subtract( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	pOut->x = pV1->x - pV2->x;
	pOut->y = pV1->y - pV2->y;
	pOut->z = pV1->z - pV2->z;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Cross( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	D3DXVECTOR3 v;

	v.x = pV1->y * pV2->z - pV1->z * pV2->y;
	v.y = pV1->z * pV2->x - pV1->x * pV2->z;
	v.z = pV1->x * pV2->y - pV1->y * pV2->x;

	*pOut = v;
	return pOut;
}

inline FLOAT D3DXVec3Dot( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

inline D3DXMATRIX* D3DXMatrixInverse( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM )
{
	Assert( sizeof( D3DXMATRIX ) == (16 * sizeof(float) ) );
	Assert( sizeof( VMatrix ) == (16 * sizeof(float) ) );
	Assert( pDeterminant == NULL );	// homey don't play that
	
	VMatrix *origM = (VMatrix*)pM;
	VMatrix *destM = (VMatrix*)pOut;
	
	bool success = MatrixInverseGeneral( *origM, *destM ); (void)success;
	Assert( success );
	
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranspose( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM )
{
	if (pOut != pM)
	{
		for( int i=0; i<4; i++)
		{
			for( int j=0; j<4; j++)
			{
				pOut->m[i][j] = pM->m[j][i];
			}
		}
	}
	else
	{
		D3DXMATRIX temp = *pM;
		D3DXMatrixTranspose( pOut, &temp );
	}

	return NULL;
}

inline D3DXPLANE* D3DXPlaneNormalize( D3DXPLANE *pOut, CONST D3DXPLANE *pP)
{
	// not very different from normalizing a vector.
	// figure out the square root of the sum-of-squares of the x,y,z components
	// make sure that's non zero
	// then divide all four components by that value
	// or return some dummy plane like 0,0,1,0 if it fails
	
	float	len = sqrt( (pP->a * pP->a) + (pP->b * pP->b) + (pP->c * pP->c) );
	if (len > 1e-10)	//FIXME need a real epsilon here ?
	{
		pOut->a = pP->a / len;		pOut->b = pP->b / len;		pOut->c = pP->c / len;		pOut->d = pP->d / len;
	}
	else
	{
		pOut->a = 0.0f;				pOut->b = 0.0f;				pOut->c = 1.0f;				pOut->d = 0.0f;
	}
	return pOut;
}

inline D3DXVECTOR4* D3DXVec4Transform( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM )
{
	VMatrix *mat = (VMatrix*)pM;
	Vector4D *vIn = (Vector4D*)pV;
	Vector4D *vOut = (Vector4D*)pOut;

	Vector4DMultiplyTranspose( *mat, *vIn, *vOut );

	return pOut;
}


inline D3DXVECTOR4* D3DXVec4Normalize( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV )
{
	Vector4D *vIn = (Vector4D*) pV;
	Vector4D *vOut = (Vector4D*) pOut;

	*vOut = *vIn;
	Vector4DNormalize( *vOut );
	
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranslation( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z )
{
	D3DXMatrixIdentity( pOut );
	pOut->m[3][0] = x;
	pOut->m[3][1] = y;
	pOut->m[3][2] = z;
	return pOut;
}

// Build an ortho projection matrix. (right-handed)
inline D3DXMATRIX* D3DXMatrixOrthoOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,FLOAT zf )
{
	DXABSTRACT_BREAK_ON_ERROR();
	return NULL;
}


inline D3DXMATRIX* D3DXMatrixPerspectiveRH( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf )
{
	DXABSTRACT_BREAK_ON_ERROR();
	return NULL;
}


inline D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf )
{
	DXABSTRACT_BREAK_ON_ERROR();
	return NULL;
}

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
inline D3DXPLANE* D3DXPlaneTransform( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM )
{
	float *out = &pOut->a;

	// dot dot dot
	for( int x=0; x<4; x++ )
	{
		out[x] =	(pM->m[0][x] * pP->a)
				+	(pM->m[1][x] * pP->b)
				+	(pM->m[2][x] * pP->c)
				+	(pM->m[3][x] * pP->d);
	}
	
	return pOut;
}

inline HRESULT D3DXCompileShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable)
{
	DXABSTRACT_BREAK_ON_ERROR();	// is anyone calling this ?
	return S_OK;
}

inline DWORD D3DXGetShaderVersion(const DWORD *byte_code)
{
	return byte_code ? *byte_code : 0;
}

#endif
