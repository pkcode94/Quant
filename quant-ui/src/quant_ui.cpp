// ============================================================
// quant_ui.cpp — Virtual router implementation
//
// Renders complete HTML pages from engine state.
// No HTTP, no httplib, no network — pure string generation.
// ============================================================

#include "quant_ui.h"
#include "quant_engine.h"
#include "quant_engine_types.h"

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <functional>

// ---- Global config ----
static std::string g_appName = "Quant";
static std::string g_navOverride;

// ---- String helpers ----

static std::string escHtml(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '<') o += "&lt;";
        else if (c == '>') o += "&gt;";
        else if (c == '&') o += "&amp;";
        else if (c == '"') o += "&quot;";
        else o += c;
    }
    return o;
}

static std::string urlDecode(const std::string& s)
{
    std::string o;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size())
        {
            auto hv = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
            };
            o += (char)(hv(s[i + 1]) * 16 + hv(s[i + 2]));
            i += 2;
        }
        else o += s[i];
    }
    return o;
}

static std::string urlEncode(const std::string& s)
{
    std::ostringstream o;
    for (unsigned char c : s)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            o << c;
        else
            o << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0xF];
    }
    return o.str();
}

static std::map<std::string, std::string> parseQS(const char* qs)
{
    std::map<std::string, std::string> m;
    if (!qs || !*qs) return m;
    std::istringstream ss(qs);
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        auto eq = pair.find('=');
        if (eq != std::string::npos)
            m[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
    }
    return m;
}

static std::string qsVal(const std::map<std::string, std::string>& m, const std::string& k)
{
    auto it = m.find(k);
    return it != m.end() ? it->second : "";
}

static double qsDbl(const std::map<std::string, std::string>& m, const std::string& k, double def = 0.0)
{
    auto it = m.find(k);
    if (it == m.end() || it->second.empty()) return def;
    try { return std::stod(it->second); } catch (...) { return def; }
}

