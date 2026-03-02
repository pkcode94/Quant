// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

#ifndef CONTAINER_SRC_ROOT_TEST_UNIT_UTILITY_HH_
#define CONTAINER_SRC_ROOT_TEST_UNIT_UTILITY_HH_

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/utility.hh"

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

namespace common = ::dfi::common;

//! \brief Tools utility fixture
//! \since docker-finance 1.0.0
struct Tools : public ::testing::Test, public tests::ToolsFixture
{
};

TEST_F(Tools, random_dist_TRandom1)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandom1>(min, max));
}

TEST_F(Tools, random_dist_TRandom2)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandom2>(min, max));
}

TEST_F(Tools, random_dist_TRandom3)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandom3>(min, max));
}

TEST_F(Tools, random_dist_TRandomMixMax)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomMixMax>(min, max));
}

TEST_F(Tools, random_dist_TRandomMixMax17)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomMixMax17>(min, max));
}

TEST_F(Tools, random_dist_TRandomMixMax256)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomMixMax256>(min, max));
}

TEST_F(Tools, random_dist_TRandomMT64)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomMT64>(min, max));
}

TEST_F(Tools, random_dist_TRandomRanlux48)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomRanlux48>(min, max));
}

TEST_F(Tools, random_dist_TRandomRanluxpp)
{
  ASSERT_NO_THROW(tools.dist_range<::TRandomRanluxpp>(min, max));
}

//! \brief Byte transformation fixture
//! \since docker-finance 1.0.0
struct ByteTransform : public ::testing::Test, public tests::ByteFixture
{
};

TEST_F(ByteTransform, byte)
{
  ASSERT_EQ(byte.encode(uint8_t{0x01}), std::byte{0x01});
  ASSERT_EQ(byte.decode(std::byte{0x01}), uint8_t{0x01});
}

