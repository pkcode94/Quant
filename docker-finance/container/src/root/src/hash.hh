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

#ifndef CONTAINER_SRC_ROOT_SRC_HASH_HH_
#define CONTAINER_SRC_ROOT_SRC_HASH_HH_

#include <string>
#include <type_traits>

#include "./internal/impl/hash.hh"
#include "./internal/type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::crypto
//! \brief Cryptographical libraries
//! \since docker-finance 1.0.0
namespace crypto
{
//! \namespace dfi::crypto::common
//! \brief Common "interface" (specializations) to library-specific implementations
//! \warning Not for direct public consumption (use library namespace instead)
//! \since docker-finance 1.0.0
namespace common
{
//! \brief Common Hash API "interface" (specializations)
//! \tparam t_impl Underlying Hash implementation
//! \warning Not for direct public consumption (use library aliases instead)
//! \ingroup cpp_API
//! \since docker-finance 1.0.0
//! \todo Specialization for non-const pointer parameter (so no return copy is needed)?
//! \todo Hash verify functions
//! \todo NPI to set/get cryptographical parameters
template <typename t_impl>
class Hash final : public t_impl
{
 public:
  Hash() = default;
  ~Hash() = default;

  Hash(const Hash&) = default;
  Hash& operator=(const Hash&) = default;

  Hash(Hash&&) = default;
  Hash& operator=(Hash&&) = default;

 public:
  //! \brief Hash encode trivial types and std::string_view
  //!
  //! \tparam t_hash Hash algorithm type from internal::type::Hash
  //! \tparam t_message Message type to encode (automatically deduced)
  //!
  //! \details
  //!   <b>Example:</b><br>
  //!   \code{.cpp}
  //!     // Hash encode with SHA256
  //!     using Hash = Hash<::impl::Hash>;  // inherits internal::type::Hash
  //!     Hash hash; std::byte byte{data};
  //!     auto encoded = hash.encode<Hash::SHA2_256>(byte);
  //!   \endcode
  template <
      typename t_hash,
      typename t_message,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_trivial<t_message>,
          bool> = true>
  std::string encode(const t_message message)
  {
    return t_impl::template encode<t_hash, std::string>(message);
  }

  //! \brief Hash encode string parameter
  //!
  //! \tparam t_hash Hash algorithm type from internal::type::Hash
  //! \tparam t_container Container type to encode (automatically deduced)
  //!
  //! \details
  //!   <b>Example:</b><br>
  //!   \code{.cpp}
  //!     // Hash encode with SHA256
  //!     using Hash = Hash<::impl::Hash>;  // inherits internal::type::Hash
  //!     Hash hash; std::string str{data};
  //!     auto encoded = hash.encode<Hash::SHA2_256>(str);
  //!   \endcode
  template <
      typename t_hash,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_string<
              t_container<t_args...>>,
          bool> = true>
  std::string encode(const t_container<t_args...>& message)
  {
    return t_impl::template encode<t_hash, std::string>(message);
  }

  //! \brief Hash encode allocator-aware non-associative containers
  //!
  //! \tparam t_hash Hash algorithm type from internal::type::Hash
  //! \tparam t_container Container type to encode (automatically deduced)
  //!
  //! \details
  //!   <b>Example:</b><br>
  //!   \code{.cpp}
  //!     // Hash encode with SHA256
  //!     using Hash = Hash<::impl::Hash>;  // inherits internal::type::Hash
  //!     Hash hash; std::vector<std::string> vec{data};
  //!     auto encoded = hash.encode<Hash::SHA2_256>(vec);
  //!   \endcode
  template <
      typename t_hash,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_value_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<std::string> encode(const t_container<t_args...>& message)
  {
    return t_impl::template encode<t_hash, std::string>(message);
  }

