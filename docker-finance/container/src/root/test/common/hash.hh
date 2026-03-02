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

#ifndef CONTAINER_SRC_ROOT_TEST_COMMON_HASH_HH_
#define CONTAINER_SRC_ROOT_TEST_COMMON_HASH_HH_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../../src/hash.hh"

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
//! \brief Trivial hash messages
//! \since docker-finance 1.0.0
struct HashData_Trivial
{
 protected:
  static constexpr std::string_view kStringView{"Also sprach Zarathustra"};
  static constexpr char kCharArray[]{"Also sprach Zarathustra"};
  static constexpr char kChar{'A'};
  static constexpr std::byte kByte{std::byte{0x41}};
};

//! \brief Integral hash messages
//! \since docker-finance 1.0.0
struct HashData_Integral
{
 protected:
  static constexpr int16_t kInt16{std::numeric_limits<int16_t>::max()};
  static constexpr int32_t kInt32{std::numeric_limits<int32_t>::max()};
  static constexpr int64_t kInt64{std::numeric_limits<int64_t>::max()};

  static constexpr uint16_t kUInt16{std::numeric_limits<uint16_t>::max()};
  static constexpr uint32_t kUInt32{std::numeric_limits<uint32_t>::max()};
  static constexpr uint64_t kUInt64{std::numeric_limits<uint64_t>::max()};
};

//! \brief Container hash messages
//! \since docker-finance 1.0.0
//! \todo fill empty 2nd (requires impl update)
struct HashData_Container
{
 protected:
  // string
  const std::string kString{"Also sprach Zarathustra"};

  // vector
  const std::vector<std::string_view> kVecStringView{
      {"Also", "sprach", "Zarathustra"}};
  const std::vector<std::string> kVecString{{"Also", "sprach", "Zarathustra"}};
  const std::vector<char> kVecChar{'A', 'B', 'C'};
  const std::vector<std::byte> kVecByte{
      std::byte{0x41},
      std::byte{0x42},
      std::byte{0x43}};
  const std::vector<int> kVecInt{{1, 2, 3}};

  // map
  const std::map<uint8_t, std::string> kMapStringView{
      {1, "Also"},
      {2, "sprach"},
      {3, "Zarathustra"}};
  const std::map<uint8_t, std::string> kMapString{
      {1, "Also"},
      {2, "sprach"},
      {3, "Zarathustra"}};
  const std::map<uint8_t, char> kMapChar{{1, 'A'}, {2, 'B'}, {3, 'C'}};
  const std::map<char, std::byte> kMapByte{
      {'A', std::byte{0x41}},
      {'B', std::byte{0x42}},
      {'C', std::byte{0x43}}};
  const std::map<std::string, int> kMapInt{
      {"one", 1},
      {"two", 2},
      {"three", 3}};
};

//! \brief Total hash messages
//! \since docker-finance 1.0.0
struct HashData : protected HashData_Trivial,
                  protected HashData_Integral,
                  protected HashData_Container
{
 protected:
  using t_encoded = std::string;
};

//! \brief Generic Hash "interface" (specialization) w/ mock impl
//! \since docker-finance 1.0.0
struct HashInterface : protected HashData
{
 protected:
  //! \brief Hash mock implementation
  //! \since docker-finance 1.0.0
  struct HashImpl : public ::dfi::internal::Transform<HashImpl>,
                    public ::dfi::internal::type::Hash
  {
    //! \brief Implements trivial type mock Hash encoder
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

      static_assert(
          std::is_same_v<t_hash, Hash::SHA1>, "Unsupported hash function");

      return t_encoded{};
    }

    //! \brief Implements non-trivial type mock Hash encoder
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

      static_assert(
          std::is_same_v<t_hash, Hash::SHA1>, "Unsupported hash function");

      return t_encoded{};
    }
  };

  using Hash = ::dfi::crypto::common::Hash<HashImpl>;
  Hash hash;
};

//
// Digests
//

// TODO(unassigned): Argon2, SipHash

//
// BLAKE2
//

//! \brief BLAKE2s digests
//! \since docker-finance 1.0.0
struct HashData_BLAKE2s : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "a0349506aa985073f6effb11cae8c29efb2c979f32cdba0327093563c3012c0c"};
  const t_encoded kCharArrayDigest{
      "a0349506aa985073f6effb11cae8c29efb2c979f32cdba0327093563c3012c0c"};
  const t_encoded kCharDigest{
      "98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618"};
  const t_encoded kByteDigest{
      "98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618"};

  // integral
  const t_encoded kInt16Digest{
      "4053c71b837ec2ab42e0060602e7ef0e4b2afcbc15cf1552342450c4118c3b6f"};
  const t_encoded kInt32Digest{
      "1831047398bae142fb8ce1f19d8edf4d976fb1c4c7d42aa147ab3915426f3feb"};
  const t_encoded kInt64Digest{
      "4a2fd851e1265e7b3cff15dc2aa5dc98c00b84d51dae5e4903ab1ca4d8b6bef6"};
  const t_encoded kUInt16Digest{
      "675966a4d6b7fe0504f4e27fa27e89336b3bbfa1b3d3ada2d07efeb836f93e50"};
  const t_encoded kUInt32Digest{
      "de754fece3a4d6cbdb012c129ea7f4e72613769ba3cc712a9fc5d464a584f8d8"};
  const t_encoded kUInt64Digest{
      "3038bb6eb76b08bf366f4ad41141e11e6c3de43e79f96fe10e97efb5ff7398c8"};

  // string
  const t_encoded kStringDigest{
      "a0349506aa985073f6effb11cae8c29efb2c979f32cdba0327093563c3012c0c"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"d2927eb1416a261fa3763fb0cad06d2d7bfdacfcc04a470d39f5dccf75fa464f",
       "c19d4efe9aadd444635c36a54ba750fc849af0f73a8b96c6d5bbfd1a0171cfe6",
       "8dc22425adc2aa3401e325ecb0139aa43dc1e21e17a9f71f0331798a63b0f402"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"d2927eb1416a261fa3763fb0cad06d2d7bfdacfcc04a470d39f5dccf75fa464f",
       "c19d4efe9aadd444635c36a54ba750fc849af0f73a8b96c6d5bbfd1a0171cfe6",
       "8dc22425adc2aa3401e325ecb0139aa43dc1e21e17a9f71f0331798a63b0f402"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618",
       "b12689dc38621e98d8cc05017716ab17be95d94f852d936d7290a6d84daf3f70",
       "a7051d7a7df225d353702b7bb1313db6cc0e1e34f8c5af3db4c09c93d716807d"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618",
       "b12689dc38621e98d8cc05017716ab17be95d94f852d936d7290a6d84daf3f70",
       "a7051d7a7df225d353702b7bb1313db6cc0e1e34f8c5af3db4c09c93d716807d"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"625851e3876e6e6da405c95ac24687ce4bb2cdd8fbd8459278f6f0ce803e13ee",
       "cd7aec459fb9c9fd67d89e6b733c394dd0503df3ab3d08e80894c9a4a14d086d",
       "7aa7f802dd1eb5d0c044c605eac5e2d0b0224121038154358e9a2bbed5e6600f"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "d2927eb1416a261fa3763fb0cad06d2d7bfdacfcc04a470d39f5dccf75fa464f"},
      {2, "c19d4efe9aadd444635c36a54ba750fc849af0f73a8b96c6d5bbfd1a0171cfe6"},
      {3, "8dc22425adc2aa3401e325ecb0139aa43dc1e21e17a9f71f0331798a63b0f402"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "d2927eb1416a261fa3763fb0cad06d2d7bfdacfcc04a470d39f5dccf75fa464f"},
      {2, "c19d4efe9aadd444635c36a54ba750fc849af0f73a8b96c6d5bbfd1a0171cfe6"},
      {3, "8dc22425adc2aa3401e325ecb0139aa43dc1e21e17a9f71f0331798a63b0f402"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618"},
      {2, "b12689dc38621e98d8cc05017716ab17be95d94f852d936d7290a6d84daf3f70"},
      {3, "a7051d7a7df225d353702b7bb1313db6cc0e1e34f8c5af3db4c09c93d716807d"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "98e14bd264b8837ddf8fd12d6f5641d59c369720b02c105feaf99f1b6a7b9618"},
      {'B', "b12689dc38621e98d8cc05017716ab17be95d94f852d936d7290a6d84daf3f70"},
      {'C',
       "a7051d7a7df225d353702b7bb1313db6cc0e1e34f8c5af3db4c09c93d716807d"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "625851e3876e6e6da405c95ac24687ce4bb2cdd8fbd8459278f6f0ce803e13ee"},
      {"two",
       "cd7aec459fb9c9fd67d89e6b733c394dd0503df3ab3d08e80894c9a4a14d086d"},
      {"three",
       "7aa7f802dd1eb5d0c044c605eac5e2d0b0224121038154358e9a2bbed5e6600f"}};
};

//! \brief BLAKE2b digests
//! \since docker-finance 1.0.0
struct HashData_BLAKE2b : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "4a2208c9c8dd32f2c7f59faaaf33491a9455af7c2d8c73ed7b805d58dd9d1f1db57f1d42"
      "64c6a4d7d4035af794ceffd0d78a97fafcac04c5b7241efbf3f8282f"};
  const t_encoded kCharArrayDigest{
      "4a2208c9c8dd32f2c7f59faaaf33491a9455af7c2d8c73ed7b805d58dd9d1f1db57f1d42"
      "64c6a4d7d4035af794ceffd0d78a97fafcac04c5b7241efbf3f8282f"};
  const t_encoded kCharDigest{
      "3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3ca"
      "c173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472"};
  const t_encoded kByteDigest{
      "3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3ca"
      "c173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472"};

  // integral
  const t_encoded kInt16Digest{
      "ec0a6676cffdefb314cf6284abf0837ab12d5b7f4657ae4f1d4b499173588dbe342d9672"
      "fd3af5891c4c1b252b05b0307c03c0868d9889d0be3b54457dace0e7"};
  const t_encoded kInt32Digest{
      "ceeb3fa3bb5f63423cea4454aec6227c6d196af9f91dec45453e8fcab3308fd697c693f2"
      "2c94a0ccc33185a4c33515f6d0fe5af10816497068d41018c4415115"};
  const t_encoded kInt64Digest{
      "a30b9ca3f745b984abcd45e7d08400935e82029cf01ebfec1f06d40c94ed22a735a42b91"
      "26195f2de9ee117d07ce899d759a6d96e51dbe7d188fdf489f24453c"};
  const t_encoded kUInt16Digest{
      "ea0a91eb2d9b18e85c700d561fa158eeb8ada73d3825f974ff0b95eb32cd194850dd9208"
      "bb7fec857b2df0d891a8146f65ab15268b223a3326f940ec09ca334a"};
  const t_encoded kUInt32Digest{
      "0efddd8f6310b9587e4a5ce261dcebc1ae5dce9af598c1a733be77db6acad411295edc20"
      "e5d1f6040a27491250225c9536e4b029f3bd3c5f85966c3bd978ae2c"};
  const t_encoded kUInt64Digest{
      "65195989208d5b67b7befef6f6b4e8e65567bca2307fb548afb40dc6ce6b05d6f26f5ab5"
      "bf84afbd7a74fed4cad903f757f831c113fb50301f3f8b0fef8f85ed"};

  // string
  const t_encoded kStringDigest{
      "4a2208c9c8dd32f2c7f59faaaf33491a9455af7c2d8c73ed7b805d58dd9d1f1db57f1d42"
      "64c6a4d7d4035af794ceffd0d78a97fafcac04c5b7241efbf3f8282f"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"c418d9ddb80021990db2aa5d46e54d205889e433972862da096e72151f809886bc1bfb8"
       "c03890da5714d5035b037faea804ebdfe05f10b93dcd07d8979a9d233",
       "01d0ecbba1c674e0cc314c8b19848a1a7af508759631b90d0d8d40aa6c3afcd979f58e5"
       "d17509f638d3d153328d9a7dc55e021cbbaa8d76acb5589995fb4c869",
       "1ca685e5e9b1f31c2d9a3ae1ec0f031a6814cd2fa07e2bdf7601526ed64a236ace0daae"
       "85b31b0b9df56a1981ae3ca0f6d836463704df01f04a8e5bb7d79788c"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"c418d9ddb80021990db2aa5d46e54d205889e433972862da096e72151f809886bc1bfb8"
       "c03890da5714d5035b037faea804ebdfe05f10b93dcd07d8979a9d233",
       "01d0ecbba1c674e0cc314c8b19848a1a7af508759631b90d0d8d40aa6c3afcd979f58e5"
       "d17509f638d3d153328d9a7dc55e021cbbaa8d76acb5589995fb4c869",
       "1ca685e5e9b1f31c2d9a3ae1ec0f031a6814cd2fa07e2bdf7601526ed64a236ace0daae"
       "85b31b0b9df56a1981ae3ca0f6d836463704df01f04a8e5bb7d79788c"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3c"
       "ac173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472",
       "b894b2e6b40a51c29369600ad433398c1521f0be45e0e4b97cc024245fd8fec554b57e9"
       "e6a6b6b8f02847107ef2cd5e47cc929a9e05ffac95ebd58c4a7c1d246",
       "bb46ad590dfc5e0a83e1563f5d9577a85f0fb013ea5f6083feb8b44518ed8b4b4117b19"
       "908f925d473379390a4f4dd53d05d55f81ee4dc2b8b37ce0129a0da50"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3c"
       "ac173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472",
       "b894b2e6b40a51c29369600ad433398c1521f0be45e0e4b97cc024245fd8fec554b57e9"
       "e6a6b6b8f02847107ef2cd5e47cc929a9e05ffac95ebd58c4a7c1d246",
       "bb46ad590dfc5e0a83e1563f5d9577a85f0fb013ea5f6083feb8b44518ed8b4b4117b19"
       "908f925d473379390a4f4dd53d05d55f81ee4dc2b8b37ce0129a0da50"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"1ced8f5be2db23a6513eba4d819c73806424748a7bc6fa0d792cc1c7d1775a9778e894a"
       "a91413f6eb79ad5ae2f871eafcc78797e4c82af6d1cbfb1a294a10d10",
       "c5faca15ac2f93578b39ef4b6bbb871bdedce4ddd584fd31f0bb66fade3947e6bb1353e"
       "562414ed50638a8829ff3daccac7ef4a50acee72a5384ba9aeb604fc9",
       "6f760b9e9eac89f07ab0223b0f4acb04d1e355d893a1b86a83f4d4b405adee99913dacb"
       "7bc3d6e6a46f996e59b965e82b1ffa1994062bcd8bef867bcf743c07c"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "c418d9ddb80021990db2aa5d46e54d205889e433972862da096e72151f809886bc1bfb8"
       "c03890da5714d5035b037faea804ebdfe05f10b93dcd07d8979a9d233"},
      {2,
       "01d0ecbba1c674e0cc314c8b19848a1a7af508759631b90d0d8d40aa6c3afcd979f58e5"
       "d17509f638d3d153328d9a7dc55e021cbbaa8d76acb5589995fb4c869"},
      {3,
       "1ca685e5e9b1f31c2d9a3ae1ec0f031a6814cd2fa07e2bdf7601526ed64a236ace0daae"
       "85b31b0b9df56a1981ae3ca0f6d836463704df01f04a8e5bb7d79788c"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "c418d9ddb80021990db2aa5d46e54d205889e433972862da096e72151f809886bc1bfb8"
       "c03890da5714d5035b037faea804ebdfe05f10b93dcd07d8979a9d233"},
      {2,
       "01d0ecbba1c674e0cc314c8b19848a1a7af508759631b90d0d8d40aa6c3afcd979f58e5"
       "d17509f638d3d153328d9a7dc55e021cbbaa8d76acb5589995fb4c869"},
      {3,
       "1ca685e5e9b1f31c2d9a3ae1ec0f031a6814cd2fa07e2bdf7601526ed64a236ace0daae"
       "85b31b0b9df56a1981ae3ca0f6d836463704df01f04a8e5bb7d79788c"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3c"
       "ac173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472"},
      {2,
       "b894b2e6b40a51c29369600ad433398c1521f0be45e0e4b97cc024245fd8fec554b57e9"
       "e6a6b6b8f02847107ef2cd5e47cc929a9e05ffac95ebd58c4a7c1d246"},
      {3,
       "bb46ad590dfc5e0a83e1563f5d9577a85f0fb013ea5f6083feb8b44518ed8b4b4117b19"
       "908f925d473379390a4f4dd53d05d55f81ee4dc2b8b37ce0129a0da50"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "3e6173df7f81c1eb9ce997312fe72e441b40b72cd5ffca23d05ef805bf6e938e1ef9c3c"
       "ac173005f77d698c2ca30dd785eb745aad32fcb4d5afff91c30ad7472"},
      {'B',
       "b894b2e6b40a51c29369600ad433398c1521f0be45e0e4b97cc024245fd8fec554b57e9"
       "e6a6b6b8f02847107ef2cd5e47cc929a9e05ffac95ebd58c4a7c1d246"},
      {'C',
       "bb46ad590dfc5e0a83e1563f5d9577a85f0fb013ea5f6083feb8b44518ed8b4b4117b19"
       "908f925d473379390a4f4dd53d05d55f81ee4dc2b8b37ce0129a0da50"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "1ced8f5be2db23a6513eba4d819c73806424748a7bc6fa0d792cc1c7d1775a9778e894a"
       "a91413f6eb79ad5ae2f871eafcc78797e4c82af6d1cbfb1a294a10d10"},
      {"two",
       "c5faca15ac2f93578b39ef4b6bbb871bdedce4ddd584fd31f0bb66fade3947e6bb1353e"
       "562414ed50638a8829ff3daccac7ef4a50acee72a5384ba9aeb604fc9"},
      {"three",
       "6f760b9e9eac89f07ab0223b0f4acb04d1e355d893a1b86a83f4d4b405adee99913dacb"
       "7bc3d6e6a46f996e59b965e82b1ffa1994062bcd8bef867bcf743c07c"}};
};

