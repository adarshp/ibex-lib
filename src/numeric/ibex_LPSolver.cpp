//============================================================================
//                                  I B E X
// Interface with the linear solver
// File        : ibex_LPSolver.cpp
// Author      : Jordan Ninin
// License     : See the LICENSE file
// Created     : May 15, 2013
// Last Update : May 15, 2013
//============================================================================

#include  <cfloat>
#include "ibex_LPSolver.h"

namespace ibex {

/** \brief Stream out \a x. */
std::ostream& operator<<(std::ostream& os, const LPSolver::Status_Sol x){

	switch(x) {
	case(LPSolver::OPTIMAL) :{
			os << "OPTIMAL";
			break;
	}
	case(LPSolver::INFEASIBLE) :{
			os << "INFEASIBLE";
			break;
	}
	case(LPSolver::OPTIMAL_PROVED) :{
			os << "OPTIMAL_PROVED";
			break;
	}
	case(LPSolver::INFEASIBLE_PROVED) :{
			os << "INFEASIBLE_PROVED";
			break;
	}
	case(LPSolver::TIME_OUT) :{
			os << "TIME_OUT";
			break;
	}
	case(LPSolver::MAX_ITER) :{
			os << "MAX_ITER";
			break;
	}
	case(LPSolver::UNKNOWN) :{
		os << "UNKNOWN";
		break;
	}
	}
	return os;

}

LPSolver::Status_Sol LPSolver::solve_proved() {
	LPSolver::Status_Sol stat = solve();
	switch (stat) {
	case LPSolver::OPTIMAL : {
		// Neumaier - Shcherbina postprocessing
		Interval obj = neumaier_shcherbina_postprocessing();
		obj_value = Interval(obj.lb(),obj_value.ub());
		stat = LPSolver::OPTIMAL_PROVED;
		break;
	}
	case  LPSolver::INFEASIBLE: {
		// infeasibility test  cf Neumaier Shcherbina paper
		if (neumaier_shcherbina_infeasibilitytest()) {
			stat = LPSolver::INFEASIBLE_PROVED;
		}
		break;
	}
	default:
		stat = LPSolver::UNKNOWN;
		break;
	}
	return stat;

}

int LPSolver::get_nb_rows() const {
	return nb_rows;
}

Interval LPSolver::get_obj_value() const {
	return obj_value;
}

void LPSolver::add_constraint(const Matrix & A, CmpOp sign, const Vector& rhs ) {
	for (int i=0; i<A.nb_rows(); i++) {
		try {
			add_constraint(A[i],sign,rhs[i]);
		} catch (LPException&) { }
	}
}


void LPSolver::clean_bounds() {
	set_bounds(IntervalVector(nb_vars));
}

void LPSolver::clean_obj() {
	set_obj(ibex::Vector::zeros(nb_vars));
}

void LPSolver::clean_all() {
	clean_ctrs();
	clean_obj();
	clean_bounds();
}

ibex::Vector LPSolver::get_primal_sol() const {
	try {
		if (status_prim) {
			return primal_solution;
		} else
			throw LPException();
	}
	catch(...) {
		throw LPException();
	}
}

ibex::Vector LPSolver::get_dual_sol() const {
	try {
		if (status_dual) {
			return dual_solution;
		} else
			throw LPException();
	}
	catch(...) {
		throw LPException();
	}
}

///////////////////////////////////////////////////////////////////////////////////

LPSolver::Status_Sol LPSolver::solve_var(LPSolver::Sense s, int var, Interval& obj) {
	assert((0<=var)&&(var<=nb_vars));

	LPSolver::Status_Sol stat = LPSolver::UNKNOWN;	
	set_sense(LPSolver::MINIMIZE);


	try {
		// the linear solver is always called in a minimization mode : in case of maximization of var , the opposite of var is minimized
		if(s==LPSolver::MINIMIZE)
			set_obj_var(var, 1.0);
		else
			set_obj_var(var, -1.0);

		//	mylinearsolver->write_file("coucou.lp");
		//	system("cat coucou.lp");
		stat = solve();
		// std::cout << "[polytope-hull]->[run_simplex] solver returns " << stat << std::endl;
		switch (stat) {
		case LPSolver::OPTIMAL : {
			// Neumaier - Shcherbina postprocessing
			obj_value = neumaier_shcherbina_postprocessing_var(var, s);
			obj = obj_value;
			stat = LPSolver::OPTIMAL_PROVED;
			break;
		}
		case  LPSolver::INFEASIBLE: {
			// infeasibility test  cf Neumaier Shcherbina paper
			if (neumaier_shcherbina_infeasibilitytest()) {
				stat = LPSolver::INFEASIBLE_PROVED;
			}
			break;
		}
		default:
			stat = LPSolver::UNKNOWN;
			break;
		}
		// Reset the objective of the LP solver
		set_obj_var(var, 0.0);
	}
	catch (LPException&) {
		stat = LPSolver::UNKNOWN;
		// Reset the objective of the LP solver
		set_obj_var(var, 0.0);
	}

	return stat;

}

Interval LPSolver::neumaier_shcherbina_postprocessing_var (int var, LPSolver::Sense s) {
	try {
		// the dual solution : used to compute the bound
		ibex::Vector dual = get_dual_sol();

		ibex::Matrix A_trans = get_rows_trans();

		IntervalVector B = get_lhs_rhs();


		//cout <<" BOUND_test "<< endl;
		IntervalVector Rest(nb_vars);
		IntervalVector Lambda(dual);
		//		std::cout << " A_t " << A_trans << std::endl;
		//		std::cout << " B " << B << std::endl;
		//		std::cout << " dual " << Lambda << std::endl;
		//		std::cout << " box " << boundvar << std::endl;
		//		std::cout << " dual B " << Lambda * B << std::endl;


		Rest = A_trans * Lambda ;   // Rest = Transpose(As) * Lambda
		if (s==LPSolver::MINIMIZE) {
			Rest[var] -=1; // because C is a vector of zero except for the coef "var"
			return (Lambda * B - Rest * boundvar);
		} else {
			Rest[var] +=1;
			return -(Lambda * B - Rest * boundvar);
		}

		//cout << " Rest " << Rest << endl;
		//cout << " dual " << Lambda << endl;
		//cout << " dual B " << Lambda * B << endl;
		//cout << " rest box " << Rest * box  << endl;


	} catch (...) {
		throw LPException();
	}
}


Interval LPSolver::neumaier_shcherbina_postprocessing() {
	try {
		// the dual solution : used to compute the bound
		ibex::Vector dual = get_dual_sol();

		ibex::Matrix A_trans = get_rows_trans();

		IntervalVector B = get_lhs_rhs();

		ibex::Vector obj = get_coef_obj();


		//cout <<" BOUND_test "<< endl;
		IntervalVector Rest(nb_vars);
		IntervalVector Lambda(dual);
		Rest = A_trans * Lambda ;
		Rest -= obj;   // Rest = Transpose(As) * Lambda - obj
		return (Lambda * B - Rest * boundvar);

		Rest = A_trans * Lambda ;   // Rest = Transpose(As) * Lambda
		if (sense==LPSolver::MINIMIZE) {
			Rest -= obj;   // Rest = Transpose(As) * Lambda - obj
			return (Lambda * B - Rest * boundvar);
		} else {
			Rest += obj;   // Rest = Transpose(As) * Lambda - obj
			return -(Lambda * B - Rest * boundvar);
		}

	} catch (...) {
		throw LPException();
	}
}


bool LPSolver::neumaier_shcherbina_infeasibilitytest() {
	try {
		ibex::Vector infeasible_dir = get_infeasible_dir();

		ibex::Matrix A_trans = get_rows_trans();

		IntervalVector B = get_lhs_rhs();

		IntervalVector Lambda(infeasible_dir);

		IntervalVector Rest(nb_vars);
		Rest = A_trans * Lambda ;
		Interval d= Rest * boundvar - Lambda * B;

		// if 0 does not belong to d, the infeasibility is proved

		if (d.contains(0.0))
			return false;
		else
			return true;

	} catch (LPException&) {
		return false;
	}

}
}
 // end namespace ibex
