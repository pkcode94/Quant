// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2025-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
//! \since docker-finance 1.1.0

//! \namespace dfi::plugin
//! \brief docker-finance plugins
//! \warning All plugins (repo/custom) must exist within this namespace
//!   and work within their own inner namespace
//! \since docker-finance 1.0.0
namespace dfi::plugin
{
//! \namespace dfi::plugin::bitcoin
//! \brief docker-finance bitcoin plugin namespace
//! \warning Bitcoin inherently pollutes the global namespace - be wary
//! \since docker-finance 1.1.0
namespace bitcoin
{
//! \brief Get container-side path to bitcoin repository
//! \return Absolute path of shared (bind-mounted) bitcoin repository
//! \todo To avoid future include conflicts, consider adjusting base path
//! \since docker-finance 1.1.0
inline std::string get_repo_path()
{
  return ::dfi::common::get_env("DOCKER_FINANCE_CONTAINER_SHARED")
         + "/bitcoin/";
}

//! \brief Pluggable entrypoint
//! \ingroup cpp_plugin_impl
//! \since docker-finance 1.1.0
class bitcoin_cc final
{
 public:
  bitcoin_cc() = default;
  ~bitcoin_cc() = default;

  bitcoin_cc(const bitcoin_cc&) = default;
  bitcoin_cc& operator=(const bitcoin_cc&) = default;

  bitcoin_cc(bitcoin_cc&&) = default;
  bitcoin_cc& operator=(bitcoin_cc&&) = default;

 public:
  //! \brief Initialize this bitcoin plugin
  //! \details Bare-essential initializations, will require manual loading for any further functionality
  //! \warning Initializations *MUST* occur here before any other bitcoin lib use
  static void load(const std::string& arg = {})
  {
    namespace common = ::dfi::common;

    const std::string repo{get_repo_path()};

    // Minimum requirements
    common::add_include_dir(repo + "src");
    common::add_linked_lib(repo + "build/lib/libbitcoinkernel.so");
    // WARNING: assertions *MUST* be enabled before loading any bitcoin headers
    common::line("#undef NDEBUG");
    common::load(repo + "src/kernel/bitcoinkernel_wrapper.h");

    // `dfi`-specific requirements (API, macros, tests, etc.)
    common::line("#define __DFI_PLUGIN_BITCOIN__");
    common::load(repo + "src/random.h");
    // ...add as needed

    if (!arg.empty())
      common::line(arg);
  }

  //! \brief Deinitialize this bitcoin plugin
  //! \details Bare-essential deinitializations, will require manual unloading for any other previously loaded functionality
  //! \warning Unloading certain bitcoin headers may result in Cling segfault due to bitcoin library design
  //! \todo It appears that, due to bitcoin design, unloading will result in
  //!   Cling segfault "*** Break *** illegal instruction". With that said, it's
  //!   best to exit `root` if you want a clean workspace (instead of unloading this plugin).
  static void unload(const std::string& arg = {})
  {
    namespace common = ::dfi::common;

    if (!arg.empty())
      common::line(arg);

    const std::string repo{get_repo_path()};

    // `dfi`-specific requirements (API, macros, tests, etc.)
    common::line("#undef __DFI_PLUGIN_BITCOIN__");
    common::unload(repo + "src/random.h");
    // ...add as needed

    // Minimum requirements
    common::unload(repo + "src/kernel/bitcoinkernel_wrapper.h");
    common::line("#define NDEBUG");
    // TODO(unassigned): remove_include_dir()
    common::remove_linked_lib(repo + "build/lib/libbitcoinkernel.so");
  }
};
}  // namespace bitcoin
}  // namespace dfi::plugin

// # vim: sw=2 sts=2 si ai et