//
// Keccak
//

//! \brief Keccak-224 digests
//! \since docker-finance 1.0.0
struct HashData_Keccak_224 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "925a1ef00898cf6437188b01e67544a62a76597992fd9bb0f75dc223"};
  const t_encoded kCharArrayDigest{
      "925a1ef00898cf6437188b01e67544a62a76597992fd9bb0f75dc223"};
  const t_encoded kCharDigest{
      "ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e"};
  const t_encoded kByteDigest{
      "ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e"};

  // integral
  const t_encoded kInt16Digest{
      "8a9542398399023cdafad1963e33a184c2c5ea50e83f4ae4c63b6627"};
  const t_encoded kInt32Digest{
      "f0acc8d4da6b5ad36b687bb0979c9a8c26a9e84e24123b6465a9aa88"};
  const t_encoded kInt64Digest{
      "9244e4de3fd346454c674730577a18e4cdf2a73b45e5f1385f9097b6"};
  const t_encoded kUInt16Digest{
      "9bedc655672e163a226848358b781edcdcc740f2d0425355964891c8"};
  const t_encoded kUInt32Digest{
      "a80fc51d0a599be005103fecefde7fd6a09696c7028976cdcb914699"};
  const t_encoded kUInt64Digest{
      "da19bde0821438ca6e4c73730ee8c52a292d3c2aad6d760ff86d5b2c"};

  // string
  const t_encoded kStringDigest{
      "925a1ef00898cf6437188b01e67544a62a76597992fd9bb0f75dc223"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"48f0d5b8a1b1e312c68d97d76205f613579dff5d031572c04324305c",
       "466878e8c7ed2684bb1e3bdfc3fad1950e19a5eb22f0de48ce7410c6",
       "1fa8fc7b27acef32d9b24cd4301c25f42e6e52a6a4f2f20f69245cb4"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"48f0d5b8a1b1e312c68d97d76205f613579dff5d031572c04324305c",
       "466878e8c7ed2684bb1e3bdfc3fad1950e19a5eb22f0de48ce7410c6",
       "1fa8fc7b27acef32d9b24cd4301c25f42e6e52a6a4f2f20f69245cb4"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e",
       "48e3b03825c2396b45c1b1a984261d8965469986d45fe834a1e3ffd5",
       "0a827debd811b6dcb3af96fbd88289e2d1e6769867f582b9a2319178"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e",
       "48e3b03825c2396b45c1b1a984261d8965469986d45fe834a1e3ffd5",
       "0a827debd811b6dcb3af96fbd88289e2d1e6769867f582b9a2319178"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"f3af0c2e122cb4d87350c0888092f2015eb6131ff2e9dd6893cc7645",
       "3a8bc59eb4ed4b22328db6c9476e63e7e76e2411225d9b5304e42807",
       "a585325409df69c24dc6112602e59225d84439c5098b8e4cb6cec45e"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "48f0d5b8a1b1e312c68d97d76205f613579dff5d031572c04324305c"},
      {2, "466878e8c7ed2684bb1e3bdfc3fad1950e19a5eb22f0de48ce7410c6"},
      {3, "1fa8fc7b27acef32d9b24cd4301c25f42e6e52a6a4f2f20f69245cb4"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "48f0d5b8a1b1e312c68d97d76205f613579dff5d031572c04324305c"},
      {2, "466878e8c7ed2684bb1e3bdfc3fad1950e19a5eb22f0de48ce7410c6"},
      {3, "1fa8fc7b27acef32d9b24cd4301c25f42e6e52a6a4f2f20f69245cb4"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e"},
      {2, "48e3b03825c2396b45c1b1a984261d8965469986d45fe834a1e3ffd5"},
      {3, "0a827debd811b6dcb3af96fbd88289e2d1e6769867f582b9a2319178"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "ef40b16ff375c834e91412489889f36538748c5454f4b02ba750b65e"},
      {'B', "48e3b03825c2396b45c1b1a984261d8965469986d45fe834a1e3ffd5"},
      {'C', "0a827debd811b6dcb3af96fbd88289e2d1e6769867f582b9a2319178"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one", "f3af0c2e122cb4d87350c0888092f2015eb6131ff2e9dd6893cc7645"},
      {"two", "3a8bc59eb4ed4b22328db6c9476e63e7e76e2411225d9b5304e42807"},
      {"three", "a585325409df69c24dc6112602e59225d84439c5098b8e4cb6cec45e"}};
};

//! \brief Keccak-256 digests
//! \since docker-finance 1.0.0
struct HashData_Keccak_256 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "149d3f963a510345a472e6d59b762e53472aa83b122f2b4eab5dd7199737161e"};
  const t_encoded kCharArrayDigest{
      "149d3f963a510345a472e6d59b762e53472aa83b122f2b4eab5dd7199737161e"};
  const t_encoded kCharDigest{
      "03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760"};
  const t_encoded kByteDigest{
      "03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760"};

  // integral
  const t_encoded kInt16Digest{
      "8c88309f8003ced72b54fe7a420af7e4b72bfceed1d536b58c3086215dad4e8f"};
  const t_encoded kInt32Digest{
      "797138452f68cb8ae56d72396aefb510ea8b05a825c0e89da23016d8dc8cd37f"};
  const t_encoded kInt64Digest{
      "c23afaf993bef633220c94fba7cf577103b120a2701d597660dfbab0720a8ab4"};
  const t_encoded kUInt16Digest{
      "37ecec75cba14bd31a337e82f3022d91cfa205a338fe152ab72f03bc2c738d10"};
  const t_encoded kUInt32Digest{
      "ee83f996188187b84610e68cd5928e27589115c11f1746dc3da1c5c4f005123a"};
  const t_encoded kUInt64Digest{
      "2f36be02f5c29e723ce25ef1eed227d5ce0d6470fe29044e77ade9e1def49e90"};

  // string
  const t_encoded kStringDigest{
      "149d3f963a510345a472e6d59b762e53472aa83b122f2b4eab5dd7199737161e"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"d0595cda76bd6c1fe26732ae924b72bde36c92667440dbc77612b58bef8cdd6d",
       "5be558e651b93976e8d1967c488657a7de7852cc7638aa389a0dc5bcecc0f5d6",
       "8a9fbb3344ef3b3bb89322290b2b90bf9fb6a91cb2312ecee4b51d5e3fccadd7"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"d0595cda76bd6c1fe26732ae924b72bde36c92667440dbc77612b58bef8cdd6d",
       "5be558e651b93976e8d1967c488657a7de7852cc7638aa389a0dc5bcecc0f5d6",
       "8a9fbb3344ef3b3bb89322290b2b90bf9fb6a91cb2312ecee4b51d5e3fccadd7"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760",
       "1f675bff07515f5df96737194ea945c36c41e7b4fcef307b7cd4d0e602a69111",
       "017e667f4b8c174291d1543c466717566e206df1bfd6f30271055ddafdb18f72"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760",
       "1f675bff07515f5df96737194ea945c36c41e7b4fcef307b7cd4d0e602a69111",
       "017e667f4b8c174291d1543c466717566e206df1bfd6f30271055ddafdb18f72"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"c89efdaa54c0f20c7adf612882df0950f5a951637e0307cdcb4c672f298b8bc6",
       "ad7c5bef027816a800da1736444fb58a807ef4c9603b7848673f7e3a68eb14a5",
       "2a80e1ef1d7842f27f2e6be0972bb708b9a135c38860dbe73c27c3486c34f4de"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "d0595cda76bd6c1fe26732ae924b72bde36c92667440dbc77612b58bef8cdd6d"},
      {2, "5be558e651b93976e8d1967c488657a7de7852cc7638aa389a0dc5bcecc0f5d6"},
      {3, "8a9fbb3344ef3b3bb89322290b2b90bf9fb6a91cb2312ecee4b51d5e3fccadd7"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "d0595cda76bd6c1fe26732ae924b72bde36c92667440dbc77612b58bef8cdd6d"},
      {2, "5be558e651b93976e8d1967c488657a7de7852cc7638aa389a0dc5bcecc0f5d6"},
      {3, "8a9fbb3344ef3b3bb89322290b2b90bf9fb6a91cb2312ecee4b51d5e3fccadd7"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760"},
      {2, "1f675bff07515f5df96737194ea945c36c41e7b4fcef307b7cd4d0e602a69111"},
      {3, "017e667f4b8c174291d1543c466717566e206df1bfd6f30271055ddafdb18f72"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "03783fac2efed8fbc9ad443e592ee30e61d65f471140c10ca155e937b435b760"},
      {'B', "1f675bff07515f5df96737194ea945c36c41e7b4fcef307b7cd4d0e602a69111"},
      {'C',
       "017e667f4b8c174291d1543c466717566e206df1bfd6f30271055ddafdb18f72"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "c89efdaa54c0f20c7adf612882df0950f5a951637e0307cdcb4c672f298b8bc6"},
      {"two",
       "ad7c5bef027816a800da1736444fb58a807ef4c9603b7848673f7e3a68eb14a5"},
      {"three",
       "2a80e1ef1d7842f27f2e6be0972bb708b9a135c38860dbe73c27c3486c34f4de"}};
};

