#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <mutex>
#include <algorithm>
#include <chrono>
#include <map>

inline void registerPriceCheckRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /price-check ==========
    svr.Get("/price-check", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>Price Check (TP/SL vs Market)</h1>";
        auto trades = db.loadTrades();
        std::vector<std::string> symbols;
        for (const auto& t : trades)
            if (t.type == TradeType::Buy && std::find(symbols.begin(), symbols.end(), t.symbol) == symbols.end())
                symbols.push_back(t.symbol);
        if (symbols.empty()) { h << "<p class='empty'>(no Buy trades)</p>"; }
        else
        {
            h << "<form class='card' method='POST' action='/price-check'><h3>Enter current market prices</h3>";
            for (const auto& sym : symbols)
            {
                double last = ctx.prices.latest(sym);
                h << "<label>" << html::esc(sym) << "</label>"
                     "<input type='number' name='price_" << html::esc(sym) << "' step='any'";
                if (last > 0) h << " value='" << last << "'";
                h << " required><br>";
            }
            h << "<br><button>Check</button></form>";
        }
        res.set_content(html::wrap("Price Check", h.str()), "text/html");
    });

    // ========== POST /price-check ==========
    svr.Post("/price-check", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        auto trades = db.loadTrades();
        std::vector<std::string> symbols;
        for (const auto& t : trades)
            if (t.type == TradeType::Buy && std::find(symbols.begin(), symbols.end(), t.symbol) == symbols.end())
                symbols.push_back(t.symbol);
        auto priceFor = [&](const std::string& sym) -> double { return fd(f, "price_" + sym, 0.0); };

        // Persist entered prices into the shared PriceSeries
        {
            auto now = std::chrono::system_clock::now();
            long long ts = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();
            for (const auto& sym : symbols)
            {
                double p = priceFor(sym);
                if (p > 0) ctx.prices.set(sym, ts, p);
            }
        }
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << "<h1>Price Check Results</h1>";
        h << "<form class='card' method='POST' action='/price-check'><h3>Update prices</h3>";
        for (const auto& sym : symbols)
        { double p = priceFor(sym); h << "<label>" << html::esc(sym) << "</label><input type='number' name='price_" << html::esc(sym) << "' step='any' value='" << p << "' required><br>"; }
        h << "<br><button>Check</button></form>";
        h << "<table><tr><th>ID</th><th>Symbol</th><th>Entry</th><th>Qty</th><th>Market</th><th>Gross P&amp;L</th><th>Net P&amp;L</th><th>ROI%</th>"
             "<th>TP Price</th><th>TP?</th><th>SL Price</th><th>SL?</th></tr>";
        struct Trigger { int id; std::string sym; double price; double qty; std::string tag; };
        std::vector<Trigger> triggers;
        for (const auto& t : trades)
        {
            if (t.type != TradeType::Buy) continue;
            double remaining = t.quantity - db.soldQuantityForParent(t.tradeId);
            if (remaining <= 0) continue;
            double cur = priceFor(t.symbol);
            if (cur <= 0) continue;
            double remainFrac = (t.quantity > 0) ? remaining / t.quantity : 0.0;
            auto pnl = QuantMath::computeProfit(t.value, cur, remaining,
                t.buyFee * remainFrac, 0.0);
            double gross = pnl.gross;
            double net   = pnl.net;
            double roi   = pnl.roiPct;
            double tpPrice = 0, slPrice = 0;
            bool tpHit = false, slHit = false;
            if (t.takeProfitActive && t.takeProfit > 0) { tpPrice = t.takeProfit / t.quantity; tpHit = (cur >= tpPrice); }
            if (t.stopLossActive && t.stopLoss > 0) { slPrice = t.stopLoss / t.quantity; slHit = (cur <= slPrice); }
            h << "<tr><td>" << t.tradeId << "</td><td>" << html::esc(t.symbol) << "</td><td>" << t.value << "</td><td>" << remaining << "</td>"
              << "<td>" << cur << "</td><td class='" << (gross >= 0 ? "buy" : "sell") << "'>" << gross << "</td>"
              << "<td class='" << (net >= 0 ? "buy" : "sell") << "'>" << net << "</td>"
              << "<td class='" << (roi >= 0 ? "buy" : "sell") << "'>" << roi << "</td>"
              << "<td>" << tpPrice << "</td><td class='" << (tpHit ? "buy" : "off") << "'>" << (tpHit ? "HIT" : "-") << "</td>"
              << "<td>" << slPrice << "</td><td class='" << (slHit ? "sell" : "off") << "'>" << (slHit ? "BREACHED" : "-") << "</td></tr>";
            auto levels = db.loadHorizonLevels(t.symbol, t.tradeId);
            for (const auto& lv : levels)
            {
                double htp = t.quantity > 0 ? lv.takeProfit / t.quantity : 0;
                double hsl = (t.quantity > 0 && lv.stopLoss > 0) ? lv.stopLoss / t.quantity : 0;
                bool htpHit = (cur >= htp);
                bool hslHit = (lv.stopLossActive && hsl > 0 && cur <= hsl);
                h << "<tr style='color:#64748b;'><td></td><td>[" << lv.index << "]</td><td></td><td></td><td></td><td></td><td></td><td></td>"
                  << "<td>" << htp << "</td><td class='" << (htpHit ? "buy" : "off") << "'>" << (htpHit ? "HIT" : (htp > 0 ? std::to_string(htp - cur) + " away" : "-")) << "</td>"
                  << "<td>" << hsl << "</td><td class='" << (hslHit ? "sell" : "off") << "'>";
                if (hsl > 0 && lv.stopLossActive) h << (hslHit ? "BREACHED" : "OK");
                else if (hsl > 0) h << "OFF";
                else h << "-";
                h << "</td></tr>";
            }
            if (tpHit) triggers.push_back({t.tradeId, t.symbol, cur, remaining * t.takeProfitFraction, "TP"});
            if (slHit) triggers.push_back({t.tradeId, t.symbol, cur, remaining * t.stopLossFraction, "SL"});
        }
        h << "</table>";
        auto pending = db.loadPendingExits();
        std::vector<TradeDatabase::PendingExit> triggered;
        for (const auto& pe : pending)
        { double cur = priceFor(pe.symbol); if (cur > 0 && cur >= pe.triggerPrice) triggered.push_back(pe); }
        if (!triggered.empty())
        {
            h << "<h2>Pending Exits Triggered</h2><table><tr><th>Order</th><th>Symbol</th><th>Trade</th><th>Trigger</th><th>Market</th><th>Qty</th></tr>";
            for (const auto& pe : triggered)
                h << "<tr><td>" << pe.orderId << "</td><td>" << html::esc(pe.symbol) << "</td><td>" << pe.tradeId << "</td>"
                  << "<td>" << pe.triggerPrice << "</td><td>" << priceFor(pe.symbol) << "</td><td>" << pe.sellQty << "</td></tr>";
            h << "</table>";
        }
        if (!triggers.empty())
        {
            h << "<h2>TP/SL Triggers</h2><table><tr><th>Tag</th><th>ID</th><th>Symbol</th><th>Qty</th><th>Market</th></tr>";
            for (const auto& tr : triggers)
                h << "<tr><td class='" << (tr.tag == "TP" ? "buy" : "sell") << "'>" << tr.tag << "</td>"
                  << "<td>" << tr.id << "</td><td>" << html::esc(tr.sym) << "</td><td>" << tr.qty << "</td><td>" << tr.price << "</td></tr>";
            h << "</table>";
        }
        {
            auto entryPts = db.loadEntryPoints();
            auto chainState = db.loadChainState();
            auto chainMembers = db.loadChainMembers();
            // Build entryId -> cycle map for chain-aware filtering
            std::map<int, int> entryToCycle;
            for (const auto& cm : chainMembers)
                entryToCycle[cm.entryId] = cm.cycle;

            struct TriggeredEP { int entryId; std::string symbol; double entryPrice; double fundingQty; double exitTP; double exitSL; bool isShort; double marketPrice; };
            std::vector<TriggeredEP> hitEntries;
            struct QueuedEP { int entryId; std::string symbol; double entryPrice; double fundingQty; int cycle; };
            std::vector<QueuedEP> queuedEntries;
            for (const auto& ep : entryPts)
            {
                if (ep.traded) continue;
                double cur = priceFor(ep.symbol);
                if (cur < 0) continue;
                bool hit = ep.isShort ? (cur >= ep.entryPrice) : (cur <= ep.entryPrice);
                if (!hit) continue;
                // Chain-aware: check if this entry belongs to a future cycle
                auto it = entryToCycle.find(ep.entryId);
                if (it != entryToCycle.end() && chainState.active && it->second > chainState.currentCycle) {
                    queuedEntries.push_back({ep.entryId, ep.symbol, ep.entryPrice, ep.fundingQty, it->second});
                } else {
                    hitEntries.push_back({ep.entryId, ep.symbol, ep.entryPrice, ep.fundingQty, ep.exitTakeProfit, ep.exitStopLoss, ep.isShort, cur});
                }
            }
            if (!hitEntries.empty())
            {
                h << "<h2>Entry Points Triggered (" << hitEntries.size() << ")</h2>"
                     "<form class='card' method='POST' action='/execute-triggered-entries'>"
                     "<table><tr><th>ID</th><th>Symbol</th><th>Entry</th><th>Market</th><th>Qty</th><th>Cost</th><th>TP</th><th>SL</th><th>Buy Fee</th></tr>";
                for (const auto& te : hitEntries)
                {
                    double cost = te.entryPrice * te.fundingQty;
                    auto cit = entryToCycle.find(te.entryId);
                    std::string cycleTag = (cit != entryToCycle.end()) ? " <span style='color:#c9a44a;'>[C" + std::to_string(cit->second) + "]</span>" : "";
                    h << "<tr><td>" << te.entryId << cycleTag << "</td><td>" << html::esc(te.symbol) << "</td><td>" << te.entryPrice << "</td>"
                      << "<td class='buy'>" << te.marketPrice << "</td><td>" << te.fundingQty << "</td><td>" << cost << "</td>"
                      << "<td>" << te.exitTP << "</td><td>" << te.exitSL << "</td>"
                      << "<td><input type='number' name='fee_" << te.entryId << "' step='any' value='0' style='width:80px;'>"
                      << "<input type='hidden' name='epid_" << te.entryId << "' value='1'></td></tr>";
                }
                h << "</table><button>Execute Triggered Entries</button></form>";
            }
            if (!queuedEntries.empty())
            {
                h << "<h2>Chain Entries Queued (" << queuedEntries.size() << ")</h2>"
                     "<p style='color:#64748b;font-size:0.82em;'>These entries are triggered but belong to future chain cycles. "
                     "Complete cycle " << chainState.currentCycle << " first.</p>"
                     "<table><tr><th>ID</th><th>Symbol</th><th>Entry</th><th>Qty</th><th>Cycle</th><th>Status</th></tr>";
                for (const auto& qe : queuedEntries)
                {
                    h << "<tr style='color:#64748b;'><td>" << qe.entryId << "</td><td>" << html::esc(qe.symbol) << "</td>"
                      << "<td>" << qe.entryPrice << "</td><td>" << qe.fundingQty << "</td>"
                      << "<td><span style='color:#c9a44a;'>C" << qe.cycle << "</span></td>"
                      << "<td>QUEUED</td></tr>";
                }
                h << "</table>";
            }
        }
        h << "<br><a class='btn' href='/price-check'>Back</a> <a class='btn' href='/trades'>Trades</a>";
        res.set_content(html::wrap("Price Check Results", h.str()), "text/html");
    });

    // ========== POST /execute-triggered-entries ==========
    svr.Post("/execute-triggered-entries", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        auto entryPts = db.loadEntryPoints();
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << "<h1>Entry Execution</h1>";
        int executed = 0, failed = 0;
        h << "<table><tr><th>ID</th><th>Symbol</th><th>Entry</th><th>Qty</th><th>Fee</th><th>Trade</th><th>TP</th><th>SL</th><th>Status</th></tr>";
        for (auto& ep : entryPts)
        {
            if (ep.traded) continue;
            if (fv(f, "epid_" + std::to_string(ep.entryId)).empty()) continue;
            double buyFee = fd(f, "fee_" + std::to_string(ep.entryId));
            double cost = ep.entryPrice * ep.fundingQty + buyFee;
            double walBal = db.loadWalletBalance();
            if (cost > walBal)
            {
                h << "<tr><td>" << ep.entryId << "</td><td>" << html::esc(ep.symbol) << "</td><td>" << ep.entryPrice << "</td>"
                  << "<td>" << ep.fundingQty << "</td><td>" << buyFee << "</td><td>-</td><td>-</td><td>-</td>"
                  << "<td class='sell'>INSUFFICIENT (need " << cost << ", have " << walBal << ")</td></tr>";
                ++failed; continue;
            }
            int bid = db.executeBuy(ep.symbol, ep.entryPrice, ep.fundingQty, buyFee);
            ep.traded = true; ep.linkedTradeId = bid;
            h << "<tr><td>" << ep.entryId << "</td><td>" << html::esc(ep.symbol) << "</td><td>" << ep.entryPrice << "</td>"
              << "<td>" << ep.fundingQty << "</td><td>" << buyFee << "</td><td class='buy'>#" << bid << "</td>"
              << "<td>" << ep.exitTakeProfit << "</td><td>" << ep.exitStopLoss << "</td><td class='buy'>OK</td></tr>";
            ++executed;
        }
        h << "</table>";
        if (executed > 0)
        {
            auto trades = db.loadTrades();
            for (const auto& ep : entryPts)
            {
                if (ep.linkedTradeId < 0) continue;
                auto* tradePtr = db.findTradeById(trades, ep.linkedTradeId);
                if (!tradePtr) continue;
                tradePtr->takeProfit = ep.exitTakeProfit * tradePtr->quantity;
                tradePtr->takeProfitFraction = (ep.exitTakeProfit > 0) ? 1.0 : 0.0;
                tradePtr->takeProfitActive = (tradePtr->takeProfitFraction > 0.0);
                tradePtr->stopLoss = ep.exitStopLoss * tradePtr->quantity;
                tradePtr->stopLossFraction = 0.0;
                tradePtr->stopLossActive = false;
                db.updateTrade(*tradePtr);
            }
        }
        db.saveEntryPoints(entryPts);
        h << "<div class='row'>"
             "<div class='stat'><div class='lbl'>Executed</div><div class='val'>" << executed << "</div></div>"
             "<div class='stat'><div class='lbl'>Failed</div><div class='val'>" << failed << "</div></div>"
             "<div class='stat'><div class='lbl'>Wallet</div><div class='val'>" << db.loadWalletBalance() << "</div></div></div>";
        h << "<br><a class='btn' href='/trades'>Trades</a> <a class='btn' href='/entry-points'>Entry Points</a> <a class='btn' href='/price-check'>Price Check</a>";
        res.set_content(html::wrap("Entry Execution", h.str()), "text/html");
    });
}
