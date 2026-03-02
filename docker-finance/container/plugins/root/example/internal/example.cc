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

#include "./example.hh"

#include <limits>

namespace dfi::plugin::example
{
std::string MyExamples::make_cmd(
    const std::string& base,
    const std::string& arg)
{
  return std::string{base + arg};
}

void MyExamples::exec_cmd(const std::string& cmd)
{
  namespace common = ::dfi::common;

  const std::string base{
      common::get_env("global_basename") + " "
      + common::get_env("global_parent_profile")
      + common::get_env("global_arg_delim_1")
      + common::get_env("global_child_profile") + " "};

  common::throw_ex_if<common::type::RuntimeError>(
      common::exec("bash -i -c '" + MyExamples::make_cmd(base, cmd) + "'"),
      "command failed");
}

void MyExamples::print_env(const std::string& env)
{
  std::cout << env << "=" << ::dfi::common::get_env(env) << "\n";
};

void MyExamples::example1()
{
  using g_Random = dfi::crypto::cryptopp::Random;
  using g_Hash = dfi::crypto::libsodium::Hash;
  g_Random r;
  g_Hash h;
  for (size_t i{}; i < 5; i++)
    {
      uint32_t num = r.generate();
      std::cout << h.encode<g_Hash::SHA2_256>(num) << " = " << num << "\n";
    }
}

void MyExamples::example2()
{
  // Container environment
  std::cout
      << "\nShell environment (container)\n-----------------------------\n";
  MyExamples::print_env("DOCKER_FINANCE_CLIENT_FLOW");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_CMD");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_CONF");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_EDITOR");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_FLOW");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_PLUGINS");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_REPO");
  MyExamples::print_env("DOCKER_FINANCE_CONTAINER_SHARED");
  MyExamples::print_env("DOCKER_FINANCE_DEBUG");
  MyExamples::print_env("DOCKER_FINANCE_VERSION");

  std::cout << "\nSame as previous but executing commands in "
               "shell\n------------------------------------------------\n";
  ::dfi::common::exec("printenv | grep ^DOCKER_FINANCE | sort");

  // Caller environment
  std::cout << "\nShell environment (caller)\n--------------------------\n";
  MyExamples::print_env("global_arg_delim_1");
  MyExamples::print_env("global_arg_delim_2");
  MyExamples::print_env("global_arg_delim_3");
  MyExamples::print_env("global_arg_subcommand");
  MyExamples::print_env("global_basename");
  MyExamples::print_env("global_child_profile");
  MyExamples::print_env("global_child_profile_flow");
  MyExamples::print_env("global_child_profile_journal");
  MyExamples::print_env("global_conf_fetch");
  MyExamples::print_env("global_conf_hledger");
  MyExamples::print_env("global_conf_meta");
  MyExamples::print_env("global_conf_subscript");
  // TODO(unassigned): read array from shell? ROOT impl simply calls stdlib getenv()
  // MyExamples::print_env("global_hledger_cmd");
  MyExamples::print_env("global_parent_profile");
  MyExamples::print_env("global_usage");
}

void MyExamples::example3()
{
  std::cout << "\nImporting journals...\n";
  MyExamples::exec_cmd("import 1>/dev/null");

  std::cout << "\nShowing BTC balance...\n";
  MyExamples::exec_cmd("hledger bal assets liabilities cur:BTC");

  std::cout << "\nGenerating income tax snippet...\n";
  const std::string delim{::dfi::common::get_env("global_arg_delim_2")};
  MyExamples::exec_cmd(
      "taxes all" + delim + "account tag" + delim + "income write" + delim
      + "off | tail -n3");
}
}  // namespace dfi::plugin::example

// # vim: sw=2 sts=2 si ai et