//! \brief Keccak-384 digests
//! \since docker-finance 1.0.0
struct HashData_Keccak_384 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "89309ff28147076be7aeaad214beaff2a11ad4d435a209cca73d8c1c511315aba1246381"
      "88590569a41a338d3156f211"};
  const t_encoded kCharArrayDigest{
      "89309ff28147076be7aeaad214beaff2a11ad4d435a209cca73d8c1c511315aba1246381"
      "88590569a41a338d3156f211"};
  const t_encoded kCharDigest{
      "5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c22"
      "c50ea866c94debf1061f37a4"};
  const t_encoded kByteDigest{
      "5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c22"
      "c50ea866c94debf1061f37a4"};

  // integral
  const t_encoded kInt16Digest{
      "0ca3d6fc3dc1e829624165bc6355141f2bdbc1676805837fa7900c350479b2110b4b0ce1"
      "bc93090756656ff4109b7889"};
  const t_encoded kInt32Digest{
      "909e19e7eb1b4aab50000a96e415b0643811a6cab721e13803561ef46a05f64d8fb410e3"
      "195827510fbeb8c5c4236cc4"};
  const t_encoded kInt64Digest{
      "93b1524ca70429a5d4c67f540e95d111381678f5862acb4b687b3ffc38bc68ec165186f6"
      "6fee7495f9a5b6f194c9a8f1"};
  const t_encoded kUInt16Digest{
      "6e5f78904d4e7b9b400654cd1253bfae6519a63427bb6515f93b338691470b58697c9d2b"
      "0713fd56d3574e3fe830020e"};
  const t_encoded kUInt32Digest{
      "e9a4d358ed9f9c6c7631ad258e19ec6eb33748d44fb14bfebef64be35de3b43fb737d07b"
      "14760d09e0c80ace3bbdacf1"};
  const t_encoded kUInt64Digest{
      "af7dd7e401bbe870ede3a4496bf36ea2e83d2ab82fa74c71df89c00e59b9fddcd3faa854"
      "4821d71f59d1410a12464fec"};

  // string
  const t_encoded kStringDigest{
      "89309ff28147076be7aeaad214beaff2a11ad4d435a209cca73d8c1c511315aba1246381"
      "88590569a41a338d3156f211"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"57ce31ce46cde58ce013abf27d4b55ccc9d05f57102e98e3ad6730831600be585a613e0"
       "130abbb69950ae76388b529bf",
       "5770591f33f4fb9662f9682951924ce29fc179552110829b04f54b15e83ac2f4a4c74d8"
       "9187197c3ec4838b2351ab145",
       "d2cbb571eea3ff391bb8630901b70641c5f6f551345c0cbfa1093b8d6a9c15a1e01ab72"
       "1e341f976523ac9e3b2de8d22"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"57ce31ce46cde58ce013abf27d4b55ccc9d05f57102e98e3ad6730831600be585a613e0"
       "130abbb69950ae76388b529bf",
       "5770591f33f4fb9662f9682951924ce29fc179552110829b04f54b15e83ac2f4a4c74d8"
       "9187197c3ec4838b2351ab145",
       "d2cbb571eea3ff391bb8630901b70641c5f6f551345c0cbfa1093b8d6a9c15a1e01ab72"
       "1e341f976523ac9e3b2de8d22"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c2"
       "2c50ea866c94debf1061f37a4",
       "9e3d2f9e71629a42b029826f74b344aefe61815636561c46cf6a43d7d20dba57a6f0a8d"
       "4ea83c598230a068108535d98",
       "5b8a19ca8c12c9f3d5a2886d00ce1da893fcd1b511bd3ea44de224fdf054e05d637dc09"
       "3bd253afd9d893620b1f6c659"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c2"
       "2c50ea866c94debf1061f37a4",
       "9e3d2f9e71629a42b029826f74b344aefe61815636561c46cf6a43d7d20dba57a6f0a8d"
       "4ea83c598230a068108535d98",
       "5b8a19ca8c12c9f3d5a2886d00ce1da893fcd1b511bd3ea44de224fdf054e05d637dc09"
       "3bd253afd9d893620b1f6c659"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"8147a2689f508cc03278c313290588b44607b5c0ccc07df5b0a80baf7d25812f0928c01"
       "8c81381ef1dc76b25dbf6bafc",
       "d59d5976e48498472b1b55a5989f1c88768ec47a901193c9fb81f05394a996bf8e4d7dd"
       "9ebf9ee94e32eff92c3e4ea9c",
       "0f51a7ed969858c735735a88b3e195701f830e4408c996ff3356d1426b7e1ea193a6bd3"
       "dbe26c29f3fec2008edce9f47"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "57ce31ce46cde58ce013abf27d4b55ccc9d05f57102e98e3ad6730831600be585a613e0"
       "130abbb69950ae76388b529bf"},
      {2,
       "5770591f33f4fb9662f9682951924ce29fc179552110829b04f54b15e83ac2f4a4c74d8"
       "9187197c3ec4838b2351ab145"},
      {3,
       "d2cbb571eea3ff391bb8630901b70641c5f6f551345c0cbfa1093b8d6a9c15a1e01ab72"
       "1e341f976523ac9e3b2de8d22"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "57ce31ce46cde58ce013abf27d4b55ccc9d05f57102e98e3ad6730831600be585a613e0"
       "130abbb69950ae76388b529bf"},
      {2,
       "5770591f33f4fb9662f9682951924ce29fc179552110829b04f54b15e83ac2f4a4c74d8"
       "9187197c3ec4838b2351ab145"},
      {3,
       "d2cbb571eea3ff391bb8630901b70641c5f6f551345c0cbfa1093b8d6a9c15a1e01ab72"
       "1e341f976523ac9e3b2de8d22"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c2"
       "2c50ea866c94debf1061f37a4"},
      {2,
       "9e3d2f9e71629a42b029826f74b344aefe61815636561c46cf6a43d7d20dba57a6f0a8d"
       "4ea83c598230a068108535d98"},
      {3,
       "5b8a19ca8c12c9f3d5a2886d00ce1da893fcd1b511bd3ea44de224fdf054e05d637dc09"
       "3bd253afd9d893620b1f6c659"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "5c744cf4b4e3fb8967189e9744261a74f0ef31cdd8850554c737803585ac109039b73c2"
       "2c50ea866c94debf1061f37a4"},
      {'B',
       "9e3d2f9e71629a42b029826f74b344aefe61815636561c46cf6a43d7d20dba57a6f0a8d"
       "4ea83c598230a068108535d98"},
      {'C',
       "5b8a19ca8c12c9f3d5a2886d00ce1da893fcd1b511bd3ea44de224fdf054e05d637dc09"
       "3bd253afd9d893620b1f6c659"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "8147a2689f508cc03278c313290588b44607b5c0ccc07df5b0a80baf7d25812f0928c01"
       "8c81381ef1dc76b25dbf6bafc"},
      {"two",
       "d59d5976e48498472b1b55a5989f1c88768ec47a901193c9fb81f05394a996bf8e4d7dd"
       "9ebf9ee94e32eff92c3e4ea9c"},
      {"three",
       "0f51a7ed969858c735735a88b3e195701f830e4408c996ff3356d1426b7e1ea193a6bd3"
       "dbe26c29f3fec2008edce9f47"}};
};

//! \brief Keccak-512 digests
//! \since docker-finance 1.0.0
struct HashData_Keccak_512 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "f058b6f74a6d3aecfe24679b7a17199a9bf62950f898404101665c56b1e59797deec8dbd"
      "2a3273b74fe28130038d58ed8902b3a467e94d3f0d1da53fe9fe7653"};
  const t_encoded kCharArrayDigest{
      "f058b6f74a6d3aecfe24679b7a17199a9bf62950f898404101665c56b1e59797deec8dbd"
      "2a3273b74fe28130038d58ed8902b3a467e94d3f0d1da53fe9fe7653"};
  const t_encoded kCharDigest{
      "421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1b"
      "c09c728a8cd4f096731298d51463f15af89543fed478053346260c38"};
  const t_encoded kByteDigest{
      "421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1b"
      "c09c728a8cd4f096731298d51463f15af89543fed478053346260c38"};

  // integral
  const t_encoded kInt16Digest{
      "9846d85e5e925aec21a5f8427b3c8e7e72277225f1a861bfc0e054a22c407438c08c6da7"
      "6629065eb92cde3505202babc95357cec97af4060dc54d603ab641af"};
  const t_encoded kInt32Digest{
      "fa5b4828216e24033fc0b07ef8e1ec45f2661823e24a371c51a3dfa7e9f2fef2c6c98a57"
      "fe83e57e42948f7c452f39066c1f6372ef26f357059ce364ee765180"};
  const t_encoded kInt64Digest{
      "43c51c51c0ee0fec5a7bf46af2380668d9753342c38ae6e975f3c43e14f3c6139d4e39b9"
      "bdd3349fabc7ee7e5659af799fdea489aa2dd7f8ac9249c032650ab9"};
  const t_encoded kUInt16Digest{
      "5d2e202e533814cafd66f7c3aabcd5ad73253a4a0917089c6a6ada8342a4a5ad2754b5ea"
      "c1b1f36fc3cc509e63f317a43861a2b2e85ee5e490feb9cca3ca77c8"};
  const t_encoded kUInt32Digest{
      "fd9dbdbec0ba38d2b5b593eef623dc66fe4ffe77db6558d04bcb50334187ac2deb5b16ca"
      "5a557bb175c017069cb3a0092268362d31ff8454a1fdedd207e46a4e"};
  const t_encoded kUInt64Digest{
      "99bf23f8d39208c9dc971d973833edb37686f782832facf06fdc750fbe33a330cd855bdd"
      "eea7db84f017bd1283cf07cdf0a03a181a306002cc7c0b740bffbb46"};

  // string
  const t_encoded kStringDigest{
      "f058b6f74a6d3aecfe24679b7a17199a9bf62950f898404101665c56b1e59797deec8dbd"
      "2a3273b74fe28130038d58ed8902b3a467e94d3f0d1da53fe9fe7653"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"b34dbb427eaf93fce94c175da0aee7eacec5f0fadd34bc79a8b2fc2e37a8ffb971c2a97"
       "95f7d7340c7768305998add2778be8214a87727c3f4fe88d6d4dc3de5",
       "da49c33fab9d99c0031bc4ca2e7506a46b68660189979cb8e1daa1de233dc4af44a9e7d"
       "ce35f121188f95ba407c96b3d92e58c54917b79636272e8c784913413",
       "dd959dae73738e76b950bdece6a71900828ce2bbeaf88ed6def7c25ced8a29be708accc"
       "8987552d3a4cdc43e232ce05498763479123535b0e9f626d6b4163047"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"b34dbb427eaf93fce94c175da0aee7eacec5f0fadd34bc79a8b2fc2e37a8ffb971c2a97"
       "95f7d7340c7768305998add2778be8214a87727c3f4fe88d6d4dc3de5",
       "da49c33fab9d99c0031bc4ca2e7506a46b68660189979cb8e1daa1de233dc4af44a9e7d"
       "ce35f121188f95ba407c96b3d92e58c54917b79636272e8c784913413",
       "dd959dae73738e76b950bdece6a71900828ce2bbeaf88ed6def7c25ced8a29be708accc"
       "8987552d3a4cdc43e232ce05498763479123535b0e9f626d6b4163047"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1"
       "bc09c728a8cd4f096731298d51463f15af89543fed478053346260c38",
       "d8ca35343ac8e74ac6e70cb3f4b952a5cd19bbc402e80d0f833ef82b79fccc93b4bcb9e"
       "dac078fa45c9456bc9df3dcfb5ec0d6ccbf6248030eed282f303c1711",
       "dd8197de0748d048891fce8b6f0b99c50cb9a50d1b95c5db17ec4aa30631984ebdce580"
       "35a09968a942619c8e7711564809549b3512afe3a27fdc2e5491eed4c"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1"
       "bc09c728a8cd4f096731298d51463f15af89543fed478053346260c38",
       "d8ca35343ac8e74ac6e70cb3f4b952a5cd19bbc402e80d0f833ef82b79fccc93b4bcb9e"
       "dac078fa45c9456bc9df3dcfb5ec0d6ccbf6248030eed282f303c1711",
       "dd8197de0748d048891fce8b6f0b99c50cb9a50d1b95c5db17ec4aa30631984ebdce580"
       "35a09968a942619c8e7711564809549b3512afe3a27fdc2e5491eed4c"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"00197a4f5f1ff8c356a78f6921b5a6bfbf71df8dbd313fbc5095a55de756bfa1ea72406"
       "95005149294f2a2e419ae251fe2f7dbb67c3bb647c2ac1be05eec7ef9",
       "ac3b6998ac9c5e2c7ee8330010a7b0f87ac9dee7ea547d4d8cd00ab7ad1bd5f57f80af2"
       "ba711a9eb137b4e83b503d24cd7665399a48734d47fff324fb74551e2",
       "ce4fd4068e56eb07a6e79d007aed4bc8257e10827c74ee422d82a29b2ce8cb079fead81"
       "d9df0513bb577f3b6c47843b17c964e7ff8f4198f32027533eaf5bcc1"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "b34dbb427eaf93fce94c175da0aee7eacec5f0fadd34bc79a8b2fc2e37a8ffb971c2a97"
       "95f7d7340c7768305998add2778be8214a87727c3f4fe88d6d4dc3de5"},
      {2,
       "da49c33fab9d99c0031bc4ca2e7506a46b68660189979cb8e1daa1de233dc4af44a9e7d"
       "ce35f121188f95ba407c96b3d92e58c54917b79636272e8c784913413"},
      {3,
       "dd959dae73738e76b950bdece6a71900828ce2bbeaf88ed6def7c25ced8a29be708accc"
       "8987552d3a4cdc43e232ce05498763479123535b0e9f626d6b4163047"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "b34dbb427eaf93fce94c175da0aee7eacec5f0fadd34bc79a8b2fc2e37a8ffb971c2a97"
       "95f7d7340c7768305998add2778be8214a87727c3f4fe88d6d4dc3de5"},
      {2,
       "da49c33fab9d99c0031bc4ca2e7506a46b68660189979cb8e1daa1de233dc4af44a9e7d"
       "ce35f121188f95ba407c96b3d92e58c54917b79636272e8c784913413"},
      {3,
       "dd959dae73738e76b950bdece6a71900828ce2bbeaf88ed6def7c25ced8a29be708accc"
       "8987552d3a4cdc43e232ce05498763479123535b0e9f626d6b4163047"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1"
       "bc09c728a8cd4f096731298d51463f15af89543fed478053346260c38"},
      {2,
       "d8ca35343ac8e74ac6e70cb3f4b952a5cd19bbc402e80d0f833ef82b79fccc93b4bcb9e"
       "dac078fa45c9456bc9df3dcfb5ec0d6ccbf6248030eed282f303c1711"},
      {3,
       "dd8197de0748d048891fce8b6f0b99c50cb9a50d1b95c5db17ec4aa30631984ebdce580"
       "35a09968a942619c8e7711564809549b3512afe3a27fdc2e5491eed4c"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "421a35a60054e5f383b6137e43d44e998f496748cc77258240ccfaa8730b51f40cf47c1"
       "bc09c728a8cd4f096731298d51463f15af89543fed478053346260c38"},
      {'B',
       "d8ca35343ac8e74ac6e70cb3f4b952a5cd19bbc402e80d0f833ef82b79fccc93b4bcb9e"
       "dac078fa45c9456bc9df3dcfb5ec0d6ccbf6248030eed282f303c1711"},
      {'C',
       "dd8197de0748d048891fce8b6f0b99c50cb9a50d1b95c5db17ec4aa30631984ebdce580"
       "35a09968a942619c8e7711564809549b3512afe3a27fdc2e5491eed4c"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "00197a4f5f1ff8c356a78f6921b5a6bfbf71df8dbd313fbc5095a55de756bfa1ea72406"
       "95005149294f2a2e419ae251fe2f7dbb67c3bb647c2ac1be05eec7ef9"},
      {"two",
       "ac3b6998ac9c5e2c7ee8330010a7b0f87ac9dee7ea547d4d8cd00ab7ad1bd5f57f80af2"
       "ba711a9eb137b4e83b503d24cd7665399a48734d47fff324fb74551e2"},
      {"three",
       "ce4fd4068e56eb07a6e79d007aed4bc8257e10827c74ee422d82a29b2ce8cb079fead81"
       "d9df0513bb577f3b6c47843b17c964e7ff8f4198f32027533eaf5bcc1"}};
};

