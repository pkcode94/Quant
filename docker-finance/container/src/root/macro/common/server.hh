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

#ifndef CONTAINER_SRC_ROOT_MACRO_COMMON_SERVER_HH_
#define CONTAINER_SRC_ROOT_MACRO_COMMON_SERVER_HH_

#include <THttpServer.h>

#include <memory>

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
//! \brief HTTP Server instance
//! \note In namespace scope because of ROOT's static functions requirement
//! \since docker-finance 1.0.0
auto g_HTTPServer = std::make_unique<::THttpServer>("http:8080");
}  // namespace common
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_COMMON_SERVER_HH_

// # vim: sw=2 sts=2 si ai et
