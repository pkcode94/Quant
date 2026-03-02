// Quant interactive API for docker-finance ROOT/Cling
//
// Provides functions to run Quant equations interactively and
// generate hledger journal output from within the ROOT interpreter.
// Also provides THttpServer command registration for the web UI (port 8080).

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

#include <TCanvas.h>
#include <TGraph.h>
#include <TAxis.h>
#include <TStyle.h>

namespace dfi::plugin::quant
{
//! \brief Get today's date as YYYY-MM-DD
inline std::string today()
{
  std::time_t now = std::time(nullptr);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&now));
  return buf;
}

//! \brief Run a full cycle simulation and print results
//! \param currentPrice Current market price
//! \param levels Number of entry levels
//! \param steepness Sigmoid steepness
//! \param risk Risk coefficient (0=conservative, 0.5=uniform, 1=aggressive)
//! \param totalFunds Total capital to deploy
//! \param surplusRate Surplus rate (e.g. 0.02 = 2%)
//! \param feeSpread Fee spread rate (e.g. 0.001 = 0.1%)
//! \param feeHedge Fee hedging coefficient
//! \param savingsRate Fraction of profit to extract (e.g. 0.10 = 10%)
inline void cycle(double currentPrice, int levels, double steepness,
                  double risk, double totalFunds, double surplusRate,
                  double feeSpread, double feeHedge,
                  double savingsRate = 0.10,
                  double rangeBelow = 0.0, double rangeAbove = 0.0)
{
  HorizonParams p;
  p.horizonCount = levels;
  p.feeHedgingCoefficient = feeHedge;
  p.portfolioPump = totalFunds;
  p.symbolCount = 1;
  p.feeSpread = feeSpread;
  p.deltaTime = 1.0;
  p.surplusRate = surplusRate;

  double oh = MultiHorizonEngine::computeOverhead(currentPrice, 1.0, p);
  double eo = MultiHorizonEngine::effectiveOverhead(currentPrice, 1.0, p);

  auto entries = MarketEntryCalculator::generate(
      currentPrice, 1.0, p, risk, steepness, rangeAbove, rangeBelow);

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "=== Quant Cycle Simulation ===\n"
            << "  Price: $" << currentPrice << "  Levels: " << levels
            << "  Funds: $" << totalFunds << "\n"
            << "  Surplus: " << (surplusRate * 100) << "%  Fee: "
            << (feeSpread * 100) << "%  Hedge: " << feeHedge << "\n"
            << "  OH=" << (oh * 100) << "%  EO=" << (eo * 100) << "%\n\n";

  std::cout << "--- Entry Levels ---\n";
  std::cout << std::setw(6) << "Level"
            << std::setw(16) << "Entry"
            << std::setw(16) << "BreakEven"
            << std::setw(12) << "Fund%"
            << std::setw(16) << "Funding"
            << std::setw(14) << "Qty" << "\n";

  double totalProfit = 0;
  for (const auto& e : entries)
  {
    double tp = e.entryPrice * (1.0 + eo + surplusRate);
    double profit = (tp - e.entryPrice) * e.fundingQty;
    totalProfit += profit;

    std::cout << std::setw(6) << e.index
              << std::setw(16) << e.entryPrice
              << std::setw(16) << e.breakEven
              << std::setw(12) << (e.fundingFraction * 100)
              << std::setw(16) << e.funding
              << std::setw(14) << e.fundingQty << "\n";
  }

  double savings = totalProfit * savingsRate;
  double reinvest = totalProfit - savings;

  std::cout << "\n--- Cycle Result ---\n"
            << "  Gross cycle profit: $" << totalProfit << "\n"
            << "  Savings extracted:  $" << savings
            << " (" << (savingsRate * 100) << "%)\n"
            << "  Reinvested:         $" << reinvest << "\n"
            << "  Next cycle pump:    $" << (totalFunds + reinvest) << "\n";
}

