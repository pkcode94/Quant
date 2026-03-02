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

#ifndef CONTAINER_SRC_ROOT_MACRO_TEST_UNIT_C_
#define CONTAINER_SRC_ROOT_MACRO_TEST_UNIT_C_

#include <gtest/gtest.h>

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
//! \brief ROOT macro for docker-finance unit tests
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
class Unit
{
 public:
  Unit() = default;
  ~Unit() = default;

  Unit(const Unit&) = default;
  Unit& operator=(const Unit&) = default;

  Unit(Unit&&) = default;
  Unit& operator=(Unit&&) = default;

 public:
  //! \brief Macro for running all tests
  static int run(const std::string& arg)
  {
    ::testing::GTEST_FLAG(filter) = arg.empty() ? "*" : arg;
    return RUN_ALL_TESTS();
  }

 public:
  static constexpr std::initializer_list<std::string_view> m_paths{
      {"test/unit/hash.hh"},
      {"test/unit/random.hh"},
      {"test/unit/type.hh"},
      {"test/unit/utility.hh"}};
};

//! \brief Pluggable entrypoint
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.1.0
class unit_C final
{
 public:
  unit_C() = default;
  ~unit_C() = default;

  unit_C(const unit_C&) = default;
  unit_C& operator=(const unit_C&) = default;

  unit_C(unit_C&&) = default;
  unit_C& operator=(unit_C&&) = default;

 public:
  //! \brief Macro auto-loader
  //! \param arg GoogleTest regex filter
  static void load(const std::string& arg = {})
  {
    for (auto path : Unit::m_paths)
      ::dfi::common::load(std::string{path});

    ::testing::InitGoogleTest();

    Unit::run(arg);
  }

  //! \brief Macro auto-unloader
  //! \param arg Optional code to execute before unloading
  static void unload(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);

    for (auto path : Unit::m_paths)
      ::dfi::common::unload(std::string{path});
  }
};
}  // namespace test
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_TEST_UNIT_C_

// # vim: sw=2 sts=2 si ai et