static int qsInt(const std::map<std::string, std::string>& m, const std::string& k, int def = 0)
{
    auto it = m.find(k);
    if (it == m.end() || it->second.empty()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

// ---- CSS (phone-first terminal ledger theme) ----

static std::string css()
{
    return
        "<style>"
        "*{box-sizing:border-box;}"
        "body{font-family:'Courier New','Fira Code',monospace;background:#000;color:#c0c0c0;margin:0;padding:0;}"
        "nav{background:#0a0a0a;padding:6px 10px;border-bottom:1px solid #00e676;"
        "display:flex;gap:2px;flex-wrap:wrap;overflow-x:auto;-webkit-overflow-scrolling:touch;}"
        "nav a{color:#00e676;text-decoration:none;font-size:0.75em;padding:5px 8px;border-radius:3px;white-space:nowrap;}"
        "nav a:hover{background:#0f0f0f;}"
        ".container{max-width:100%;margin:0 auto;padding:10px;}"
        "h1{color:#00e676;margin:0 0 8px 0;font-size:1.05em;letter-spacing:1px;text-transform:uppercase;}"
        "h2{color:#757575;font-size:0.85em;border-bottom:1px solid #1a1a1a;padding-bottom:3px;margin:12px 0 6px 0;}"
        "table{border-collapse:collapse;width:100%;margin:6px 0 14px 0;font-size:0.75em;}"
        "th,td{border:1px solid #1a1a1a;padding:3px 5px;text-align:right;}"
        "th{background:#0a0a0a;color:#ffd600;text-align:left;font-weight:normal;"
        "text-transform:uppercase;font-size:0.72em;letter-spacing:0.5px;}"
        "td{background:#000;}tr:hover td{background:#0a0a0a;}"
        ".buy{color:#00e676;font-weight:bold;}"
        ".sell{color:#ff1744;font-weight:bold;}"
        "form.card{background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:8px;margin:6px 0;}"
        "label{display:inline-block;min-width:90px;color:#757575;font-size:0.75em;}"
        "input,select{background:#050505;border:1px solid #1a1a1a;color:#c0c0c0;padding:3px 5px;"
        "border-radius:3px;margin:2px 3px 2px 0;font-family:inherit;font-size:0.8em;}"
        "input:focus,select:focus{border-color:#00e676;outline:none;box-shadow:0 0 4px #00e67633;}"
        "button,.btn{background:#004d25;color:#00e676;border:1px solid #00e67688;padding:3px 8px;"
        "border-radius:3px;cursor:pointer;font-size:0.75em;font-family:inherit;"
        "text-decoration:none;display:inline-block;}"
        "button:hover,.btn:hover{background:#006b35;border-color:#00e676;}"
        ".btn-danger{background:#3a0000;color:#ff1744;border-color:#ff174488;}"
        ".btn-danger:hover{background:#5a0000;border-color:#ff1744;}"
        ".row{display:flex;gap:6px;flex-wrap:wrap;}"
        ".stat{background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;"
        "padding:6px 10px;min-width:100px;flex:1;}"
        ".stat .lbl{color:#757575;font-size:0.68em;text-transform:uppercase;letter-spacing:0.5px;}"
        ".stat .val{font-size:1.05em;color:#00e676;font-weight:bold;}"
        ".msg{background:#001a0a;border:1px solid #00e676;padding:4px 8px;"
        "border-radius:3px;margin:6px 0;color:#00e676;font-size:0.8em;}"
        ".err{background:#1a0000;border-color:#ff1744;color:#ff1744;}"
        "input[type=number]{width:75px;}"
        "input[type=text]{width:100px;}"
        ".forms-row{display:flex;gap:6px;flex-wrap:wrap;}"
        ".forms-row form.card{flex:1;min-width:240px;}"
        "@media(max-width:600px){"
        ".container{padding:6px;}"
        "table{font-size:0.7em;}"
        "th,td{padding:2px 3px;}"
        ".stat{min-width:80px;padding:4px 6px;}"
        ".stat .val{font-size:0.95em;}"
        "input[type=number]{width:60px;}"
        "input[type=text]{width:80px;}"
        "}"
        "</style>";
}

static std::string nav()
{
    if (!g_navOverride.empty())
    {
        std::string h = "<nav>";
        std::istringstream ss(g_navOverride);
        std::string line;
        while (std::getline(ss, line, '\n'))
        {
            auto tab = line.find('\t');
            if (tab == std::string::npos) continue;
            std::string label = line.substr(0, tab);
            std::string href  = line.substr(tab + 1);
            h += "<a href='" + escHtml(href) + "'>" + escHtml(label) + "</a>";
        }
        return h + "</nav>";
    }
    return
        "<nav>"
        "<a href='/' style='color:#ffd600;'>&#9939; Home</a>"
        "<a href='/dashboard'>Dashboard</a>"
        "<a href='/trades'>Trades</a>"
        "<a href='/wallet'>Wallet</a>"
        "<a href='/profit'>P&amp;L</a>"
        "<a href='/serial-generator'>Serial Gen</a>"
        "<a href='/entry-points'>Entries</a>"
        "<a href='/exits'>Exits</a>"
        "<a href='/price-check' style='color:#ffd600;'>&#9733; Check</a>"
        "<a href='/chains' style='color:#f472b6;'>&#9939; Chains</a>"
        "<a href='/overhead'>OH</a>"
        "<a href='/mcp-daemon' style='color:#38bdf8;'>&#9889; MCP</a>"
#ifndef QUANT_PUBLIC_BUILD
        "<a href='/sync' style='color:#ffd600;'>&#9729; Sync</a>"
#endif
        "</nav>";
}

static std::string wrapPage(const std::string& title, const std::string& body)
{
    return "<!DOCTYPE html><html><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>" + escHtml(title) + " - " + escHtml(g_appName) + "</title>"
           + css() +
           "</head><body>" + nav() +
           "<div class='container'>" + body + "</div>"
           "</body></html>";
}

static std::string msgBanner(const std::map<std::string, std::string>& qs)
{
    auto it = qs.find("msg");
    if (it == qs.end()) return "";
    return "<div class='msg'>" + escHtml(it->second) + "</div>";
}

static std::string errBanner(const std::map<std::string, std::string>& qs)
{
    auto it = qs.find("err");
    if (it == qs.end()) return "";
    return "<div class='msg err'>" + escHtml(it->second) + "</div>";
}

// ---- Response helpers ----

static QUIResponse makeHtml(const std::string& html, int code = 200)
{
    QUIResponse r{};
    r.statusCode = code;
    r.html = (char*)std::malloc(html.size() + 1);
    if (r.html) std::memcpy(r.html, html.c_str(), html.size() + 1);
    r.redirect[0] = '\0';
    return r;
}

static QUIResponse makeRedirect(const std::string& url, int code = 303)
{
    QUIResponse r{};
    r.statusCode = code;
    r.html = nullptr;
    size_t len = std::min(url.size(), sizeof(r.redirect) - 1);
    std::memcpy(r.redirect, url.c_str(), len);
    r.redirect[len] = '\0';
    return r;
}

static QUIResponse make404(const std::string& path)
{
    std::string body = "<h1>404</h1><p>Page not found: " + escHtml(path) + "</p>"
                       "<a class='btn' href='/dashboard'>Dashboard</a>";
    return makeHtml(wrapPage("Not Found", body), 404);
}

// ============================================================
// Page renderers
// ============================================================

// ---- Home / Landing ----

static QUIResponse pageHome(QEngine* /*e*/, const std::map<std::string, std::string>& /*qs*/)
{
    std::ostringstream h;

    // ---- Hero ----
    h << "<div style='text-align:center;padding:20px 0 12px 0;'>"
         "<h1 style='font-size:1.4em;letter-spacing:2px;margin-bottom:4px;'>&#9939; QUANT LEDGER</h1>"
         "<p style='color:#757575;font-size:0.82em;margin:0;'>Don't just trade &mdash; keep track of them.</p>"
         "</div>";

    // ---- Intro ----
    h << "<div style='max-width:520px;margin:0 auto;'>"
         "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:10px 0;'>"
         "<p style='font-size:0.82em;line-height:1.6;margin:0;'>"
         "This ledger lets you record your trades alongside their take-profit "
         "and stop-loss levels &mdash; or go further and augment your trading strategy "
         "with our <span style='color:#00e676;'>open-source risk management system</span>.</p>"
         "</div>";

    // ---- Why track? ----
    h << "<h2 style='margin-top:16px;'>Why track trades yourself?</h2>"
         "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:8px 0;'>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0 0 8px 0;'>"
         "Your broker tracks positions, fills, and P&amp;L for you &mdash; but only "
         "for assets <em>on that broker</em>. What about physical gold, real estate, "
         "private deals, or tokens on a DEX with no portfolio view?</p>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0 0 8px 0;'>"
         "Quant Ledger is a <strong style='color:#ffd600;'>single source of truth</strong> "
         "across every asset class. Record the buy, set your exits, watch the math, "
         "and own your data &mdash; no API keys, no broker lock-in, no subscriptions.</p>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0;'>"
#ifndef QUANT_PUBLIC_BUILD
         "Try it for free. The only paid feature is optional "
         "<a href='/sync' style='color:#ffd600;'>cloud sync</a> &mdash; "
         "encrypt your ledger and store it on your own server.</p>"
#else
         "Try it for free. Your data stays entirely on your device &mdash; "
         "no accounts, no telemetry, no cloud.</p>"
#endif
         "</div>";

    // ---- Feature list ----
    h << "<h2>What you get</h2>";

    auto feature = [&](const char* icon, const char* title, const char* desc) {
        h << "<div style='display:flex;gap:10px;align-items:flex-start;margin:8px 0;'>"
             "<span style='color:#00e676;font-size:1.1em;'>" << icon << "</span>"
             "<div><strong style='color:#00e676;font-size:0.82em;'>" << title << "</strong>"
             "<div style='color:#757575;font-size:0.75em;line-height:1.5;'>" << desc << "</div>"
             "</div></div>";
    };

    feature("&#9654;", "Trade Journal",
            "Record buys and sells with price, quantity, fees, and timestamps. "
            "Import historical trades or execute new ones that move your wallet balance.");

    feature("&#9650;", "Exit Strategies",
            "Attach take-profit and stop-loss levels to every position. "
            "Set multiple exit points per trade &mdash; partial sells at different targets. "
            "Toggle stop-losses on or off as the market moves.");

    feature("&#9733;", "Profit &amp; Loss",
            "Instant P&amp;L calculation for any trade at any price. "
            "Net profit, gross profit, ROI &mdash; with fee deductions built in.");

    feature("&#9881;", "Serial Generator",
            "Model compound-growth strategies: define an entry price, overhead, "
            "exit fraction, and steepness &mdash; generate a full plan of scaled entries "
            "with calculated take-profits and sell quantities.");

    feature("&#9830;", "Entry Points",
            "Plan where to enter the market before you commit capital. "
            "Store target prices, quantities, and risk parameters. "
            "Feed entries into managed chains for systematic execution.");

    feature("&#9939;", "Managed Chains",
            "Group entries into cycles. Activate a chain, add entries to each cycle, "
            "track savings rate and reinvestment. "
            "The chain manager handles the bookkeeping of compounding strategies.");

    #ifndef QUANT_PUBLIC_BUILD
        feature("&#9729;", "Encrypted Cloud Sync",
                "Optionally sync your ledger to your server. "
                "Client-side AES-256-GCM encryption &mdash; the server never sees your data. "
                "Free tier: 5 MB. Upgrade via Bitcoin for more storage.");
    #endif

    feature("&#9783;", "Overhead Calculator",
            "Compute the real cost of a trade: spreads, hedging fees, time decay, "
            "and opportunity cost across your portfolio. "
            "Know your breakeven before you click buy.");

    // ---- Workflow ----
    h << "<h2>Workflow</h2>"
         "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:8px 0;"
         "font-size:0.78em;line-height:1.7;'>"
         "<span style='color:#ffd600;'>1.</span> "
         "<strong style='color:#00e676;'>Fund your wallet</strong> &mdash; "
         "deposit your starting capital so the ledger can track balances.<br>"
         "<span style='color:#ffd600;'>2.</span> "
         "<strong style='color:#00e676;'>Plan entries</strong> &mdash; "
         "use the Serial Generator to model a strategy, or add entry points manually.<br>"
         "<span style='color:#ffd600;'>3.</span> "
         "<strong style='color:#00e676;'>Execute or import trades</strong> &mdash; "
         "<em>Execute Buy</em> debits your wallet; <em>Import Buy</em> records without moving funds.<br>"
         "<span style='color:#ffd600;'>4.</span> "
         "<strong style='color:#00e676;'>Set exits</strong> &mdash; "
         "attach TP/SL levels to each buy. Partial sells at different price targets.<br>"
         "<span style='color:#ffd600;'>5.</span> "
         "<strong style='color:#00e676;'>Close positions</strong> &mdash; "
         "<em>Execute Sell</em> credits your wallet; the ledger calculates realized P&amp;L automatically.<br>"
         "<span style='color:#ffd600;'>6.</span> "
         "<strong style='color:#00e676;'>Repeat &amp; compound</strong> &mdash; "
         "use managed chains to roll profits into the next cycle with a savings rate.<br>"
#ifndef QUANT_PUBLIC_BUILD
         "<span style='color:#ffd600;'>7.</span> "
         "<strong style='color:#00e676;'>Sync (optional)</strong> &mdash; "
         "encrypt your database and back it up to your server. Restore on any device."
#endif
         "</div>";

    // ---- Philosophy ----
    h << "<h2>Philosophy</h2>"
         "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:8px 0;'>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0 0 8px 0;'>"
         "Quant Ledger is <strong style='color:#00e676;'>open source</strong> and runs "
         "entirely on your device. There is no account, no telemetry, no cloud requirement. "
         "Your trades are your business.</p>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0 0 8px 0;'>"
         "The risk management tools &mdash; serial plans, overhead calculations, chain compounding "
         "&mdash; exist because trading is not just about entries. "
         "Knowing <em>when to sell</em>, <em>how much to sell</em>, and <em>what it really costs</em> "
         "is what separates a journal from a strategy.</p>"
         "<p style='font-size:0.8em;line-height:1.6;margin:0;'>"
         "We don't connect to brokers, don't stream prices, and don't offer signals. "
         "This is a <span style='color:#ffd600;'>calculator and a notebook</span> &mdash; "
         "you bring the decisions.</p>"
         "</div>";

    // ---- CTA ----
    h << "<div style='text-align:center;padding:16px 0;'>"
         "<a class='btn' href='/dashboard' style='padding:8px 20px;font-size:0.9em;'>"
         "&#9654; Open Dashboard</a>"
         "</div>"
         "</div>"; // close max-width wrapper

    return makeHtml(wrapPage("Home", h.str()));
}

// ---- Dashboard ----

static QUIResponse pageDashboard(QEngine* e, const std::map<std::string, std::string>& qs)
{
    QWalletInfo w = qe_wallet_info(e);
    int nTrades  = qe_trade_count(e);
    int nEntries = qe_entry_count(e);
    int nChains  = qe_chain_count(e);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>" << escHtml(g_appName) << " Dashboard</h1>";

    int nExits   = qe_exitpt_count(e);

    h << "<div class='row'>"
         "<div class='stat'><div class='lbl'>Liquid</div><div class='val'>" << w.balance << "</div></div>"
         "<div class='stat'><div class='lbl'>Deployed</div><div class='val'>" << w.deployed << "</div></div>"
         "<div class='stat'><div class='lbl'>Total</div><div class='val'>" << w.total << "</div></div>"
         "<div class='stat'><div class='lbl'>Trades</div><div class='val'>" << nTrades << "</div></div>"
         "<div class='stat'><div class='lbl'>Entries</div><div class='val'>" << nEntries << "</div></div>"
         "<div class='stat'><div class='lbl'>Exits</div><div class='val'>" << nExits << "</div></div>"
         "<div class='stat'><div class='lbl'>Chains</div><div class='val'>" << nChains << "</div></div>"
         "</div>";

    h << "<h2>Quick Actions</h2>"
         "<div class='row'>"
         "<a class='btn' href='/wallet'>Wallet</a> "
         "<a class='btn' href='/trades'>Trades</a> "
         "<a class='btn' href='/exits'>Exit Strategies</a> "
         "<a class='btn' href='/price-check' style='background:#1a1a00;color:#ffd600;border-color:#ffd600;'>&#9733; Price Check</a> "
         "<a class='btn' href='/serial-generator'>Serial Generator</a> "
         "<a class='btn' href='/entry-points'>Entry Points</a> "
         "<a class='btn' href='/chains'>Chains</a>"
         "</div>";

    return makeHtml(wrapPage("Dashboard", h.str()));
}

// ---- Wallet ----

static QUIResponse pageWallet(QEngine* e, const std::map<std::string, std::string>& qs)
{
    QWalletInfo w = qe_wallet_info(e);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Wallet</h1>";

    h << "<div class='row'>"
         "<div class='stat'><div class='lbl'>Liquid Balance</div><div class='val'>" << w.balance << "</div></div>"
         "<div class='stat'><div class='lbl'>Deployed</div><div class='val'>" << w.deployed << "</div></div>"
         "<div class='stat'><div class='lbl'>Total Capital</div><div class='val'>" << w.total << "</div></div>"
         "</div>";

    h << "<div class='forms-row'>"
         "<form class='card' method='POST' action='/wallet/deposit'>"
         "<label>Amount</label><input type='number' name='amount' step='any' required>"
         "<button>Deposit</button></form>"
         "<form class='card' method='POST' action='/wallet/withdraw'>"
         "<label>Amount</label><input type='number' name='amount' step='any' required>"
         "<button>Withdraw</button></form>"
         "</div>";

    h << "<br><a class='btn' href='/dashboard'>Back</a>";
    return makeHtml(wrapPage("Wallet", h.str()));
}

static QUIResponse postWalletDeposit(QEngine* e, const std::map<std::string, std::string>& form)
{
    double amt = qsDbl(form, "amount");
    if (amt > 0) qe_wallet_deposit(e, amt);
    return makeRedirect("/wallet?msg=Deposited+" + std::to_string(amt));
}

static QUIResponse postWalletWithdraw(QEngine* e, const std::map<std::string, std::string>& form)
{
    double amt = qsDbl(form, "amount");
    QWalletInfo w = qe_wallet_info(e);
    if (amt > w.balance)
        return makeRedirect("/wallet?err=Insufficient+balance");
    if (amt > 0) qe_wallet_withdraw(e, amt);
    return makeRedirect("/wallet?msg=Withdrew+" + std::to_string(amt));
}

// ---- Trades ----

static void copySymbol(char* dst, size_t dstSize, const std::string& src)
{
    size_t len = std::min(src.size(), dstSize - 1);
    std::memcpy(dst, src.c_str(), len);
    dst[len] = '\0';
}

static QUIResponse pageTrades(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int n = qe_trade_count(e);
    std::vector<QTrade> trades(n > 0 ? n : 1);
    n = qe_trade_list(e, trades.data(), n);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Trades (" << n << ")</h1>";

    if (n > 0)
    {
        // Separate parents (Buy) and orphan sells
        h << "<table><tr>"
             "<th>ID</th><th>Symbol</th><th>Type</th><th>Price</th><th>Qty</th>"
             "<th>Cost</th><th>Fees</th><th>Sold</th><th>Rem</th><th>Actions</th>"
             "</tr>";

        for (int i = 0; i < n; ++i)
        {
            const auto& t = trades[i];
            if (t.type != Q_BUY) continue; // render buys first

            double sold = qe_trade_sold_qty(e, t.tradeId);
            double remaining = t.quantity - sold;
            double cost = t.value * t.quantity;

            // ---- parent BUY row ----
            h << "<tr>"
              << "<td><strong>" << t.tradeId << "</strong></td>"
              << "<td><strong>" << escHtml(t.symbol) << "</strong></td>"
              << "<td class='buy'>BUY</td>"
              << "<td>" << t.value << "</td>"
              << "<td>" << t.quantity << "</td>"
              << "<td>" << cost << "</td>"
              << "<td>" << (t.buyFees + t.sellFees) << "</td>"
              << "<td>" << sold << "</td>"
              << "<td>" << remaining << "</td>"
              << "<td>"
              << "<a class='btn' style='padding:2px 6px;font-size:0.75em;' href='/edit-trade?id=" << t.tradeId << "'>Edit</a> "
              << "<a class='btn' style='padding:2px 6px;font-size:0.75em;' href='/profit?tradeId=" << t.tradeId << "'>P&amp;L</a> "
              << "<form method='POST' action='/trades/delete' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='tradeId' value='" << t.tradeId << "'>"
                 "<button class='btn-danger' style='padding:2px 6px;font-size:0.75em;'>Del</button></form>"
              << "</td></tr>";

            // ---- exit strategy rows for this buy ----
            int xn = 0;
            {
                // count first
                std::vector<QExitPointData> tmp(32);
                xn = qe_exitpt_list_for_trade(e, t.tradeId, tmp.data(), 32);
                tmp.resize(xn > 0 ? xn : 0);
                for (int xi = 0; xi < xn; ++xi)
                {
                    const auto& xp = tmp[xi];
                    h << "<tr style='color:#64748b;font-size:0.85em;'>"
                      << "<td>&#9500; X[" << xp.levelIndex << "]</td>"
                      << "<td></td><td style='font-size:0.78em;'>EXIT</td>";
                    if (!xp.executed)
                    {
                        // Editable TP
                        h << "<td><form method='POST' action='/exit/set-tp' style='margin:0;display:inline;'>"
                             "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                             "<input type='number' name='tp' step='any' value='" << xp.tpPrice << "' style='width:70px;'>"
                             "<button class='btn' style='padding:1px 4px;font-size:0.7em;'>&#10003;</button></form></td>";
                        // Editable qty
                        h << "<td><form method='POST' action='/exit/set-qty' style='margin:0;display:inline;'>"
                             "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                             "<input type='number' name='qty' step='any' value='" << xp.sellQty << "' style='width:55px;'>"
                             "<button class='btn' style='padding:1px 4px;font-size:0.7em;'>&#10003;</button></form></td>";
                        h << "<td></td><td></td>";
                        // Editable SL
                        h << "<td><form method='POST' action='/exit/set-sl' style='margin:0;display:inline;'>"
                             "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                             "<input type='number' name='sl' step='any' value='" << xp.slPrice << "' style='width:70px;'>"
                             "<button class='btn' style='padding:1px 4px;font-size:0.7em;'>&#10003;</button></form></td>";
                        // SL toggle + status + delete
                        h << "<td>"
                          << "<span class='" << (xp.executed ? "off" : "buy") << "'>PENDING</span> "
                          << "<form method='POST' action='/exit/toggle-sl' style='margin:0;display:inline;'>"
                             "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                             "<input type='hidden' name='active' value='" << (xp.slActive ? "0" : "1") << "'>"
                             "<button class='btn" << (xp.slActive ? "" : " btn-danger") << "' style='padding:1px 4px;font-size:0.7em;'>SL:" << (xp.slActive ? "ON" : "OFF") << "</button></form> "
                          << "<form method='POST' action='/exit/delete' style='margin:0;display:inline;'>"
                             "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                             "<button class='btn-danger' style='padding:1px 4px;font-size:0.7em;'>Del</button></form>"
                          << "</td></tr>";
                    }
                    else
                    {
                        h << "<td>" << xp.tpPrice << "</td>"
                          << "<td>" << xp.sellQty << "</td>"
                          << "<td></td><td></td>"
                          << "<td>" << xp.slPrice << "</td>"
                          << "<td class='off'>DONE (#" << xp.linkedSellId << ")</td></tr>";
                    }
                }
            }
            // Add exit point row
            h << "<tr style='font-size:0.82em;'><td colspan='10'>"
                 "<form method='POST' action='/exit/add' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='tradeId' value='" << t.tradeId << "'>"
                 "<input type='hidden' name='symbol' value='" << escHtml(t.symbol) << "'>"
                 "<input type='number' name='tp' step='any' placeholder='TP price' style='width:70px;' required> "
                 "<input type='number' name='sl' step='any' placeholder='SL price' style='width:70px;' value='0'> "
                 "<input type='number' name='qty' step='any' placeholder='Sell qty' style='width:60px;' required> "
                 "<button class='btn' style='padding:2px 6px;font-size:0.8em;'>+ Exit</button></form>"
                 "</td></tr>";

            // ---- child SELL rows ----
            for (int j = 0; j < n; ++j)
            {
                const auto& c = trades[j];
                if (c.type != Q_COVERED_SELL) continue;
                // check if parent by inspecting parentTradeId field from QTrade
                // Unfortunately QTrade doesn't carry parentTradeId, so we match by symbol
                // We'll show all sells after all buys separately
            }
        }

        // ---- Sell trades ----
        for (int i = 0; i < n; ++i)
        {
            const auto& t = trades[i];
            if (t.type != Q_COVERED_SELL) continue;
            double cost = t.value * t.quantity;
            h << "<tr>"
              << "<td>" << t.tradeId << "</td>"
              << "<td>" << escHtml(t.symbol) << "</td>"
              << "<td class='sell'>SELL</td>"
              << "<td>" << t.value << "</td>"
              << "<td>" << t.quantity << "</td>"
              << "<td>" << cost << "</td>"
              << "<td>" << (t.buyFees + t.sellFees) << "</td>"
              << "<td>-</td><td>-</td>"
              << "<td>"
              << "<a class='btn' style='padding:2px 6px;font-size:0.75em;' href='/edit-trade?id=" << t.tradeId << "'>Edit</a> "
              << "<form method='POST' action='/trades/delete' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='tradeId' value='" << t.tradeId << "'>"
                 "<button class='btn-danger' style='padding:2px 6px;font-size:0.75em;'>Del</button></form>"
              << "</td></tr>";
        }

        h << "</table>";
    }
    else
    {
        h << "<p style='color:#475569;'>No trades yet.</p>";
    }

    // ---- Forms row: Execute Buy, Execute Sell, Import Buy, Import Sell ----
    h << "<div class='forms-row'>";

    h << "<form class='card' method='POST' action='/execute-buy'>"
         "<h2 style='margin-top:0;'>Execute Buy</h2>"
         "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Creates trade &amp; debits wallet</div>"
         "<label>Symbol</label><input type='text' name='symbol' required><br>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
         "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='0'><br>"
         "<button>Execute Buy</button></form>";

    h << "<form class='card' method='POST' action='/execute-sell'>"
         "<h2 style='margin-top:0;'>Execute Sell</h2>"
         "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Deducts from holdings &amp; credits wallet</div>"
         "<label>Symbol</label><input type='text' name='symbol' required><br>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='quantity' step='any' required><br>"
         "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='0'><br>"
         "<button>Execute Sell</button></form>";

    h << "<form class='card' method='POST' action='/trades/add'>"
         "<h2 style='margin-top:0;'>Import Buy</h2>"
         "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Record only &mdash; no wallet movement</div>"
         "<input type='hidden' name='type' value='buy'>"
         "<label>Symbol</label><input type='text' name='symbol' required><br>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='qty' step='any' required><br>"
         "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='0'><br>"
         "<button>Import Buy</button></form>";

    h << "<form class='card' method='POST' action='/trades/add'>"
         "<h2 style='margin-top:0;'>Import Sell</h2>"
         "<div style='color:#64748b;font-size:0.78em;margin-bottom:8px;'>Record only &mdash; no wallet movement</div>"
         "<input type='hidden' name='type' value='sell'>"
         "<label>Symbol</label><input type='text' name='symbol' required><br>"
         "<label>Parent Buy ID</label><input type='number' name='parentId' required><br>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='qty' step='any' required><br>"
         "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='0'><br>"
         "<button>Import Sell</button></form>";

    h << "</div>";
    return makeHtml(wrapPage("Trades", h.str()));
}

// ---- Trade POST handlers ----

static QUIResponse postTradeAdd(QEngine* e, const std::map<std::string, std::string>& form)
{
    std::string sym = qsVal(form, "symbol");
    if (sym.empty())
        return makeRedirect("/trades?err=" + urlEncode("Symbol is required"));

    QTrade t{};
    copySymbol(t.symbol, sizeof(t.symbol), sym);
    t.type     = (qsVal(form, "type") == "sell") ? Q_COVERED_SELL : Q_BUY;
    t.value    = qsDbl(form, "price");
    t.quantity = qsDbl(form, "qty");
    if (t.value <= 0 || t.quantity <= 0)
        return makeRedirect("/trades?err=" + urlEncode("Price and quantity must be positive"));
    if (t.type == Q_BUY)
    {
        t.buyFees  = qsDbl(form, "buyFee");
        t.sellFees = 0;
    }
    else
    {
        t.buyFees  = 0;
        t.sellFees = qsDbl(form, "sellFee");
    }
    int id = qe_trade_add(e, &t);
    if (id <= 0)
        return makeRedirect("/trades?err=" + urlEncode("Failed to add trade"));
    return makeRedirect("/trades?msg=" + urlEncode("Trade #" + std::to_string(id) + " added"));
}

static QUIResponse postTradeDelete(QEngine* e, const std::map<std::string, std::string>& form)
{
    int id = qsInt(form, "tradeId");
    if (id <= 0)
        return makeRedirect("/trades?err=" + urlEncode("Invalid trade ID"));
    qe_trade_delete(e, id);
    return makeRedirect("/trades?msg=" + urlEncode("Trade #" + std::to_string(id) + " deleted"));
}

static QUIResponse postExecuteBuy(QEngine* e, const std::map<std::string, std::string>& form)
{
    std::string sym = qsVal(form, "symbol");
    double price = qsDbl(form, "price");
    double qty   = qsDbl(form, "quantity");
    double fee   = qsDbl(form, "buyFee");
    if (sym.empty() || price <= 0 || qty <= 0)
        return makeRedirect("/trades?err=" + urlEncode("Invalid buy parameters"));
    int id = qe_execute_buy(e, sym.c_str(), price, qty, fee);
    if (id < 0)
        return makeRedirect("/trades?err=" + urlEncode("Insufficient funds"));
    return makeRedirect("/trades?msg=" + urlEncode("Buy #" + std::to_string(id) + " executed"));
}

static QUIResponse postExecuteSell(QEngine* e, const std::map<std::string, std::string>& form)
{
    std::string sym = qsVal(form, "symbol");
    double price = qsDbl(form, "price");
    double qty   = qsDbl(form, "quantity");
    double fee   = qsDbl(form, "sellFee");
    if (sym.empty() || price <= 0 || qty <= 0)
        return makeRedirect("/trades?err=" + urlEncode("Invalid sell parameters"));
    int id = qe_execute_sell(e, sym.c_str(), price, qty, fee);
    if (id < 0)
        return makeRedirect("/trades?err=" + urlEncode("Sell failed (insufficient holdings)"));
    return makeRedirect("/trades?msg=" + urlEncode("Sell #" + std::to_string(id) + " executed"));
}

// ---- Edit Trade ----

static QUIResponse pageEditTrade(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int id = qsInt(qs, "id");
    QTrade t{};
    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);

    if (id <= 0 || !qe_trade_get(e, id, &t))
    {
        h << "<h1>Edit Trade</h1><div class='msg err'>Trade not found</div>";
    }
    else
    {
        bool isBuy = (t.type == Q_BUY);
        h << "<h1>Edit Trade #" << t.tradeId << "</h1>"
             "<form class='card' method='POST' action='/edit-trade'>"
             "<input type='hidden' name='id' value='" << t.tradeId << "'>"
             "<label>Symbol</label><input type='text' name='symbol' value='" << escHtml(t.symbol) << "'><br>"
             "<label>Type</label><select name='type'>"
             "<option value='Buy'" << (isBuy ? " selected" : "") << ">Buy</option>"
             "<option value='CoveredSell'" << (!isBuy ? " selected" : "") << ">CoveredSell</option>"
             "</select><br>"
             "<label>Price</label><input type='number' name='price' step='any' value='" << t.value << "'><br>"
             "<label>Quantity</label><input type='number' name='quantity' step='any' value='" << t.quantity << "'><br>"
             "<label>Buy Fee</label><input type='number' name='buyFee' step='any' value='" << t.buyFees << "'><br>"
             "<label>Sell Fee</label><input type='number' name='sellFee' step='any' value='" << t.sellFees << "'><br>"
             "<button>Save Changes</button></form>";
    }
    h << "<br><a class='btn' href='/trades'>Back to Trades</a>";
    return makeHtml(wrapPage("Edit Trade", h.str()));
}

static QUIResponse postEditTrade(QEngine* e, const std::map<std::string, std::string>& form)
{
    int id = qsInt(form, "id");
    QTrade t{};
    if (!qe_trade_get(e, id, &t))
        return makeRedirect("/trades?err=" + urlEncode("Trade not found"));

    std::string sym = qsVal(form, "symbol");
    if (!sym.empty()) copySymbol(t.symbol, sizeof(t.symbol), sym);
    std::string typeStr = qsVal(form, "type");
    if (typeStr == "CoveredSell") t.type = Q_COVERED_SELL;
    else if (typeStr == "Buy") t.type = Q_BUY;
    double price = qsDbl(form, "price");
    if (price > 0) t.value = price;
    double qty = qsDbl(form, "quantity");
    if (qty > 0) t.quantity = qty;
    t.buyFees = qsDbl(form, "buyFee", t.buyFees);
    t.sellFees = qsDbl(form, "sellFee", t.sellFees);

    qe_trade_update(e, &t);
    return makeRedirect("/trades?msg=" + urlEncode("Trade #" + std::to_string(id) + " updated"));
}

// ---- Exit Point POST handlers ----

static QUIResponse postExitAdd(QEngine* e, const std::map<std::string, std::string>& form)
{
    int tradeId = qsInt(form, "tradeId");
    std::string sym = qsVal(form, "symbol");
    double tp  = qsDbl(form, "tp");
    double sl  = qsDbl(form, "sl");
    double qty = qsDbl(form, "qty");
    if (tp <= 0 || qty <= 0)
        return makeRedirect("/trades?err=" + urlEncode("TP and qty required"));

    QExitPointData xp{};
    xp.tradeId  = tradeId;
    copySymbol(xp.symbol, sizeof(xp.symbol), sym);
    xp.tpPrice  = tp;
    xp.slPrice  = sl;
    xp.sellQty  = qty;
    xp.slActive = (sl > 0) ? 1 : 0;
    qe_exitpt_add(e, &xp);
    return makeRedirect("/trades?msg=" + urlEncode("Exit X" + std::to_string(xp.exitId) + " added"));
}

static QUIResponse postExitDelete(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId = qsInt(form, "exitId");
    qe_exitpt_delete(e, exitId);
    return makeRedirect("/trades?msg=" + urlEncode("Exit X" + std::to_string(exitId) + " deleted"));
}

static QUIResponse postExitSetTp(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId = qsInt(form, "exitId");
    double tp  = qsDbl(form, "tp");
    // Load, update, save
    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId) { all[i].tpPrice = tp; qe_exitpt_update(e, &all[i]); break; }
    return makeRedirect("/trades?msg=" + urlEncode("Exit TP set for X" + std::to_string(exitId)));
}