//
// SHA1
//

//! \brief SHA1 digests
//! \since docker-finance 1.0.0
struct HashData_SHA1 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{"e321e4126974b9818625f6847e570661eaa436bb"};
  const t_encoded kCharArrayDigest{"e321e4126974b9818625f6847e570661eaa436bb"};
  const t_encoded kCharDigest{"6dcd4ce23d88e2ee9568ba546c007c63d9131c1b"};
  const t_encoded kByteDigest{"6dcd4ce23d88e2ee9568ba546c007c63d9131c1b"};

  // integral
  const t_encoded kInt16Digest{"757c3591bceaf1f79929056b743736221b54151f"};
  const t_encoded kInt32Digest{"75878664309b1e12165b0823315376b0d27e4f80"};
  const t_encoded kInt64Digest{"458b642b137e2c76e0b746c6fa43e64c3d4c47f1"};
  const t_encoded kUInt16Digest{"0cca08dd76e222548eed11f9c7bb0f3bffd1792e"};
  const t_encoded kUInt32Digest{"1d193a8b2ac32f724eb7126536af21e885e6048d"};
  const t_encoded kUInt64Digest{"234524f46607504594696f875bd0ca86fe0ee671"};

  // string
  const t_encoded kStringDigest{"e321e4126974b9818625f6847e570661eaa436bb"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"630df194c5b10abfa95d9d050a3cb36c24a133d2",
       "d714329621891484c1a89091a703f13163a9e2e2",
       "52d042673ff4e30435bf417ccf4751586c9ff907"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"630df194c5b10abfa95d9d050a3cb36c24a133d2",
       "d714329621891484c1a89091a703f13163a9e2e2",
       "52d042673ff4e30435bf417ccf4751586c9ff907"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"6dcd4ce23d88e2ee9568ba546c007c63d9131c1b",
       "ae4f281df5a5d0ff3cad6371f76d5c29b6d953ec",
       "32096c2e0eff33d844ee6d675407ace18289357d"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"6dcd4ce23d88e2ee9568ba546c007c63d9131c1b",
       "ae4f281df5a5d0ff3cad6371f76d5c29b6d953ec",
       "32096c2e0eff33d844ee6d675407ace18289357d"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"356a192b7913b04c54574d18c28d46e6395428ab",
       "da4b9237bacccdf19c0760cab7aec4a8359010b0",
       "77de68daecd823babbb58edb1c8e14d7106e83bb"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "630df194c5b10abfa95d9d050a3cb36c24a133d2"},
      {2, "d714329621891484c1a89091a703f13163a9e2e2"},
      {3, "52d042673ff4e30435bf417ccf4751586c9ff907"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "630df194c5b10abfa95d9d050a3cb36c24a133d2"},
      {2, "d714329621891484c1a89091a703f13163a9e2e2"},
      {3, "52d042673ff4e30435bf417ccf4751586c9ff907"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "6dcd4ce23d88e2ee9568ba546c007c63d9131c1b"},
      {2, "ae4f281df5a5d0ff3cad6371f76d5c29b6d953ec"},
      {3, "32096c2e0eff33d844ee6d675407ace18289357d"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "6dcd4ce23d88e2ee9568ba546c007c63d9131c1b"},
      {'B', "ae4f281df5a5d0ff3cad6371f76d5c29b6d953ec"},
      {'C', "32096c2e0eff33d844ee6d675407ace18289357d"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one", "356a192b7913b04c54574d18c28d46e6395428ab"},
      {"two", "da4b9237bacccdf19c0760cab7aec4a8359010b0"},
      {"three", "77de68daecd823babbb58edb1c8e14d7106e83bb"}};
};

//
// SHA2
//

//! \brief SHA2-224 digests
//! \since docker-finance 1.0.0
struct HashData_SHA2_224 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "9e68ebd165b75191dd40a271e356c5fe74e1c0e9e95875fac442a519"};
  const t_encoded kCharArrayDigest{
      "9e68ebd165b75191dd40a271e356c5fe74e1c0e9e95875fac442a519"};
  const t_encoded kCharDigest{
      "5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2"};
  const t_encoded kByteDigest{
      "5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2"};

  // integral
  const t_encoded kInt16Digest{
      "5c5375b097c36d120dd93cefb1fc60ecc5c75f8718217f301c9dde19"};
  const t_encoded kInt32Digest{
      "14a1e8a9f4ecf1a57f20564c3a7ec62df3108f6da804ba0ba50131fd"};
  const t_encoded kInt64Digest{
      "e6d7593f541250f45e758f178ebb314247225ea85c38677d0de13499"};
  const t_encoded kUInt16Digest{
      "1726c64b57fcc6e5811c1c48a3c41075c7993b9619c1ea3cd56a9471"};
  const t_encoded kUInt32Digest{
      "e6a61d339a4eb5e3ebb9328fc0baa31dacb8f2adc995781399c90a96"};
  const t_encoded kUInt64Digest{
      "0aca237baf743228821b774a7a12312df24cc973a2c0b9f93965ab82"};

  // string
  const t_encoded kStringDigest{
      "9e68ebd165b75191dd40a271e356c5fe74e1c0e9e95875fac442a519"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"bdf32e45f4d6aa05d0b957f041bda8f0983b22327dd2609814b782ad",
       "d3348fb312b0a08dcc93985a4fcc83aa214876c9d802f0a1d9787dd9",
       "e3e2b65dfae4d25f44048e475ab19bc05987242e264d28f7b1ee4c55"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"bdf32e45f4d6aa05d0b957f041bda8f0983b22327dd2609814b782ad",
       "d3348fb312b0a08dcc93985a4fcc83aa214876c9d802f0a1d9787dd9",
       "e3e2b65dfae4d25f44048e475ab19bc05987242e264d28f7b1ee4c55"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2",
       "d56cc1ac775bdf54c4536f6ef08de810454487513640e201af923c79",
       "484d52691fcadbfabec5a318d1cf9692c7f293cbc8c1d5f22b2d839b"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2",
       "d56cc1ac775bdf54c4536f6ef08de810454487513640e201af923c79",
       "484d52691fcadbfabec5a318d1cf9692c7f293cbc8c1d5f22b2d839b"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"e25388fde8290dc286a6164fa2d97e551b53498dcbf7bc378eb1f178",
       "58b2aaa0bfae7acc021b3260e941117b529b2e69de878fd7d45c61a9",
       "4cfc3a1811fe40afa401b25ef7fa0379f1f7c1930a04f8755d678474"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "bdf32e45f4d6aa05d0b957f041bda8f0983b22327dd2609814b782ad"},
      {2, "d3348fb312b0a08dcc93985a4fcc83aa214876c9d802f0a1d9787dd9"},
      {3, "e3e2b65dfae4d25f44048e475ab19bc05987242e264d28f7b1ee4c55"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "bdf32e45f4d6aa05d0b957f041bda8f0983b22327dd2609814b782ad"},
      {2, "d3348fb312b0a08dcc93985a4fcc83aa214876c9d802f0a1d9787dd9"},
      {3, "e3e2b65dfae4d25f44048e475ab19bc05987242e264d28f7b1ee4c55"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2"},
      {2, "d56cc1ac775bdf54c4536f6ef08de810454487513640e201af923c79"},
      {3, "484d52691fcadbfabec5a318d1cf9692c7f293cbc8c1d5f22b2d839b"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "5cfe2cddbb9940fb4d8505e25ea77e763a0077693dbb01b1a6aa94f2"},
      {'B', "d56cc1ac775bdf54c4536f6ef08de810454487513640e201af923c79"},
      {'C', "484d52691fcadbfabec5a318d1cf9692c7f293cbc8c1d5f22b2d839b"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one", "e25388fde8290dc286a6164fa2d97e551b53498dcbf7bc378eb1f178"},
      {"two", "58b2aaa0bfae7acc021b3260e941117b529b2e69de878fd7d45c61a9"},
      {"three", "4cfc3a1811fe40afa401b25ef7fa0379f1f7c1930a04f8755d678474"}};
};

//! \brief SHA2-256 digests
//! \since docker-finance 1.0.0
struct HashData_SHA2_256 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "f369d3ac5349ff95eccb542229c3e53ed6a4430aca916b0b61a07c23c35231f4"};
  const t_encoded kCharArrayDigest{
      "f369d3ac5349ff95eccb542229c3e53ed6a4430aca916b0b61a07c23c35231f4"};
  const t_encoded kCharDigest{
      "559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd"};
  const t_encoded kByteDigest{
      "559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd"};

  // integral
  const t_encoded kInt16Digest{
      "d8442baf6f437f1f682f0cae9dde1af2d3c595d88a36971881d36b304785d01c"};
  const t_encoded kInt32Digest{
      "972dcafa6fb4c2c88bce752fca4ab18c6bd88599330a4ad9813915b05bfbe76d"};
  const t_encoded kInt64Digest{
      "b34a1c30a715f6bf8b7243afa7fab883ce3612b7231716bdcbbdc1982e1aed29"};
  const t_encoded kUInt16Digest{
      "f2f89ede8e7d4b3d2243dea1ca96b8ece56f793811d9708b4a0181bf81a50011"};
  const t_encoded kUInt32Digest{
      "f807212b6748a8300fc3702322caa515edf0a6ed9bbc86e94c451e351b080c60"};
  const t_encoded kUInt64Digest{
      "2cdb26265b4dc65e3b44d694f121fd6de99b9e4b8ae7f08d84bfa9537635ae43"};

  // string
  const t_encoded kStringDigest{
      "f369d3ac5349ff95eccb542229c3e53ed6a4430aca916b0b61a07c23c35231f4"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"3edc274f12ee1cd46e13fc4949244f89e0d1d1d066a6c56345c57fc555d90df8",
       "fcc960f5203b2793d60ed497e9f2148488a80ed3839b78a0f6cb71aca638cb55",
       "9f1d4250b0115ee2a8bbc876f90f2048159bafd860d61a4fd1de3df44ee0aa55"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"3edc274f12ee1cd46e13fc4949244f89e0d1d1d066a6c56345c57fc555d90df8",
       "fcc960f5203b2793d60ed497e9f2148488a80ed3839b78a0f6cb71aca638cb55",
       "9f1d4250b0115ee2a8bbc876f90f2048159bafd860d61a4fd1de3df44ee0aa55"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd",
       "df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c",
       "6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd",
       "df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c",
       "6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b",
       "d4735e3a265e16eee03f59718b9b5d03019c07d8b6c51f90da3a666eec13ab35",
       "4e07408562bedb8b60ce05c1decfe3ad16b72230967de01f640b7e4729b49fce"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "3edc274f12ee1cd46e13fc4949244f89e0d1d1d066a6c56345c57fc555d90df8"},
      {2, "fcc960f5203b2793d60ed497e9f2148488a80ed3839b78a0f6cb71aca638cb55"},
      {3, "9f1d4250b0115ee2a8bbc876f90f2048159bafd860d61a4fd1de3df44ee0aa55"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "3edc274f12ee1cd46e13fc4949244f89e0d1d1d066a6c56345c57fc555d90df8"},
      {2, "fcc960f5203b2793d60ed497e9f2148488a80ed3839b78a0f6cb71aca638cb55"},
      {3, "9f1d4250b0115ee2a8bbc876f90f2048159bafd860d61a4fd1de3df44ee0aa55"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd"},
      {2, "df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c"},
      {3, "6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd"},
      {'B', "df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c"},
      {'C',
       "6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b"},
      {"two",
       "d4735e3a265e16eee03f59718b9b5d03019c07d8b6c51f90da3a666eec13ab35"},
      {"three",
       "4e07408562bedb8b60ce05c1decfe3ad16b72230967de01f640b7e4729b49fce"}};
};

