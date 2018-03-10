////////////////////////////////////////////////////////////////////////////////
#include "Problem.hpp"
#include "AssemblerUtils.hpp"
#include "Common.hpp"

#include <catch.hpp>
#include <iostream>
////////////////////////////////////////////////////////////////////////////////

using namespace poly_fem;


TEST_CASE("franke 2d", "[problem]") {
    Eigen::MatrixXd pts(400, 2);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Franke");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();

    Eigen::MatrixXd fx;

    //fun
    {

        auto cx2 = (9*x-2) * (9*x-2);
        auto cy2 = (9*y-2) * (9*y-2);

        auto cx1 = (9*x+1) * (9*x+1);
        auto cx7 = (9*x-7) * (9*x-7);

        auto cy3 = (9*y-3) * (9*y-3);
        auto cx4 = (9*x-4) * (9*x-4);

        auto cy7 = (9*y-7) * (9*y-7);

        Eigen::MatrixXd val = (3./4.)*exp(-(1./4.)*cx2-(1./4.)*cy2)+(3./4.)*exp(-(1./49.)*cx1-(9./10.)*y-1./10.)+(1./2.)*exp(-(1./4.)*cx7-(1./4.)*cy3)-(1./5.)*exp(-cx4-cy7);
        fx = val;

        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }


    //grad
    {
        Eigen::MatrixXd gradX = -0.243e3 / 0.8e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y) * x + 0.27e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y) - 0.243e3 / 0.98e2 * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.59e2 / 0.490e3 - 0.9e1 / 0.10e2 * y) * x - 0.27e2 / 0.98e2 * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.59e2 / 0.490e3 - 0.9e1 / 0.10e2 * y) - 0.81e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.29e2 / 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.27e2 / 0.2e1 * y) * x + 0.63e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.29e2 / 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.27e2 / 0.2e1 * y) + 0.162e3 / 0.5e1 * exp(-0.81e2 * x * x - 0.81e2 * y * y + 0.72e2 * x + 0.126e3 * y - 0.65e2) * x - 0.72e2 / 0.5e1 * exp(-0.81e2 * x * x - 0.81e2 * y * y + 0.72e2 * x + 0.126e3 * y - 0.65e2);
        Eigen::MatrixXd gradY = -0.243e3 / 0.8e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y) * y + 0.27e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y) - 0.27e2 / 0.40e2 * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.59e2 / 0.490e3 - 0.9e1 / 0.10e2 * y) - 0.81e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.29e2 / 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.27e2 / 0.2e1 * y) * y + 0.27e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.29e2 / 0.2e1 - 0.81e2 / 0.4e1 * y * y + 0.27e2 / 0.2e1 * y) + 0.162e3 / 0.5e1 * exp(-0.81e2 * x * x - 0.81e2 * y * y + 0.72e2 * x + 0.126e3 * y - 0.65e2) * y - 0.126e3 / 0.5e1 * exp(-0.81e2 * x * x - 0.81e2 * y * y + 0.72e2 * x + 0.126e3 * y - 0.65e2);

        probl->exact_grad(pts, other);

        Eigen::MatrixXd diff = (other.col(0) - gradX);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        diff = (other.col(1) - gradY);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }


    //rhs
    {
        auto cx2 = (9*x-2) * (9*x-2);
        auto cy2 = (9*y-2) * (9*y-2);

        auto cx1 = (9*x+1) * (9*x+1);
        auto cx7 = (9*x-7) * (9*x-7);

        auto cy3 = (9*y-3) * (9*y-3);
        auto cx4 = (9*x-4) * (9*x-4);

        auto cy7 = (9*y-7) * (9*y-7);

        auto s1 = (-40.5 * x+9) * (-40.5 * x + 9);
        auto s2 = (-162./49. * x - 18./49.) * (-162./49. * x - 18./49.);
        auto s3 = (-40.5 * x + 31.5) * (-40.5 * x + 31.5);
        auto s4 = (-162. * x + 72) * (-162 * x + 72);

        auto s5 = (-40.5 * y + 9) * (-40.5 * y + 9);
        auto s6 = (-40.5 * y + 13.5) * (-40.5 * y + 13.5);
        auto s7 = (-162 * y + 126) * (-162 * y + 126);

        Eigen::MatrixXd rhs = 243./4. * (-0.25 * cx2 - 0.25 * cy2).exp() -   0.75 * s1 * (-0.25 * cx2 - 0.25 *cy2).exp() +
        36693./19600. * (-1./49. * cx1 - 0.9 * y - 0.1).exp()  - 0.75 * s2 * (- 1./49 * cx1 - 0.9 * y - 0.1).exp() +
        40.5 * (-0.25 * cx7 - 0.25 * cy3).exp()   - 0.5 * s3 * (-0.25 * cx7 - 0.25 * cy3).exp() -
        324./5.  * (-cx4-cy7).exp() + 0.2 * s4 * (-cx4-cy7).exp() -
        0.75 * s5 * (-0.25 * cx2 - 0.25 *cy2).exp()  - 0.5 * s6 * (-0.25 * cx7 - 0.25 * cy3).exp() +
        0.2 * s7 * (-cx4-cy7).exp();
        rhs*=-1;

        probl->rhs("Laplacian", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        rhs += k * fx;
        probl->rhs("Helmholtz", pts, other);
        diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }
}