static QUIResponse postExitSetSl(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId = qsInt(form, "exitId");
    double sl  = qsDbl(form, "sl");
    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId) { all[i].slPrice = sl; qe_exitpt_update(e, &all[i]); break; }
    return makeRedirect("/trades?msg=" + urlEncode("Exit SL set for X" + std::to_string(exitId)));
}

static QUIResponse postExitSetQty(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId = qsInt(form, "exitId");
    double qty = qsDbl(form, "qty");
    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId) { all[i].sellQty = qty; qe_exitpt_update(e, &all[i]); break; }
    return makeRedirect("/trades?msg=" + urlEncode("Exit qty set for X" + std::to_string(exitId)));
}

static QUIResponse postExitToggleSl(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId = qsInt(form, "exitId");
    int active = qsInt(form, "active");
    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId) { all[i].slActive = active; qe_exitpt_update(e, &all[i]); break; }
    std::string state = active ? "ON" : "OFF";
    return makeRedirect("/trades?msg=" + urlEncode("Exit SL " + state + " for X" + std::to_string(exitId)));
}

// ---- Profit ----

static QUIResponse pageProfit(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int tradeId = qsInt(qs, "tradeId");
    std::ostringstream h;
    h << std::fixed << std::setprecision(8);

    if (tradeId <= 0)
    {
        h << "<h1>Profit Calculator</h1>"
             "<form class='card' method='POST' action='/profit'>"
             "<label>Trade ID</label><input type='number' name='tradeId' required> "
             "<label>Current Price</label><input type='number' name='currentPrice' step='any' required><br>"
             "<button>Calculate</button></form>"
             "<br><a class='btn' href='/trades'>Back to Trades</a>";
    }
    else
    {
        QTrade t{};
        qe_trade_get(e, tradeId, &t);
        h << "<h1>Profit — Trade #" << tradeId << "</h1>"
             "<div class='row'>"
             "<div class='stat'><div class='lbl'>Symbol</div><div class='val'>" << escHtml(t.symbol) << "</div></div>"
             "<div class='stat'><div class='lbl'>Entry</div><div class='val'>" << t.value << "</div></div>"
             "<div class='stat'><div class='lbl'>Qty</div><div class='val'>" << t.quantity << "</div></div>"
             "</div>"
             "<form class='card' method='POST' action='/profit'>"
             "<input type='hidden' name='tradeId' value='" << tradeId << "'>"
             "<label>Current Price</label><input type='number' name='currentPrice' step='any' required placeholder='market price'> "
             "<button>Calculate</button></form>"
             "<br><a class='btn' href='/trades'>Back to Trades</a>";
    }
    return makeHtml(wrapPage("Profit", h.str()));
}

