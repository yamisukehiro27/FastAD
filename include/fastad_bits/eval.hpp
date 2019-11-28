#pragma once
#include <cstdlib>
#include <type_traits>
#include <tuple>

#ifdef USE_BOOST_THREAD_MT

#include <thread>                       // hardware_concurrency
#include <boost/asio/thread_pool.hpp>   // thread_pool
#include <boost/asio.hpp>               // post

#endif

namespace ad {

#ifdef USE_BOOST_THREAD_MT

// Minimum number of expressions in tuple to evaluate to use thread pool.
// See autodiff(std::tuple) for more details.
static constexpr size_t THR_THRESHOLD = 10;

#endif

// Evaluates expression in the forward direction of reverse-mode AD.
// @tparam ExprType expression type
// @param expr  expression to forward evaluate
// @return the expression value
template <class ExprType>
inline auto evaluate(ExprType&& expr)
{
    return expr.feval();
}

// Evaluates expression in the backward direction of reverse-mode AD.
// @tparam ExprType expression type
// @param expr  expression to backward evaluate
template <class ExprType>
inline void evaluate_adj(ExprType&& expr)
{
    expr.beval(1);
}

// Evaluates expression both in the forward and backward direction of reverse-mode AD.
// @tparam ExprType expression type
// @param expr  expression to forward and backward evaluate
// Returns the forward expression value
template <class ExprType>
inline auto autodiff(ExprType&& expr)
{
    auto t = evaluate(expr);
    evaluate_adj(expr);
    return t;
}

//====================================================================================================

namespace eval {
namespace details {

///////////////////////////////////////////////////////
// Sequential autodiff
///////////////////////////////////////////////////////

// This function is the ending condition when number of expressions is equal to I.
// @tparam I    index of first expression to auto-differentiate
// @tparam ExprTypes expression types
template <size_t I, class... ExprTypes>
inline typename std::enable_if<I == sizeof...(ExprTypes)>::type
autodiff(std::tuple<ExprTypes...>&) 
{}

// This function calls ad::autodiff from the Ith expression to the last expression in tup.
// @tparam I    index of first expression to auto-differentiate
// @tparam ExprTypes    expression types
// @param tup   the tuple of expressions to auto-differentiate
template <size_t I, class... ExprTypes>
inline typename std::enable_if < I < sizeof...(ExprTypes)>::type
autodiff(std::tuple<ExprTypes...>& tup)
{
    ad::autodiff(std::get<I>(tup)); 
    autodiff<I + 1>(tup);
}

///////////////////////////////////////////////////////
// Multi-threaded autodiff
///////////////////////////////////////////////////////

#ifdef USE_BOOST_THREAD_MT

// This function is the ending condition when there are no expressions to auto-differentiate.
template <size_t I, class... ExprTypes>
inline typename std::enable_if<(I == sizeof...(ExprTypes))>::type 
autodiff(boost::asio::thread_pool&, std::tuple<ExprTypes...>&)
{}

// This function auto-differentiates from the Ith expression by posting as jobs to pool.
// @tparam ExprTypes    rest of the expression types
// @tparam I    index of first expression to auto-differentiate
// @param   pool    thread pool in which to post auto-differentiating job
// @param   tup tuple of expressions to auto-differentiate
template <size_t I, class... ExprTypes>
inline typename std::enable_if<(I < sizeof...(ExprTypes))>::type
autodiff(boost::asio::thread_pool& pool, std::tuple<ExprTypes...>& tup)
{
    boost::asio::post(pool, [&](){
            ad::autodiff(std::get<I>(tup));
            });
    autodiff<I+1>(pool, tup);
}

#endif

} // namespace details 

///////////////////////////////////////////////////////
// Sequential/Multi-threaded Chooser 
///////////////////////////////////////////////////////

#ifdef USE_BOOST_THREAD_MT

// This function initializes a thread pool with the hardware max number of threads
// and auto-differentiates every expression in tup.
// It is a blocking call since it waits until pool finishes executing all jobs.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>& tup, std::true_type)
{
    const int thread_num = std::thread::hardware_concurrency();
    boost::asio::thread_pool pool(thread_num);
    details::autodiff<0>(pool, tup);
    pool.join();
}

#endif

// This function auto-differentiates every expression in tup.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>& tup, std::false_type)
{
    details::autodiff<0>(tup);
}

} // namespace eval

#ifdef USE_BOOST_THREAD_MT

// Auto-differentiator for lvalue reference of tuple of expressions
// If number of expressions is above THR_THRESHOLD, multi-threading is used.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>& tup)
{
    eval::autodiff(tup
        , std::integral_constant<bool, (sizeof...(ExprTypes) >= THR_THRESHOLD)>());
}

// Auto-differentiator for rvalue reference of tuple of expressions
// If number of expressions is above THR_THRESHOLD, multi-threading is used.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>&& tup)
{
    eval::autodiff(tup
        , std::integral_constant<bool, (sizeof...(ExprTypes) >= THR_THRESHOLD)>());
}

#else

// Auto-differentiator for lvalue reference of tuple of expressions.
// Always processes sequentially.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>& tup)
{
    eval::autodiff(tup, std::false_type());
}

// Auto-differentiator for rvalue reference of tuple of expressions
// Always processes sequentially.
// @tparam  ExprTypes   expression types
// @param   tup tuple of expressions to auto-differentiate
template <class... ExprTypes>
inline void autodiff(std::tuple<ExprTypes...>&& tup)
{
    eval::autodiff(tup, std::false_type());
}

#endif

} // namespace ad