#include "Multiscale.hpp"

#include <polyfem/basis/Basis.hpp>
#include <polyfem/autogen/auto_elasticity_rhs.hpp>

#include <polyfem/utils/MatrixUtils.hpp>
#include <polyfem/utils/Timer.hpp>
#include <polyfem/State.hpp>
#include <polyfem/utils/Logger.hpp>
#include <polyfem/utils/par_for.hpp>
#include <polyfem/utils/MaybeParallelFor.hpp>
#include <polyfem/io/Evaluator.hpp>
#include <polyfem/io/MatrixIO.hpp>

#include <polyfem/solver/NLProblem.hpp>
#include <polyfem/solver/NonlinearSolver.hpp>
#include <polyfem/solver/SparseNewtonDescentSolver.hpp>
#include <polyfem/solver/DenseNewtonDescentSolver.hpp>

#include <unsupported/Eigen/MatrixFunctions>
#include <finitediff.hpp>
#include <filesystem>

#ifdef POLYSOLVE_WITH_SPECTRA
#include <SymEigsSolver.h>
#endif

namespace polyfem::assembler
{
	namespace {

		bool delta(int i, int j)
		{
			return i == j;
		}

		bool compare_matrix(
			const Eigen::MatrixXd& x,
			const Eigen::MatrixXd& y,
			const double test_eps = 1e-4)
		{
			assert(x.rows() == y.rows());

			bool same = true;
			double scale = std::max(x.norm(), y.norm());
			double error = (x - y).norm();
			
			// std::cout << "error: " << error << " scale: " << scale << "\n";

			if (error > scale * test_eps)
				same = false;

			return same;
		}

		Eigen::MatrixXd generate_linear_field(const State &state, const Eigen::MatrixXd &grad)
		{
			const int problem_dim = grad.rows();
			const int dim = state.mesh->dimension();
			assert(dim == grad.cols());

			Eigen::MatrixXd func(state.n_bases * problem_dim, 1);
			func.setZero();

			for (int i = 0; i < state.n_bases; i++)
			{
				func.block(i * problem_dim, 0, problem_dim, 1) = grad * state.mesh_nodes->node_position(i).transpose();
			}

			return func;
		}

		class LocalThreadVecStorage
		{
		public:
			Eigen::MatrixXd vec;
			assembler::ElementAssemblyValues vals;
			QuadratureVector da;

			LocalThreadVecStorage(const int size)
			{
				vec.resize(size, 1);
				vec.setZero();
			}
		};
	}

	Multiscale::Multiscale()
	{
	}

	Multiscale::~Multiscale()
	{
		
	}

	double Multiscale::homogenize_energy(const Eigen::MatrixXd &x) const
	{
		const auto &bases = state->bases;
		const auto &gbases = state->geom_bases();

		return state->assembler.assemble_energy(state->formulation(), size() == 3, bases, gbases, state->ass_vals_cache, 0, x, x) / microstructure_volume;
	}

	Eigen::MatrixXd Multiscale::homogenize_def_grad(const Eigen::MatrixXd &x) const
	{
		const int dim = state->mesh->dimension();
		Eigen::VectorXd avgs;
		avgs.setZero(dim * dim);
		for (int e = 0; e < state->bases.size(); e++)
		{
			assembler::ElementAssemblyValues vals;
			state->ass_vals_cache.compute(e, dim == 3, state->bases[e], state->geom_bases()[e], vals);

			Eigen::MatrixXd u, grad_u;
			io::Evaluator::interpolate_at_local_vals(e, dim, dim, vals, x, u, grad_u);

			const quadrature::Quadrature &quadrature = vals.quadrature;
			Eigen::VectorXd da = quadrature.weights * vals.det;
			avgs += grad_u.transpose() * da;
		}
		avgs /= microstructure_volume;

		return utils::unflatten(avgs, dim);
	}

