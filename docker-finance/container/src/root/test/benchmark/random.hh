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

#ifndef CONTAINER_SRC_ROOT_TEST_BENCHMARK_RANDOM_HH_
#define CONTAINER_SRC_ROOT_TEST_BENCHMARK_RANDOM_HH_

#include <benchmark/benchmark.h>

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
//! \namespace dfi::tests::benchmarks
//! \brief docker-finance benchmark cases
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace benchmarks
{
//! \brief Generic Random generator "interface" (specializations) fixture
//! \since docker-finance 1.0.0
template <typename t_interface>
struct RandomInterfaceFixture : public ::benchmark::Fixture,
                                protected t_interface
{
};

//! \brief Primary Random generator "interface" fixture w/ mockup impl
//! \since docker-finance 1.0.0
struct RandomInterface : public RandomInterfaceFixture<tests::RandomInterface>
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(RandomInterface, generate)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      random.generate();
    }
}

//
// Botan
//

//! \brief Botan Random fixture w/ real implementation
//! \since docker-finance 1.0.0
struct RandomBotan : public ::benchmark::Fixture, public tests::RandomBotan_Impl
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(RandomBotan, generate)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      random.generate();
    }
}

//
// Crypto++
//

//! \brief Crypto++ Random fixture w/ real implementation
//! \since docker-finance 1.0.0
struct RandomCryptoPP : public ::benchmark::Fixture,
                        public tests::RandomCryptoPP_Impl
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(RandomCryptoPP, generate)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      random.generate();
    }
}

//
// libsodium
//

//! \brief libsodium Random fixture w/ real implementation
//! \since docker-finance 1.0.0
struct RandomLibsodium : public ::benchmark::Fixture,
                         public tests::RandomLibsodium_Impl
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(RandomLibsodium, generate)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      random.generate();
    }
}

//
// Bitcoin
//

#ifdef __DFI_PLUGIN_BITCOIN__
//! \brief Bitcoin Random fixture w/ real implementation
//! \since docker-finance 1.1.0
struct RandomBitcoin : public ::benchmark::Fixture,
                       public tests::RandomBitcoin_Impl
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(RandomBitcoin, generate)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      random.generate();
    }
}
#endif

}  // namespace benchmarks
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_BENCHMARK_RANDOM_HH_

// # vim: sw=2 sts=2 si ai et
