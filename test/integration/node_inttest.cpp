#include <testutil/base_fixture.hpp>
#include <fastad_bits/reverse/core/eval.hpp>
#include <fastad_bits/reverse/core/unary.hpp>
#include <fastad_bits/reverse/core/binary.hpp>
#include <fastad_bits/reverse/core/var.hpp>
#include <fastad_bits/reverse/core/eq.hpp>
#include <fastad_bits/reverse/core/glue.hpp>
#include <fastad_bits/reverse/core/sum.hpp>
#include <fastad_bits/reverse/core/norm.hpp>
#include <fastad_bits/reverse/core/dot.hpp>
#include <fastad_bits/reverse/core/for_each.hpp>

namespace ad {
namespace core {

struct node_integration_fixture: base_fixture
{
protected:
};

////////////////////////////////////////////////////////////
// LeafNode, UnaryNode Integration Test 
////////////////////////////////////////////////////////////

// LeafNode -> UnaryNode 

TEST_F(node_integration_fixture, leaf_unary) 
{
    Var<double> x(3.1);
    auto expr = ad::sin(x);
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), std::sin(3.1));
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), std::cos(3.1));
}

// LeafNode -> -UnaryNode 

TEST_F(node_integration_fixture, leaf_unary_minus) 
{
    Var<double> x(3.1);
    auto expr = -x;
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), -3.1);
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), -1.);
}

// LeafNode -> UnaryNode -> UnaryNode

TEST_F(node_integration_fixture, leaf_unary_unary)
{
    Var<double> x(3.1);
    auto expr = ad::sin(ad::log(x));
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), std::sin(std::log(3.1)));
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), std::cos(std::log(3.1)) / 3.1);
}

////////////////////////////////////////////////////////////
// LeafNode, BinaryNode Integration Test 
////////////////////////////////////////////////////////////

// LeafNode, LeafNode -> BinaryNode

TEST_F(node_integration_fixture, leaf_binary)
{
    Var<double> x(1.), y(2.);
    auto expr = x + y;
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), 3.);
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), 1.);
    EXPECT_DOUBLE_EQ(y.get_adj(0,0), 1.);
}

// LeafNode, LeafNode -> BinaryNode, LeafNode -> BinaryNode

TEST_F(node_integration_fixture, leaf_leaf_binary)
{
    Var<double> x(1.), y(2.), z(3.);
    auto expr = (x + y) - z;
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), 0.);
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), 1.);
    EXPECT_DOUBLE_EQ(y.get_adj(0,0), 1.);
    EXPECT_DOUBLE_EQ(z.get_adj(0,0), -1.);
}

////////////////////////////////////////////////////////////
// LeafNode, UnaryNode, BinaryNode Integration Test 
////////////////////////////////////////////////////////////

TEST_F(node_integration_fixture, leaf_unary_binary) {
    double x1 = 2.0, x2 = 1.31, x3 = -3.14;
    double dfs[3] = { 0 };
    VarView<double> leaf1(&x1, dfs);
    VarView<double> leaf2(&x2, dfs+1);
    VarView<double> leaf3(&x3, dfs+2);

    auto expr = leaf1 + ad::sin(leaf2 + leaf3);
    bind(expr);
    EXPECT_DOUBLE_EQ(expr.feval(), x1 + std::sin(x2 + x3));
    expr.beval(1);
    EXPECT_DOUBLE_EQ(dfs[0], 1);
    EXPECT_DOUBLE_EQ(dfs[1], std::cos(x2 + x3));
    EXPECT_DOUBLE_EQ(dfs[2], std::cos(x2 + x3));
}

TEST_F(node_integration_fixture, leaf_unary_binary_2) {
    double x1 = 1.2041, x2 = -2.2314;
    double dfs[2] = { 0 };
    VarView<double> leaf1(&x1, dfs);
    VarView<double> leaf2(&x2, dfs + 1);

    EXPECT_DOUBLE_EQ(leaf1.feval(), x1);
    EXPECT_DOUBLE_EQ(leaf2.get(), x2);

    auto expr = leaf1 * leaf2 + sin(leaf1);
    bind(expr);
    EXPECT_DOUBLE_EQ(expr.feval(), x1*x2 + std::sin(x1));
    expr.beval(1);
    EXPECT_DOUBLE_EQ(dfs[0], x2 + std::cos(x1));
    EXPECT_DOUBLE_EQ(dfs[1], x1);
}

