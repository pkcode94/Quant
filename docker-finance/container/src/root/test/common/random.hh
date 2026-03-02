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

#ifndef CONTAINER_SRC_ROOT_TEST_COMMON_RANDOM_HH_
#define CONTAINER_SRC_ROOT_TEST_COMMON_RANDOM_HH_

#include <cstdint>
#include <limits>
#include <type_traits>

#include "../../src/random.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::tests
//! \brief docker-finance common test framework
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace tests
{
//! \brief Generic Random "interface" (specialization) w/ mock impl
//! \since docker-finance 1.0.0
struct RandomInterface
{
 protected:
  //! \brief Random mock implementation
  //! \since docker-finance 1.0.0
  struct RandomImpl : public ::dfi::internal::Random<RandomImpl>
  {
    //! \brief Implements mock Random generator
    template <typename t_random>
    t_random generate_impl()
    {
      static_assert(
          ::dfi::internal::type::is_real_integral<t_random>::value,
          "Random interface has changed");

      return t_random{std::numeric_limits<t_random>::max()};
    }
  };

  using Random = ::dfi::crypto::common::Random<RandomImpl>;
  Random random;
};

//! \brief Botan random implementation fixture
//! \since docker-finance 1.0.0
struct RandomBotan_Impl
{
 protected:
  using Random = ::dfi::crypto::botan::Random;
  Random random;
};

//! \brief Crypto++ random implementation fixture
//! \since docker-finance 1.0.0
struct RandomCryptoPP_Impl
{
 protected:
  using Random = ::dfi::crypto::cryptopp::Random;
  Random random;
};

//! \brief libsodium random implementation fixture
//! \since docker-finance 1.0.0
struct RandomLibsodium_Impl
{
 protected:
  using Random = ::dfi::crypto::libsodium::Random;
  Random random;
};

#ifdef __DFI_PLUGIN_BITCOIN__
//! \brief Bitcoin random implementation fixture
//! \since docker-finance 1.1.0
struct RandomBitcoin_Impl
{
 protected:
  using Random = ::dfi::crypto::bitcoin::Random;
  Random random;
};
#endif

}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_COMMON_RANDOM_HH_

// # vim: sw=2 sts=2 si ai et
