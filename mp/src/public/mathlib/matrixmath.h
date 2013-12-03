//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//  A set of generic, template-based matrix functions.
//===========================================================================//

#ifndef MATRIXMATH_H
#define MATRIXMATH_H

#include <stdarg.h>

// The operations in this file can perform basic matrix operations on matrices represented
// using any class that supports the necessary operations:
//
//  .Element( row, col )  - return the element at a given matrox position
//  .SetElement( row, col, val ) - modify an element
//  .Width(), .Height() - get dimensions
//  .SetDimensions( nrows, ncols) - set a matrix to be un-initted and the appropriate size
//
// Generally, vectors can be used with these functions by using N x 1 matrices to represent them.
//  Matrices are addressed as row, column, and indices are 0-based
//
//
// Note that the template versions of these routines are defined for generality - it is expected
// that template specialization is used for common high performance cases.

namespace MatrixMath
{
	/// M *= flScaleValue
	template<class MATRIXCLASS>
	void ScaleMatrix( MATRIXCLASS &matrix, float flScaleValue )
	{
		for( int i = 0; i < matrix.Height(); i++ )
		{
			for( int j = 0; j < matrix.Width(); j++ )
			{
				matrix.SetElement( i, j, flScaleValue * matrix.Element( i, j ) );
			}
		}
	}

	/// AppendElementToMatrix - same as setting the element, except only works when all calls
	/// happen in top to bottom left to right order, end you have to call FinishedAppending when
	/// done. For normal matrix classes this is not different then SetElement, but for
	/// CSparseMatrix, it is an accelerated way to fill a matrix from scratch.
	template<class MATRIXCLASS>
	FORCEINLINE void AppendElement( MATRIXCLASS &matrix, int nRow, int nCol, float flValue )
	{
		matrix.SetElement( nRow, nCol, flValue );			// default implementation
	}

	template<class MATRIXCLASS>
	FORCEINLINE void FinishedAppending( MATRIXCLASS &matrix ) {} // default implementation

	/// M += fl
	template<class MATRIXCLASS>
	void AddToMatrix( MATRIXCLASS &matrix, float flAddend )
	{
		for( int i = 0; i < matrix.Height(); i++ )
		{
			for( int j = 0; j < matrix.Width(); j++ )
			{
				matrix.SetElement( i, j, flAddend + matrix.Element( i, j ) );
			}
		}
	}

	/// transpose
	template<class MATRIXCLASSIN, class MATRIXCLASSOUT>
	void TransposeMatrix( MATRIXCLASSIN const &matrixIn, MATRIXCLASSOUT *pMatrixOut )
	{
		pMatrixOut->SetDimensions( matrixIn.Width(), matrixIn.Height() );
		for( int i = 0; i < pMatrixOut->Height(); i++ )
		{
			for( int j = 0; j < pMatrixOut->Width(); j++ )
			{
				AppendElement( *pMatrixOut, i, j, matrixIn.Element( j, i ) );
			}
		}
		FinishedAppending( *pMatrixOut );
	}

	/// copy
	template<class MATRIXCLASSIN, class MATRIXCLASSOUT>
	void CopyMatrix( MATRIXCLASSIN const &matrixIn, MATRIXCLASSOUT *pMatrixOut )
	{
		pMatrixOut->SetDimensions( matrixIn.Height(), matrixIn.Width() );
		for( int i = 0; i < matrixIn.Height(); i++ )
		{
			for( int j = 0; j < matrixIn.Width(); j++ )
			{
				AppendElement( *pMatrixOut, i, j, matrixIn.Element( i, j ) );
			}
		}
		FinishedAppending( *pMatrixOut );
	}



	/// M+=M
	template<class MATRIXCLASSIN, class MATRIXCLASSOUT>
	void AddMatrixToMatrix( MATRIXCLASSIN const &matrixIn, MATRIXCLASSOUT *pMatrixOut )
	{
		for( int i = 0; i < matrixIn.Height(); i++ )
		{
			for( int j = 0; j < matrixIn.Width(); j++ )
			{
				pMatrixOut->SetElement( i, j, pMatrixOut->Element( i, j ) + matrixIn.Element( i, j ) );
			}
		}
	}

	// M += scale * M
	template<class MATRIXCLASSIN, class MATRIXCLASSOUT>
	void AddScaledMatrixToMatrix( float flScale, MATRIXCLASSIN const &matrixIn, MATRIXCLASSOUT *pMatrixOut )
	{
		for( int i = 0; i < matrixIn.Height(); i++ )
		{
			for( int j = 0; j < matrixIn.Width(); j++ )
			{
				pMatrixOut->SetElement( i, j, pMatrixOut->Element( i, j ) + flScale * matrixIn.Element( i, j ) );
			}
		}
	}


	// simple way to initialize a matrix with constants from code.
	template<class MATRIXCLASSOUT> 
	void SetMatrixToIdentity( MATRIXCLASSOUT *pMatrixOut, float flDiagonalValue = 1.0 )
	{
		for( int i = 0; i < pMatrixOut->Height(); i++ )
		{
			for( int j = 0; j < pMatrixOut->Width(); j++ )
			{
				AppendElement( *pMatrixOut, i, j, ( i == j ) ? flDiagonalValue : 0 );
			}
		}
		FinishedAppending( *pMatrixOut );
	}

