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

#ifndef CONTAINER_SRC_ROOT_MACRO_WEB_SERVER_C_
#define CONTAINER_SRC_ROOT_MACRO_WEB_SERVER_C_

#include "../common/server.hh"
#include "../common/utility.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \namespace dfi::macro::web
//! \brief ROOT web-based macros
//! \since docker-finance 1.0.0
namespace web
{
//! \brief Web server managing class
//! \ingroup cpp_macro
//! \since docker-finance 1.0.0
class Server final
{
 public:
  Server() = default;
  ~Server() = default;

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  Server(Server&&) = default;
  Server& operator=(Server&&) = default;

 private:
  //! \brief ROOT HTTP server command registrar
  //! \details Registers internal macros
  static void register_commands()
  {
    ::dfi::common::load("macro/web/internal/crypto.C");
    ::dfi::macro::common::g_HTTPServer->RegisterCommand(
        "/rng_sample",
        "::dfi::macro::web::internal::Random::rng_sample(\"%arg1%"
        "\")");

    ::dfi::common::load("macro/web/internal/meta.C");
    ::dfi::macro::common::g_HTTPServer->RegisterCommand(
        "/meta_sample",
        "::dfi::macro::web::internal::Meta::meta_sample(\"%arg1%"
        "\")");
  }

 public:
  //! \brief Start webserver
  static void run() { Server::register_commands(); }
};

//! \brief Pluggable entrypoint
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.1.0
class server_C final
{
 public:
  server_C() = default;
  ~server_C() = default;

  server_C(const server_C&) = default;
  server_C& operator=(const server_C&) = default;

  server_C(server_C&&) = default;
  server_C& operator=(server_C&&) = default;

 public:
  //! \brief Macro auto-loader
  //! \param arg Optional code to execute after loading
  static void load(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);

    Server::run();
  }

  //! \brief Macro auto-unloader
  //! \param arg Optional code to execute before unloading
  static void unload(const std::string& arg = {})
  {
    if (!arg.empty())
      ::dfi::common::line(arg);
  }
};
}  // namespace web
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_WEB_SERVER_C_

// # vim: sw=2 sts=2 si ai et