TEST_F(node_integration_fixture, leaf_unary_binary_3) {
    double x1 = 1.2041, x2 = -2.2314;
    double dfs[2] = { 0 };
    VarView<double> leaf1(&x1, dfs);
    VarView<double> leaf2(&x2, dfs + 1);

    auto expr = leaf1 * leaf2 + sin(leaf1 + leaf2) * leaf2 - leaf1 / leaf2;
    bind(expr);
    EXPECT_DOUBLE_EQ(expr.feval(), x1*x2 + std::sin(x1 + x2)*x2 - x1 / x2);
    expr.beval(1);
    EXPECT_DOUBLE_EQ(dfs[0],
        x2 + std::cos(x1 + x2) * x2 - 1. / x2);
    EXPECT_DOUBLE_EQ(dfs[1],
        x1 + std::cos(x1 + x2) * x2 + std::sin(x1 + x2) + x1 / (x2*x2));
}

TEST_F(node_integration_fixture, leaf_unary_binary_4) {
    double x1 = 1.5928, x2 = -0.291, x3 = 5.1023;
    double dfs[3] = { 0 };
    VarView<double> leaf1(&x1, dfs);
    VarView<double> leaf2(&x2, dfs + 1);
    VarView<double> leaf3(&x3, dfs + 2);

    auto expr =
        leaf1 * leaf3 + sin(cos(leaf1 + leaf2)) * leaf2 - leaf1 / exp(leaf3);
    bind(expr);
    EXPECT_DOUBLE_EQ(expr.feval(),
        x1*x3 + std::sin(std::cos(x1 + x2))*x2 - x1 / std::exp(x3));
    expr.beval(1);
    EXPECT_DOUBLE_EQ(dfs[0],
        x3 - x2 * std::cos(std::cos(x1 + x2))*std::sin(x1 + x2) - std::exp(-x3));
    EXPECT_DOUBLE_EQ(dfs[1],
        std::sin(std::cos(x1 + x2))
        - x2 * std::cos(std::cos(x1 + x2))*std::sin(x1 + x2));
    EXPECT_DOUBLE_EQ(dfs[2], x1 + x1 * std::exp(-x3));
}

////////////////////////////////////////////////////////////
// LeafNode, UnaryNode, EqNode Integration Test 
////////////////////////////////////////////////////////////

// LeafNode, (LeafNode -> UnaryNode) -> EqNode
TEST_F(node_integration_fixture, leaf_unary_eq)
{
    Var<double> x(1.), y(2.);
    auto expr = (x = ad::tan(y));
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), std::tan(2.));
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), 1.);
    EXPECT_DOUBLE_EQ(y.get_adj(0,0), 1./(std::cos(2.) * std::cos(2.)));
}

////////////////////////////////////////////////////////////
// LeafNode, ConstantNode Integration Test 
////////////////////////////////////////////////////////////

TEST_F(node_integration_fixture, leaf_constant)
{
    Var<double> x(0.);
    auto expr = (x = 1.);
    bind(expr);
    EXPECT_DOUBLE_EQ(autodiff(expr), 1.);
    EXPECT_DOUBLE_EQ(x.get_adj(0,0), 1.);
}

////////////////////////////////////////////////////////////
// LeafNode, OpEqNode Integration Test 
////////////////////////////////////////////////////////////

TEST_F(node_integration_fixture, leaf_opeq_many_nested)
{
    auto expr = (vec_expr *= scl_expr,
                 vec_expr += vec_expr - scl_expr,
                 scl_expr *= scl_expr - 2.,
                 scl_expr /= 1.,
                 vec_expr -= 2. * vec_expr,
                 vec_expr *= -1.,
                 vec_expr -= scl_expr,
                 vec_expr /= scl_expr,
                 ad::sum(vec_expr));
    bind(expr);
    Eigen::VectorXd vec_orig = vec_expr.get();
    double scl_orig = scl_expr.get();
    double n = vec_orig.size();
    double actual = 1./(scl_orig - 2.) * 
        (2. * vec_orig.array().sum() - n) - n;

    EXPECT_DOUBLE_EQ(autodiff(expr), actual);

    double scl_adj = -1./std::pow(scl_orig - 2, 2) * 
        (2. * vec_orig.array().sum() - n);
    EXPECT_DOUBLE_EQ(scl_expr.get_adj(), scl_adj);

    double vec_adj = 2./(scl_orig - 2.);
    for (size_t i = 0; i < vec_expr.size(); ++i) {
        EXPECT_DOUBLE_EQ(vec_expr.get_adj()(i), vec_adj);
    }
}

