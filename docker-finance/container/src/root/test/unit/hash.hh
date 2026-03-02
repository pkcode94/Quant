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

#ifndef CONTAINER_SRC_ROOT_TEST_UNIT_HASH_HH_
#define CONTAINER_SRC_ROOT_TEST_UNIT_HASH_HH_

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/hash.hh"

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
//! \namespace dfi::tests::unit
//! \brief docker-finance unit test cases
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace unit
{
//! \brief Generic hash interface (specializations) fixture
//! \since docker-finance 1.0.0
template <typename t_interface>
struct HashInterfaceFixture : public ::testing::Test, protected t_interface
{
};

//! \brief Primary hash interface fixture w/ mockup impl
//! \since docker-finance 1.0.0
struct HashInterface : public HashInterfaceFixture<tests::HashInterface>
{
};

TEST_F(HashInterface, trivial)
{
  ASSERT_EQ(hash.encode<Hash::SHA1>(kStringView), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kCharArray), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kChar), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kByte), "");
}

TEST_F(HashInterface, integral)
{
  ASSERT_EQ(hash.encode<Hash::SHA1>(kInt16), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kInt32), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kInt64), "");

  ASSERT_EQ(hash.encode<Hash::SHA1>(kUInt16), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kUInt32), "");
  ASSERT_EQ(hash.encode<Hash::SHA1>(kUInt64), "");
}

TEST_F(HashInterface, string)
{
  ASSERT_EQ(hash.encode<Hash::SHA1>(kString), "");
}

TEST_F(HashInterface, vector)
{
  const std::vector<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, deque)
{
  const std::deque<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, list)
{
  const std::list<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, forward_list)
{
  const std::forward_list<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, map)
{
  const std::map<uint8_t, std::string> arg{kMapString},
      ret{{1, ""}, {2, ""}, {3, ""}};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, multimap)
{
  const std::multimap<uint8_t, std::string> arg{
      {1, "Also"}, {1, "sprach"}, {1, "Zarathustra"}},
      ret{{1, ""}, {1, ""}, {1, ""}};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, multiset)
{
  const std::multiset<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, set)
{
  const std::set<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, unordered_map)
{
  const std::unordered_map<uint8_t, std::string> arg{
      {1, "Also"}, {2, "sprach"}, {3, "Zarathustra"}},
      ret{{1, ""}, {2, ""}, {3, ""}};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, unordered_multimap)
{
  const std::unordered_multimap<uint8_t, std::string> arg{
      {1, "Also"}, {1, "sprach"}, {1, "Zarathustra"}},
      ret{{1, ""}, {1, ""}, {1, ""}};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, unordered_multiset)
{
  const std::unordered_multiset<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

TEST_F(HashInterface, unordered_set)
{
  const std::unordered_set<std::string> arg{kString}, ret{""};
  ASSERT_EQ(hash.encode<Hash::SHA1>(arg), ret);
}

//! \brief Generic hash encoding fixture
//! \since docker-finance 1.0.0
template <typename t_impl, typename t_data>
struct HashFixture : public ::testing::Test, protected t_impl, private t_data
{
 protected:
  template <typename t_hash>
  void encode()
  {
    // trivial
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kStringView),
        t_data::kStringViewDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kCharArray),
        t_data::kCharArrayDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kChar),
        t_data::kCharDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kByte),
        t_data::kByteDigest);

    // integral
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kInt16),
        t_data::kInt16Digest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kInt32),
        t_data::kInt32Digest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kInt64),
        t_data::kInt64Digest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kUInt16),
        t_data::kUInt16Digest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kUInt32),
        t_data::kUInt32Digest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kUInt64),
        t_data::kUInt64Digest);

    // string
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kString),
        t_data::kStringDigest);

    // vector
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kVecStringView),
        t_data::kVecStringViewDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kVecString),
        t_data::kVecStringDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kVecChar),
        t_data::kVecCharDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kVecByte),
        t_data::kVecByteDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kVecInt),
        t_data::kVecIntDigest);

    // map
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kMapStringView),
        t_data::kMapStringViewDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kMapString),
        t_data::kMapStringDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kMapChar),
        t_data::kMapCharDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kMapByte),
        t_data::kMapByteDigest);
    ASSERT_EQ(
        t_impl::hash.template encode<t_hash>(t_data::kMapInt),
        t_data::kMapIntDigest);
  };
};

//
// Botan
//

//! \brief Botan testing implementation and data fixture
//! \since docker-finance 1.0.0
template <typename t_data>
struct HashBotan : public HashFixture<tests::HashBotan_Impl, t_data>
{
};