	//// simple way to initialize a matrix with constants from code
	template<class MATRIXCLASSOUT> 
	void SetMatrixValues( MATRIXCLASSOUT *pMatrix, int nRows, int nCols, ... )
	{
		va_list argPtr;
		va_start( argPtr, nCols );

		pMatrix->SetDimensions( nRows, nCols );
		for( int nRow = 0; nRow < nRows; nRow++ )
		{
			for( int nCol = 0; nCol < nCols; nCol++ )
			{
				double flNewValue = va_arg( argPtr, double );
				pMatrix->SetElement( nRow, nCol, flNewValue );
			}
		}
		va_end( argPtr );
	}


	/// row and colum accessors. treat a row or a column as a column vector
	template<class MATRIXTYPE> class MatrixRowAccessor
	{
	public:
		FORCEINLINE MatrixRowAccessor( MATRIXTYPE const &matrix, int nRow )
		{
			m_pMatrix = &matrix;
			m_nRow = nRow;
		}

		FORCEINLINE float Element( int nRow, int nCol ) const
		{
			Assert( nCol == 0 );
			return m_pMatrix->Element( m_nRow, nRow );
		}

		FORCEINLINE int Width( void ) const { return 1; };
		FORCEINLINE int Height( void ) const { return m_pMatrix->Width(); }

	private:
		MATRIXTYPE const *m_pMatrix;
		int m_nRow;
	};

	template<class MATRIXTYPE> class MatrixColumnAccessor
	{
	public:
		FORCEINLINE MatrixColumnAccessor( MATRIXTYPE const &matrix, int nColumn )
		{
			m_pMatrix = &matrix;
			m_nColumn = nColumn;
		}

		FORCEINLINE float Element( int nRow, int nColumn ) const
		{
			Assert( nColumn == 0 );
			return m_pMatrix->Element( nRow, m_nColumn );
		}

		FORCEINLINE int Width( void ) const { return 1; }
		FORCEINLINE int Height( void ) const { return m_pMatrix->Height(); }
	private:
		MATRIXTYPE const *m_pMatrix;
		int m_nColumn;
	};

	/// this translator acts as a proxy for the transposed matrix
	template<class MATRIXTYPE> class MatrixTransposeAccessor
	{
	public:
		FORCEINLINE MatrixTransposeAccessor( MATRIXTYPE const & matrix )
		{
			m_pMatrix = &matrix;
		}

		FORCEINLINE float Element( int nRow, int nColumn ) const
		{
			return m_pMatrix->Element( nColumn, nRow );
		}

		FORCEINLINE int Width( void ) const { return m_pMatrix->Height(); }
		FORCEINLINE int Height( void ) const { return m_pMatrix->Width(); }
	private:
		MATRIXTYPE const *m_pMatrix;
	};

	/// this tranpose returns a wrapper around it's argument, allowing things like AddMatrixToMatrix( Transpose( matA ), &matB ) without an extra copy
	template<class MATRIXCLASSIN>
	MatrixTransposeAccessor<MATRIXCLASSIN> TransposeMatrix( MATRIXCLASSIN const &matrixIn )
	{
		return MatrixTransposeAccessor<MATRIXCLASSIN>( matrixIn );
	}


	/// retrieve rows and columns
	template<class MATRIXTYPE>
	FORCEINLINE MatrixColumnAccessor<MATRIXTYPE> MatrixColumn( MATRIXTYPE const &matrix, int nColumn )
	{
		return MatrixColumnAccessor<MATRIXTYPE>( matrix, nColumn );
	}

	template<class MATRIXTYPE>
	FORCEINLINE MatrixRowAccessor<MATRIXTYPE> MatrixRow( MATRIXTYPE const &matrix, int nRow )
	{
		return MatrixRowAccessor<MATRIXTYPE>( matrix, nRow );
	}

	//// dot product between vectors (or rows and/or columns via accessors)
	template<class MATRIXACCESSORATYPE, class MATRIXACCESSORBTYPE >
	float InnerProduct( MATRIXACCESSORATYPE const &vecA, MATRIXACCESSORBTYPE const &vecB )
	{
		Assert( vecA.Width() == 1 );
		Assert( vecB.Width() == 1 );
		Assert( vecA.Height() == vecB.Height() );
		double flResult = 0;
		for( int i = 0; i < vecA.Height(); i++ )
		{
			flResult += vecA.Element( i, 0 ) * vecB.Element( i, 0 );
		}
		return flResult;
	}



	/// matrix x matrix multiplication
	template<class MATRIXATYPE, class MATRIXBTYPE, class MATRIXOUTTYPE>
	void MatrixMultiply( MATRIXATYPE const &matA, MATRIXBTYPE const &matB, MATRIXOUTTYPE *pMatrixOut )
	{
		Assert( matA.Width() == matB.Height() );
		pMatrixOut->SetDimensions( matA.Height(), matB.Width() );
		for( int i = 0; i < matA.Height(); i++ )
		{
			for( int j = 0; j < matB.Width(); j++ )
			{
				pMatrixOut->SetElement( i, j, InnerProduct( MatrixRow( matA, i ), MatrixColumn( matB, j ) ) );
			}
		}
	}

