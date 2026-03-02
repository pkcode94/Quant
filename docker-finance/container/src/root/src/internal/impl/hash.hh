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

#ifndef CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_HASH_HH_
#define CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_HASH_HH_

#include <botan/hash.h>
#include <botan/hex.h>
#include <cryptopp/blake2.h>
#include <cryptopp/filters.h>  // StringSource
#include <cryptopp/hex.h>
#include <cryptopp/keccak.h>
#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>
#include <cryptopp/shake.h>
#include <sodium.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

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
//! \brief Generic Hash implementation common among all library-specific implementations
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo Hash-type implementations only
template <typename t_impl>
class HashImpl : public ::dfi::internal::Transform<t_impl>
{
 public:
  HashImpl() = default;
  ~HashImpl() = default;

  HashImpl(const HashImpl&) = default;
  HashImpl& operator=(const HashImpl&) = default;

  HashImpl(HashImpl&&) = default;
  HashImpl& operator=(HashImpl&&) = default;

 public:
  //! \brief Implements trivial type Hash encoder
  //! \since docker-finance 1.0.0
  template <
      typename t_hash,
      typename t_decoded,
      typename t_encoded,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_trivial<t_decoded>,
          bool> = true>
  t_encoded encoder_impl(const t_decoded decoded)
  {
    static_assert(
        std::is_same_v<t_encoded, std::string>, "Hash interface has changed");

    // Massage the decoded message
    t_encoded message;

    if constexpr (
        std::is_same_v<t_decoded, std::string_view>
        || std::is_pointer_v<t_decoded>)
      {
        message.assign(decoded);
      }
    else if constexpr (std::is_same_v<t_decoded, std::byte>)
      {
        message.push_back(static_cast<unsigned char>(decoded));
      }
    else if constexpr (::dfi::internal::type::is_real_integral<
                           t_decoded>::value)
      {
        message.assign(std::to_string(decoded));
      }
    else
      {
        message.push_back(decoded);
      }

    return this->t_impl::that()->template hash<t_hash, t_encoded>(message);
  }

  //! \brief
  //!   Implements non-trivial type Hash encoder
  //!
  //! \note
  //!   The data that's passed is not the container but the actual Type
  //!   of the container (std::byte, std::vector<T>::value_type, etc.)
  //!
  //! \since docker-finance 1.0.0
  template <
      typename t_hash,
      typename t_decoded,
      typename t_encoded,
      std::enable_if_t<
          ::dfi::internal::type::has_const_iterator<t_decoded>::value
              && !std::is_same_v<t_decoded, std::string_view>,
          bool> = true>
  t_encoded encoder_impl(const t_decoded& decoded)
  {
    static_assert(
        std::is_same_v<t_encoded, std::string>, "Hash interface has changed");

    return this->t_impl::that()->template hash<t_hash, t_encoded>(decoded);
  }
};
}  // namespace common

