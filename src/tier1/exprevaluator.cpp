//===== Copyright � 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: 	ExprSimplifier builds a binary tree from an infix expression (in the
//				form of a character array). Evaluates C style infix parenthetic logical
//				expressions. Supports !, ||, &&, (). Symbols are resolved via callback.
//				Syntax is $<name>. $0 evaluates to false. $<number> evaluates to true.
//				e.g: ( $1 || ( $FOO || $WHATEVER ) && !$BAR )
//===========================================================================//

#include <ctype.h>
#include <vstdlib/IKeyValuesSystem.h>
#include "tier1/exprevaluator.h"
#include "tier1/convar.h"
#include "tier1/fmtstr.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Default conditional symbol handler callback. Symbols are the form $<name>.
// Return true or false for the value of the symbol.
//-----------------------------------------------------------------------------
bool DefaultConditionalSymbolProc( const char *pKey )
{
	if ( pKey[0] == '$' )
	{
		pKey++;
	}

	if ( !V_stricmp( pKey, "WIN32" ) )
	{
		return IsPC();
	}

	if ( !V_stricmp( pKey, "WINDOWS" ) )
	{
		return IsWindows();
	}
	
	if ( !V_stricmp( pKey, "X360" ) )
	{
		return IsX360();
	}

	if ( !V_stricmp( pKey, "PS3" ) )
	{
		return IsPS3();
	}

	if ( !V_stricmp( pKey, "OSX" ) )
	{
		return IsOSX();
	}

	if ( !V_stricmp( pKey, "LINUX" ) )
	{
		return IsLinux();
	}

	if ( !V_stricmp( pKey, "POSIX" ) )
	{
		return IsPosix();
	}	
	
	if ( !V_stricmp( pKey, "GAMECONSOLE" ) )
	{
		return IsGameConsole();
	}

	if ( !V_stricmp( pKey, "DEMO" ) )
	{
#if defined( _DEMO )
		return true;
#else
		return false;
#endif
	}

	if ( !V_stricmp( pKey, "LOWVIOLENCE" ) )
	{
#if defined( _LOWVIOLENCE )
		return true;
#endif
		// If it is not a LOWVIOLENCE binary build, then fall through
		// and check if there was a run-time symbol installed for it
	}

	// don't know it at compile time, so fall through to installed symbol values
	return KeyValuesSystem()->GetKeyValuesExpressionSymbol( pKey );
}

void DefaultConditionalErrorProc( const char *pReason )
{
	Warning( "Conditional Error: %s\n", pReason );
}

CExpressionEvaluator::CExpressionEvaluator()
{
	m_ExprTree = NULL;
}

CExpressionEvaluator::~CExpressionEvaluator()
{
	FreeTree( m_ExprTree );
}

//-----------------------------------------------------------------------------
//	Sets mCurToken to the next token in the input string. Skips all whitespace.
//-----------------------------------------------------------------------------
char CExpressionEvaluator::GetNextToken( void )
{
	// while whitespace, Increment CurrentPosition
	while ( m_pExpression[m_CurPosition] == ' ' )
		++m_CurPosition;
    
	// CurrentToken = Expression[CurrentPosition]
	m_CurToken = m_pExpression[m_CurPosition++];
  
	return m_CurToken;
}


//-----------------------------------------------------------------------------
//	Utility funcs
//-----------------------------------------------------------------------------
void CExpressionEvaluator::FreeNode( ExprNode *pNode )
{
	delete pNode;
}

ExprNode *CExpressionEvaluator::AllocateNode( void )
{
	return new ExprNode;
}

void CExpressionEvaluator::FreeTree( ExprTree& node )
{
	if ( !node )
		return;

	FreeTree( node->left );
	FreeTree( node->right );
	FreeNode( node );
	node = 0;
}

