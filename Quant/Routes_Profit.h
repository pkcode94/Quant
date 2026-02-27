#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <mutex>

inline void registerProfitRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /profit � Profit calculator form ==========
    svr.Get("/profit", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>Profit Calculator</h1>"
             "<form class='card' method='POST' action='/profit'><h3>Calculate</h3>"
             "<label>Trade ID</label><input type='number' name='tradeId' required><br>"
             "<label>Current Price</label><input type='number' name='currentPrice' step='any' required><br>"
             "<label>Buy Fees</label><input type='number' name='buyFees' step='any' value='0'><br>"
             "<label>Sell Fees</label><input type='number' name='sellFees' step='any' value='0'><br>"
             "<button>Calculate</button></form>";
        auto trades = db.loadTrades();
        bool any = false;
        for (const auto& t : trades) if (t.type == TradeType::Buy) { any = true; break; }
        if (any)
        {
            h << "<h2>Buy Trades</h2><table><tr><th>ID</th><th>Symbol</th><th>Price</th><th>Qty</th><th>Sold</th><th>Rem</th><th>Buy Fee</th><th>Sell Fee</th></tr>";
            for (const auto& t : trades)
                if (t.type == TradeType::Buy)
                {
                    double sold = db.soldQuantityForParent(t.tradeId);
                    h << "<tr><td>" << t.tradeId << "</td><td>" << html::esc(t.symbol)
                      << "</td><td>" << t.value << "</td><td>" << t.quantity
                      << "</td><td>" << sold << "</td><td>" << (t.quantity - sold)
                      << "</td><td>" << t.buyFee << "</td><td>" << t.sellFee << "</td></tr>";
                }
            h << "</table>";
        }
        res.set_content(html::wrap("Profit", h.str()), "text/html");
    });

    // ========== POST /profit � Calculate and show result ==========
    svr.Post("/profit", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "tradeId");
        double cur = fd(f, "currentPrice");
        double buyFees = fd(f, "buyFees");
        double sellFees = fd(f, "sellFees");
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);

        if (tp) {
            if (buyFees == 0.0) buyFees = tp->buyFee;
            if (sellFees == 0.0) sellFees = tp->sellFee;
        }

        std::ostringstream h;
        h << std::fixed << std::setprecision(17);

        if (!tp) { h << "<div class='msg err'>Trade not found</div>"; }
        else
        {
            bool isBuy = (tp->type == TradeType::Buy);
            double sold = isBuy ? db.soldQuantityForParent(tp->tradeId) : 0.0;
            double remaining = isBuy ? (tp->quantity - sold) : tp->quantity;
            double proRatedBuyFee = (tp->quantity > 0 && isBuy) ? buyFees * (remaining / tp->quantity) : buyFees;
            auto r = ProfitCalculator::calculateForQty(*tp, cur, remaining, proRatedBuyFee, sellFees);
            db.saveProfitSnapshot(tp->symbol, id, cur, r);

            h << "<h1>Profit Result</h1>";

            // realized profit from child sells (buy trades only)
            double realizedNet = 0;
            if (isBuy)
            {
                h << "<h2>Realized (Child Sells)</h2>";
                bool anyChild = false;
                for (const auto& c : trades)
                {
                    if (c.type != TradeType::CoveredSell || c.parentTradeId != tp->tradeId) continue;
                    if (!anyChild)
                    {
                        h << "<table><tr><th>Sell ID</th><th>Sell Price</th><th>Qty</th>"
                             "<th>Gross</th><th>Net</th><th>ROI</th></tr>";
                        anyChild = true;
                    }
                    auto cp = ProfitCalculator::childProfit(c, tp->value);
                    realizedNet += cp.netProfit;
                    h << "<tr><td>" << c.tradeId << "</td><td>" << c.value << "</td>"
                      << "<td>" << c.quantity << "</td>"
                      << "<td>" << cp.grossProfit << "</td>"
                      << "<td class='" << (cp.netProfit >= 0 ? "buy" : "sell") << "'>" << cp.netProfit << "</td>"
                      << "<td>" << cp.roi << "%</td></tr>";
                }
                if (anyChild) h << "</table>";
                else h << "<p class='empty'>(no sells yet)</p>";
            }

            h << "<h2>Unrealized (Remaining " << remaining << " qty)</h2>"
                 "<div class='row'>"
                 "<div class='stat'><div class='lbl'>Trade</div><div class='val'>#" << id << " " << html::esc(tp->symbol) << "</div></div>"
                 "<div class='stat'><div class='lbl'>Entry</div><div class='val'>" << tp->value << "</div></div>"
                 "<div class='stat'><div class='lbl'>Current</div><div class='val'>" << cur << "</div></div>"
                 "<div class='stat'><div class='lbl'>Qty</div><div class='val'>" << remaining << " / " << tp->quantity << "</div></div>"
                 "<div class='stat'><div class='lbl'>Gross</div><div class='val'>" << r.grossProfit << "</div></div>"
                 "<div class='stat'><div class='lbl'>Net</div><div class='val'>" << r.netProfit << "</div></div>"
                 "<div class='stat'><div class='lbl'>ROI</div><div class='val'>" << r.roi << "%</div></div>"
                 "</div>";

            if (isBuy)
            {
                double totalNet = realizedNet + r.netProfit;
                h << "<h2>Combined</h2>"
                     "<div class='row'>"
                     "<div class='stat'><div class='lbl'>Realized</div><div class='val "
                  << (realizedNet >= 0 ? "buy" : "sell") << "'>" << realizedNet << "</div></div>"
                     "<div class='stat'><div class='lbl'>Unrealized</div><div class='val "
                  << (r.netProfit >= 0 ? "buy" : "sell") << "'>" << r.netProfit << "</div></div>"
                     "<div class='stat'><div class='lbl'>Total</div><div class='val "
                  << (totalNet >= 0 ? "buy" : "sell") << "'>" << totalNet << "</div></div>"
                     "</div>";
            }

            h << "<div class='msg'>Snapshot saved</div>";
            h << "<details><summary style='cursor:pointer;color:#c9a44a;font-size:0.85em;margin:8px 0;'>"
                  "&#9654; Calculation Steps</summary><div class='calc-console'>";
            // build a temp trade with remaining qty for trace display
            Trade tempTrade = *tp;
            tempTrade.quantity = remaining;
            tempTrade.buyFee = proRatedBuyFee;
            h << html::traceProfit(tempTrade, cur, proRatedBuyFee, sellFees, r)
              << "</div></details>";
        }

        h << "<br><form class='card' method='POST' action='/profit'><h3>Calculate Again</h3>"
             "<label>Trade ID</label><input type='number' name='tradeId' value='" << id << "' required><br>"
             "<label>Current Price</label><input type='number' name='currentPrice' step='any' value='" << cur << "' required><br>"
             "<label>Buy Fees</label><input type='number' name='buyFees' step='any' value='" << buyFees << "'><br>"
             "<label>Sell Fees</label><input type='number' name='sellFees' step='any' value='" << sellFees << "'><br>"
             "<button>Calculate</button></form>"
             "<a class='btn' href='/profit'>Back</a>";
        res.set_content(html::wrap("Profit Result", h.str()), "text/html");
    });

    // ========== GET /profit-history ==========
    svr.Get("/profit-history", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << "<h1>Profit History</h1>";
        auto rows = db.loadProfitHistory();
        if (rows.empty()) { h << "<p class='empty'>(no history)</p>"; }
        else
        {
            h << "<table><tr><th>Trade</th><th>Symbol</th><th>Price</th>"
                 "<th>Gross</th><th>Net</th><th>ROI</th></tr>";
            for (const auto& r : rows)
            {
                h << "<tr><td>" << r.tradeId << "</td><td>" << html::esc(r.symbol)
                  << "</td><td>" << r.currentPrice << "</td><td>" << r.grossProfit
                  << "</td><td>" << r.netProfit << "</td><td>" << r.roi << "%</td></tr>";
            }
            h << "</table>";
        }
        res.set_content(html::wrap("Profit History", h.str()), "text/html");
    });

    // ========== GET /params-history ==========
    svr.Get("/params-history", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << "<h1>Parameter History</h1>";
        auto rows = db.loadParamsHistory();
        if (rows.empty()) { h << "<p class='empty'>(no history)</p>"; }
        else
        {
            h << "<table><tr><th>Type</th><th>Symbol</th><th>Trade</th>"
                 "<th>Price</th><th>Qty</th><th>Levels</th><th>BuyFee</th><th>SellFee</th>"
                 "<th>Hedging</th><th>Pump</th><th>Symbols</th><th>K</th>"
                 "<th>Spread</th><th>dT</th><th>Surplus</th><th>MaxRisk</th><th>MinRisk</th><th>Risk</th></tr>";
            for (const auto& r : rows)
            {
                h << "<tr><td>" << html::esc(r.calcType) << "</td>"
                  << "<td>" << html::esc(r.symbol) << "</td>"
                  << "<td>" << r.tradeId << "</td>"
                  << "<td>" << r.currentPrice << "</td><td>" << r.quantity << "</td>"
                  << "<td>" << r.horizonCount << "</td>"
                  << "<td>" << r.buyFees << "</td><td>" << r.sellFees << "</td>"
                  << "<td>" << r.feeHedgingCoefficient << "</td>"
                  << "<td>" << r.portfolioPump << "</td><td>" << r.symbolCount << "</td>"
                  << "<td>" << r.coefficientK << "</td>"
                  << "<td>" << r.feeSpread << "</td><td>" << r.deltaTime << "</td>"
                  << "<td>" << r.surplusRate << "</td><td>" << r.maxRisk << "</td><td>" << r.minRisk << "</td><td>" << r.riskCoefficient << "</td></tr>";
            }
            h << "</table>";
        }
        res.set_content(html::wrap("Params History", h.str()), "text/html");
    });
}