	void Multiscale::homogenize_stress(const Eigen::MatrixXd &x, Eigen::MatrixXd &stress) const
	{
		const auto &bases = state->bases;
		const auto &gbases = state->geom_bases();

		stress.setZero(size(), size());

		auto storage = utils::create_thread_storage(LocalThreadVecStorage(stress.size()));

		utils::maybe_parallel_for(bases.size(), [&](int start, int end, int thread_id) {
			LocalThreadVecStorage &local_storage = utils::get_local_thread_storage(storage, thread_id);
			Eigen::MatrixXd stresses, avg_stress, tmp;

			for (int e = start; e < end; ++e)
			{
				assembler::ElementAssemblyValues &vals = local_storage.vals;
				state->ass_vals_cache.compute(e, size() == 3, bases[e], gbases[e], vals);

				const quadrature::Quadrature &quadrature = vals.quadrature;
				local_storage.da = vals.det.array() * quadrature.weights.array();

				state->assembler.compute_tensor_value(state->formulation(), e, bases[e], gbases[e], quadrature.points, x, stresses);
				tmp = stresses.transpose() * local_storage.da;
				local_storage.vec += tmp;
				// avg_stress = Eigen::Map<Eigen::MatrixXd>(tmp.data(), size(), size());
				// stress += avg_stress;
			}
		});

		for (const LocalThreadVecStorage &local_storage : storage)
			for (int i = 0; i < stress.size(); i++)
				stress(i) += local_storage.vec(i);

		stress /= microstructure_volume;
	}

	void Multiscale::homogenize_stiffness(const Eigen::MatrixXd &x, Eigen::MatrixXd &stiffness) const
	{
		double time;
		POLYFEM_SCOPED_TIMER("homogenize variables", time);

		const auto &bases = state->bases;
		const auto &gbases = state->geom_bases();

		Eigen::MatrixXd avg_stiffness;
		avg_stiffness.setZero(size()*size(), size()*size());

		Eigen::MatrixXd stiffnesses;
		Eigen::MatrixXd CB;
		CB.setZero(size()*size(), state->ndof());
		for (int e = 0; e < bases.size(); ++e)
		{
			assembler::ElementAssemblyValues vals;
			state->ass_vals_cache.compute(e, size() == 3, bases[e], gbases[e], vals);

			const quadrature::Quadrature &quadrature = vals.quadrature;
			Eigen::VectorXd da = vals.det.array() * quadrature.weights.array();

			state->assembler.compute_stiffness_value(state->formulation(), vals, quadrature.points, x, stiffnesses);
			avg_stiffness += utils::unflatten(stiffnesses.transpose() * da, size()*size());

			for (int i = 0; i < vals.basis_values.size(); i++)
			{
				const auto &v = vals.basis_values[i];
				assert(v.global.size() == 1);
				for (int a = 0; a < size(); a++)
				for (int b = 0; b < size(); b++)
				for (int k = 0; k < size(); k++)
				for (int l = 0; l < size(); l++)
				{
					int X = a * size() + b;
					int Y = k * size() + l;
					CB(X, v.global[0].index * size() + k) += (stiffnesses.col(X * size() * size() + Y).array() * v.grad_t_m.col(l).array() * da.array()).sum();
				}
			}
		}

		// compute term2 given CB
		Eigen::MatrixXd term2;
		term2.setZero(CB.cols(), CB.rows());
		if (!state->solve_data.nl_problem)
			log_and_throw_error("Need nl problem to homogenize stiffness!");
		if (!state->diff_cached.size())
			log_and_throw_error("Need differentiability of micro state!");

		for (int i = 0; i < CB.rows(); i++)
		{
			Eigen::VectorXd b = CB.row(i);
			state->solve_adjoint(b);
			term2.col(i) = state->diff_cached[0].p;
		}

		stiffness = (avg_stiffness - CB * term2) / microstructure_volume;
	}

	void Multiscale::homogenization(const Eigen::MatrixXd &def_grad, double &energy, Eigen::MatrixXd &stress, Eigen::MatrixXd &stiffness) const
	{
		Eigen::MatrixXd x;
		{
			double time;
			POLYFEM_SCOPED_TIMER("micro newton", time);
			Eigen::MatrixXd disp_grad = def_grad - Eigen::MatrixXd::Identity(size(), size());

			state->disp_offset.setZero(state->ndof(), 1);
			for (int i = 0; i < state->n_bases; i++)
				state->disp_offset.block(i * state->mesh->dimension(), 0, state->mesh->dimension(), 1) = disp_grad * state->mesh_nodes->node_position(i).transpose();

			Eigen::MatrixXd pressure;
			state->solve_problem(x, pressure);
		}

		// effective energy = average energy over unit cell
		energy = homogenize_energy(x);

		// effective stress = average stress over unit cell
		homogenize_stress(x, stress);

		homogenize_stiffness(x, stiffness);
	}

