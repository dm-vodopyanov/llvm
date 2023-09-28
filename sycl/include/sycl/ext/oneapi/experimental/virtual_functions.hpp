#pragma once

#include <sycl/ext/oneapi/properties/property.hpp>
#include <sycl/ext/oneapi/properties/property_value.hpp>

namespace sycl {
inline namespace _V1 {
namespace ext::oneapi::experimental {

struct indirectly_callable_key {
  template <typename Set>
  using value_t = property_value<indirectly_callable_key, Set>;
};

template <typename Set = void>
inline constexpr indirectly_callable_key::value_t<Set> indirectly_callable;

struct calls_indirectly_key {
  template <typename SetIds = void>
  using value_t = property_value<calls_indirectly_key, SetIds>;
      /* std::conditional_t<sizeof...(SetIds) == 0,
                         property_value<calls_indirectly_key, void>,
                         property_value<calls_indirectly_key, SetIds...>>;*/
};

template <typename SetIds>
inline constexpr calls_indirectly_key::value_t<SetIds> calls_indirectly;

template <> struct is_property_key<indirectly_callable_key> : std::true_type {};
template <> struct is_property_key<calls_indirectly_key> : std::true_type {};

namespace detail {

template <>
struct IsCompileTimeProperty<indirectly_callable_key> : std::true_type {};
template <>
struct IsCompileTimeProperty<calls_indirectly_key> : std::true_type {};

template <> struct PropertyToKind<indirectly_callable_key> {
  static constexpr PropKind Kind = PropKind::IndirectlyCallable;
};

template <> struct PropertyToKind<calls_indirectly_key> {
  static constexpr PropKind Kind = PropKind::CallsIndirectly;
};

template <typename Set>
struct PropertyMetaInfo<indirectly_callable_key::value_t<Set>> {
  static constexpr const char *name = "indirectly-callable";
  static constexpr const char *value =
#ifdef __SYCL_DEVICE_ONLY__
      __builtin_sycl_unique_stable_name(Set);
#else
      "";
#endif
};


// FIXME: We should be able to pass several sets into calls_indirectly property
template <typename Set>
struct PropertyMetaInfo<calls_indirectly_key::value_t<Set>> {
  static constexpr const char *name = "uses-indirectly-callable";
  static constexpr const char *value =
#ifdef __SYCL_DEVICE_ONLY__
      __builtin_sycl_unique_stable_name(Set);
#else
      "";
#endif
};
} // namespace detail

} // namespace ext::oneapi::experimental
} // namespace _V1
} // namespace sycl

#ifdef __SYCL_DEVICE_ONLY__
#define SYCL_EXT_ONEAPI_INDIRECTLY_CALLABLE(SetId)                             \
  __attribute__((sycl_device)) [[__sycl_detail__::add_ir_attribute_function(   \
      "indirectly-callable", __builtin_sycl_unique_stable_name(SetId))]]
#else

#define SYCL_EXT_ONEAPI_INDIRECTLY_CALLABLE(SetId)
#endif
