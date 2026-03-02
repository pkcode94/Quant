#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <mutex>
#include <algorithm>
#include <set>

inline void registerSymbolRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /symbols ==========
    svr.Get("/symbols", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>Symbol Manager</h1>";

        // Gather all unique symbols from trades, entry points, exit points
        auto trades = db.loadTrades();
        auto entryPts = db.loadEntryPoints();
        auto exitPts = db.loadExitPoints();

        std::set<std::string> symSet;
        for (const auto& t : trades) symSet.insert(t.symbol);
        for (const auto& ep : entryPts) symSet.insert(ep.symbol);
        for (const auto& xp : exitPts) symSet.insert(xp.symbol);

        if (symSet.empty())
        {
            h << "<p class='empty'>(no symbols)</p>";
        }
        else
        {
            for (const auto& sym : symSet)
            {
                h << "<h2 style='color:#c9a44a;'>" << html::esc(sym) << "</h2>";

                // Trades summary
                int buyCount = 0, sellCount = 0;
                double totalQty = 0, totalSold = 0;
                for (const auto& t : trades)
                {
                    if (t.symbol != sym) continue;
                    if (t.type == TradeType::Buy)
                    {
                        ++buyCount;
                        totalQty += t.quantity;
                        totalSold += db.soldQuantityForParent(t.tradeId);
                    }
                    else ++sellCount;
                }
                double remaining = totalQty - totalSold;

                h << "<div class='row'>"
                     "<div class='stat'><div class='lbl'>Buys</div><div class='val'>" << buyCount << "</div></div>"
                     "<div class='stat'><div class='lbl'>Sells</div><div class='val'>" << sellCount << "</div></div>"
                     "<div class='stat'><div class='lbl'>Total Qty</div><div class='val'>" << totalQty << "</div></div>"
                     "<div class='stat'><div class='lbl'>Remaining</div><div class='val'>" << remaining << "</div></div>"
                     "</div>";

                // Entry points for this symbol
                int epPending = 0, epTraded = 0;
                for (const auto& ep : entryPts)
                {
                    if (ep.symbol != sym) continue;
                    if (ep.traded) ++epTraded; else ++epPending;
                }
                if (epPending > 0 || epTraded > 0)
                {
                    h << "<h4 style='color:#7b97c4;'>Entry Points</h4>";
                    h << "<table><tr><th>ID</th><th>Lvl</th><th>Entry</th><th>Qty</th><th>TP</th><th>SL</th><th>Status</th></tr>";
                    for (const auto& ep : entryPts)
                    {
                        if (ep.symbol != sym) continue;
                        std::string st = ep.traded ? "TRADED" : "PENDING";
                        std::string cl = ep.traded ? "off" : "buy";
                        h << "<tr><td>" << ep.entryId << "</td><td>" << ep.levelIndex << "</td>"
                          << "<td>" << ep.entryPrice << "</td><td>" << ep.fundingQty << "</td>"
                          << "<td>" << ep.exitTakeProfit << "</td><td>" << ep.exitStopLoss << "</td>"
                          << "<td class='" << cl << "'>" << st << "</td></tr>";
                    }
                    h << "</table>";
                }

                // Exit points for this symbol
                int xpPending = 0, xpDone = 0;
                for (const auto& xp : exitPts)
                {
                    if (xp.symbol != sym) continue;
                    if (xp.executed) ++xpDone; else ++xpPending;
                }
                if (xpPending > 0 || xpDone > 0)
                {
                    h << "<h4 style='color:#7b97c4;'>Exit Points</h4>";
                    h << "<table><tr><th>ID</th><th>Trade</th><th>Lvl</th><th>TP</th><th>SL</th><th>Qty</th><th>SL?</th><th>Status</th></tr>";
                    for (const auto& xp : exitPts)
                    {
                        if (xp.symbol != sym) continue;
                        std::string st = xp.executed ? "DONE" : "PENDING";
                        std::string cl = xp.executed ? "off" : "buy";
                        h << "<tr><td>" << xp.exitId << "</td><td>" << xp.tradeId << "</td>"
                          << "<td>" << xp.levelIndex << "</td><td>" << xp.tpPrice << "</td>"
                          << "<td>" << xp.slPrice << "</td><td>" << xp.sellQty << "</td>"
                          << "<td>" << (xp.slActive ? "ON" : "OFF") << "</td>"
                          << "<td class='" << cl << "'>" << st << "</td></tr>";
                    }
                    h << "</table>";
                }

                // Latest price
                double latest = ctx.prices.latest(sym);
                if (latest > 0)
                    h << "<div class='row'><div class='stat'><div class='lbl'>Last Price</div><div class='val'>" << latest << "</div></div></div>";
            }
        }

        h << "<br><a class='btn' href='/trades'>Trades</a> <a class='btn' href='/entry-points'>Entry Points</a> <a class='btn' href='/price-check'>Price Check</a>";
        res.set_content(html::wrap("Symbols", h.str()), "text/html");
    });
}
