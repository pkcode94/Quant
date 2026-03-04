// Quant plugin for docker-finance
//
// Provides the Quant trade-math engine as a ROOT/Cling plugin.
// Exposes serial plan generation, exit strategies, chain planning,
// profit calculation, DCA tracking, and overhead computation.
//
// No login, no premium, no database, no HTTP -- pure math only.
//
// Usage (from ROOT prompt):
//
//   root [0] dfi::plugin::reload("repo/quant/quant.cc")
//
//   root [1] namespace q = dfi::plugin::quant;
//
//   root [2] q::internal::SerialParams sp;
//            sp.current_price = 2650; sp.quantity = 1; sp.levels = 4;
//            sp.steepness = 6; sp.risk = 0.5; sp.available_funds = 1000;
//            sp.fee_spread = 0.001; sp.fee_hedging_coefficient = 2;
//            sp.delta_time = 1; sp.symbol_count = 1; sp.coefficient_k = 1;
//            sp.surplus_rate = 0.02; sp.max_risk = 0.05; sp.min_risk = 0.01;
//   root [3] auto plan = q::Quant::serial_plan(sp);
//   root [4] q::Quant::print_plan(plan);
//
//   root [5] auto pnl = q::Quant::profit(100.0, 110.0, 10.0, 0.5, 0.5);
//   root [6] q::Quant::print_profit(pnl);
//
//   root [7] auto dca = q::Quant::dca({{100, 2}, {95, 3}, {105, 1}});
//   root [8] q::Quant::print_dca(dca);

//! \file
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.1.0

//! \namespace dfi::plugin
//! \brief docker-finance plugins
//! \warning All plugins (repo/custom) must exist within this namespace
//!          and work within their own inner namespace
//! \since docker-finance 1.0.0
namespace dfi::plugin
{
//! \namespace dfi::plugin::quant
//! \brief Quant trade-math engine plugin
//!
//! \details Provides serial plan generation, exit strategies,
//!   chain planning, profit/DCA calculation, and overhead computation.
//!   No login, no premium, no database, no HTTP -- pure math only.
//!
//! \warning
//!   This namespace *MUST* match the parent plugin directory,
//!   e.g., `quant/quant.cc` must be `dfi::plugin::quant`
//!   in order to benefit from the plugin auto-loader.
//!   Otherwise, use general common file loader.
//!
//! \since docker-finance 1.1.0
namespace quant
{
//! \brief Pluggable entrypoint
//! \ingroup cpp_plugin_impl
//! \since docker-finance 1.1.0
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
  //! \brief Plugin auto-loader
  //! \param arg String to pass to auto-loader (plugin-implementation defined)
  static void load(const std::string& arg = {})
  {
    namespace plugin = ::dfi::plugin;
    namespace common = ::dfi::common;

    // Load the internal implementation
    plugin::common::PluginPath path("repo/quant/internal/quant.cc");
    common::load(path.absolute());

    if (!arg.empty())
      common::line(arg);
  }

  //! \brief Plugin auto-unloader
  //! \param arg String to pass to auto-unloader (plugin-implementation defined)
  static void unload(const std::string& arg = {})
  {
    namespace plugin = ::dfi::plugin;
    namespace common = ::dfi::common;

    if (!arg.empty())
      common::line(arg);

    // Unload the internal implementation
    plugin::common::PluginPath path("repo/quant/internal/quant.cc");
    common::unload(path.absolute());
  }
};
}  // namespace quant
}  // namespace dfi::plugin

// # vim: sw=2 sts=2 si ai et
