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

#ifndef CONTAINER_SRC_ROOT_TEST_COMMON_TYPE_HH_
#define CONTAINER_SRC_ROOT_TEST_COMMON_TYPE_HH_

#include <gtest/gtest.h>

#include <string>

#include "../../src/internal/type.hh"

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
// TODO(afiore): move this out of common into pure unit test file
//! \namespace dfi::tests::unit
//! \brief docker-finance unit test cases
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace unit
{
//! \brief Exception fixture (base)
//! \since docker-finance 1.0.0
struct Exception : public ::testing::Test
{
 protected:
  std::string what = "better to be caught than thrown";
  using t_type = ::dfi::common::type::Exception::kType;
};
}  // namespace unit
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_COMMON_TYPE_HH_

// # vim: sw=2 sts=2 si ai et
