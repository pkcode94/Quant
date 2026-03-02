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
//! \todo C++20 refactor

#ifndef CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_RANDOM_HH_
#define CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_RANDOM_HH_

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <cryptopp/osrng.h>
#include <sodium.h>

#include <cstdint>
#include <limits>
#include <type_traits>

#include "../generic.hh"
#include "../type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::crypto
//! \brief Cryptographical libraries
//! \since docker-finance 1.0.0
namespace crypto
{
//! \namespace dfi::crypto::impl
//! \brief Cryptographical library implementations
//! \since docker-finance 1.0.0
namespace impl
{
//! \namespace dfi::crypto::impl::common
//! \brief Common implementation among all crypto implementations
//! \since docker-finance 1.0.0
namespace common
{
//! \brief Generic Random implementation common among all library-specific implementations
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo Random-type implementations only
template <typename t_impl>
class RandomImpl : public ::dfi::internal::Random<t_impl>
{
 public:
  RandomImpl() = default;
  ~RandomImpl() = default;

  RandomImpl(const RandomImpl&) = default;
  RandomImpl& operator=(const RandomImpl&) = default;

  RandomImpl(RandomImpl&&) = default;
  RandomImpl& operator=(RandomImpl&&) = default;

 public:
  template <typename t_random>
  t_random generate_impl()
  {
    static_assert(
        ::dfi::internal::type::is_real_integral<t_random>::value,
        "Random interface has changed");

    return this->t_impl::that()->template generate<t_random>();
  }
};
}  // namespace common

//! \namespace dfi::crypto::impl::botan
//! \since docker-finance 1.0.0
namespace botan
{
//! \brief Implements Random API with Botan
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo feed generate w/ buffer
//! \todo reseed
class Random : public common::RandomImpl<botan::Random>
{
 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = delete;
  Random& operator=(const Random&) = delete;

  Random(Random&&) = delete;
  Random& operator=(Random&&) = delete;

 private:
  //! \warning Our CRTP *SHOULD* provide external locks (which it does) for a
  //!   single generator (see Botan notes on its internal locks)
  ::Botan::AutoSeeded_RNG m_csprng;
  ::Botan::BigInt m_bigint;

 private:
  //! \brief Implements random number generator
  template <typename t_random>
  t_random generate_impl()
  {
    static_assert(
        ::dfi::internal::type::is_real_integral<t_random>::value,
        "Random interface has changed");

    static_assert(
        std::is_same_v<t_random, uint16_t>
            || std::is_same_v<t_random, uint32_t>,
        "Invalid type (only uint16_t or uint32_t supported)");

    ::dfi::common::throw_ex_if<::dfi::common::type::RuntimeError>(
        !m_csprng.is_seeded(), "Botan is not seeded");

    // WARNING: DO *NOT* set_high_bit to true here!
    // Otherwise, [0..(~2150*10^6)] WILL NOT BE GENERATED!
    // (see `rng_sample()` in macro, give a large sample)
    m_bigint.randomize(m_csprng, sizeof(t_random) * 8, false);
    return m_bigint.to_u32bit();
  }

 public:
  //! \brief Generate random number
  template <typename t_random>
  t_random generate()
  {
    return this->generate_impl<t_random>();
  }
};
}  // namespace botan

//! \namespace dfi::crypto::impl::cryptopp
//! \since docker-finance 1.0.0
namespace cryptopp
{
//! \brief Implements Random API with Crypto++
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo feed generate w/ buffer
//! \todo reseed
class Random : public common::RandomImpl<cryptopp::Random>
{
 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = delete;
  Random& operator=(const Random&) = delete;

  Random(Random&&) = delete;
  Random& operator=(Random&&) = delete;

 private:
  //! \warning Our CRTP *MUST* provide external locks (which it does) for a
  //!   single generator (see Crypto++ notes)
  ::CryptoPP::AutoSeededRandomPool m_csprng;