	/// solve Ax=B via the conjugate graident method. Code and naming conventions based on the
	/// wikipedia article.
	template<class ATYPE, class XTYPE, class BTYPE>
	void ConjugateGradient( ATYPE const &matA, BTYPE const &vecB, XTYPE &vecX, float flTolerance = 1.0e-20 )
	{
		XTYPE vecR;
		vecR.SetDimensions( vecX.Height(), 1 );
		MatrixMultiply( matA, vecX, &vecR );
		ScaleMatrix( vecR, -1 );
		AddMatrixToMatrix( vecB, &vecR );
		XTYPE vecP;
		CopyMatrix( vecR, &vecP );
		float flRsOld = InnerProduct( vecR, vecR );
		for( int nIter = 0; nIter < 100; nIter++ )
		{
			XTYPE vecAp;
			MatrixMultiply( matA, vecP, &vecAp );
			float flDivisor = InnerProduct( vecAp, vecP );
			float flAlpha = flRsOld / flDivisor;
			AddScaledMatrixToMatrix( flAlpha, vecP, &vecX );
			AddScaledMatrixToMatrix( -flAlpha, vecAp, &vecR );
			float flRsNew = InnerProduct( vecR, vecR );
			if ( flRsNew < flTolerance )
			{
				break;
			}
			ScaleMatrix( vecP, flRsNew / flRsOld );
			AddMatrixToMatrix( vecR, &vecP );
			flRsOld = flRsNew;
		}
	}

	/// solve (A'*A) x=B via the conjugate gradient method. Code and naming conventions based on
	/// the wikipedia article. Same as Conjugate gradient but allows passing in two matrices whose
	/// product is used as the A matrix (in order to preserve sparsity)
	template<class ATYPE, class APRIMETYPE, class XTYPE, class BTYPE>
	void ConjugateGradient( ATYPE const &matA, APRIMETYPE const &matAPrime, BTYPE const &vecB, XTYPE &vecX, float flTolerance = 1.0e-20 )
	{
		XTYPE vecR1;
		vecR1.SetDimensions( vecX.Height(), 1 );
		MatrixMultiply( matA, vecX, &vecR1 );
		XTYPE vecR;
		vecR.SetDimensions( vecR1.Height(), 1 );
		MatrixMultiply( matAPrime, vecR1, &vecR );
		ScaleMatrix( vecR, -1 );
		AddMatrixToMatrix( vecB, &vecR );
		XTYPE vecP;
		CopyMatrix( vecR, &vecP );
		float flRsOld = InnerProduct( vecR, vecR );
		for( int nIter = 0; nIter < 100; nIter++ )
		{
			XTYPE vecAp1;
			MatrixMultiply( matA, vecP, &vecAp1 );
			XTYPE vecAp;
			MatrixMultiply( matAPrime, vecAp1, &vecAp );
			float flDivisor = InnerProduct( vecAp, vecP );
			float flAlpha = flRsOld / flDivisor;
			AddScaledMatrixToMatrix( flAlpha, vecP, &vecX );
			AddScaledMatrixToMatrix( -flAlpha, vecAp, &vecR );
			float flRsNew = InnerProduct( vecR, vecR );
			if ( flRsNew < flTolerance )
			{
				break;
			}
			ScaleMatrix( vecP, flRsNew / flRsOld );
			AddMatrixToMatrix( vecR, &vecP );
			flRsOld = flRsNew;
		}
	}

	
	template<class ATYPE,  class XTYPE, class BTYPE>
	void LeastSquaresFit( ATYPE const &matA, BTYPE const &vecB, XTYPE &vecX )
	{
		// now, generate the normal equations
		BTYPE vecBeta;
		MatrixMath::MatrixMultiply( MatrixMath::TransposeMatrix( matA ), vecB, &vecBeta );

		vecX.SetDimensions( matA.Width(), 1 );
		MatrixMath::SetMatrixToIdentity( &vecX );

		ATYPE matATransposed;
		TransposeMatrix( matA, &matATransposed );
		ConjugateGradient( matA, matATransposed, vecBeta, vecX, 1.0e-20 );
	}

};

/// a simple fixed-size matrix class
template<int NUMROWS, int NUMCOLS> class CFixedMatrix
{
public:
	FORCEINLINE int Width( void ) const { return NUMCOLS; }
	FORCEINLINE int Height( void ) const { return NUMROWS; }
	FORCEINLINE float Element( int nRow, int nCol ) const { return m_flValues[nRow][nCol]; }
	FORCEINLINE void SetElement( int nRow, int nCol, float flValue ) { m_flValues[nRow][nCol] = flValue; }
	FORCEINLINE void SetDimensions( int nNumRows, int nNumCols ) { Assert( ( nNumRows == NUMROWS ) && ( nNumCols == NUMCOLS ) ); }

private:
	float m_flValues[NUMROWS][NUMCOLS];
};



#endif //matrixmath_h
