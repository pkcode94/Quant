// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2024-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

#ifndef CONTAINER_SRC_ROOT_MACRO_CRYPTO_RANDOM_C_
#define CONTAINER_SRC_ROOT_MACRO_CRYPTO_RANDOM_C_

#include <iostream>
#include <map>
#include <string>

#include "../common/crypto.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \namespace dfi::macro::crypto
//! \brief ROOT cryptographic-based macros
//! \since docker-finance 1.0.0
namespace crypto
{
namespace common = ::dfi::macro::common;
namespace libsodium = common::crypto::libsodium;
namespace cryptopp = common::crypto::cryptopp;
namespace botan = common::crypto::botan;
#ifdef __DFI_PLUGIN_BITCOIN__
namespace bitcoin = common::crypto::bitcoin;
#endif
//! \brief CSPRNG macro
//! \since docker-finance 1.0.0
class Random final
{
  //! Text description of number type, random number
  using t_rng = std::map<std::string, size_t>;

 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = delete;
  Random& operator=(const Random&) = delete;

  Random(Random&&) = default;
  Random& operator=(Random&&) = default;

 protected:
  //! \brief Botan RNG
  //! \return t_rng Random map to print (label, num)
  static t_rng botan_generate()
  {
    t_rng rng;

    rng["int16_t (unsupported)"] = 0;
    rng["int32_t (unsupported)"] = 0;

    rng["uint16_t"] = botan::g_Random->generate<uint16_t>();
    rng["uint32_t"] = botan::g_Random->generate<uint32_t>();

    return rng;
  }

  //! \brief Crypto++ RNG
  //! \return t_rng Random map to print (label, num)
  static t_rng cryptopp_generate()
  {
    t_rng rng;

    rng["int16_t"] = cryptopp::g_Random->generate<int16_t>();
    rng["int32_t"] = cryptopp::g_Random->generate<int32_t>();

    rng["uint16_t"] = cryptopp::g_Random->generate<uint16_t>();
    rng["uint32_t"] = cryptopp::g_Random->generate<uint32_t>();

    return rng;
  }

  //! \brief libsodium RNG
  //! \return t_rng Random map to print (label, num)
  static t_rng libsodium_generate()
  {
    t_rng rng;

    rng["int16_t (unsupported)"] = 0;
    rng["int32_t (unsupported)"] = 0;

    rng["uint16_t"] = libsodium::g_Random->generate<uint16_t>();
    rng["uint32_t"] = libsodium::g_Random->generate<uint32_t>();

    return rng;
  }

#ifdef __DFI_PLUGIN_BITCOIN__
  //! \brief bitcon RNG
  //! \return t_rng Random map to print (label, num)
  static t_rng bitcoin_generate()
  {
    t_rng rng;

    rng["int16_t (unsupported)"] = 0;
    rng["int32_t (unsupported)"] = 0;

    rng["uint16_t (unsupported)"] = 0;
    rng["uint32_t"] = bitcoin::g_Random->generate<uint32_t>();
    rng["uint64_t"] = bitcoin::g_Random->generate<uint64_t>();

    return rng;
  }
#endif

 public:
  //! \brief Print t_rng of CSPRNG numbers in CSV format
  static void generate()
  {
    auto print = [](const std::string& impl, const t_rng& rng) {
      for (const auto& [type, num] : rng)
        {
          std::cout << impl << "," << type << "," << num << "\n";
        }
    };

    std::cout << "\nimpl,type,num\n";
#ifdef __DFI_PLUGIN_BITCOIN__
    print("bitcoin::Random", Random::bitcoin_generate());
#endif
    print("botan::Random", Random::botan_generate());
    print("cryptopp::Random", Random::cryptopp_generate());
    print("libsodium::Random", Random::libsodium_generate());
    std::cout << std::flush;
  }

  //! \brief Wrapper to Random generator
  static void run() { Random::generate(); }
};

//! \brief Pluggable entrypoint
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.1.0
class random_C final
{
 public:
  random_C() = default;
  ~random_C() = default;

  random_C(const random_C&) = default;
  random_C& operator=(const random_C&) = default;

  random_C(random_C&&) = default;
  random_C& operator=(random_C&&) = default;

 public:
  //! \brief Macro auto-loader
  //! \param arg Optional code to execute after loading
  static void load(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);

    Random::run();
  }

  //! \brief Macro auto-unloader
  //! \param arg Optional code to execute before unloading
  static void unload(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);
  }
};
}  // namespace crypto
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_CRYPTO_RANDOM_C_

// # vim: sw=2 sts=2 si ai et