//! \brief Generate hledger journal and write to file
//! \param symbol Trading symbol (e.g. "BTC")
//! \param journalPath Path to append journal entries (e.g. ~/.hledger.journal)
//! \param currentPrice Current market price
//! \param levels Number of entry levels
//! \param steepness Sigmoid steepness
//! \param risk Risk coefficient
//! \param totalFunds Total capital
//! \param surplusRate Surplus rate
//! \param feeSpread Fee spread
//! \param feeHedge Fee hedging coefficient
//! \param savingsRate Savings extraction rate
inline void journal(const std::string& symbol, const std::string& journalPath,
                    double currentPrice, int levels, double steepness,
                    double risk, double totalFunds, double surplusRate,
                    double feeSpread, double feeHedge,
                    double savingsRate = 0.10)
{
  HorizonParams p;
  p.horizonCount = levels;
  p.feeHedgingCoefficient = feeHedge;
  p.portfolioPump = totalFunds;
  p.symbolCount = 1;
  p.feeSpread = feeSpread;
  p.deltaTime = 1.0;
  p.surplusRate = surplusRate;

  double eo = MultiHorizonEngine::effectiveOverhead(currentPrice, 1.0, p);

  auto entries = MarketEntryCalculator::generate(
      currentPrice, 1.0, p, risk, steepness);

  std::string symLower = symbol;
  for (auto& c : symLower)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  std::string date = today();
  std::ostringstream j;
  j << std::fixed << std::setprecision(8);
  j << "\n; Quant cycle \xe2\x80\x94 " << symbol
    << " \xe2\x80\x94 generated " << date << "\n\n";

  double totalCost = 0, totalRevenue = 0, totalFees = 0;

  for (const auto& e : entries)
  {
    double fee = e.funding * feeSpread;
    totalCost += e.funding;
    totalFees += fee;
    j << date << " Buy " << symbol << " level " << e.index << "\n"
      << "    assets:crypto:" << symLower << "    "
      << e.fundingQty << " " << symbol << " @ $" << e.entryPrice << "\n"
      << "    expenses:fees:exchange       $" << fee << "\n"
      << "    assets:bank:trading         $-" << (e.funding + fee) << "\n\n";
  }

  for (const auto& e : entries)
  {
    double tp = e.entryPrice * (1.0 + eo + surplusRate);
    double revenue = tp * e.fundingQty;
    double fee = revenue * feeSpread;
    totalRevenue += revenue;
    totalFees += fee;
    j << date << " Sell " << symbol << " TP level " << e.index << "\n"
      << "    assets:bank:trading          $" << (revenue - fee) << "\n"
      << "    expenses:fees:exchange       $" << fee << "\n"
      << "    assets:crypto:" << symLower << "   -"
      << e.fundingQty << " " << symbol << " @ $" << tp << "\n\n";
  }

  double gross = totalRevenue - totalCost - totalFees;
  double savings = gross * savingsRate;
  if (savings > 0)
  {
    j << date << " Quant savings extraction\n"
      << "    assets:savings:quant         $" << savings << "\n"
      << "    income:trading:quant        $-" << savings << "\n\n";
  }

  j << "; Total fees: $" << totalFees << "\n"
    << "; Net profit: $" << gross << "\n"
    << "; Savings:    $" << savings << "\n";

  // Append to journal file
  std::ofstream out(journalPath, std::ios::app);
  if (!out.is_open())
  {
    std::cerr << "ERROR: Cannot open " << journalPath << "\n";
    std::cout << j.str();
    return;
  }
  out << j.str();
  out.close();
  std::cout << "Appended " << entries.size() << " buy + "
            << entries.size() << " sell entries to " << journalPath << "\n"
            << "Net profit: $" << gross << "  Savings: $" << savings << "\n";
}

//! \brief Quick overhead calculation
inline void overhead(double price, double qty, double feeSpread,
                     double feeHedge, double pump)
{
  HorizonParams p;
  p.feeSpread = feeSpread;
  p.feeHedgingCoefficient = feeHedge;
  p.portfolioPump = pump;
  p.symbolCount = 1;
  p.deltaTime = 1.0;

  double oh = MultiHorizonEngine::computeOverhead(price, qty, p);
  double eo = MultiHorizonEngine::effectiveOverhead(price, qty, p);

  std::cout << std::fixed << std::setprecision(8)
            << "  OH = " << oh << " (" << (oh * 100) << "%)\n"
            << "  EO = " << eo << " (" << (eo * 100) << "%)\n";
}

// ---- THttpServer web commands (port 8080) ----

