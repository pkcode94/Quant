// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2024-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation  either version 3 of the License  or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not  see <https://www.gnu.org/licenses/>.

//! \file
//! \author Aaron Fiore (Founder, Evergreen Crypto LLC)
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.0.0

//! \namespace dfi::plugin
//! \brief docker-finance plugins
//! \warning All plugins (repo/custom) must exist within this namespace
//!          and work within their own inner namespace
//! \since docker-finance 1.0.0
namespace dfi::plugin
{
//! \namespace dfi::plugin::example
//! \brief docker-finance plugin example namespace
//! \details An example namespace used only within this example file
//!
//! \warning
//!   This namespace *MUST* match the parent plugin directory,
//!   e.g., `example/anyfile.cc` must be `dfi::plugin::example`
//!   in order to benefit from the plugin auto-loader.
//!   Otherwise, use general common file loader.
//!
//! \note Not a real namespace - for example purposes only
//! \since docker-finance 1.1.0
namespace example
{
//! \brief Pluggable entrypoint
//! \ingroup cpp_plugin_impl
//! \since docker-finance 1.1.0
class example_cc final
{
 public:
  example_cc() = default;
  ~example_cc() = default;

  example_cc(const example_cc&) = default;
  example_cc& operator=(const example_cc&) = default;

  example_cc(example_cc&&) = default;
  example_cc& operator=(example_cc&&) = default;

 public:
  //! \brief Example plugin's auto-loader
  //! \param arg String to pass to auto-loader (use-case is plugin-implementation defined)
  static void load(const std::string& arg = {})
  {
    namespace plugin = ::dfi::plugin;
    namespace common = ::dfi::common;

    // Any code can follow here: run example functions, etc.
    //
    // For this example:
    //
    //   1. Load needed requirements for this plugin's impl
    //   2. Get absolute path of plugin's impl and then load
    //
    const std::string src{common::get_root_path() + "src/"};
    common::load(src + "hash.hh");
    common::load(src + "random.hh");
    // ...add as needed

    plugin::common::PluginPath path("repo/example/internal/example.cc");
    common::load(path.absolute());

    // For this example, expect and pass a line of code to the interpreter after loading
    // NOTE: the following arg code can utilize any code that was loaded/interpreted in load()
    if (!arg.empty())
      common::line(arg);
  }

  //! \brief Example plugin's auto-unloader
  //! \param arg String to pass to auto-unloader (use-case is plugin-implementation defined)
  static void unload(const std::string& arg = {})
  {
    namespace plugin = ::dfi::plugin;
    namespace common = ::dfi::common;

    // For this example, expect and pass a line of code to the interpreter before unloading.
    // NOTE: the following arg code can utilize any code that was previously loaded/interpreted
    if (!arg.empty())
      common::line(arg);

    // Any code can follow here: run example functions, etc.
    //
    // For this example:
    //
    //   1. Unload needed requirements for this plugin's impl
    //   2. Get absolute path of plugin's impl and then unload
    //
    const std::string src{common::get_root_path() + "src/"};
    common::unload(src + "hash.hh");
    common::unload(src + "random.hh");
    // ...add as needed

    plugin::common::PluginPath path("repo/example/internal/example.cc");
    common::unload(path.absolute());
  }
  // NOTE: to auto-reload this plugin, load()/unload() here or utilize one of the `dfi::plugin::reload()` functions.
};
}  // namespace example
}  // namespace dfi::plugin

// # vim: sw=2 sts=2 si ai et