static QUIResponse postProfit(QEngine* e, const std::map<std::string, std::string>& form)
{
    int tradeId       = qsInt(form, "tradeId");
    double curPrice   = qsDbl(form, "currentPrice");
    if (tradeId <= 0)
        return makeRedirect("/profit?err=" + urlEncode("Trade ID required"));

    QTrade t{};
    qe_trade_get(e, tradeId, &t);
    QProfitResult pr = qe_profit_calculate(e, tradeId, curPrice);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << "<h1>P&amp;L — Trade #" << tradeId << " (" << escHtml(t.symbol) << ")</h1>"
         "<div class='row'>"
         "<div class='stat'><div class='lbl'>Entry</div><div class='val'>" << t.value << "</div></div>"
         "<div class='stat'><div class='lbl'>Qty</div><div class='val'>" << t.quantity << "</div></div>"
         "<div class='stat'><div class='lbl'>Current</div><div class='val'>" << curPrice << "</div></div>"
         "</div><br>"
         "<div class='row'>"
         "<div class='stat'><div class='lbl'>Gross P&amp;L</div><div class='val " << (pr.grossProfit >= 0 ? "buy" : "sell") << "'>" << pr.grossProfit << "</div></div>"
         "<div class='stat'><div class='lbl'>Net P&amp;L</div><div class='val " << (pr.netProfit >= 0 ? "buy" : "sell") << "'>" << pr.netProfit << "</div></div>"
         "<div class='stat'><div class='lbl'>ROI</div><div class='val " << (pr.roi >= 0 ? "buy" : "sell") << "'>" << pr.roi << "%</div></div>"
         "</div>"
         "<br><a class='btn' href='/profit?tradeId=" << tradeId << "'>Recalculate</a> "
         "<a class='btn' href='/trades'>Back to Trades</a>";
    return makeHtml(wrapPage("P&L Result", h.str()));
}

// ---- Entry Points ----

static QUIResponse pageEntryPoints(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int n = qe_entry_count(e);
    std::vector<QEntryPoint> entries(n > 0 ? n : 1);
    n = qe_entry_list(e, entries.data(), n);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Entry Points (" << n << ")</h1>";

    if (n == 0)
    {
        h << "<p style='color:#475569;'>No entry points. Use the "
             "<a href='/serial-generator' style='color:#38bdf8;'>Serial Generator</a> to create them.</p>";
    }
    else
    {
        h << "<table><tr><th>#</th><th>Symbol</th><th>Entry</th><th>Qty</th>"
             "<th>Funding</th><th>TP</th><th>SL</th><th>Status</th><th></th></tr>";
        for (int i = 0; i < n; ++i)
        {
            const auto& ep = entries[i];
            h << "<tr>"
              << "<td>" << ep.entryId << "</td>"
              << "<td>" << escHtml(ep.symbol) << "</td>"
              << "<td>" << ep.entryPrice << "</td>"
              << "<td>" << ep.fundingQty << "</td>"
              << "<td>" << ep.funding << "</td>"
              << "<td class='buy'>" << ep.exitTakeProfit << "</td>"
              << "<td class='sell'>" << ep.exitStopLoss << "</td>"
              << "<td>" << (ep.traded ? "<span class='buy'>FILLED</span>"
                                      : "<span style='color:#64748b;'>PENDING</span>") << "</td>"
              << "<td>";
            if (!ep.traded)
            {
                h << "<form method='POST' action='/price-check/fill-entry' style='margin:0;display:inline;'>"
                     "<input type='hidden' name='symbol' value='" << escHtml(ep.symbol) << "'>"
                     "<input type='hidden' name='entryId' value='" << ep.entryId << "'>"
                     "<input type='hidden' name='entryPrice' value='" << ep.entryPrice << "'>"
                     "<input type='hidden' name='qty' value='" << ep.fundingQty << "'>"
                     "<input type='hidden' name='price' value='0'>"
                     "<button class='btn' style='padding:2px 6px;font-size:0.72em;'>Fill</button></form> ";
            }
            h << "<form method='POST' action='/entry-points/delete' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='entryId' value='" << ep.entryId << "'>"
                 "<button class='btn-danger' style='padding:2px 6px;font-size:0.75em;'>Del</button></form>"
                 "</td>"
              << "</tr>";
        }
        h << "</table>";
    }

    h << "<br><a class='btn' href='/price-check' style='background:#1a1a00;color:#ffd600;border-color:#ffd600;'>&#9733; Price Check</a> "
         "<a class='btn' href='/dashboard'>Back</a>";
    return makeHtml(wrapPage("Entry Points", h.str()));
}

static QUIResponse postEntryDelete(QEngine* e, const std::map<std::string, std::string>& form)
{
    int id = qsInt(form, "entryId");
    if (id <= 0)
        return makeRedirect("/entry-points?err=" + urlEncode("Invalid entry ID"));
    qe_entry_delete(e, id);
    return makeRedirect("/entry-points?msg=" + urlEncode("Entry #" + std::to_string(id) + " deleted"));
}

// ---- Chains ----

static QUIResponse pageChains(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int n = qe_chain_count(e);
    std::vector<QManagedChain> chains(n > 0 ? n : 1);
    n = qe_chain_list(e, chains.data(), n);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Managed Chains (" << n << ")</h1>";

    if (n == 0)
    {
        h << "<p style='color:#475569;'>No chains yet.</p>";
    }
    else
    {
        h << "<table><tr><th>ID</th><th>Name</th><th>Symbol</th>"
             "<th>Capital</th><th>Cycle</th><th>Status</th><th>Savings</th></tr>";
        for (int i = 0; i < n; ++i)
        {
            const auto& c = chains[i];
            std::string status = c.theoretical ? "THEORETICAL"
                               : c.active ? "<span class='buy'>ACTIVE</span>"
                               : "<span style='color:#eab308;'>PAUSED</span>";
            h << "<tr>"
              << "<td><a href='/chains/" << c.chainId << "' style='color:#38bdf8;'>"
              << c.chainId << "</a></td>"
              << "<td>" << escHtml(c.name) << "</td>"
              << "<td>" << escHtml(c.symbol) << "</td>"
              << "<td>" << c.capital << "</td>"
              << "<td>" << c.currentCycle << "</td>"
              << "<td>" << status << "</td>"
              << "<td>" << c.totalSavings << "</td>"
              << "</tr>";
        }
        h << "</table>";
    }

    h << "<a class='btn' href='/chains/new'>+ New Chain</a> "
         "<a class='btn' href='/dashboard'>Back</a>";
    return makeHtml(wrapPage("Chains", h.str()));
}

static QUIResponse pageChainNew(QEngine* e, const std::map<std::string, std::string>& qs)
{
    std::ostringstream h;
    h << "<h1>Create Chain</h1>"
         "<form class='card' method='POST' action='/chains/new'>"
         "<label>Name</label><input type='text' name='name' required><br>"
         "<label>Symbol</label><input type='text' name='symbol' required><br>"
         "<label>Capital</label><input type='number' name='capital' step='any' required><br>"
         "<label>Savings Rate</label><input type='number' name='savingsRate' step='any' value='0.05'><br>"
         "<button>Create Chain</button></form>"
         "<br><a class='btn' href='/chains'>Back</a>";
    return makeHtml(wrapPage("New Chain", h.str()));
}

