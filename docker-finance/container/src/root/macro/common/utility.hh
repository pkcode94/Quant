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

#ifndef CONTAINER_SRC_ROOT_MACRO_COMMON_UTILITY_HH_
#define CONTAINER_SRC_ROOT_MACRO_COMMON_UTILITY_HH_

#include <initializer_list>
#include <string>
#include <utility>

#include "../../common/type.hh"
#include "../../common/utility.hh"

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
//! \brief Macro's pseudo-path handler
//! \note Parent directory for all macros are outside of repository's `root` directory
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
class MacroPath final : public ::dfi::common::PluggablePath
{
 public:
  //! \param pseudo Must be string "repo/<relative>" or "custom/<relative>"
  //!   where "repo" is a repository macro, "custom" is a custom macro,
  //!   and where <relative> is a pseudo-path relative to macro's
  //!   operating system location.
  explicit MacroPath(const std::string& pseudo)
      : ::dfi::common::PluggablePath(
            pseudo,
            ::dfi::common::type::PluggablePath({
                ::dfi::common::get_root_path() + "macro/",
                "" /* TODO(afiore): currently no-op */
            }))
  {
  }
};

//! \brief Macro's pluggable namespace and entrypoint handler
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
class MacroSpace final : public ::dfi::common::PluggableSpace
{
 public:
  explicit MacroSpace(const std::string& inner, const std::string& entry)
      : ::dfi::common::PluggableSpace(
            ::dfi::common::type::PluggableSpace({"macro", inner, entry}))
  {
  }
};

//! \brief Macro's arguments handler
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
class MacroArgs final : public ::dfi::common::PluggableArgs
{
 public:
  explicit MacroArgs(const std::string& load, const std::string& unload)
      : ::dfi::common::PluggableArgs(
            ::dfi::common::type::PluggableArgs({load, unload}))
  {
  }
};

