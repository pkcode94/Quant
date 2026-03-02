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

#ifndef CONTAINER_SRC_ROOT_MACRO_COMMON_CRYPTO_HH_
#define CONTAINER_SRC_ROOT_MACRO_COMMON_CRYPTO_HH_

#include <memory>

#include "../../src/hash.hh"
#include "../../src/random.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \namespace dfi::macro::common
//! \brief Shared ROOT macro-related functionality
//! \since docker-finance 1.0.0
namespace common
{
//! \namespace dfi::macro::common::crypto::botan
//! \since docker-finance 1.0.0
namespace crypto::botan
{
//! \brief ROOT's Botan Hash class
//! \ingroup cpp_macro
using Hash = ::dfi::crypto::botan::Hash;

//! \brief ROOT's Botan Hash instance
//! \ingroup cpp_macro
auto g_Hash = std::make_unique<Hash>();

//! \brief ROOT's Botan Random class
//! \ingroup cpp_macro
using Random = ::dfi::crypto::botan::Random;

//! \brief ROOT's Botan Random instance
//! \ingroup cpp_macro
auto g_Random = std::make_unique<Random>();
}  // namespace crypto::botan

//! \namespace dfi::macro::common::crypto::cryptopp
//! \since docker-finance 1.0.0
namespace crypto::cryptopp
{
//! \brief ROOT's Crypto++ Hash class
//! \ingroup cpp_macro
using Hash = ::dfi::crypto::cryptopp::Hash;

//! \brief ROOT's Crypto++ Hash instance
//! \ingroup cpp_macro
auto g_Hash = std::make_unique<Hash>();

//! \brief ROOT's Crypto++ Random class
//! \ingroup cpp_macro
using Random = ::dfi::crypto::cryptopp::Random;

//! \brief ROOT's Crypto++ Random instance
//! \ingroup cpp_macro
auto g_Random = std::make_unique<Random>();
}  // namespace crypto::cryptopp

//! \namespace dfi::macro::common::crypto::libsodium
//! \since docker-finance 1.0.0
namespace crypto::libsodium
{
//! \brief ROOT's libsodium Hash class
//! \ingroup cpp_macro
using Hash = ::dfi::crypto::libsodium::Hash;

//! \brief ROOT's libsodium Hash instance
//! \ingroup cpp_macro
auto g_Hash = std::make_unique<Hash>();

//! \brief ROOT's libsodium Random class
//! \ingroup cpp_macro
using Random = ::dfi::crypto::libsodium::Random;

//! \brief ROOT's libsodium Random instance
//! \ingroup cpp_macro
auto g_Random = std::make_unique<Random>();
}  // namespace crypto::libsodium

#ifdef __DFI_PLUGIN_BITCOIN__
//! \namespace dfi::macro::common::crypto::bitcoin
//! \since docker-finance 1.1.0
namespace crypto::bitcoin
{
//! \brief ROOT's bitcoin Random class
//! \ingroup cpp_macro
using Random = ::dfi::crypto::bitcoin::Random;

//! \brief ROOT's bitcoin Random instance
//! \ingroup cpp_macro
auto g_Random = std::make_unique<Random>();
}  // namespace crypto::bitcoin
#endif

}  // namespace common
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_COMMON_CRYPTO_HH_

// # vim: sw=2 sts=2 si ai et
