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
//! \since docker-finance 1.1.0

#ifndef CONTAINER_SRC_ROOT_COMMON_TYPE_HH_
#define CONTAINER_SRC_ROOT_COMMON_TYPE_HH_

#include <stdexcept>
#include <string>
#include <utility>

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::common
//! \brief Shared common functionality
//! \since docker-finance 1.1.0
namespace common
{
//! \namespace dfi::common::type
//! \brief docker-finance defined common types
//! \since docker-finance 1.1.0
namespace type
{
//! \brief Base exception class
//! \ingroup cpp_type_exceptions
class Exception : virtual public std::exception
{
 public:
  //! \brief Exception type tag
  enum struct kType : uint8_t
  {
    RuntimeError,
    InvalidArgument,
  };

  //! \brief Construct by type tag with given message
  Exception(const kType type, const std::string& what)
      : m_type(type), m_what(what)
  {
  }
  virtual ~Exception() = default;

  Exception(const Exception&) = default;
  Exception& operator=(const Exception&) = default;

  Exception(Exception&&) = default;
  Exception& operator=(Exception&&) = default;

 public:
  //! \return Exeption type tag
  kType type() const noexcept { return m_type; }

  //! \return Exception message
  const char* what() const noexcept { return m_what.c_str(); }

 private:
  kType m_type;
  std::string m_what;
};

//! \brief Exception type for runtime errors
//! \ingroup cpp_type_exceptions
struct RuntimeError final : public Exception
{
  explicit RuntimeError(const std::string& what = {})
      : Exception(Exception::kType::RuntimeError, what)
  {
  }
};

//! \brief Exception type for invalid arguments (logic error)
//! \ingroup cpp_type_exceptions
struct InvalidArgument final : public Exception
{
  explicit InvalidArgument(const std::string& what = {})
      : Exception(Exception::kType::InvalidArgument, what)
  {
  }
};

//! \brief Data type of underlying operating system locations for pluggable pseudo-paths
//! \todo C++23 concept pair-like
//! \since docker-finance 1.1.0
class PluggablePath
{
  using t_pair = std::pair<std::string, std::string>;

 public:
  //! \brief Construct with {repository, custom} pluggable directory locations
  //! \param pair An object where the first element is the repository
  //!   absolute directory path and the second element is the custom
  //!   absolute directory path.
  explicit PluggablePath(const t_pair& pair) : m_pair(pair) {}
  ~PluggablePath() = default;

  PluggablePath(const PluggablePath&) = default;
  PluggablePath& operator=(const PluggablePath&) = default;

  PluggablePath(PluggablePath&&) = default;
  PluggablePath& operator=(PluggablePath&&) = default;

 public:
  //! \brief Set operating system path to repository pluggable
  //! \return NPI reference
  auto& repo(const std::string& arg)
  {
    m_pair.first = arg;
    return *this;
  }

  //! \brief Set operating system path to custom pluggable
  //! \return NPI reference
  auto& custom(const std::string& arg)
  {
    m_pair.second = arg;
    return *this;
  }

  //! \return Operating system path to repository pluggable
  const std::string& repo() const { return m_pair.first; }

  //! \return Operating system path to custom pluggable
  const std::string& custom() const { return m_pair.second; }

 private:
  t_pair m_pair;
};

//! \brief Data type for the pluggable's namespace and entrypoint class name
//! \todo C++23 concept tuple-like
//! \since docker-finance 1.1.0
class PluggableSpace
{
  using t_space = std::tuple<std::string, std::string, std::string>;

 public:
  //! \brief Construct data type with most-outer and inner namespaces
  explicit PluggableSpace(const t_space& args) : m_space(args) {}
  ~PluggableSpace() = default;

  PluggableSpace(const PluggableSpace&) = default;
  PluggableSpace& operator=(const PluggableSpace&) = default;

  PluggableSpace(PluggableSpace&&) = default;
  PluggableSpace& operator=(PluggableSpace&&) = default;

 public:
  //! \brief Set pluggable's most-outer namespace
  //! \return NPI reference
  auto& outer(const std::string& arg)
  {
    std::get<0>(m_space) = arg;
    return *this;
  }

  //! \brief Set pluggable's inner namespace(s)
  //! \return NPI reference
  auto& inner(const std::string& arg)
  {
    std::get<1>(m_space) = arg;
    return *this;
  }

