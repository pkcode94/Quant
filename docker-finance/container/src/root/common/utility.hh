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

#ifndef CONTAINER_SRC_ROOT_COMMON_UTILITY_HH_
#define CONTAINER_SRC_ROOT_COMMON_UTILITY_HH_

#include <ctime>
#include <filesystem>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "./type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::common
//! \brief Shared common functionality
//! \since docker-finance 1.1.0
namespace common
{

//! \concept Exception
//! \brief Exception type concept for `dfi` exceptions
//! \ingroup cpp_common_impl
//! \since docker-finance 1.1.0
template <typename t_exception>
concept Exception = std::derived_from<t_exception, type::Exception>;

//! \brief Throw exception using `dfi` exception type
//! \param message Message to pass to exception type
//! \param location Source location of exception
//! \ingroup cpp_common_impl
//! \since docker-finance 1.1.0
template <Exception t_exception>
inline void throw_ex(
    const std::string& message = {},
    const std::source_location location = std::source_location::current())
{
  // TODO(unassigned): when throwing because of heap allocation
  std::string msg;
  msg += std::string("     \n\tFILE = ") + location.file_name();
  msg += std::string("     \n\tFUNC = ") + location.function_name();
  msg += std::string("     \n\tLINE = ") + location.line();
  msg += std::string("     \n\tWHAT = ") + message;
  throw t_exception(msg);
}

//! \brief Throw exception using `dfi` exception type (but only on condition)
//! \param condition If boolean condition is true, throw
//! \param message Message to pass to exception type
//! \param location Source location of exception
//! \ingroup cpp_common_impl
//! \since docker-finance 1.1.0
template <Exception t_exception>
inline void throw_ex_if(
    const bool condition,
    const std::string& message = {},
    const std::source_location location = std::source_location::current())
{
  if (condition)
    {
      throw_ex<t_exception>(message, location);
    }
}

// TODO(afiore): add logger with loglevels

//! \brief Process a line of code or interpreter command
//! \since docker-finance 1.1.0
inline void line(const std::string& line)
{
  using EErrorCode = ::TInterpreter::EErrorCode;
  auto ecode = std::make_unique<EErrorCode>();

  // TODO(afiore): log levels
  std::cout << "Interpreting: '" << line << "'" << std::endl;
  gInterpreter->ProcessLine(line.c_str(), ecode.get());
  throw_ex_if<type::RuntimeError>(*ecode != EErrorCode::kNoError, line);
}

//! \brief Load file with given path into interpreter
//! \details Can load a source file or library file
//! \param path Operating system path (absolute or relative)
//! \since docker-finance 1.1.0
inline void load(const std::string& path)
{
  throw_ex_if<type::RuntimeError>(
      !std::filesystem::is_regular_file(path), "invalid file: '" + path + "'");

  // TODO(afiore): log levels
  std::cout << "Loading: '" << path << "'" << std::endl;
  gInterpreter->LoadFile(path.c_str());
}

//! \brief Load files with given paths into interpreter
//! \details Can load source files and/or library files
//! \param path Operating system paths (absolute or relative)
//! \since docker-finance 1.1.0
inline void load(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    load(path);
}

//! \brief Wrappers to commonly used interpreter abstractions
//! \ingroup cpp_common_impl
//! \deprecated This will be removed in the v2 API; use the `dfi::common` free functions instead
//! \since docker-finance 1.0.0
class Command
{
 public:
  Command() = default;
  ~Command() = default;

  Command(const Command&) = default;
  Command& operator=(const Command&) = default;

  Command(Command&&) = default;
  Command& operator=(Command&&) = default;

 public:
  //! \deprecated This will be removed in the v2 API; use the `dfi::common` free function instead
  //! \since docker-finance 1.0.0
  static void load(const std::string& path) { ::dfi::common::load(path); }

