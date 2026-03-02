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

#ifndef CONTAINER_SRC_ROOT_TEST_COMMON_UTILITY_HH_
#define CONTAINER_SRC_ROOT_TEST_COMMON_UTILITY_HH_

#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

#include "../../common/utility.hh"
#include "../../src/utility.hh"

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
//! \brief Tools utility fixture
//! \since docker-finance 1.0.0
struct ToolsFixture
{
  //! \brief Tools implementation wrapper
  //! \since docker-finance 1.0.0
  struct ToolsImpl : public ::dfi::utility::impl::Tools
  {
    using t_impl = ::dfi::utility::impl::Tools;

    template <typename t_gen, typename t_num = double>
    std::vector<t_num>
    dist_range(const t_num min, const t_num max, const uint16_t precision = 8)
    {
      return t_impl::random_dist<t_gen, t_num>(min, max, precision);
    }
  };

  ToolsImpl tools;

  static constexpr double min{0.12345678}, max{1.23456789};
};

//! \brief Byte transformation fixture
//! \since docker-finance 1.0.0
struct ByteFixture
{
 protected:
  using t_byte = ::dfi::utility::Byte<uint8_t, std::byte>;

  //! \since docker-finance 1.0.0
  template <typename t_type>
  struct fixed_bytes final
  {
    auto operator()()
    {
      if constexpr (std::is_trivial_v<typename t_type::value_type>)
        {
          using t_value = typename t_type::value_type;
          t_type bytes{{t_value{0x01}, t_value{0x02}}};
          return bytes;
        }
      else if constexpr (std::is_trivial_v<typename t_type::mapped_type>)
        {
          using t_value = typename t_type::mapped_type;
          t_type bytes{{{}, t_value{0x01}}, {{}, t_value{0x02}}};
          return bytes;
        }
    }
  };

  t_byte byte;
};

//! \brief Common fixture
//! \note Not a 'common' fixture but rather a fixture for 'common'
//! \since docker-finance 1.1.0
struct CommonFixture
{
 protected:
};
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_COMMON_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