TEST_CASE("franke 3d", "[problem]") {
    Eigen::MatrixXd pts(400, 3);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Franke");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();
    auto z = pts.col(2).array();

    Eigen::MatrixXd fx;

    ///fun
    {
        auto cx2 = (9*x-2) * (9*x-2);
        auto cy2 = (9*y-2) * (9*y-2);
        auto cz2 = (9*z-2) * (9*z-2);

        auto cx1 = (9*x+1) * (9*x+1);
        auto cx7 = (9*x-7) * (9*x-7);

        auto cy3 = (9*y-3) * (9*y-3);
        auto cx4 = (9*x-4) * (9*x-4);
        auto cy7 = (9*y-7) * (9*y-7);

        auto cz5 = (9*y-5) * (9*y-5);

        Eigen::MatrixXd val =
        3./4. * exp( -1./4.*cx2 - 1./4.*cy2 - 1./4.*cz2) +
        3./4. * exp(-1./49. * cx1 - 9./10.*y - 1./10. -  9./10.*z - 1./10.) +
        1./2. * exp(-1./4. * cx7 - 1./4. * cy3 - 1./4. * cz5) -
        1./5. * exp(- cx4 - cy7 - cz5);
        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        fx = val;
    }


    //grad
    {

        Eigen::MatrixXd gradX = (-59535 * x + 13230) * exp(-0.81e2 / 0.4e1 * x * x + (9 * x) - 0.3e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y - 0.81e2 / 0.4e1 * z * z + 0.9e1 * z) / 0.1960e4 + (-39690 * x + 30870) * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.83e2 / 0.4e1 - 0.81e2 / 0.2e1 * y * y + 0.36e2 * y) / 0.1960e4 + (-4860 * x - 540) * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.54e2 / 0.245e3 - 0.9e1 / 0.10e2 * y - 0.9e1 / 0.10e2 * z) / 0.1960e4 + 0.162e3 / 0.5e1 * exp(-(81 * x * x) - 0.162e3 * y * y + (72 * x) + 0.216e3 * y - 0.90e2) * (x - 0.4e1 / 0.9e1);
        Eigen::MatrixXd gradY = -0.243e3 / 0.8e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.3e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y - 0.81e2 / 0.4e1 * z * z + 0.9e1 * z) * y + 0.27e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.3e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y - 0.81e2 / 0.4e1 * z * z + 0.9e1 * z) - 0.27e2 / 0.40e2 * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.54e2 / 0.245e3 - 0.9e1 / 0.10e2 * y - 0.9e1 / 0.10e2 * z) - 0.81e2 / 0.2e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.83e2 / 0.4e1 - 0.81e2 / 0.2e1 * y * y + 0.36e2 * y) * y + 0.18e2 * exp(-0.81e2 / 0.4e1 * x * x + 0.63e2 / 0.2e1 * x - 0.83e2 / 0.4e1 - 0.81e2 / 0.2e1 * y * y + 0.36e2 * y) + 0.324e3 / 0.5e1 * exp(-0.81e2 * x * x - 0.162e3 * y * y + 0.72e2 * x + 0.216e3 * y - 0.90e2) * y - 0.216e3 / 0.5e1 * exp(-0.81e2 * x * x - 0.162e3 * y * y + 0.72e2 * x + 0.216e3 * y - 0.90e2);
        Eigen::MatrixXd gradZ = -0.243e3 / 0.8e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.3e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y - 0.81e2 / 0.4e1 * z * z + 0.9e1 * z) * z + 0.27e2 / 0.4e1 * exp(-0.81e2 / 0.4e1 * x * x + 0.9e1 * x - 0.3e1 - 0.81e2 / 0.4e1 * y * y + 0.9e1 * y - 0.81e2 / 0.4e1 * z * z + 0.9e1 * z) - 0.27e2 / 0.40e2 * exp(-0.81e2 / 0.49e2 * x * x - 0.18e2 / 0.49e2 * x - 0.54e2 / 0.245e3 - 0.9e1 / 0.10e2 * y - 0.9e1 / 0.10e2 * z);

        probl->exact_grad(pts, other);

        Eigen::MatrixXd diff = (other.col(0) - gradX);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        diff = (other.col(1) - gradY);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        diff = (other.col(2) - gradZ);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }


    //rhs
    {
        Eigen::MatrixXd rhs =
        (1181472075 * x * x + 1181472075 * y * y + 1181472075 * z * z - 525098700 * x - 525098700 * y - 525098700 * z + 87516450) / 960400. *
        exp(-81./4. * x * x + 9 * x - 3 - 81./4. * y * y + 9 * y - 81./4. * z * z + 9 * z) +

        (787648050 * x * x + 3150592200 * y * y - 1225230300 * x - 2800526400 * y + 1040473350) / 960400. *
        exp(-81./4. * x * x + 63./2. * x - 83./4. - 81./2. * y * y + 36 * y) +

        (7873200 * x * x + 1749600 * x - 1117314) / 960400. *
        exp(-81./49. * x * x - 18./49. * x - 54./245. - 9./10. * y - 9./10. * z) -

        26244./ 5. * (x * x + 4 * y * y - 8./9. * x - 16./3. * y + 317./162.) *
        exp(-81 * x * x - 162 * y * y + 72 * x + 216 * y - 90);
        // rhs*=-1;

        probl->rhs("Laplacian", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        rhs += k*fx;
        probl->rhs("Helmholtz", pts, other);
        diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }
}


