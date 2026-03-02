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
//! \since docker-finance 1.1.0

#ifndef CONTAINER_PLUGINS_ROOT_EXAMPLE_INTERNAL_EXAMPLE_HH_
#define CONTAINER_PLUGINS_ROOT_EXAMPLE_INTERNAL_EXAMPLE_HH_

#include <string>

namespace dfi::plugin::example
{
//! \brief An example plugin implementation class
//!
//! \warning When implementing pluggables, prefer member functions with
//!   definitions within source file(s) over certain lambdas to prevent
//!   root segfaults. TODO(afiore): investigate.
//!
//! \note For demonstration purposes only
//!
//! \ingroup cpp_plugin_impl
//! \since docker-finance 1.1.0
class MyExamples
{
 public:
  //! \brief Example plugin function 1
  //! \details Directly access docker-finance crypto abstractions and
  //!   print a handful of SHA2-256 libsodium-generated hashes of
  //!   Crypto++-generated cryptographically secure random numbers
  void example1();

  //! \brief Example plugin function 2
  //! \details Directly access underlying docker-finance shell environment
  void example2();

  //! \brief Example plugin function 3
  //! \details Execute docker-finance shell commands
  void example3();

 protected:
  std::string make_cmd(const std::string& base, const std::string& arg);
  void exec_cmd(const std::string& cmd);
  void print_env(const std::string& env);
};
}  // namespace dfi::plugin::example

#endif  // CONTAINER_PLUGINS_ROOT_EXAMPLE_INTERNAL_EXAMPLE_HH_

// # vim: sw=2 sts=2 si ai et