	void Multiscale::homogenization(const Eigen::MatrixXd &def_grad, double &energy, Eigen::MatrixXd &stress) const
	{
		Eigen::MatrixXd x;
		{
			double time;
			POLYFEM_SCOPED_TIMER("micro newton", time);
			Eigen::MatrixXd disp_grad = def_grad - Eigen::MatrixXd::Identity(size(), size());

			state->disp_offset.setZero(state->ndof(), 1);
			for (int i = 0; i < state->n_bases; i++)
				state->disp_offset.block(i * state->mesh->dimension(), 0, state->mesh->dimension(), 1) = disp_grad * state->mesh_nodes->node_position(i).transpose();

			Eigen::MatrixXd pressure;
			state->solve_problem(x, pressure);
		}

		// effective energy = average energy over unit cell
		energy = homogenize_energy(x);

		// effective stress = average stress over unit cell
		homogenize_stress(x, stress);
	}

	void Multiscale::homogenization(const Eigen::MatrixXd &def_grad, double &energy) const
	{
		Eigen::MatrixXd x;
		{
			double time;
			POLYFEM_SCOPED_TIMER("micro newton", time);
			Eigen::MatrixXd disp_grad = def_grad - Eigen::MatrixXd::Identity(size(), size());

			state->disp_offset.setZero(state->ndof(), 1);
			for (int i = 0; i < state->n_bases; i++)
				state->disp_offset.block(i * state->mesh->dimension(), 0, state->mesh->dimension(), 1) = disp_grad * state->mesh_nodes->node_position(i).transpose();

			Eigen::MatrixXd pressure;
			state->solve_problem(x, pressure);
		}

		// effective energy = average energy over unit cell
		energy = homogenize_energy(x);
	}

	void Multiscale::set_microstructure_state(const std::shared_ptr<polyfem::State> state_ptr)
	{
		state = state_ptr;

		RowVectorNd min, max;
		state->mesh->bounding_box(min, max);
		microstructure_volume = (max - min).prod();
	}

	void Multiscale::add_multimaterial(const int index, const json &params)
	{
		assert(size_ == 2 || size_ == 3);

		if (params.contains("type") && params["type"] == name())
		{
			json unit_cell_args = params["microstructure"];

			{
				state = std::make_shared<polyfem::State>(utils::get_n_threads(), true);
				state->init(unit_cell_args, false, "", false);
				state->load_mesh(false);
				if (state->mesh == nullptr)
					log_and_throw_error("No microstructure mesh found!");
				state->stats.compute_mesh_stats(*state->mesh);
				state->build_basis();
				state->assemble_rhs();
				state->assemble_stiffness_mat(true);

				RowVectorNd min, max;
				state->mesh->bounding_box(min, max);
				microstructure_volume = (max - min).prod();
			}
		}
	}

	void Multiscale::set_size(const int size)
	{
		size_ = size;
	}

	Eigen::Matrix<double, Eigen::Dynamic, 1, 0, 3, 1>
	Multiscale::compute_rhs(const AutodiffHessianPt &pt) const
	{
		assert(pt.size() == size());
		Eigen::Matrix<double, Eigen::Dynamic, 1, 0, 3, 1> res;

		double lambda, mu;
		params_.lambda_mu(0, 0, 0, pt(0).getValue(), pt(1).getValue(), size() == 2 ? 0. : pt(2).getValue(), 0, lambda, mu);

		if (size() == 2)
			autogen::neo_hookean_2d_function(pt, lambda, mu, res);
		else if (size() == 3)
			autogen::neo_hookean_3d_function(pt, lambda, mu, res);
		else
			assert(false);

		return res;
	}

