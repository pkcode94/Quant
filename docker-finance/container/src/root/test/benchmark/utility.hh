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

#ifndef CONTAINER_SRC_ROOT_TEST_BENCHMARK_UTILITY_HH_
#define CONTAINER_SRC_ROOT_TEST_BENCHMARK_UTILITY_HH_

#include <benchmark/benchmark.h>

#include "../common/utility.hh"

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
//! \brief Tools benchmark fixture
//! \since docker-finance 1.0.0
struct Tools : public ::benchmark::Fixture, public tests::ToolsFixture
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(Tools, random_dist_TRandom1)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandom1>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandom2)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandom2>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandom3)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandom3>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomMixMax)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomMixMax>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomMixMax17)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomMixMax17>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomMixMax256)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomMixMax256>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomMT64)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomMT64>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomRanlux48)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomRanlux48>(min, max);
    }
}

BENCHMARK_F(Tools, random_dist_TRandomRanluxpp)
(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      tools.dist_range<::TRandomRanluxpp>(min, max);
    }
}

// TODO(unassigned): Byte

}  // namespace benchmarks
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_BENCHMARK_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
