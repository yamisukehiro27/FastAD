#pragma once
#include <iterator>

namespace utils {

// type_traits extension

// Dummy function used to SFINAE on return value.
// Checks if type T is pointer-like (dereferenceable)
template <typename T>
auto is_pointer_like()
    -> decltype(*std::declval<T>(), std::true_type{});

template <typename T>
using is_pointer_like_dereferenceable = decltype(is_pointer_like<T>());

// Checks if type T is a tuple
template <class T>
struct is_tuple { static constexpr bool value = false; };
template <class... Ts>
struct is_tuple<std::tuple<Ts...>> { static constexpr bool value = true; };
template <class... Ts>
struct is_tuple<std::tuple<Ts...>&> { static constexpr bool value = true; };
template <class... Ts>
struct is_tuple<const std::tuple<Ts...>> { static constexpr bool value = true; };
template <class... Ts>
struct is_tuple<const std::tuple<Ts...>&> { static constexpr bool value = true; };

} // namespace utils