  //! \deprecated This will be removed in the v2 API; use the `dfi::common` free function instead
  //! \since docker-finance 1.0.0
  static void load(const std::initializer_list<std::string>& paths)
  {
    ::dfi::common::load(paths);
  }
};

//! \brief Unload file with given path out of interpreter
//! \details Can unload a source file or library file
//! \param path Operating system path (absolute or relative)
//! \since docker-finance 1.1.0
inline void unload(const std::string& path)
{
  throw_ex_if<type::RuntimeError>(
      !std::filesystem::is_regular_file(path), "invalid file: '" + path + "'");

  // TODO(afiore): log levels
  std::cout << "Unloading: '" << path << "'" << std::endl;
  gInterpreter->UnloadFile(path.c_str());
}

//! \brief Unload files with given paths out of interpreter
//! \details Can unload source files and/or library files
//! \param path Operating system paths (absolute or relative)
//! \since docker-finance 1.1.0
inline void unload(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    unload(path);
}

//! \brief Reload file with given path out of/into interpreter
//! \details Can reload source files and/or library files
//! \param path Operating system paths (absolute or relative)
//! \since docker-finance 1.1.0
inline void reload(const std::string& path)
{
  unload(path);
  load(path);
}

//! \brief Reload files with given paths out of/into interpreter
//! \details Can reload source files and/or library files
//! \param path Operating system paths (absolute or relative)
//! \since docker-finance 1.1.0
inline void reload(const std::initializer_list<std::string>& paths)
{
  unload(paths);
  load(paths);
}

//! \brief Add an include directory to interpreter
//! \param path Path to include (absolute or relative)
//! \warning Only pass the complete path to directory and not "-I"
//! \warning To pass multiple paths, call this function multiple times
//! \exception type::RuntimeError if path doesn't exist or cannot be included
//! \since docker-finance 1.1.0
inline void add_include_dir(const std::string& path)
{
  throw_ex_if<type::RuntimeError>(
      !std::filesystem::is_directory(path),
      "invalid directory: '" + path + "'");

  // TODO(afiore): log levels
  std::cout << "Adding: '" << path << "'" << std::endl;
  gInterpreter->AddIncludePath(path.c_str());
}

//! \brief Add a linked library into interpreter
//! \param path Path of library to add (absolute or relative)
//! \warning Only pass the complete path to library and not "-l"
//! \warning To pass multiple libraries, call this function multiple times
//! \exception type::RuntimeError if library doesn't exist or cannot be linked
//! \since docker-finance 1.1.0
inline void add_linked_lib(const std::string& path)
{
  load(path);
}

//! \brief Remove a linked library from interpreter
//! \param path Path of library to remove (absolute or relative)
//! \warning Only pass the complete path to library and not "-l"
//! \warning To pass multiple libraries, call this function multiple times
//! \exception type::RuntimeError if library doesn't exist or cannot be linked
//! \since docker-finance 1.1.0
inline void remove_linked_lib(const std::string& path)
{
  unload(path);
}

//! \brief Get underlying environment variable
//! \param var Environment variable
//! \return Value of given environment variable
//! \exception type::RuntimeError Throws if env var is not set or unavailable
//! \note ROOT environment variables include shell (and shell caller) environment
//! \since docker-finance 1.0.0
std::string get_env(const std::string& var)
{
  const auto* env = gSystem->Getenv(var.c_str());

  throw_ex_if<type::RuntimeError>(
      !env, "'" + var + "' is not set or is unavailable");

  return std::string{env};
}

//! \brief Get `dfi`'s root-related code directory
//! \details Operating system path to where all `dfi` root-related code resides
//! \return Operating system absolute path (with trailing slash)
//! \exception type::RuntimeError if unable to get environment
//! \since docker-finance 1.1.0
std::string get_root_path()
{
  return get_env("DOCKER_FINANCE_CONTAINER_REPO") + "/src/root/";
}

//! \brief Execute a command in operating system shell
//! \param cmd Operating system shell command [args]
//! \return Return value of command
//! \since docker-finance 1.0.0
int exec(const std::string& cmd)
{
  return gSystem->Exec(cmd.c_str());
}

//! \brief Exit the interpreter with given status (and message)
//! \param code Exit code status (return code)
//! \param message Optional exit message
//! \since docker-finance 1.2.0
void exit(const int code, const std::string& message = {})
{
  // TODO(unassigned): logger
  if (!message.empty())
    std::cout << "Exiting: '" << message << "'" << std::endl;

  gSystem->Exit(code);
}

//! \brief Make current timestamp
//! \return timestamp in "yyyy-mm-ddThh:mm:ssZ" format
//! \since docker-finance 1.0.0
std::string make_timestamp()
{
  const std::time_t t{std::time({})};
  std::vector<char> time(std::size("yyyy-mm-ddThh:mm:ssZ"));
  std::strftime(time.data(), time.size(), "%FT%TZ", std::gmtime(&t));
  return std::string{time.data()};
}

//! \brief Base pseudo-path handler for various pluggables (plugins and macros)
//!
//! \details Handles pluggable paths, typically used by pluggable
//!   implementations when auto-(un|re)loading.
//!
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
class PluggablePath
{
 public:
  //! \brief Parses (or re-parses) constructed types
  //! \warning Only call this function after constructing if underlying type::PluggablePath has been changed (post-construction)
  //! \return NPI reference
  auto& parse()
  {
    // Invalid characters
    const std::regex regex{"[a-zA-Z0-9/_\\-\\.]+"};
    const std::string msg{"invalid characters in path"};

    if (!m_path.repo().empty())
      throw_ex_if<type::RuntimeError>(
          !std::regex_match(m_path.repo(), regex), msg);

    if (!m_path.custom().empty())
      throw_ex_if<type::RuntimeError>(
          !std::regex_match(m_path.custom(), regex), msg);

    throw_ex_if<type::RuntimeError>(!std::regex_match(m_pseudo, regex), msg);

    // Parse out pseudo tag
    auto pos = m_pseudo.find('/');
    throw_ex_if<type::RuntimeError>(!pos, "no pseudo tag");
    const std::string tag{m_pseudo.substr(0, pos)};

    // Set family group
    m_family = m_pseudo;
    m_family.erase(0, pos + 1);
    throw_ex_if<type::RuntimeError>(m_family.empty(), "no family found");

    // Set absolute path
    m_absolute = m_family;
    if (tag == "repo")
      {
        m_is_repo = true;
        m_absolute.insert(0, m_path.repo());
      }
    else if (tag == "custom")
      {
        m_is_custom = true;
        m_absolute.insert(0, m_path.custom());
      }
    else
      {
        throw_ex<type::RuntimeError>(
            "must be of tag 'repo' or 'custom' | was given: '" + tag + "'");
      }

    // Set parent path (director(y|ies))
    pos = m_family.find_last_of('/');
    m_parent = m_family.substr(0, pos);

    // Set child (filename)
    m_child = m_family.substr(pos + 1);
    throw_ex_if<type::RuntimeError>(
        m_child.empty() || m_child == m_parent, "child not found");

    return *this;
  }

