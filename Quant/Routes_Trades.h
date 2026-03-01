#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include "ProfitCalculator.h"
#include <mutex>

inline void registerTradeRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /trades � Trades list + forms ==========
    svr.Get("/trades", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>Trades</h1>";
        auto trades = db.loadTrades();
        if (trades.empty()) { h << "<p class='empty'>(no trades)</p>"; }
        else
        {
            // collect parent buys, then render each with children indented below
            std::vector<const Trade*> parents;
            std::vector<const Trade*> orphanSells;
            for (const auto& t : trades)
            {
                if (t.type == TradeType::Buy)
                    parents.push_back(&t);
                else if (t.parentTradeId < 0)
                    orphanSells.push_back(&t);
            }

            h << "<table><tr>"
                 "<th>ID</th><th>Date</th><th>Symbol</th><th>Type</th><th>Price</th><th>Qty</th>"
                 "<th>Cost</th><th>Fees</th>"
                 "<th>TP</th><th>TP?</th><th>SL</th><th>SL?</th>"
                 "<th>Sold</th><th>Rem</th><th>Realized</th><th>Actions</th>"
                 "</tr>";

            for (const auto* bp : parents)
            {
                const Trade& b = *bp;
                double sold = db.soldQuantityForParent(b.tradeId);
                double remaining = b.quantity - sold;
                double grossCost = b.value * b.quantity;
                double totalFees = b.buyFee + b.sellFee;
                // sum realized from children
                double realized = 0;
                std::vector<const Trade*> children;
                for (const auto& c : trades)
                {
                    if (c.type == TradeType::CoveredSell && c.parentTradeId == b.tradeId)
                    {
                        children.push_back(&c);
                        auto cp = ProfitCalculator::childProfit(c, b.value);
                        realized += cp.netProfit;
                    }
                }

                // ---- parent row ----
                h << "<tr>"
                  << "<td><a href='/horizons?tradeId=" << b.tradeId << "'><strong>" << b.tradeId << "</strong></a></td>"
                  << "<td style='font-size:0.78em;'>" << html::fmtTime(b.timestamp) << "</td>"
                  << "<td><strong>" << html::esc(b.symbol) << "</strong></td>"
                  << "<td class='buy'>BUY</td>"
                  << "<td>" << b.value << "</td><td>" << b.quantity << "</td>"
                  << "<td>" << grossCost << "</td>"
                  << "<td>" << totalFees << "</td>"
                  // TP inline
                  << "<td><form class='iform' method='POST' action='/set-tp'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='number' name='tp' step='any' value='" << b.takeProfit << "' style='width:70px;'>"
                  << "<button class='btn-sm' title='Set TP'>&#10003;</button></form>"
                  << "<form class='iform' method='POST' action='/set-tp' style='display:inline;'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='hidden' name='tp' value='0'>"
                  << "<button class='btn-sm btn-danger' title='Clear TP'>&times;</button></form></td>"
                  // TP active checkbox
                  << "<td><form class='iform' method='POST' action='/set-tp-active'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='hidden' name='active' value='" << (b.takeProfitActive ? "0" : "1") << "'>"
                  << "<input type='checkbox'" << (b.takeProfitActive ? " checked" : "")
                  << " onchange='this.form.submit()' title='TP " << (b.takeProfitActive ? "ON" : "OFF") << "'>"
                  << "</form></td>"
                  // SL inline
                  << "<td><form class='iform' method='POST' action='/set-sl'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='number' name='sl' step='any' value='" << b.stopLoss << "' style='width:70px;'>"
                  << "<button class='btn-sm' title='Set SL'>&#10003;</button></form>"
                  << "<form class='iform' method='POST' action='/set-sl' style='display:inline;'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='hidden' name='sl' value='0'>"
                  << "<button class='btn-sm btn-danger' title='Clear SL'>&times;</button></form></td>"
                  // SL active checkbox
                  << "<td><form class='iform' method='POST' action='/set-sl-active'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<input type='hidden' name='active' value='" << (b.stopLossActive ? "0" : "1") << "'>"
                  << "<input type='checkbox'" << (b.stopLossActive ? " checked" : "")
                  << " onchange='this.form.submit()' title='SL " << (b.stopLossActive ? "ON" : "OFF") << "'>"
                  << "</form></td>"
                  << "<td>" << sold << "</td><td>" << remaining << "</td>"
                  << "<td class='" << (realized >= 0 ? "buy" : "sell") << "'>" << realized << "</td>"
                  << "<td>"
                  << "<a class='btn btn-sm' href='/edit-trade?id=" << b.tradeId << "'>Edit</a> "
                  << "<form class='iform' method='POST' action='/delete-trade'>"
                  << "<input type='hidden' name='id' value='" << b.tradeId << "'>"
                  << "<button class='btn-sm btn-danger'>Del</button></form>"
                  << "</td></tr>";

                // ---- child rows (indented) ----
                for (const auto* cp : children)
                {
                    const Trade& c = *cp;
                    auto profit = ProfitCalculator::childProfit(c, b.value);
                    double cGross = c.value * c.quantity;
                    h << "<tr class='child-row'>"
                      << "<td><span class='child-indent'>&#9492;&#9472;</span>"
                      << "<a href='/edit-trade?id=" << c.tradeId << "'>" << c.tradeId << "</a></td>"
                      << "<td style='font-size:0.78em;'>" << html::fmtTime(c.timestamp) << "</td>"
                      << "<td>" << html::esc(c.symbol) << "</td>"
                      << "<td class='sell'>SELL</td>"
                      << "<td>" << c.value << "</td><td>" << c.quantity << "</td>"
                      << "<td>" << cGross << "</td>"
                      << "<td>" << c.sellFee << "</td>"
                      << "<td>-</td><td>-</td><td>-</td><td>-</td>"
                      << "<td>-</td><td>-</td>"
                      << "<td class='" << (profit.netProfit >= 0 ? "buy" : "sell") << "'>"
                      << profit.netProfit << " (" << std::setprecision(2) << profit.roi << "%)" << std::setprecision(17) << "</td>"
                      << "<td>"
                      << "<a class='btn btn-sm' href='/edit-trade?id=" << c.tradeId << "'>Edit</a> "
                      << "<form class='iform' method='POST' action='/delete-trade'>"
                      << "<input type='hidden' name='id' value='" << c.tradeId << "'>"
                      << "<button class='btn-sm btn-danger'>Del</button></form>"
                      << "</td></tr>";
                }
            }

            // orphan sells (no valid parent)
            for (const auto* sp : orphanSells)
            {
                const Trade& s = *sp;
                double cGross = s.value * s.quantity;
                h << "<tr>"
                  << "<td>" << s.tradeId << "</td>"
                  << "<td style='font-size:0.78em;'>" << html::fmtTime(s.timestamp) << "</td>"
                  << "<td>" << html::esc(s.symbol) << "</td>"
                  << "<td class='sell'>SELL</td>"
                  << "<td>" << s.value << "</td><td>" << s.quantity << "</td>"
                  << "<td>" << cGross << "</td>"
                  << "<td>" << s.sellFee << "</td>"
                  << "<td>-</td><td>-</td><td>-</td><td>-</td>"
                  << "<td>-</td><td>-</td><td>-</td>"
                  << "<td>"
                  << "<a class='btn btn-sm' href='/edit-trade?id=" << s.tradeId << "'>Edit</a> "
                  << "<form class='iform' method='POST' action='/delete-trade'>"
                  << "<input type='hidden' name='id' value='" << s.tradeId << "'>"
                  << "<button class='btn-sm btn-danger'>Del</button></form>"
                  << "</td></tr>";
            }

            h << "</table>";
        }
        h << "<div class='forms-row'>";
        h << "<form class='card' method='POST' action='/execute-buy'>"
             "<h3>Execute Buy</h3>"
             "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Creates trade &amp; debits wallet</div>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Date</label><input type='datetime-local' name='timestamp'><br>"
             "<label>Price</label><input type='number' name='price' step='any' required><br>"
             "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
             "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='0'><br>"
             "<label>Take Profit</label><input type='number' name='takeProfit' step='any' value='0'><br>"
             "<label>Stop Loss</label><input type='number' name='stopLoss' step='any' value='0'><br>"
             "<button>Execute Buy</button></form>";
        h << "<form class='card' method='POST' action='/execute-sell'>"
             "<h3>Execute Sell</h3>"
             "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Deducts from symbol holdings &amp; credits wallet</div>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Date</label><input type='datetime-local' name='timestamp'><br>"
             "<label>Price</label><input type='number' name='price' step='any' required><br>"
             "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
             "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='0'><br>"
             "<button>Execute Sell</button></form>";
        h << "<form class='card' method='POST' action='/add-trade'>"
             "<h3>Import Buy</h3>"
             "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Record only &mdash; no wallet movement</div>"
             "<input type='hidden' name='type' value='Buy'>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Date</label><input type='datetime-local' name='timestamp'><br>"
             "<label>Price</label><input type='number' name='price' step='any' required><br>"
             "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
             "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='0'><br>"
             "<label>Take Profit</label><input type='number' name='takeProfit' step='any' value='0'><br>"
             "<label>Stop Loss</label><input type='number' name='stopLoss' step='any' value='0'><br>"
             "<button>Import Buy</button></form>";
        h << "<form class='card' method='POST' action='/add-trade'>"
             "<h3>Import Sell</h3>"
             "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Record only &mdash; no wallet movement</div>"
             "<input type='hidden' name='type' value='CoveredSell'>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Date</label><input type='datetime-local' name='timestamp'><br>"
             "<label>Parent Buy ID</label><input type='number' name='parentTradeId' required><br>"
             "<label>Price</label><input type='number' name='price' step='any' required><br>"
             "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
             "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='0'><br>"
             "<button>Import Sell</button></form>";
        h << "</div>";
        res.set_content(html::wrap("Trades", h.str()), "text/html");
    });

    // ========== POST /add-trade ==========
    svr.Post("/add-trade", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        if (!ctx.canAddTrade(req))
        {
            res.set_redirect("/trades?err=Trade+limit+reached.+Upgrade+to+Premium+for+unlimited+trades.", 303);
            return;
        }
        auto f = parseForm(req.body);
        Trade t;
        t.tradeId = db.nextTradeId();
        t.symbol = normalizeSymbol(fv(f, "symbol"));
        t.type = (fv(f, "type") == "CoveredSell") ? TradeType::CoveredSell : TradeType::Buy;
        t.value = fd(f, "price");
        t.quantity = fd(f, "quantity");
        t.stopLossActive = false;
        t.shortEnabled = false;
        t.timestamp = html::parseDatetimeLocal(fv(f, "timestamp"));
        if (t.timestamp <= 0) t.timestamp = static_cast<long long>(std::time(nullptr));
        if (t.value <= 0 || t.quantity <= 0) { res.set_redirect("/trades?err=Price+and+quantity+must+be+positive", 303); return; }
        if (t.type == TradeType::Buy)
        {
            t.parentTradeId = -1;
            t.buyFee = fd(f, "buyFee");
            t.sellFee = 0.0;
            t.takeProfit = fd(f, "takeProfit");
            t.stopLoss = fd(f, "stopLoss");
        }
        else
        {
            t.parentTradeId = fi(f, "parentTradeId", -1);
            auto trades = db.loadTrades();
            auto* parent = db.findTradeById(trades, t.parentTradeId);
            if (!parent || parent->type != TradeType::Buy)
            {
                res.set_redirect("/trades?err=Parent+must+be+an+existing+Buy+trade", 303);
                return;
            }
            t.buyFee = 0.0;
            t.sellFee = fd(f, "sellFee");
        }
        db.addTrade(t);
        ctx.symbols.getOrCreate(t.symbol);
        res.set_redirect("/trades?msg=Trade+" + std::to_string(t.tradeId) + "+created", 303);
    });

    // ========== POST /delete-trade ==========
    svr.Post("/delete-trade", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        if (!db.findTradeById(trades, id)) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        db.removeTrade(id);
        res.set_redirect("/trades?msg=Trade+" + std::to_string(id) + "+deleted", 303);
    });

    // ========== POST /toggle-sl ==========
    svr.Post("/toggle-sl", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);
        if (!tp) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        tp->stopLossActive = !tp->stopLossActive;
        db.updateTrade(*tp);
        std::string state = tp->stopLossActive ? "ON" : "OFF";
        res.set_redirect("/trades?msg=SL+now+" + state + "+for+" + std::to_string(id), 303);
    });

    // ========== POST /set-sl-active ==========
    svr.Post("/set-sl-active", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);
        if (!tp) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        bool want = (fv(f, "active") == "1");
        tp->stopLossActive = want && (tp->stopLoss > 0);
        db.updateTrade(*tp);
        std::string state = tp->stopLossActive ? "ON" : "OFF";
        res.set_redirect("/trades?msg=SL+now+" + state + "+for+" + std::to_string(id), 303);
    });

    // ========== POST /set-tp ==========
    svr.Post("/set-tp", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* t = db.findTradeById(trades, id);
        if (!t) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        t->takeProfit = fd(f, "tp");
        t->takeProfitActive = (t->takeProfit > 0);
        db.updateTrade(*t);
        res.set_redirect("/trades?msg=TP+set+to+" + fv(f, "tp") + "+for+" + std::to_string(id), 303);
    });

    // ========== POST /set-tp-active ==========
    svr.Post("/set-tp-active", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);
        if (!tp) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        bool want = (fv(f, "active") == "1");
        tp->takeProfitActive = want && (tp->takeProfit > 0);
        db.updateTrade(*tp);
        std::string state = tp->takeProfitActive ? "ON" : "OFF";
        res.set_redirect("/trades?msg=TP+now+" + state + "+for+" + std::to_string(id), 303);
    });

    // ========== POST /set-sl ==========
    svr.Post("/set-sl", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* t = db.findTradeById(trades, id);
        if (!t) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        t->stopLoss = fd(f, "sl");
        t->stopLossActive = (t->stopLoss > 0);
        db.updateTrade(*t);
        res.set_redirect("/trades?msg=SL+set+to+" + fv(f, "sl") + "+for+" + std::to_string(id), 303);
    });

    // ========== POST /execute-buy ==========
    svr.Post("/execute-buy", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        if (!ctx.canAddTrade(req))
        {
            res.set_redirect("/trades?err=Trade+limit+reached.+Upgrade+to+Premium+for+unlimited+trades.", 303);
            return;
        }
        auto f = parseForm(req.body);
        std::string sym = normalizeSymbol(fv(f, "symbol"));
        double price = fd(f, "price");
        double qty = fd(f, "quantity");
        double fee = fd(f, "buyFee");
        double tp = fd(f, "takeProfit");
        double sl = fd(f, "stopLoss");
        long long ts = html::parseDatetimeLocal(fv(f, "timestamp"));
        if (sym.empty() || price <= 0 || qty <= 0) { res.set_redirect("/trades?err=Invalid+buy+parameters", 303); return; }
        double walBal = db.loadWalletBalance();
        double needed = price * qty + fee;
        if (needed > walBal) { res.set_redirect("/trades?err=Insufficient+funds", 303); return; }
        int bid = db.executeBuy(sym, price, qty, fee, tp, sl);
        if (ts > 0)
        {
            auto trades = db.loadTrades();
            auto* tp2 = db.findTradeById(trades, bid);
            if (tp2) { tp2->timestamp = ts; db.updateTrade(*tp2); }
        }
        res.set_redirect("/trades?msg=Buy+" + std::to_string(bid) + "+executed", 303);
    });

    // ========== POST /execute-sell ==========
    svr.Post("/execute-sell", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        if (!ctx.canAddTrade(req))
        {
            res.set_redirect("/trades?err=Trade+limit+reached.+Upgrade+to+Premium+for+unlimited+trades.", 303);
            return;
        }
        auto f = parseForm(req.body);
        std::string sym = normalizeSymbol(fv(f, "symbol"));
        double price = fd(f, "price");
        double qty = fd(f, "quantity");
        double fee = fd(f, "sellFee");
        long long ts = html::parseDatetimeLocal(fv(f, "timestamp"));
        if (sym.empty() || price <= 0 || qty <= 0) { res.set_redirect("/trades?err=Invalid+sell+parameters", 303); return; }
        int sid = db.executeSell(sym, price, qty, fee);
        if (sid < 0) { res.set_redirect("/trades?err=Sell+failed+(insufficient+holdings)", 303); return; }
        if (ts > 0)
        {
            auto trades = db.loadTrades();
            auto* tp = db.findTradeById(trades, sid);
            if (tp) { tp->timestamp = ts; db.updateTrade(*tp); }
        }
        res.set_redirect("/trades?msg=Sell+" + std::to_string(sid) + "+executed", 303);
    });

    // ========== GET /edit-trade ==========
    svr.Get("/edit-trade", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        int id = 0;
        try { id = std::stoi(req.get_param_value("id")); } catch (...) {}
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);
        if (!tp) { h << "<h1>Edit Trade</h1><div class='msg err'>Trade not found</div>"; }
        else
        {
            bool isBuy = (tp->type == TradeType::Buy);
            h << "<h1>Edit Trade #" << tp->tradeId << "</h1>"
                 "<form class='card' method='POST' action='/edit-trade'>"
                 "<input type='hidden' name='id' value='" << tp->tradeId << "'>"
                 "<label>Symbol</label><input type='text' name='symbol' value='" << html::esc(tp->symbol) << "'><br>"
                 "<label>Date</label><input type='datetime-local' name='timestamp' value='" << html::fmtDatetimeLocal(tp->timestamp) << "'><br>"
                 "<label>Type</label><select name='type'>"
                 "<option value='Buy'" << (isBuy ? " selected" : "") << ">Buy</option>"
                 "<option value='CoveredSell'" << (!isBuy ? " selected" : "") << ">CoveredSell</option>"
                 "</select><br>"
                 "<label>Price</label><input type='number' name='price' step='any' value='" << tp->value << "'><br>"
                 "<label>Quantity</label><input type='number' name='quantity' step='any' value='" << tp->quantity << "'><br>"
                 "<label>Parent Buy ID</label><input type='number' name='parentTradeId' value='" << tp->parentTradeId << "'>"
                 "<div style='color:#64748b;font-size:0.78em;'>-1 = none (for Buy trades)</div><br>"
                 "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='" << tp->buyFee << "'><br>"
                 "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='" << tp->sellFee << "'><br>"
                 "<label>Take Profit</label><input type='number' name='takeProfit' step='any' value='" << tp->takeProfit << "'>"
                 "<span style='color:#64748b;font-size:0.78em;margin-left:8px;'>" << (tp->takeProfitActive ? "active" : "inactive (set value &gt; 0 to activate)") << "</span><br>"
                 "<label>Stop Loss</label><input type='number' name='stopLoss' step='any' value='" << tp->stopLoss << "'>"
                 "<span style='color:#64748b;font-size:0.78em;margin-left:8px;'>" << (tp->stopLossActive ? "active" : "inactive (set value &gt; 0 to activate)") << "</span><br>"
                 "<br><button>Save Changes</button></form>";
        }
        h << "<br><a class='btn' href='/trades'>Back to Trades</a>";
        res.set_content(html::wrap("Edit Trade", h.str()), "text/html");
    });

    // ========== POST /edit-trade ==========
    svr.Post("/edit-trade", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);
        int id = fi(f, "id");
        auto trades = db.loadTrades();
        auto* tp = db.findTradeById(trades, id);
        if (!tp) { res.set_redirect("/trades?err=Trade+not+found", 303); return; }
        auto sym = fv(f, "symbol");
        if (!sym.empty()) tp->symbol = normalizeSymbol(sym);
        auto typeStr = fv(f, "type");
        if (!typeStr.empty()) tp->type = (typeStr == "CoveredSell") ? TradeType::CoveredSell : TradeType::Buy;
        double price = fd(f, "price");
        if (price > 0) tp->value = price;
        double qty = fd(f, "quantity");
        if (qty > 0) tp->quantity = qty;
        int pid = fi(f, "parentTradeId", tp->parentTradeId);
        if (tp->type == TradeType::Buy)
        {
            tp->parentTradeId = -1;
        }
        else
        {
            if (pid >= 0)
            {
                auto* parent = db.findTradeById(trades, pid);
                if (!parent || parent->type != TradeType::Buy)
                {
                    res.set_redirect("/trades?err=Parent+must+be+an+existing+Buy+trade", 303);
                    return;
                }
            }
            tp->parentTradeId = pid;
        }
        tp->buyFee = fd(f, "buyFee", tp->buyFee);
        tp->sellFee = fd(f, "sellFee", tp->sellFee);
        tp->takeProfit = fd(f, "takeProfit", tp->takeProfit);
        tp->takeProfitActive = (tp->takeProfit > 0);
        tp->stopLoss = fd(f, "stopLoss", tp->stopLoss);
        tp->stopLossActive = (tp->stopLoss > 0);
        auto tsStr = fv(f, "timestamp");
        if (!tsStr.empty()) tp->timestamp = html::parseDatetimeLocal(tsStr);
        db.updateTrade(*tp);
        res.set_redirect("/trades?msg=Trade+" + std::to_string(id) + "+updated", 303);
    });
}
