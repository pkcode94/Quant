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

#ifndef CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_UTILITY_HH_
#define CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_UTILITY_HH_

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <vector>

// #include <calc/core.h>  // TODO(afiore): file upstream (calc) bug report
#include "../../random.hh"
#include "../generic.hh"
#include "../type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::utility
//! \brief Various docker-finance utilities
//! \since docker-finance 1.0.0
namespace utility
{
//! \namespace dfi::utility::impl
//! \brief Implementations of various docker-finance utilities
//! \since docker-finance 1.0.0
namespace impl
{
//! \brief Misc tools
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
class Tools
{
 public:
  Tools() = default;
  ~Tools() = default;

  Tools(const Tools&) = delete;
  Tools& operator=(const Tools&) = delete;

  Tools(Tools&&) = delete;
  Tools& operator=(Tools&&) = delete;

 private:
  //! \brief
  //!   TRandom-derived (ROOT) specialized random number generator
  //!
  //! \details
  //!   Generates uniformly distributed random value with intervals min and max
  //!
  //! \tparam t_gen ROOT TRandom derived generator
  //! \tparam t_num Floating point type to process
  //!
  //! \param min Minimum value of floating point type
  //! \param max Maximum value of floating type
  //!
  //! \note
  //!   You may be asking "Why seed with a CSPRNG?!" Answer: ROOT's Uniform()
  //!   (for most of the derived TRandom's) apparently *MUST* be seeded upon
  //!   every call, as TRandom-derived ctors apparently don't properly seed by
  //!   default. So, since only MT can be safely seeded with std::random_device
  //!   (or similar), Crypto++'s RNG works very well (it's incredibly inexpensive
  //!   (see benchmarks)) - thus guaranteeing that TRandom is properly seeded.
  //!
  //! \warning
  //!   This distribution is not cryptographically secure. Do not use for security purposes!
  template <
      typename t_gen,
      typename t_num,
      std::enable_if_t<
          std::is_base_of_v<::TRandom, t_gen>
              && std::is_floating_point_v<t_num>,
          bool> = true>
  t_num random_gen(const t_num min, const t_num max)
  {
    static_assert(
        !std::is_same_v<::TRandom, t_gen>,
        "should not instantiate directly, use derived classes");

    t_gen dist{};
    dist.SetSeed(m_random.generate());

    return dist.Uniform(min, max);
  }

  //! \brief
  //!   Uniform distribution with standard library's 32-bit Mersenne Twister
  //!
  //! \details
  //!   Generates uniformly distributed random value with intervals min and max
  //!
  //! \tparam t_gen Standard library generator
  //! \tparam t_num Floating point type to process
  //!
  //! \param min Minimum value of floating point type
  //! \param max Maximum value of floating type
  //!
  //! \note
  //!   Uniform distribution from the standard library. Intentionally avoids
  //!   deriving from TRandomGen as this is a proof-of-concept to allow for the
  //!   usage of non-ROOT implementations.
  //!
  //! \warning
  //!   This distribution is not cryptographically secure. Do not use for security purposes!
  template <
      typename t_gen,
      typename t_num,
      std::enable_if_t<
          std::is_same_v<t_gen, std::uniform_real_distribution<t_num>>,
          bool> = true,
      std::enable_if_t<std::is_floating_point_v<t_num>, bool> = true>
  t_num random_gen(const t_num min, const t_num max)
  {
    // TODO(unassigned): Crypto++ has an LCG
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<t_num> dist(min, max);
    return dist(gen);
  }

 protected:
  //! \brief
  //!   Generates set of randomly generated numbers with given intervals
  //!
  //! \tparam t_gen Random number generator type
  //! \tparam t_num Number type
  //!
  //! \param min Minimum value of number type
  //! \param max Maximum value of number type
  //! \param precision Decimal places to generate
  //!
  //! \return
  //!   Randomly generated numbers totaling max value of number type
  template <typename t_gen, typename t_num>
  std::vector<t_num>
  random_dist(const t_num min, const t_num max, const uint16_t precision = 8)
  {
    ::dfi::common::throw_ex_if<::dfi::common::type::InvalidArgument>(
        min > max, "minimum is greater than maximum");

    // Get first chunk
    std::vector<t_num> chunks;
    t_num chunk, sub;

    chunk = this->random_gen<t_gen, t_num>(min, max);
    chunks.push_back(chunk);

    // Get remaining chunks
    sub = max - chunk;

    while (sub > min)
      {
        chunk = this->random_gen<t_gen, t_num>(min, sub);
        chunks.push_back(chunk);
        sub = sub - chunk;
      }

    chunks.push_back(sub);

    // Check if maximum was fulfilled
    // NOTE: *MUST* round or else a difference of (for ex.) 4.94066e-324 will fail
    t_num total{}, pow{std::pow(10, precision)}, rounded{};

    total = std::accumulate(chunks.begin(), chunks.end(), t_num{0});
    rounded = std::round(total * pow) / pow;

    if (rounded != max)
      {
        std::cout.precision(precision);
        std::cout << "rounded = " << rounded << '\n'
                  << "max = " << max << std::fixed << std::endl;
        ::dfi::common::throw_ex<::dfi::common::type::RuntimeError>(
            "random distribution not fulfilled");
      }

    return chunks;
  }

 private:
  ::dfi::crypto::cryptopp::Random m_random;
};

//! \brief Implements Byte transformer
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
class Byte : public ::dfi::internal::Transform<Byte>
{
 public:
  Byte() = default;
  ~Byte() = default;

  Byte(const Byte&) = default;
  Byte& operator=(const Byte&) = default;

  Byte(Byte&&) = default;
  Byte& operator=(Byte&&) = default;

 public:
  //! \brief Implements Byte encoder
  template <typename t_tag, typename t_decoded, typename t_encoded>
  t_encoded encoder_impl(t_decoded arg)
  {
    static_assert(
        std::is_same_v<t_decoded, uint8_t>
        || std::is_same_v<t_encoded, uint8_t>);

    static_assert(
        std::is_same_v<t_decoded, std::byte>
        || std::is_same_v<t_encoded, std::byte>);

    return {static_cast<t_encoded>(arg)};
  }
};
}  // namespace impl
}  // namespace utility
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