//! \brief Web command: plot sigmoid entry levels on a TCanvas
//! \param args Format: "price,levels,steepness,risk,funds,surplus,feeSpread,feeHedge"
inline void web_entries(const std::string& args = "100000,10,6,0.5,5000,0.02,0.001,1.5")
{
  // Parse CSV args
  std::istringstream ss(args);
  std::string tok;
  std::vector<double> v;
  while (std::getline(ss, tok, ','))
  {
    try { v.push_back(std::stod(tok)); }
    catch (...) { v.push_back(0); }
  }

  double price     = v.size() > 0 ? v[0] : 100000;
  int    levels    = v.size() > 1 ? static_cast<int>(v[1]) : 10;
  double steep     = v.size() > 2 ? v[2] : 6.0;
  double risk      = v.size() > 3 ? v[3] : 0.5;
  double funds     = v.size() > 4 ? v[4] : 5000;
  double surplus   = v.size() > 5 ? v[5] : 0.02;
  double feeSpread = v.size() > 6 ? v[6] : 0.001;
  double feeHedge  = v.size() > 7 ? v[7] : 1.5;

  HorizonParams p;
  p.horizonCount = levels;
  p.feeHedgingCoefficient = feeHedge;
  p.portfolioPump = funds;
  p.symbolCount = 1;
  p.feeSpread = feeSpread;
  p.deltaTime = 1.0;
  p.surplusRate = surplus;

  double eo = MultiHorizonEngine::effectiveOverhead(price, 1.0, p);
  auto entries = MarketEntryCalculator::generate(price, 1.0, p, risk, steep);

  int N = static_cast<int>(entries.size());
  std::vector<Double_t> xEntry(N), yEntry(N), xTP(N), yTP(N), xFund(N), yFund(N);

  for (int i = 0; i < N; ++i)
  {
    xEntry[i] = i;
    yEntry[i] = entries[i].entryPrice;
    xTP[i]    = i;
    yTP[i]    = entries[i].entryPrice * (1.0 + eo + surplus);
    xFund[i]  = i;
    yFund[i]  = entries[i].funding;
  }

  auto* c = new TCanvas("quant_entries", "Quant Entry Levels", 900, 600);
  c->Divide(1, 2);

  // Top: entry prices + TP targets
  c->cd(1);
  auto* grEntry = new TGraph(N, xEntry.data(), yEntry.data());
  grEntry->SetTitle(("Entry + TP Levels (price=" + std::to_string(price)
                      + " risk=" + std::to_string(risk) + ")").c_str());
  grEntry->SetMarkerColor(kBlue);
  grEntry->SetMarkerStyle(kFullCircle);
  grEntry->SetLineColor(kBlue);
  grEntry->GetXaxis()->SetTitle("Level");
  grEntry->GetYaxis()->SetTitle("Price");
  grEntry->Draw("ALP");

  auto* grTP = new TGraph(N, xTP.data(), yTP.data());
  grTP->SetMarkerColor(kGreen + 2);
  grTP->SetMarkerStyle(kFullTriangleUp);
  grTP->SetLineColor(kGreen + 2);
  grTP->SetLineStyle(2);
  grTP->Draw("LP SAME");

  // Bottom: funding allocation
  c->cd(2);
  auto* grFund = new TGraph(N, xFund.data(), yFund.data());
  grFund->SetTitle(("Funding Allocation (total=$" + std::to_string(funds) + ")").c_str());
  grFund->SetMarkerColor(kOrange + 1);
  grFund->SetMarkerStyle(kFullSquare);
  grFund->SetLineColor(kOrange + 1);
  grFund->SetFillColor(kOrange - 9);
  grFund->GetXaxis()->SetTitle("Level");
  grFund->GetYaxis()->SetTitle("Funding ($)");
  grFund->Draw("ALB");

  c->Update();

  // Also print summary to stdout
  std::cout << std::fixed << std::setprecision(4)
            << "Plotted " << N << " levels | OH=" << (eo * 100) << "% | "
            << "Entries: $" << yEntry[0] << " .. $" << yEntry[N - 1] << "\n";
}

//! \brief Register quant commands with the running THttpServer
//! \note Must be called AFTER starting the web server macro
//! \example
//!   dfi <profile> root macros/repo/web/server.C
//!   g_plugin_quant_cc.load()
//!   dfi::plugin::quant::register_web()
inline void register_web()
{
  if (!::dfi::macro::common::g_HTTPServer)
  {
    std::cerr << "ERROR: THttpServer not running. Start web server first:\n"
              << "  dfi <profile> root macros/repo/web/server.C\n";
    return;
  }

  ::dfi::macro::common::g_HTTPServer->RegisterCommand(
      "/quant/entries",
      "::dfi::plugin::quant::web_entries(\"%arg1%\")");

  std::cout << "Quant web commands registered on port 8080:\n"
            << "  /quant/entries?arg1=price,levels,steepness,risk,funds,surplus,feeSpread,feeHedge\n"
            << "\n"
            << "Example:\n"
            << "  http://localhost:8080/quant/entries?arg1=100000,10,6,0.5,5000,0.02,0.001,1.5\n";
}

}  // namespace dfi::plugin::quant

// # vim: sw=2 sts=2 si ai et