 private:
  //! \brief Implements random number generator
  template <typename t_random>
  t_random generate_impl()
  {
    // NOTE: signed/unsigned types are supported (up to uint32_t)
    static_assert(
        ::dfi::internal::type::is_real_integral<t_random>::value,
        "Random interface has changed");

    static_assert(
        !std::is_same_v<t_random, int64_t>
            && !std::is_same_v<t_random, uint64_t>,
        "Invalid type (no greater than uint32_t allowed)");

    return m_csprng.GenerateWord32(0, std::numeric_limits<t_random>::max());
  }

 public:
  //! \brief Generate random number
  template <typename t_random>
  t_random generate()
  {
    return this->generate_impl<t_random>();
  }
};
}  // namespace cryptopp

//! \namespace dfi::crypto::impl::libsodium
//! \since docker-finance 1.0.0
namespace libsodium
{
//! \brief Implements Random API with libsodium
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo feed generate w/ buffer
//! \todo reseed
class Random : public common::RandomImpl<libsodium::Random>
{
 public:
  //! \note
  //!   From the docs:
  //!
  //!   "sodium_init() initializes the library and should be called before any
  //!   other function provided by Sodium. It is safe to call this function
  //!   more than once and from different threads - subsequent calls won't
  //!   have any effects."
  Random()
  {
    ::dfi::common::throw_ex_if<::dfi::common::type::RuntimeError>(
        ::sodium_init() < 0, "sodium_init could not be initialized");
  }
  ~Random() = default;

  Random(const Random&) = default;
  Random& operator=(const Random&) = default;

  Random(Random&&) = default;
  Random& operator=(Random&&) = default;

 private:
  //! \brief Implements random number generator
  template <typename t_random>
  t_random generate_impl()
  {
    static_assert(
        ::dfi::internal::type::is_real_integral<t_random>::value,
        "Random interface has changed");

    // signed or uint64_t types are not supported
    static_assert(
        std::is_same_v<t_random, uint16_t>
            || std::is_same_v<t_random, uint32_t>,
        "Invalid type (only uint16_t or uint32_t supported)");

    return ::randombytes_random();
  }

 public:
  //! \brief Generate random number
  template <typename t_random>
  t_random generate()
  {
    return this->generate_impl<t_random>();
  }
};
}  // namespace libsodium

#ifdef __DFI_PLUGIN_BITCOIN__
//! \namespace dfi::crypto::impl::bitcoin
//! \since docker-finance 1.1.0
namespace bitcoin
{
//! \concept dfi::crypto::impl::bitcoin::RType
//! \brief Random number type
//! \details Requirements include that "interface" specialization
//!   has not changed and that given type is supported by bitcoin.
//! \since docker-finance 1.1.0
template <typename t_random>
concept RType =
    ::dfi::internal::type::is_real_integral<t_random>::value
    && (std::same_as<t_random, uint32_t> || std::same_as<t_random, uint64_t>);

//! \brief Implements Random API with bitcoin
//! \ingroup cpp_API_impl
//! \since docker-finance 1.1.0
//! \todo span/bytes
//! \todo reseed
//! \todo insecure RNG option
class Random : public common::RandomImpl<bitcoin::Random>
{
 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = delete;
  Random& operator=(const Random&) = delete;

  Random(Random&&) = delete;
  Random& operator=(Random&&) = delete;

 private:
  //! \brief Implements random number generator
  template <RType t_random>
  t_random generate_impl()
  {
    if constexpr (std::same_as<t_random, uint32_t>)
      {
        return m_ctx.rand32();
      }
    else if constexpr (std::same_as<t_random, uint64_t>)
      {
        return m_ctx.rand64();
      }
  }

 public:
  //! \brief Generate random number
  template <RType t_random>
  t_random generate()
  {
    return this->generate_impl<t_random>();
  }

 private:
  //! \warning Bitcoin's RNG is not thread-safe (but dfi's CRTP provides external locks, by default)
  ::FastRandomContext m_ctx;
};
}  // namespace bitcoin
#endif

}  // namespace impl
}  // namespace crypto
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_RANDOM_HH_

// # vim: sw=2 sts=2 si ai et
