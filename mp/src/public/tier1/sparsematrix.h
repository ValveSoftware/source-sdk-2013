//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//  A class allowing storage of a sparse NxN matirx as an array of sparse rows
//===========================================================================//

#ifndef SPARSEMATRIX_H
#define SPARSEMATRIX_H

#include "tier1/utlvector.h"

/// CSparseMatrix is a matrix which compresses each row individually, not storing the zeros.  NOte,
/// that while you can randomly set any element in a CSparseMatrix, you really want to do it top to
/// bottom or you will have bad perf as data is moved around to insert new elements.
class CSparseMatrix
{
public:
	struct NonZeroValueDescriptor_t
	{
		int m_nColumnNumber;
		float m_flValue;
	};

	struct RowDescriptor_t
	{
		int m_nNonZeroCount;								// number of non-zero elements in the row
		int m_nDataIndex;									// index of NonZeroValueDescriptor_t for the first non-zero value
	};

	int m_nNumRows;
	int m_nNumCols;
	CUtlVector<RowDescriptor_t> m_rowDescriptors;
	CUtlVector<NonZeroValueDescriptor_t> m_entries;
	int m_nHighestRowAppendedTo;

	void AdjustAllRowIndicesAfter( int nStartRow, int nDelta );
public:
	FORCEINLINE float Element( int nRow, int nCol ) const;
	void SetElement( int nRow, int nCol, float flValue );
	void SetDimensions( int nNumRows, int nNumCols );
	void AppendElement( int nRow, int nCol, float flValue );
	void FinishedAppending( void );

	FORCEINLINE int Height( void ) const { return m_nNumRows; }
	FORCEINLINE int Width( void ) const { return m_nNumCols; }

};



FORCEINLINE float CSparseMatrix::Element( int nRow, int nCol ) const
{
	Assert( nCol < m_nNumCols );
	int nCount = m_rowDescriptors[nRow].m_nNonZeroCount;
	if ( nCount )
	{
		NonZeroValueDescriptor_t const *pValue = &(m_entries[m_rowDescriptors[nRow].m_nDataIndex]);
		do
		{
			int nIdx = pValue->m_nColumnNumber;
			if ( nIdx == nCol )
			{
				return pValue->m_flValue;
			}
			if ( nIdx > nCol )
			{
				break;
			}
			pValue++;
		} while( --nCount );
	}
	return 0;
}



// type-specific overrides of matrixmath template for special case sparse routines

namespace MatrixMath
{
	/// sparse * dense matrix x matrix multiplication
	template<class BTYPE, class OUTTYPE> 
	void MatrixMultiply( CSparseMatrix const &matA, BTYPE const &matB, OUTTYPE *pMatrixOut )
	{
		Assert( matA.Width() == matB.Height() );
		pMatrixOut->SetDimensions( matA.Height(), matB.Width() );
		for( int i = 0; i < matA.Height(); i++ )
		{
			for( int j = 0; j < matB.Width(); j++ )
			{
				// compute inner product efficiently because of sparsity
				int nCnt = matA.m_rowDescriptors[i].m_nNonZeroCount;
				int nDataIdx = matA.m_rowDescriptors[i].m_nDataIndex;
				float flDot = 0.0;
				for( int nIdx = 0; nIdx < nCnt; nIdx++ )
				{
					float flAValue = matA.m_entries[nIdx + nDataIdx].m_flValue;
					int nCol = matA.m_entries[nIdx + nDataIdx].m_nColumnNumber;
					flDot += flAValue * matB.Element( nCol, j );
				}
				pMatrixOut->SetElement( i, j, flDot );
			}
		}
	}

	FORCEINLINE void AppendElement( CSparseMatrix &matrix, int nRow, int nCol, float flValue )
	{
		matrix.AppendElement( nRow, nCol, flValue );			// default implementation
	}

	FORCEINLINE void FinishedAppending( CSparseMatrix &matrix )
	{
		matrix.FinishedAppending();
	}

};





#endif // SPARSEMATRIX_H