//! \brief SHA2-384 digests
//! \since docker-finance 1.0.0
struct HashData_SHA2_384 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "93d51fe96ecbcd0b7a4a1172f554cc3349581ddec47b49236acf9b1fcda3b46f056303fc"
      "4684a5f224e1bdb364731a50"};
  const t_encoded kCharArrayDigest{
      "93d51fe96ecbcd0b7a4a1172f554cc3349581ddec47b49236acf9b1fcda3b46f056303fc"
      "4684a5f224e1bdb364731a50"};
  const t_encoded kCharDigest{
      "ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac072998537"
      "4beeaa5b80a504d048be1864"};
  const t_encoded kByteDigest{
      "ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac072998537"
      "4beeaa5b80a504d048be1864"};

  // trivial
  const t_encoded kInt16Digest{
      "c8ce400d5021894061957923622d5162ed06f11bac6a1902dd678c57719eb18f00614578"
      "d8f43c928d17b84ec185bca8"};
  const t_encoded kInt32Digest{
      "20da2b0895553890352d767a5d345ac396c38231b2556862aa5bd0591d7f1bc9854c13be"
      "ea6a0519d2a3e4ff4d07f932"};
  const t_encoded kInt64Digest{
      "4bc7d78da95bb03325ab72f4006696f79e3472f71b873869cc2e3828630d967f74876c27"
      "938f1cb16c1e425498f98284"};
  const t_encoded kUInt16Digest{
      "6483f34ebcdc37cc6d4bb7d10b549d7460dd1014ce820ff6e2db7b09adff027ef0ba261f"
      "e9dab5ab6265ee88fec79a0c"};
  const t_encoded kUInt32Digest{
      "841e4a4899fb4b463b08973475ed471a8035e155391456f40a3ea444a088d16ab3185869"
      "ba80ec22b18cb841a9807b1b"};
  const t_encoded kUInt64Digest{
      "5a77a181f7030f8bef208ba8fd03cc7061369994cfdcbca2f2c527a316103c73aa09882b"
      "0982d15ca8415374e3fdf22c"};

  // string
  const t_encoded kStringDigest{
      "93d51fe96ecbcd0b7a4a1172f554cc3349581ddec47b49236acf9b1fcda3b46f056303fc"
      "4684a5f224e1bdb364731a50"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"976502df6024f1d826fe4ab22bbc087aa576f73ce5f7cbb147f271a3d1b4f2d6d42deb9"
       "34619ba570b9f543d4b08cdfd",
       "62ba8d8f137b5ddf52207a40203788e071383bc1f62f4f786a9727700ab8bafe9d78e1d"
       "4f71f206735b816e53862e3d6",
       "513e9bcc7f6ead4e52f2d1a606d858ad63edca2609d8710425c01ee8b2e374ca15758f8"
       "c82b2577303f1f9ad6d6076f2"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"976502df6024f1d826fe4ab22bbc087aa576f73ce5f7cbb147f271a3d1b4f2d6d42deb9"
       "34619ba570b9f543d4b08cdfd",
       "62ba8d8f137b5ddf52207a40203788e071383bc1f62f4f786a9727700ab8bafe9d78e1d"
       "4f71f206735b816e53862e3d6",
       "513e9bcc7f6ead4e52f2d1a606d858ad63edca2609d8710425c01ee8b2e374ca15758f8"
       "c82b2577303f1f9ad6d6076f2"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac07299853"
       "74beeaa5b80a504d048be1864",
       "8a5e6d9081b08ada24918c6a8697952bc7c7c92f74a3341eb4a31be93dd425c8781f88c"
       "2f2fe40d5f81018ba81a54b48",
       "7860d388ac9e470c83d65c4b0b66bdd00e6c96fbadc78882174e020fab9793a6221724b"
       "3df9a2ec99f9395d9a410b9ed"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac07299853"
       "74beeaa5b80a504d048be1864",
       "8a5e6d9081b08ada24918c6a8697952bc7c7c92f74a3341eb4a31be93dd425c8781f88c"
       "2f2fe40d5f81018ba81a54b48",
       "7860d388ac9e470c83d65c4b0b66bdd00e6c96fbadc78882174e020fab9793a6221724b"
       "3df9a2ec99f9395d9a410b9ed"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"47f05d367b0c32e438fb63e6cf4a5f35c2aa2f90dc7543f8a41a0f95ce8a40a313ab5cf"
       "36134a2068c4c969cb50db776",
       "d063457705d66d6f016e4cdd747db3af8d70ebfd36badd63de6c8ca4a9d8bfb5d874e7f"
       "bd750aa804dcaddae7eeef51e",
       "6af11c83586822c3c74bb3ccef728bae5cfee67cad82dd7402711e530bec782fc02aff2"
       "73569d22ddffb3b145f343768"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "976502df6024f1d826fe4ab22bbc087aa576f73ce5f7cbb147f271a3d1b4f2d6d42deb9"
       "34619ba570b9f543d4b08cdfd"},
      {2,
       "62ba8d8f137b5ddf52207a40203788e071383bc1f62f4f786a9727700ab8bafe9d78e1d"
       "4f71f206735b816e53862e3d6"},
      {3,
       "513e9bcc7f6ead4e52f2d1a606d858ad63edca2609d8710425c01ee8b2e374ca15758f8"
       "c82b2577303f1f9ad6d6076f2"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "976502df6024f1d826fe4ab22bbc087aa576f73ce5f7cbb147f271a3d1b4f2d6d42deb9"
       "34619ba570b9f543d4b08cdfd"},
      {2,
       "62ba8d8f137b5ddf52207a40203788e071383bc1f62f4f786a9727700ab8bafe9d78e1d"
       "4f71f206735b816e53862e3d6"},
      {3,
       "513e9bcc7f6ead4e52f2d1a606d858ad63edca2609d8710425c01ee8b2e374ca15758f8"
       "c82b2577303f1f9ad6d6076f2"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac07299853"
       "74beeaa5b80a504d048be1864"},
      {2,
       "8a5e6d9081b08ada24918c6a8697952bc7c7c92f74a3341eb4a31be93dd425c8781f88c"
       "2f2fe40d5f81018ba81a54b48"},
      {3,
       "7860d388ac9e470c83d65c4b0b66bdd00e6c96fbadc78882174e020fab9793a6221724b"
       "3df9a2ec99f9395d9a410b9ed"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "ad14aaf25020bef2fd4e3eb5ec0c50272cdfd66074b0ed037c9a11254321aac07299853"
       "74beeaa5b80a504d048be1864"},
      {'B',
       "8a5e6d9081b08ada24918c6a8697952bc7c7c92f74a3341eb4a31be93dd425c8781f88c"
       "2f2fe40d5f81018ba81a54b48"},
      {'C',
       "7860d388ac9e470c83d65c4b0b66bdd00e6c96fbadc78882174e020fab9793a6221724b"
       "3df9a2ec99f9395d9a410b9ed"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "47f05d367b0c32e438fb63e6cf4a5f35c2aa2f90dc7543f8a41a0f95ce8a40a313ab5cf"
       "36134a2068c4c969cb50db776"},
      {"two",
       "d063457705d66d6f016e4cdd747db3af8d70ebfd36badd63de6c8ca4a9d8bfb5d874e7f"
       "bd750aa804dcaddae7eeef51e"},
      {"three",
       "6af11c83586822c3c74bb3ccef728bae5cfee67cad82dd7402711e530bec782fc02aff2"
       "73569d22ddffb3b145f343768"}};
};

//! \brief SHA2-512 digests
//! \since docker-finance 1.0.0
struct HashData_SHA2_512 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "2638860d3b51c4bf3d51fe23733bb7ddf52a5d0e3ad2853447e298bea5962dd7dc96e7a5"
      "eda42d5f4f59e1fc551df8fdf556f9228f888fdc4955444983de5537"};
  const t_encoded kCharArrayDigest{
      "2638860d3b51c4bf3d51fe23733bb7ddf52a5d0e3ad2853447e298bea5962dd7dc96e7a5"
      "eda42d5f4f59e1fc551df8fdf556f9228f888fdc4955444983de5537"};
  const t_encoded kCharDigest{
      "21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd083"
      "bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5"};
  const t_encoded kByteDigest{
      "21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd083"
      "bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5"};

  // integral
  const t_encoded kInt16Digest{
      "c4cf7a33fd0ad54dece2df22ae70814fb0e5f950f99bb018e12e0c1c10b204a442671471"
      "4b4e14ad7560c33fb27ee2efcef7bcc67014d2f13de28860725bc884"};
  const t_encoded kInt32Digest{
      "ff561ab18cbd3379ff2abb77b43e23331bea8520c99ea1dfa255b985efeb671fc46ed96b"
      "774da5837397bb3b7bbdd1b8e66237804f94da3b341b00d80a3fd788"};
  const t_encoded kInt64Digest{
      "85a4f762e59771074e103a5b1fedf1f6c262814d8a87cffb08a5082b4be0de43a7b186ab"
      "402cb919fb7d6a4dba1b57b0c2a06071d11bb83ee3f1977abbcf59e9"};
  const t_encoded kUInt16Digest{
      "676015d3f7b293495beaf3827cbe3ae9a873866c21367a7dc969a2dae9ecf4d6f3d7fca2"
      "225c255a8858dfb93a7843669bdf20749f7443897f98a47289849500"};
  const t_encoded kUInt32Digest{
      "115c5dd07b816b208e0fb06474fa77262994a27629298552f65bcb17cbeb1bc08330f810"
      "fcf46ca198230a57b53b97e810f61cd998f4972081df14f57a99b5a3"};
  const t_encoded kUInt64Digest{
      "2b1d6802dd9b3d7f35030f29b2cd88191749a977ec432a9aad010defb21c77748d606a4f"
      "ef7c05a3946d7dd1586a8b53fa673b10639bac330d8230c0952a4fb4"};

  // string
  const t_encoded kStringDigest{
      "2638860d3b51c4bf3d51fe23733bb7ddf52a5d0e3ad2853447e298bea5962dd7dc96e7a5"
      "eda42d5f4f59e1fc551df8fdf556f9228f888fdc4955444983de5537"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"0064adf229660e62651526a5562d27ea8caf164eff40e913522e5da3e7629377cdf352c"
       "367f078d318f8ec3b4f7e358679461c1a036519cc972a752d98120949",
       "ab2ec075c03715b32c3660beeef626719bf395a7885929594c4faa746ad3105eeb35fa6"
       "4ebc755cd749af41eb3d1190046b80b46c29636ac3b4bc58eed4bb380",
       "e9aba5ffdfbcd421bbd205f2bc040149caec334b4437fb6e2a341378ff07e925e8a5755"
       "7a3f45c08ebf02f5add60861f674040154d0f8353268e317b8cf3a58b"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"0064adf229660e62651526a5562d27ea8caf164eff40e913522e5da3e7629377cdf352c"
       "367f078d318f8ec3b4f7e358679461c1a036519cc972a752d98120949",
       "ab2ec075c03715b32c3660beeef626719bf395a7885929594c4faa746ad3105eeb35fa6"
       "4ebc755cd749af41eb3d1190046b80b46c29636ac3b4bc58eed4bb380",
       "e9aba5ffdfbcd421bbd205f2bc040149caec334b4437fb6e2a341378ff07e925e8a5755"
       "7a3f45c08ebf02f5add60861f674040154d0f8353268e317b8cf3a58b"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd08"
       "3bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5",
       "848b0779ff415f0af4ea14df9dd1d3c29ac41d836c7808896c4eba19c51ac40a439caf5"
       "e61ec88c307c7d619195229412eaa73fb2a5ea20d23cc86a9d8f86a0f",
       "3d637ae63d59522dd3cb1b81c1ad67e56d46185b0971e0bc7dd2d8ad3b26090acb634c2"
       "52fc6a63b3766934314ea1a6e59fa0c8c2bc027a7b6a460b291cd4dfb"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd08"
       "3bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5",
       "848b0779ff415f0af4ea14df9dd1d3c29ac41d836c7808896c4eba19c51ac40a439caf5"
       "e61ec88c307c7d619195229412eaa73fb2a5ea20d23cc86a9d8f86a0f",
       "3d637ae63d59522dd3cb1b81c1ad67e56d46185b0971e0bc7dd2d8ad3b26090acb634c2"
       "52fc6a63b3766934314ea1a6e59fa0c8c2bc027a7b6a460b291cd4dfb"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903"
       "a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a",
       "40b244112641dd78dd4f93b6c9190dd46e0099194d5a44257b7efad6ef9ff4683da1eda"
       "0244448cb343aa688f5d3efd7314dafe580ac0bcbf115aeca9e8dc114",
       "3bafbf08882a2d10133093a1b8433f50563b93c14acd05b79028eb1d127990272414509"
       "80651994501423a66c276ae26c43b739bc65c4e16b10c3af6c202aebb"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "0064adf229660e62651526a5562d27ea8caf164eff40e913522e5da3e7629377cdf352c"
       "367f078d318f8ec3b4f7e358679461c1a036519cc972a752d98120949"},
      {2,
       "ab2ec075c03715b32c3660beeef626719bf395a7885929594c4faa746ad3105eeb35fa6"
       "4ebc755cd749af41eb3d1190046b80b46c29636ac3b4bc58eed4bb380"},
      {3,
       "e9aba5ffdfbcd421bbd205f2bc040149caec334b4437fb6e2a341378ff07e925e8a5755"
       "7a3f45c08ebf02f5add60861f674040154d0f8353268e317b8cf3a58b"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "0064adf229660e62651526a5562d27ea8caf164eff40e913522e5da3e7629377cdf352c"
       "367f078d318f8ec3b4f7e358679461c1a036519cc972a752d98120949"},
      {2,
       "ab2ec075c03715b32c3660beeef626719bf395a7885929594c4faa746ad3105eeb35fa6"
       "4ebc755cd749af41eb3d1190046b80b46c29636ac3b4bc58eed4bb380"},
      {3,
       "e9aba5ffdfbcd421bbd205f2bc040149caec334b4437fb6e2a341378ff07e925e8a5755"
       "7a3f45c08ebf02f5add60861f674040154d0f8353268e317b8cf3a58b"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd08"
       "3bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5"},
      {2,
       "848b0779ff415f0af4ea14df9dd1d3c29ac41d836c7808896c4eba19c51ac40a439caf5"
       "e61ec88c307c7d619195229412eaa73fb2a5ea20d23cc86a9d8f86a0f"},
      {3,
       "3d637ae63d59522dd3cb1b81c1ad67e56d46185b0971e0bc7dd2d8ad3b26090acb634c2"
       "52fc6a63b3766934314ea1a6e59fa0c8c2bc027a7b6a460b291cd4dfb"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "21b4f4bd9e64ed355c3eb676a28ebedaf6d8f17bdc365995b319097153044080516bd08"
       "3bfcce66121a3072646994c8430cc382b8dc543e84880183bf856cff5"},
      {'B',
       "848b0779ff415f0af4ea14df9dd1d3c29ac41d836c7808896c4eba19c51ac40a439caf5"
       "e61ec88c307c7d619195229412eaa73fb2a5ea20d23cc86a9d8f86a0f"},
      {'C',
       "3d637ae63d59522dd3cb1b81c1ad67e56d46185b0971e0bc7dd2d8ad3b26090acb634c2"
       "52fc6a63b3766934314ea1a6e59fa0c8c2bc027a7b6a460b291cd4dfb"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903"
       "a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a"},
      {"two",
       "40b244112641dd78dd4f93b6c9190dd46e0099194d5a44257b7efad6ef9ff4683da1eda"
       "0244448cb343aa688f5d3efd7314dafe580ac0bcbf115aeca9e8dc114"},
      {"three",
       "3bafbf08882a2d10133093a1b8433f50563b93c14acd05b79028eb1d127990272414509"
       "80651994501423a66c276ae26c43b739bc65c4e16b10c3af6c202aebb"}};
};