static QUIResponse postChainNew(QEngine* e, const std::map<std::string, std::string>& form)
{
    QManagedChain c{};
    auto name = qsVal(form, "name");
    auto symbol = qsVal(form, "symbol");
    size_t nlen = std::min(name.size(), sizeof(c.name) - 1);
    std::memcpy(c.name, name.c_str(), nlen); c.name[nlen] = '\0';
    size_t slen = std::min(symbol.size(), sizeof(c.symbol) - 1);
    std::memcpy(c.symbol, symbol.c_str(), slen); c.symbol[slen] = '\0';
    c.capital     = qsDbl(form, "capital");
    c.savingsRate = qsDbl(form, "savingsRate", 0.05);
    c.theoretical = 1;
    c.active      = 0;
    int id = qe_chain_create(e, &c);
    return makeRedirect("/chains?msg=Chain+" + std::to_string(id) + "+created");
}

static QUIResponse pageChainDetail(QEngine* e, int chainId, const std::map<std::string, std::string>& qs)
{
    QManagedChain c{};
    if (!qe_chain_get(e, chainId, &c))
        return makeRedirect("/chains?err=Chain+not+found");

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Chain #" << chainId << ": " << escHtml(c.name) << "</h1>";

    std::string status = c.theoretical ? "THEORETICAL"
                       : c.active ? "<span class='buy'>ACTIVE</span>"
                       : "<span style='color:#eab308;'>PAUSED</span>";

    h << "<div class='row'>"
         "<div class='stat'><div class='lbl'>Symbol</div><div class='val'>" << escHtml(c.symbol) << "</div></div>"
         "<div class='stat'><div class='lbl'>Capital</div><div class='val'>" << c.capital << "</div></div>"
         "<div class='stat'><div class='lbl'>Cycle</div><div class='val'>" << c.currentCycle << "</div></div>"
         "<div class='stat'><div class='lbl'>Status</div><div class='val'>" << status << "</div></div>"
         "<div class='stat'><div class='lbl'>Savings</div><div class='val'>" << c.totalSavings << "</div></div>"
         "</div>";

    h << "<div style='margin-top:16px;'>";
    if (c.theoretical)
        h << "<form method='POST' action='/chains/" << chainId << "/activate' style='display:inline;'>"
             "<button style='background:#16a34a;'>Activate</button></form> ";
    else if (c.active)
        h << "<form method='POST' action='/chains/" << chainId << "/deactivate' style='display:inline;'>"
             "<button style='background:#eab308;color:#000;'>Pause</button></form> ";
    else
        h << "<form method='POST' action='/chains/" << chainId << "/activate' style='display:inline;'>"
             "<button style='background:#16a34a;'>Reactivate</button></form> ";

    h << "<form method='POST' action='/chains/" << chainId << "/delete' style='display:inline;'"
         " onsubmit='return confirm(\"Delete chain?\")'>"
         "<button class='btn-danger'>Delete</button></form>";
    h << "</div>";

    h << "<br><a class='btn' href='/chains'>Back to Chains</a>";
    return makeHtml(wrapPage("Chain " + std::string(c.name), h.str()));
}

static QUIResponse postChainActivate(QEngine* e, int chainId)
{
    qe_chain_activate(e, chainId);
    return makeRedirect("/chains/" + std::to_string(chainId) + "?msg=Activated");
}

static QUIResponse postChainDeactivate(QEngine* e, int chainId)
{
    qe_chain_deactivate(e, chainId);
    return makeRedirect("/chains/" + std::to_string(chainId) + "?msg=Paused");
}

static QUIResponse postChainDelete(QEngine* e, int chainId)
{
    qe_chain_delete(e, chainId);
    return makeRedirect("/chains?msg=Chain+deleted");
}

// ---- Serial Generator ----

static QUIResponse pageSerialGen(QEngine* e, const std::map<std::string, std::string>& qs)
{
    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Serial Generator</h1>"
         "<form class='card' method='POST' action='/serial-generator'>"
         "<label>Symbol</label><input type='text' name='symbol' value='BTC'><br>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='quantity' step='any' value='1'><br>"
         "<label>Levels</label><input type='number' name='levels' value='4'><br>"
         "<label>Exit Levels</label><input type='number' name='exitLevels' value='0'><br>"
         "<label>Steepness</label><input type='number' name='steepness' step='any' value='4'><br>"
         "<label>Risk</label><input type='number' name='risk' step='any' value='0.5'><br>"
         "<label>Funds</label><input type='number' name='funds' step='any' required><br>"
         "<label>Fee Spread</label><input type='number' name='feeSpread' step='any' value='0.001'><br>"
         "<label>Fee Hedging</label><input type='number' name='feeHedging' step='any' value='1'><br>"
         "<label>Delta Time</label><input type='number' name='deltaTime' step='any' value='1'><br>"
         "<label>Symbol Count</label><input type='number' name='symbolCount' value='1'><br>"
         "<label>Coefficient K</label><input type='number' name='coefficientK' step='any' value='0'><br>"
         "<label>Surplus</label><input type='number' name='surplus' step='any' value='0.02'><br>"
         "<label>Max Risk</label><input type='number' name='maxRisk' step='any' value='0'><br>"
         "<label>Min Risk</label><input type='number' name='minRisk' step='any' value='0'><br>"
         "<label>Exit Risk</label><input type='number' name='exitRisk' step='any' value='0.5'><br>"
         "<label>Exit Fraction</label><input type='number' name='exitFraction' step='any' value='1'><br>"
         "<label>Exit Steep</label><input type='number' name='exitSteepness' step='any' value='4'><br>"
         "<label>Savings Rate</label><input type='number' name='savingsRate' step='any' value='0.05'><br>"
         "<button>Generate</button></form>"
         "<br><a class='btn' href='/dashboard'>Back</a>";
    return makeHtml(wrapPage("Serial Generator", h.str()));
}

static QUIResponse postSerialGen(QEngine* e, const std::map<std::string, std::string>& form)
{
    QSerialParams p{};
    p.currentPrice          = qsDbl(form, "price");
    p.quantity              = qsDbl(form, "quantity", 1.0);
    p.levels                = qsInt(form, "levels", 4);
    p.exitLevels            = qsInt(form, "exitLevels", 0);
    p.steepness             = qsDbl(form, "steepness", 4.0);
    p.risk                  = qsDbl(form, "risk", 0.5);
    p.availableFunds        = qsDbl(form, "funds");
    p.feeSpread             = qsDbl(form, "feeSpread", 0.001);
    p.feeHedgingCoefficient = qsDbl(form, "feeHedging", 1.0);
    p.deltaTime             = qsDbl(form, "deltaTime", 1.0);
    p.symbolCount           = qsInt(form, "symbolCount", 1);
    p.coefficientK          = qsDbl(form, "coefficientK");
    p.surplusRate           = qsDbl(form, "surplus", 0.02);
    p.maxRisk               = qsDbl(form, "maxRisk");
    p.minRisk               = qsDbl(form, "minRisk");
    p.exitRisk              = qsDbl(form, "exitRisk", 0.5);
    p.exitFraction          = qsDbl(form, "exitFraction", 1.0);
    p.exitSteepness         = qsDbl(form, "exitSteepness", 4.0);
    p.savingsRate           = qsDbl(form, "savingsRate", 0.05);

    std::vector<QSerialEntry> entries(p.levels > 0 ? p.levels : 1);
    QSerialPlanSummary summary = qe_serial_generate(&p, entries.data(), p.levels);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << "<h1>Serial Plan Results</h1>";

    h << "<div class='row'>"
         "<div class='stat'><div class='lbl'>Overhead</div><div class='val'>" << summary.overhead << "</div></div>"
         "<div class='stat'><div class='lbl'>Effective OH</div><div class='val'>" << summary.effectiveOH << "</div></div>"
         "<div class='stat'><div class='lbl'>Total Funding</div><div class='val'>" << summary.totalFunding << "</div></div>"
         "<div class='stat'><div class='lbl'>Total TP Gross</div><div class='val buy'>" << summary.totalTpGross << "</div></div>"
         "</div>";

    h << "<table><tr><th>#</th><th>Entry</th><th>Break Even</th><th>Discount%</th>"
         "<th>Funding</th><th>Qty</th><th>TP/unit</th><th>TP gross</th></tr>";
    for (int i = 0; i < summary.entryCount; ++i)
    {
        const auto& se = entries[i];
        h << "<tr>"
          << "<td>" << se.index << "</td>"
          << "<td>" << se.entryPrice << "</td>"
          << "<td>" << se.breakEven << "</td>"
          << "<td>" << se.discountPct << "%</td>"
          << "<td>" << se.funding << "</td>"
          << "<td>" << se.fundQty << "</td>"
          << "<td class='buy'>" << se.tpUnit << "</td>"
          << "<td class='buy'>" << se.tpGross << "</td>"
          << "</tr>";
    }
    h << "</table>";

    h << "<a class='btn' href='/serial-generator'>New Plan</a> "
         "<a class='btn' href='/dashboard'>Dashboard</a>";
    return makeHtml(wrapPage("Serial Plan", h.str()));
}

// ---- Pure math page ----

static QUIResponse pageOverhead(QEngine* e, const std::map<std::string, std::string>& qs)
{
    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << "<h1>Overhead Calculator</h1>"
         "<form class='card' method='POST' action='/overhead'>"
         "<label>Price</label><input type='number' name='price' step='any' required><br>"
         "<label>Quantity</label><input type='number' name='qty' step='any' value='1'><br>"
         "<label>Fee Spread</label><input type='number' name='feeSpread' step='any' value='0.001'><br>"
         "<label>Fee Hedging</label><input type='number' name='feeHedging' step='any' value='1'><br>"
         "<label>Delta Time</label><input type='number' name='deltaTime' step='any' value='1'><br>"
         "<label>Symbol Count</label><input type='number' name='symbolCount' value='1'><br>"
         "<label>Available Funds</label><input type='number' name='funds' step='any' required><br>"
         "<label>Coefficient K</label><input type='number' name='coeffK' step='any' value='0'><br>"
         "<label>Surplus</label><input type='number' name='surplus' step='any' value='0.02'><br>"
         "<button>Calculate</button></form>"
         "<br><a class='btn' href='/dashboard'>Back</a>";
    return makeHtml(wrapPage("Overhead", h.str()));
}

static QUIResponse postOverhead(QEngine* e, const std::map<std::string, std::string>& form)
{
    double price  = qsDbl(form, "price");
    double qty    = qsDbl(form, "qty", 1.0);
    double fs     = qsDbl(form, "feeSpread", 0.001);
    double fh     = qsDbl(form, "feeHedging", 1.0);
    double dt     = qsDbl(form, "deltaTime", 1.0);
    int    ns     = qsInt(form, "symbolCount", 1);
    double funds  = qsDbl(form, "funds");
    double K      = qsDbl(form, "coeffK");
    double s      = qsDbl(form, "surplus", 0.02);

    double oh = qe_overhead(price, qty, fs, fh, dt, ns, funds, K, 0);
    double eo = qe_effective_overhead(oh, s, fs, fh, dt);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << "<h1>Overhead Result</h1>"
         "<div class='row'>"
         "<div class='stat'><div class='lbl'>Raw Overhead</div><div class='val'>" << oh << "</div></div>"
         "<div class='stat'><div class='lbl'>Effective OH</div><div class='val'>" << eo << "</div></div>"
         "<div class='stat'><div class='lbl'>Break Even</div><div class='val'>" << (price * (1.0 + oh)) << "</div></div>"
         "<div class='stat'><div class='lbl'>TP (1 level)</div><div class='val buy'>" << (price * (1.0 + eo)) << "</div></div>"
         "</div>"
              "<br><a class='btn' href='/overhead'>Recalculate</a> "
                   "<a class='btn' href='/dashboard'>Dashboard</a>";
             return makeHtml(wrapPage("Overhead Result", h.str()));
         }

// ---- Exit Strategy ----