//! \brief Macro handler
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
class Macro final
    : public ::dfi::common::Pluggable<MacroPath, MacroSpace, MacroArgs>
{
 public:
  explicit Macro(
      ::dfi::common::type::Pluggable<MacroPath, MacroSpace, MacroArgs> macro)
      : ::dfi::common::Pluggable<MacroPath, MacroSpace, MacroArgs>(macro)
  {
  }
};

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void load(const std::string& path)
{
  ::dfi::common::load(path);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void load(const std::initializer_list<std::string>& paths)
{
  ::dfi::common::load(paths);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void unload(const std::string& path)
{
  ::dfi::common::unload(path);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void unload(const std::initializer_list<std::string>& paths)
{
  ::dfi::common::unload(paths);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::common` free functions instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.0.0
class Command : public ::dfi::common::Command
{
};

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void reload(const std::string& path)
{
  ::dfi::common::reload(path);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::macro` free function instead
//! \todo Remove in 2.0.0
void reload(const std::initializer_list<std::string>& paths)
{
  ::dfi::common::reload(paths);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::common` free function instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.0.0
std::string get_env(const std::string& var)
{
  return ::dfi::common::get_env(var);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::common` free function instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.0.0
int exec(const std::string& cmd)
{
  return ::dfi::common::exec(cmd);
}

//! \deprecated This will be removed in the v2 API; use the `dfi::common` free function instead
//! \todo Remove in 2.0.0
//! \since docker-finance 1.0.0
std::string make_timestamp()
{
  return ::dfi::common::make_timestamp();
}
}  // namespace common

//
// Loader
//

//! \brief Loads a single macro
//!
//! \param path common::MacroPath
//! \param args common::MacroArgs
//!
//! \ref dfi::macro::common::Macro
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::load({"repo/example/example1.C"}, {"using foo = int; foo f;"})
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using foo = int; foo f;` to the macro's loader (macro-implementation defined).
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void load(const common::MacroPath& path, const common::MacroArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::MacroPath, common::MacroSpace, common::MacroArgs>
          type{path, common::MacroSpace{path.parent(), path.child()}, args};

  common::Macro macro{type};
  macro.load();
}

//! \brief Convenience wrapper to load a list of single macros with argument
//!
//! \ref load(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param macros List of single macros
//!
//! \details
//!
//!   Example:
//!
//!     &emsp; root [0] dfi::macro::load({{"repo/example/example1.C", "using foo = int; foo f;"}, {"repo/example/example2.C", "using bar = char; bar b;"}})
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//!   And pass macro-implementation defined auto-loader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C`
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void load(
    const std::initializer_list<
        std::pair<common::MacroPath, common::MacroArgs>>& macros)
{
  for (const auto& macro : macros)
    load(macro.first, macro.second);
}

//! \brief Convenience wrapper to load a single macro with optional argument
//!
//! \ref load(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \param arg Optional string argument to pass to macro's auto-loader (macro-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::load("repo/example/example1.C", "using foo = int; foo f;")
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using foo = int; foo f;` to the macro's loader (macro-implementation defined).
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void load(const std::string& path, const std::string& arg = {})
{
  load(common::MacroPath{path}, common::MacroArgs{arg, ""});
}

//! \brief Convenience wrapper to load a list of macros with no arguments
//!
//! \ref load(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::load({"repo/example/example1.C", "repo/example/example2.C"})
//!
//!   Will load both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
void load(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    load(path);
}

//
// Unloader
//

//! \brief Unloads a single macro
//!
//! \param path common::MacroPath
//! \param args common::MacroArgs
//!
//! \ref dfi::macro::common::Macro
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::unload({"repo/example/example1.C"}, {"using foo = int; foo f;"})
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using foo = int; foo f;` to the macro's unloader (macro-implementation defined).
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void unload(const common::MacroPath& path, const common::MacroArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::MacroPath, common::MacroSpace, common::MacroArgs>
          type{path, common::MacroSpace{path.parent(), path.child()}, args};

  common::Macro macro{type};
  macro.unload();
}

//! \brief Convenience wrapper to unload a list of single macros with arguments
//!
//! \ref unload(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param macros List of single macros
//!
//! \details
//!
//!   Example:
//!
//!     &emsp; root [0] dfi::macro::unload({{"repo/example/example1.C", "using foo = int; foo f;"}, {"repo/example/example2.C", "using bar = char; bar b;"}})
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//!   And pass macro-implementation defined auto-unloader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C`
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void unload(
    const std::initializer_list<
        std::pair<common::MacroPath, common::MacroArgs>>& macros)
{
  for (const auto& macro : macros)
    unload(macro.first, macro.second);
}

//! \brief Convenience wrapper to unload a single macro with optional argument
//!
//! \ref unload(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \param arg Optional string argument to pass to macro's auto-unloader (macro-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::unload("repo/example/example1.C", "using foo = int; foo f;")
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using foo = int; foo f;` to the macro's unloader (macro-implementation defined).
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void unload(const std::string& path, const std::string& arg = {})
{
  unload(common::MacroPath{path}, common::MacroArgs{"", arg});
}

//! \brief Convenience wrapper to unload a list of macros with no arguments
//!
//! \ref unload(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::unload({"repo/example/example1.C", "repo/example/example2.C"})
//!
//!   Will unload both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void unload(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    unload(path);
}

//
// Reloader
//

//! \brief Reloads a single macro
//!
//! \param path common::MacroPath
//! \param args common::MacroArgs
//!
//! \ref dfi::macro::common::Macro
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::reload({"repo/example/example1.C"}, {"using foo = int; foo f;", "using bar = char; bar b;"})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using bar = char; bar b;` to the macro's unloader (macro-implementation defined)
//!   and pass `using foo = int; foo f;` to the macro's loader (macro-implementation defined)
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void reload(const common::MacroPath& path, const common::MacroArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::MacroPath, common::MacroSpace, common::MacroArgs>
          type{path, common::MacroSpace{path.parent(), path.child()}, args};

  common::Macro macro{type};
  macro.reload();
}

//! \brief Convenience wrapper to reload a list of single macros with arguments
//!
//! \ref reload(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param macros List of populated pair of common::MacroPath and common::MacroArgs
//!
//! \details
//!
//!   Example:
//!     &emsp; root [0] dfi::macro::reload({{"repo/example/example1.C", "using foo = int; foo f;"}, {"repo/example/example2.C", "using bar = char; bar b;"}})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//!   And pass macro-implementation defined auto-loader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C`
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void reload(
    const std::initializer_list<
        std::pair<common::MacroPath, common::MacroArgs>>& macros)
{
  for (const auto& macro : macros)
    reload(macro.first, macro.second);
}

//! \brief Convenience wrapper to reload a single macro with optional arguments
//!
//! \ref reload(const common::MacroPath& path, const common::MacroArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \param args Optional pair of arguments to pass to macro's auto-loader/unloader (macro-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::reload("repo/example/example1.C", {"using foo = int; foo f;", ""})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!
//!   and pass `using foo = int; foo f;` to the macro's auto-loader (macro-implementation defined).
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void reload(
    const std::string& path,
    const std::pair<std::string, std::string>& args = {})
{
  reload(common::MacroPath{path}, common::MacroArgs{args.first, args.second});
}

//! \brief Convenience wrapper to reload a list of macros with no arguments
//!
//! \ref reload(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to macro location, as described by common::MacroPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::macro::reload({"repo/example/example1.C", "repo/example/example2.C"})
//!
//!   Will reload both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example1.C
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/example/example2.C
//!
//! \warning
//!   To utilize macro auto-(un)loader functionality, the macro's parent directory *MUST* align with the macro's namespace\n
//!   and the macro's entrypoint class *MUST* align with the macro's filename; e.g., "repo/example/example1.C" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example1.C` within the namespace `dfi::macro::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_macro
//! \since docker-finance 1.1.0
void reload(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    reload(path);
}
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_COMMON_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