TEST_F(ByteTransform, basic_string)
{
  using t_one = std::basic_string<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::basic_string<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, vector)
{
  using t_one = std::vector<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::vector<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, deque)
{
  using t_one = std::deque<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::deque<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, list)
{
  using t_one = std::list<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::list<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, forward_list)
{
  using t_one = std::forward_list<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::forward_list<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, map)
{
  using t_one = std::map<std::string_view, uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::map<std::string_view, std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, multimap)
{
  using t_one = std::multimap<std::string_view, uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::multimap<std::string_view, std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, multiset)
{
  using t_one = std::multiset<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::multiset<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, set)
{
  using t_one = std::set<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::set<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, unordered_map)
{
  using t_one = std::unordered_map<std::string_view, uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::unordered_map<std::string_view, std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, unordered_multimap)
{
  using t_one = std::unordered_multimap<std::string_view, uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::unordered_multimap<std::string_view, std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, unordered_multiset)
{
  using t_one = std::unordered_multiset<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::unordered_multiset<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

TEST_F(ByteTransform, unordered_set)
{
  using t_one = std::unordered_set<uint8_t>;
  t_one one{fixed_bytes<t_one>()()};

  using t_two = std::unordered_set<std::byte>;
  t_two two{fixed_bytes<t_two>()()};

  ASSERT_EQ(byte.encode(one), two);
  ASSERT_EQ(byte.decode(two), one);
}

//! \brief Common fixture (free functions)
//! \note Not a 'common' fixture but rather a fixture for 'common'
//! \since docker-finance 1.1.0
struct CommonFree : public ::testing::Test, public ::dfi::tests::CommonFixture
{
};

TEST_F(CommonFree, Line)
{
  ASSERT_NO_THROW(common::line(""));
  ASSERT_THROW(common::line("a bar without a foo"), common::type::RuntimeError);
  ASSERT_NO_THROW(common::line("#define FOO 1"));
}

TEST_F(CommonFree, add_include_dir)
{
  ASSERT_THROW(
      common::add_include_dir("/should-not-exist"), common::type::RuntimeError);
  ASSERT_NO_THROW(common::add_include_dir("/usr/include"));
}

TEST_F(CommonFree, add_linked_lib)
{
  ASSERT_THROW(
      common::add_linked_lib("/should-not-exist.so"),
      common::type::RuntimeError);
  ASSERT_NO_THROW(common::add_linked_lib("/usr/lib/LLVMgold.so"));
}

TEST_F(CommonFree, remove_linked_lib)
{
  ASSERT_THROW(
      common::remove_linked_lib("/should-not-exist.so"),
      common::type::RuntimeError);
  ASSERT_NO_THROW(common::add_linked_lib("/usr/lib/LLVMgold.so"));
  ASSERT_NO_THROW(common::remove_linked_lib("/usr/lib/LLVMgold.so"));
}

TEST_F(CommonFree, get_env)
{
  ASSERT_THROW(common::get_env("should-not-exist"), common::type::RuntimeError);
  ASSERT_NO_THROW(common::get_env("DOCKER_FINANCE_VERSION"));
}

TEST_F(CommonFree, get_root_path)
{
  ASSERT_NO_THROW(common::get_root_path());
  ASSERT_EQ(
      common::get_root_path(),
      common::get_env("DOCKER_FINANCE_CONTAINER_REPO") + "/src/root/");
}

TEST_F(CommonFree, exec)
{
  ASSERT_NE(common::exec("should-not-exist"), 0);
  ASSERT_EQ(common::exec("pwd"), 0);
}

TEST_F(CommonFree, exit)
{
  // NOTE: no-op (or else entire framework exits).
  // Execution is covered in functional tests (gitea workflow).
}

TEST_F(CommonFree, make_timestamp)
{
  ASSERT_EQ(common::make_timestamp().size(), 20);
}

//! \brief Common fixture (raw files)
//! \note Not a 'common' fixture but rather a fixture for 'common'
//! \since docker-finance 1.1.0
struct CommonFreeFiles : public ::testing::Test,
                         public ::dfi::tests::CommonFixture
{
  std::string path_1, path_2;

  void SetUp() override
  {
    try
      {
        path_1 = std::tmpnam(nullptr);
        path_2 = std::tmpnam(nullptr);
      }
    catch (...)
      {
        common::throw_ex<common::type::RuntimeError>("could not generate path");
      }

    std::ofstream file_1(path_1), file_2(path_2);
    common::throw_ex_if<common::type::RuntimeError>(
        !file_1.is_open() || !file_2.is_open(), "could not create file");

    file_1 << "using my_foo = int;\n";
    file_1.close();
    file_2 << "using my_bar = char;\n";
    file_2.close();

    common::throw_ex_if<common::type::RuntimeError>(
        file_1.bad() || file_2.bad(), "could not write to file");
  }

  void TearDown() override
  {
    common::throw_ex_if<common::type::RuntimeError>(
        std::remove(path_1.c_str()), "could not remove file '" + path_1 + "'");

    common::throw_ex_if<common::type::RuntimeError>(
        std::remove(path_2.c_str()), "could not remove file '" + path_2 + "'");
  }
};

TEST_F(CommonFreeFiles, LoadSingle)
{
  ASSERT_THROW(
      common::load({path_1 + "should-not-exist"}), common::type::RuntimeError);
  ASSERT_THROW(common::line("my_foo foo;"), common::type::RuntimeError);

  ASSERT_NO_THROW(common::load(path_1));
  ASSERT_NO_THROW(common::line("my_foo foo;"));
}

TEST_F(CommonFreeFiles, LoadMultiple)
{
  ASSERT_THROW(
      common::load(
          {{path_1 + "should-not-exist"}, {path_2 + "nor-should-this"}}),
      common::type::RuntimeError);
  ASSERT_THROW(common::line("my_bar bar;"), common::type::RuntimeError);

  ASSERT_NO_THROW(common::load({path_1, path_2}));
  ASSERT_NO_THROW(common::line("my_bar bar;"));
}

TEST_F(CommonFreeFiles, UnloadSingle)
{
  ASSERT_NO_THROW(common::load(path_1));
  ASSERT_NO_THROW(common::line("my_foo foo;"));

  ASSERT_NO_THROW(common::unload(path_1));
  ASSERT_THROW(common::line("my_foo foo;"), common::type::RuntimeError);
}

TEST_F(CommonFreeFiles, UnloadMultiple)
{
  ASSERT_NO_THROW(common::load({path_1, path_2}));
  ASSERT_NO_THROW(common::line("my_foo foo;"));
  ASSERT_NO_THROW(common::line("my_bar bar;"));

  ASSERT_NO_THROW(common::unload({path_1, path_2}));
  ASSERT_THROW(common::line("my_foo foo;"), common::type::RuntimeError);
  ASSERT_THROW(common::line("my_bar bar;"), common::type::RuntimeError);
}

TEST_F(CommonFreeFiles, ReloadSingle)
{
  ASSERT_NO_THROW(common::load(path_1));
  ASSERT_NO_THROW(common::line("my_foo foo;"));

  ASSERT_NO_THROW(common::reload(path_1));
  ASSERT_NO_THROW(common::line("my_foo foo;"));
}

TEST_F(CommonFreeFiles, ReloadMultiple)
{
  ASSERT_NO_THROW(common::load({path_1, path_2}));
  ASSERT_NO_THROW(common::line("my_foo foo;"));
  ASSERT_NO_THROW(common::line("my_bar bar;"));

  ASSERT_NO_THROW(common::reload({path_1, path_2}));
  ASSERT_NO_THROW(common::line("my_foo foo;"));
  ASSERT_NO_THROW(common::line("my_bar bar;"));
}

//! \brief PluggablePath fixture
//! \ref dfi::common::PluggablePath
//! \since docker-finance 1.1.0
struct PluggablePath : public ::testing::Test,
                       public ::dfi::tests::CommonFixture
{
  PluggablePath() : m_repo(kRepoPseudo), m_custom(kCustomPseudo) {}

  const std::string kParent{"a_pluggable_dir"};
  const std::string kChild{"a_pluggable.file"};
  const std::string kFamily{kParent + "/" + kChild};

  const std::string kRepoPseudo{"repo" + std::string{"/"} + kFamily};
  const std::string kCustomPseudo{"custom" + std::string{"/"} + kFamily};

  static constexpr std::string_view kRepoBase{"/the/repo/path/"};
  static constexpr std::string_view kCustomBase{"/the/custom/path/"};

  const std::string kRepoAbsolute{std::string{kRepoBase} + kFamily};
  const std::string kCustomAbsolute{std::string{kCustomBase} + kFamily};

  struct Path : public ::dfi::common::PluggablePath
  {
    using outer = ::dfi::tests::unit::PluggablePath;
    explicit Path(const std::string& path)
        : ::dfi::common::PluggablePath(
              path,
              ::dfi::common::type::PluggablePath{
                  {std::string{outer::kRepoBase},
                   std::string{outer::kCustomBase}}})
    {
    }

    auto& operator()()
    {
      return this->::dfi::common::PluggablePath::operator()();
    }

    const auto& operator()() const
    {
      return this->::dfi::common::PluggablePath::operator()();
    }
  };

  Path m_repo, m_custom;
};

TEST_F(PluggablePath, Pseudo)
{
  ASSERT_EQ(m_repo.pseudo(), kRepoPseudo);
  ASSERT_EQ(m_custom.pseudo(), kCustomPseudo);
}

TEST_F(PluggablePath, Absolute)
{
  ASSERT_EQ(m_repo.absolute(), kRepoAbsolute);
  ASSERT_EQ(m_custom.absolute(), kCustomAbsolute);
}

TEST_F(PluggablePath, Parent)
{
  ASSERT_EQ(m_repo.parent(), kParent);
  ASSERT_EQ(m_custom.parent(), kParent);
}

TEST_F(PluggablePath, Child)
{
  ASSERT_EQ(m_repo.child(), kChild);
  ASSERT_EQ(m_custom.child(), kChild);
}

TEST_F(PluggablePath, Family)
{
  ASSERT_EQ(m_repo.family(), kFamily);
  ASSERT_EQ(m_custom.family(), kFamily);
}

TEST_F(PluggablePath, Family_nested)
{
  const std::string nested{kParent + "/a_nested_dir/" + kChild};
  Path repo("repo" + std::string{"/"} + nested),
      custom("custom" + std::string{"/"} + nested);

  ASSERT_EQ(repo.parent(), kParent + "/a_nested_dir");
  ASSERT_EQ(custom.parent(), kParent + "/a_nested_dir");

  ASSERT_EQ(repo.child(), kChild);
  ASSERT_EQ(custom.child(), kChild);

  ASSERT_EQ(repo.family(), nested);
  ASSERT_EQ(custom.family(), nested);
}

TEST_F(PluggablePath, InvalidFamily)
{
  ASSERT_THROW(Path path(""), common::type::RuntimeError);
  ASSERT_THROW(Path path("nope"), common::type::RuntimeError);

  ASSERT_THROW(Path path("repo/"), common::type::RuntimeError);
  ASSERT_THROW(Path path("custom/"), common::type::RuntimeError);

  ASSERT_THROW(Path path("repo/incomplete"), common::type::RuntimeError);
  ASSERT_THROW(Path path("custom/incomplete"), common::type::RuntimeError);

  ASSERT_THROW(Path path("repo/incomplete/"), common::type::RuntimeError);
  ASSERT_THROW(Path path("custom/incomplete/"), common::type::RuntimeError);

  ASSERT_THROW(
      Path path("repo/incomplete/incomplete"), common::type::RuntimeError);
  ASSERT_THROW(
      Path path("custom/incomplete/incomplete"), common::type::RuntimeError);
}

TEST_F(PluggablePath, InvalidCharacters)
{
  ASSERT_THROW(Path path("repo/no spaces/a.file"), common::type::RuntimeError);
  ASSERT_THROW(
      Path path("custom/no spaces/a.file"), common::type::RuntimeError);

  ASSERT_THROW(Path path("repo/'no_ticks'/a.file"), common::type::RuntimeError);
  ASSERT_THROW(
      Path path("custom/'no_ticks'/a.file"), common::type::RuntimeError);

  ASSERT_THROW(Path path("repo/any/@a.file"), common::type::RuntimeError);
  ASSERT_THROW(Path path("custom/any/@a.file"), common::type::RuntimeError);

  ASSERT_THROW(
      Path path("repo/bad\\ slash/a.file"), common::type::RuntimeError);
  ASSERT_THROW(
      Path path("custom/bad\\ slash/a.file"), common::type::RuntimeError);
}

TEST_F(PluggablePath, Base)
{
  ASSERT_EQ(m_repo().repo(), kRepoBase);
  ASSERT_EQ(m_custom().custom(), kCustomBase);
}

TEST_F(PluggablePath, Booleans)
{
  ASSERT_EQ(m_repo.is_repo(), true);
  ASSERT_EQ(m_repo.is_custom(), false);

  ASSERT_EQ(m_custom.is_custom(), true);
  ASSERT_EQ(m_custom.is_repo(), false);
}

TEST_F(PluggablePath, Mutators)
{
  const std::string kOne{std::string{kRepoBase} + std::string{"one/"}};
  const std::string kTwo{std::string{kCustomBase} + std::string{"two/"}};

  //
  // Mutated
  //

  ASSERT_NO_THROW(m_repo().repo(kOne).custom(""));
  ASSERT_NO_THROW(m_custom().repo("").custom(kTwo));

  ASSERT_EQ(m_repo().repo(), kOne);
  ASSERT_EQ(m_repo().custom(), "");

  ASSERT_EQ(m_custom().custom(), kTwo);
  ASSERT_EQ(m_custom().repo(), "");

  const std::string kOneAbsolute{kOne + kFamily};
  const std::string kTwoAbsolute{kTwo + kFamily};

  ASSERT_EQ(m_repo.parse().absolute(), kOneAbsolute);
  ASSERT_EQ(m_custom.parse().absolute(), kTwoAbsolute);

  //
  // Unchanged
  //

  ASSERT_EQ(m_repo.parse().pseudo(), kRepoPseudo);
  ASSERT_EQ(m_repo.pseudo(), kRepoPseudo);
  ASSERT_EQ(m_custom.parse().pseudo(), kCustomPseudo);
  ASSERT_EQ(m_custom.pseudo(), kCustomPseudo);

  ASSERT_EQ(m_repo.parse().parent(), kParent);
  ASSERT_EQ(m_repo.parent(), kParent);
  ASSERT_EQ(m_custom.parse().parent(), kParent);
  ASSERT_EQ(m_custom.parent(), kParent);

  ASSERT_EQ(m_repo.parse().child(), kChild);
  ASSERT_EQ(m_repo.child(), kChild);
  ASSERT_EQ(m_custom.parse().child(), kChild);
  ASSERT_EQ(m_custom.child(), kChild);

  ASSERT_EQ(m_repo.parse().family(), kFamily);
  ASSERT_EQ(m_repo.family(), kFamily);
  ASSERT_EQ(m_custom.parse().family(), kFamily);
  ASSERT_EQ(m_custom.family(), kFamily);

  ASSERT_EQ(m_repo.parse().is_repo(), true);
  ASSERT_EQ(m_repo.is_repo(), true);
  ASSERT_EQ(m_repo.parse().is_custom(), false);
  ASSERT_EQ(m_repo.is_custom(), false);

  ASSERT_EQ(m_custom.parse().is_custom(), true);
  ASSERT_EQ(m_custom.is_custom(), true);
  ASSERT_EQ(m_custom.parse().is_repo(), false);
  ASSERT_EQ(m_custom.is_repo(), false);
}

//! \brief PluggableSpace fixture
//! \ref dfi::common::PluggableSpace
//! \since docker-finance 1.1.0
struct PluggableSpace : public ::testing::Test,
                        public ::dfi::tests::CommonFixture
{
  PluggableSpace() : m_space(kOuter, kInner, kEntry) {}

  static constexpr std::string_view kOuter{"outer::space"};
  static constexpr std::string_view kInner{"inner::space"};
  static constexpr std::string_view kEntry{"class_C"};

  struct Space : public ::dfi::common::PluggableSpace
  {
    explicit Space(
        const std::string_view outer,
        const std::string_view inner,
        const std::string_view entry)
        : ::dfi::common::PluggableSpace(
              ::dfi::common::type::PluggableSpace{
                  {std::string{outer}, std::string{inner}, std::string{entry}}})
    {
    }

    auto& operator()()
    {
      return this->::dfi::common::PluggableSpace::operator()();
    }

    const auto& operator()() const
    {
      return this->::dfi::common::PluggableSpace::operator()();
    }
  };

  Space m_space;
};

TEST_F(PluggableSpace, Accessors)
{
  ASSERT_EQ(m_space.outer(), kOuter);
  ASSERT_EQ(m_space.inner(), kInner);
  ASSERT_EQ(m_space.entry(), kEntry);

  ASSERT_EQ(m_space().outer(), kOuter);
  ASSERT_EQ(m_space().inner(), kInner);
  ASSERT_EQ(m_space().entry(), kEntry);
}

TEST_F(PluggableSpace, Mutators)
{
  const std::string kOne{std::string{kOuter} + std::string{"::one"}};
  const std::string kTwo{std::string{kInner} + std::string{"::two"}};
  const std::string kThree{std::string{kInner} + std::string{"_"}};

  ASSERT_NO_THROW(m_space().outer(kOne).inner(kTwo).entry(kThree));

  ASSERT_EQ(m_space().outer(), kOne);
  ASSERT_EQ(m_space().inner(), kTwo);
  ASSERT_EQ(m_space().entry(), kThree);

  ASSERT_EQ(m_space.outer(), kOne);
  ASSERT_EQ(m_space.inner(), kTwo);
  ASSERT_EQ(m_space.entry(), kThree);

  const std::string kFour{std::string{kOuter} + std::string{"::four"}};
  const std::string kFive{std::string{kInner} + std::string{"::five"}};
  const std::string kSix{std::string{kInner} + std::string{"__"}};

  ASSERT_NO_THROW(m_space().outer(kFour).inner(kFive).entry(kSix));

  ASSERT_EQ(m_space().outer(), kFour);
  ASSERT_EQ(m_space().inner(), kFive);
  ASSERT_EQ(m_space().entry(), kSix);

  ASSERT_EQ(m_space.outer(), kFour);
  ASSERT_EQ(m_space.inner(), kFive);
  ASSERT_EQ(m_space.entry(), kSix);
}

TEST_F(PluggableSpace, Conversions)
{
  ASSERT_NO_THROW(Space space("", "", ""));
  ASSERT_NO_THROW(Space space("hi/hey", "there/now", "dot.ext"));
  ASSERT_THROW(
      Space space("hi hey", "there now", "dot ext"),
      common::type::RuntimeError);
  ASSERT_THROW(
      Space space("hi@hey", "there@now", "dot@ext"),
      common::type::RuntimeError);

  Space space("hi/hey", "there/now", "dot.ext");
  ASSERT_NO_THROW(space().outer("hi hey").inner("there now").entry("dot ext"));
  ASSERT_THROW(space.parse(), common::type::RuntimeError);
  ASSERT_NO_THROW(space().outer("hi@hey").inner("there@now").entry("dot ext"));
  ASSERT_THROW(space.parse(), common::type::RuntimeError);

  space = Space{"hi/hey", "there/now", "dot.ext"};
  ASSERT_EQ(space.outer(), "hi::hey");
  ASSERT_EQ(space.inner(), "there::now");
  ASSERT_EQ(space.entry(), "dot_ext");

  ASSERT_NO_THROW(space().outer("hi-hey").inner("there-now").entry("dot-ext"));
  ASSERT_NO_THROW(space.parse());
  ASSERT_EQ(space.outer(), "hi_hey");
  ASSERT_EQ(space.inner(), "there_now");
  ASSERT_EQ(space.entry(), "dot_ext");
}

TEST_F(PluggableSpace, Booleans)
{
  ASSERT_EQ(m_space.has_outer(), true);
  ASSERT_EQ(m_space.has_inner(), true);
  ASSERT_EQ(m_space.has_entry(), true);

  ASSERT_NO_THROW(m_space().outer("").inner("").entry(""));

  ASSERT_EQ(m_space.has_outer(), false);
  ASSERT_EQ(m_space.has_inner(), false);
  ASSERT_EQ(m_space.has_entry(), false);
}

//! \brief PluggableArgs fixture
//! \ref dfi::common::PluggableArgs
//! \since docker-finance 1.1.0
struct PluggableArgs : public ::testing::Test,
                       public ::dfi::tests::CommonFixture
{
  PluggableArgs() : m_args(kLoad, kUnload) {}

  static constexpr std::string_view kLoad{"using foo = int;"};
  static constexpr std::string_view kUnload{"using bar = char;"};

  struct Args : public ::dfi::common::PluggableArgs
  {
    using args = ::dfi::tests::unit::PluggableArgs;
    explicit Args(const std::string_view load, const std::string_view unload)
        : ::dfi::common::PluggableArgs(
              ::dfi::common::type::PluggableArgs{
                  {std::string{args::kLoad}, std::string{args::kUnload}}})
    {
    }

    auto& operator()()
    {
      return this->::dfi::common::PluggableArgs::operator()();
    }

    const auto& operator()() const
    {
      return this->::dfi::common::PluggableArgs::operator()();
    }
  };

  Args m_args;
};

TEST_F(PluggableArgs, Accessors)
{
  ASSERT_EQ(m_args.load(), kLoad);
  ASSERT_EQ(m_args.unload(), kUnload);

  ASSERT_EQ(m_args().load(), kLoad);
  ASSERT_EQ(m_args().unload(), kUnload);
}

TEST_F(PluggableArgs, Mutators)
{
  const std::string kOne{std::string{kLoad} + std::string{" foo f1;"}};
  const std::string kTwo{std::string{kUnload} + std::string{" bar b1;"}};

  ASSERT_NO_THROW(m_args().load(kOne).unload(kTwo));

  ASSERT_EQ(m_args().load(), kOne);
  ASSERT_EQ(m_args().unload(), kTwo);

  ASSERT_EQ(m_args.load(), kOne);
  ASSERT_EQ(m_args.unload(), kTwo);

  const std::string kThree{std::string{kLoad} + std::string{" foo f2;"}};
  const std::string kFour{std::string{kUnload} + std::string{" bar b2;"}};

  ASSERT_NO_THROW(m_args().load(kThree).unload(kFour));

  ASSERT_EQ(m_args().load(), kThree);
  ASSERT_EQ(m_args().unload(), kFour);

  ASSERT_EQ(m_args.load(), kThree);
  ASSERT_EQ(m_args.unload(), kFour);
}

TEST_F(PluggableArgs, Booleans)
{
  ASSERT_EQ(m_args.has_load(), true);
  ASSERT_EQ(m_args.has_unload(), true);

  ASSERT_NO_THROW(m_args().load("").unload(""));

  ASSERT_EQ(m_args.has_load(), false);
  ASSERT_EQ(m_args.has_unload(), false);
}

//! \brief MacroFreeFiles fixture
//! \since docker-finance 1.1.0
//! \todo Move to Macro tests
struct MacroFreeFiles : public ::testing::Test,
                        public ::dfi::tests::CommonFixture
{
};

TEST_F(MacroFreeFiles, LoadUnloadSingleSimple)
{
  ASSERT_THROW(
      ::dfi::macro::load("repo/should-not/exist.C"),
      common::type::RuntimeError);
  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::botan::Hash h;"),
      common::type::RuntimeError);

  ASSERT_NO_THROW(::dfi::macro::load("repo/crypto/hash.C"));
  ASSERT_NO_THROW(common::line("dfi::macro::common::crypto::botan::Hash h;"));

  ASSERT_NO_THROW(::dfi::macro::unload("repo/crypto/hash.C"));
  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::botan::Hash h;"),
      common::type::RuntimeError);
}