 protected:
  //! \brief Sets pluggable absolute path from given pseudo path
  //! \param pseudo Pseudo-path ('repo/<relative>' or 'custom/<relative>')
  //! \param base Operating system absolute paths to pluggable locations
  //! \exception type::RuntimeError If not a valid pseudo-path
  PluggablePath(const std::string& pseudo, const type::PluggablePath& path)
      : m_pseudo(pseudo), m_path(path), m_is_repo(false), m_is_custom(false)
  {
    parse();
  }
  ~PluggablePath() = default;

  PluggablePath(const PluggablePath&) = default;
  PluggablePath& operator=(const PluggablePath&) = default;

  PluggablePath(PluggablePath&&) = default;
  PluggablePath& operator=(PluggablePath&&) = default;

 public:
  //! \brief Mutator to underlying type::PluggablePath
  //! \warning Use with caution: underlying implementation may change
  auto& operator()() { return m_path; }

  //! \brief Accessor to underlying type::PluggablePath
  //! \warning Use with caution: underlying implementation may change
  const auto& operator()() const { return m_path; }

 public:
  //! \return The pluggable's complete pseudo-path
  const std::string& pseudo() const { return m_pseudo; }

  //! \return The pluggable's operating system absolute path (with parsed pseudo-path)
  const std::string& absolute() const { return m_absolute; }

  //! \return The pluggable's relative parent director(y|ies) derived from pseudo-path
  //! \warning Trailing slash is removed
  //! \note This also represents the expected namespace used for pluggable auto-loading
  const std::string& parent() const { return m_parent; }

  //! \return The pluggable's child (filename)
  const std::string& child() const { return m_child; }

  //! \return The pluggable's group of parent and child members
  const std::string& family() const { return m_family; }