bool CExpressionEvaluator::IsConditional( bool &bConditional, const char token )
{
	char nextchar = ' ';
	if ( token == OR_OP || token == AND_OP )
	{
		// expect || or &&
		nextchar = m_pExpression[m_CurPosition++];
		if ( (token & nextchar) == token )
		{
			bConditional = true;
		}
		else if ( m_pSyntaxErrorProc )
		{
			m_pSyntaxErrorProc( CFmtStr( "Bad expression operator: '%c%c', expected C style operator", token, nextchar ) );
			return false;
		}
	}
	else
	{
		bConditional = false;
	}

	// valid
	return true;
}

bool CExpressionEvaluator::IsNotOp( const char token )
{
	if ( token == NOT_OP )
		return true;
	else
		return false;
}

bool CExpressionEvaluator::IsIdentifierOrConstant( const char token )
{
	bool success = false;
	if ( token == '$' )
	{
		// store the entire identifier
		int i = 0;
		m_Identifier[i++] = token;
		while( (V_isalnum( m_pExpression[m_CurPosition] ) || m_pExpression[m_CurPosition] == '_') && i < MAX_IDENTIFIER_LEN )
		{
			m_Identifier[i] = m_pExpression[m_CurPosition];
			++m_CurPosition;
			++i;
		}

		if ( i < MAX_IDENTIFIER_LEN - 1 )
		{
			m_Identifier[i] = '\0';
			success = true;
		}
	}
	else
	{
		if ( V_isdigit( token ) )
		{
			int i = 0;
			m_Identifier[i++] = token;
			while( V_isdigit( m_pExpression[m_CurPosition] ) && ( i < MAX_IDENTIFIER_LEN ) )
			{
				m_Identifier[i] = m_pExpression[m_CurPosition];
				++m_CurPosition;
				++i;
			}
			if ( i < MAX_IDENTIFIER_LEN - 1 )
			{
				m_Identifier[i] = '\0';
				success = true;
			}
		}
	}

	return success;
}

