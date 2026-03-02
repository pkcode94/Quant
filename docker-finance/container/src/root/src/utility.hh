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

#ifndef CONTAINER_SRC_ROOT_SRC_UTILITY_HH_
#define CONTAINER_SRC_ROOT_SRC_UTILITY_HH_

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "./internal/impl/utility.hh"
#include "./internal/type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::utility
//! \brief Various docker-finance utilities
//! \since docker-finance 1.0.0
namespace utility
{

namespace common = ::dfi::common;

//! \brief Misc utility tools
//! \ingroup cpp_utils
//! \since docker-finance 1.0.0
class Tools final : protected impl::Tools
{
  using t_impl = impl::Tools;

 public:
  Tools() = default;
  ~Tools() = default;

  Tools(const Tools&) = delete;
  Tools& operator=(const Tools&) = delete;

  Tools(Tools&&) = delete;
  Tools& operator=(Tools&&) = delete;

 public:
  //! \brief Print precision formatted value
  //! \tparam t_num Number type
  //! \param value Value of number type
  //! \param precision Precision (decimal places) to print
  //! \todo WARNING: cout precision appears to become innacurate around ~16-18?
  template <typename t_num = double>
  void print_value(const t_num value, const size_t precision = 8)
  {
    std::cout.precision(precision);
    std::cout << value << std::fixed << std::endl;
  }

  //! \brief Print random numbers with given intervals
  //! \tparam t_gen Random number generator type
  //! \tparam t_num Number type
  //! \param min Minimum value of number type
  //! \param max Maximum value of number type
  //! \param precision Decimal places to print
  //! \todo WARNING: cout precision appears to become innacurate around ~16-18?
  template <typename t_gen = ::TRandomMixMax, typename t_num = double>
  void print_dist(t_num min, t_num max, const size_t precision = 8)
  {
    // Specific to this use-case
    common::throw_ex_if<common::type::InvalidArgument>(
        min < 0 || max < 0, "no negative values allowed");

    if (min > max)
      {
        std::cout << "WARNING: min is greater than max, flipping" << std::endl;

        t_num tmp{min};

        min = max;
        max = tmp;
      }

    const std::vector<t_num> chunks{
        t_impl::random_dist<t_gen, t_num>(min, max, precision)};

    t_num total{};

    std::cout.precision(precision);
    for (const auto& chunk : chunks)
      {
        std::cout << chunk << std::fixed << std::endl;
        total += chunk;
      }

    std::cout << "Maximum = " << max << "\n"
              << "Printed = " << total << std::endl;
  }
};

//! \brief Byte transformer
//! \ingroup cpp_utils
//! \since docker-finance 1.0.0
template <typename t_decoded, typename t_encoded>
class Byte final : public impl::Byte
{
  using t_impl = impl::Byte;

 public:
  Byte() = default;
  ~Byte() = default;

  Byte(const Byte&) = default;
  Byte& operator=(const Byte&) = default;

  Byte(Byte&&) = default;
  Byte& operator=(Byte&&) = default;

 public:
  t_encoded encode(const t_decoded byte)
  {
    return t_impl::template encode<void, t_encoded>(byte);
  }

  template <
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_value_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<t_encoded> encode(const t_container<t_args...>& byte)
  {
    return t_impl::template encode<void, t_encoded>(byte);
  }

  template <
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_mapped_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<typename t_container<t_args...>::key_type, t_encoded> encode(
      const t_container<t_args...>& byte)
  {
    return t_impl::template encode<void, t_encoded>(byte);
  }

 public:
  t_decoded decode(const t_encoded byte)
  {
    return t_impl::template encode<void, t_decoded>(byte);
  }

  template <
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_value_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<t_decoded> decode(const t_container<t_args...>& byte)
  {
    return t_impl::template encode<void, t_decoded>(byte);
  }

  template <
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_mapped_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<typename t_container<t_args...>::key_type, t_decoded> decode(
      const t_container<t_args...>& byte)
  {
    return t_impl::template encode<void, t_decoded>(byte);
  }
};

}  // namespace utility
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