//
// SHA3
//

//! \brief SHA3-224 digests
//! \since docker-finance 1.0.0
struct HashData_SHA3_224 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "13a852d1b6bb61d9dbbefb5d3861c06888efbebe45a809e551c26056"};
  const t_encoded kCharArrayDigest{
      "13a852d1b6bb61d9dbbefb5d3861c06888efbebe45a809e551c26056"};
  const t_encoded kCharDigest{
      "97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca"};
  const t_encoded kByteDigest{
      "97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca"};

  // integral
  const t_encoded kInt16Digest{
      "595c2cd8893813145ecc043dec23a994a12f083dcb4915e4cc822af8"};
  const t_encoded kInt32Digest{
      "dd078f75641cb8cd1af8514d63b469aa32c960c8df0d2a8a45f9d0d9"};
  const t_encoded kInt64Digest{
      "c7fca650ec5751df139bad7a69e40b5833a23c25c7589fb0b6e35d75"};
  const t_encoded kUInt16Digest{
      "ff1f0605b4f7ef13c4b0e0c282c4c0219e3295bc8fde0019db5be097"};
  const t_encoded kUInt32Digest{
      "26d6ecb65ecf5f20329d42cb23dd479b3dd059ed2283be690f0c5406"};
  const t_encoded kUInt64Digest{
      "34ed24b12191b4cbf2c66915347d90f0613809e00f73d0f751880af4"};

  // string
  const t_encoded kStringDigest{
      "13a852d1b6bb61d9dbbefb5d3861c06888efbebe45a809e551c26056"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"e67f2cf381b35a0db67f43f0d437c8a0321f6d258acee1a362c17895",
       "b7c9f87aeabc06f1885d41643f4e9a0e5f50af8573502af043c952c4",
       "cd1c40f0837257e0b76e5099a025562107b50238b39f6c7826f5b8c3"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"e67f2cf381b35a0db67f43f0d437c8a0321f6d258acee1a362c17895",
       "b7c9f87aeabc06f1885d41643f4e9a0e5f50af8573502af043c952c4",
       "cd1c40f0837257e0b76e5099a025562107b50238b39f6c7826f5b8c3"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca",
       "b60bd459170afa28b3ef45a22ce41ede9ad62a9a0b250482a7e1beb6",
       "64d7aef1a5121b17a819ed6a6da3ac4930420a916f30acfe909f9482"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca",
       "b60bd459170afa28b3ef45a22ce41ede9ad62a9a0b250482a7e1beb6",
       "64d7aef1a5121b17a819ed6a6da3ac4930420a916f30acfe909f9482"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"300d01f3a910045fefa16d6a149f38167b2503dbc37c1b24fd6f751e",
       "f3ff4f073ed24d62051c8d7bb73418b95db2f6ff9e4441af466f6d98",
       "b6f194539618d1e5eec08a56b8c7d09b8198fe1faa3f16e9703b91bd"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "e67f2cf381b35a0db67f43f0d437c8a0321f6d258acee1a362c17895"},
      {2, "b7c9f87aeabc06f1885d41643f4e9a0e5f50af8573502af043c952c4"},
      {3, "cd1c40f0837257e0b76e5099a025562107b50238b39f6c7826f5b8c3"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "e67f2cf381b35a0db67f43f0d437c8a0321f6d258acee1a362c17895"},
      {2, "b7c9f87aeabc06f1885d41643f4e9a0e5f50af8573502af043c952c4"},
      {3, "cd1c40f0837257e0b76e5099a025562107b50238b39f6c7826f5b8c3"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca"},
      {2, "b60bd459170afa28b3ef45a22ce41ede9ad62a9a0b250482a7e1beb6"},
      {3, "64d7aef1a5121b17a819ed6a6da3ac4930420a916f30acfe909f9482"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "97e2f98c0938943ab1a18a1721a04dff922ecc1ad14d4bbf905c02ca"},
      {'B', "b60bd459170afa28b3ef45a22ce41ede9ad62a9a0b250482a7e1beb6"},
      {'C', "64d7aef1a5121b17a819ed6a6da3ac4930420a916f30acfe909f9482"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one", "300d01f3a910045fefa16d6a149f38167b2503dbc37c1b24fd6f751e"},
      {"two", "f3ff4f073ed24d62051c8d7bb73418b95db2f6ff9e4441af466f6d98"},
      {"three", "b6f194539618d1e5eec08a56b8c7d09b8198fe1faa3f16e9703b91bd"}};
};

//! \brief SHA3-256 digests
//! \since docker-finance 1.0.0
struct HashData_SHA3_256 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "c60f839cdc885825f4c0c68b9e9e6b60818ea1d238a8d886c614407990e3645e"};
  const t_encoded kCharArrayDigest{
      "c60f839cdc885825f4c0c68b9e9e6b60818ea1d238a8d886c614407990e3645e"};
  const t_encoded kCharDigest{
      "1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6"};
  const t_encoded kByteDigest{
      "1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6"};

  // integral
  const t_encoded kInt16Digest{
      "9691d3d7a4f428a63c6b2b565bca34ccfd3998c18d39dd0412fd268115ebcdc5"};
  const t_encoded kInt32Digest{
      "69cab95cf0c5d0fae6b04a5da61c32ee00fd0139475590d9488b8a0dba6c72dd"};
  const t_encoded kInt64Digest{
      "177753cd8ceaee9af28d49eb2cad0dd99584ba8e8a3405505c5dee15c3df085b"};
  const t_encoded kUInt16Digest{
      "826d91290ceae2dd95ecc7697a3bed0878c165a85eb8a6165b96228337fbe109"};
  const t_encoded kUInt32Digest{
      "e05cc330faff9e8652ecec4fa41ba4f793c2c65f9b0012bd2d4147de4882dfb0"};
  const t_encoded kUInt64Digest{
      "b24a6eca932d14afe04bd6bd21cad6c70df1a3b59b2208c9d97f8014694989ac"};

  // string
  const t_encoded kStringDigest{
      "c60f839cdc885825f4c0c68b9e9e6b60818ea1d238a8d886c614407990e3645e"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"973241d0048acaecf86697a8a38778ba12bb50a982cb97493765f66dbf78987c",
       "2cf8b0731ce26c1ff40b111030f53164f716f75635be2f94490cce631efe1315",
       "1356d2d0c688f85690c33df194bec7b3683190923e7a5db3ad483e3c433b202a"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"973241d0048acaecf86697a8a38778ba12bb50a982cb97493765f66dbf78987c",
       "2cf8b0731ce26c1ff40b111030f53164f716f75635be2f94490cce631efe1315",
       "1356d2d0c688f85690c33df194bec7b3683190923e7a5db3ad483e3c433b202a"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6",
       "521ec18851e17bbba961bc46c70baf03ee67ebdea11a8306de39c15a90e9d2e5",
       "2248e6be26f60c9baa59adbda2a136a4a5305d7b475d8465ba4911b4886e39a5"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6",
       "521ec18851e17bbba961bc46c70baf03ee67ebdea11a8306de39c15a90e9d2e5",
       "2248e6be26f60c9baa59adbda2a136a4a5305d7b475d8465ba4911b4886e39a5"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"67b176705b46206614219f47a05aee7ae6a3edbe850bbbe214c536b989aea4d2",
       "b1b1bd1ed240b1496c81ccf19ceccf2af6fd24fac10ae42023628abbe2687310",
       "1bf0b26eb2090599dd68cbb42c86a674cb07ab7adc103ad3ccdf521bb79056b9"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "973241d0048acaecf86697a8a38778ba12bb50a982cb97493765f66dbf78987c"},
      {2, "2cf8b0731ce26c1ff40b111030f53164f716f75635be2f94490cce631efe1315"},
      {3, "1356d2d0c688f85690c33df194bec7b3683190923e7a5db3ad483e3c433b202a"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "973241d0048acaecf86697a8a38778ba12bb50a982cb97493765f66dbf78987c"},
      {2, "2cf8b0731ce26c1ff40b111030f53164f716f75635be2f94490cce631efe1315"},
      {3, "1356d2d0c688f85690c33df194bec7b3683190923e7a5db3ad483e3c433b202a"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6"},
      {2, "521ec18851e17bbba961bc46c70baf03ee67ebdea11a8306de39c15a90e9d2e5"},
      {3, "2248e6be26f60c9baa59adbda2a136a4a5305d7b475d8465ba4911b4886e39a5"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "1c9ebd6caf02840a5b2b7f0fc870ec1db154886ae9fe621b822b14fd0bf513d6"},
      {'B', "521ec18851e17bbba961bc46c70baf03ee67ebdea11a8306de39c15a90e9d2e5"},
      {'C',
       "2248e6be26f60c9baa59adbda2a136a4a5305d7b475d8465ba4911b4886e39a5"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "67b176705b46206614219f47a05aee7ae6a3edbe850bbbe214c536b989aea4d2"},
      {"two",
       "b1b1bd1ed240b1496c81ccf19ceccf2af6fd24fac10ae42023628abbe2687310"},
      {"three",
       "1bf0b26eb2090599dd68cbb42c86a674cb07ab7adc103ad3ccdf521bb79056b9"}};
};

//! \brief SHA3-384 digests
//! \since docker-finance 1.0.0
struct HashData_SHA3_384 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "3caa411e4e909b7365a191b0add87bddf6ad89e46b6a4b241ad6bba8cb76c7b804523748"
      "1cde837a147a77c3173ac571"};
  const t_encoded kCharArrayDigest{
      "3caa411e4e909b7365a191b0add87bddf6ad89e46b6a4b241ad6bba8cb76c7b804523748"
      "1cde837a147a77c3173ac571"};
  const t_encoded kCharDigest{
      "15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7abd"
      "b9553baf220320384c6b1cd6"};
  const t_encoded kByteDigest{
      "15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7abd"
      "b9553baf220320384c6b1cd6"};

  // integral
  const t_encoded kInt16Digest{
      "8d16ba4a479f2dec0a7e97ceb5827918d05ba16546d1769bbe25dfda339ef664992dabf5"
      "7fa9d5a6778946967f0df286"};
  const t_encoded kInt32Digest{
      "17a5bb7ed47db37760c4cab50fc026514f661833839ecdbe0b5352f63fa856ef03fd607e"
      "645eb94254efea64db21b1c3"};
  const t_encoded kInt64Digest{
      "3ecf1b384138d5a9a56630dbb2449cbcc394e52c7eed7496115900bb0fecaa81d6c9671a"
      "17ef288103a5d08d0e4c916e"};
  const t_encoded kUInt16Digest{
      "8fabe9d13ab89238bf6c264e2166b2896a8259ef7cef666a072367f105291a75dc9eb9fb"
      "4fafe0010155814c52c110e5"};
  const t_encoded kUInt32Digest{
      "e9a5a1a968c27ef35d3bc0b7223201503cf999d33456c96bb3f8066b67ce1563ce086f70"
      "bf979fec7fe1744fc73456d2"};
  const t_encoded kUInt64Digest{
      "aa7a8f601edfa414943708781fa9632370e94ccba7ee0690839af5d5947bc9cfb40cbf93"
      "5b16c12f6ce0404f6196a97e"};

  // string
  const t_encoded kStringDigest{
      "3caa411e4e909b7365a191b0add87bddf6ad89e46b6a4b241ad6bba8cb76c7b804523748"
      "1cde837a147a77c3173ac571"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"7ff6cd481f1d36daded198fa0449e56e82bb17c3dcb4503e40094c48243eae9b522a3d9"
       "ba8f73d0ce21c37d8a0bf92de",
       "69066989b84d2385c5695e1c3d65e7d7a8504e73e8e1e25f4ce1e70cd5d6d6c66c742db"
       "a0d08ebbf1119235f89168654",
       "987cfa6fa49f364d3a4ef037c7818a738ba31a0504fd7ecfe674d6c706f0142ef029b37"
       "bf621c032e18c47d062e6cba3"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"7ff6cd481f1d36daded198fa0449e56e82bb17c3dcb4503e40094c48243eae9b522a3d9"
       "ba8f73d0ce21c37d8a0bf92de",
       "69066989b84d2385c5695e1c3d65e7d7a8504e73e8e1e25f4ce1e70cd5d6d6c66c742db"
       "a0d08ebbf1119235f89168654",
       "987cfa6fa49f364d3a4ef037c7818a738ba31a0504fd7ecfe674d6c706f0142ef029b37"
       "bf621c032e18c47d062e6cba3"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7ab"
       "db9553baf220320384c6b1cd6",
       "8283d235852af9bbf7d81037b8b70aaba733a4433a4438f1b944c04c9e1d9d6d927e96d"
       "61b1fb7e7ecfcf2983ad816b5",
       "7b95dc4d4d327449e6b84bd769d053f190568504270b32789990ac7b75bef7fcfda2931"
       "d16cb5785b9ae61a15bbead55"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7ab"
       "db9553baf220320384c6b1cd6",
       "8283d235852af9bbf7d81037b8b70aaba733a4433a4438f1b944c04c9e1d9d6d927e96d"
       "61b1fb7e7ecfcf2983ad816b5",
       "7b95dc4d4d327449e6b84bd769d053f190568504270b32789990ac7b75bef7fcfda2931"
       "d16cb5785b9ae61a15bbead55"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"f39de487a8aed2d19069ed7a7bcfc274e9f026bba97c8f059be6a2e5eed051d7ee437b9"
       "3d80aa6163bf8039543b612dd",
       "39773563a8fc5c19ba80f0dc0f57bf49ba0e804abe8e68a1ed067252c30ef499d54ab4e"
       "b4e8f4cfa2cfac6c83798997e",
       "5f9714f2c47c4ee6af02e96db42b64a3750f5ec5f3541d1a1a6fd20d3632395c55439e2"
       "08557e782f22a9714885b6e0c"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "7ff6cd481f1d36daded198fa0449e56e82bb17c3dcb4503e40094c48243eae9b522a3d9"
       "ba8f73d0ce21c37d8a0bf92de"},
      {2,
       "69066989b84d2385c5695e1c3d65e7d7a8504e73e8e1e25f4ce1e70cd5d6d6c66c742db"
       "a0d08ebbf1119235f89168654"},
      {3,
       "987cfa6fa49f364d3a4ef037c7818a738ba31a0504fd7ecfe674d6c706f0142ef029b37"
       "bf621c032e18c47d062e6cba3"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "7ff6cd481f1d36daded198fa0449e56e82bb17c3dcb4503e40094c48243eae9b522a3d9"
       "ba8f73d0ce21c37d8a0bf92de"},
      {2,
       "69066989b84d2385c5695e1c3d65e7d7a8504e73e8e1e25f4ce1e70cd5d6d6c66c742db"
       "a0d08ebbf1119235f89168654"},
      {3,
       "987cfa6fa49f364d3a4ef037c7818a738ba31a0504fd7ecfe674d6c706f0142ef029b37"
       "bf621c032e18c47d062e6cba3"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7ab"
       "db9553baf220320384c6b1cd6"},
      {2,
       "8283d235852af9bbf7d81037b8b70aaba733a4433a4438f1b944c04c9e1d9d6d927e96d"
       "61b1fb7e7ecfcf2983ad816b5"},
      {3,
       "7b95dc4d4d327449e6b84bd769d053f190568504270b32789990ac7b75bef7fcfda2931"
       "d16cb5785b9ae61a15bbead55"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "15000d20f59aa483b5eac0a1f33abe8e09dea1054d173d3e7443c68035b99240b50f7ab"
       "db9553baf220320384c6b1cd6"},
      {'B',
       "8283d235852af9bbf7d81037b8b70aaba733a4433a4438f1b944c04c9e1d9d6d927e96d"
       "61b1fb7e7ecfcf2983ad816b5"},
      {'C',
       "7b95dc4d4d327449e6b84bd769d053f190568504270b32789990ac7b75bef7fcfda2931"
       "d16cb5785b9ae61a15bbead55"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "f39de487a8aed2d19069ed7a7bcfc274e9f026bba97c8f059be6a2e5eed051d7ee437b9"
       "3d80aa6163bf8039543b612dd"},
      {"two",
       "39773563a8fc5c19ba80f0dc0f57bf49ba0e804abe8e68a1ed067252c30ef499d54ab4e"
       "b4e8f4cfa2cfac6c83798997e"},
      {"three",
       "5f9714f2c47c4ee6af02e96db42b64a3750f5ec5f3541d1a1a6fd20d3632395c55439e2"
       "08557e782f22a9714885b6e0c"}};
};