//! \namespace dfi::crypto::impl::botan
//! \since docker-finance 1.0.0
namespace botan
{
//! \brief Implements Hash API with Botan
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo Support large/split messages
class Hash : public common::HashImpl<botan::Hash>,
             public ::dfi::internal::type::Hash
{
 public:
  Hash() = default;
  ~Hash() = default;

  Hash(const Hash&) = default;
  Hash& operator=(const Hash&) = default;

  Hash(Hash&&) = default;
  Hash& operator=(Hash&&) = default;

 private:
  template <typename t_hash, typename t_encoded>
  t_encoded hash_impl(const std::string& hash_name, const t_encoded& message)
  {
    static_assert(
        std::is_same_v<t_encoded, std::string>, "Hash interface has changed");

    // Compute digest
    // NOTE: Botan's interface has no ctor for creating hash...
    auto hash = ::Botan::HashFunction::create_or_throw(hash_name);
    hash->update(
        reinterpret_cast<const uint8_t*>(message.data()), message.size());

    return ::Botan::hex_encode(hash->final(), false /* lowercase */);
  }

 public:
  //! \brief Create encoded digest
  template <typename t_hash, typename t_encoded>
  t_encoded hash(const t_encoded& message)
  {
    // BLAKE2
    if constexpr (std::is_same_v<t_hash, Hash::BLAKE2b>)
      {
        return this->hash_impl<t_encoded>("BLAKE2b", message);
      }
    // Keccak
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_224>)
      {
        return this->hash_impl<t_encoded>("Keccak-1600(224)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_256>)
      {
        return this->hash_impl<t_encoded>("Keccak-1600(256)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_384>)
      {
        return this->hash_impl<t_encoded>("Keccak-1600(384)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_512>)
      {
        return this->hash_impl<t_encoded>("Keccak-1600(512)", message);
      }
    // SHA1
    else if constexpr (std::is_same_v<t_hash, Hash::SHA1>)
      {
        return this->hash_impl<t_encoded>("SHA-1", message);
      }
    // SHA2
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_224>)
      {
        return this->hash_impl<t_encoded>("SHA-224", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_256>)
      {
        return this->hash_impl<t_encoded>("SHA-256", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_384>)
      {
        return this->hash_impl<t_encoded>("SHA-384", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_512>)
      {
        return this->hash_impl<t_encoded>("SHA-512", message);
      }
    // SHA3
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_224>)
      {
        return this->hash_impl<t_encoded>("SHA-3(224)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_256>)
      {
        return this->hash_impl<t_encoded>("SHA-3(256)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_384>)
      {
        return this->hash_impl<t_encoded>("SHA-3(384)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_512>)
      {
        return this->hash_impl<t_encoded>("SHA-3(512)", message);
      }
    // SHAKE
    else if constexpr (std::is_same_v<t_hash, Hash::SHAKE128>)
      {
        return this->hash_impl<t_encoded>("SHAKE-128(256)", message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHAKE256>)
      {
        return this->hash_impl<t_encoded>("SHAKE-256(512)", message);
      }
    // Unsupported
    else
      {
        // NOTE: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2593r0.html
        static_assert(
            std::is_same_v<t_hash, Hash>, "Unsupported hash function");
      }
  }
};
}  // namespace botan

//! \namespace dfi::crypto::impl::cryptopp
//! \since docker-finance 1.0.0
namespace cryptopp
{
//! \brief Implements Hash API with Crypto++
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo Support large/split messages
class Hash : public common::HashImpl<cryptopp::Hash>,
             public ::dfi::internal::type::Hash
{
 public:
  Hash() = default;
  ~Hash() = default;

  Hash(const Hash&) = default;
  Hash& operator=(const Hash&) = default;

  Hash(Hash&&) = default;
  Hash& operator=(Hash&&) = default;

 private:
  //! \todo optimize
  template <typename t_hash, typename t_encoded>
  t_encoded hash_impl(const t_encoded& message)
  {
    static_assert(
        std::is_same_v<t_encoded, std::string>, "Hash interface has changed");

    // Raw digest
    std::vector<std::byte> digest;

    // Compute digest
    t_hash hash;

    hash.Update((const ::CryptoPP::byte*)message.data(), message.size());
    digest.resize(hash.DigestSize());
    hash.Final(reinterpret_cast<::CryptoPP::byte*>(digest.data()));

    // Encode digest
    ::CryptoPP::HexEncoder encoder(nullptr, false /* lowercase */);
    t_encoded encoded{};

    encoder.Attach(new ::CryptoPP::StringSink(encoded));
    encoder.Put(
        reinterpret_cast<::CryptoPP::byte*>(digest.data()), digest.size());
    encoder.MessageEnd();

    return encoded;
  }

 public:
  //! \brief Create encoded digest
  template <typename t_hash, typename t_encoded>
  t_encoded hash(const t_encoded& message)
  {
    // BLAKE2
    if constexpr (std::is_same_v<t_hash, Hash::BLAKE2s>)
      {
        return this->hash_impl<::CryptoPP::BLAKE2s, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::BLAKE2b>)
      {
        return this->hash_impl<::CryptoPP::BLAKE2b, t_encoded>(message);
      }
    // Keccak
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_224>)
      {
        return this->hash_impl<::CryptoPP::Keccak_224, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_256>)
      {
        return this->hash_impl<::CryptoPP::Keccak_256, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_384>)
      {
        return this->hash_impl<::CryptoPP::Keccak_384, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::Keccak_512>)
      {
        return this->hash_impl<::CryptoPP::Keccak_512, t_encoded>(message);
      }
    // SHA1
    else if constexpr (std::is_same_v<t_hash, Hash::SHA1>)
      {
        return this->hash_impl<::CryptoPP::SHA1, t_encoded>(message);
      }
    // SHA2
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_224>)
      {
        return this->hash_impl<::CryptoPP::SHA224, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_256>)
      {
        return this->hash_impl<::CryptoPP::SHA256, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_384>)
      {
        return this->hash_impl<::CryptoPP::SHA384, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_512>)
      {
        return this->hash_impl<::CryptoPP::SHA512, t_encoded>(message);
      }
    // SHA3
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_224>)
      {
        return this->hash_impl<::CryptoPP::SHA3_224, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_256>)
      {
        return this->hash_impl<::CryptoPP::SHA3_256, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_384>)
      {
        return this->hash_impl<::CryptoPP::SHA3_384, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA3_512>)
      {
        return this->hash_impl<::CryptoPP::SHA3_512, t_encoded>(message);
      }
    // SHAKE
    else if constexpr (std::is_same_v<t_hash, Hash::SHAKE128>)
      {
        return this->hash_impl<::CryptoPP::SHAKE128, t_encoded>(message);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHAKE256>)
      {
        return this->hash_impl<::CryptoPP::SHAKE256, t_encoded>(message);
      }
    // Unsupported
    else
      {
        // NOTE: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2593r0.html
        static_assert(
            std::is_same_v<t_hash, Hash>, "Unsupported hash function");
      }
  }
};
}  // namespace cryptopp

//! \namespace dfi::crypto::impl::libsodium
//! \since docker-finance 1.0.0
namespace libsodium
{
//! \brief Implements Hash API with libsodium
//! \ingroup cpp_API_impl
//! \since docker-finance 1.0.0
//! \todo Support large/split messages
class Hash : public common::HashImpl<libsodium::Hash>,
             public ::dfi::internal::type::Hash
{
 public:
  //! \note
  //!   From the docs:
  //!
  //!   "sodium_init() initializes the library and should be called before any
  //!   other function provided by Sodium. It is safe to call this function
  //!   more than once and from different threads - subsequent calls won't
  //!   have any effects."
  Hash()
  {
    ::dfi::common::throw_ex_if<::dfi::common::type::RuntimeError>(
        ::sodium_init() < 0, "sodium_init could not be initialized");
  }
  ~Hash() = default;

  Hash(const Hash&) = default;
  Hash& operator=(const Hash&) = default;

  Hash(Hash&&) = default;
  Hash& operator=(Hash&&) = default;

 private:
  template <typename t_hash, typename t_encoded>
  t_encoded hash_impl(const t_encoded& message)
  {
    static_assert(
        std::is_same_v<t_encoded, std::string>, "Hash interface has changed");

    // Raw digest
    std::vector<std::byte> digest;

    // Compute digest
    if constexpr (std::is_same_v<t_hash, Hash::BLAKE2b>)
      {
        digest.resize(crypto_generichash_BYTES_MAX);
        ::crypto_generichash(
            reinterpret_cast<uint8_t*>(digest.data()),
            digest.size(),
            reinterpret_cast<const uint8_t*>(message.data()),
            message.size(),
            NULL,
            0);
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_256>)
      {
        digest.resize(crypto_hash_sha256_BYTES);
        ::crypto_hash_sha256(
            reinterpret_cast<uint8_t*>(digest.data()),
            reinterpret_cast<const uint8_t*>(message.data()),
            message.size());
      }
    else if constexpr (std::is_same_v<t_hash, Hash::SHA2_512>)
      {
        digest.resize(crypto_hash_sha512_BYTES);
        ::crypto_hash_sha512(
            reinterpret_cast<uint8_t*>(digest.data()),
            reinterpret_cast<const uint8_t*>(message.data()),
            message.size());
      }
    else
      {
        // NOTE: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2593r0.html
        static_assert(
            std::is_same_v<t_hash, Hash>, "Unsupported hash function");
      }

    // Encode digest
    t_encoded encoded(digest.size() * 2 + 1, ' ');
    ::sodium_bin2hex(
        encoded.data(),
        encoded.size(),
        reinterpret_cast<const uint8_t*>(digest.data()),
        digest.size());

    // Remove libsodium's EOL null byte
    encoded.resize(encoded.find('\0'));

    return encoded;
  }

 public:
  //! \brief Create encoded digest
  template <typename t_hash, typename t_encoded>
  t_encoded hash(const t_encoded& message)
  {
    return this->hash_impl<t_hash, t_encoded>(message);
  }
};
}  // namespace libsodium

}  // namespace impl
}  // namespace crypto
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_INTERNAL_IMPL_HASH_HH_

// # vim: sw=2 sts=2 si ai et
