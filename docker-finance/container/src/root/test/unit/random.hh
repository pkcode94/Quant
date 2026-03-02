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

#ifndef CONTAINER_SRC_ROOT_TEST_UNIT_RANDOM_HH_
#define CONTAINER_SRC_ROOT_TEST_UNIT_RANDOM_HH_

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <type_traits>

#include "../common/random.hh"

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
//! \namespace dfi::tests::unit
//! \brief docker-finance unit test cases
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace unit
{
//! \brief Generic random generator interface (specializations) fixture
//! \since docker-finance 1.0.0
template <typename t_interface>
struct RandomInterfaceFixture : public ::testing::Test, protected t_interface
{
};

//! \brief Primary random generator interface fixture w/ mockup impl
//! \since docker-finance 1.0.0
struct RandomInterface : public RandomInterfaceFixture<tests::RandomInterface>
{
};

TEST_F(RandomInterface, generate_uint16_t)
{
  ASSERT_EQ(random.generate<uint16_t>(), std::numeric_limits<uint16_t>::max());
}

TEST_F(RandomInterface, generate_uint32_t)
{
  ASSERT_EQ(random.generate<uint32_t>(), std::numeric_limits<uint32_t>::max());
}

TEST_F(RandomInterface, generate_uint64_t)
{
  ASSERT_EQ(random.generate<uint64_t>(), std::numeric_limits<uint64_t>::max());
}

TEST_F(RandomInterface, generate_int16_t)
{
  ASSERT_EQ(random.generate<int16_t>(), std::numeric_limits<int16_t>::max());
}

TEST_F(RandomInterface, generate_int32_t)
{
  ASSERT_EQ(random.generate<int32_t>(), std::numeric_limits<int32_t>::max());
}

//! \brief Generic random generator fixture
//! \since docker-finance 1.0.0
template <typename t_impl>
struct RandomFixture : public ::testing::Test, protected t_impl
{
 protected:
  template <typename t_random>
  void generate()
  {
    // NOTE: Why auto? Because no accidental type casting or under/overflow
    auto one = t_impl::random.template generate<t_random>();
    static_assert(std::is_same_v<decltype(one), t_random>);

    auto two = t_impl::random.template generate<t_random>();
    static_assert(std::is_same_v<decltype(two), t_random>);

    // TODO(unassigned): gtest implicit conversion of uint64_t with ASSERT_NEAR
    if constexpr (!std::is_same_v<t_random, uint64_t>)
      {
        // NOTE: t_random limits are implementation-specific
        ASSERT_NEAR(0, one, std::numeric_limits<t_random>::max());
        ASSERT_NEAR(0, two, std::numeric_limits<t_random>::max());
      }

    // Exceedingly rare (tests the obvious, but is not an accurate entropy test)
    // If fails, re-run tests to confirm
    ASSERT_NE(one, two);
  };
};

//
// Botan
//

//! \brief Botan random number fixture
//! \since docker-finance 1.0.0
struct RandomBotan : public RandomFixture<tests::RandomBotan_Impl>
{
};

TEST_F(RandomBotan, generate_uint16_t)
{
  ASSERT_NO_THROW(generate<uint16_t>());
}

TEST_F(RandomBotan, generate_uint32_t)
{
  ASSERT_NO_THROW(generate<uint32_t>());
}

//
// Crypto++
//

//! \brief Crypto++ random number fixture
//! \since docker-finance 1.0.0
struct RandomCryptoPP : public RandomFixture<tests::RandomCryptoPP_Impl>
{
};

TEST_F(RandomCryptoPP, generate_uint16_t)
{
  ASSERT_NO_THROW(generate<uint16_t>());
}

TEST_F(RandomCryptoPP, generate_uint32_t)
{
  ASSERT_NO_THROW(generate<uint32_t>());
}

TEST_F(RandomCryptoPP, generate_int16_t)
{
  ASSERT_NO_THROW(generate<int16_t>());
}

TEST_F(RandomCryptoPP, generate_int32_t)
{
  ASSERT_NO_THROW(generate<int32_t>());
}

//
// libsodium
//

//! \brief libsodium random number fixture
//! \since docker-finance 1.0.0
struct RandomLibsodium : public RandomFixture<tests::RandomLibsodium_Impl>
{
};

TEST_F(RandomLibsodium, generate_uint16_t)
{
  ASSERT_NO_THROW(generate<uint16_t>());
}

TEST_F(RandomLibsodium, generate_uint32_t)
{
  ASSERT_NO_THROW(generate<uint32_t>());
}

#ifdef __DFI_PLUGIN_BITCOIN__
//
// Bitcoin
//

//! \brief Bitcoin random number fixture
//! \since docker-finance 1.1.0
struct RandomBitcoin : public RandomFixture<tests::RandomBitcoin_Impl>
{
};

TEST_F(RandomBitcoin, generate_uint32_t)
{
  ASSERT_NO_THROW(generate<uint32_t>());
}

TEST_F(RandomBitcoin, generate_uint64_t)
{
  ASSERT_NO_THROW(generate<uint64_t>());
}
#endif

}  // namespace unit
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_UNIT_RANDOM_HH_

// # vim: sw=2 sts=2 si ai et