  //! \brief Hash encode allocator-aware associative containers
  //!
  //! \tparam t_hash Hash algorithm type from internal::type::Hash
  //! \tparam t_container Container type to encode (automatically deduced)
  //!
  //! \details
  //!   <b>Example:</b><br>
  //!   \code{.cpp}
  //!     // Hash encode with SHA256
  //!     using Hash = Hash<::impl::Hash>;  // inherits internal::type::Hash
  //!     Hash hash; std::map<uint8_t, std::string> map{{data}};
  //!     auto encoded = hash.encode<Hash::SHA2_256>(map);
  //!   \endcode
  template <
      typename t_hash,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          ::dfi::internal::type::is_signature_with_mapped_type_allocator<
              t_container<t_args...>>,
          bool> = true>
  t_container<typename t_container<t_args...>::key_type, std::string> encode(
      const t_container<t_args...>& message)
  {
    return t_impl::template encode<t_hash, std::string>(message);
  }
};
}  // namespace common

//! \namespace dfi::crypto::botan
//! \brief Public-facing API namespace (Botan)
//! \since docker-finance 1.0.0
namespace botan
{
//! \brief Botan Hash API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \details
//!   Supported algorithms:
//!     <br>&emsp; \ref internal::type::Hash::BLAKE2b
//!     <br>&emsp; \ref internal::type::Hash::Keccak_224
//!     <br>&emsp; \ref internal::type::Hash::Keccak_256
//!     <br>&emsp; \ref internal::type::Hash::Keccak_384
//!     <br>&emsp; \ref internal::type::Hash::Keccak_512
//!     <br>&emsp; \ref internal::type::Hash::SHA1
//!     <br>&emsp; \ref internal::type::Hash::SHA2_224
//!     <br>&emsp; \ref internal::type::Hash::SHA2_256
//!     <br>&emsp; \ref internal::type::Hash::SHA2_384
//!     <br>&emsp; \ref internal::type::Hash::SHA2_512
//!     <br>&emsp; \ref internal::type::Hash::SHA3_224
//!     <br>&emsp; \ref internal::type::Hash::SHA3_256
//!     <br>&emsp; \ref internal::type::Hash::SHA3_384
//!     <br>&emsp; \ref internal::type::Hash::SHA3_512
//!     <br>&emsp; \ref internal::type::Hash::SHAKE128
//!     <br>&emsp; \ref internal::type::Hash::SHAKE256
//!
//!   <b>Example:</b><br>
//!   \code{.cpp}
//!     // Hash encode with SHA256
//!     using Hash = dfi::crypto::botan::Hash;  // inherits internal::type::Hash
//!     Hash hash; hash.encode<Hash::SHA2_256>(data);
//!   \endcode
//!
//! \see crypto::common::Hash for API details
//! \note For public consumption
//! \since docker-finance 1.0.0
using Hash = ::dfi::crypto::common::Hash<impl::botan::Hash>;
}  // namespace botan

//! \namespace dfi::crypto::cryptopp
//! \brief Public-facing API namespace (Crypto++)
//! \since docker-finance 1.0.0
namespace cryptopp
{
//! \brief Crypto++ Hash API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \details
//!   Supported algorithms:
//!     <br>&emsp; \ref internal::type::Hash::BLAKE2b
//!     <br>&emsp; \ref internal::type::Hash::BLAKE2s
//!     <br>&emsp; \ref internal::type::Hash::Keccak_224
//!     <br>&emsp; \ref internal::type::Hash::Keccak_256
//!     <br>&emsp; \ref internal::type::Hash::Keccak_384
//!     <br>&emsp; \ref internal::type::Hash::Keccak_512
//!     <br>&emsp; \ref internal::type::Hash::SHA1
//!     <br>&emsp; \ref internal::type::Hash::SHA2_224
//!     <br>&emsp; \ref internal::type::Hash::SHA2_256
//!     <br>&emsp; \ref internal::type::Hash::SHA2_384
//!     <br>&emsp; \ref internal::type::Hash::SHA2_512
//!     <br>&emsp; \ref internal::type::Hash::SHA3_224
//!     <br>&emsp; \ref internal::type::Hash::SHA3_256
//!     <br>&emsp; \ref internal::type::Hash::SHA3_384
//!     <br>&emsp; \ref internal::type::Hash::SHA3_512
//!     <br>&emsp; \ref internal::type::Hash::SHAKE128
//!     <br>&emsp; \ref internal::type::Hash::SHAKE256
//!
//!   <b>Example:</b><br>
//!   \code{.cpp}
//!     // Hash encode with SHA256
//!     using Hash = dfi::crypto::cryptopp::Hash;  // inherits internal::type::Hash
//!     Hash hash; hash.encode<Hash::SHA2_256>(data);
//!   \endcode
//!
//! \see crypto::common::Hash for API details
//! \note For public consumption
//! \since docker-finance 1.0.0
using Hash = ::dfi::crypto::common::Hash<impl::cryptopp::Hash>;
}  // namespace cryptopp

//! \namespace dfi::crypto::libsodium
//! \brief Public-facing API namespace (libsodium)
//! \since docker-finance 1.0.0
namespace libsodium
{
//! \brief libsodium Hash API specialization ("interface" / implementation)
//! \ingroup cpp_API
//! \details
//!   Supported algorithms:
//!     <br>&emsp; \ref internal::type::Hash::BLAKE2b
//!     <br>&emsp; \ref internal::type::Hash::SHA2_256
//!     <br>&emsp; \ref internal::type::Hash::SHA2_512
//!
//!   <b>Example:</b><br>
//!   \code{.cpp}
//!     // Hash encode with SHA256
//!     using Hash = dfi::crypto::libsodium::Hash;  // inherits internal::type::Hash
//!     Hash hash; hash.encode<Hash::SHA2_256>(data);
//!   \endcode
//!
//! \see crypto::common::Hash for API details
//! \note For public consumption
//! \since docker-finance 1.0.0
using Hash = ::dfi::crypto::common::Hash<impl::libsodium::Hash>;
}  // namespace libsodium

}  // namespace crypto
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_HASH_HH_

// # vim: sw=2 sts=2 si ai et