  //! \return true if pseudo-path describes a repository location
  bool is_repo() const { return m_is_repo; }

  //! \return true if pseudo-path describes a custom location
  bool is_custom() const { return m_is_custom; }

 private:
  type::PluggablePath m_path;
  std::string m_pseudo, m_absolute;
  std::string m_family, m_parent, m_child;
  bool m_is_repo, m_is_custom;
};

//! \brief Base pluggable space handler for various pluggables (plugins and macros)
//!
//! \details Handles pluggable namespace and entrypoint class name,
//!   typically used by pluggable implementations when auto-(un|re)loading.
//!
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
class PluggableSpace
{
 public:
  //! \brief Parses (or re-parses) constructed types
  //! \warning Only call this function after constructing if underlying type::PluggableSpace has been changed (post-construction)
  //! \return NPI reference
  auto& parse()
  {
    auto const parser = [](const std::string& space) -> std::string {
      std::string parsed{space};

      // NOTE: allowed to be empty (for now)
      if (!parsed.empty())
        {
          throw_ex_if<type::RuntimeError>(
              !std::regex_match(
                  parsed,
                  std::regex{
                      "[a-zA-Z0-9:/_\\-\\.]+"} /* TODO(unassigned): refine */),
              "invalid characters in pluggable space");

          if (parsed.find('/'))  // cppcheck-suppress stlIfStrFind
            {
              parsed = std::regex_replace(parsed, std::regex{"/"}, "::");
            }
          if (parsed.find('-'))  // cppcheck-suppress stlIfStrFind
            {
              parsed = std::regex_replace(parsed, std::regex{"-"}, "_");
            }
          if (parsed.find('.'))  // cppcheck-suppress stlIfStrFind
            {
              parsed = std::regex_replace(parsed, std::regex{"\\."}, "_");
            }
        }

      return parsed;
    };

    m_space.outer(parser(m_space.outer()));
    m_space.inner(parser(m_space.inner()));
    m_space.entry(parser(m_space.entry()));

    return *this;
  }

 protected:
  // \note Since the current presumption is that a PluggableSpace is likely
  //   to be derived from an operating system path (via PluggablePath),
  //   there's leeway for path-to-space conversions to be done here.
  explicit PluggableSpace(const type::PluggableSpace& space) : m_space(space)
  {
    parse();
  }
  ~PluggableSpace() = default;

  PluggableSpace(const PluggableSpace&) = default;
  PluggableSpace& operator=(const PluggableSpace&) = default;

  PluggableSpace(PluggableSpace&&) = default;
  PluggableSpace& operator=(PluggableSpace&&) = default;

 public:
  //! \brief Mutator to underlying type::PluggableSpace
  //! \warning Use with caution: underlying implementation may change
  auto& operator()() { return m_space; }

  //! \brief Accessor to underlying type::PluggableSpace
  //! \warning Use with caution: underlying implementation may change
  const auto& operator()() const { return m_space; }

 public:
  //! \return The pluggable's outer namespace
  const std::string& outer() const { return m_space.outer(); }

  //! \return The pluggable's inner namespace
  const std::string& inner() const { return m_space.inner(); }

  //! \return The pluggable's entrypoint class name
  const std::string& entry() const { return m_space.entry(); }

  //! \return true if the outer namespace was set
  bool has_outer() const { return !m_space.outer().empty(); }

  //! \return true if the inner namespace was set
  bool has_inner() const { return !m_space.inner().empty(); }

  //! \return true if the entrypoint class name was set
  bool has_entry() const { return !m_space.entry().empty(); }

 private:
  type::PluggableSpace m_space;
};

//! \brief Base argument handler for various pluggables (plugins and macros)
//!
//! \details Handles pluggable arguments, typically used by pluggable
//!   implementations when auto-(un|re)loading.
//!
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
class PluggableArgs
{
 protected:
  explicit PluggableArgs(const type::PluggableArgs& args) : m_args(args) {}
  ~PluggableArgs() = default;

  PluggableArgs(const PluggableArgs&) = default;
  PluggableArgs& operator=(const PluggableArgs&) = default;

  PluggableArgs(PluggableArgs&&) = default;
  PluggableArgs& operator=(PluggableArgs&&) = default;