	Eigen::VectorXd
	Multiscale::assemble_grad(const NonLinearAssemblerData &data) const
	{
		const auto &bs = data.vals.basis_values;
		Eigen::MatrixXd local_disp;
		local_disp.setZero(bs.size(), size());
		for (size_t i = 0; i < bs.size(); ++i)
		{
			const auto &b = bs[i];
			for (size_t ii = 0; ii < b.global.size(); ++ii)
				for (int d = 0; d < size(); ++d)
					local_disp(i, d) += b.global[ii].val * data.x(b.global[ii].index * size() + d);
		}

		Eigen::MatrixXd G;
		G.setZero(bs.size(), size());

		const int n_pts = data.da.size();
		Eigen::MatrixXd def_grad(size(), size()), stress_tensor;
		for (long p = 0; p < n_pts; ++p)
		{
			Eigen::MatrixXd grad(bs.size(), size());
			for (size_t i = 0; i < bs.size(); ++i)
				grad.row(i) = bs[i].grad.row(p);

			Eigen::MatrixXd delF_delU = grad * data.vals.jac_it[p];

			def_grad.setZero();
			def_grad = local_disp.transpose() * delF_delU + Eigen::MatrixXd::Identity(size(), size());

			double energy = 0;
			homogenization(def_grad, energy, stress_tensor);

			// {
			// 	Eigen::VectorXd fgrad, grad;
			// 	Eigen::VectorXd x0 = utils::flatten(def_grad);
			// 	fd::finite_gradient(
			// 		x0, [this](const Eigen::VectorXd &x) -> double 
			// 		{ 
			// 			Eigen::MatrixXd F = utils::unflatten(x, this->size());
			// 			double val;
			// 			this->homogenization(F, val);
			// 			return val;
			// 		}, fgrad, fd::AccuracyOrder::SECOND, 1e-6);

			// 	grad = utils::flatten(stress_tensor);
			// 	if ((grad.norm() != 0) && !compare_matrix(grad, fgrad))
			// 	{
			// 		std::cout << "Gradient: " << grad.transpose() << std::endl;
			// 		std::cout << "Finite gradient: " << fgrad.transpose() << std::endl;
			// 		logger().error("Gradient mismatch");
			// 	}
			// 	else
			// 	{
			// 		logger().error("Gradient match!");
			// 	}
			// }

			G += delF_delU * stress_tensor.transpose() * data.da(p);
		}

		Eigen::MatrixXd G_T = G.transpose();

		Eigen::VectorXd temp(Eigen::Map<Eigen::VectorXd>(G_T.data(), G_T.size()));

		return temp;
	}

	Eigen::MatrixXd
	Multiscale::assemble_hessian(const NonLinearAssemblerData &data) const
	{
		const auto &bs = data.vals.basis_values;
		Eigen::MatrixXd hessian;
		hessian.setZero(bs.size() * size(), bs.size() * size());
		Eigen::MatrixXd local_disp;
		local_disp.setZero(bs.size(), size());
		for (size_t i = 0; i < bs.size(); ++i)
		{
			const auto &b = bs[i];
			for (size_t ii = 0; ii < b.global.size(); ++ii)
				for (int d = 0; d < size(); ++d)
					local_disp(i, d) += b.global[ii].val * data.x(b.global[ii].index * size() + d);
		}

		const int n_pts = data.da.size();

		Eigen::MatrixXd def_grad(size(), size());
		Eigen::MatrixXd stress_tensor, hessian_temp;
		for (long p = 0; p < n_pts; ++p)
		{
			Eigen::MatrixXd grad(bs.size(), size());

			for (size_t i = 0; i < bs.size(); ++i)
				grad.row(i) = bs[i].grad.row(p);

			Eigen::MatrixXd jac_it = data.vals.jac_it[p];
			
			def_grad = local_disp.transpose() * grad * jac_it + Eigen::MatrixXd::Identity(size(), size());

			double energy = 0;
			homogenization(def_grad, energy, stress_tensor, hessian_temp);

			{
				Eigen::MatrixXd hessian_temp2 = hessian_temp;
				for (int i = 0; i < size(); i++)
				for (int j = 0; j < size(); j++)
				for (int k = 0; k < size(); k++)
				for (int l = 0; l < size(); l++)
					hessian_temp(i + j * size(), k + l * size()) = hessian_temp2(i * size() + j, k * size() + l);
			}

			Eigen::MatrixXd delF_delU_tensor(jac_it.size(), grad.size());
			Eigen::MatrixXd temp;
			for (size_t j = 0; j < local_disp.cols(); ++j)
			{
				temp.setZero(size(), size());
				for (size_t i = 0; i < local_disp.rows(); ++i)
				{
					temp.row(j) = grad.row(i);
					temp = temp * jac_it;
					Eigen::VectorXd temp_flattened(Eigen::Map<Eigen::VectorXd>(temp.data(), temp.size()));
					delF_delU_tensor.col(i * size() + j) = temp_flattened;
				}
			}

			hessian += delF_delU_tensor.transpose() * hessian_temp * delF_delU_tensor * data.da(p);
		}

		return hessian;
	}