static QUIResponse pageExits(QEngine* e, const std::map<std::string, std::string>& qs)
{
    int nExits = qe_exitpt_count(e);
    std::vector<QExitPointData> exits(nExits > 0 ? nExits : 1);
    nExits = qe_exitpt_list(e, exits.data(), nExits);

    int nTrades = qe_trade_count(e);
    std::vector<QTrade> trades(nTrades > 0 ? nTrades : 1);
    nTrades = qe_trade_list(e, trades.data(), nTrades);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Exit Strategies</h1>";

    // Group exits by tradeId
    bool anyExits = false;
    for (int ti = 0; ti < nTrades; ++ti)
    {
        const auto& t = trades[ti];
        if (t.type != Q_BUY) continue;

        // Collect exits for this trade
        std::vector<QExitPointData> txExits;
        for (int xi = 0; xi < nExits; ++xi)
            if (exits[xi].tradeId == t.tradeId) txExits.push_back(exits[xi]);
        if (txExits.empty()) continue;
        anyExits = true;

        double sold = qe_trade_sold_qty(e, t.tradeId);
        double remaining = t.quantity - sold;

        h << "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:8px;margin:8px 0;'>"
             "<div style='display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;'>"
             "<div><strong style='color:#ffd600;'>" << escHtml(t.symbol) << "</strong>"
             " <span style='color:#757575;'>Trade #" << t.tradeId << "</span></div>"
             "<div style='font-size:0.78em;'>"
             "<span style='color:#757575;'>Entry:</span> " << t.value
             << " <span style='color:#757575;'>Qty:</span> " << t.quantity
             << " <span style='color:#757575;'>Rem:</span> <span class='" << (remaining > 0 ? "buy" : "sell") << "'>" << remaining << "</span>"
             "</div></div>";

        h << "<table style='margin:6px 0 4px 0;'><tr>"
             "<th>Lvl</th><th>TP Price</th><th>SL Price</th><th>Sell Qty</th>"
             "<th>Value</th><th>P&amp;L</th><th>SL</th><th>Status</th><th>Actions</th></tr>";

        for (const auto& xp : txExits)
        {
            double sellValue = xp.tpPrice * xp.sellQty;
            double entryCost = t.value * xp.sellQty;
            double pnl = sellValue - entryCost;

            h << "<tr>"
              << "<td>" << xp.levelIndex << "</td>"
              << "<td class='buy'>" << xp.tpPrice << "</td>"
              << "<td class='sell'>" << xp.slPrice << "</td>"
              << "<td>" << xp.sellQty << "</td>"
              << "<td>" << sellValue << "</td>"
              << "<td class='" << (pnl >= 0 ? "buy" : "sell") << "'>" << pnl << "</td>"
              << "<td>" << (xp.slActive ? "<span class='buy'>ON</span>" : "<span style='color:#757575;'>OFF</span>") << "</td>";

            if (xp.executed)
            {
                h << "<td><span style='color:#757575;'>DONE #" << xp.linkedSellId << "</span></td>"
                     "<td></td>";
            }
            else
            {
                h << "<td><span class='buy'>PENDING</span></td>"
                     "<td>"
                     "<form method='POST' action='/exit/execute' style='margin:0;display:inline;'>"
                     "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                     "<input type='hidden' name='tradeId' value='" << xp.tradeId << "'>"
                     "<input type='hidden' name='symbol' value='" << escHtml(t.symbol) << "'>"
                     "<input type='hidden' name='tpPrice' value='" << xp.tpPrice << "'>"
                     "<input type='hidden' name='sellQty' value='" << xp.sellQty << "'>"
                     "<button class='btn' style='padding:2px 6px;font-size:0.72em;'>Execute TP</button></form> "
                     "<form method='POST' action='/exit/execute-sl' style='margin:0;display:inline;'>"
                     "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                     "<input type='hidden' name='tradeId' value='" << xp.tradeId << "'>"
                     "<input type='hidden' name='symbol' value='" << escHtml(t.symbol) << "'>"
                     "<input type='hidden' name='slPrice' value='" << xp.slPrice << "'>"
                     "<input type='hidden' name='sellQty' value='" << xp.sellQty << "'>"
                     "<button class='btn-danger' style='padding:2px 6px;font-size:0.72em;'>Execute SL</button></form>"
                     "</td>";
            }
            h << "</tr>";
        }
        h << "</table></div>";
    }

    if (!anyExits)
    {
        h << "<p style='color:#757575;'>No exit strategies yet. Add exits from the "
             "<a href='/trades' style='color:#00e676;'>Trades</a> page.</p>";
    }

    h << "<br><a class='btn' href='/dashboard'>Dashboard</a> "
         "<a class='btn' href='/price-check'>Price Check</a>";
    return makeHtml(wrapPage("Exit Strategies", h.str()));
}

static QUIResponse postExitExecute(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId  = qsInt(form, "exitId");
    int tradeId = qsInt(form, "tradeId");
    std::string sym = qsVal(form, "symbol");
    double price = qsDbl(form, "tpPrice");
    double qty   = qsDbl(form, "sellQty");
    if (exitId <= 0 || qty <= 0 || price <= 0)
        return makeRedirect("/exits?err=" + urlEncode("Invalid exit parameters"));

    // Execute the sell through the wallet
    int sellId = qe_execute_sell(e, sym.c_str(), price, qty, 0);
    if (sellId < 0)
        return makeRedirect("/exits?err=" + urlEncode("Sell failed — insufficient holdings"));

    // Mark exit as executed
    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId)
        {
            all[i].executed = 1;
            all[i].linkedSellId = sellId;
            qe_exitpt_update(e, &all[i]);
            break;
        }

    std::string returnTo = qsVal(form, "returnTo");
    std::string msg = urlEncode("Exit X" + std::to_string(exitId)
        + " executed — Sell #" + std::to_string(sellId) + " @ " + std::to_string(price));
    if (!returnTo.empty())
        return makeRedirect(returnTo + "?msg=" + msg);
    return makeRedirect("/exits?msg=" + msg);
}

static QUIResponse postExitExecuteSl(QEngine* e, const std::map<std::string, std::string>& form)
{
    int exitId  = qsInt(form, "exitId");
    std::string sym = qsVal(form, "symbol");
    double price = qsDbl(form, "slPrice");
    double qty   = qsDbl(form, "sellQty");
    if (exitId <= 0 || qty <= 0 || price <= 0)
        return makeRedirect("/exits?err=" + urlEncode("Invalid SL parameters"));

    int sellId = qe_execute_sell(e, sym.c_str(), price, qty, 0);
    if (sellId < 0)
        return makeRedirect("/exits?err=" + urlEncode("SL sell failed — insufficient holdings"));

    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId)
        {
            all[i].executed = 1;
            all[i].linkedSellId = sellId;
            qe_exitpt_update(e, &all[i]);
            break;
        }

    std::string returnTo = qsVal(form, "returnTo");
    std::string msg = urlEncode("Stop-loss X" + std::to_string(exitId)
        + " executed — Sell #" + std::to_string(sellId) + " @ " + std::to_string(price));
    if (!returnTo.empty())
        return makeRedirect(returnTo + "?msg=" + msg);
    return makeRedirect("/exits?msg=" + msg);
}

// ---- Price Check ----

static QUIResponse postPriceCheck(QEngine* e, const std::map<std::string, std::string>& form);

static QUIResponse pagePriceCheck(QEngine* e, const std::map<std::string, std::string>& qs)
{
    // If symbol & price are in the query string (redirect back from action), auto-show results
    std::string qSym = qsVal(qs, "symbol");
    double qPrice    = qsDbl(qs, "price");
    if (!qSym.empty() && qPrice > 0)
    {
        std::map<std::string, std::string> fakeForm;
        fakeForm["symbol"] = qSym;
        fakeForm["price"]  = std::to_string(qPrice);
        // Carry msg/err banners through
        auto r = postPriceCheck(e, fakeForm);
        return r;
    }

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>Price Check</h1>"
         "<p style='color:#757575;font-size:0.78em;'>"
         "Enter a symbol and the current market price. The system shows which entries "
         "can be filled, which exits are triggered, and chain events available.</p>";

    h << "<form class='card' method='POST' action='/price-check'>"
         "<label>Symbol</label><input type='text' name='symbol' required style='width:120px;'> "
         "<label>Price</label><input type='number' name='price' step='any' required style='width:100px;'> "
         "<button>Check</button></form>";

    h << "<br><a class='btn' href='/dashboard'>Dashboard</a>";
    return makeHtml(wrapPage("Price Check", h.str()));
}