// BLAKE2

//! \brief Botan BLAKE2b fixture
//! \since docker-finance 1.0.0
struct HashBotan_BLAKE2b : public HashBotan<tests::HashData_BLAKE2b>
{
};

TEST_F(HashBotan_BLAKE2b, encode)
{
  ASSERT_NO_THROW(encode<Hash::BLAKE2b>());
}

// Keccak

//! \brief Botan Keccak-224 fixture
//! \since docker-finance 1.0.0
struct HashBotan_Keccak_224 : public HashBotan<tests::HashData_Keccak_224>
{
};

TEST_F(HashBotan_Keccak_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_224>());
}

//! \brief Botan Keccak-256 fixture
//! \since docker-finance 1.0.0
struct HashBotan_Keccak_256 : public HashBotan<tests::HashData_Keccak_256>
{
};

TEST_F(HashBotan_Keccak_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_256>());
}

//! \brief Botan Keccak-384 fixture
//! \since docker-finance 1.0.0
struct HashBotan_Keccak_384 : public HashBotan<tests::HashData_Keccak_384>
{
};

TEST_F(HashBotan_Keccak_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_384>());
}

//! \brief Botan Keccak-512 fixture
//! \since docker-finance 1.0.0
struct HashBotan_Keccak_512 : public HashBotan<tests::HashData_Keccak_512>
{
};

TEST_F(HashBotan_Keccak_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_512>());
}

// SHA1

//! \brief Botan SHA1 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA1 : public HashBotan<tests::HashData_SHA1>
{
};

TEST_F(HashBotan_SHA1, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA1>());
}

// SHA2

//! \brief Botan SHA2-224 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA2_224 : public HashBotan<tests::HashData_SHA2_224>
{
};

TEST_F(HashBotan_SHA2_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_224>());
}

//! \brief Botan SHA2-256 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA2_256 : public HashBotan<tests::HashData_SHA2_256>
{
};

TEST_F(HashBotan_SHA2_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_256>());
}

//! \brief Botan SHA2-384 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA2_384 : public HashBotan<tests::HashData_SHA2_384>
{
};

TEST_F(HashBotan_SHA2_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_384>());
}

//! \brief Botan SHA2-512 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA2_512 : public HashBotan<tests::HashData_SHA2_512>
{
};

TEST_F(HashBotan_SHA2_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_512>());
}

// SHA3

//! \brief Botan SHA3-224 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA3_224 : public HashBotan<tests::HashData_SHA3_224>
{
};

TEST_F(HashBotan_SHA3_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_224>());
}

//! \brief Botan SHA3-256 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA3_256 : public HashBotan<tests::HashData_SHA3_256>
{
};

TEST_F(HashBotan_SHA3_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_256>());
}

//! \brief Botan SHA3-384 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA3_384 : public HashBotan<tests::HashData_SHA3_384>
{
};

TEST_F(HashBotan_SHA3_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_384>());
}

//! \brief Botan SHA3-512 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHA3_512 : public HashBotan<tests::HashData_SHA3_512>
{
};

TEST_F(HashBotan_SHA3_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_512>());
}

// SHAKE

//! \brief Botan SHAKE128 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHAKE128 : public HashBotan<tests::HashData_SHAKE128>
{
};

TEST_F(HashBotan_SHAKE128, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHAKE128>());
}

//! \brief Botan SHAKE256 fixture
//! \since docker-finance 1.0.0
struct HashBotan_SHAKE256 : public HashBotan<tests::HashData_SHAKE256>
{
};

TEST_F(HashBotan_SHAKE256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHAKE256>());
}

//
// Crypto++
//

//! \brief Crypto++ testing implementation and data fixture
//! \since docker-finance 1.0.0
template <typename t_data>
struct HashCryptoPP : public HashFixture<tests::HashCryptoPP_Impl, t_data>
{
};

// BLAKE2

//! \brief Crypto++ BLAKE2s fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_BLAKE2s : public HashCryptoPP<tests::HashData_BLAKE2s>
{
};

TEST_F(HashCryptoPP_BLAKE2s, encode)
{
  ASSERT_NO_THROW(encode<Hash::BLAKE2s>());
}

//! \brief Crypto++ BLAKE2b fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_BLAKE2b : public HashCryptoPP<tests::HashData_BLAKE2b>
{
};

TEST_F(HashCryptoPP_BLAKE2b, encode)
{
  ASSERT_NO_THROW(encode<Hash::BLAKE2b>());
}

// Keccak

