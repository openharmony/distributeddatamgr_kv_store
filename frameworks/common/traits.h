/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TRAITS_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TRAITS_H
#include <type_traits>
#include <variant>
namespace OHOS {
namespace Traits {
// If there is one in the ...Types, that is equal to T. same_index_of_v is the index.
// If there is no one in the ...Types, that is equal to T. same_index_of_v is sizeof ...(Types)
template<typename T, typename... Types>
inline constexpr size_t same_index_of_v = std::__detail::__variant::__index_of_v<T, Types...>;

// There is one in the ...Types, that is equal to T. If not, the same_in_v will be false.
template<typename T, typename... Types>
inline constexpr bool same_in_v = (same_index_of_v<T, Types...> < sizeof...(Types));

template<typename _Tp, typename... _Types>
struct convertible_index_of : std::integral_constant<size_t, 0> {
};

// If there is one in the ...Types that can convert to T implicitly, convertible_index_v is the index.
// If there is no one in the ...Types that can convert to T implicitly, convertible_index_v is sizeof ...(Types)
template<typename _Tp, typename... _Types>
inline constexpr size_t convertible_index_of_v = convertible_index_of<_Tp, _Types...>::value;

template<typename _Tp, typename _First, typename... _Rest>
struct convertible_index_of<_Tp, _First, _Rest...>
    : std::integral_constant<size_t, std::is_convertible_v<_Tp, _First> ? 0 : convertible_index_of_v<_Tp, _Rest...> + 1> {
};

// There is one in the ...Types, that can convert to T implicitly. If not, the convertible_in_v will be false.
template<typename T, typename... Types>
inline constexpr bool convertible_in_v = (convertible_index_of_v<T, Types...> < sizeof...(Types));

template<typename... Types>
struct variant_size_of {
    static constexpr size_t value = sizeof...(Types);
};

template<typename T, typename... Types>
struct variant_index_of {
    static constexpr size_t value = same_index_of_v<T, Types...>;
};

template<typename... Types>
variant_size_of<Types...> variant_size_test(const std::variant<Types...> &);

template<typename T, typename... Types>
variant_index_of<T, Types...> variant_index_test(const T &, const std::variant<Types...> &);

// variant_index_of_v is the count of the variant V's types.
template<typename V>
inline constexpr size_t variant_size_of_v = decltype(variant_size_test(std::declval<V>()))::value;

// If T is one type of the variant V, variant_index_of_v is the index. If not, variant_index_of_v is the size.
template<typename T, typename V>
inline constexpr size_t variant_index_of_v = decltype(variant_index_test(std::declval<T>(), std::declval<V>()))::value;

/*
 * Extend the template<typename _Tp, typename... _Types> std::get_if(variant<_Types...>*) function to support these:
 * 1. When the _Tp is a type in the ..._Types, the get_if is equal to the std::get_if.
 * 2. When the _Tp is not a type in the ..._Types but someone in the ...Types can convert to _Tp implicitly,
 *    the get_if will return it.
 * 3. When the _Tp is not a type in the ..._Types and can't convert, the get_if will return nullptr.
 * */
template<typename T, typename... Types>
std::enable_if_t<same_in_v<T, Types...>, T *> get_if(std::variant<Types...> *input)
{
    return std::get_if<T>(input);
}

template<typename T, typename... Types>
std::enable_if_t<same_in_v<T, Types...>, const T *> get_if(const std::variant<Types...> *input)
{
    return std::get_if<T>(input);
}

template<typename T, typename... Types, size_t NP = convertible_in_v<T, Types...> ? convertible_index_of_v<T, Types...> : 0>
constexpr std::enable_if_t<!same_in_v<T, Types...> && convertible_in_v<T, Types...>,
    std::add_pointer_t<std::variant_alternative_t<NP, std::variant<Types...>>>>
get_if(std::variant<Types...> *input)
{
    return std::get_if<NP>(input);
}

template<typename T, typename... Types, size_t NP = convertible_in_v<T, Types...> ? convertible_index_of_v<T, Types...> : 0>
constexpr std::enable_if_t<!same_in_v<T, Types...> && convertible_in_v<T, Types...>,
    std::add_pointer_t<const std::variant_alternative_t<NP, std::variant<Types...>>>>
get_if(const std::variant<Types...> *input)
{
    return std::get_if<NP>(input);
}

template<typename T, typename... Types>
std::enable_if_t<!same_in_v<T, Types...> && !convertible_in_v<T, Types...>, T *> get_if(std::variant<Types...> *input)
{
    (void)input;
    return nullptr;
}

template<typename T, typename... Types>
std::enable_if_t<!same_in_v<T, Types...> && !convertible_in_v<T, Types...>, const T *> get_if(
    const std::variant<Types...> *input)
{
    (void)input;
    return nullptr;
}
} // namespace Traits
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TRAITS_H
