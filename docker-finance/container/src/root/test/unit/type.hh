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

#ifndef CONTAINER_SRC_ROOT_TEST_UNIT_TYPE_HH_
#define CONTAINER_SRC_ROOT_TEST_UNIT_TYPE_HH_

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "../common/type.hh"

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

//
// RuntimeError
//

//! \brief Exception fixture (RuntimeError)
//! \since docker-finance 1.0.0
struct RuntimeError : public Exception
{
};

TEST_F(RuntimeError, throw_ex_if)  // cppcheck-suppress syntaxError
{
  try
    {
      common::throw_ex_if<common::type::RuntimeError>(false, what);
    }
  catch (const common::type::Exception& ex)
    {
      ASSERT_EQ(ex.type(), t_type::RuntimeError);
      ASSERT_EQ(ex.what(), what);
    }
}

TEST_F(RuntimeError, throw_ex)
{
  try
    {
      common::throw_ex<common::type::RuntimeError>(what);
    }
  catch (const common::type::Exception& ex)
    {
      ASSERT_EQ(ex.type(), t_type::RuntimeError);
      // NOTE: no case for ex.what() because message is formatted within function
    }
}

TEST_F(RuntimeError, rethrow)
{
  ASSERT_THROW(
      {
        try
          {
            throw common::type::RuntimeError(what);
          }
        catch (const common::type::Exception& ex)
          {
            ASSERT_EQ(ex.type(), t_type::RuntimeError);
            ASSERT_EQ(ex.what(), what);
            throw;
          }
      },
      common::type::Exception);
}

TEST_F(RuntimeError, copy_assignment)
{
  common::type::RuntimeError one(what);
  common::type::RuntimeError two;

  two = one;

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STREQ(one.what(), two.what());
}

TEST_F(RuntimeError, copy_ctor)
{
  common::type::RuntimeError one(what);
  common::type::RuntimeError two(one);

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STREQ(one.what(), two.what());
}

TEST_F(RuntimeError, move_assignment)
{
  common::type::RuntimeError one(what);
  common::type::RuntimeError two;

  two = std::move(one);

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STRNE(one.what(), two.what());
}

TEST_F(RuntimeError, move_ctor)
{
  common::type::RuntimeError one(what);
  common::type::RuntimeError two(std::move(one));

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STRNE(one.what(), two.what());
}

//
// InvalidArgument
//

//! \brief Exception fixture (InvalidArgument)
//! \since docker-finance 1.0.0
struct InvalidArgument : public Exception
{
};

TEST_F(InvalidArgument, throw_ex_if)
{
  try
    {
      common::throw_ex_if<common::type::InvalidArgument>(false, what);
    }
  catch (const common::type::Exception& ex)
    {
      ASSERT_EQ(ex.type(), t_type::InvalidArgument);
      ASSERT_EQ(ex.what(), what);
    }
}

TEST_F(InvalidArgument, throw_ex)
{
  try
    {
      common::throw_ex<common::type::InvalidArgument>(what);
    }
  catch (const common::type::Exception& ex)
    {
      ASSERT_EQ(ex.type(), t_type::InvalidArgument);
      // NOTE: no case for ex.what() because message is formatted within function
    }
}

TEST_F(InvalidArgument, rethrow)
{
  ASSERT_THROW(
      {
        try
          {
            throw common::type::InvalidArgument(what);
          }
        catch (const common::type::Exception& ex)
          {
            ASSERT_EQ(ex.type(), t_type::InvalidArgument);
            ASSERT_EQ(ex.what(), what);
            throw;
          }
      },
      common::type::Exception);
}

TEST_F(InvalidArgument, copy_assignment)
{
  common::type::InvalidArgument one(what);
  common::type::InvalidArgument two;

  two = one;

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STREQ(one.what(), two.what());
}