	void Multiscale::compute_stress_tensor(const int el_id, const basis::ElementBases &bs, const basis::ElementBases &gbs, const Eigen::MatrixXd &local_pts, const Eigen::MatrixXd &displacement, Eigen::MatrixXd &stresses) const
	{
		assign_stress_tensor(el_id, bs, gbs, local_pts, displacement, size() * size(), stresses, [&](const Eigen::MatrixXd &stress) {
			Eigen::MatrixXd tmp = stress;
			auto a = Eigen::Map<Eigen::MatrixXd>(tmp.data(), 1, size() * size());
			return Eigen::MatrixXd(a);
		});
	}

	void Multiscale::compute_von_mises_stresses(const int el_id, const basis::ElementBases &bs, const basis::ElementBases &gbs, const Eigen::MatrixXd &local_pts, const Eigen::MatrixXd &displacement, Eigen::MatrixXd &stresses) const
	{
		assign_stress_tensor(el_id, bs, gbs, local_pts, displacement, 1, stresses, [&](const Eigen::MatrixXd &stress) {
			Eigen::Matrix<double, 1, 1> res;
			res.setConstant(von_mises_stress_for_stress_tensor(stress));
			return res;
		});
	}

	void Multiscale::assign_stress_tensor(const int el_id, const basis::ElementBases &bs, const basis::ElementBases &gbs, const Eigen::MatrixXd &local_pts, const Eigen::MatrixXd &displacement, const int all_size, Eigen::MatrixXd &all, const std::function<Eigen::MatrixXd(const Eigen::MatrixXd &)> &fun) const
	{
		Eigen::MatrixXd displacement_grad(size(), size());

		assert(displacement.cols() == 1);

		all.resize(local_pts.rows(), all_size);

		ElementAssemblyValues vals;
		vals.compute(el_id, size() == 3, local_pts, bs, gbs);

		for (long p = 0; p < local_pts.rows(); ++p)
		{
			compute_diplacement_grad(size(), bs, vals, local_pts, p, displacement, displacement_grad);

			const Eigen::MatrixXd def_grad = Eigen::MatrixXd::Identity(size(), size()) + displacement_grad;

			Eigen::MatrixXd stress_tensor, stiffness_tensor;
			// double energy = 0;
			// homogenization(def_grad, energy, stress_tensor, stiffness_tensor);
			stress_tensor.setZero(size(),size());

			all.row(p) = fun(stress_tensor);
		}
	}

	double Multiscale::compute_energy(const NonLinearAssemblerData &data) const
	{
			Eigen::MatrixXd local_disp;
			local_disp.setZero(data.vals.basis_values.size(), size());
			for (size_t i = 0; i < data.vals.basis_values.size(); ++i)
			{
				const auto &bs = data.vals.basis_values[i];
				for (size_t ii = 0; ii < bs.global.size(); ++ii)
					for (int d = 0; d < size(); ++d)
						local_disp(i, d) += bs.global[ii].val * data.x(bs.global[ii].index * size() + d);
			}
			
			double energy = 0;
			const int n_pts = data.da.size();

			Eigen::MatrixXd def_grad(size(), size());
			for (long p = 0; p < n_pts; ++p)
			{
				def_grad.setZero();

				for (size_t i = 0; i < data.vals.basis_values.size(); ++i)
				{
					const auto &bs = data.vals.basis_values[i];

					for (int d = 0; d < size(); ++d)
						for (int c = 0; c < size(); ++c)
							def_grad(d, c) += bs.grad(p, c) * local_disp(i, d);
				}

				Eigen::MatrixXd jac_it(size(), size());
				for (long k = 0; k < jac_it.size(); ++k)
					jac_it(k) = data.vals.jac_it[p](k);
				
				def_grad = def_grad * jac_it + Eigen::MatrixXd::Identity(size(), size());
				
				double val = 0;
				homogenization(def_grad, val);

				energy += val * data.da(p);
			}

			return energy;
	}
} // namespace polyfem::assembler
