// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

//! \file
//! \author Aaron Fiore (Founder, Evergreen Crypto LLC)
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.0.0

#ifndef CONTAINER_SRC_ROOT_SRC_INTERNAL_TYPE_HH_
#define CONTAINER_SRC_ROOT_SRC_INTERNAL_TYPE_HH_

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../../common/type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::internal
//! \brief For internal use only
//! \since docker-finance 1.0.0
namespace internal
{
//! \namespace dfi::internal::type
//! \brief docker-finance defined types
//! \since docker-finance 1.0.0
//! \todo *_v for all the booleans
namespace type
{
//! \deprecated This will be removed in the v2 API; use `dfi::common::type` namespace instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.1.0
using Exception = ::dfi::common::type::Exception;

//! \deprecated This will be removed in the v2 API; use `dfi::common::type` namespace instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.1.0
using RuntimeError = ::dfi::common::type::RuntimeError;

//! \deprecated This will be removed in the v2 API; use `dfi::common::type` namespace instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.1.0
using InvalidArgument = ::dfi::common::type::InvalidArgument;

//! \ingroup cpp_type_traits
template <typename t_type>
struct is_byte : std::false_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_byte<uint8_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_byte<std::byte> : std::true_type
{
};

// NOTE: uint8_t/char won't be treated as real integrals

//! \ingroup cpp_type_traits
template <typename t_type>
struct is_real_integral : std::false_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<uint16_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<uint32_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<uint64_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<int16_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<int32_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <>
struct is_real_integral<int64_t> : std::true_type
{
};

//! \ingroup cpp_type_traits
template <typename, typename = void>
struct has_value_type : std::false_type
{
};

//! \ingroup cpp_type_traits
template <typename t_type>
struct has_value_type<t_type, std::void_t<typename t_type::value_type>>
    : std::true_type
{
};

//! \ingroup cpp_type_traits
template <typename, typename = void>
struct has_mapped_type : std::false_type
{
};

//! \ingroup cpp_type_traits
template <typename t_type>
struct has_mapped_type<t_type, std::void_t<typename t_type::mapped_type>>
    : std::true_type
{
};

//! \ingroup cpp_type_traits
template <typename, typename = void>
struct has_allocator_type : std::false_type
{
};

//! \ingroup cpp_type_traits
//! \brief Specialization for AllocatorAware containers
template <template <typename...> typename t_container, typename... t_args>
struct has_allocator_type<
    t_container<t_args...>,
    std::void_t<
        typename std::allocator_traits<t_container<t_args...>>::allocator_type>>
    : std::true_type
{
};

// TODO(unassigned): specializations for non-AllocatorAware containers

//! \ingroup cpp_type_traits
template <typename, typename = void>
struct has_const_iterator : std::false_type
{
};

//! \ingroup cpp_type_traits
template <typename t_type>
struct has_const_iterator<t_type, std::void_t<typename t_type::const_iterator>>
    : std::true_type
{
};

// Type traits for external/internal signatures
// (reusable specializations which usually act as "interfaces")
// NOTE: Doxygen doesn't allow multiple groups for variables

//! \ingroup cpp_type_traits
//! \todo std::string_view is trivially copyable in c++23 but `dfi` ROOT build is c++20
template <typename t_type>
constexpr bool is_signature_with_trivial =
    (std::is_trivial_v<t_type> || std::is_same_v<t_type, std::string_view>);

//! \ingroup cpp_type_traits
template <typename t_type>
constexpr bool is_signature_with_string = std::is_same_v<t_type, std::string>;

//! \ingroup cpp_type_traits
template <typename t_type>
constexpr bool is_signature_with_value_type_allocator =
    (type::has_allocator_type<t_type>::value
     && type::has_value_type<t_type>::value
     && !std::is_same_v<t_type, std::string>
     && !std::is_same_v<t_type, std::string_view>);

//! \ingroup cpp_type_traits
template <typename t_type>
constexpr bool is_signature_with_mapped_type_allocator =
    (type::has_allocator_type<t_type>::value
     && type::has_mapped_type<t_type>::value);

//! \ingroup cpp_type_tags
//! \ingroup cpp_type_traits
//! \brief Supported Hash algorithm tags
//! \since docker-finance 1.0.0
struct Hash
{
  // TODO(unassigned): Argon2, SipHash

  // BLAKE2

  struct BLAKE2s
  {
  };

  struct BLAKE2b
  {
  };

  // Keccak

  struct Keccak_224
  {
  };

  struct Keccak_256
  {
  };

  struct Keccak_384
  {
  };

  struct Keccak_512
  {
  };

  // SHA1

  struct SHA1
  {
  };

  // SHA2

  struct SHA2_224
  {
  };

  struct SHA2_256
  {
  };

  struct SHA2_384
  {
  };

  struct SHA2_512
  {
  };

  // SHA3

  struct SHA3_224
  {
  };

  struct SHA3_256
  {
  };

  struct SHA3_384
  {
  };

  struct SHA3_512
  {
  };

  // SHAKE

  struct SHAKE128
  {
  };

  struct SHAKE256
  {
  };
};

}  // namespace type
}  // namespace internal
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_INTERNAL_TYPE_HH_

// # vim: sw=2 sts=2 si ai et
