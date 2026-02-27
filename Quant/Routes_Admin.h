#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <sstream>

inline void registerAdminRoutes(httplib::Server& svr, AppContext& ctx)
{
    // ========== GET /admin ==========
    svr.Get("/admin", [&](const httplib::Request& req, httplib::Response& res) {
        if (!ctx.isAdmin(req)) { res.set_redirect("/?err=Admin+only", 303); return; }

        std::ostringstream pg;
        pg << html::msgBanner(req) << html::errBanner(req);
        pg << "<h1>Admin Panel</h1>";

        // config form
        pg << "<form class='card' method='POST' action='/admin/config'>"
              "<h3>Platform Settings</h3>"
              "<label>Premium Price (mBTC)</label>"
              "<input type='number' name='premiumPriceMbtc' step='any' value='"
           << ctx.config.premiumPriceMbtc << "'><br>"
              "<label>Free Trade Limit</label>"
              "<input type='number' name='freeTradeLimit' value='"
           << ctx.config.freeTradeLimit << "'><br>"
              "<label>Electrum CLI Path</label>"
              "<input type='text' name='electrumPath' value='"
           << html::esc(ctx.config.electrumPath) << "'><br>"
              "<label>Electrum Wallet File</label>"
              "<input type='text' name='electrumWallet' value='"
           << html::esc(ctx.config.electrumWallet) << "'>"
              "<div style='color:#64748b;font-size:0.78em;'>Leave empty for default wallet</div>"
              "<button>Save Config</button></form>";

        // user list
        pg << "<h2>Users</h2>"
              "<table><tr><th>Username</th><th>Email</th><th>Created</th>"
              "<th>Premium</th><th>Admin</th><th>Actions</th></tr>";
        for (const auto& u : ctx.users.users())
        {
            bool adm = ctx.users.isAdmin(u.username);
            pg << "<tr><td>" << html::esc(u.username) << "</td>"
               << "<td>" << html::esc(u.email) << "</td>"
               << "<td>" << html::esc(u.createdAt) << "</td>"
               << "<td class='" << (u.premium ? "buy" : "off") << "'>"
               << (u.premium ? "YES" : "NO") << "</td>"
               << "<td>" << (adm ? "ADMIN" : "-") << "</td>"
               << "<td>";
            if (!adm)
            {
                pg << "<form class='iform' method='POST' action='/admin/toggle-premium'>"
                   << "<input type='hidden' name='username' value='" << html::esc(u.username) << "'>"
                   << "<button class='btn-sm " << (u.premium ? "btn-warn" : "btn-sm") << "'>"
                   << (u.premium ? "Revoke" : "Grant") << "</button></form>";
            }
            pg << "</td></tr>";
        }
        pg << "</table>";

        // pending payments
        const auto& pp = ctx.users.pendingPayments();
        if (!pp.empty())
        {
            pg << "<h2>Pending Payments</h2>"
                  "<table><tr><th>User</th><th>Address</th><th>Required BTC</th>"
                  "<th>Created</th></tr>";
            for (const auto& p : pp)
            {
                pg << "<tr><td>" << html::esc(p.username) << "</td>"
                   << "<td style='font-family:monospace;font-size:0.8em;'>"
                   << html::esc(p.btcAddress) << "</td>"
                   << "<td>" << std::fixed << std::setprecision(8) << p.requiredBtc << "</td>"
                   << "<td>" << html::esc(p.createdAt) << "</td></tr>";
            }
            pg << "</table>";
        }

        res.set_content(html::wrap("Admin", pg.str()), "text/html");
    });

    // ========== POST /admin/config ==========
    svr.Post("/admin/config", [&](const httplib::Request& req, httplib::Response& res) {
        if (!ctx.isAdmin(req)) { res.set_redirect("/", 303); return; }
        auto f = parseForm(req.body);
        ctx.config.premiumPriceMbtc = fd(f, "premiumPriceMbtc", ctx.config.premiumPriceMbtc);
        ctx.config.freeTradeLimit   = fi(f, "freeTradeLimit", ctx.config.freeTradeLimit);
        auto ep = fv(f, "electrumPath");
        if (!ep.empty()) ctx.config.electrumPath = ep;
        ctx.config.electrumWallet = fv(f, "electrumWallet");
        ctx.config.save();
        res.set_redirect("/admin?msg=Config+saved", 303);
    });

    // ========== POST /admin/toggle-premium ==========
    svr.Post("/admin/toggle-premium", [&](const httplib::Request& req, httplib::Response& res) {
        if (!ctx.isAdmin(req)) { res.set_redirect("/", 303); return; }
        auto f = parseForm(req.body);
        std::string username = fv(f, "username");
        if (username.empty() || ctx.users.isAdmin(username))
        {
            res.set_redirect("/admin?err=Invalid+user", 303);
            return;
        }
        bool current = ctx.users.isPremium(username);
        ctx.users.setPremium(username, !current);
        res.set_redirect("/admin?msg=" + urlEnc(username) + "+premium+" +
            (current ? "revoked" : "granted"), 303);
    });
}