TEST_F(MacroFreeFiles, LoadUnloadMultipleSimple)
{
  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::botan::Hash h;"),
      common::type::RuntimeError);

  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::libsodium::Random r;"),
      common::type::RuntimeError);

  ASSERT_NO_THROW(
      ::dfi::macro::load({"repo/crypto/hash.C", "repo/crypto/random.C"}));
  ASSERT_NO_THROW(common::line("dfi::macro::common::crypto::botan::Hash h;"));
  ASSERT_NO_THROW(
      common::line("dfi::macro::common::crypto::libsodium::Random r;"));

  ASSERT_NO_THROW(
      ::dfi::macro::unload({"repo/crypto/random.C", "repo/crypto/hash.C"}));
  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::libsodium::Random r;"),
      common::type::RuntimeError);
  ASSERT_THROW(
      common::line("dfi::macro::common::crypto::botan::Hash h;"),
      common::type::RuntimeError);
}
// TODO(afiore): load/unload w/ Pluggable types
// TODO(afiore): reload

//! \brief Plugin command fixture for testing auto-(un|re)loading functionality
//! \details Will test loading/unloading/reloading and argument passing to plugin
//! \ref dfi::plugin
//! \note
//!   `dfi`'s example repository plugin will process:
//!
//!     - load argument *after* plugin is loaded
//!     - unload argument *before* plugin is unloaded
//!
//! \warning Since the custom plugin bind-mount is read-only,
//!   only-the-fly custom plugins cannot be created at this time.
//!
//! \since docker-finance 1.1.0
//! \todo Move to Plugin tests
struct PluginCommands : public ::testing::Test,
                        public ::dfi::tests::CommonFixture
{
  const std::string kValidFile{"repo/example/example.cc"};
  const std::string kInvalidFile{"repo/should-not/exist.cc"};

  const std::string kValidArg{"dfi::plugin::example::MyExamples my;"};

  //! \note File is processed but plugin-implementation argument will intentionally throw
  const std::string kInvalidArg{
      "dfi::common::throw_ex<dfi::common::type::InvalidArgument>();"};
};