  //! \brief Set pluggable's entrypoint class name
  //! \return NPI reference
  auto& entry(const std::string& arg)
  {
    std::get<2>(m_space) = arg;
    return *this;
  }

  //! \return Pluggable's most-outer namespace
  const std::string& outer() const { return std::get<0>(m_space); }

  //! \return Pluggable's inner namespace(s)
  const std::string& inner() const { return std::get<1>(m_space); }

  //! \return Pluggable's entrypoint class name
  const std::string& entry() const { return std::get<2>(m_space); }

 private:
  t_space m_space;
};

//! \brief Data type for pluggable auto-(un)loader arguments
//! \todo C++23 concept pair-like
//! \since docker-finance 1.1.0
class PluggableArgs
{
  using t_pair = std::pair<std::string, std::string>;

 public:
  //! \brief Construct data type with load/unload arguments
  explicit PluggableArgs(const t_pair& pair) : m_pair(pair) {}
  ~PluggableArgs() = default;

  PluggableArgs(const PluggableArgs&) = default;
  PluggableArgs& operator=(const PluggableArgs&) = default;

  PluggableArgs(PluggableArgs&&) = default;
  PluggableArgs& operator=(PluggableArgs&&) = default;

 public:
  //! \brief Set pluggable loader argument
  //! \return NPI reference
  auto& load(const std::string& arg)
  {
    m_pair.first = arg;
    return *this;
  }

  //! \brief Set pluggable unloader argument
  //! \return NPI reference
  auto& unload(const std::string& arg)
  {
    m_pair.second = arg;
    return *this;
  }

  //! \return Pluggable loader argument
  const std::string& load() const { return m_pair.first; }

  //! \return Pluggable unloader argument
  const std::string& unload() const { return m_pair.second; }

 private:
  t_pair m_pair;
};

//! \concept dfi::common::type::PPath
//! \brief Pluggable constrained data type (PluggablePath)
//! \ref dfi::common::type::PluggablePath
//! \since docker-finance 1.1.0
template <typename t_path>
concept PPath = requires(t_path path, PluggablePath plug) {
  path.operator()().operator=(plug);
  path.operator()().repo("").custom("");
  path.operator()().repo();
  path.operator()().custom();
};

//! \concept dfi::common::type::PSpace
//! \brief Pluggable constrained data type (PluggableSpace)
//! \ref dfi::common::type::PluggableSpace
//! \since docker-finance 1.1.0
template <typename t_space>
concept PSpace = requires(t_space space, PluggableSpace plug) {
  space.operator()().operator=(plug);
  space.operator()().outer("").inner("").entry("");
  space.operator()().outer();
  space.operator()().inner();
  space.operator()().entry();
};

//! \concept dfi::common::type::PArgs
//! \brief Pluggable constrained data type (PluggableArgs)
//! \ref dfi::common::type::PluggableArgs
//! \since docker-finance 1.1.0
template <typename t_args>
concept PArgs = requires(t_args args, PluggableArgs plug) {
  args.operator()().operator=(plug);
  args.operator()().load("").unload("");
  args.operator()().load();
  args.operator()().unload();
};

//! \brief Data type for pluggable handler
//! \since docker-finance 1.1.0
template <PPath t_path, PSpace t_space, PArgs t_args>
class Pluggable
{
 public:
  explicit Pluggable(
      const t_path& path,
      const t_space& space,
      const t_args& args)
      : m_path(path), m_space(space), m_args(args)
  {
  }
  ~Pluggable() = default;

  Pluggable(const Pluggable&) = default;
  Pluggable& operator=(const Pluggable&) = default;

  Pluggable(Pluggable&&) = default;
  Pluggable& operator=(Pluggable&&) = default;

 public:
  //! \brief Mutator to underlying path type
  t_path& path() { return m_path; }

  //! \brief Mutator to underlying space type
  t_space& space() { return m_space; }

  //! \brief Mutator to underlying args type
  t_args& args() { return m_args; }

 public:
  //! \brief Accessor to underlying path type
  const t_path& path() const { return m_path; }

  //! \brief Accessor to underlying space type
  const t_space& space() const { return m_space; }

  //! \brief Accessor to underlying args type
  const t_args& args() const { return m_args; }

 private:
  t_path m_path;
  t_space m_space;
  t_args m_args;
};
}  // namespace type
}  // namespace common
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_COMMON_TYPE_HH_

// # vim: sw=2 sts=2 si ai et