TEST_F(node_integration_fixture, leaf_opeq_head_tail)
{
    // note this is NOT same as AR model (you need a for loop for that)
    auto head = vec_expr.head(vec_expr.size()-1);
    auto tail = vec_expr.tail(vec_expr.size()-1);
    auto expr = (tail += scl_expr * head,
                 ad::sum(tail));
    bind(expr);
    Eigen::VectorXd head_orig = head.get();
    Eigen::VectorXd tail_orig = tail.get();
    double scl_orig = scl_expr.get();
    double tail_sum = tail_orig.array().sum(); 
    double head_sum = head_orig.array().sum();
    double actual = tail_sum + scl_orig * head_sum;

    EXPECT_DOUBLE_EQ(autodiff(expr), actual);

    double scl_adj = head_sum;
    EXPECT_DOUBLE_EQ(scl_expr.get_adj(), scl_adj);

    EXPECT_DOUBLE_EQ(vec_expr.get_adj()(0), scl_orig);
    for (size_t i = 1; i < vec_expr.size() - 1; ++i) {
        EXPECT_DOUBLE_EQ(vec_expr.get_adj()(i), 1 + scl_orig);
    }
    EXPECT_DOUBLE_EQ(vec_expr.get_adj()(vec_expr.size()-1), 1.);
}

////////////////////////////////////////////////////////////
// LeafNode, UnaryNode, BinaryNode, EqNode, GlueNode Integration Test 
////////////////////////////////////////////////////////////

TEST_F(node_integration_fixture, leaf_binary_eq_glue) 
{
    Var<double> w1(1.0), w2(2.0), w3(3.0), w4(4.0);
    auto expr = (w3 = w1 * w2, w4 = w3 * w3);
    bind(expr);

    EXPECT_DOUBLE_EQ(expr.feval(), 4.);
    EXPECT_DOUBLE_EQ(w1.get(), 1.);
    EXPECT_DOUBLE_EQ(w2.get(), 2.);
    EXPECT_DOUBLE_EQ(w3.get(), 2.);
    EXPECT_DOUBLE_EQ(w4.get(), 4.);

    expr.beval(1);
    EXPECT_DOUBLE_EQ(w4.get_adj(0,0), 1.0);
    EXPECT_DOUBLE_EQ(w3.get_adj(0,0), 2 * w3.get());
    EXPECT_DOUBLE_EQ(w2.get_adj(0,0), 2 * w2.get()*w1.get()*w1.get());
    EXPECT_DOUBLE_EQ(w1.get_adj(0,0), 2 * w1.get()*w2.get()*w2.get());
}

TEST_F(node_integration_fixture, leaf_unary_binary_eq_glue) 
{
    double x1 = -0.201, x2 = 1.2241;
    Var<double> w1(x1), w2(x2), w3, w4, w5;
    auto expr = (
        w3 = w1 * sin(w2),
        w4 = w3 + w1 * w2,
        w5 = exp(w4*w3)
        );
    bind(expr);

    expr.feval();
    EXPECT_DOUBLE_EQ(w5.get(), std::exp((x1*std::sin(x2) + x1 * x2)*(x1*std::sin(x2))));
    EXPECT_DOUBLE_EQ(w4.get(), x1*std::sin(x2) + x1 * x2);
    EXPECT_DOUBLE_EQ(w3.get(), x1*std::sin(x2));

    expr.beval(1);
    EXPECT_DOUBLE_EQ(w5.get_adj(0,0), 1);
    EXPECT_DOUBLE_EQ(w4.get_adj(0,0), w3.get() * w5.get());
    EXPECT_DOUBLE_EQ(w3.get_adj(0,0), (w3.get() + w4.get()) * w5.get());
    EXPECT_DOUBLE_EQ(w2.get_adj(0,0), 
            w5.get()*x1*x1*
            (std::cos(x2)*(std::sin(x2) + x2) + std::sin(x2)*(1 + std::cos(x2))));
    EXPECT_DOUBLE_EQ(w1.get_adj(0,0), w5.get() * 2 * x1 * std::sin(x2) *(std::sin(x2) + x2));
}