//! \brief Crypto++ Keccak-224 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_Keccak_224 : public HashCryptoPP<tests::HashData_Keccak_224>
{
};

TEST_F(HashCryptoPP_Keccak_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_224>());
}

//! \brief Crypto++ Keccak-256 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_Keccak_256 : public HashCryptoPP<tests::HashData_Keccak_256>
{
};

TEST_F(HashCryptoPP_Keccak_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_256>());
}

//! \brief Crypto++ Keccak-384 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_Keccak_384 : public HashCryptoPP<tests::HashData_Keccak_384>
{
};

TEST_F(HashCryptoPP_Keccak_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_384>());
}

//! \brief Crypto++ Keccak-512 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_Keccak_512 : public HashCryptoPP<tests::HashData_Keccak_512>
{
};

TEST_F(HashCryptoPP_Keccak_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::Keccak_512>());
}

// SHA1

//! \brief Crypto++ SHA1 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA1 : public HashCryptoPP<tests::HashData_SHA1>
{
};

TEST_F(HashCryptoPP_SHA1, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA1>());
}

// SHA2

//! \brief Crypto++ SHA2-224 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA2_224 : public HashCryptoPP<tests::HashData_SHA2_224>
{
};

TEST_F(HashCryptoPP_SHA2_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_224>());
}

//! \brief Crypto++ SHA2-256 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA2_256 : public HashCryptoPP<tests::HashData_SHA2_256>
{
};

TEST_F(HashCryptoPP_SHA2_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_256>());
}

//! \brief Crypto++ SHA2-384 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA2_384 : public HashCryptoPP<tests::HashData_SHA2_384>
{
};

TEST_F(HashCryptoPP_SHA2_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_384>());
}

//! \brief Crypto++ SHA2-512 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA2_512 : public HashCryptoPP<tests::HashData_SHA2_512>
{
};

TEST_F(HashCryptoPP_SHA2_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_512>());
}

// SHA3

//! \brief Crypto++ SHA3-224 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA3_224 : public HashCryptoPP<tests::HashData_SHA3_224>
{
};

TEST_F(HashCryptoPP_SHA3_224, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_224>());
}

//! \brief Crypto++ SHA3-256 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA3_256 : public HashCryptoPP<tests::HashData_SHA3_256>
{
};

TEST_F(HashCryptoPP_SHA3_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_256>());
}

//! \brief Crypto++ SHA3-384 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA3_384 : public HashCryptoPP<tests::HashData_SHA3_384>
{
};

TEST_F(HashCryptoPP_SHA3_384, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_384>());
}

//! \brief Crypto++ SHA3-512 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHA3_512 : public HashCryptoPP<tests::HashData_SHA3_512>
{
};

TEST_F(HashCryptoPP_SHA3_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA3_512>());
}

// SHAKE

//! \brief Crypto++ SHAKE128 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHAKE128 : public HashCryptoPP<tests::HashData_SHAKE128>
{
};

TEST_F(HashCryptoPP_SHAKE128, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHAKE128>());
}

//! \brief Crypto++ SHAKE256 fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_SHAKE256 : public HashCryptoPP<tests::HashData_SHAKE256>
{
};

TEST_F(HashCryptoPP_SHAKE256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHAKE256>());
}

//
// libsodium
//

//! \brief libsodium testing implementation and data fixture
//! \since docker-finance 1.0.0
template <typename t_data>
struct HashLibsodium : public HashFixture<tests::HashLibsodium_Impl, t_data>
{
};

// BLAKE2

//! \brief libsodium BLAKE2b fixture
//! \since docker-finance 1.0.0
struct HashLibsodium_BLAKE2b : public HashLibsodium<tests::HashData_BLAKE2b>
{
};

TEST_F(HashLibsodium_BLAKE2b, encode)
{
  ASSERT_NO_THROW(encode<Hash::BLAKE2b>());
}

// SHA2

//! \brief libsodium SHA2-256 fixture
//! \since docker-finance 1.0.0
struct HashLibsodium_SHA2_256 : public HashLibsodium<tests::HashData_SHA2_256>
{
};

TEST_F(HashLibsodium_SHA2_256, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_256>());
}

//! \brief libsodium SHA2-512 fixture
//! \since docker-finance 1.0.0
struct HashLibsodium_SHA2_512 : public HashLibsodium<tests::HashData_SHA2_512>
{
};

TEST_F(HashLibsodium_SHA2_512, encode)
{
  ASSERT_NO_THROW(encode<Hash::SHA2_512>());
}
}  // namespace unit
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_UNIT_HASH_HH_

// # vim: sw=2 sts=2 si ai et
