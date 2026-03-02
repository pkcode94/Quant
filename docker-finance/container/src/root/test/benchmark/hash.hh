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

#ifndef CONTAINER_SRC_ROOT_TEST_BENCHMARK_HASH_HH_
#define CONTAINER_SRC_ROOT_TEST_BENCHMARK_HASH_HH_

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
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
//! \namespace dfi::tests::benchmarks
//! \brief docker-finance benchmark cases
//! \ingroup cpp_tests
//! \since docker-finance 1.0.0
namespace benchmarks
{
//! \brief Generic Hash "interface" (specializations) fixture
//! \since docker-finance 1.0.0
template <typename t_interface>
struct HashInterfaceFixture : public ::benchmark::Fixture, protected t_interface
{
};

//! \brief Primary Hash "interface" fixture w/ mockup impl
//! \since docker-finance 1.0.0
struct HashInterface : public HashInterfaceFixture<tests::HashInterface>
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

BENCHMARK_F(HashInterface, string_view)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kStringView);
    }
}

BENCHMARK_F(HashInterface, char_array)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kCharArray);
    }
}

BENCHMARK_F(HashInterface, single_char)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kChar);
    }
}

BENCHMARK_F(HashInterface, std_byte)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kByte);
    }
}

BENCHMARK_F(HashInterface, int)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kInt16);
    }
}

BENCHMARK_F(HashInterface, string)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kString);
    }
}

BENCHMARK_F(HashInterface, vector)(::benchmark::State& state)  // NOLINT
{
  const std::vector<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, deque)(::benchmark::State& state)  // NOLINT
{
  const std::deque<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, list)(::benchmark::State& state)  // NOLINT
{
  const std::list<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, forward_list)(::benchmark::State& state)  // NOLINT
{
  const std::forward_list<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, map)(::benchmark::State& state)  // NOLINT
{
  const std::map<uint8_t, std::string> arg{kMapString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, multimap)(::benchmark::State& state)  // NOLINT
{
  const std::multimap<uint8_t, std::string> arg{
      {1, "Also"}, {1, "sprach"}, {1, "Zarathustra"}};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, multiset)(::benchmark::State& state)  // NOLINT
{
  const std::multiset<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, set)(::benchmark::State& state)  // NOLINT
{
  const std::set<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, unordered_map)(::benchmark::State& state)  // NOLINT
{
  const std::unordered_map<uint8_t, std::string> arg{
      {1, "Also"}, {2, "sprach"}, {3, "Zarathustra"}};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

// clang-format off
BENCHMARK_F(HashInterface, unordered_multimap)(::benchmark::State& state)  // NOLINT
// clang-format on
{
  const std::unordered_multimap<uint8_t, std::string> arg{
      {1, "Also"}, {1, "sprach"}, {1, "Zarathustra"}};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

// clang-format off
BENCHMARK_F(HashInterface, unordered_multiset)(::benchmark::State& state)  // NOLINT
// clang-format on
{
  const std::unordered_multiset<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

BENCHMARK_F(HashInterface, unordered_set)(::benchmark::State& state)  // NOLINT
{
  const std::unordered_set<std::string> arg{kString};
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(arg);
    }
}

//
// Botan
//

//! \brief Botan Hash fixture w/ real implementation
//! \since docker-finance 1.0.0
struct HashBotan : public ::benchmark::Fixture,
                   public tests::HashBotan_Impl,
                   public tests::HashData_Trivial
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

// BLAKE2

BENCHMARK_F(HashBotan, BLAKE2b)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::BLAKE2b>(kChar);
    }
}

// Keccak

BENCHMARK_F(HashBotan, Keccak_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_224>(kChar);
    }
}

BENCHMARK_F(HashBotan, Keccak_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_256>(kChar);
    }
}

BENCHMARK_F(HashBotan, Keccak_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_384>(kChar);
    }
}

BENCHMARK_F(HashBotan, Keccak_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_512>(kChar);
    }
}

// SHA1

BENCHMARK_F(HashBotan, SHA1)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kChar);
    }
}

// SHA2

BENCHMARK_F(HashBotan, SHA2_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_224>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA2_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_256>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA2_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_384>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA2_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_512>(kChar);
    }
}

// SHA3

BENCHMARK_F(HashBotan, SHA3_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_224>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA3_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_256>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA3_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_384>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHA3_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_512>(kChar);
    }
}

// SHAKE

BENCHMARK_F(HashBotan, SHAKE128)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHAKE128>(kChar);
    }
}

BENCHMARK_F(HashBotan, SHAKE256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHAKE256>(kChar);
    }
}

//
// Crypto++
//

//! \brief Crypto++ Hash fixture w/ real implementation
//! \since docker-finance 1.0.0
struct HashCryptoPP : public ::benchmark::Fixture,
                      public tests::HashCryptoPP_Impl,
                      public tests::HashData_Trivial
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

// BLAKE2

BENCHMARK_F(HashCryptoPP, BLAKE2s)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::BLAKE2s>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, BLAKE2b)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::BLAKE2b>(kChar);
    }
}

// Keccak

BENCHMARK_F(HashCryptoPP, Keccak_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_224>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, Keccak_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_256>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, Keccak_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_384>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, Keccak_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::Keccak_512>(kChar);
    }
}

// SHA1

BENCHMARK_F(HashCryptoPP, SHA1)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA1>(kChar);
    }
}

// SHA2

BENCHMARK_F(HashCryptoPP, SHA2_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_224>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA2_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_256>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA2_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_384>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA2_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_512>(kChar);
    }
}

// SHA3

BENCHMARK_F(HashCryptoPP, SHA3_224)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_224>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA3_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_256>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA3_384)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_384>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHA3_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA3_512>(kChar);
    }
}

// SHAKE

BENCHMARK_F(HashCryptoPP, SHAKE128)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHAKE128>(kChar);
    }
}

BENCHMARK_F(HashCryptoPP, SHAKE256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHAKE256>(kChar);
    }
}

//
// libsodium
//

//! \brief libsodium Hash fixture w/ real implementation
//! \since docker-finance 1.0.0
struct HashLibsodium : public ::benchmark::Fixture,
                       public tests::HashLibsodium_Impl,
                       public tests::HashData_Trivial
{
  void SetUp(const ::benchmark::State& state) {}
  void TearDown(const ::benchmark::State& state) {}
};

// BLAKE2

BENCHMARK_F(HashLibsodium, BLAKE2b)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::BLAKE2b>(kChar);
    }
}

// SHA2

BENCHMARK_F(HashLibsodium, SHA2_256)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_256>(kChar);
    }
}

BENCHMARK_F(HashLibsodium, SHA2_512)(::benchmark::State& state)  // NOLINT
{
  for (auto st : state)
    {
      hash.encode<Hash::SHA2_512>(kChar);
    }
}
}  // namespace benchmarks
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_BENCHMARK_HASH_HH_

// # vim: sw=2 sts=2 si ai et