static QUIResponse postPriceCheck(QEngine* e, const std::map<std::string, std::string>& form)
{
    std::string sym = qsVal(form, "symbol");
    double price    = qsDbl(form, "price");
    if (sym.empty() || price <= 0)
        return makeRedirect("/price-check?err=" + urlEncode("Symbol and price required"));

    // Load data
    int nEntries = qe_entry_count(e);
    std::vector<QEntryPoint> entries(nEntries > 0 ? nEntries : 1);
    nEntries = qe_entry_list(e, entries.data(), nEntries);

    int nExits = qe_exitpt_count(e);
    std::vector<QExitPointData> exits(nExits > 0 ? nExits : 1);
    nExits = qe_exitpt_list(e, exits.data(), nExits);

    int nTrades = qe_trade_count(e);
    std::vector<QTrade> trades(nTrades > 0 ? nTrades : 1);
    nTrades = qe_trade_list(e, trades.data(), nTrades);

    int nChains = qe_chain_count(e);
    std::vector<QManagedChain> chains(nChains > 0 ? nChains : 1);
    nChains = qe_chain_list(e, chains.data(), nChains);

    QWalletInfo w = qe_wallet_info(e);

    std::ostringstream h;
    h << std::fixed << std::setprecision(8);

    h << "<h1>Price Check: " << escHtml(sym) << " @ " << price << "</h1>"
         "<div class='row' style='margin-bottom:10px;'>"
         "<div class='stat'><div class='lbl'>Symbol</div><div class='val' style='font-size:0.9em;'>" << escHtml(sym) << "</div></div>"
         "<div class='stat'><div class='lbl'>Price</div><div class='val' style='font-size:0.9em;'>" << price << "</div></div>"
         "<div class='stat'><div class='lbl'>Wallet</div><div class='val' style='font-size:0.9em;'>" << w.balance << "</div></div>"
         "</div>";

    // ---- Section 1: Entry Points at or below price (fillable) ----
    h << "<h2>&#9654; Fillable Entries</h2>";
    bool anyFillable = false;
    for (int i = 0; i < nEntries; ++i)
    {
        const auto& ep = entries[i];
        if (ep.traded) continue;
        if (std::string(ep.symbol) != sym) continue;
        // For a long entry: fillable if price <= entryPrice
        // For a short: fillable if price >= entryPrice
        bool fillable = ep.isShort ? (price >= ep.entryPrice) : (price <= ep.entryPrice);
        if (!fillable) continue;
        anyFillable = true;

        double cost = ep.entryPrice * ep.fundingQty;
        bool canAfford = (cost <= w.balance);

        h << "<div style='background:#0a0a0a;border:1px solid " << (canAfford ? "#00e676" : "#ff1744")
          << ";border-radius:4px;padding:8px;margin:4px 0;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;'>"
             "<div>"
             "<strong style='color:#ffd600;'>Entry #" << ep.entryId << "</strong>"
             " <span style='color:#757575;'>Lvl " << ep.levelIndex << "</span>"
             " <span style='color:#757575;'>@</span> " << ep.entryPrice
          << " <span style='color:#757575;'>qty:</span> " << ep.fundingQty
          << " <span style='color:#757575;'>cost:</span> " << cost;
        if (ep.exitTakeProfit > 0)
            h << " <span style='color:#757575;'>TP:</span> <span class='buy'>" << ep.exitTakeProfit << "</span>";
        if (ep.exitStopLoss > 0)
            h << " <span style='color:#757575;'>SL:</span> <span class='sell'>" << ep.exitStopLoss << "</span>";
        h << "</div><div>";
        if (canAfford)
        {
            h << "<form method='POST' action='/price-check/fill-entry' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='symbol' value='" << escHtml(sym) << "'>"
                 "<input type='hidden' name='price' value='" << price << "'>"
                 "<input type='hidden' name='entryId' value='" << ep.entryId << "'>"
                 "<input type='hidden' name='entryPrice' value='" << ep.entryPrice << "'>"
                 "<input type='hidden' name='qty' value='" << ep.fundingQty << "'>"
                 "<button class='btn' style='padding:3px 8px;font-size:0.78em;'>&#9654; Execute Buy</button></form>";
        }
        else
        {
            h << "<span class='sell' style='font-size:0.75em;'>Insufficient funds</span>";
        }
        h << "</div></div>";
    }
    if (!anyFillable)
        h << "<p style='color:#757575;font-size:0.8em;'>No entries fillable at this price.</p>";

    // ---- Section 2: Triggered Exits (TP or SL hit) ----
    h << "<h2>&#9650; Triggered Exits</h2>";
    bool anyTriggered = false;
    for (int xi = 0; xi < nExits; ++xi)
    {
        const auto& xp = exits[xi];
        if (xp.executed) continue;
        if (std::string(xp.symbol) != sym) continue;

        bool tpHit = (price >= xp.tpPrice && xp.tpPrice > 0);
        bool slHit = (xp.slActive && xp.slPrice > 0 && price <= xp.slPrice);
        if (!tpHit && !slHit) continue;
        anyTriggered = true;

        // Find parent trade
        QTrade parent{};
        for (int ti = 0; ti < nTrades; ++ti)
            if (trades[ti].tradeId == xp.tradeId) { parent = trades[ti]; break; }

        double pnl = (price - parent.value) * xp.sellQty;

        h << "<div style='background:#0a0a0a;border:1px solid " << (tpHit ? "#00e676" : "#ff1744")
          << ";border-radius:4px;padding:8px;margin:4px 0;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;'>"
             "<div>"
             "<strong style='color:" << (tpHit ? "#00e676" : "#ff1744") << ";'>"
          << (tpHit ? "&#9650; TP HIT" : "&#9660; SL HIT") << "</strong>"
             " <span style='color:#757575;'>Exit X" << xp.exitId << "</span>"
             " <span style='color:#757575;'>Trade #" << xp.tradeId << "</span>"
             " <span style='color:#757575;'>qty:</span> " << xp.sellQty
          << " <span style='color:#757575;'>P&amp;L:</span> "
          << "<span class='" << (pnl >= 0 ? "buy" : "sell") << "'>" << pnl << "</span>"
          << "</div><div>";

        if (tpHit)
        {
            h << "<form method='POST' action='/exit/execute' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                 "<input type='hidden' name='tradeId' value='" << xp.tradeId << "'>"
                 "<input type='hidden' name='symbol' value='" << escHtml(sym) << "'>"
                 "<input type='hidden' name='tpPrice' value='" << price << "'>"
                 "<input type='hidden' name='sellQty' value='" << xp.sellQty << "'>"
                 "<input type='hidden' name='returnTo' value='/price-check'>"
                 "<button class='btn' style='padding:3px 8px;font-size:0.78em;'>Sell @ TP</button></form>";
        }
        if (slHit)
        {
            h << "<form method='POST' action='/exit/execute-sl' style='margin:0;display:inline;'>"
                 "<input type='hidden' name='exitId' value='" << xp.exitId << "'>"
                 "<input type='hidden' name='tradeId' value='" << xp.tradeId << "'>"
                 "<input type='hidden' name='symbol' value='" << escHtml(sym) << "'>"
                 "<input type='hidden' name='slPrice' value='" << price << "'>"
                 "<input type='hidden' name='sellQty' value='" << xp.sellQty << "'>"
                 "<input type='hidden' name='returnTo' value='/price-check'>"
                 "<button class='btn-danger' style='padding:3px 8px;font-size:0.78em;'>Sell @ SL</button></form>";
        }
        h << "</div></div>";
    }
    if (!anyTriggered)
        h << "<p style='color:#757575;font-size:0.8em;'>No exits triggered at this price.</p>";

    // ---- Section 3: Open Positions P&L ----
    h << "<h2>&#9733; Open Positions</h2>";
    bool anyPositions = false;
    for (int ti = 0; ti < nTrades; ++ti)
    {
        const auto& t = trades[ti];
        if (t.type != Q_BUY) continue;
        if (std::string(t.symbol) != sym) continue;
        double sold = qe_trade_sold_qty(e, t.tradeId);
        double remaining = t.quantity - sold;
        if (remaining <= 0) continue;
        anyPositions = true;

        double pnl = (price - t.value) * remaining;
        double roi = (t.value > 0) ? ((price - t.value) / t.value * 100.0) : 0;

        h << "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:6px 8px;margin:3px 0;"
             "display:flex;justify-content:space-between;flex-wrap:wrap;font-size:0.82em;'>"
             "<div>"
             "<span style='color:#757575;'>#" << t.tradeId << "</span>"
             " <span style='color:#ffd600;'>" << t.value << "</span>"
             " <span style='color:#757575;'>x</span> " << remaining
          << "</div><div>"
             "P&amp;L: <span class='" << (pnl >= 0 ? "buy" : "sell") << "'>" << pnl << "</span>"
             " ROI: <span class='" << (roi >= 0 ? "buy" : "sell") << "'>" << roi << "%</span>"
             "</div></div>";
    }
    if (!anyPositions)
        h << "<p style='color:#757575;font-size:0.8em;'>No open positions for " << escHtml(sym) << ".</p>";

    // ---- Section 4: Chain Events ----
    h << "<h2>&#9939; Chain Events</h2>";
    bool anyChainEvents = false;
    for (int ci = 0; ci < nChains; ++ci)
    {
        const auto& ch = chains[ci];
        if (!ch.active) continue;
        if (std::string(ch.symbol) != sym) continue;
        anyChainEvents = true;

        h << "<div style='background:#0a0a0a;border:1px solid #f472b6;border-radius:4px;padding:8px;margin:4px 0;'>"
             "<div style='display:flex;justify-content:space-between;flex-wrap:wrap;'>"
             "<div><strong style='color:#f472b6;'>Chain #" << ch.chainId << ": " << escHtml(ch.name) << "</strong>"
             " <span style='color:#757575;'>Cycle " << ch.currentCycle << "</span></div>"
             "<div style='font-size:0.78em;'>"
             "<span style='color:#757575;'>Capital:</span> " << ch.capital
          << " <span style='color:#757575;'>Savings:</span> " << ch.totalSavings
          << "</div></div>";

        // Show chain entries for current cycle that are fillable
        // We need to get chain entry IDs — through the C API we load chain details
        // For now, scan entries that match this symbol and are not traded
        h << "<div style='margin-top:6px;font-size:0.78em;'>"
             "<span style='color:#757575;'>Pending entries for this chain symbol at current price:</span></div>";

        bool anyChainEntries = false;
        for (int ei = 0; ei < nEntries; ++ei)
        {
            const auto& ep = entries[ei];
            if (ep.traded) continue;
            if (std::string(ep.symbol) != sym) continue;
            bool fillable = ep.isShort ? (price >= ep.entryPrice) : (price <= ep.entryPrice);
            if (!fillable) continue;
            anyChainEntries = true;

            double cost = ep.entryPrice * ep.fundingQty;
            bool canAfford = (cost <= w.balance);

            h << "<div style='margin:2px 0;padding:4px 8px;border-left:2px solid #f472b6;font-size:0.82em;'>"
                 "Entry #" << ep.entryId << " @ " << ep.entryPrice << " qty:" << ep.fundingQty;
            if (canAfford)
            {
                h << " <form method='POST' action='/price-check/fill-entry' style='margin:0;display:inline;'>"
                     "<input type='hidden' name='symbol' value='" << escHtml(sym) << "'>"
                     "<input type='hidden' name='price' value='" << price << "'>"
                     "<input type='hidden' name='entryId' value='" << ep.entryId << "'>"
                     "<input type='hidden' name='entryPrice' value='" << ep.entryPrice << "'>"
                     "<input type='hidden' name='qty' value='" << ep.fundingQty << "'>"
                     "<input type='hidden' name='chainId' value='" << ch.chainId << "'>"
                     "<button class='btn' style='padding:2px 6px;font-size:0.72em;'>Fill for Chain</button></form>";
            }
            else
            {
                h << " <span class='sell' style='font-size:0.78em;'>insufficient funds</span>";
            }
            h << "</div>";
        }
        if (!anyChainEntries)
            h << "<div style='color:#757575;font-size:0.78em;margin:4px 0;'>No fillable entries at this price.</div>";

        h << "</div>";
    }
    if (!anyChainEvents)
        h << "<p style='color:#757575;font-size:0.8em;'>No active chains for " << escHtml(sym) << ".</p>";

    // ---- Recheck form ----
    h << "<br><form class='card' method='POST' action='/price-check' style='display:inline;'>"
         "<input type='hidden' name='symbol' value='" << escHtml(sym) << "'>"
         "<label>New Price</label><input type='number' name='price' step='any' value='" << price << "' required> "
         "<button>Recheck</button></form>"
         "<br><br><a class='btn' href='/price-check'>Reset</a> "
         "<a class='btn' href='/exits'>Exits</a> "
         "<a class='btn' href='/dashboard'>Dashboard</a>";

    return makeHtml(wrapPage("Price Check: " + sym, h.str()));
}

static QUIResponse postFillEntry(QEngine* e, const std::map<std::string, std::string>& form)
{
    std::string sym = qsVal(form, "symbol");
    double price    = qsDbl(form, "price");
    int entryId     = qsInt(form, "entryId");
    double entryPr  = qsDbl(form, "entryPrice");
    double qty      = qsDbl(form, "qty");
    int chainId     = qsInt(form, "chainId"); // optional

    if (sym.empty() || entryPr <= 0 || qty <= 0 || entryId <= 0)
        return makeRedirect("/price-check?err=" + urlEncode("Invalid entry parameters"));

    // Execute buy at the entry price
    int tradeId = qe_execute_buy(e, sym.c_str(), entryPr, qty, 0);
    if (tradeId < 0)
        return makeRedirect("/price-check?err=" + urlEncode("Buy failed — insufficient wallet balance"));

    // Mark the entry point as traded
    QEntryPoint ep{};
    if (qe_entry_get(e, entryId, &ep))
    {
        ep.traded = 1;
        ep.linkedTradeId = tradeId;
        qe_entry_update(e, &ep);

        // Auto-create exit points from entry's TP/SL
        if (ep.exitTakeProfit > 0)
        {
            QExitPointData xp{};
            xp.tradeId = tradeId;
            copySymbol(xp.symbol, sizeof(xp.symbol), sym);
            xp.tpPrice  = ep.exitTakeProfit;
            xp.slPrice  = ep.exitStopLoss;
            xp.sellQty  = qty;
            xp.slActive = (ep.exitStopLoss > 0) ? 1 : 0;
            qe_exitpt_add(e, &xp);
        }
    }

    std::string msg = "Entry #" + std::to_string(entryId) + " filled — Trade #" + std::to_string(tradeId);

    // If price was given, redirect back to price check with context
    if (price > 0)
    {
        return makeRedirect("/price-check?msg=" + urlEncode(msg)
            + "&symbol=" + urlEncode(sym) + "&price=" + std::to_string(price));
    }
    return makeRedirect("/entry-points?msg=" + urlEncode(msg));
}

// ---- MCP Daemon Info ----

