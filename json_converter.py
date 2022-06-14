import json
import sys

def copy_entry(key, f, t):
    if (key in f):
        t[key] = f[key]


def rename_entry(key, f, key2, t):
    if (key in f):
        t[key2] = f[key]


def PolyFEM_convert(old):

    j = {}
    rename_entry("default_params", old, "defaults", j)
    copy_entry("root_path",old,j)

    # Meshes to Geometry
    j["geometry"] = []
    if "meshes" in old:
        for o in old["meshes"]:
            n = {}
            copy_entry("type",o,n)
            copy_entry("mesh",o,n)
            n["is_obstacle"] = False
            copy_entry("enabled",o,n)

            # Transformation
            n["transformation"] = {}
            rename_entry("position",o,"translation",n["transformation"])
            copy_entry("rotation",o,n["transformation"])
            copy_entry("rotation_mode",o,n["transformation"])
            copy_entry("scale",o,n["transformation"])
            copy_entry("dimensions",o,n["transformation"])

            rename_entry("body_id",o,"volume_selection",n)
            rename_entry("boundary_id",o,"surface_selection",n)

            copy_entry("n_refs",old,n)
            n["advanced"] = {}
            copy_entry("force_linear_geometry",old,n["advanced"])
            copy_entry("refinement_location",old,n["advanced"])
            copy_entry("normalize_mesh",old,n["advanced"])
            copy_entry("min_component",old,n["advanced"])

            j["geometry"].append(n)

    # Obstacles to Geometry
    # if "obstacles" in old:
    #     for o in old["obstacles"]:
    #         n = {}
    #         copy_entry("type",o,n)
    #         copy_entry("mesh",o,n)
    #         n["is_obstacle"] = True
    #         copy_entry("enabled",o,n)

    #         # Transformation
    #         n["transformation"] = {}
    #         rename_entry("position",o,"translation",n["transformation"])
    #         copy_entry("rotation",o,n["transformation"])
    #         copy_entry("rotation_mode",o,n["transformation"])
    #         copy_entry("scale",o,n["transformation"])
    #         copy_entry("dimensions",o,n["transformation"])

    #         rename_entry("body_id",o,"volume_selection",n)
    #         rename_entry("boundary_id",o,"surface_selection",n)

    #         copy_entry("n_refs",old,n)
    #         n["advanced"] = {}
    #         copy_entry("force_linear_geometry",old,n["advanced"])
    #         copy_entry("refinement_location",old,n["advanced"])
    #         copy_entry("normalize_mesh",old,n["advanced"])
    #         copy_entry("min_component",old,n["advanced"])

    #         j["geometry"].append(n)

    # Space
    j["space"] = {}
    copy_entry("discr_order",old,j["space"])
    copy_entry("pressure_discr_order",old,j["space"])
    copy_entry("use_p_ref",old,j["space"])

    j["space"]["advanced"] = {}

    copy_entry("particle",old,j["space"]["advanced"])
    copy_entry("discr_order_max",old,j["space"]["advanced"])
    copy_entry("serendipity",old,j["space"]["advanced"])
    copy_entry("isoparametric",old,j["space"]["advanced"])
    copy_entry("use_spline",old,j["space"]["advanced"])
    copy_entry("bc_method",old,j["space"]["advanced"])
    copy_entry("n_boundary_samples",old,j["space"]["advanced"])
    copy_entry("poly_bases",old,j["space"]["advanced"])
    copy_entry("quadrature_order",old,j["space"]["advanced"])
    copy_entry("integral_constraints",old,j["space"]["advanced"])
    copy_entry("n_harmonic_samples",old,j["space"]["advanced"])
    copy_entry("force_no_ref_for_harmonic",old,j["space"]["advanced"])
    copy_entry("B",old,j["space"]["advanced"])
    copy_entry("h1_formula",old,j["space"]["advanced"])
    copy_entry("count_flipped_els",old,j["space"]["advanced"])

    # Time
    j["time"] = {}
    copy_entry("t0",old,j["time"])
    copy_entry("tend",old,j["time"])
    copy_entry("dt",old,j["time"])
    copy_entry("time_steps",old,j["time"])
    copy_entry("integrator",old,j["time"])

    if "time_integrator_params" in old:
        j["time"]["newmark"] = {}
        j["time"]["BDF"] = {}
        copy_entry("gamma",old["time_integrator_params"],j["time"]["newmark"])
        copy_entry("beta",old["time_integrator_params"],j["time"]["newmark"])
        rename_entry("num_steps",old["time_integrator_params"],"steps",j["time"]["BDF"])

    # Contact

    j["contact"] = {}
    rename_entry("has_collision",old,"enabled",j["contact"])
    copy_entry("dhat_percentage",old,j["contact"])
    copy_entry("dhat",old,j["contact"])
    copy_entry("epsv",old,j["contact"])
    copy_entry("coeff_friction",old,j["contact"])

    # Solver

    j["solver"] = {}
    j["solver"]["linear"] = {}

    rename_entry("solver_type",old,"solver",j["solver"]["linear"])
    rename_entry("precond_type",old,"precond",j["solver"]["linear"])

    j["solver"]["nonlinear"] ={}
    j["solver"]["nonlinear"]["line_search"] ={}

    rename_entry("nl_solver",old,"solver",j["solver"]["nonlinear"])
    if "solver_params" in old:
        rename_entry("fDelta",old["solver_params"],"f_delta",j["solver"]["nonlinear"])
        rename_entry("gradNorm",old["solver_params"],"grad_norm",j["solver"]["nonlinear"])
        rename_entry("nl_iterations",old["solver_params"],"max_iterations",j["solver"]["nonlinear"])
        rename_entry("useGradNorm",old["solver_params"],"use_grad_norm",j["solver"]["nonlinear"])
        rename_entry("relativeGradient",old["solver_params"],"relative_gradient",j["solver"]["nonlinear"])
        rename_entry("use_grad_norm_tol",old["solver_params"],"use_grad_norm_tol",j["solver"]["nonlinear"]["line_search"])

    rename_entry("line_search",old,"line_search",j["solver"]["nonlinear"]["line_search"])

    j["solver"]["augmented_lagrangian"] = {}
    rename_entry("force_al",old,"force",j["solver"]["augmented_lagrangian"])
    rename_entry("al_weight",old,"initial_weight",j["solver"]["augmented_lagrangian"])
    rename_entry("max_al_weight",old,"max_weight",j["solver"]["augmented_lagrangian"])

    j["solver"]["contact"] = {}

    copy_entry("friction_iterations",old,j["solver"]["contact"])
    copy_entry("friction_convergence_tol",old,j["solver"]["contact"])
    copy_entry("barrier_stiffness",old,j["solver"]["contact"])
    copy_entry("lagged_damping_weight",old,j["solver"]["contact"])

    if "solver_params" in old:
        j["solver"]["contact"]["CCD"] = {}
        rename_entry("broad_phase_method",old["solver_params"],"broad_phase",j["solver"]["contact"]["CCD"])
        rename_entry("ccd_tolerance",old["solver_params"],"tolerance",j["solver"]["contact"]["CCD"])
        rename_entry("ccd_max_iterations",old["solver_params"],"max_iterations",j["solver"]["contact"]["CCD"])

    copy_entry("ignore_inertia",old,j["solver"])

    j["solver"]["advanced"] = {}

    copy_entry("cache_size",old,j["solver"]["advanced"])
    copy_entry("lump_mass_matrix",old,j["solver"]["advanced"])

    if "problem" in old:
        if old["problem"] == "GenericScalar" or old["problem"] == "GenericTensor":
            if "problem_params" in old:
                j["boundary_conditions"] = {}
                copy_entry("rhs",old["problem_params"],j["boundary_conditions"])
                copy_entry("dirichlet_boundary",old["problem_params"],j["boundary_conditions"])
                copy_entry("neumann_boundary",old["problem_params"],j["boundary_conditions"])
                copy_entry("pressure_boundary",old["problem_params"],j["boundary_conditions"])

                j["initial_conditions"] = {}
                rename_entry("initial_solution",old["problem_params"],"solution",j["initial_conditions"])
                rename_entry("initial_velocity",old["problem_params"],"velocity",j["initial_conditions"])
                rename_entry("initial_acceleration",old["problem_params"],"acceleration",j["initial_conditions"])

        else:
            rename_entry("problem_params",old,"preset_problem",j)
            j["preset_problem"]["name"] = old["problem"]

    # Materials


    material_name = "NeoHookean"
    if "scalar_formulation" in old:
        material_name = old["scalar_formulation"]
    elif "tensor_formulation" in old:
        material_name = old["tensor_formulation"]
    else:
        print("Warning using default material name:",material_name)

    if "params" in old:
        j["materials"] = {}
        j["materials"]["type"] = material_name
        copy_entry("lambda",old["params"],j["materials"])
        copy_entry("mu",old["params"],j["materials"])
        copy_entry("k",old["params"],j["materials"])
        copy_entry("elasticity_tensor",old["params"],j["materials"])
        copy_entry("E",old["params"],j["materials"])
        copy_entry("nu",old["params"],j["materials"])
        copy_entry("young",old["params"],j["materials"])
        copy_entry("poisson",old["params"],j["materials"])
        copy_entry("density",old["params"],j["materials"])
        copy_entry("rho",old["params"],j["materials"])
        copy_entry("alphas",old["params"],j["materials"])
        copy_entry("mus",old["params"],j["materials"])
        copy_entry("Ds",old["params"],j["materials"])

    if "body_params" in old:
        j["materials"] = []
        for o in old["body_params"]:
            n = {}
            copy_entry("id",o,n)
            copy_entry("E",o,n)
            copy_entry("nu",o,n)
            copy_entry("rho",o,n)
            n["type"] = material_name
            j["materials"].append(n)

    # Output

    j["output"] = {}

    rename_entry("output",old,"json",j["output"])
    if "export" in old:
        j["output"]["paraview"] = {}
        rename_entry("paraview",old["export"],"file_name",j["output"]["paraview"])

        j["output"]["data"] = {}
        copy_entry("solution",old["export"],j["output"]["data"])
        copy_entry("full_mat",old["export"],j["output"]["data"])
        copy_entry("stiffness_mat",old["export"],j["output"]["data"])
        copy_entry("solution_mat",old["export"],j["output"]["data"])
        copy_entry("stress_mat",old["export"],j["output"]["data"])
        copy_entry("u_path",old["export"],j["output"]["data"])
        copy_entry("v_path",old["export"],j["output"]["data"])
        copy_entry("a_path",old["export"],j["output"]["data"])
        copy_entry("mises",old["export"],j["output"]["data"])

    j["output"]["advanced"] = {}

    copy_entry("compute_error",old,j["output"]["advanced"])
    copy_entry("curved_mesh_size",old,j["output"]["advanced"])
    copy_entry("save_solve_sequence_debug",old,j["output"]["advanced"])
    copy_entry("save_time_sequence",old,j["output"]["advanced"])
    copy_entry("save_nl_solve_sequence",old,j["output"]["advanced"])

    if "export" in old:
        copy_entry("sol_on_grid",old["export"],j["output"]["advanced"])
        copy_entry("sol_at_node",old["export"],j["output"]["advanced"])
        copy_entry("vis_boundary_only",old["export"],j["output"]["advanced"])
        copy_entry("nodes",old["export"],j["output"]["advanced"])
        copy_entry("spectrum",old["export"],j["output"]["advanced"])

    # Reference
    j["output"]["reference"] = {}

    if "problem_params" in old:
        rename_entry("exact",old["problem_params"],"solution",j["output"]["reference"])
        rename_entry("exact_grad",old["problem_params"],"gradient",j["output"]["reference"])

    if "import" in old:

        j["input"] = {}
        j["input"]["data"] = {}
        copy_entry("u_path",old["import"],j["input"]["data"])
        copy_entry("v_path",old["import"],j["input"]["data"])
        copy_entry("a_path",old["import"],j["input"]["data"])

    # Body_ids are global and are added to volume selections

    if "body_ids" in old:
        for t in j["geometry"]:
            t["volume_selection"] = []

        for o in old["body_ids"]:
            n = {}
            copy_entry("id",o,n)
            copy_entry("axis",o,n)
            copy_entry("position",o,n)
            for t in j["geometry"]:
                t["volume_selection"].append(n)

    # boundary_sidesets are global and are added to surface selections

    if "boundary_sidesets" in old:
        for t in j["geometry"]:
            t["surface_selection"] = []

        for o in old["boundary_sidesets"]:
            n = {}
            copy_entry("id",o,n)
            copy_entry("axis",o,n)
            copy_entry("position",o,n)
            for t in j["geometry"]:
                t["surface_selection"].append(n)


    return j


if __name__ == "__main__":

    # read old json
    with open(sys.argv[1], 'r') as myfile:
        data_old=myfile.read()
    old = json.loads(data_old)

    # convert
    conv = PolyFEM_convert(old);

    # save it to file
    with open(sys.argv[2], 'w', encoding='utf-8') as f:
        json.dump(conv, f, ensure_ascii=False, indent=4)