TEST_F(PluginCommands, LoadUnloadSingleInvalidFile)
{
  ASSERT_THROW(plugin::load(kInvalidFile), common::type::RuntimeError);
  ASSERT_THROW(common::line(kValidArg), common::type::RuntimeError);
  ASSERT_THROW(plugin::unload(kInvalidFile), common::type::RuntimeError);
}

TEST_F(PluginCommands, LoadUnloadSingleSimpleNoArg)
{
  ASSERT_NO_THROW(plugin::load(kValidFile));
  ASSERT_NO_THROW(common::line(kValidArg));
  ASSERT_NO_THROW(plugin::unload(kValidFile));
  ASSERT_THROW(common::line(kValidArg), common::type::RuntimeError);
}

TEST_F(PluginCommands, LoadUnloadSingleSimpleWithArg)
{
  ASSERT_NO_THROW(plugin::load(kValidFile, kValidArg + " my.example1();"));
  ASSERT_NO_THROW(plugin::unload(kValidFile));

  ASSERT_THROW(
      plugin::load(kValidFile, kInvalidArg), common::type::InvalidArgument);
  ASSERT_NO_THROW(common::line(kValidArg));  // File is still loaded

  ASSERT_NO_THROW(plugin::unload(kValidFile, kValidArg + " my.example1();"));
  ASSERT_THROW(
      common::line(kValidArg + " my.example1();"), common::type::RuntimeError);
}