//! \brief SHA3-512 digests
//! \since docker-finance 1.0.0
struct HashData_SHA3_512 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "6a4aa07cbb12414b2458b42775fc0060345b5393207d9f822cad96c49099cf597570c414"
      "81e69099861ddd2603611ac21640d6320f9997c0d5c317743000549f"};
  const t_encoded kCharArrayDigest{
      "6a4aa07cbb12414b2458b42775fc0060345b5393207d9f822cad96c49099cf597570c414"
      "81e69099861ddd2603611ac21640d6320f9997c0d5c317743000549f"};
  const t_encoded kCharDigest{
      "f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707c"
      "c9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df"};
  const t_encoded kByteDigest{
      "f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707c"
      "c9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df"};

  // integral
  const t_encoded kInt16Digest{
      "7f619c192252de46ca8111a9bedb366f7bf32a88a1b112b1d57eea1574fcab6afa36e5ae"
      "98e27df0e90a08bd6bd9812037f12a6ff4a3b3d6e0bc2ae27e468aa7"};
  const t_encoded kInt32Digest{
      "8bb07758c4183ee11def4ad81e1e8ee9687516aeb9a490eaba5a06dd2bdbd4efc8d03a1f"
      "9b94914975c510063f0107b1c10bbe5b34b636b2bd95ee4d9c180b2f"};
  const t_encoded kInt64Digest{
      "680c7f184f8ae868d55e0882f782732d91dcd795a24bc0862f024a1e237ee4413b4902cc"
      "f71199726061fd41573dcb0e6b7abb6cc3ba2485c2a5b04362798f4c"};
  const t_encoded kUInt16Digest{
      "2220cf4474315fec8c2d47bb64ac516e512fdd81fd324e2fa73ef9b3dd37da042e37ad3c"
      "0ff711704e09feda30a7a25d2f175028e4d594005bddf2dd82778c84"};
  const t_encoded kUInt32Digest{
      "5f4af34c76f18906400fb1109aa338273133c150303e40f6c2f8ce3a554c6ad9bf4e3865"
      "777af971e0cfae5f5f888aba076cb5a8246bcd4b547e08f27c39b197"};
  const t_encoded kUInt64Digest{
      "7d0d532d147304fb57e82c4618508c1ba30f7e77b4444451cafbfb195f92be5b50fc5df6"
      "00447424d7943d57cd403a9afdd2d73a952b784257ed910f2948e12e"};

  // string
  const t_encoded kStringDigest{
      "6a4aa07cbb12414b2458b42775fc0060345b5393207d9f822cad96c49099cf597570c414"
      "81e69099861ddd2603611ac21640d6320f9997c0d5c317743000549f"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"23d2da0c21dc0f8369c9fb0465415ddf65b49a527fa20b87b5c32aef2086840ae56f0c7"
       "496eec79c1dc3e774737d9acb6c4b27660a9034f5525eab9d3a60d784",
       "fe71654787b8d33c869baa366025051df549a7f1fe9ad3d79ba1ea8da0dd6e7607d9924"
       "d926c91ac903a734f4ea98dea0c20bdcdc097ad131ec29304f0d7092b",
       "dc12a214d33ef0dace57c7d92377dbb7117e4ffc29d4977cee08956c75055100ba0e43d"
       "167bb1dbebfdf40f34b8a08c0d8f5a27b9e7643d5334368a3338caf5f"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"23d2da0c21dc0f8369c9fb0465415ddf65b49a527fa20b87b5c32aef2086840ae56f0c7"
       "496eec79c1dc3e774737d9acb6c4b27660a9034f5525eab9d3a60d784",
       "fe71654787b8d33c869baa366025051df549a7f1fe9ad3d79ba1ea8da0dd6e7607d9924"
       "d926c91ac903a734f4ea98dea0c20bdcdc097ad131ec29304f0d7092b",
       "dc12a214d33ef0dace57c7d92377dbb7117e4ffc29d4977cee08956c75055100ba0e43d"
       "167bb1dbebfdf40f34b8a08c0d8f5a27b9e7643d5334368a3338caf5f"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707"
       "cc9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df",
       "7b637bc5543d96f49500aaad3b27d8bd37624db23d415c4d0f3dd231e9b9fb061f39b7d"
       "8561c540650de8bef02aca43a2069cc2512697bd34f2244ee732743a9",
       "be0c2f5b07cb96d61f0e1e3fd250f0a1709d7388a24e192b9734ae0f9c92abcbd095fed"
       "6c5978ad9e55bb62a6a2ca16532eb3f3e5b3bc1e64209546e7e887445"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707"
       "cc9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df",
       "7b637bc5543d96f49500aaad3b27d8bd37624db23d415c4d0f3dd231e9b9fb061f39b7d"
       "8561c540650de8bef02aca43a2069cc2512697bd34f2244ee732743a9",
       "be0c2f5b07cb96d61f0e1e3fd250f0a1709d7388a24e192b9734ae0f9c92abcbd095fed"
       "6c5978ad9e55bb62a6a2ca16532eb3f3e5b3bc1e64209546e7e887445"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"ca2c70bc13298c5109ee0cb342d014906e6365249005fd4beee6f01aee44edb531231e9"
       "8b50bf6810de6cf687882b09320fdd5f6375d1f2debd966fbf8d03efa",
       "564e1971233e098c26d412f2d4e652742355e616fed8ba88fc9750f869aac1c29cb9441"
       "75c374a7b6769989aa7a4216198ee12f53bf7827850dfe28540587a97",
       "73fb266a903f956a9034d52c2d2793c37fddc32077898f5d871173da1d646fb80bbc21a"
       "0522390b75d3bcc88bd78960bdb73be323ad5fc5b3a16089992957d3a"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "23d2da0c21dc0f8369c9fb0465415ddf65b49a527fa20b87b5c32aef2086840ae56f0c7"
       "496eec79c1dc3e774737d9acb6c4b27660a9034f5525eab9d3a60d784"},
      {2,
       "fe71654787b8d33c869baa366025051df549a7f1fe9ad3d79ba1ea8da0dd6e7607d9924"
       "d926c91ac903a734f4ea98dea0c20bdcdc097ad131ec29304f0d7092b"},
      {3,
       "dc12a214d33ef0dace57c7d92377dbb7117e4ffc29d4977cee08956c75055100ba0e43d"
       "167bb1dbebfdf40f34b8a08c0d8f5a27b9e7643d5334368a3338caf5f"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "23d2da0c21dc0f8369c9fb0465415ddf65b49a527fa20b87b5c32aef2086840ae56f0c7"
       "496eec79c1dc3e774737d9acb6c4b27660a9034f5525eab9d3a60d784"},
      {2,
       "fe71654787b8d33c869baa366025051df549a7f1fe9ad3d79ba1ea8da0dd6e7607d9924"
       "d926c91ac903a734f4ea98dea0c20bdcdc097ad131ec29304f0d7092b"},
      {3,
       "dc12a214d33ef0dace57c7d92377dbb7117e4ffc29d4977cee08956c75055100ba0e43d"
       "167bb1dbebfdf40f34b8a08c0d8f5a27b9e7643d5334368a3338caf5f"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707"
       "cc9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df"},
      {2,
       "7b637bc5543d96f49500aaad3b27d8bd37624db23d415c4d0f3dd231e9b9fb061f39b7d"
       "8561c540650de8bef02aca43a2069cc2512697bd34f2244ee732743a9"},
      {3,
       "be0c2f5b07cb96d61f0e1e3fd250f0a1709d7388a24e192b9734ae0f9c92abcbd095fed"
       "6c5978ad9e55bb62a6a2ca16532eb3f3e5b3bc1e64209546e7e887445"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "f5f0eaa9ca3fd0c4e0d72a3471e4b71edaabe2d01c4b25e16715004ed91e663a1750707"
       "cc9f04430f19b995f4aba21b0ec878fc5c4eb838a18df5bf9fdc949df"},
      {'B',
       "7b637bc5543d96f49500aaad3b27d8bd37624db23d415c4d0f3dd231e9b9fb061f39b7d"
       "8561c540650de8bef02aca43a2069cc2512697bd34f2244ee732743a9"},
      {'C',
       "be0c2f5b07cb96d61f0e1e3fd250f0a1709d7388a24e192b9734ae0f9c92abcbd095fed"
       "6c5978ad9e55bb62a6a2ca16532eb3f3e5b3bc1e64209546e7e887445"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "ca2c70bc13298c5109ee0cb342d014906e6365249005fd4beee6f01aee44edb531231e9"
       "8b50bf6810de6cf687882b09320fdd5f6375d1f2debd966fbf8d03efa"},
      {"two",
       "564e1971233e098c26d412f2d4e652742355e616fed8ba88fc9750f869aac1c29cb9441"
       "75c374a7b6769989aa7a4216198ee12f53bf7827850dfe28540587a97"},
      {"three",
       "73fb266a903f956a9034d52c2d2793c37fddc32077898f5d871173da1d646fb80bbc21a"
       "0522390b75d3bcc88bd78960bdb73be323ad5fc5b3a16089992957d3a"}};
};

//
// SHAKE
//

