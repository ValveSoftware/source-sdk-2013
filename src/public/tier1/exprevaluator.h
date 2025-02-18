//===== Copyright © 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: 	ExprSimplifier builds a binary tree from an infix expression (in the
//				form of a character array).
//
//===========================================================================//

#ifndef EXPREVALUATOR_H
#define EXPREVALUATOR_H

#if defined( _WIN32 )
#pragma once
#endif

static const char OR_OP   = '|';
static const char AND_OP  = '&';
static const char NOT_OP  = '!';

#define MAX_IDENTIFIER_LEN	128
enum Kind {CONDITIONAL, NOT, LITERAL};

struct ExprNode
{
	ExprNode *left;  // left sub-expression
	ExprNode *right; // right sub-expression
	Kind kind;       // kind of node this is
	union 
	{
		char cond;	// the conditional
		bool value;	// the value
	} data;
};

typedef ExprNode *ExprTree;

// callback to evaluate a $<symbol> during evaluation, return true or false
typedef bool (*GetSymbolProc_t)( const char *pKey );
typedef void (*SyntaxErrorProc_t)( const char *pReason );

class CExpressionEvaluator
{
public:
	CExpressionEvaluator();
	~CExpressionEvaluator();
	bool Evaluate( bool &result, const char *pInfixExpression, GetSymbolProc_t pGetSymbolProc = 0, SyntaxErrorProc_t pSyntaxErrorProc = 0 );

private:
	CExpressionEvaluator( CExpressionEvaluator& );	// prevent copy constructor being used

	char GetNextToken( void );
	void FreeNode( ExprNode *pNode );
	ExprNode *AllocateNode( void );
	void FreeTree( ExprTree &node );
	bool IsConditional( bool &bCondition, const char token );
	bool IsNotOp( const char token );
	bool IsIdentifierOrConstant( const char token );
	bool MakeExprNode( ExprTree &tree, char token, Kind kind, ExprTree left, ExprTree right );
	bool MakeFactor( ExprTree &tree );
	bool MakeTerm( ExprTree &tree );
	bool MakeExpression( ExprTree &tree );
	bool BuildExpression( void );
	bool SimplifyNode( ExprTree &node );

	ExprTree			m_ExprTree;							// Tree representation of the expression
	char				m_CurToken;							// Current token read from the input expression
	const char			*m_pExpression;						// Array of the expression characters
	int					m_CurPosition;						// Current position in the input expression
	char				m_Identifier[MAX_IDENTIFIER_LEN];	// Stores the identifier string
	GetSymbolProc_t		m_pGetSymbolProc;
	SyntaxErrorProc_t	m_pSyntaxErrorProc;
	bool				m_bSetup;
};

#endif
