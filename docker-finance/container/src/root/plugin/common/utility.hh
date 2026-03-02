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

#ifndef CONTAINER_SRC_ROOT_PLUGIN_COMMON_UTILITY_HH_
#define CONTAINER_SRC_ROOT_PLUGIN_COMMON_UTILITY_HH_

#include <initializer_list>
#include <string>
#include <utility>

#include "../../common/type.hh"
#include "../../common/utility.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::plugin
//! \brief docker-finance plugins
//! \warning All plugins (repo/custom) must exist within this namespace
//!   and work within their own inner namespace
//! \since docker-finance 1.0.0
namespace plugin
{
//! \namespace dfi::plugin::common
//! \brief Shared ROOT plugin-related functionality
//! \since docker-finance 1.1.0
namespace common
{
//! \brief Plugin's pseudo-path handler
//! \note Parent directory for all plugins are outside of repository's `root` directory
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
class PluginPath final : public ::dfi::common::PluggablePath
{
 public:
  //! \param pseudo Must be string "repo/<relative>" or "custom/<relative>"
  //!   where "repo" is a repository plugin, "custom" is a custom plugin,
  //!   and where <relative> is a pseudo-path relative to plugin's
  //!   operating system location.
  explicit PluginPath(const std::string& pseudo)
      : ::dfi::common::PluggablePath(
            pseudo,
            ::dfi::common::type::PluggablePath(
                {::dfi::common::get_env("DOCKER_FINANCE_CONTAINER_REPO")
                     + "/plugins/root/",
                 ::dfi::common::get_env("DOCKER_FINANCE_CONTAINER_PLUGINS")
                     + "/root/"}))
  {
  }
};

//! \brief Plugin's namespace handler
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
class PluginSpace final : public ::dfi::common::PluggableSpace
{
 public:
  explicit PluginSpace(const std::string& inner, const std::string& entry)
      : ::dfi::common::PluggableSpace(
            ::dfi::common::type::PluggableSpace({"plugin", inner, entry}))
  {
  }
};

//! \brief Plugin's arguments handler
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
class PluginArgs final : public ::dfi::common::PluggableArgs
{
 public:
  explicit PluginArgs(const std::string& load, const std::string& unload)
      : ::dfi::common::PluggableArgs(
            ::dfi::common::type::PluggableArgs({load, unload}))
  {
  }
};

//! \brief Plugin handler
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
class Plugin final
    : public ::dfi::common::Pluggable<PluginPath, PluginSpace, PluginArgs>
{
 public:
  explicit Plugin(
      ::dfi::common::type::Pluggable<PluginPath, PluginSpace, PluginArgs>
          plugin)
      : ::dfi::common::Pluggable<PluginPath, PluginSpace, PluginArgs>(plugin)
  {
  }
};
}  // namespace common

//
// Loader
//

//! \brief Loads a single plugin
//!
//! \param path common::PluginPath
//! \param args common::PluginArgs
//!
//! \ref dfi::plugin::common::Plugin
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::load({"repo/example/example.cc"}, {"using foo = int; foo f;"})
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using foo = int; foo f;` to the plugin's loader (plugin-implementation defined).
//!
//!
//!   Example 2:
//!
//!     &emsp; root [0] dfi::plugin::load({"custom/example/example.cc"}, {"using bar = char; bar b;"})
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's loader (plugin-implementation defined).
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void load(const common::PluginPath& path, const common::PluginArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::PluginPath, common::PluginSpace, common::PluginArgs>
          type{path, common::PluginSpace{path.parent(), path.child()}, args};

  common::Plugin plugin{type};
  plugin.load();
}

//! \brief Convenience wrapper to load a list of single plugins with argument
//!
//! \ref load(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param plugins List of single plugins
//!
//! \details
//!
//!   Example:
//!
//!     &emsp; root [0] dfi::plugin::load({{"repo/example/example.cc", "using foo = int; foo f;"}, {"custom/example/example.cc", "using bar = char; bar b;"}})
//!
//!   Will load:
//!
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//!   And pass plugin-implementation defined auto-loader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void load(
    const std::initializer_list<
        std::pair<common::PluginPath, common::PluginArgs>>& plugins)
{
  for (const auto& plugin : plugins)
    load(plugin.first, plugin.second);
}

//! \brief Convenience wrapper to load a single plugin with optional argument
//!
//! \ref load(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \param arg Optional string argument to pass to plugin's auto-loader (plugin-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::load("repo/example.cc", "using foo = int; foo f;")
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using foo = int; foo f;` to the plugin's loader (plugin-implementation defined).
//!
//!
//!   Example 2:
//!
//!     &emsp; root [0] dfi::plugin::load("custom/example/example.cc", "using bar = char; bar b;")
//!
//!   Will load:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's loader (plugin-implementation defined).
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void load(const std::string& path, const std::string& arg = {})
{
  load(common::PluginPath{path}, common::PluginArgs{arg, ""});
}

//! \brief Convenience wrapper to load a list of plugins with no arguments
//!
//! \ref load(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::load({"repo/example/example.cc", "custom/example/example.cc"})
//!
//!   Will load both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc\n
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.0.0
void load(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    load(path);
}

//
// Unloader
//

//! \brief Unloads a single plugin
//!
//! \param path common::PluginPath
//! \param args common::PluginArgs
//!
//! \ref dfi::plugin::common::Plugin
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::unload({"repo/example/example.cc"}, {"using foo = int; foo f;"})
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using foo = int; foo f;` to the plugin's unloader (plugin-implementation defined).
//!
//!
//!   Example 2:
//!
//!     &emsp; root [0] dfi::plugin::unload({"custom/example/example.cc"}, {"using bar = char; bar b;"})
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's unloader (plugin-implementation defined).
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void unload(const common::PluginPath& path, const common::PluginArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::PluginPath, common::PluginSpace, common::PluginArgs>
          type{path, common::PluginSpace{path.parent(), path.child()}, args};

  common::Plugin plugin{type};
  plugin.unload();
}

//! \brief Convenience wrapper to unload a list of single plugins with arguments
//!
//! \ref unload(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param plugins List of single plugins
//!
//! \details
//!
//!   Example:
//!
//!     &emsp; root [0] dfi::plugin::unload({{"repo/example/example.cc", "using foo = int; foo f;"}, {"custom/example/example.cc", "using bar = char; bar b;"}})
//!
//!   Will unload:
//!
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//!   And pass plugin-implementation defined auto-unloader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void unload(
    const std::initializer_list<
        std::pair<common::PluginPath, common::PluginArgs>>& plugins)
{
  for (const auto& plugin : plugins)
    unload(plugin.first, plugin.second);
}

//! \brief Convenience wrapper to unload a single plugin with optional argument
//!
//! \ref unload(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \param arg Optional string argument to pass to plugin's auto-unloader (plugin-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::unload("repo/example/example.cc", "using foo = int; foo f;")
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using foo = int; foo f;` to the plugin's unloader (plugin-implementation defined).
//!
//!
//!   Example 2:
//!
//!     &emsp; root [0] dfi::plugin::unload("custom/example/example.cc", "using bar = char; bar b;")
//!
//!   Will unload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's unloader (plugin-implementation defined).
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void unload(const std::string& path, const std::string& arg = {})
{
  unload(common::PluginPath{path}, common::PluginArgs{"", arg});
}

//! \brief Convenience wrapper to unload a list of plugins with no arguments
//!
//! \ref unload(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::unload({"repo/example/example.cc", "custom/example/example.cc"})
//!
//!   Will unload both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc\n
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void unload(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    unload(path);
}

//
// Reloader
//

//! \brief Reloads a single plugin
//!
//! \param path common::PluginPath
//! \param args common::PluginArgs
//!
//! \ref dfi::plugin::common::Plugin
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::reload({"repo/example/example.cc"}, {"using foo = int; foo f;", "using bar = char; bar b;"})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's unloader (plugin-implementation defined)
//!   and pass `using foo = int; foo f;` to the plugin's loader (plugin-implementation defined)
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void reload(const common::PluginPath& path, const common::PluginArgs& args)
{
  ::dfi::common::type::
      Pluggable<common::PluginPath, common::PluginSpace, common::PluginArgs>
          type{path, common::PluginSpace{path.parent(), path.child()}, args};

  common::Plugin plugin{type};
  plugin.reload();
}

//! \brief Convenience wrapper to unload a list of single plugins with arguments
//!
//! \ref reload(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param plugins List of populated pair of common::PluginPath and common::PluginArgs
//!
//! \details
//!
//!   Example:
//!     &emsp; root [0] dfi::plugin::reload({{"repo/example/example.cc", "using foo = int; foo f;"}, {"custom/example/example.cc", "using bar = char; bar b;"}})
//!
//!   Will reload:
//!
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//!   And pass plugin-implementation defined auto-loader arguments:
//!
//!     &emsp; `using foo = int; foo f;` for `${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc`\n
//!     &emsp; `using bar = char; bar b;` for `${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc`
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void reload(
    const std::initializer_list<
        std::pair<common::PluginPath, common::PluginArgs>>& plugins)
{
  for (const auto& plugin : plugins)
    reload(plugin.first, plugin.second);
}

//! \brief Convenience wrapper to reload a single plugin with optional arguments
//!
//! \ref reload(const common::PluginPath& path, const common::PluginArgs& args)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \param args Optional pair of arguments to pass to plugin's auto-loader/unloader (plugin-implementation defined)
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::reload("repo/example/example.cc", {"using foo = int; foo f;", ""})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc
//!
//!   and pass `using foo = int; foo f;` to the plugin's auto-loader (plugin-implementation defined).
//!
//!
//!   Example 2:
//!
//!     &emsp; root [0] dfi::plugin::reload("custom/example/example.cc", {"", "using bar = char; bar b;"})
//!
//!   Will reload:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//!   and pass `using bar = char; bar b;` to the plugin's auto-unloader (plugin-implementation defined).
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void reload(
    const std::string& path,
    const std::pair<std::string, std::string>& args = {})
{
  reload(common::PluginPath{path}, common::PluginArgs{args.first, args.second});
}

//! \brief Convenience wrapper to reload a list of plugins with no arguments
//!
//! \ref reload(const std::string& path, const std::string& arg)
//!
//! \param path Must be of string "repo/<relative>" or "custom/<relative>" to plugin location, as described by common::PluginPath
//!
//! \details
//!
//!   Example 1:
//!
//!     &emsp; root [0] dfi::plugin::reload({"repo/example/example.cc", "custom/example/example.cc"})
//!
//!   Will reload both:
//!
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc\n
//!     &emsp; ${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/example/example.cc
//!
//! \warning
//!   To utilize plugin auto-(un)loader functionality, the plugin's parent directory *MUST* align with the plugin's namespace\n
//!   and the plugin's entrypoint class *MUST* align with the plugin's filename; e.g., "repo/example/example.cc" requires that\n
//!   the auto-(un)loader exist within the entrypoint class name `example_cc` within the namespace `dfi::plugin::example`.\n\n
//!   To avoid this requisite, though ill-advised, use a `dfi::common` file loader instead.
//!
//! \ingroup cpp_plugin
//! \since docker-finance 1.1.0
void reload(const std::initializer_list<std::string>& paths)
{
  for (const auto& path : paths)
    reload(path);
}
}  // namespace plugin
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_PLUGIN_COMMON_UTILITY_HH_

// # vim: sw=2 sts=2 si ai et