TEST_F(node_integration_fixture, sumnode) {
    Var<double> vec[3];
    vec[0].get() = 0.203104;
    vec[1].get() = 1.4231;
    vec[2].get() = -1.231;
    auto expr = ad::sum(vec, vec + 3,
        [](Var<double> const& v) {return ad::cos(ad::sin(v)*v); });
    bind(expr);

    double actual_sum = 0;
    for (size_t i = 0; i < 3; ++i) {
        actual_sum += std::cos(std::sin(vec[i].get())*vec[i].get());
    }

    EXPECT_DOUBLE_EQ(expr.feval(), actual_sum);
    expr.beval(1);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_DOUBLE_EQ(vec[i].get_adj(0,0),
            -std::sin(std::sin(vec[i].get()) *
            vec[i].get()) * 
            (std::cos(vec[i].get()) *
            vec[i].get() + 
            std::sin(vec[i].get())));
    }
    EXPECT_DOUBLE_EQ(expr.get(), actual_sum);

    // Reset adjoint and re-evaluate
    vec[0].reset_adj();
    vec[1].reset_adj();
    vec[2].reset_adj();

    EXPECT_DOUBLE_EQ(expr.feval(), actual_sum);

    expr.beval(1);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_DOUBLE_EQ(vec[i].get_adj(0,0),
            -std::sin(std::sin(vec[i].get()) *
            vec[i].get()) * 
            (std::cos(vec[i].get()) *
            vec[i].get() + 
            std::sin(vec[i].get())));
    }
}

TEST_F(node_integration_fixture, foreach) {
    std::vector<ad::Var<double>> vec(3);
    vec[0].get() = 100.; 
    vec[1].get() = 20.;
    vec[2].get() = -10.;
    vec.emplace_back(1e-3);
    std::vector<ad::Var<double>> prod(4);
    prod[0].bind({vec[0].data(), vec[0].data_adj()});
    auto it_prev = prod.begin();
    auto vec_it = vec.begin();
    auto expr = ad::for_each(std::next(prod.begin()), 
                             prod.end(), 
                             [&](const auto& cur) 
                             { return cur = (*it_prev++) * (*++vec_it); }
        );

    double actual = 1;
    for (size_t i = 0; i < vec.size(); ++i) {
        actual *= vec[i].get();
    }

    Var<double> res, w4;
    auto expr2 = (res = expr, w4 = res * res + vec[0]);
    bind(expr2);

    expr2.feval();
    expr2.beval(1);

    EXPECT_DOUBLE_EQ(res.get(), actual);
    EXPECT_DOUBLE_EQ(w4.get(), actual*actual + vec[0].get());

    for (size_t i = 0; i < vec.size(); ++i) {
        EXPECT_DOUBLE_EQ(vec[i].get_adj(0,0),
            ((i == 0) ? 1 : 0) + 2 * actual * actual / vec[i].get());
    }
}

TEST_F(node_integration_fixture, mat_scl_reduction) 
{
    Var<double, ad::mat> v(3, 2), w(3, 2);
    Var<double> x;
    v.get(0,0) = 2.;
    v.get(1,0) = 3.;
    v.get(2,0) = 4.;

    auto expr = (w = v * v,
                 x = ad::sum(w));
    bind(expr);

    double val = autodiff(expr);
    EXPECT_DOUBLE_EQ(val, 29.);

    for (size_t i = 0; i < v.rows(); ++i) {
        for (size_t j = 0; j < v.cols(); ++j) {
            EXPECT_DOUBLE_EQ(v.get_adj(i,j), v.get(i,j) * 2.);
        }
    }
}

TEST_F(node_integration_fixture, dot_sum_norm)
{
    Var<value_t, mat> M(mat_rows, mat_cols);
    Var<value_t, vec> x(mat_cols), v(mat_rows);
    Var<value_t> w;
    auto c = ad::constant(3.);

    this->mat_initialize(M);
    auto x_raw = x.get();
    x_raw(0) = 1.;
    x_raw(0) = 0.;
    x_raw(0) = 2.;

    auto expr = (v = ad::dot(M, x),
                 w = ad::norm(v) + ad::sum(v - c));
    bind(expr);

    value_t res = ad::autodiff(expr);

    // compare actual expression value
    Eigen::VectorXd v_actual = M.get() * x.get();
    value_t w_actual = v_actual.squaredNorm() + 
        (v_actual.array() - c.feval()).sum();

    EXPECT_DOUBLE_EQ(res, w_actual);

    // compare adjoints for x
    Eigen::VectorXd x_adj = M.get().transpose() *
        ((2.*M.get()*x.get()).array() + 1).matrix();
    for (size_t i = 0; i < x.size(); ++i) {
        EXPECT_DOUBLE_EQ(x_adj(i), x.get_adj(i,0));
    }

    // compare adjoints for M
    for (size_t i = 0; i < M.rows(); ++i) {
        for (size_t j = 0; j < M.cols(); ++j) {
            value_t Mij_adj = (2.*M.get().row(i)*x.get() + 1) * x.get(j,0);
            EXPECT_DOUBLE_EQ(Mij_adj, M.get_adj(i,j));
        }
    }
}

} // namespace core
} // namespace ad
