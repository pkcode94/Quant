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

#ifndef CONTAINER_SRC_ROOT_MACRO_CRYPTO_HASH_C_
#define CONTAINER_SRC_ROOT_MACRO_CRYPTO_HASH_C_

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
//! \brief Hash generator macro
//! \since docker-finance 1.0.0
class Hash final
{
  //! Text description of Hash impl, encoded digest
  using t_hash = std::map<std::string, std::string>;

 public:
  Hash() = default;
  ~Hash() = default;

  Hash(const Hash&) = delete;
  Hash& operator=(const Hash&) = delete;

  Hash(Hash&&) = default;
  Hash& operator=(Hash&&) = default;

 protected:
  //! \brief Botan Hash encoder
  //! \param message Message to encode
  //! \return t_hash Hash map to print (label, digest)
  static t_hash botan_encode(const std::string& message)
  {
    t_hash hash;

    //
    // BLAKE2
    //

    hash["botan::Hash::BLAKE2b"] =
        botan::g_Hash->encode<botan::Hash::BLAKE2b>(message);

    //
    // Keccak
    //

    hash["botan::Hash::Keccak_224"] =
        botan::g_Hash->encode<botan::Hash::Keccak_224>(message);
    hash["botan::Hash::Keccak_256"] =
        botan::g_Hash->encode<botan::Hash::Keccak_256>(message);
    hash["botan::Hash::Keccak_384"] =
        botan::g_Hash->encode<botan::Hash::Keccak_384>(message);
    hash["botan::Hash::Keccak_512"] =
        botan::g_Hash->encode<botan::Hash::Keccak_512>(message);

    //
    // SHA
    //

    hash["botan::Hash::SHA1"] =
        botan::g_Hash->encode<botan::Hash::SHA1>(message);

    hash["botan::Hash::SHA2_224"] =
        botan::g_Hash->encode<botan::Hash::SHA2_224>(message);
    hash["botan::Hash::SHA2_256"] =
        botan::g_Hash->encode<botan::Hash::SHA2_256>(message);
    hash["botan::Hash::SHA2_384"] =
        botan::g_Hash->encode<botan::Hash::SHA2_384>(message);
    hash["botan::Hash::SHA2_512"] =
        botan::g_Hash->encode<botan::Hash::SHA2_512>(message);

    hash["botan::Hash::SHA3_224"] =
        botan::g_Hash->encode<botan::Hash::SHA3_224>(message);
    hash["botan::Hash::SHA3_256"] =
        botan::g_Hash->encode<botan::Hash::SHA3_256>(message);
    hash["botan::Hash::SHA3_384"] =
        botan::g_Hash->encode<botan::Hash::SHA3_384>(message);
    hash["botan::Hash::SHA3_512"] =
        botan::g_Hash->encode<botan::Hash::SHA3_512>(message);

    //
    // SHAKE
    //

    hash["botan::Hash::SHAKE128"] =
        botan::g_Hash->encode<botan::Hash::SHAKE128>(message);
    hash["botan::Hash::SHAKE256"] =
        botan::g_Hash->encode<botan::Hash::SHAKE256>(message);

    return hash;
  }

  //! \brief Crypto++ Hash encoder
  //! \param message Message to encode
  //! \return t_hash Hash map to print (label, digest)
  static t_hash cryptopp_encode(const std::string& message)
  {
    t_hash hash;

    //
    // BLAKE2
    //

    hash["cryptopp::Hash::BLAKE2b"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::BLAKE2b>(message);
    hash["cryptopp::Hash::BLAKE2s"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::BLAKE2s>(message);

    //
    // Keccak
    //

    hash["cryptopp::Hash::Keccak_224"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::Keccak_224>(message);
    hash["cryptopp::Hash::Keccak_256"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::Keccak_256>(message);
    hash["cryptopp::Hash::Keccak_384"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::Keccak_384>(message);
    hash["cryptopp::Hash::Keccak_512"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::Keccak_512>(message);

    //
    // SHA
    //

    hash["cryptopp::Hash::SHA1"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA1>(message);

    hash["cryptopp::Hash::SHA2_224"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA2_224>(message);
    hash["cryptopp::Hash::SHA2_256"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA2_256>(message);
    hash["cryptopp::Hash::SHA2_384"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA2_384>(message);
    hash["cryptopp::Hash::SHA2_512"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA2_512>(message);

    hash["cryptopp::Hash::SHA3_224"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA3_224>(message);
    hash["cryptopp::Hash::SHA3_256"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA3_256>(message);
    hash["cryptopp::Hash::SHA3_384"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA3_384>(message);
    hash["cryptopp::Hash::SHA3_512"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHA3_512>(message);

    //
    // SHAKE
    //

    hash["cryptopp::Hash::SHAKE128"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHAKE128>(message);
    hash["cryptopp::Hash::SHAKE256"] =
        cryptopp::g_Hash->encode<cryptopp::Hash::SHAKE256>(message);

    return hash;
  }

  //! \brief libsodium Hash encoder
  //! \param message Message to encode
  //! \return t_hash Hash map to print (label, digest)
  static t_hash libsodium_encode(const std::string& message)
  {
    t_hash hash;

    //
    // BLAKE2
    //

    hash["libsodium::Hash::BLAKE2b"] =
        libsodium::g_Hash->encode<libsodium::Hash::BLAKE2b>(message);

    //
    // SHA
    //

    hash["libsodium::Hash::SHA2_256"] =
        libsodium::g_Hash->encode<libsodium::Hash::SHA2_256>(message);
    hash["libsodium::Hash::SHA2_512"] =
        libsodium::g_Hash->encode<libsodium::Hash::SHA2_512>(message);

    return hash;
  }

 public:
  //! \brief Print t_hash of given message in CSV format
  //! \param message Message to encode
  static void encode(const std::string& message)
  {
    std::cout << "\nNOTE: outer quotes (that contain the message) will not be "
                 "digested. Use escapes to digest literal quotes."
              << std::endl;

    auto print = [](const std::string& message, const t_hash& hash) {
      for (const auto& [impl, digest] : hash)
        {
          std::cout << impl << ",\"" << message << "\"," << digest << "\n";
        }
    };

    std::cout << "\nimpl,message,digest\n";
    print(message, Hash::botan_encode(message));
    print(message, Hash::cryptopp_encode(message));
    print(message, Hash::libsodium_encode(message));
    std::cout << std::flush;
  }

  //! \brief Wrapper to encoder
  //! \param message Message to encode
  static void run(const std::string& message) { Hash::encode(message); }
};

//! \brief Pluggable entrypoint
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.1.0
class hash_C final
{
 public:
  hash_C() = default;
  ~hash_C() = default;

  hash_C(const hash_C&) = default;
  hash_C& operator=(const hash_C&) = default;

  hash_C(hash_C&&) = default;
  hash_C& operator=(hash_C&&) = default;

 public:
  //! \brief Macro auto-loader
  //! \param arg Message to encode
  static void load(const std::string& arg = {})
  {
    std::string message{arg};

    // If no message is given, default to unique profile/subprofile
    if (message.empty())
      {
        message += ::dfi::common::get_env("global_parent_profile");
        message += ::dfi::common::get_env("global_arg_delim_1");
        message += ::dfi::common::get_env("global_child_profile");
      }

    Hash::run(message);
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

#endif  // CONTAINER_SRC_ROOT_MACRO_CRYPTO_HASH_C_

// # vim: sw=2 sts=2 si ai et
