// Quant trade framework plugin for docker-finance
//
// Loads the Quant equation engine (MultiHorizonEngine, MarketEntryCalculator,
// ProfitCalculator, ExitStrategyCalculator) into ROOT/Cling for interactive
// use inside docker-finance.
//
// Copyright (C) 2026 Patrick K.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation  either version 3 of the License  or
// (at your option) any later version.

//! \file
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.3.0

namespace dfi::plugin
{
namespace quant
{
//! \brief Get container-side path to quant plugin internals
//! \return Absolute path of quant plugin internal directory
inline std::string get_plugin_path()
{
  return ::dfi::common::get_plugin_path("repo/quant/internal/");
}

//! \brief Pluggable entrypoint
//! \ingroup cpp_plugin_impl
class quant_cc final
{
 public:
  quant_cc() = default;
  ~quant_cc() = default;

  quant_cc(const quant_cc&) = default;
  quant_cc& operator=(const quant_cc&) = default;

  quant_cc(quant_cc&&) = default;
  quant_cc& operator=(quant_cc&&) = default;

 public:
  //! \brief Load Quant equation engine into Cling
  //! \param arg Optional code to execute after loading
  static void load(const std::string& arg = {})
  {
    namespace common = ::dfi::common;

    const std::string path{get_plugin_path()};

    // Load equation headers in dependency order
    common::load(path + "Trade.h");
    common::load(path + "MultiHorizonEngine.h");
    common::load(path + "MarketEntryCalculator.h");
    common::load(path + "ProfitCalculator.h");
    common::load(path + "ExitStrategyCalculator.h");

    // Load interactive API (cycle, journal, overhead helpers)
    common::load(path + "quant.cc");

    if (!arg.empty())
      common::line(arg);
  }

  //! \brief Unload Quant equation engine from Cling
  //! \param arg Optional code to execute before unloading
  static void unload(const std::string& arg = {})
  {
    namespace common = ::dfi::common;

    if (!arg.empty())
      common::line(arg);

    const std::string path{get_plugin_path()};

    common::unload(path + "quant.cc");
    common::unload(path + "ExitStrategyCalculator.h");
    common::unload(path + "ProfitCalculator.h");
    common::unload(path + "MarketEntryCalculator.h");
    common::unload(path + "MultiHorizonEngine.h");
    common::unload(path + "Trade.h");
  }
};
}  // namespace quant
}  // namespace dfi::plugin

// # vim: sw=2 sts=2 si ai et