TEST_F(PluginCommands, LoadUnloadSinglePluggable)
{
  ASSERT_NO_THROW(
      plugin::load(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " my.example1();",
              kValidArg + " /* unload code */"}));

  ASSERT_NO_THROW(
      plugin::unload(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " /* load code */", kValidArg + " my.example2();"}));

  ASSERT_THROW(
      plugin::load(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{kInvalidArg, kInvalidArg}),
      common::type::InvalidArgument);

  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(
      plugin::unload(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " /* load code */", kValidArg + " my.example2();"}));

  ASSERT_THROW(
      common::line(kValidArg + " my.example1();"), common::type::RuntimeError);
  ASSERT_THROW(
      common::line(kValidArg + " my.example2();"), common::type::RuntimeError);
}

// TODO(afiore): multiple load

TEST_F(PluginCommands, ReloadSingleInvalidFile)
{
  ASSERT_THROW(plugin::load(kInvalidFile), common::type::RuntimeError);
  ASSERT_THROW(common::line(kValidArg), common::type::RuntimeError);
  ASSERT_THROW(plugin::reload(kInvalidFile), common::type::RuntimeError);
  ASSERT_THROW(plugin::unload(kInvalidFile), common::type::RuntimeError);
}

TEST_F(PluginCommands, ReloadSingleSimpleNoArg)
{
  ASSERT_NO_THROW(plugin::load(kValidFile));
  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(plugin::reload(kValidFile));
  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(plugin::unload(kValidFile));
}