TEST_F(InvalidArgument, copy_ctor)
{
  common::type::InvalidArgument one(what);
  common::type::InvalidArgument two(one);

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STREQ(one.what(), two.what());
}

TEST_F(InvalidArgument, move_assignment)
{
  common::type::InvalidArgument one(what);
  common::type::InvalidArgument two;

  two = std::move(one);

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STRNE(one.what(), two.what());
}

TEST_F(InvalidArgument, move_ctor)
{
  common::type::InvalidArgument one(what);
  common::type::InvalidArgument two(std::move(one));

  ASSERT_EQ(one.type(), two.type());
  ASSERT_STRNE(one.what(), two.what());
}

//! \brief PluggablePath type fixture
//! \ref dfi::common::type::PluggablePath
//! \since docker-finance 1.1.0
struct PluggablePathType : public ::testing::Test,
                           public ::dfi::common::type::PluggablePath
{
  PluggablePathType()
      : ::dfi::common::type::PluggablePath(
            {"/path/to/repository/pluggable", "/path/to/custom/pluggable"})
  {
  }

  const std::string kRepo{"/path/to/repository/pluggable"};
  const std::string kCustom{"/path/to/custom/pluggable"};
};

TEST_F(PluggablePathType, Accessors)
{
  ASSERT_EQ(repo(), kRepo);
  ASSERT_EQ(custom(), kCustom);
}

TEST_F(PluggablePathType, Mutators)
{
  ASSERT_NO_THROW(repo(kRepo + "/new").custom(kCustom + "/new"));

  ASSERT_EQ(repo(), kRepo + "/new");
  ASSERT_EQ(custom(), kCustom + "/new");
}

//! \brief PluggableSpace type fixture
//! \ref dfi::common::type::PluggableSpace
//! \since docker-finance 1.1.0
struct PluggableSpaceType : public ::testing::Test,
                            public ::dfi::common::type::PluggableSpace
{
  PluggableSpaceType()
      : ::dfi::common::type::PluggableSpace(
            {"outer::space", "inner::space", "class_C"})
  {
  }

  const std::string kOuter{"outer::space"};
  const std::string kInner{"inner::space"};
  const std::string kEntry{"class_C"};
};

TEST_F(PluggableSpaceType, Accessors)
{
  ASSERT_EQ(outer(), kOuter);
  ASSERT_EQ(inner(), kInner);
  ASSERT_EQ(entry(), kEntry);
}

TEST_F(PluggableSpaceType, Mutators)
{
  ASSERT_NO_THROW(outer(kOuter + "::more_outer")
                      .inner(kInner + "::more_inner")
                      .entry(kEntry + "_"));

  ASSERT_EQ(outer(), kOuter + "::more_outer");
  ASSERT_EQ(inner(), kInner + "::more_inner");
  ASSERT_EQ(entry(), kEntry + "_");
}

//! \brief PluggableArgs type fixture
//! \ref dfi::common::type::PluggableArgs
//! \since docker-finance 1.1.0
struct PluggableArgsType : public ::testing::Test,
                           public ::dfi::common::type::PluggableArgs
{
  PluggableArgsType()
      : ::dfi::common::type::PluggableArgs(
            {"loader argument;", "unloader argument;"})
  {
  }

  const std::string kLoad{"loader argument;"};
  const std::string kUnload{"unloader argument;"};
};

TEST_F(PluggableArgsType, Accessors)
{
  ASSERT_EQ(load(), kLoad);
  ASSERT_EQ(unload(), kUnload);
}

TEST_F(PluggableArgsType, Mutators)
{
  ASSERT_NO_THROW(load(kLoad + " another argument;")
                      .unload(kUnload + " another argument;"));

  ASSERT_EQ(load(), kLoad + " another argument;");
  ASSERT_EQ(unload(), kUnload + " another argument;");
}

}  // namespace unit
}  // namespace tests
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_TEST_UNIT_TYPE_HH_

// # vim: sw=2 sts=2 si ai et