 public:
  //! \brief Mutator to underlying type::PluggableArgs
  //! \warning Use with caution: underlying implementation may change
  auto& operator()() { return m_args; }

  //! \brief Accessor to underlying type::PluggableArgs
  //! \warning Use with caution: underlying implementation may change
  const auto& operator()() const { return m_args; }

 public:
  //! \return The pluggable's loader argument
  const std::string& load() const { return m_args.load(); }

  //! \return The pluggable's unloader argument
  const std::string& unload() const { return m_args.unload(); }

  //! \return true if the load argument was set
  bool has_load() const { return !m_args.load().empty(); }

  //! \return true if the unload argument was set
  bool has_unload() const { return !m_args.unload().empty(); }

 private:
  type::PluggableArgs m_args;
};

//! \concept dfi::common::PPath
//! \brief Pluggable base implementation constraint (PluggablePath)
//! \ref dfi::common::PluggablePath
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
template <typename t_path>
concept PPath = std::derived_from<t_path, PluggablePath>;

//! \concept dfi::common::PSpace
//! \brief Pluggable base implementation constraint (PluggableSpace)
//! \ref dfi::common::PluggableSpace
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
template <typename t_space>
concept PSpace = std::derived_from<t_space, PluggableSpace>;

//! \concept dfi::common::PArgs
//! \brief Pluggable base implementation constraint (PluggableArgs)
//! \ref dfi::common::PluggableArgs
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
template <typename t_args>
concept PArgs = std::derived_from<t_args, PluggableArgs>;

//! \brief Base pluggable handler
//! \ingroup cpp_plugin_impl cpp_macro_impl
//! \since docker-finance 1.1.0
template <PPath t_path, PSpace t_space, PArgs t_args>
class Pluggable
{
 protected:
  explicit Pluggable(const type::Pluggable<t_path, t_space, t_args>& plug)
      : m_plug(plug)
  {
  }
  ~Pluggable() = default;

  Pluggable(const Pluggable&) = default;
  Pluggable& operator=(const Pluggable&) = default;

  Pluggable(Pluggable&&) = default;
  Pluggable& operator=(Pluggable&&) = default;

 public:
  //! \brief Mutator to underlying type::Pluggable
  //! \warning Use with caution: underlying implementation may change
  auto& operator()() { return m_plug; }

  //! \brief Accessor to underlying type::Pluggable
  //! \warning Use with caution: underlying implementation may change
  const auto& operator()() const { return m_plug; }

 public:
  //! \brief Loads a single pluggable
  //! \return NPI reference
  //! \ingroup cpp_plugin_impl cpp_macro_impl
  //! \since docker-finance 1.1.0
  const auto& load() const
  {
    // Load pluggable file
    ::dfi::common::load(m_plug.path().absolute());

    // Prepare pluggable entry
    const std::string entry{
        "dfi::" + m_plug.space().outer() + "::" + m_plug.space().inner()
        + "::" + m_plug.space().entry()};

    // Allow quotations in loader argument
    std::stringstream arg;
    arg << std::quoted(m_plug.args().load());

    // Execute pluggable's loader
    ::dfi::common::line(entry + "::load(" + arg.str() + ")");

    return *this;
  }

  //! \brief Unloads a single pluggable
  //! \return NPI reference
  //! \ingroup cpp_plugin_impl cpp_macro_impl
  //! \since docker-finance 1.1.0
  const auto& unload() const
  {
    // Prepare pluggable entry
    const std::string entry{
        "dfi::" + m_plug.space().outer() + "::" + m_plug.space().inner()
        + "::" + m_plug.space().entry()};

    // Allow quotations in unloader argument
    std::stringstream arg;
    arg << std::quoted(m_plug.args().unload());

    // Execute pluggable's unloader
    ::dfi::common::line(entry + "::unload(" + arg.str() + ")");

    // Unload pluggable file
    ::dfi::common::unload(m_plug.path().absolute());

    return *this;
  }

  //! \brief Reloads a single pluggable
  //! \return NPI reference
  //! \ingroup cpp_plugin_impl cpp_macro_impl
  //! \since docker-finance 1.1.0
  const auto& reload() const { return unload().load(); }

 private:
  type::Pluggable<t_path, t_space, t_args> m_plug;
};

}  // namespace common
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_COMMON_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
