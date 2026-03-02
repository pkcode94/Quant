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

#ifndef CONTAINER_SRC_ROOT_MACRO_ROOTLOGON_C_
#define CONTAINER_SRC_ROOT_MACRO_ROOTLOGON_C_

#include <iostream>
#include <string>

#include "./common/utility.hh"  // The one-and-only `dfi` header required by rootlogon

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \brief `dfi` macro-specific usage help
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
void help()
{
  std::cout << "Use `dfi::help()` instead" << std::endl;
}
}  // namespace macro

//! \brief `dfi` generic help descriptions
//! \ingroup cpp_macro
//! \todo Factor out for specific help descriptions per-namespace
//!       (e.g., dfi::macro::help())
//! \since docker-finance 1.1.0
void help()
{
  const std::string help =
      "Description:\n"
      "\n"
      "  docker-finance C++ interpretations / interactive calculations\n"
      "\n"
      "  *** TIP: save your fingers! Use tab completion! ***\n"
      "\n"
      "  1. Print current directory (all commands/files are relative to\n"
      "     this directory):\n"
      "\n"
      "      root [0] .!pwd  // eg., "
      "${DOCKER_FINANCE_CONTAINER_REPO}/src/root\n"
      "\n"
      "Library:\n"
      "\n"
      "  1. Directly access docker-finance crypto abstractions and\n"
      "     print hundreds of SHA2-256 libsodium-generated hashes\n"
      "     of Crypto++-generated cryptographically secure random\n"
      "     numbers:\n"
      "\n"
      "      root [0] using g_Random = dfi::crypto::cryptopp::Random;\n"
      "      root [1] using g_Hash = dfi::crypto::libsodium::Hash;\n"
      "      root [2] g_Random r; g_Hash h;\n"
      "      root [3] for (size_t i{}; i < "
      "std::numeric_limits<uint8_t>::max(); i++) {\n"
      "      root (cont'ed, cancel with .@) [4] uint32_t num = r.generate();\n"
      "      root (cont'ed, cancel with .@) [5] std::cout << "
      "h.encode<g_Hash::SHA2_256>(num)\n"
      "      root (cont'ed, cancel with .@) [6] << \" = \" << num << \"\\n\";\n"
      "      root (cont'ed, cancel with .@) [7] }\n"
      "\n"
      "     Note: generate Doxygen to see all supported cryptographic\n"
      "     libraries and hash types.\n"
      "\n"
      "  2. Use Tools utility\n"
      "\n"
      "      root [0] dfi::utility::Tools tools;\n"
      "\n"
      "      // Create a variable within interpreter\n"
      "      root [1] btc=0.87654321+0.12345678+0.00000078\n"
      "      (double) 1.0000008\n"
      "\n"
      "      // Print variable up to N decimal places\n"
      "      root [2] tools.print_value<double>(btc, 8);\n"
      "      1.00000077\n"
      "\n"
      "      // Use tab autocomplete for class members to print random\n"
      "      // numbers within given intervals\n"
      "      root [3] "
      "tools.print_dist<TRandomGen<ROOT::Math::MixMaxEngine<240, 0> >, "
      "double>(0.1, btc);\n"
      "      0.13787670\n"
      "      0.36534066\n"
      "      0.33885582\n"
      "      0.15792758\n"
      "      Maximum = 1.00000077\n"
      "      Printed = 1.00000077\n"
      "\n"
      "Plugins:\n"
      "\n"
      "  WARNING: unlike macro loader, the prepended 'repo' and 'custom'\n"
      "  are not real directories but rather indications that the file\n"
      "  is either a repository plugin or an non-repo (custom) plugin.\n"
      "\n"
      "  1. Connect to docker-finance API via plugins:\n"
      "\n"
      "      // Load and run an example repository plugin\n"
      "      root [0] dfi::plugin::load(\"repo/example.cc\");\n"
      "      root [1] dfi::plugin::my_plugin_namespace::example1();\n"
      "      root [2] dfi::plugin::my_plugin_namespace::example2();\n"
      "      root [3] dfi::plugin::my_plugin_namespace::example3();\n"
      "\n"
      "      // Load and run your own custom plugin\n"
      "      root [4] dfi::plugin::load(\"custom/example.cc\");\n"
      "      root [5] dfi::plugin::your_plugin_namespace::example();\n"
      "\n"
      "Macros:\n"
      "\n"
      "  NOTE: the macro loader interprets from base 'macro' path (.!pwd)\n"
      "\n"
      "  1. Load and run docker-finance unit tests and benchmarks:\n"
      "\n"
      "      root [0] dfi::macro::load(\"macro/test/unit.C\")\n"
      "      root [1] dfi::macro::test::Unit::run()\n"
      "      ...\n"
      "      root [2] dfi::macro::load(\"macro/test/benchmark.C\")\n"
      "      root [3] dfi::macro::test::Benchmark::run()\n"
      "      ...\n"
      "\n"
      "  2. Load and run docker-finance cryptographic macros:\n"
      "\n"
      "      root [0] dfi::macro::load(\"macro/crypto/hash.C\")\n"
      "      root [1] dfi::macro::crypto::Hash::run(\"better to be raw than "
      "digested\")\n"
      "      ...\n"
      "      root [2] dfi::macro::load(\"macro/crypto/random.C\")\n"
      "      root [3] dfi::macro::crypto::Random::run()\n"
      "      ...\n"
      "\n"
      "  3. Load ROOT webserver and run commands for metadata analysis:\n"
      "\n"
      "      root [0] dfi::macro::load(\"macro/web/server.C\")\n"
      "      root [1] dfi::macro::web::Server::run()\n"
      "\n"
      "     Now, open your web browser to http://127.0.0.1:8080\n"
      "\n"
      "Help:\n"
      "\n"
      "  1. Print all help options\n"
      "\n"
      "      root [0] help()\n";
  // TODO(unassigned): add multi-threading example of generating numbers and/or hashes

  std::cout << help << std::endl;
}
}  // namespace dfi

//! \brief Print all help options
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
void help()
{
  const std::string help =
      "Help:\n"
      "\n"
      "  1. Print this help\n"
      "      root [0] help()\n"
      "\n"
      "  2. Print `root` help\n"
      "      root [0] .help\n"
      "\n"
      "  3. Print `dfi` help\n"
      "      root [0] dfi::help()\n";

  std::cout << help << std::endl;
}

//! \brief ROOT logon macro, runs on `root` startup
//! \since docker-finance 1.0.0
void rootlogon()
{
  namespace common = dfi::common;

  // Add nested directory headers
  common::add_include_dir("/usr/include/botan-3");

  // Link default packaged libraries
  // TODO(afiore): linking here appears to no longer be needed?
  //   Is ROOT loading all libraries on-the-fly (in default paths)?
  common::add_linked_lib("/usr/lib/libgtest.so");  // gtest/gmock
  common::add_linked_lib("/usr/lib/libbenchmark.so");  // gbenchmark
  common::add_linked_lib("/usr/lib/libpthread.so.0");  // gtest/gmock/gbenchmark

  common::add_linked_lib("/usr/lib/libbotan-3.so");  // Botan
  common::add_linked_lib("/usr/lib/libcryptopp.so");  // Crypto++
  common::add_linked_lib("/usr/lib/libsodium.so");  // libsodium

  // Load default `dfi` public consumables
  common::load("plugin/common/utility.hh");
  common::load("macro/common/utility.hh");
}

#endif  // CONTAINER_SRC_ROOT_MACRO_ROOTLOGON_C_

// # vim: sw=2 sts=2 si ai et