TEST_F(PluginCommands, ReloadSingleSimpleWithArg)
{
  ASSERT_NO_THROW(plugin::load(kValidFile, kValidArg + " my.example1();"));

  ASSERT_NO_THROW(
      plugin::reload(
          kValidFile,
          {kValidArg + " /* load code */", kValidArg + " /* unload code */"}));

  ASSERT_THROW(
      plugin::reload(kValidFile, {" /* valid load code */", kInvalidArg}),
      common::type::InvalidArgument);

  ASSERT_THROW(
      plugin::reload(kValidFile, {kInvalidArg, " /* valid unload code */"}),
      common::type::InvalidArgument);

  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(
      plugin::reload(
          kValidFile,
          {kValidArg + " /* load code */", kValidArg + " /* unload code */"}));

  ASSERT_NO_THROW(plugin::unload(kValidFile, kValidArg + " my.example1();"));
  ASSERT_THROW(
      common::line(kValidArg + " my.example1();"), common::type::RuntimeError);
}

TEST_F(PluginCommands, ReloadSinglePluggable)
{
  ASSERT_NO_THROW(
      plugin::load(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " my.example1();",
              kValidArg + " /* unload code */"}));

  ASSERT_NO_THROW(
      plugin::reload(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " my.example1();", kValidArg + " my.example2();"}));

  ASSERT_THROW(
      plugin::load(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{kInvalidArg, kInvalidArg}),
      common::type::InvalidArgument);

  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(
      plugin::reload(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " my.example1();", kValidArg + " my.example2();"}));

  ASSERT_NO_THROW(common::line(kValidArg));

  ASSERT_NO_THROW(
      plugin::unload(
          plugin::common::PluginPath{kValidFile},
          plugin::common::PluginArgs{
              kValidArg + " /* load code */", kValidArg + " my.example3();"}));

  ASSERT_THROW(common::line(kValidArg), common::type::RuntimeError);
  ASSERT_THROW(
      common::line(kValidArg + " my.example1();"), common::type::RuntimeError);
  ASSERT_THROW(
      common::line(kValidArg + " my.example2();"), common::type::RuntimeError);
  ASSERT_THROW(
      common::line(kValidArg + " my.example3();"), common::type::RuntimeError);
}

TEST_F(PluginCommands, LoaderNPI)
{
  using t_path = plugin::common::PluginPath;
  using t_space = plugin::common::PluginSpace;
  using t_args = plugin::common::PluginArgs;

  ::dfi::common::type::Pluggable<t_path, t_space, t_args> type{
      t_path{kValidFile},
      t_space{"example" /* inner */, "example_cc" /* class */},
      t_args{kValidArg + " my.example1();", kValidArg + " my.example2();"}};

  plugin::common::Plugin plugin{type};
  ASSERT_NO_THROW(plugin.load().reload().unload());
}
}  // namespace unit
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_UNIT_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
