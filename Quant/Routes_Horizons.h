#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <mutex>
#include <algorithm>

inline void registerHorizonRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /horizons ==========
    svr.Get("/horizons", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << "<h1>Horizon Levels</h1>"
             "<form class='card' method='GET' action='/horizons'><h3>View Horizons</h3>"
             "<label>Trade ID</label><input type='number' name='tradeId' value='"
          << req.get_param_value("tradeId") << "' required> "
             "<button>View</button></form>";
        if (req.has_param("tradeId"))
        {
            int id = 0;
            try { id = std::stoi(req.get_param_value("tradeId")); } catch (...) {}
            auto trades = db.loadTrades();
            auto* tp = db.findTradeById(trades, id);
            if (!tp) { h << "<div class='msg err'>Trade not found</div>"; }
            else
            {
                auto levels = db.loadHorizonLevels(tp->symbol, id);
                h << "<h2>" << html::esc(tp->symbol) << " #" << id
                  << " (price=" << tp->value << " qty=" << tp->quantity << ")</h2>";
                if (levels.empty()) { h << "<p class='empty'>(no horizons)</p>"; }
                else
                {
                    h << "<table><tr><th>Index</th><th>Take Profit</th><th>TP/unit</th>"
                         "<th>Stop Loss</th><th>SL/unit</th><th>SL?</th></tr>";
                    for (const auto& lv : levels)
                    {
                        double tpu = tp->quantity > 0 ? lv.takeProfit / tp->quantity : 0;
                        double slu = (tp->quantity > 0 && lv.stopLoss > 0) ? lv.stopLoss / tp->quantity : 0;
                        h << "<tr><td>" << lv.index << "</td>"
                          << "<td>" << lv.takeProfit << "</td><td>" << tpu << "</td>"
                          << "<td>" << lv.stopLoss << "</td><td>" << slu << "</td>"
                          << "<td class='" << (lv.stopLossActive ? "on" : "off") << "'>"
                          << (lv.stopLossActive ? "ON" : "OFF") << "</td></tr>";
                    }
                    h << "</table>";
                }
            }
        }
        res.set_content(html::wrap("Horizons", h.str()), "text/html");
    });

    // ========== GET /generate-horizons ==========
    svr.Get("/generate-horizons", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        if (!db.hasBuyTrades())
        {
            res.set_redirect("/market-entry?err=Add+buy+trades+before+generating+horizons", 303);
            return;
        }
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        { bool canH = db.hasBuyTrades();
          h << html::workflow(1, canH, canH); }
        h << "<h1>Generate TP/SL Horizons</h1>";
        auto trades = db.loadTrades();
        bool any = false;
        for (const auto& t : trades) if (t.type == TradeType::Buy) { any = true; break; }
        h << "<form class='card' method='POST' action='/generate-horizons'><h3>Shared Parameters</h3>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Trade IDs</label><input type='text' name='tradeIds' placeholder='1,2,3 or 0=all' value='0'><br>"
             "<label>Fee Hedging</label><input type='number' name='feeHedgingCoefficient' step='any' value='1'><br>"
             "<label>Pump</label><input type='number' name='portfolioPump' step='any' value='0'><br>"
             "<label>Symbol Count</label><input type='number' name='symbolCount' value='1'><br>"
             "<label>Coefficient K</label><input type='number' name='coefficientK' step='any' value='0'><br>"
             "<label>Fee Spread</label><input type='number' name='feeSpread' step='any' value='0'><br>"
             "<label>Delta Time</label><input type='number' name='deltaTime' step='any' value='1'><br>"
             "<label>Surplus Rate</label><input type='number' name='surplusRate' step='any' value='0.02'><br>"
             "<label>Max Risk</label><input type='number' name='maxRisk' step='any' value='0'><br>"
             "<label>Min Risk</label><input type='number' name='minRisk' step='any' value='0'><br>"
             "<label>Stop Losses</label><select name='generateStopLosses'>"
             "<option value='0'>No</option><option value='1'>Yes</option></select><br>";
        if (any)
        {
            h << "<h3 style='margin-top:12px;'>Buy Trades</h3>"
                 "<table><tr><th>ID</th><th>Symbol</th><th>Price</th>"
                 "<th>Qty</th><th>Cost</th><th>TP</th><th>SL</th>"
                 "<th>Buy Fee</th></tr>";
            for (const auto& t : trades)
            {
                if (t.type != TradeType::Buy) continue;
                h << "<tr><td>" << t.tradeId << "</td><td>" << html::esc(t.symbol)
                  << "</td><td>" << t.value << "</td><td>" << t.quantity
                  << "</td><td>" << (t.value * t.quantity) << "</td><td>" << t.takeProfit
                  << "</td><td>" << t.stopLoss << "</td>"
                  << "<td>" << t.buyFee << "</td></tr>";
            }
            h << "</table>";
        }
        h << "<button>Generate</button></form>";
        res.set_content(html::wrap("Generate Horizons", h.str()), "text/html");
    });

    // ========== POST /generate-horizons ==========
    svr.Post("/generate-horizons", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        std::string sym = normalizeSymbol(fv(f, "symbol"));
        std::string idsStr = fv(f, "tradeIds", "0");
        HorizonParams baseP;
        baseP.horizonCount = 1;
        baseP.feeHedgingCoefficient = fd(f, "feeHedgingCoefficient", 1.0);
        baseP.portfolioPump = fd(f, "portfolioPump");
        baseP.symbolCount = fi(f, "symbolCount", 1);
        baseP.coefficientK = fd(f, "coefficientK");
        baseP.feeSpread = fd(f, "feeSpread");
        baseP.deltaTime = fd(f, "deltaTime", 1.0);
        baseP.surplusRate = fd(f, "surplusRate");
        baseP.maxRisk = fd(f, "maxRisk");
        baseP.minRisk = fd(f, "minRisk");
        baseP.generateStopLosses = (fv(f, "generateStopLosses") == "1");
        auto trades = db.loadTrades();
        bool selectAll = (idsStr == "0" || idsStr.empty());
        std::vector<int> selectedIds;
        if (!selectAll)
        {
            std::istringstream ss(idsStr);
            std::string tok;
            while (std::getline(ss, tok, ','))
            {
                try { selectedIds.push_back(std::stoi(tok)); } catch (...) {}
            }
        }
        std::vector<Trade*> buyTrades;
        for (auto& t : trades)
        {
            if (t.symbol != sym || t.type != TradeType::Buy) continue;
            if (selectAll || std::find(selectedIds.begin(), selectedIds.end(), t.tradeId) != selectedIds.end())
                buyTrades.push_back(&t);
        }
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        if (buyTrades.empty())
        {
            h << "<div class='msg err'>No Buy trades found for " << html::esc(sym) << "</div>";
        }
        else
        {
            { bool canH = db.hasBuyTrades();
              h << html::workflow(1, canH, canH); }
            h << "<h1>Horizons for " << html::esc(sym) << "</h1>";
            for (auto* bt : buyTrades)
            {
                HorizonParams tp = baseP;
                auto levels = MultiHorizonEngine::generate(*bt, tp);
                MultiHorizonEngine::applyFirstHorizon(*bt, levels, false);
                db.updateTrade(*bt);
                db.saveHorizonLevels(sym, bt->tradeId, levels);
                db.saveParamsSnapshot(
                    TradeDatabase::ParamsRow::from("horizon", sym, bt->tradeId,
                                                   bt->value, bt->quantity, tp, 0.0,
                                                   bt->buyFee, bt->sellFee));
                double overhead = MultiHorizonEngine::computeOverhead(*bt, tp);
                double eo = MultiHorizonEngine::effectiveOverhead(*bt, tp);
                h << "<h2>Trade #" << bt->tradeId
                  << " (price=" << bt->value << " qty=" << bt->quantity
                  << " buyFee=" << bt->buyFee << ")</h2>"
                  << "<div class='row'>"
                     "<div class='stat'><div class='lbl'>Overhead</div><div class='val'>"
                  << std::fixed << std::setprecision(17) << (overhead * 100) << "%</div></div>"
                     "<div class='stat'><div class='lbl'>Effective</div><div class='val'>"
                  << (eo * 100) << "%</div></div></div>"
                  << std::fixed << std::setprecision(17);
                {
                    double base = bt->value * bt->quantity;
                    std::ostringstream con;
                    con << std::fixed << std::setprecision(8);
                    con << "<details><summary style='cursor:pointer;color:#c9a44a;font-size:0.85em;margin:4px 0;'>"
                           "&#9654; Calculation Steps</summary><div class='calc-console'>";
                    con << html::traceOverhead(bt->value, bt->quantity, tp);
                    con << "<span class='hd'>Horizon Levels</span>"
                        << "base = price &times; qty = " << bt->value << " &times; " << bt->quantity
                        << " = <span class='vl'>" << base << "</span>\n";
                    for (const auto& lv : levels)
                    {
                        double factor = eo * (lv.index + 1);
                        con << "\n<span class='vl'>Level " << lv.index << "</span>\n"
                            << "  factor = " << eo << " &times; " << (lv.index + 1) << " = " << factor << "\n"
                            << "  TP = " << base << " &times; (1 + " << factor << ") = <span class='rs'>" << lv.takeProfit << "</span>\n";
                        if (lv.stopLoss > 0)
                            con << "  SL = " << base << " &times; (1 - " << factor << ") = <span class='rs'>" << lv.stopLoss << "</span>\n";
                    }
                    con << "</div></details>";
                    h << con.str();
                }
                h << "<table><tr><th>Lvl</th><th>Take Profit</th><th>TP/unit</th>"
                     "<th>Stop Loss</th><th>SL/unit</th><th>SL?</th></tr>";
                for (const auto& lv : levels)
                {
                    double tpu = bt->quantity > 0 ? lv.takeProfit / bt->quantity : 0;
                    double slu = (bt->quantity > 0 && lv.stopLoss > 0) ? lv.stopLoss / bt->quantity : 0;
                    h << "<tr><td>" << lv.index << "</td>"
                      << "<td>" << lv.takeProfit << "</td><td>" << tpu << "</td>"
                      << "<td>" << lv.stopLoss << "</td><td>" << slu << "</td>"
                      << "<td class='" << (lv.stopLossActive ? "on" : "off") << "'>"
                      << (lv.stopLossActive ? "ON" : "OFF") << "</td></tr>";
                }
                h << "</table>";
            }
            h << "<div class='msg'>Generated and saved for " << buyTrades.size() << " trade(s). SL is OFF by default.</div>";
        }
        h << "<br><a class='btn' href='/generate-horizons'>Back</a> "
             "<a class='btn' href='/trades'>Trades</a>";
        res.set_content(html::wrap("Horizon Results", h.str()), "text/html");
    });
}
