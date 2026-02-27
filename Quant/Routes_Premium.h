#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <sstream>
#include <cstdio>
#include <array>

// Run a command and capture its stdout
static inline std::string execCmd(const std::string& cmd)
{
    std::string result;
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return "";
    std::array<char, 256> buf;
    while (fgets(buf.data(), (int)buf.size(), pipe))
        result += buf.data();
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    // trim trailing whitespace
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
        result.pop_back();
    return result;
}

inline void registerPremiumRoutes(httplib::Server& svr, AppContext& ctx)
{
    // ========== GET /premium ==========
    svr.Get("/premium", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = ctx.currentUser(req);
        if (user.empty()) { res.set_redirect("/login", 303); return; }

        std::ostringstream pg;
        pg << std::fixed << std::setprecision(8);
        pg << html::msgBanner(req) << html::errBanner(req);
        pg << "<h1>Premium</h1>";

        if (ctx.users.isAdmin(user))
        {
            pg << "<div class='msg'>Admin accounts have permanent premium access.</div>";
            res.set_content(html::wrap("Premium", pg.str()), "text/html");
            return;
        }

        if (ctx.users.isPremium(user))
        {
            pg << "<div class='msg'>You have premium access! Unlimited trades enabled.</div>";
            res.set_content(html::wrap("Premium", pg.str()), "text/html");
            return;
        }

        double priceMbtc = ctx.config.premiumPriceMbtc;
        double priceBtc  = ctx.config.premiumPriceBtc();
        int freeLimit    = ctx.config.freeTradeLimit;
        int tradeCount   = (int)ctx.defaultDb.loadTrades().size();
        int remaining    = freeLimit - tradeCount;
        if (remaining < 0) remaining = 0;

        pg << "<div class='row'>"
              "<div class='stat'><div class='lbl'>Price</div><div class='val'>"
           << priceMbtc << " mBTC</div></div>"
              "<div class='stat'><div class='lbl'>Free Limit</div><div class='val'>"
           << freeLimit << "</div></div>"
              "<div class='stat'><div class='lbl'>Your Trades</div><div class='val'>"
           << tradeCount << "</div></div>"
              "<div class='stat'><div class='lbl'>Remaining</div><div class='val'>"
           << remaining << "</div></div>"
              "</div>";

        pg << "<div class='card' style='margin-top:16px;'>"
              "<h3>Buy Premium</h3>"
              "<p style='color:#64748b;font-size:0.85em;'>"
              "Premium unlocks unlimited trades. Payment is via Bitcoin (Electrum).</p>"
              "<p style='color:#c9a44a;font-size:1.1em;font-weight:bold;'>"
           << priceMbtc << " mBTC (" << priceBtc << " BTC)</p>";

        // check for existing pending payment
        auto* pending = ctx.users.findPendingPayment(user);
        if (pending)
        {
            pg << "<div style='margin:12px 0;padding:12px;background:#0b1426;border:1px solid #1a2744;border-radius:6px;'>"
                  "<div style='color:#64748b;font-size:0.8em;margin-bottom:4px;'>Send exactly:</div>"
                  "<div style='color:#c9a44a;font-size:1.1em;font-weight:bold;'>"
               << pending->requiredBtc << " BTC</div>"
                  "<div style='color:#64748b;font-size:0.8em;margin:8px 0 4px;'>To address:</div>"
                  "<div style='font-family:monospace;color:#58a6ff;font-size:0.95em;word-break:break-all;'>"
               << html::esc(pending->btcAddress) << "</div>"
                  "</div>"
                  "<form method='POST' action='/premium/check'>"
                  "<button style='background:#166534;color:#fff;border:none;padding:8px 20px;"
                  "border-radius:4px;cursor:pointer;font-family:inherit;margin-top:8px;'>"
                  "Check Payment</button></form>"
                  "<form method='POST' action='/premium/cancel' style='margin-top:6px;'>"
                  "<button style='background:#7f1d1d;color:#fca5a5;border:none;padding:6px 16px;"
                  "border-radius:4px;cursor:pointer;font-family:inherit;font-size:0.82em;'>"
                  "Cancel</button></form>";
        }
        else
        {
            pg << "<form method='POST' action='/premium/buy'>"
                  "<button style='background:#1e40af;color:#fff;border:none;padding:10px 24px;"
                  "border-radius:4px;cursor:pointer;font-family:inherit;font-size:0.95em;'>"
                  "Generate Payment Address</button></form>";
        }
        pg << "</div>";
        res.set_content(html::wrap("Premium", pg.str()), "text/html");
    });

    // ========== POST /premium/buy — generate address ==========
    svr.Post("/premium/buy", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = ctx.currentUser(req);
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        if (ctx.users.isPremium(user) || ctx.users.isAdmin(user))
        {
            res.set_redirect("/premium?msg=Already+premium", 303);
            return;
        }

        // remove any existing pending for this user
        ctx.users.removePendingPayment(user);

        // generate new address via electrum CLI
        std::string cmd = ctx.config.electrumCmd() + " createnewaddress";
        std::string address = execCmd(cmd);

        if (address.empty() || address.find("Error") != std::string::npos ||
            address.find("error") != std::string::npos)
        {
            res.set_redirect("/premium?err=" + urlEnc(
                "Electrum error: " + (address.empty() ? "no output" : address)), 303);
            return;
        }

        UserManager::PendingPayment pp;
        pp.username    = user;
        pp.btcAddress  = address;
        pp.requiredBtc = ctx.config.premiumPriceBtc();
        pp.createdAt   = std::chrono::system_clock::now().time_since_epoch().count() > 0
                         ? [&]() {
                             auto now = std::chrono::system_clock::now();
                             auto t = std::chrono::system_clock::to_time_t(now);
                             std::tm tm{};
#ifdef _WIN32
                             localtime_s(&tm, &t);
#else
                             localtime_r(&t, &tm);
#endif
                             std::ostringstream ss;
                             ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
                             return ss.str();
                         }() : "";
        ctx.users.addPendingPayment(pp);

        res.set_redirect("/premium?msg=Address+generated", 303);
    });

    // ========== POST /premium/check — verify payment ==========
    svr.Post("/premium/check", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = ctx.currentUser(req);
        if (user.empty()) { res.set_redirect("/login", 303); return; }

        auto* pending = ctx.users.findPendingPayment(user);
        if (!pending)
        {
            res.set_redirect("/premium?err=No+pending+payment", 303);
            return;
        }

        // check balance via electrum CLI
        std::string cmd = ctx.config.electrumCmd() + " getaddressbalance " + pending->btcAddress;
        std::string output = execCmd(cmd);

        // parse confirmed balance from electrum output
        // output format: {"confirmed": "0.005", "unconfirmed": "0.0"}
        double confirmed = 0.0;
        auto cpos = output.find("\"confirmed\"");
        if (cpos != std::string::npos)
        {
            auto qstart = output.find('"', cpos + 11);
            if (qstart != std::string::npos)
            {
                qstart++;
                auto qend = output.find('"', qstart);
                if (qend != std::string::npos)
                {
                    try { confirmed = std::stod(output.substr(qstart, qend - qstart)); }
                    catch (...) {}
                }
            }
        }

        if (confirmed >= pending->requiredBtc)
        {
            ctx.users.setPremium(user, true);
            ctx.users.removePendingPayment(user);
            res.set_redirect("/premium?msg=Payment+confirmed!+Premium+activated.", 303);
        }
        else
        {
            std::ostringstream msg;
            msg << std::fixed << std::setprecision(8);
            msg << "Not enough confirmed yet. Got " << confirmed
                << " BTC, need " << pending->requiredBtc << " BTC";
            res.set_redirect("/premium?err=" + urlEnc(msg.str()), 303);
        }
    });

    // ========== POST /premium/cancel ==========
    svr.Post("/premium/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = ctx.currentUser(req);
        if (!user.empty())
            ctx.users.removePendingPayment(user);
        res.set_redirect("/premium?msg=Payment+cancelled", 303);
    });
}