//! \brief SHAKE128 digests
//! \since docker-finance 1.0.0
struct HashData_SHAKE128 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "7df6bb7b08770b57fb4b5e7b1ce34413b0c94626a80c6ecd26ae31f23e33bd37"};
  const t_encoded kCharArrayDigest{
      "7df6bb7b08770b57fb4b5e7b1ce34413b0c94626a80c6ecd26ae31f23e33bd37"};
  const t_encoded kCharDigest{
      "a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef"};
  const t_encoded kByteDigest{
      "a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef"};

  // integral
  const t_encoded kInt16Digest{
      "5896a950157403f9cf64224a9d0703ea2561832b49ac04c892b127d4490fad37"};
  const t_encoded kInt32Digest{
      "90798254713cefc5b5b54aee805dc4bb46ae738367971d5a4c3afaa8b93554ac"};
  const t_encoded kInt64Digest{
      "259e8673619c6424504d54919e1892c6680f438ff38fc09cf9d9e80b33bdd420"};
  const t_encoded kUInt16Digest{
      "0d6fe7613492d459c4755de92c2187463b07e1edc4ca914bd42eb8c70cdd74fd"};
  const t_encoded kUInt32Digest{
      "aab82bc9c1a86f210012e25c035f0839ea04b37a2ba195a594ba2466f60b3ed7"};
  const t_encoded kUInt64Digest{
      "1e482158373cf5e025b9f1d7a25177d25574ff2501e320fac407f309be184b69"};

  // string
  const t_encoded kStringDigest{
      "7df6bb7b08770b57fb4b5e7b1ce34413b0c94626a80c6ecd26ae31f23e33bd37"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"89e0d8da0966dae684e1808433e74ae055cde444ef550470bb1a55733dc27090",
       "0e46b206e7181464f21daa04ab8c0dccb4c8130c7a546e06d4352bc9778d1036",
       "5f1a09a07fab112a705036954a452d9bb0f32e947aff52577a439ed26b7be845"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"89e0d8da0966dae684e1808433e74ae055cde444ef550470bb1a55733dc27090",
       "0e46b206e7181464f21daa04ab8c0dccb4c8130c7a546e06d4352bc9778d1036",
       "5f1a09a07fab112a705036954a452d9bb0f32e947aff52577a439ed26b7be845"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef",
       "74865dbf56f0d9c27b76300b1872ba5f852c2419010ac745bcabf4500db91fc2",
       "2e9587f4196d40eccfc86540d53c34f2282cbbdac7e17c873594d93fb849b2b3"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef",
       "74865dbf56f0d9c27b76300b1872ba5f852c2419010ac745bcabf4500db91fc2",
       "2e9587f4196d40eccfc86540d53c34f2282cbbdac7e17c873594d93fb849b2b3"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"ebaf5ccd6f37291d34bade1bbff539e76c47afb293c5d53914d492e0bdc24045",
       "4e9e3870a3187c0b898817f12c0aaeb7b664894185f7955e9b2d5e44b154ead0",
       "0a7fddc22e37eaf05b744459f6129fd1c97cb501aaf497ecb6d5d9b1cfadcbf5"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1, "89e0d8da0966dae684e1808433e74ae055cde444ef550470bb1a55733dc27090"},
      {2, "0e46b206e7181464f21daa04ab8c0dccb4c8130c7a546e06d4352bc9778d1036"},
      {3, "5f1a09a07fab112a705036954a452d9bb0f32e947aff52577a439ed26b7be845"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1, "89e0d8da0966dae684e1808433e74ae055cde444ef550470bb1a55733dc27090"},
      {2, "0e46b206e7181464f21daa04ab8c0dccb4c8130c7a546e06d4352bc9778d1036"},
      {3, "5f1a09a07fab112a705036954a452d9bb0f32e947aff52577a439ed26b7be845"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1, "a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef"},
      {2, "74865dbf56f0d9c27b76300b1872ba5f852c2419010ac745bcabf4500db91fc2"},
      {3, "2e9587f4196d40eccfc86540d53c34f2282cbbdac7e17c873594d93fb849b2b3"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A', "a5ba3aeee1525b4ae5439e54cd711f14850251e02c5999a53f61374c0ae089ef"},
      {'B', "74865dbf56f0d9c27b76300b1872ba5f852c2419010ac745bcabf4500db91fc2"},
      {'C',
       "2e9587f4196d40eccfc86540d53c34f2282cbbdac7e17c873594d93fb849b2b3"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "ebaf5ccd6f37291d34bade1bbff539e76c47afb293c5d53914d492e0bdc24045"},
      {"two",
       "4e9e3870a3187c0b898817f12c0aaeb7b664894185f7955e9b2d5e44b154ead0"},
      {"three",
       "0a7fddc22e37eaf05b744459f6129fd1c97cb501aaf497ecb6d5d9b1cfadcbf5"}};
};

//! \brief SHAKE256 digests
//! \since docker-finance 1.0.0
struct HashData_SHAKE256 : protected HashData
{
 protected:
  // trivial
  const t_encoded kStringViewDigest{
      "7d5d8c4e5f49f2ac51b44c282f653385c4f0679894275987d8794bdec8d9025be8dee0e0"
      "ba533f6f61e54d47bd6b20ea2238afcbaade17f4ce3b38f8e6a0d37b"};
  const t_encoded kCharArrayDigest{
      "7d5d8c4e5f49f2ac51b44c282f653385c4f0679894275987d8794bdec8d9025be8dee0e0"
      "ba533f6f61e54d47bd6b20ea2238afcbaade17f4ce3b38f8e6a0d37b"};
  const t_encoded kCharDigest{
      "5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e6"
      "d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8"};
  const t_encoded kByteDigest{
      "5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e6"
      "d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8"};

  // integral
  const t_encoded kInt16Digest{
      "3d4f74244cfebfea7def1a7f0df4d74c61b629da0b2f900ed3c8f49b7f269e578d29bb08"
      "656a753e035e9a0b341a451b50bca2b09af6c80c3ed68ca38e41362a"};
  const t_encoded kInt32Digest{
      "2b30b3e70cf8e15e0d03cb1b0064087b181b34fcb36a3e1ccd88cd0ad0d0b39370f60a76"
      "d5d0fb47584ede31611005eb366999219b9951e3e82a5a3a35a2f8e6"};
  const t_encoded kInt64Digest{
      "ed25d20893b7e4d271d51a1fb70afe09be98072fa8fdf86447f74e9099f6567ea16ee4b8"
      "cdcc44fb380628b151c4f7a914f9e00b865d8c56bb1ade0a560dca8a"};
  const t_encoded kUInt16Digest{
      "69c1b343d764bd91db68662a3b7477e8bd2083cf4ab455ffd8668a7982fe5cb2cb75b07f"
      "11fc5d0223340f80f12435f029ec1032879d2e5c0b8bee0e0630e8c7"};
  const t_encoded kUInt32Digest{
      "b6653d8cf7a35e88027f598b8dede5d6dfa968eb7cafb792cdb6bb831679c1cb44c95d4b"
      "e1d4ba7c7e79d6250d3a643c330d99744f24550b66c9070ff6dcc65c"};
  const t_encoded kUInt64Digest{
      "7e1da431a74c135c204d53f436194e2d7d1e7b4be8f074dffb7db8cd2ec53fcf811c5fe3"
      "4a13ff4f2cd56b2094763a8f2bc6e767cf12552b56f44977974aea81"};

  // string
  const t_encoded kStringDigest{
      "7d5d8c4e5f49f2ac51b44c282f653385c4f0679894275987d8794bdec8d9025be8dee0e0"
      "ba533f6f61e54d47bd6b20ea2238afcbaade17f4ce3b38f8e6a0d37b"};

  // vector
  const std::vector<t_encoded> kVecStringViewDigest{
      {"52640828e40a800be8d3dd88533a1447515558aeae7a8ccfbc98303b8dc90695de564e6"
       "cc78bc5346c5ae31c9d4f0c58d4984c3f274a994b79b4d60b559534c7",
       "09ff21f85ef8c00cb66fef818acde28c2001284bb4f4e23a2293a97e11363e42c11abcb"
       "c3a8c28094d85184760638b1bc3ac1782dd1804d63aadebc5dd9c5aeb",
       "29fad53249f9dbd05ebe5d5f20cca3a18760912175b57acc89e2ab3e2ea8b9d1dffe987"
       "3adfc73ad7f048111c1ccd7489046ffa490365b9e45fbfc8a3d53b459"}};
  const std::vector<t_encoded> kVecStringDigest{
      {"52640828e40a800be8d3dd88533a1447515558aeae7a8ccfbc98303b8dc90695de564e6"
       "cc78bc5346c5ae31c9d4f0c58d4984c3f274a994b79b4d60b559534c7",
       "09ff21f85ef8c00cb66fef818acde28c2001284bb4f4e23a2293a97e11363e42c11abcb"
       "c3a8c28094d85184760638b1bc3ac1782dd1804d63aadebc5dd9c5aeb",
       "29fad53249f9dbd05ebe5d5f20cca3a18760912175b57acc89e2ab3e2ea8b9d1dffe987"
       "3adfc73ad7f048111c1ccd7489046ffa490365b9e45fbfc8a3d53b459"}};
  const std::vector<t_encoded> kVecCharDigest{
      {"5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e"
       "6d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8",
       "9b4033bf5151724308b4b1fc90f1534688ea1a17c911aa3a897d5b6a05db5c25a42554d"
       "d16994405fb1b57b6bbaadd66849892aac9d814828c90e0f092428cf5",
       "6c1fcc1b777f8c560a5c6ac7a21d5d4a73e6948eae9c7c7b93bc5e208556499924e3123"
       "c0f6c8189adbdda0bd81a48e924b548e0e196fc8487df0fbb08a60f5a"}};
  const std::vector<t_encoded> kVecByteDigest{
      {"5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e"
       "6d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8",
       "9b4033bf5151724308b4b1fc90f1534688ea1a17c911aa3a897d5b6a05db5c25a42554d"
       "d16994405fb1b57b6bbaadd66849892aac9d814828c90e0f092428cf5",
       "6c1fcc1b777f8c560a5c6ac7a21d5d4a73e6948eae9c7c7b93bc5e208556499924e3123"
       "c0f6c8189adbdda0bd81a48e924b548e0e196fc8487df0fbb08a60f5a"}};
  const std::vector<t_encoded> kVecIntDigest{
      {"2f169f9b4e6a1024752209cd5410ebb84959eee0ac73c29a04c23bd524c12f810b6b656"
       "5d4c967ca59c231345d651871ca867a596ced40d41afe1660983bb649",
       "a5a4f007abc4dfe1eb19f685efde94ca76f77dff7279de620dd52074b33fa1c61e4ebee"
       "6d77c8587047a86666e373d28237488ded96b1d930ec3479335044877",
       "08946cd494a2c00b0e9321af0c225309e9d1b9d14ce8eeb4ed5182031c3f29b0708cd8a"
       "24edee1ceaf383c20bf625dcdbce94504416cead3bcdb65572207b366"}};

  // map
  const std::map<uint8_t, t_encoded> kMapStringViewDigest{
      {1,
       "52640828e40a800be8d3dd88533a1447515558aeae7a8ccfbc98303b8dc90695de564e6"
       "cc78bc5346c5ae31c9d4f0c58d4984c3f274a994b79b4d60b559534c7"},
      {2,
       "09ff21f85ef8c00cb66fef818acde28c2001284bb4f4e23a2293a97e11363e42c11abcb"
       "c3a8c28094d85184760638b1bc3ac1782dd1804d63aadebc5dd9c5aeb"},
      {3,
       "29fad53249f9dbd05ebe5d5f20cca3a18760912175b57acc89e2ab3e2ea8b9d1dffe987"
       "3adfc73ad7f048111c1ccd7489046ffa490365b9e45fbfc8a3d53b459"}};
  const std::map<uint8_t, t_encoded> kMapStringDigest{
      {1,
       "52640828e40a800be8d3dd88533a1447515558aeae7a8ccfbc98303b8dc90695de564e6"
       "cc78bc5346c5ae31c9d4f0c58d4984c3f274a994b79b4d60b559534c7"},
      {2,
       "09ff21f85ef8c00cb66fef818acde28c2001284bb4f4e23a2293a97e11363e42c11abcb"
       "c3a8c28094d85184760638b1bc3ac1782dd1804d63aadebc5dd9c5aeb"},
      {3,
       "29fad53249f9dbd05ebe5d5f20cca3a18760912175b57acc89e2ab3e2ea8b9d1dffe987"
       "3adfc73ad7f048111c1ccd7489046ffa490365b9e45fbfc8a3d53b459"}};
  const std::map<uint8_t, t_encoded> kMapCharDigest{
      {1,
       "5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e"
       "6d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8"},
      {2,
       "9b4033bf5151724308b4b1fc90f1534688ea1a17c911aa3a897d5b6a05db5c25a42554d"
       "d16994405fb1b57b6bbaadd66849892aac9d814828c90e0f092428cf5"},
      {3,
       "6c1fcc1b777f8c560a5c6ac7a21d5d4a73e6948eae9c7c7b93bc5e208556499924e3123"
       "c0f6c8189adbdda0bd81a48e924b548e0e196fc8487df0fbb08a60f5a"}};
  const std::map<char, t_encoded> kMapByteDigest{
      {'A',
       "5e6812c0bbaaee6440dcc8b81ca6809645f7512e06cf5acb57bd16dc3a2bfc57dc2bf9e"
       "6d8941950594bef5191d8394691f86edffcad6c5ebad9365f282f37a8"},
      {'B',
       "9b4033bf5151724308b4b1fc90f1534688ea1a17c911aa3a897d5b6a05db5c25a42554d"
       "d16994405fb1b57b6bbaadd66849892aac9d814828c90e0f092428cf5"},
      {'C',
       "6c1fcc1b777f8c560a5c6ac7a21d5d4a73e6948eae9c7c7b93bc5e208556499924e3123"
       "c0f6c8189adbdda0bd81a48e924b548e0e196fc8487df0fbb08a60f5a"}};
  const std::map<std::string, t_encoded> kMapIntDigest{
      {"one",
       "2f169f9b4e6a1024752209cd5410ebb84959eee0ac73c29a04c23bd524c12f810b6b656"
       "5d4c967ca59c231345d651871ca867a596ced40d41afe1660983bb649"},
      {"two",
       "a5a4f007abc4dfe1eb19f685efde94ca76f77dff7279de620dd52074b33fa1c61e4ebee"
       "6d77c8587047a86666e373d28237488ded96b1d930ec3479335044877"},
      {"three",
       "08946cd494a2c00b0e9321af0c225309e9d1b9d14ce8eeb4ed5182031c3f29b0708cd8a"
       "24edee1ceaf383c20bf625dcdbce94504416cead3bcdb65572207b366"}};
};

//
// Botan
//

//! \brief Botan hash implementation fixture
//! \since docker-finance 1.0.0
struct HashBotan_Impl
{
 protected:
  using Hash = ::dfi::crypto::botan::Hash;
  Hash hash;
};

//
// Crypto++
//

//! \brief Crypto++ hash implementation fixture
//! \since docker-finance 1.0.0
struct HashCryptoPP_Impl
{
 protected:
  using Hash = ::dfi::crypto::cryptopp::Hash;
  Hash hash;
};

//
// libsodium
//

//! \brief libsodium hash implementation fixture
//! \since docker-finance 1.0.0
struct HashLibsodium_Impl
{
 protected:
  using Hash = ::dfi::crypto::libsodium::Hash;
  Hash hash;
};
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_COMMON_HASH_HH_

// # vim: sw=2 sts=2 si ai et