static QUIResponse pageMcpDaemon(QEngine* /*e*/, const std::map<std::string, std::string>& qs)
{
    std::ostringstream h;
    h << msgBanner(qs) << errBanner(qs);
    h << "<h1>&#9889; MCP Daemon</h1>";

    h << "<div class='row' style='margin-bottom:10px;'>"
         "<div class='stat'><div class='lbl'>Status</div>"
         "<div class='val' style='font-size:0.9em;'>Running</div></div>"
         "<div class='stat'><div class='lbl'>Port</div>"
         "<div class='val' style='font-size:0.9em;'>9876</div></div>"
         "<div class='stat'><div class='lbl'>Transport</div>"
         "<div class='val' style='font-size:0.9em;color:#757575;'>HTTP</div></div>"
         "</div>";

    h << "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:10px 0;'>"
         "<h2 style='margin-top:0;'>Connect your AI</h2>"
         "<p style='font-size:0.82em;line-height:1.6;margin:0 0 8px 0;'>"
         "The MCP daemon is running on this device. AI assistants (Claude, "
         "Copilot, etc.) can connect using the <strong style='color:#ffd600;'>"
         "MCP Streamable HTTP</strong> transport.</p>"
         "<p style='font-size:0.82em;margin:0 0 8px 0;'>Endpoint:</p>"
         "<code style='display:block;background:#050505;padding:6px 10px;border-radius:3px;"
         "color:#ffd600;font-size:0.82em;word-break:break-all;'>"
         "POST http://&lt;device-ip&gt;:9876/mcp</code>"
         "<p style='font-size:0.75em;color:#757575;margin:8px 0 0 0;'>"
         "Find your device IP in Wi-Fi settings, or open "
         "<code style='color:#ffd600;'>http://&lt;device-ip&gt;:9876/</code> "
         "in a browser to verify.</p></div>";

    h << "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:10px 0;'>"
         "<h2 style='margin-top:0;'>Claude Desktop</h2>"
         "<p style='font-size:0.78em;color:#757575;margin:0 0 6px 0;'>"
         "Add to <code>claude_desktop_config.json</code>:</p>"
         "<pre style='background:#050505;padding:8px;border-radius:3px;color:#c0c0c0;"
         "font-size:0.72em;overflow-x:auto;margin:0;'>"
         "{\n"
         "  \"mcpServers\": {\n"
         "    \"quant\": {\n"
         "      \"url\": \"http://&lt;device-ip&gt;:9876/mcp\"\n"
         "    }\n"
         "  }\n"
         "}</pre></div>";

    h << "<div style='background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:12px;margin:10px 0;'>"
         "<h2 style='margin-top:0;'>Available Tools</h2>"
         "<p style='font-size:0.78em;color:#757575;margin:0 0 6px 0;'>"
         "The MCP server exposes 22 tools:</p>"
         "<div style='font-size:0.75em;line-height:1.8;'>"
         "<span style='color:#00e676;'>&#9654;</span> portfolio_status, list_trades, list_entries, list_exits, list_chains<br>"
         "<span style='color:#ffd600;'>&#9733;</span> price_check, calculate_profit, position_risk_summary<br>"
         "<span style='color:#38bdf8;'>&#9881;</span> generate_serial_plan, compute_overhead, what_if, compare_plans<br>"
         "<span style='color:#f472b6;'>&#9939;</span> list_presets, save_preset, delete_preset<br>"
         "<span style='color:#ff1744;'>&#9650;</span> execute_buy, execute_sell, fill_entry, execute_exit, confirm_execution<br>"
         "<span style='color:#c0c0c0;'>&#9783;</span> wallet_deposit, wallet_withdraw"
         "</div></div>";

    return makeHtml(wrapPage("MCP Daemon", h.str()));
}

#ifndef QUANT_PUBLIC_BUILD
         // ---- Sync Settings ----

         static QUIResponse pageSync(QEngine* /*e*/, const std::map<std::string, std::string>& qs)
         {
             std::ostringstream h;
             h << msgBanner(qs) << errBanner(qs);
             h << "<h1>&#9939; Cloud Sync</h1>";

             // Mode selector
             h << "<div class='row' style='margin-bottom:10px;'>"
                  "<div class='stat'><div class='lbl'>Mode</div>"
                  "<div class='val' style='font-size:0.9em;'>Offline</div></div>"
                  "<div class='stat'><div class='lbl'>Last Sync</div>"
                  "<div class='val' style='font-size:0.9em;color:#757575;'>Never</div></div>"
                  "<div class='stat'><div class='lbl'>Vault</div>"
                  "<div class='val' style='font-size:0.9em;color:#757575;'>Not set</div></div>"
                  "</div>";

             // Server connection
             h << "<form class='card' method='POST' action='/sync/connect'>"
                  "<h2 style='margin-top:0;'>Server Connection</h2>"
                  "<label>Server URL</label><input type='text' name='serverUrl' style='width:200px;' "
                  "placeholder='https://your-server:9443'><br>"
                  "<label>Username</label><input type='text' name='username' placeholder='username'><br>"
                  "<label>Password</label><input type='password' name='password' placeholder='password' "
                  "style='background:#050505;border:1px solid #1a1a1a;color:#c0c0c0;padding:3px 5px;"
                  "border-radius:3px;font-family:inherit;font-size:0.8em;'><br>"
                  "<button>Connect</button></form>";

             // Vault password
             h << "<form class='card' method='POST' action='/sync/vault-password'>"
                  "<h2 style='margin-top:0;'>Vault Encryption</h2>"
                  "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
                  "Your database is encrypted client-side with this password.<br>"
                  "The server never sees your data. If you lose this password, your vault cannot be recovered.</div>"
                  "<label>Vault Password</label><input type='password' name='vaultPassword' "
                  "style='background:#050505;border:1px solid #1a1a1a;color:#c0c0c0;padding:3px 5px;"
                  "border-radius:3px;font-family:inherit;font-size:0.8em;width:150px;' placeholder='vault password'><br>"
                  "<button>Set Vault Password</button></form>";

             // Sync actions
             h << "<div class='forms-row'>"
                  "<form class='card' method='POST' action='/sync/upload'>"
                  "<h2 style='margin-top:0;'>Upload</h2>"
                  "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
                  "Encrypt &amp; upload your database to the server.</div>"
                  "<button>&#9650; Upload Now</button></form>"
                  "<form class='card' method='POST' action='/sync/download'>"
                  "<h2 style='margin-top:0;'>Download</h2>"
                  "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
                  "Download &amp; decrypt your database from the server.</div>"
                  "<button>&#9660; Download Now</button></form>"
                  "</div>";

             // Support ticket
             h << "<form class='card' method='POST' action='/sync/ticket'>"
                  "<h2 style='margin-top:0;'>Request More Storage</h2>"
                  "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
                  "Create a support ticket to request additional storage. Payment in Bitcoin.</div>"
                  "<label>Subject</label><input type='text' name='subject' style='width:200px;' "
                  "placeholder='Request 100MB storage'><br>"
                  "<label>Message</label><input type='text' name='message' style='width:200px;' "
                  "placeholder='Details...'><br>"
                  "<button>Create Ticket</button></form>";

             return makeHtml(wrapPage("Cloud Sync", h.str()));
         }
#endif // QUANT_PUBLIC_BUILD

         // ============================================================
         // Router
// ============================================================

// Parse path like "/chains/5" into base="/chains" and id=5
static bool parsePathId(const std::string& path, const std::string& prefix, int& id)
{
    if (path.size() <= prefix.size() + 1) return false;
    if (path.compare(0, prefix.size(), prefix) != 0) return false;
    if (path[prefix.size()] != '/') return false;
    std::string rest = path.substr(prefix.size() + 1);
    // Extract numeric ID (stop at next '/')
    auto slash = rest.find('/');
    std::string idStr = (slash != std::string::npos) ? rest.substr(0, slash) : rest;
    try { id = std::stoi(idStr); return true; }
    catch (...) { return false; }
}

static std::string pathSuffix(const std::string& path, const std::string& prefix, int id)
{
    std::string idPart = prefix + "/" + std::to_string(id);
    if (path.size() <= idPart.size()) return "";
    return path.substr(idPart.size());
}

extern "C" {

QUIResponse qui_get(QEngine* e, const char* path, const char* queryString)
{
    if (!e || !path) return make404(path ? path : "");
    std::string p = path;
    auto qs = parseQS(queryString);
    int id = 0;

    if (p == "/" || p.empty())               return pageHome(e, qs);
    if (p == "/dashboard")                   return pageDashboard(e, qs);
    if (p == "/wallet")                      return pageWallet(e, qs);
    if (p == "/trades")                      return pageTrades(e, qs);
    if (p == "/edit-trade")                  return pageEditTrade(e, qs);
    if (p == "/profit")                      return pageProfit(e, qs);
    if (p == "/entry-points")                return pageEntryPoints(e, qs);
    if (p == "/chains")                      return pageChains(e, qs);
    if (p == "/chains/new")                  return pageChainNew(e, qs);
    if (p == "/serial-generator")            return pageSerialGen(e, qs);
    if (p == "/overhead")                    return pageOverhead(e, qs);
    if (p == "/exits")                       return pageExits(e, qs);
    if (p == "/price-check")                 return pagePriceCheck(e, qs);
    if (p == "/mcp-daemon")                  return pageMcpDaemon(e, qs);
#ifndef QUANT_PUBLIC_BUILD
    if (p == "/sync")                        return pageSync(e, qs);
#endif

    if (parsePathId(p, "/chains", id))
    {
        std::string suffix = pathSuffix(p, "/chains", id);
        if (suffix.empty() || suffix == "/") return pageChainDetail(e, id, qs);
    }

    return make404(p);
}

QUIResponse qui_post(QEngine* e, const char* path, const char* formBody)
{
    if (!e || !path) return make404(path ? path : "");
    std::string p = path;
    auto form = parseQS(formBody);
    int id = 0;

    if (p == "/wallet/deposit")              return postWalletDeposit(e, form);
    if (p == "/wallet/withdraw")             return postWalletWithdraw(e, form);
    if (p == "/trades/add")                  return postTradeAdd(e, form);
    if (p == "/trades/delete")               return postTradeDelete(e, form);
    if (p == "/execute-buy")                 return postExecuteBuy(e, form);
    if (p == "/execute-sell")                return postExecuteSell(e, form);
    if (p == "/edit-trade")                  return postEditTrade(e, form);
    if (p == "/profit")                      return postProfit(e, form);
    if (p == "/exit/add")                    return postExitAdd(e, form);
    if (p == "/exit/delete")                 return postExitDelete(e, form);
    if (p == "/exit/set-tp")                 return postExitSetTp(e, form);
    if (p == "/exit/set-sl")                 return postExitSetSl(e, form);
    if (p == "/exit/set-qty")                return postExitSetQty(e, form);
    if (p == "/exit/toggle-sl")              return postExitToggleSl(e, form);
    if (p == "/exit/execute")                return postExitExecute(e, form);
    if (p == "/exit/execute-sl")             return postExitExecuteSl(e, form);
    if (p == "/price-check")                 return postPriceCheck(e, form);
    if (p == "/price-check/fill-entry")      return postFillEntry(e, form);
    if (p == "/entry-points/delete")         return postEntryDelete(e, form);
    if (p == "/chains/new")                  return postChainNew(e, form);
    if (p == "/serial-generator")            return postSerialGen(e, form);
    if (p == "/overhead")                    return postOverhead(e, form);

    if (parsePathId(p, "/chains", id))
    {
        std::string suffix = pathSuffix(p, "/chains", id);
        if (suffix == "/activate")           return postChainActivate(e, id);
        if (suffix == "/deactivate")         return postChainDeactivate(e, id);
        if (suffix == "/delete")             return postChainDelete(e, id);
    }

    return make404(p);
}

void qui_free_response(QUIResponse* r)
{
    if (r && r->html) { std::free(r->html); r->html = nullptr; }
}

void qui_set_app_name(const char* name)
{
    g_appName = name ? name : "Quant";
}

void qui_set_nav(const char* navSpec)
{
    g_navOverride = navSpec ? navSpec : "";
}

} // extern "C"
