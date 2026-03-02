// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

#ifndef CONTAINER_SRC_ROOT_MACRO_TEST_BENCHMARK_C_
#define CONTAINER_SRC_ROOT_MACRO_TEST_BENCHMARK_C_

#include <benchmark/benchmark.h>

#include <initializer_list>
#include <string>

#include "../../common/utility.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \namespace dfi::macro::test
//! \brief ROOT macros for docker-finance testing
//! \since docker-finance 1.0.0
namespace test
{
//! \brief ROOT macro for docker-finance benchmarks
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
class Benchmark
{
 public:
  Benchmark() = default;
  ~Benchmark() = default;

  Benchmark(const Benchmark&) = default;
  Benchmark& operator=(const Benchmark&) = default;

  Benchmark(Benchmark&&) = default;
  Benchmark& operator=(Benchmark&&) = default;

 public:
  //! \brief Macro for running all benchmarks
  static void run() { ::benchmark::RunSpecifiedBenchmarks(); }

 public:
  static constexpr std::initializer_list<std::string_view> m_paths{
      {"test/benchmark/hash.hh"},
      {"test/benchmark/random.hh"},
      {"test/benchmark/utility.hh"}};
};

//! \brief Pluggable entrypoint
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.1.0
class benchmark_C final
{
 public:
  benchmark_C() = default;
  ~benchmark_C() = default;

  benchmark_C(const benchmark_C&) = default;
  benchmark_C& operator=(const benchmark_C&) = default;

  benchmark_C(benchmark_C&&) = default;
  benchmark_C& operator=(benchmark_C&&) = default;

 public:
  //! \brief Macro auto-loader
  //! \param arg Optional code to execute after loading
  static void load(const std::string& arg = {})
  {
    for (auto path : Benchmark::m_paths)
      ::dfi::common::load(std::string{path});

    static int argc{1};
    static const char* argv{"docker-finance"};
    ::benchmark::Initialize(&argc, const_cast<char**>(&argv));

    // NOTE: benchmark filters can only be set via environment
    // (when calling this macro via `dfi` `root`, as an argument)
    Benchmark::run();

    if (!arg.empty())
      ::dfi::common::line(arg);
  }

  //! \brief Macro auto-unloader
  //! \param arg Optional code to execute before unloading
  static void unload(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);

    ::benchmark::Shutdown();

    for (auto path : Benchmark::m_paths)
      ::dfi::common::unload(std::string{path});
  }
};
}  // namespace test
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_TEST_BENCHMARK_C_

// # vim: sw=2 sts=2 si ai et
