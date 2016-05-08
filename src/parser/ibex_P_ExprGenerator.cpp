//============================================================================
//                                  I B E X                                   
// File        : ibex_ExprExprGenerator.cpp
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Jun 19, 2012
// Last Update : Jun 19, 2012
//============================================================================

#include "ibex_P_ExprGenerator.h"
#include "ibex_ConstantGenerator.h"
#include "ibex_SyntaxError.h"
#include "ibex_Exception.h"

using namespace std;

namespace ibex {
namespace parser {

int to_integer(const Domain& d) {
	assert(d.dim.is_scalar());
	assert(d.i().is_degenerated());
	double x=d.i().mid();
	assert(floor(x)==x);
	return (int)x;
}

double to_double(const Domain& d) {
	assert(d.dim.is_scalar());

//	switch(number_type) {
//	case NEG_INF: return NEG_INFINITY;
//	case POS_INF: return POS_INFINITY;
//	default:      return to_double(d); //eval(expr));
//	}
//
	// WARNING: this is quite unsafe. But
	// requiring d.i().is_degenerated() is wrong,
	// the result of a calculus with degenerated intervals
	// may not be degenerated (and large)... Still, we could
	// give some warning if, say, the diameter here was > 1e-10.
	return d.i().mid();
}

ExprGenerator::ExprGenerator(const Scope& scope) : scope(scope) {

}

const Domain& ExprGenerator::generate_cst(const P_ExprNode& y) {
	visit(y);

	return y.lab->domain();
}

const ExprNode& ExprGenerator::generate(const Array<const P_ExprSymbol>& old_x, const Array<const ExprSymbol>& new_x, const P_ExprNode& y) {

	for (int i=0; i<old_x.size(); i++) {
		old_x[i].lab = &new_x[i];
	}
	visit(y);

	return y.lab->node();
}

void ExprGenerator::visit(const P_ExprNode& e) {

	if (e.lab!=NULL) return;

	if (e.op==P_ExprNode::POWER) {
		visit_power(e);
		return;
	}
	if (e.op==P_ExprNode::EXPR_WITH_IDX) {
		e.acceptVisitor(*this);
		return;
	} if (e.op==P_ExprNode::SYMBOL) {
		e.acceptVisitor(*this);
		return;
	}

	Array<const Domain> arg_cst(e.arg.size());

	bool all_cst=true;
	for (int i=0; i<e.arg.size(); i++) {
		visit(e.arg[i]);
		all_cst = all_cst && (e.arg[i].lab->type()==Label::CONST);
		arg_cst.set_ref(i,e.arg[i].lab->domain());
	}

	if (all_cst) {

		if (e.op==P_ExprNode::MINUS) {
			LabelConst& c=*((LabelConst*) e.arg[0].lab);
			switch(c.number_type) {
			case LabelConst::POS_INF:
				e.lab = new LabelConst::neg_infinity();
				break;
			case LabelConst::NEG_INF:
				e.lab = new LabelConst::pos_infinity();
				break;
			default:
				e.lab = new LabelConst(-arg_cst[0]);
			}
		}

		try {
			switch(e.op) {
			case P_ExprNode::INF:    e.lab=new LabelConst::pos_infinity(); break;
			case P_ExprNode::SYMBOL: assert(false); /* impossible */ break;
			case P_ExprNode::CST:    e.lab=new LabelConst(((P_ExprConstant&) e).value); break; //TODO: ref??
			case P_ExprNode::ITER:   e.lab=new LabelConst(scope.get_iter_value(((P_ExprIter&) e).name)); break;
			case P_ExprNode::IDX:	 e.lab=new LabelConst(arg_cst[0]); break; // TODO: should be ref??
			case P_ExprNode::IDX_RANGE:
			case P_ExprNode::IDX_ALL:
			case P_ExprNode::EXPR_WITH_IDX:  assert(false); /* impossible */ break;
			case P_ExprNode::ROW_VEC: e.lab=new LabelConst(Domain(arg_cst,true)); break;
			case P_ExprNode::COL_VEC: e.lab=new LabelConst(Domain(arg_cst,false)); break;
			case P_ExprNode::APPLY:   e.lab=new LabelConst((((const P_ExprApply&) e).f).basic_evaluator().eval(arg_cst)); break;
			case P_ExprNode::CHI:     not_implemented("Chi in constant expressions"); break;
			case P_ExprNode::ADD:     e.lab=new LabelConst(arg_cst[0] + arg_cst[1]); break;
			case P_ExprNode::MUL:     e.lab=new LabelConst(arg_cst[0] * arg_cst[1]); break;
			case P_ExprNode::SUB:     e.lab=new LabelConst(arg_cst[0] - arg_cst[1]); break;
			case P_ExprNode::DIV:     e.lab=new LabelConst(arg_cst[0] / arg_cst[1]); break;
			case P_ExprNode::MAX:     e.lab=new LabelConst(max(arg_cst)); break;
			case P_ExprNode::MIN:     e.lab=new LabelConst(min(arg_cst)); break;
			case P_ExprNode::ATAN2:   e.lab=new LabelConst(atan2(arg_cst[0], arg_cst[1])); break;
			case P_ExprNode::POWER:
			case P_ExprNode::MINUS:   assert(false); /* impossible */ break;
			case P_ExprNode::TRANS:   e.lab=new LabelConst(transpose(arg_cst[0])); break;
			case P_ExprNode::SIGN:    e.lab=new LabelConst(sign (arg_cst[0])); break;
			case P_ExprNode::ABS:     e.lab=new LabelConst(abs  (arg_cst[0])); break;
			case P_ExprNode::SQR:     e.lab=new LabelConst(sqr  (arg_cst[0])); break;
			case P_ExprNode::SQRT:    e.lab=new LabelConst(sqrt (arg_cst[0])); break;
			case P_ExprNode::EXP:     e.lab=new LabelConst(exp  (arg_cst[0])); break;
			case P_ExprNode::LOG:     e.lab=new LabelConst(log  (arg_cst[0])); break;
			case P_ExprNode::COS:     e.lab=new LabelConst(cos  (arg_cst[0])); break;
			case P_ExprNode::SIN:     e.lab=new LabelConst(sin  (arg_cst[0])); break;
			case P_ExprNode::TAN:     e.lab=new LabelConst(tan  (arg_cst[0])); break;
			case P_ExprNode::ACOS:    e.lab=new LabelConst(acos (arg_cst[0])); break;
			case P_ExprNode::ASIN:    e.lab=new LabelConst(asin (arg_cst[0])); break;
			case P_ExprNode::ATAN:    e.lab=new LabelConst(atan (arg_cst[0])); break;
			case P_ExprNode::COSH:    e.lab=new LabelConst(cosh (arg_cst[0])); break;
			case P_ExprNode::SINH:    e.lab=new LabelConst(sinh (arg_cst[0])); break;
			case P_ExprNode::TANH:    e.lab=new LabelConst(tanh (arg_cst[0])); break;
			case P_ExprNode::ACOSH:   e.lab=new LabelConst(acosh(arg_cst[0])); break;
			case P_ExprNode::ASINH:   e.lab=new LabelConst(asinh(arg_cst[0])); break;
			case P_ExprNode::ATANH:   e.lab=new LabelConst(atanh(arg_cst[0])); break;
			}
		} catch(DimException& e) {
			throw SyntaxError(e.message());
		}
		return;
	}

	Array<const ExprNode> arg_node(e.arg.size());

	bool all_cst=true;
	for (int i=0; i<e.arg.size(); i++) {
		// may force constants to become nodes
		arg_node.set_ref(i,e.arg[i].lab->node());
	}

	//throw SyntaxError("Unexpected infinity symbol \"oo\""); break;
	ExprNode* node;

	try {
		switch(e.op) {
		case P_ExprNode::INF:
		case P_ExprNode::SYMBOL:
		case P_ExprNode::CST:
		case P_ExprNode::ITER:
		case P_ExprNode::IDX:
		case P_ExprNode::IDX_RANGE:
		case P_ExprNode::IDX_ALL:
		case P_ExprNode::EXPR_WITH_IDX: assert(false); /* impossible */ break;
		case P_ExprNode::ROW_VEC:node=&ExprVector::new_(arg_node,true); break;
		case P_ExprNode::COL_VEC:node=&ExprVector::new_(arg_node,false); break;
		case P_ExprNode::APPLY:  node=&(((const P_ExprApply&) e).f)(arg_node); break;
		case P_ExprNode::CHI:    node=&chi(arg_node[0], arg_node[1], arg_node[2]); break;
		case P_ExprNode::ADD:    node=&(arg_node[0] + arg_node[1]); break;
		case P_ExprNode::MUL:    node=&(arg_node[0] * arg_node[1]); break;
		case P_ExprNode::SUB:    node=&(arg_node[0] - arg_node[1]); break;
		case P_ExprNode::DIV:    node=&(arg_node[0] / arg_node[1]); break;
		case P_ExprNode::MAX:    node=&max(arg_node); break;
		case P_ExprNode::MIN:    node=&min(arg_node); break;
		case P_ExprNode::ATAN2:  node=&atan2(arg_node[0], arg_node[1]); break;
		case P_ExprNode::POWER:  assert(false); /* impossible */ break;
		case P_ExprNode::MINUS:  node=&(-arg_node[0]); break;
		case P_ExprNode::TRANS:  node=&transpose(arg_node[0]); break;
		case P_ExprNode::SIGN:   node=&sign (arg_node[0]); break;
		case P_ExprNode::ABS:    node=&abs  (arg_node[0]); break;
		case P_ExprNode::SQR:    node=&sqr  (arg_node[0]); break;
		case P_ExprNode::SQRT:   node=&sqrt (arg_node[0]); break;
		case P_ExprNode::EXP:    node=&exp  (arg_node[0]); break;
		case P_ExprNode::LOG:    node=&log  (arg_node[0]); break;
		case P_ExprNode::COS:    node=&cos  (arg_node[0]); break;
		case P_ExprNode::SIN:    node=&sin  (arg_node[0]); break;
		case P_ExprNode::TAN:    node=&tan  (arg_node[0]); break;
		case P_ExprNode::ACOS:   node=&acos (arg_node[0]); break;
		case P_ExprNode::ASIN:   node=&asin (arg_node[0]); break;
		case P_ExprNode::ATAN:   node=&atan (arg_node[0]); break;
		case P_ExprNode::COSH:   node=&cosh (arg_node[0]); break;
		case P_ExprNode::SINH:   node=&sinh (arg_node[0]); break;
		case P_ExprNode::TANH:   node=&tanh (arg_node[0]); break;
		case P_ExprNode::ACOSH:  node=&acosh(arg_node[0]); break;
		case P_ExprNode::ASINH:  node=&asinh(arg_node[0]); break;
		case P_ExprNode::ATANH:  node=&atanh(arg_node[0]); break;
		}
		e.lab = new LabelNode(node);
	} catch(DimException& e) {
		throw SyntaxError(e.message());
	}
}

void ExprGenerator::visit_power(const P_ExprNode& e) {

	visit(e.arg[0]);
	visit(e.arg[1]);

	Label& left=(*(e.arg[0].lab));
	Label& right=(*(e.arg[1].lab));

	typedef enum { IBEX_INTEGER, IBEX_INTERVAL, IBEX_EXPRNODE } _type;
	_type right_type;
	_type left_type;

	int int_right=0;
	Interval itv_right;
	Interval itv_left;

	if (right.type()==Label::CONST) {
		if (!right.domain().dim.is_scalar()) throw SyntaxError("exponent must be scalar");

		right_type=IBEX_INTERVAL;
		itv_right=right.domain().i();
		//delete cr; // not now (see comment in ExprCopy.h)
		// NOTE: we can delete cr because
		// even in the case where right_type==IBEX_INTERVAL and left_type==IBEX_EXPRNODE
		// we will recreate a ExprConstant node by multiplying itv_right (instead of RIGHT)
		// with LEFT.

		// try to see if the exponent is an integer
		if (itv_right.is_degenerated()) {
			double x=itv_right.mid();
			if (floor(x)==x) {
				right_type=IBEX_INTEGER;
				int_right=(int)floor(x);
			}
		}
	} else
		right_type=IBEX_EXPRNODE;


	if (left.type()==Label::CONST) {
		left_type=IBEX_INTERVAL;
		itv_left=left.domain().i();
		//delete cl; // LEFT will no longer be used // not now (see comment in ExprCopy.h)
	} else
		left_type=IBEX_EXPRNODE;


	if (left_type==IBEX_INTERVAL) {
		if (right_type==IBEX_INTEGER) {
			e.lab=new LabelConst(pow(itv_left,int_right));
		} else if (right_type==IBEX_INTERVAL) {
			e.lab=new LabelConst(pow(itv_left,itv_right));
		} else {
			e.lab=new LabelNode(&exp(right.node() * log(itv_left))); // *log(...) will create a new ExprConstant.
		}
	}  else {
		if (right_type==IBEX_INTEGER) {
			e.lab=new LabelNode(&pow(left.node(),int_right));
		} else if (right_type==IBEX_INTERVAL) { // do not forget this case (RIGHT does not exist anymore)
			e.lab=new LabelNode(&exp(itv_right * log(left.node())));
		} else {
			e.lab=new LabelNode(&exp(right.node() * log(left.node())));
		}
	}
}

pair<int,int> ExprGenerator::visit_index_tmp(const Dim& dim, const P_ExprNode& idx, bool matlab_style) {
	int i1,i2;

	switch(idx.op) {
	case P_ExprNode::IDX_ALL :
		i1=i2=-1;
		break;
	case P_ExprNode::IDX_RANGE :
		visit(idx.arg[0]);
		visit(idx.arg[1]);
		i1=to_integer(idx.arg[0].lab->domain());
		i2=to_integer(idx.arg[1].lab->domain());
		if (matlab_style) { i1--; i2--; }
		break;
	case P_ExprNode::IDX :
		visit(idx.arg[0]);
		i1=i2=i1=to_integer(idx.arg[0].lab->domain());
		if (matlab_style) { i1--; i2--; }
		break;
	default:
		assert(false);
	}
	if (i1<0 || i2<0)
		throw SyntaxError("negative index. Note: indices in Matlab-style (using parenthesis like in \"x(i)\") start from 1 (not 0).");

	return pair<int,int>(i1,i2);
}

DoubleIndex ExprGenerator::visit_index(const Dim& dim, const P_ExprNode& idx1, bool matlab_style) {

	pair<int,int> p=visit_index_tmp(dim,idx1,matlab_style);

	// If only one index is supplied
	// it may corresponds to a row number or
	// a column number (in case of row vector).
	if (p.first==-1) {
		return DoubleIndex::all(dim);
	} else if (p.second==p.first) {
		if (dim.is_matrix())
			return DoubleIndex::one_row(dim,p.first);
		// A single index i with a row vector
		// gives the ith column.
		else
			return DoubleIndex::one_col(dim,p.first);
	} else {
		if (dim.is_matrix())
			return DoubleIndex::rows(dim,p.first,p.second);
		else
			return DoubleIndex::cols(dim,p.first,p.second);
	}
}

DoubleIndex ExprGenerator::visit_index(const Dim& dim, const P_ExprNode& idx1, const P_ExprNode& idx2, bool matlab_style) {

	pair<int,int> r=visit_index_tmp(dim,idx1,matlab_style);
	pair<int,int> c=visit_index_tmp(dim,idx2,matlab_style);

	if (r.first==-1) {
		if (c.first==-1)
			return DoubleIndex::all(dim);
		else if (c.first==c.second)
			return DoubleIndex::cols(dim,c.first,c.second);
		else
			return DoubleIndex::one_col(dim,c.first);
	} else if (r.first==r.second) {
		if (c.first==-1)
			return DoubleIndex::one_row(dim,r.first);
		else if (c.first==c.second)
			return DoubleIndex::one_elt(dim,r.first,c.first);
		else
			return DoubleIndex::subrow(dim,r.first,c.first,c.second);
	} else {
		if (c.first==-1)
			return DoubleIndex::rows(dim,r.first,r.second);
		else if (c.first==c.second)
			return DoubleIndex::subcol(dim,r.first,r.second,c.first);
		else
			return DoubleIndex::submatrix(dim,r.first,r.second,c.first,c.second);
	}
}

void ExprGenerator::visit(const P_ExprIndex& e) {
	visit(e.arg[0]);

	Label& expr=(*(e.arg[0].lab));

	DoubleIndex idx=e.arg.size()==2?
			visit_index(expr.node().dim, e.arg[1], e.matlab_style)
			:
			visit_index(expr.node().dim, e.arg[1], e.arg[2], e.matlab_style);

	if (expr.type()==Label::CONST) {
		e.lab = new LabelConst(expr.domain()[idx]); // TODO: should be ref?
	} else {
		e.lab = new LabelNode(new ExprIndex(expr.node(),idx));
	}
}

void ConstantGenerator::visit(const P_ExprSymbol& x) {
	if (x.type!=P_ExprSymbol::SYB) {
		// only possible if called from generate_cst(..)
		stringstream s;
		s << "Cannot use symbol\"" << x.name << "\" inside a constant expression";
		throw SyntaxError(s.str());
	} else {
		x.lab = new LabelConst(x.domain); // TOOD: reference
	}
}

} // end namespace parser
} // end namespace ibex