bool CExpressionEvaluator::MakeExprNode( ExprTree &tree, char token, Kind kind, ExprTree left, ExprTree right )
{
	tree = AllocateNode();
	tree->left = left;
	tree->right = right;
	tree->kind = kind;

	switch ( kind )
	{
	case CONDITIONAL:
		tree->data.cond = token;
		break;

	case LITERAL:
		if ( V_isdigit( m_Identifier[0] ) )
		{
			tree->data.value = ( atoi( m_Identifier ) != 0 );
		}
		else
		{
			tree->data.value = m_pGetSymbolProc( m_Identifier );
		}
		break;

	case NOT:
		break;

	default:
		if ( m_pSyntaxErrorProc )
		{
			Assert( 0 );
			m_pSyntaxErrorProc( CFmtStr( "Logic Error in CExpressionEvaluator" ) );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//	Makes a factor :: { <expression> } | <identifier>.
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::MakeFactor( ExprTree &tree )
{
	if ( m_CurToken == '(' )
	{
		// Get the next token
		GetNextToken();

		// Make an expression, setting Tree to point to it
		if ( !MakeExpression( tree ) )
		{
			return false;
		}
	}
	else if ( IsIdentifierOrConstant( m_CurToken ) )
	{
		// Make a literal node, set Tree to point to it, set left/right children to NULL. 
		if ( !MakeExprNode( tree, m_CurToken, LITERAL, NULL, NULL ) )
		{
			return false;
		}
	}
	else if ( IsNotOp( m_CurToken ) )
	{
		// do nothing
		return true;
	}
	else
	{
		// This must be a bad token
		if ( m_pSyntaxErrorProc )
		{
			m_pSyntaxErrorProc( CFmtStr( "Bad expression token: %c", m_CurToken ) );
		}
		return false;
	}

	// Get the next token
	GetNextToken();
	return true;
}

//-----------------------------------------------------------------------------
//	Makes a term :: <factor> { <not> }.
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::MakeTerm( ExprTree &tree )
{
	// Make a factor, setting Tree to point to it
	if ( !MakeFactor( tree ) )
	{
		return false;
	}

	// while the next token is !
	while ( IsNotOp( m_CurToken ) )
	{
		// Make an operator node, setting left child to Tree and right to NULL. (Tree points to new node)
		if ( !MakeExprNode( tree, m_CurToken, NOT, tree, NULL ) )
		{
			return false;
		}

		// Get the next token.
		GetNextToken();

		// Make a factor, setting the right child of Tree to point to it.
		if ( !MakeFactor( tree->right ) )
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
//	Makes a complete expression :: <term> { <cond> <term> }.
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::MakeExpression( ExprTree &tree )
{
	// Make a term, setting Tree to point to it
	if ( !MakeTerm( tree ) )
	{
		return false;
	}

	// while the next token is a conditional
	while ( 1 )
	{
		bool bConditional = false;
		bool bValid = IsConditional( bConditional, m_CurToken );
		if ( !bValid )
		{
			return false;
		}

		if ( !bConditional )
		{
			break;
		}

		// Make a conditional node, setting left child to Tree and right to NULL. (Tree points to new node)
		if ( !MakeExprNode( tree, m_CurToken, CONDITIONAL, tree, NULL ) )
		{
			return false;
		}

		// Get the next token.
		GetNextToken();

		// Make a term, setting the right child of Tree to point to it.
		if ( !MakeTerm( tree->right ) )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
//	returns true for success, false for failure
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::BuildExpression( void )
{
	// Get the first token, and build the tree.
	GetNextToken();

	return ( MakeExpression( m_ExprTree ) );
}

//-----------------------------------------------------------------------------
//	returns the value of the node after resolving all children
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::SimplifyNode( ExprTree& node )
{
	if ( !node )
		return false;

	// Simplify the left and right children of this node
	bool leftVal = SimplifyNode(node->left);
	bool rightVal = SimplifyNode(node->right);

	// Simplify this node
	switch( node->kind )
	{
	case NOT:
		// the child of '!' is always to the right
		node->data.value = !rightVal;
		break;
	
	case CONDITIONAL:
		if ( node->data.cond == AND_OP )
		{
			node->data.value = leftVal && rightVal;
		}
		else // OR_OP
		{	
			node->data.value = leftVal || rightVal;
		}
		break;

	default: // LITERAL
		break;
	}

	// This node has beed resolved
	node->kind = LITERAL;
	return node->data.value;
}

//-----------------------------------------------------------------------------
//	Interface to solve a conditional expression. Returns false on failure, Result is undefined.
//-----------------------------------------------------------------------------
bool CExpressionEvaluator::Evaluate( bool &bResult, const char *pInfixExpression, GetSymbolProc_t pGetSymbolProc, SyntaxErrorProc_t pSyntaxErrorProc )
{
	if ( !pInfixExpression )
	{
		return false;
	}

	// for caller simplicity, we strip of any enclosing braces
	// strip the bracketing [] if present
	char szCleanToken[512];
	if ( pInfixExpression[0] == '[' )
	{
		int len = V_strlen( pInfixExpression );

		// SECURITY: Bail on input buffers that are too large, they're used for RCEs and we don't 
		// need to support them.
		if ( len + 1 > ARRAYSIZE( szCleanToken ) )
		{
			return false;
		}

		V_strncpy( szCleanToken, pInfixExpression + 1, len );
		len--;
		if ( szCleanToken[len-1] == ']' )
		{
			szCleanToken[len-1] = '\0';
		}
		pInfixExpression = szCleanToken;
	}

	// reset state
	m_pExpression = pInfixExpression;
	m_pGetSymbolProc = pGetSymbolProc ? pGetSymbolProc : DefaultConditionalSymbolProc;
	m_pSyntaxErrorProc = pSyntaxErrorProc ? pSyntaxErrorProc : DefaultConditionalErrorProc;
	m_ExprTree = 0;
	m_CurPosition = 0;
	m_CurToken = 0;

	// Building the expression tree will fail on bad syntax
	bool bValid = BuildExpression();
	if ( bValid )
	{
		bResult = SimplifyNode( m_ExprTree );
	}

	// don't leak
	FreeTree( m_ExprTree );
	m_ExprTree = NULL;

	return bValid;
}








