//============================================================================
//                                  I B E X                                   
// File        : ibex_ExprGenerator.h
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Jun 19, 2012
// Last Update : May 06, 2016
//============================================================================

#ifndef __IBEX_PARSER_EXPR_GENERATOR_H__
#define __IBEX_PARSER_EXPR_GENERATOR_H__

#include "ibex_P_ExprVisitor.h"
#include "ibex_Expr.h"
#include "ibex_Scope.h"
#include "ibex_DoubleIndex.h"

namespace ibex {
namespace parser {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

class ExprGenerator : public virtual P_ExprVisitor {
public:
	ExprGenerator(const Scope& scope);

	const Domain& generate_cst(const P_ExprNode& y);

	const ExprNode& generate(const Array<const P_ExprSymbol>& old_x, const Array<const ExprSymbol>& new_x, const P_ExprNode& y);

protected:
	void visit(const P_ExprNode&);
	void visit(const P_ExprIndex&);

	void visit_power(const P_ExprNode&);

	std::pair<int,int> visit_index_tmp(const Dim& dim, const P_ExprNode& idx, bool matlab_style);
	DoubleIndex visit_index(const Dim& dim, const P_ExprNode& idx1, bool matlab_style);
	DoubleIndex visit_index(const Dim& dim, const P_ExprNode& idx1, const P_ExprNode& idx2, bool matlab_style);

	const Scope& scope;
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // end namespace parser
} // end namespace ibex

#endif // __IBEX_PARSER_EXPR_GENERATOR_H__
