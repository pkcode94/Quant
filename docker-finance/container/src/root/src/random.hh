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
//! \todo Factor into larger cryptographical layout that includes encrypt/decrypt functionality

#ifndef CONTAINER_SRC_ROOT_SRC_RANDOM_HH_
#define CONTAINER_SRC_ROOT_SRC_RANDOM_HH_

#include <cstdint>
#include <type_traits>

#include "./internal/impl/random.hh"
#include "./internal/type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::crypto
//! \brief Cryptographical libraries
//! \since docker-finance 1.0.0
namespace crypto
{
//! \namespace dfi::crypto::common
//! \brief Common "interface" (specializations) to library-specific implementations
//! \warning Not for direct public consumption (use library namespace instead)
//! \since docker-finance 1.0.0
namespace common
{
//! \brief Common Random API "interface" (specializations)
//! \tparam t_impl Underlying Random implementation
//! \warning Not for direct public consumption (use library aliases instead)
//! \ingroup cpp_API
//! \since docker-finance 1.0.0
//! \todo NPI to set/get cryptographical parameters
template <typename t_impl>
class Random final : public t_impl
{
 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = default;
  Random& operator=(const Random&) = default;

  Random(Random&&) = default;
  Random& operator=(Random&&) = default;

 public:
  //! \brief Random number generator
  //! \details Returns a random value between 0 and 0xFFFFFFFF (depending on type)
  //! \warning Signed types will return positive value (per implementation)
  //! \tparam t_random Integral type of random number
  //! \return t_random Random number of type t_random
  template <
      typename t_random = uint32_t,
      std::enable_if_t<
          ::dfi::internal::type::is_real_integral<t_random>::value,
          bool> = true>
  t_random generate()
  {
    return t_impl::template generate<t_random>();
  }
};
}  // namespace common

//! \namespace dfi::crypto::botan
//! \brief Public-facing API namespace (Botan)
//! \since docker-finance 1.0.0
namespace botan
{
//! \brief Botan Random API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \note For public consumption
//! \since docker-finance 1.0.0
using Random = ::dfi::crypto::common::Random<impl::botan::Random>;
}  // namespace botan

//! \namespace dfi::crypto::cryptopp
//! \brief Public-facing API namespace (Crypto++)
//! \since docker-finance 1.0.0
namespace cryptopp
{
//! \brief Crypto++ Random API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \note For public consumption
//! \since docker-finance 1.0.0
using Random = ::dfi::crypto::common::Random<impl::cryptopp::Random>;
}  // namespace cryptopp

//! \namespace dfi::crypto::libsodium
//! \brief Public-facing API namespace (libsodium)
//! \since docker-finance 1.0.0
namespace libsodium
{
//! \brief libsodium Random API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \note For public consumption
//! \since docker-finance 1.0.0
using Random = ::dfi::crypto::common::Random<impl::libsodium::Random>;
}  // namespace libsodium

#ifdef __DFI_PLUGIN_BITCOIN__
//! \namespace dfi::crypto::bitcoin
//! \brief Public-facing API namespace (bitcoin)
//! \since docker-finance 1.1.0
namespace bitcoin
{
//! \brief bitcoin Random API specialization ("interface" / implementation)
//! \note For public consumption
//! \ingroup cpp_API
//! \since docker-finance 1.1.0
using Random = ::dfi::crypto::common::Random<impl::bitcoin::Random>;
}  // namespace bitcoin
#endif

}  // namespace crypto
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_RANDOM_HH_

// # vim: sw=2 sts=2 si ai et