TEST_CASE("linear", "[problem]") {
    Eigen::MatrixXd pts(400, 2);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Linear");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();

    {
        Eigen::MatrixXd val = x;
        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }

    {

        Eigen::MatrixXd gradX = x;
        Eigen::MatrixXd gradY = x;
        gradX.setOnes();
        gradY.setZero();

        probl->exact_grad(pts, other);

        Eigen::MatrixXd diff = (other.col(0) - gradX);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        diff = (other.col(1) - gradY);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

    }



    {
        Eigen::MatrixXd rhs = x;
        rhs.setZero();

        probl->rhs("Laplacian", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }

    {
        Eigen::MatrixXd rhs = k*x;

        probl->rhs("Helmholtz", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }
}


TEST_CASE("quadratic", "[problem]") {
    Eigen::MatrixXd pts(400, 2);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Quadratic");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();

    {
        Eigen::MatrixXd val = 5*x*x;
        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }

    {

        Eigen::MatrixXd gradX = 10 * x;
        Eigen::MatrixXd gradY = x;
        gradY.setZero();

        probl->exact_grad(pts, other);

        Eigen::MatrixXd diff = (other.col(0) - gradX);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

        diff = (other.col(1) - gradY);
        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);

    }



    {
        Eigen::MatrixXd rhs = x;
        rhs.setConstant(10);

        probl->rhs("Laplacian", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }

    {
        Eigen::MatrixXd rhs = k*5*x*x + 10;

        probl->rhs("Helmholtz", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }
}


TEST_CASE("zero bc 2d", "[problem]") {
    Eigen::MatrixXd pts(400, 2);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Zero_BC");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();

    {
        Eigen::MatrixXd val = (1 - x)  * x * x * y * (1-y) *(1-y);
        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }


    {
        Eigen::MatrixXd rhs = -4 * x * y * (1 - y) * (1 - y) + 2 * (1 - x) * y * (1 - y) *(1 - y) - 4 * (1 - x) * x * x * (1 - y) + 2 * (1 - x) * x * x * y;

        probl->rhs("Laplacian", pts, other);
        Eigen::MatrixXd diff = (other - rhs);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }
}


TEST_CASE("zero bc 3d", "[problem]") {
    Eigen::MatrixXd pts(40, 3);
    Eigen::MatrixXd other;
    pts.setRandom();

    const double k = 0.2;
    const double lambda = 0.375, mu = 0.375;
    const auto &probl = ProblemFactory::factory().get_problem("Zero_BC");
    json params = {
            {"k", k},
            {"size", pts.cols()},
            {"lambda", lambda},
            {"mu", mu}
        };
    auto &assembler = AssemblerUtils::instance();
    assembler.set_parameters(params);

    auto x = pts.col(0).array();
    auto y = pts.col(1).array();
    auto z = pts.col(2).array();

    {
        Eigen::MatrixXd val = (1 - x)  * x * x * y * (1-y) *(1-y) * z * (1 - z);
        probl->exact(pts, other);
        Eigen::MatrixXd diff = (other - val);

        REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    }

    // {
    //     Eigen::MatrixXd rhs =  -4 * x * y * (1 - y) *(1 - y) * z * (1 - z) + 2 * (1 - x) * y * (1 - y) * (1 - y) * z * (1 - z) - 4 * (1 - x) * x * x * (1 - y) * z * (1 - z) + 2 * (1 - x) * x * x * y * z * (1 - z) - 2 * (1 - x) * x * y * (1 - y) * (1 - y);

    //     std::cout<<rhs<<"\n\n\n"<<std::endl;
    //     probl->rhs("Laplacian", pts, other);
    //     std::cout<<other<<std::endl;
    //     Eigen::MatrixXd diff = (other - rhs);

    //     REQUIRE(diff.array().abs().maxCoeff() < 1e-10);
    // }
}