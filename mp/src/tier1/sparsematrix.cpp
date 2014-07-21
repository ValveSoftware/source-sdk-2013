//========= Copyright Valve Corporation, All rights reserved. ============//
//
//===========================================================================//

#include "tier1/sparsematrix.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CSparseMatrix::AdjustAllRowIndicesAfter( int nStartRow, int nDelta )
{
	// now, we need to offset the starting position of all subsequent rows by -1 to compensate for the removal of this element
	for( int nOtherRow = nStartRow + 1 ; nOtherRow < Height(); nOtherRow++ )
	{
		m_rowDescriptors[nOtherRow].m_nDataIndex += nDelta;
	}
}

void CSparseMatrix::SetDimensions( int nNumRows, int nNumCols )
{
	m_nNumRows = nNumRows;
	m_nNumCols = nNumCols;
	m_entries.SetCount( 0 );
	m_rowDescriptors.SetCount( m_nNumRows );
	// and set all rows to be empty
	for( int i = 0; i < m_nNumRows; i++ )
	{
		m_rowDescriptors[i].m_nNonZeroCount = 0;
		m_rowDescriptors[i].m_nDataIndex = 0;
	}
	m_nHighestRowAppendedTo = -1;

}


void CSparseMatrix::SetElement( int nRow, int nCol, float flValue ) 
{
	Assert( nCol < m_nNumCols );
	int nCount = m_rowDescriptors[nRow].m_nNonZeroCount;
	bool bValueIsZero = ( flValue == 0.0 );
	int nFirstEntryIndex = m_rowDescriptors[nRow].m_nDataIndex;
	if ( nCount )
	{
		NonZeroValueDescriptor_t *pValue = &( m_entries[nFirstEntryIndex] );
		int i;
		for( i = 0; i < nCount; i++ )
		{
			int nIdx = pValue->m_nColumnNumber;
			if ( nIdx == nCol )								// we found it!
			{
				if ( !bValueIsZero )
				{
					// we want to overwrite the existing value
					pValue->m_flValue = flValue;
				}
				else
				{
					// there is a non-zero element currently at this position. We need to remove it
					// and we need to remove its storage.
					m_rowDescriptors[nRow].m_nNonZeroCount--;
					m_entries.Remove( nFirstEntryIndex + i );
					// now, we need to offset the starting position of all subsequent rows by -1 to compensate for the removal of this element
					AdjustAllRowIndicesAfter( nRow, -1 );
				}
				return;
			}
			if ( nIdx > nCol )
			{
				break;
			}
			pValue++;
		}
		// we did not find an entry for this cell. If we were writing zero, fine - we are
		// done, otherwise insert
		if (! bValueIsZero )
		{
			m_rowDescriptors[nRow].m_nNonZeroCount++;
			NonZeroValueDescriptor_t newValue;
			newValue.m_nColumnNumber = nCol;
			newValue.m_flValue = flValue;
			if ( i == nCount )								// need to append
			{
				m_entries.InsertAfter( nFirstEntryIndex + nCount - 1, newValue );
			}
			else
			{
				m_entries.InsertBefore( nFirstEntryIndex + i, newValue );
			}
			// now, we need to offset the starting position of all subsequent rows by -1 to compensate for the addition of this element
			AdjustAllRowIndicesAfter( nRow, +1 );
		}
	}
	else
	{
		// row is empty. We may need to insert
		if ( ! bValueIsZero )
		{
			m_rowDescriptors[nRow].m_nNonZeroCount++;
			NonZeroValueDescriptor_t newValue;
			newValue.m_nColumnNumber = nCol;
			newValue.m_flValue = flValue;
			m_entries.InsertBefore( nFirstEntryIndex, newValue );
			AdjustAllRowIndicesAfter( nRow, +1 );
		}
	}
}

void CSparseMatrix::FinishedAppending( void )
{
	// set all pointers to space for subsequent rows to the right value
	for( int i = m_nHighestRowAppendedTo + 1 ; i < Height(); i++ )
	{
		m_rowDescriptors[i].m_nDataIndex = m_entries.Count();
	}
}

void CSparseMatrix::AppendElement( int nRow, int nColumn, float flValue )
{
	if ( flValue != 0.0 )
	{
		if ( m_nHighestRowAppendedTo != nRow )
		{
			Assert( nRow > m_nHighestRowAppendedTo );
			for( int i = m_nHighestRowAppendedTo + 1; i <= nRow; i++ )
			{
				m_rowDescriptors[i].m_nDataIndex = m_entries.Count();
			}
		}
		m_nHighestRowAppendedTo = nRow;
		m_rowDescriptors[nRow].m_nNonZeroCount++;
		NonZeroValueDescriptor_t newDesc;
		newDesc.m_nColumnNumber = nColumn;
		newDesc.m_flValue = flValue;
		m_entries.AddToTail( newDesc );
	}
}
	
	


